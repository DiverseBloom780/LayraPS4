// src/core/loader/elf_loader.cpp
// SPDX-FileCopyrightText: Copyright 2025 LayraPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "elf_loader.h"
#include "cpu_patcher.h"
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

// Default constructor
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

// ── SELF detection and parsing ───────────────────────────────────────
bool ElfLoader::DetectAndParseSelf(const std::vector<uint8_t> &data) {
  // Reset SELF state
  is_self = false;
  self_segments.clear();
  elf_header_offset = 0;

  if (data.size() < sizeof(SelfHeader)) {
    return false;
  }

  // Check for SELF magic
  uint32_t magic = 0;
  std::memcpy(&magic, data.data(), sizeof(magic));
  if (magic != SELF_MAGIC) {
    return false; // Not a SELF file, that's fine
  }

  // Parse the SELF header
  std::memcpy(&self_header, data.data(), sizeof(SelfHeader));

  printf("[ElfLoader] SELF file detected!\n");
  printf("[ElfLoader]   magic .........: 0x%08X\n", self_header.magic);
  printf("[ElfLoader]   version .......: %u\n", self_header.version);
  printf("[ElfLoader]   mode ..........: 0x%02X\n", self_header.mode);
  printf("[ElfLoader]   endian ........: %u\n", self_header.endian);
  printf("[ElfLoader]   attributes ....: 0x%02X\n", self_header.attributes);
  printf("[ElfLoader]   category ......: 0x%02X\n", self_header.category);
  printf("[ElfLoader]   program_type ..: 0x%02X\n", self_header.program_type);
  printf("[ElfLoader]   header_size ...: %u\n", self_header.header_size);
  printf("[ElfLoader]   segment_count .: %u\n", self_header.segment_count);

  // Validate basic SELF header fields (based on shadPS4 reference)
  if (self_header.version != 0x00 || self_header.mode != 0x01 ||
      self_header.endian != 0x01 || self_header.attributes != 0x12) {
    fprintf(stderr, "[ElfLoader] WARNING: Unusual SELF header fields, "
                    "attempting to parse anyway\n");
  }

  // Read SELF segment headers
  size_t seg_offset = sizeof(SelfHeader);
  size_t seg_table_size =
      static_cast<size_t>(self_header.segment_count) * sizeof(SelfSegmentHeader);

  if (seg_offset + seg_table_size > data.size()) {
    fprintf(stderr,
            "[ElfLoader] ERROR: SELF segment table extends beyond file\n");
    return false;
  }

  self_segments.resize(self_header.segment_count);
  std::memcpy(self_segments.data(), data.data() + seg_offset, seg_table_size);

  // Log segment info
  for (uint16_t i = 0; i < self_header.segment_count; i++) {
    const auto &seg = self_segments[i];
    printf("[ElfLoader]   SELF Segment %u: flags=0x%016llX offset=0x%llX "
           "size=0x%llX mem=0x%llX blocked=%d id=%u enc=%d\n",
           i, (unsigned long long)seg.flags,
           (unsigned long long)seg.file_offset,
           (unsigned long long)seg.file_size,
           (unsigned long long)seg.memory_size, seg.IsBlocked(), seg.GetId(),
           seg.IsEncrypted());

    if (seg.IsEncrypted()) {
      fprintf(stderr, "[ElfLoader] WARNING: SELF segment %u is encrypted! "
                      "Decryption is not yet supported.\n",
              i);
    }
    if (seg.IsCompressed()) {
      fprintf(stderr, "[ElfLoader] WARNING: SELF segment %u is compressed! "
                      "Decompression is not yet supported.\n",
              i);
    }
  }

  // The ELF header starts after the SELF segment table, rounded up to a 16-byte boundary.
  // Some SELF files include padding between the segment table and the ELF header.
  elf_header_offset = (seg_offset + seg_table_size + 0xF) & ~static_cast<size_t>(0xF);
  is_self = true;

  printf("[ElfLoader] ELF header starts at file offset 0x%zX (aligned)\n",
         elf_header_offset);

  return true;
}

