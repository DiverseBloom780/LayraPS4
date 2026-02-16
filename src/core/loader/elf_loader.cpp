// src/core/loader/elf_loader.cpp
#include "elf_loader.h"
#include "core/kernel/module_manager.h"
#include "core/memory/memory_manager.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <vector>

namespace Core {
namespace Loader {

// Default constructor (ADDED - this was missing!)
ElfLoader::ElfLoader() : memory(nullptr), module_manager(nullptr) {
  printf("[ElfLoader] Default constructor called\n");
}

// Constructor requires managers (matches header)
ElfLoader::ElfLoader(Memory::MemoryManager *memoryManager,
                     Kernel::ModuleManager *moduleManager)
    : memory(memoryManager), module_manager(moduleManager) {
  printf("[ElfLoader] Parameterized constructor called\n");
}

ElfLoader::~ElfLoader() { printf("[ElfLoader] Destructor called\n"); }

ElfLoader::LoadResult ElfLoader::Load(const std::string &path) {
  std::ifstream file(path, std::ios::binary | std::ios::ate);
  if (!file.is_open()) {
    return {false, 0, 0, 0, "Failed to open ELF file: " + path};
  }

  std::streamsize size = file.tellg();
  file.seekg(0, std::ios::beg);

  if (size <= 0) {
    return {false, 0, 0, 0, "ELF file empty or error reading size: " + path};
  }

  std::vector<uint8_t> data(static_cast<size_t>(size));
  if (!file.read(reinterpret_cast<char *>(data.data()), size)) {
    return {false, 0, 0, 0, "Failed to read ELF file"};
  }

  Elf64_Ehdr ehdr;
  std::vector<Elf64_Phdr> phdrs;

  if (!ParseHeaders(data, ehdr, phdrs)) {
    return {false, 0, 0, 0, "Failed to parse ELF headers"};
  }

  // Calculate load base for relocatable ELFs (ET_DYN)
  uint64_t load_base = 0;
  if (ehdr.e_type == 3) { // ET_DYN
    // Determine the total size needed for the image
    // Find min and max vaddr
    uint64_t min_vaddr = UINT64_MAX;
    uint64_t max_vaddr = 0;

    for (const auto &phdr : phdrs) {
      if (phdr.p_type == PT_LOAD) {
        if (phdr.p_vaddr < min_vaddr)
          min_vaddr = phdr.p_vaddr;
        if (phdr.p_vaddr + phdr.p_memsz > max_vaddr)
          max_vaddr = phdr.p_vaddr + phdr.p_memsz;
      }
    }

    // uint64_t image_size = max_vaddr - min_vaddr; // This is calculated later

    // Find a free region for this image
    // For now, simpler: Use a fixed base for the main executable if possible,
    // or allocate via MemoryManager Just hardcoding 0x400000 for main
    // executable if it's relocatable is a good PS4 convention
    load_base = 0x400000;
    printf("[Loader] Detected ET_DYN. Using load base: 0x%llx\n", load_base);
  } else {
    printf("[Loader] Detected ET_EXEC. Using absolute addresses (base 0)\n");
  }

  if (!MapSegments(data, phdrs, load_base)) {
    return {false, 0, 0, 0, "Failed to map segments"};
  }

  if (!ApplyRelocations(data, ehdr, phdrs, load_base)) {
    return {false, 0, 0, 0, "Failed to apply relocations"};
  }
  HandleImports(data, ehdr, phdrs);

  // Calculate total size
  uint64_t min_vaddr = UINT64_MAX;
  uint64_t max_vaddr = 0;

  for (const auto &phdr : phdrs) {
    if (phdr.p_type == PT_LOAD) {
      if (phdr.p_vaddr < min_vaddr)
        min_vaddr = phdr.p_vaddr;
      if (phdr.p_vaddr + phdr.p_memsz > max_vaddr)
        max_vaddr = phdr.p_vaddr + phdr.p_memsz;
    }
  }

  printf("[Loader] ELF loaded successfully. Entry point: 0x%llx\n",
         load_base + ehdr.e_entry);
  return {true, load_base + ehdr.e_entry, load_base, max_vaddr - min_vaddr, ""};
}

bool ElfLoader::ParseHeaders(const std::vector<uint8_t> &data, Elf64_Ehdr &ehdr,
                             std::vector<Elf64_Phdr> &phdrs) {
  if (data.size() < sizeof(Elf64_Ehdr))
    return false;

  std::memcpy(&ehdr, data.data(), sizeof(Elf64_Ehdr));

  if (std::memcmp(ehdr.e_ident, "\x7f\x45\x4c\x46", 4) != 0) {
    std::cerr << "[Loader] Invalid ELF magic.\n";
    return false;
  }

  if (ehdr.e_ident[4] != 2) { // 64-bit
    std::cerr << "[Loader] Not a 64-bit ELF.\n";
    return false;
  }

  if (ehdr.e_phnum == 0 || ehdr.e_phentsize < sizeof(Elf64_Phdr)) {
    std::cerr << "[Loader] No program headers or unexpected phentsize.\n";
    return false;
  }

  phdrs.resize(ehdr.e_phnum);
  for (uint16_t i = 0; i < ehdr.e_phnum; ++i) {
    size_t offset =
        static_cast<size_t>(ehdr.e_phoff) +
        static_cast<size_t>(i) * static_cast<size_t>(ehdr.e_phentsize);
    if (offset + sizeof(Elf64_Phdr) > data.size())
      return false;
    std::memcpy(&phdrs[i], data.data() + offset, sizeof(Elf64_Phdr));
  }

  return true;
}

bool ElfLoader::MapSegments(const std::vector<uint8_t> &data,
                            const std::vector<Elf64_Phdr> &phdrs,
                            uint64_t load_base) {
  if (!memory) {
    std::cerr << "[Loader] MapSegments called but MemoryManager is not set.\n";
    return false;
  }

  for (const auto &phdr : phdrs) {
    if (phdr.p_type == PT_LOAD || phdr.p_type == PT_SCE_RELRO ||
        phdr.p_type == PT_SCE_DYNLIBDATA) {

      uint64_t vaddr = load_base + phdr.p_vaddr;

      std::cout << "[Loader] Mapping segment: type=0x" << std::hex
                << phdr.p_type << ", vaddr=0x" << vaddr << ", size=0x"
                << phdr.p_memsz << std::dec << "\n";

      memory->Map(vaddr, phdr.p_memsz, phdr.p_flags, "ELF_SEGMENT");

      if (phdr.p_filesz > 0) {
        size_t file_offset = static_cast<size_t>(phdr.p_offset);
        size_t file_size = static_cast<size_t>(phdr.p_filesz);
        if (file_offset + file_size <= data.size()) {
          memory->Write(vaddr, data.data() + file_offset, file_size);
        } else {
          std::cerr << "[Loader] Segment file range out of bounds\n";
          return false;
        }
      }
    }
  }
  return true;
}

bool ElfLoader::ApplyRelocations(const std::vector<uint8_t> &data,
                                 const Elf64_Ehdr & /*ehdr*/,
                                 const std::vector<Elf64_Phdr> &phdrs,
                                 uint64_t load_base) {
  if (!memory) {
    std::cerr
        << "[Loader] ApplyRelocations called but MemoryManager is not set.\n";
    return false;
  }

  uint64_t rela_addr = 0;
  uint64_t rela_size = 0;
  uint64_t rela_ent_size = sizeof(Elf64_Rela);
  uint64_t sym_addr = 0;
  uint64_t str_addr = 0;

  for (const auto &phdr : phdrs) {
    if (phdr.p_type == PT_DYNAMIC) {
      uint64_t dyn_count = phdr.p_filesz / sizeof(Elf64_Dyn);
      for (uint64_t i = 0; i < dyn_count; ++i) {
        Elf64_Dyn dyn;
        size_t dyn_offset = static_cast<size_t>(phdr.p_offset) +
                            static_cast<size_t>(i * sizeof(Elf64_Dyn));
        if (dyn_offset + sizeof(Elf64_Dyn) > data.size())
          continue;
        std::memcpy(&dyn, data.data() + dyn_offset, sizeof(Elf64_Dyn));

        switch (dyn.d_tag) {
        case 7: // DT_RELA
          rela_addr = dyn.d_un.d_ptr + load_base;
          break;
        case 8: // DT_RELASZ
          rela_size = dyn.d_un.d_val;
          break;
        case 9: // DT_RELAENT
          rela_ent_size = dyn.d_un.d_val;
          break;
        case 5: // DT_SYMTAB
          sym_addr = dyn.d_un.d_ptr + load_base;
          break;
        case 10: // DT_STRTAB
          str_addr = dyn.d_un.d_ptr + load_base;
          break;
        default:
          break;
        }
      }
    } else if (phdr.p_type == PT_SCE_RELA) {
      rela_addr = phdr.p_vaddr + load_base;
      rela_size = phdr.p_memsz;
    }
  }

  if (rela_addr == 0 || rela_size == 0) {
    // Some ELFs don't have relocations, this is fine
    return true;
  }

  std::cout << "[Loader] Applying relocations: addr=0x" << std::hex << rela_addr
            << ", size=0x" << rela_size << std::dec << "\n";

  uint64_t count = rela_size / rela_ent_size;
  for (uint64_t i = 0; i < count; ++i) {
    Elf64_Rela rela;
    std::vector<uint8_t> rela_bytes(static_cast<size_t>(rela_ent_size));
    memory->Read(rela_addr + (i * rela_ent_size), rela_bytes.data(),
                 static_cast<size_t>(rela_ent_size));
    std::memcpy(&rela, rela_bytes.data(), sizeof(Elf64_Rela));

    // Relocations apply to VAddr: load_base + r_offset
    uint64_t target_vaddr = load_base + rela.r_offset;

    uint32_t type = static_cast<uint32_t>(rela.r_info & 0xFFFFFFFF);
    uint32_t sym_idx = static_cast<uint32_t>(rela.r_info >> 32);

    switch (type) {
    case R_X86_64_RELATIVE: {
      // B + A
      uint64_t value = load_base + static_cast<uint64_t>(rela.r_addend);
      memory->Write(target_vaddr, &value, sizeof(value));
      break;
    }
    case R_X86_64_64:
    case R_X86_64_GLOB_DAT:
    case R_X86_64_JUMP_SLOT: {
      if (sym_idx != 0 && sym_addr != 0) {
        Elf64_Sym sym;
        std::vector<uint8_t> sym_bytes(sizeof(Elf64_Sym));
        memory->Read(sym_addr + (sym_idx * sizeof(Elf64_Sym)), sym_bytes.data(),
                     sizeof(Elf64_Sym));
        std::memcpy(&sym, sym_bytes.data(), sizeof(Elf64_Sym));

        if (sym.st_value != 0) {
          uint64_t value = load_base + sym.st_value + rela.r_addend;
          memory->Write(target_vaddr, &value, sizeof(value));
        } else if (str_addr != 0) {
          char sym_name[256] = {0};
          memory->Read(str_addr + sym.st_name, sym_name, sizeof(sym_name));

          if (module_manager) {
            uint64_t resolved_addr = static_cast<uint64_t>(
                module_manager->ResolveSymbol("", sym_name));
            if (resolved_addr != 0) {
              uint64_t value = resolved_addr + rela.r_addend;
              memory->Write(target_vaddr, &value, sizeof(value));
              // std::cout << "[Loader] Resolved import: " << sym_name << " ->
              // 0x"
              //           << std::hex << resolved_addr << std::dec << "\n";
            } else {
              // std::cout << "[Loader] Unresolved import for relocation: "
              //           << sym_name << "\n";
            }
          }
        }
      }
      break;
    }
    default:
      break;
    }
  }

  return true;
}

bool ElfLoader::HandleImports(const std::vector<uint8_t> & /*data*/,
                              const Elf64_Ehdr & /*ehdr*/,
                              const std::vector<Elf64_Phdr> & /*phdrs*/) {
  if (!module_manager) {
    std::cout << "[Loader] HandleImports: module_manager not set; skipping.\n";
    return true;
  }
  // Implementation placeholder: resolve imports via module_manager when needed.
  return true;
}

} // namespace Loader
} // namespace Core