// ── Resolve actual file offset for SELF segment data ─────────────────
size_t
ElfLoader::ResolveSegmentFileOffset(const std::vector<Elf64_Phdr> &phdrs,
                                    const std::vector<uint8_t> &data,
                                    uint64_t desired_offset,
                                    uint64_t desired_size) const {
  if (!is_self) {
    // For plain ELF, the offset in the file is just the p_offset.
    return static_cast<size_t>(desired_offset);
  }

  // For SELF files, resolve blocked segment data back into the SELF file.
  // This matches shadPS4's Elf::LoadSegment() behavior.
  for (uint16_t i = 0; i < self_segments.size(); i++) {
    const auto &seg = self_segments[i];
    if (!seg.IsBlocked())
      continue;

    uint32_t phdr_id = seg.GetId();
    if (phdr_id >= phdrs.size())
      continue;

    const auto &phdr = phdrs[phdr_id];
    if (desired_offset >= phdr.p_offset &&
        desired_offset < phdr.p_offset + phdr.p_filesz) {
      uint64_t offset_within = desired_offset - phdr.p_offset;
      return static_cast<size_t>(seg.file_offset + offset_within);
    }
  }

  // Fallback: if the ELF image is embedded directly after the SELF header,
  // resolve offsets relative to the ELF header start.
  if (elf_header_offset + desired_offset + desired_size <= data.size()) {
    return static_cast<size_t>(elf_header_offset + desired_offset);
  }

  fprintf(stderr,
          "[ElfLoader] WARNING: Could not resolve SELF offset for "
          "ELF offset 0x%llX (size 0x%llX)\n",
          (unsigned long long)desired_offset,
          (unsigned long long)desired_size);

  return SIZE_MAX;
}

ElfLoader::LoadResult ElfLoader::Load(const std::string &path) {
  std::ifstream file(path, std::ios::binary | std::ios::ate);
  if (!file.is_open()) {
    return {false, 0, 0, 0, "Failed to open file: " + path};
  }

  std::streamsize size = file.tellg();
  file.seekg(0, std::ios::beg);

  if (size <= 0) {
    return {false, 0, 0, 0, "File empty or error reading size: " + path};
  }

  std::vector<uint8_t> data(static_cast<size_t>(size));
  if (!file.read(reinterpret_cast<char *>(data.data()), size)) {
    return {false, 0, 0, 0, "Failed to read file"};
  }

  // Step 1: Detect and parse SELF if applicable
  DetectAndParseSelf(data);

  if (is_self) {
    printf("[ElfLoader] Processing as SELF (Signed ELF) file\n");
  } else {
    printf("[ElfLoader] Processing as raw ELF file\n");
  }

  // Step 2: Parse ELF headers (from the correct offset)
  Elf64_Ehdr ehdr;
  std::vector<Elf64_Phdr> phdrs;

  if (!ParseHeaders(data, ehdr, phdrs)) {
    return {false, 0, 0, 0, "Failed to parse ELF headers"};
  }

  // Calculate load base for relocatable ELFs (ET_DYN / SCE types)
  uint64_t load_base = 0;
  uint64_t min_vaddr = UINT64_MAX;
  uint64_t max_vaddr = 0;

  for (const auto &phdr : phdrs) {
    if (phdr.p_type == PT_LOAD || phdr.p_type == PT_SCE_RELRO) {
      if (phdr.p_memsz == 0)
        continue;
      if (phdr.p_vaddr < min_vaddr)
        min_vaddr = phdr.p_vaddr;
      if (phdr.p_vaddr + phdr.p_memsz > max_vaddr)
        max_vaddr = phdr.p_vaddr + phdr.p_memsz;
    }
  }

  // PS4 executables use SCE types (0xFE00, 0xFE10, 0xFE18)
  // ET_SCE_EXEC=0xFE00, ET_SCE_DYNEXEC=0xFE10, ET_SCE_DYNAMIC=0xFE18
  if (ehdr.e_type == 3 ||             // ET_DYN
      ehdr.e_type == 0xFE10 ||        // ET_SCE_DYNEXEC
      ehdr.e_type == 0xFE18) {        // ET_SCE_DYNAMIC
    load_base = 0x400000;
    printf("[ElfLoader] Relocatable type (0x%04X). Using load base: 0x%llx\n",
           ehdr.e_type, (unsigned long long)load_base);
  } else if (ehdr.e_type == 0xFE00) { // ET_SCE_EXEC (fixed address)
    printf("[ElfLoader] SCE EXEC type (0x%04X). Using absolute addresses\n",
           ehdr.e_type);
  } else {
    printf("[ElfLoader] ELF type 0x%04X. Using absolute addresses (base 0)\n",
           ehdr.e_type);
  }

  printf("[ElfLoader] Trace: Headers parsed, now mapping segments...\n");

  if (!MapSegments(data, phdrs, load_base)) {
    fprintf(stderr, "[ElfLoader] ERROR: Failed to map ELF segments\n");
    return {false, 0, 0, 0, "Failed to map segments"};
  }

  printf("[ElfLoader] Trace: Segments mapped, now applying relocations...\n");

  if (!ApplyRelocations(data, ehdr, phdrs, load_base)) {
    fprintf(stderr, "[ElfLoader] ERROR: Failed to apply ELF relocations\n");
    return {false, 0, 0, 0, "Failed to apply relocations"};
  }

  printf("[ElfLoader] Trace: Relocations applied, finishing load...\n");
  HandleImports(data, ehdr, phdrs);

  // Apply CPU patches to fix FS segment conflicts on Windows
  if (memory) {
    for (const auto &phdr : phdrs) {
      // Only patch executable segments
      if (phdr.p_type == PT_LOAD && (phdr.p_flags & 1) != 0) {
        uint8_t *host_ptr = static_cast<uint8_t *>(
            memory->GetHostPtr(load_base + phdr.p_vaddr));
        if (host_ptr && phdr.p_memsz > 0) {
          ApplyLitePatches(host_ptr, phdr.p_memsz);
        }
      }
    }
  }

  // Apply final memory protections now that all relocations/patches are done
  if (memory) {
    printf("[ElfLoader] Applying final memory protections...\n");
    for (const auto &phdr : phdrs) {
      if (phdr.p_type == PT_LOAD || phdr.p_type == PT_SCE_RELRO) {
        if (phdr.p_memsz == 0)
          continue;

        uint32_t final_prot = 0;
        if (phdr.p_flags & 1)
          final_prot |= 4; // X
        if (phdr.p_flags & 2)
          final_prot |= 2; // W
        if (phdr.p_flags & 4)
          final_prot |= 1; // R

        uint64_t vaddr = load_base + phdr.p_vaddr;
        uint64_t mem_size = phdr.p_memsz;
        if (phdr.p_align != 0) {
          mem_size = (mem_size + (phdr.p_align - 1)) & ~(phdr.p_align - 1);
        }

        memory->Protect(vaddr, mem_size, static_cast<int32_t>(final_prot));
      }
    }
  }

  // Return load result
  LoadResult result;
  result.success = true;
  result.entry_point = load_base + ehdr.e_entry;
  result.load_base = load_base;
  result.image_size = (max_vaddr > min_vaddr) ? (max_vaddr - min_vaddr) : 0;
  printf("[ElfLoader] %s loaded successfully. Entry point: 0x%llx\n",
         is_self ? "SELF" : "ELF",
         (unsigned long long)(load_base + ehdr.e_entry));
  return result;
}

bool ElfLoader::ParseHeaders(const std::vector<uint8_t> &data, Elf64_Ehdr &ehdr,
                             std::vector<Elf64_Phdr> &phdrs) {
  // For SELF files, the ELF header starts after SELF header + SELF segments
  // For raw ELF files, it starts at offset 0
  size_t ehdr_off = elf_header_offset;

  if (ehdr_off + sizeof(Elf64_Ehdr) > data.size()) {
    fprintf(stderr, "[ElfLoader] File too small for ELF header at offset 0x%zX\n",
            ehdr_off);
    return false;
  }

  std::memcpy(&ehdr, data.data() + ehdr_off, sizeof(Elf64_Ehdr));

  if (std::memcmp(ehdr.e_ident, "\x7f\x45\x4c\x46", 4) != 0) {
    fprintf(stderr, "[ElfLoader] Invalid ELF magic at offset 0x%zX\n", ehdr_off);
    return false;
  }

  if (ehdr.e_ident[4] != 2) { // 64-bit
    fprintf(stderr, "[ElfLoader] Not a 64-bit ELF\n");
    return false;
  }

  printf("[ElfLoader] ELF header parsed at offset 0x%zX\n", ehdr_off);
  printf("[ElfLoader]   type .......: 0x%04X\n", ehdr.e_type);
  printf("[ElfLoader]   machine ....: 0x%04X\n", ehdr.e_machine);
  printf("[ElfLoader]   entry ......: 0x%llX\n", (unsigned long long)ehdr.e_entry);
  printf("[ElfLoader]   phnum ......: %u\n", ehdr.e_phnum);
  printf("[ElfLoader]   phoff ......: 0x%llX\n", (unsigned long long)ehdr.e_phoff);

  if (ehdr.e_phnum == 0 || ehdr.e_phentsize < sizeof(Elf64_Phdr)) {
    fprintf(stderr, "[ElfLoader] No program headers or unexpected phentsize (%u)\n",
            ehdr.e_phentsize);
    return false;
  }

  // Read program headers - for SELF files, phoff is relative to ELF header pos
  phdrs.resize(ehdr.e_phnum);
  for (uint16_t i = 0; i < ehdr.e_phnum; ++i) {
    size_t offset =
        ehdr_off + static_cast<size_t>(ehdr.e_phoff) +
        static_cast<size_t>(i) * static_cast<size_t>(ehdr.e_phentsize);
    if (offset + sizeof(Elf64_Phdr) > data.size()) {
      fprintf(stderr, "[ElfLoader] Program header %u extends beyond file\n", i);
      return false;
    }
    std::memcpy(&phdrs[i], data.data() + offset, sizeof(Elf64_Phdr));
  }

  // Log program headers
  for (uint16_t i = 0; i < ehdr.e_phnum; ++i) {
    const auto &ph = phdrs[i];
    const char *type_str = "UNKNOWN";
    switch (ph.p_type) {
    case PT_LOAD: type_str = "LOAD"; break;
    case PT_DYNAMIC: type_str = "DYNAMIC"; break;
    case PT_TLS: type_str = "TLS"; break;
    case PT_SCE_RELA: type_str = "SCE_RELA"; break;
    case PT_SCE_DYNLIBDATA: type_str = "SCE_DYNLIBDATA"; break;
    case PT_SCE_PROCPARAM: type_str = "SCE_PROCPARAM"; break;
    case PT_SCE_MODULE_INFO: type_str = "SCE_MODULE_PARAM"; break;
    case PT_SCE_RELRO: type_str = "SCE_RELRO"; break;
    case PT_PHDR: type_str = "PHDR"; break;
    case 0x6474e550: type_str = "GNU_EH_FRAME"; break;
    case 0x6474e551: type_str = "GNU_STACK"; break;
    case 0x6474e552: type_str = "GNU_RELRO"; break;
    }
    printf("[ElfLoader]   Phdr[%u] type=%-16s vaddr=0x%08llX filesz=0x%llX "
           "memsz=0x%llX offset=0x%llX flags=0x%X\n",
           i, type_str, (unsigned long long)ph.p_vaddr,
           (unsigned long long)ph.p_filesz, (unsigned long long)ph.p_memsz,
           (unsigned long long)ph.p_offset, ph.p_flags);
  }

  return true;
}

bool ElfLoader::MapSegments(const std::vector<uint8_t> &data,
                            const std::vector<Elf64_Phdr> &phdrs,
                            uint64_t load_base) {
  if (!memory) {
    fprintf(stderr, "[ElfLoader] MapSegments called but MemoryManager is not set\n");
    return false;
  }

  // --- Phase 1: Compute the total virtual address span ---
  // Find the lowest and highest addresses across all loadable segments
  // so we can reserve one contiguous block.
  uint64_t min_vaddr = UINT64_MAX;
  uint64_t max_vaddr = 0;

  for (size_t i = 0; i < phdrs.size(); i++) {
    const auto &phdr = phdrs[i];
    if ((phdr.p_type == PT_LOAD || phdr.p_type == PT_SCE_RELRO) &&
        phdr.p_memsz > 0) {
      uint64_t vaddr = load_base + phdr.p_vaddr;
      uint64_t mem_size = phdr.p_memsz;
      if (phdr.p_align != 0) {
        mem_size = (mem_size + (phdr.p_align - 1)) & ~(phdr.p_align - 1);
      }
      if (vaddr < min_vaddr) min_vaddr = vaddr;
      if (vaddr + mem_size > max_vaddr) max_vaddr = vaddr + mem_size;
    }
  }

  if (min_vaddr >= max_vaddr) {
    fprintf(stderr, "[ElfLoader] No loadable segments found\n");
    return false;
  }

  printf("[ElfLoader] Mapping ELF segments directly into guest address space\n");

  // --- Phase 2: Write segment data into the mapped region ---
  for (size_t phdr_idx = 0; phdr_idx < phdrs.size(); phdr_idx++) {
    const auto &phdr = phdrs[phdr_idx];

    if (phdr.p_type == PT_LOAD || phdr.p_type == PT_SCE_RELRO ||
        phdr.p_type == PT_SCE_DYNLIBDATA) {

      if (phdr.p_memsz == 0)
        continue;

      uint64_t vaddr = load_base + phdr.p_vaddr;

      if (phdr.p_type == PT_LOAD || phdr.p_type == PT_SCE_RELRO) {
        uint32_t prot = 0;
        if (phdr.p_flags & 1)
          prot |= 4; // X
        if (phdr.p_flags & 2)
          prot |= 2; // W
        if (phdr.p_flags & 4)
          prot |= 1; // R

        uint64_t mem_size = phdr.p_memsz;
        if (phdr.p_align != 0) {
          mem_size = (mem_size + (phdr.p_align - 1)) & ~(phdr.p_align - 1);
        }

        printf("[ElfLoader] Segment[%zu]: type=0x%X, vaddr=0x%llX, "
               "memsz=0x%llX, filesz=0x%llX, prot=0x%X\n",
               phdr_idx, phdr.p_type, (unsigned long long)vaddr,
               (unsigned long long)mem_size,
               (unsigned long long)phdr.p_filesz, prot);

        // Temporarily map all segments as writable for relocations/patching
        uint32_t temp_prot = prot | 2; // Add Write permission
        if (!memory->Map(vaddr, mem_size, temp_prot, "ELF_SEGMENT")) {
          fprintf(stderr,
                  "[ElfLoader] ERROR: Failed to map segment %zu at 0x%llX\n",
                  phdr_idx, (unsigned long long)vaddr);
          return false;
        }
      }

      if (phdr.p_filesz > 0) {
        // Resolve where the segment data actually lives in the file
        size_t file_offset = ResolveSegmentFileOffset(phdrs, data, phdr.p_offset,
                                                     phdr.p_filesz);

        if (file_offset == SIZE_MAX) {
          fprintf(stderr,
                  "[ElfLoader] ERROR: Segment mapping failed due to unresolved offset for phdr %zu\n",
                  phdr_idx);
          return false;
        }

        size_t file_size = static_cast<size_t>(phdr.p_filesz);
        if (file_offset + file_size <= data.size()) {
          memory->Write(vaddr, data.data() + file_offset, file_size);
        } else {
          fprintf(stderr,
                  "[ElfLoader] Segment file range out of bounds "
                  "(offset=0x%zX size=0x%zX filesize=0x%zX)\n",
                  file_offset, file_size, data.size());
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
    fprintf(stderr,
            "[ElfLoader] ApplyRelocations called but MemoryManager is not set\n");
    return false;
  }

  uint64_t rela_addr = 0;
  uint64_t rela_size = 0;
  uint64_t rela_ent_size = sizeof(Elf64_Rela);
  uint64_t sym_addr = 0;
  uint64_t str_addr = 0;

  for (size_t phdr_idx = 0; phdr_idx < phdrs.size(); phdr_idx++) {
    const auto &phdr = phdrs[phdr_idx];

    if (phdr.p_type == PT_DYNAMIC) {
      // For SELF files, we need to read DYNAMIC data from the right offset
      size_t dyn_file_offset = ResolveSegmentFileOffset(phdrs, data, phdr.p_offset,
                                                       phdr.p_filesz);
      if (dyn_file_offset == SIZE_MAX) {
        fprintf(stderr, "[ElfLoader] WARNING: Failed to resolve DYNAMIC segment offset\n");
        continue;
      }

      uint64_t dyn_count = phdr.p_filesz / sizeof(Elf64_Dyn);
      for (uint64_t i = 0; i < dyn_count; ++i) {
        Elf64_Dyn dyn;
        size_t dyn_offset = dyn_file_offset +
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

  printf("[ElfLoader] Applying relocations: addr=0x%llX, size=0x%llX\n",
         (unsigned long long)rela_addr, (unsigned long long)rela_size);

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
              printf("[ElfLoader] Resolved import: %s -> 0x%llx\n", sym_name,
                     (unsigned long long)resolved_addr);
            } else {
              printf("[ElfLoader] ERROR: Unresolved import: %s\n", sym_name);
              return false;
            }
          } else {
            printf("[ElfLoader] ERROR: No module manager available to resolve %s\n",
                   sym_name);
            return false;
          }
        } else {
          fprintf(stderr, "[ElfLoader] ERROR: Cannot resolve relocation for symbol "
                          "index %u because symbol or string table is missing\n",
                  sym_idx);
          return false;
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
    printf("[ElfLoader] HandleImports: module_manager not set; skipping.\n");
    return true;
  }
  // Implementation placeholder: resolve imports via module_manager when needed.
  return true;
}

} // namespace Loader
} // namespace Core