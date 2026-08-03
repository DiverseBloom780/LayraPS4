// src/core/loader/elf_loader.cpp
// SPDX-FileCopyrightText: Copyright 2025 LayraPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "elf_loader.h"
#include "cpu_patcher.h"
#include "core/kernel/module_manager.h"
#include "core/memory/memory_manager.h"
#include "common/types.h"

#include "core/kernel/sysv_abi_wrapper.h"

static uint64_t PS4_SYSV_ABI GenericHleStub() {
  printf("[HLE Stub] Called unimplemented function stub!\n");
  return 0;
}

static uint64_t GetWrappedGenericStub() {
  static Core::AbiWrapperManager wrapper_manager;
  static uint64_t wrapped_stub = 0;
  if (wrapped_stub == 0) {
    wrapped_stub = wrapper_manager.CreateWrapper(reinterpret_cast<uint64_t>(&GenericHleStub));
  }
  return wrapped_stub;
}


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

  uint64_t total_span = max_vaddr - min_vaddr;
  printf("[ElfLoader] Pre-reserving address range: 0x%llX - 0x%llX (span: 0x%llX)\n",
         (unsigned long long)min_vaddr, (unsigned long long)max_vaddr,
         (unsigned long long)total_span);

  // Reserve the entire range directly in address_space without adding to vma_map_
  if (!memory->GetAddressSpace().Map(min_vaddr, total_span, -1, Memory::Protection::ReadWriteExecute)) {
    fprintf(stderr, "[ElfLoader] ERROR: Failed to reserve address range 0x%llX-0x%llX\n",
            (unsigned long long)min_vaddr, (unsigned long long)max_vaddr);
    return false;
  }

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

  // --- SCE-specific dynamic tag constants ---
  // PS4 binaries use these instead of standard DT_RELA/DT_SYMTAB/etc.
  constexpr int64_t DT_SCE_STRTAB    = 0x61000035;
  constexpr int64_t DT_SCE_STRSZ     = 0x61000037;
  constexpr int64_t DT_SCE_SYMTAB    = 0x61000039;
  constexpr int64_t DT_SCE_SYMTABSZ  = 0x6100003f;
  constexpr int64_t DT_SCE_SYMENT    = 0x6100003b;
  constexpr int64_t DT_SCE_RELA      = 0x6100002f;
  constexpr int64_t DT_SCE_RELASZ    = 0x61000031;
  constexpr int64_t DT_SCE_RELAENT   = 0x61000033;
  constexpr int64_t DT_SCE_JMPREL    = 0x61000029;
  constexpr int64_t DT_SCE_PLTRELSZ  = 0x6100002d;
  constexpr int64_t DT_SCE_PLTREL    = 0x6100002b;
  constexpr int64_t DT_SCE_PLTGOT    = 0x61000027;
  constexpr int64_t DT_INIT          = 0x0000000c;
  constexpr int64_t DT_FINI          = 0x0000000d;

  // --- Phase 1: Load PT_SCE_DYNLIBDATA into a buffer ---
  // In PS4 binaries, the relocation/symbol/string tables live inside
  // SCE_DYNLIBDATA, and the dynamic tag pointers are offsets into it.
  std::vector<uint8_t> dynlib_data;
  for (size_t i = 0; i < phdrs.size(); i++) {
    if (phdrs[i].p_type == PT_SCE_DYNLIBDATA && phdrs[i].p_filesz > 0) {
      size_t file_offset = ResolveSegmentFileOffset(phdrs, data,
                                                    phdrs[i].p_offset,
                                                    phdrs[i].p_filesz);
      if (file_offset == SIZE_MAX) {
        fprintf(stderr, "[ElfLoader] WARNING: Failed to resolve SCE_DYNLIBDATA offset\n");
        break;
      }
      size_t file_size = static_cast<size_t>(phdrs[i].p_filesz);
      if (file_offset + file_size <= data.size()) {
        dynlib_data.resize(file_size);
        std::memcpy(dynlib_data.data(), data.data() + file_offset, file_size);
        printf("[ElfLoader] Loaded SCE_DYNLIBDATA: 0x%llX bytes\n",
               (unsigned long long)file_size);
      } else {
        fprintf(stderr, "[ElfLoader] WARNING: SCE_DYNLIBDATA extends beyond file\n");
      }
      break;
    }
  }

  // --- Phase 2: Parse PT_DYNAMIC to find relocation/symbol/string tables ---
  // SCE tags' d_ptr values are offsets into dynlib_data.
  // Standard tags' d_ptr values are virtual addresses (relative to load_base).
  uint64_t sce_rela_offset = 0;   // Offset into dynlib_data
  uint64_t sce_rela_size = 0;
  uint64_t sce_rela_ent = sizeof(Elf64_Rela);
  uint64_t sce_jmprel_offset = 0; // Offset into dynlib_data
  uint64_t sce_jmprel_size = 0;
  uint64_t sce_symtab_offset = 0; // Offset into dynlib_data
  uint64_t sce_strtab_offset = 0; // Offset into dynlib_data
  uint64_t sce_strtab_size = 0;
  uint64_t init_vaddr = 0;
  uint64_t fini_vaddr = 0;
  bool has_sce_tags = false;

  // Also track standard ELF tags as fallback
  uint64_t std_rela_addr = 0;
  uint64_t std_rela_size = 0;
  uint64_t std_rela_ent = sizeof(Elf64_Rela);
  uint64_t std_sym_addr = 0;
  uint64_t std_str_addr = 0;

  for (size_t phdr_idx = 0; phdr_idx < phdrs.size(); phdr_idx++) {
    const auto &phdr = phdrs[phdr_idx];

    if (phdr.p_type == PT_DYNAMIC) {
      size_t dyn_file_offset = ResolveSegmentFileOffset(phdrs, data,
                                                        phdr.p_offset,
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

        if (dyn.d_tag == 0) // DT_NULL
          break;

        switch (dyn.d_tag) {
        // --- SCE-specific tags (offsets into dynlib_data) ---
        case DT_SCE_RELA:
          sce_rela_offset = dyn.d_un.d_ptr;
          has_sce_tags = true;
          break;
        case DT_SCE_RELASZ:
          sce_rela_size = dyn.d_un.d_val;
          break;
        case DT_SCE_RELAENT:
          sce_rela_ent = dyn.d_un.d_val;
          break;
        case DT_SCE_JMPREL:
          sce_jmprel_offset = dyn.d_un.d_ptr;
          has_sce_tags = true;
          break;
        case DT_SCE_PLTRELSZ:
          sce_jmprel_size = dyn.d_un.d_val;
          break;
        case DT_SCE_SYMTAB:
          sce_symtab_offset = dyn.d_un.d_ptr;
          has_sce_tags = true;
          break;
        case DT_SCE_STRTAB:
          sce_strtab_offset = dyn.d_un.d_ptr;
          has_sce_tags = true;
          break;
        case DT_SCE_STRSZ:
          sce_strtab_size = dyn.d_un.d_val;
          break;
        // --- Standard ELF tags (virtual addresses) ---
        case 7: // DT_RELA
          std_rela_addr = dyn.d_un.d_ptr + load_base;
          break;
        case 8: // DT_RELASZ
          std_rela_size = dyn.d_un.d_val;
          break;
        case 9: // DT_RELAENT
          std_rela_ent = dyn.d_un.d_val;
          break;
        case 5: // DT_SYMTAB
          std_sym_addr = dyn.d_un.d_ptr + load_base;
          break;
        case 10: // DT_STRTAB
          std_str_addr = dyn.d_un.d_ptr + load_base;
          break;
        case DT_INIT:
          init_vaddr = dyn.d_un.d_ptr;
          break;
        case DT_FINI:
          fini_vaddr = dyn.d_un.d_ptr;
          break;
        default:
          break;
        }
      }
    }
  }

  if (init_vaddr) {
    printf("[ElfLoader] DT_INIT: 0x%llX\n", (unsigned long long)(load_base + init_vaddr));
  }
  if (fini_vaddr) {
    printf("[ElfLoader] DT_FINI: 0x%llX\n", (unsigned long long)(load_base + fini_vaddr));
  }

  // --- Phase 3: Apply relocations ---
  // Lambda to apply a single relocation entry
  auto apply_one_rela = [&](const Elf64_Rela &rela) {
    uint64_t target_vaddr = load_base + rela.r_offset;
    uint32_t type = static_cast<uint32_t>(rela.r_info & 0xFFFFFFFF);
    uint32_t sym_idx = static_cast<uint32_t>(rela.r_info >> 32);

    switch (type) {
    case R_X86_64_NONE:
      break;
    case R_X86_64_RELATIVE: {
      // B + A
      uint64_t value = load_base + static_cast<uint64_t>(rela.r_addend);
      memory->Write(target_vaddr, &value, sizeof(value));
      break;
    }
    case R_X86_64_64:
    case R_X86_64_GLOB_DAT:
    case R_X86_64_JUMP_SLOT: {
      if (sym_idx == 0)
        break;

      // Read symbol from the symbol table
      Elf64_Sym sym{};
      if (has_sce_tags && !dynlib_data.empty()) {
        // SCE path: symbol table is in dynlib_data
        size_t sym_off = static_cast<size_t>(sce_symtab_offset +
                                             sym_idx * sizeof(Elf64_Sym));
        if (sym_off + sizeof(Elf64_Sym) <= dynlib_data.size()) {
          std::memcpy(&sym, dynlib_data.data() + sym_off, sizeof(Elf64_Sym));
        }
      } else if (std_sym_addr != 0) {
        // Standard ELF path: symbol table is in mapped memory
        std::vector<uint8_t> sym_bytes(sizeof(Elf64_Sym));
        memory->Read(std_sym_addr + (sym_idx * sizeof(Elf64_Sym)),
                     sym_bytes.data(), sizeof(Elf64_Sym));
        std::memcpy(&sym, sym_bytes.data(), sizeof(Elf64_Sym));
      } else {
        fprintf(stderr, "[ElfLoader] ERROR: No symbol table for reloc at 0x%llX\n",
                (unsigned long long)target_vaddr);
        break;
      }

      if (sym.st_value != 0) {
        // Symbol has a defined value in this module
        uint64_t value = load_base + sym.st_value;
        if (type == R_X86_64_64)
          value += rela.r_addend;
        memory->Write(target_vaddr, &value, sizeof(value));
      } else {
        // Symbol needs to be resolved from another module (import)
        char sym_name[256] = {0};
        if (has_sce_tags && !dynlib_data.empty()) {
          size_t name_off = static_cast<size_t>(sce_strtab_offset + sym.st_name);
          if (name_off < dynlib_data.size()) {
            size_t max_len = std::min<size_t>(255, dynlib_data.size() - name_off);
            std::memcpy(sym_name, dynlib_data.data() + name_off, max_len);
            sym_name[max_len] = '\0';
          }
        } else if (std_str_addr != 0) {
          memory->Read(std_str_addr + sym.st_name, sym_name, sizeof(sym_name));
        }

        if (module_manager && sym_name[0] != '\0') {
          uint64_t resolved_addr = static_cast<uint64_t>(
              module_manager->ResolveSymbol("", sym_name));
          if (resolved_addr != 0) {
            uint64_t value = resolved_addr;
            if (type == R_X86_64_64)
              value += rela.r_addend;
            memory->Write(target_vaddr, &value, sizeof(value));
          } else {
            // Stub unresolved imports with a trap instead of failing
            // This allows the game to progress further before hitting the stub
            printf("[ElfLoader] WARNING: Unresolved import: %s (stubbed)\n", sym_name);
            uint64_t stub_value = GetWrappedGenericStub();
            memory->Write(target_vaddr, &stub_value, sizeof(stub_value));
          }
        } else {
          // No module manager or empty name - stub it
          uint64_t stub_value = GetWrappedGenericStub();
          memory->Write(target_vaddr, &stub_value, sizeof(stub_value));
        }
      }
      break;
    }
    default:
      // Silently skip unknown relocation types
      break;
    }
  };

  // Lambda to process a relocation table from dynlib_data
  auto process_rela_from_dynlib = [&](uint64_t offset, uint64_t size,
                                      uint64_t ent_size, const char *name) {
    if (size == 0 || ent_size == 0)
      return;
    uint64_t count = size / ent_size;
    printf("[ElfLoader] Applying %s: %llu entries (offset=0x%llX in DYNLIBDATA)\n",
           name, (unsigned long long)count, (unsigned long long)offset);

    uint64_t applied = 0;
    for (uint64_t i = 0; i < count; ++i) {
      size_t rela_off = static_cast<size_t>(offset + i * ent_size);
      if (rela_off + sizeof(Elf64_Rela) > dynlib_data.size())
        break;
      Elf64_Rela rela;
      std::memcpy(&rela, dynlib_data.data() + rela_off, sizeof(Elf64_Rela));
      apply_one_rela(rela);
      applied++;
    }
    printf("[ElfLoader] Applied %llu %s relocations\n",
           (unsigned long long)applied, name);
  };

  if (has_sce_tags && !dynlib_data.empty()) {
    // SCE path: read relocations from dynlib_data buffer
    printf("[ElfLoader] Using SCE dynamic tags for relocations\n");
    printf("[ElfLoader]   SCE_RELA: offset=0x%llX size=0x%llX ent=0x%llX\n",
           (unsigned long long)sce_rela_offset,
           (unsigned long long)sce_rela_size,
           (unsigned long long)sce_rela_ent);
    printf("[ElfLoader]   SCE_JMPREL: offset=0x%llX size=0x%llX\n",
           (unsigned long long)sce_jmprel_offset,
           (unsigned long long)sce_jmprel_size);
    printf("[ElfLoader]   SCE_SYMTAB: offset=0x%llX\n",
           (unsigned long long)sce_symtab_offset);
    printf("[ElfLoader]   SCE_STRTAB: offset=0x%llX size=0x%llX\n",
           (unsigned long long)sce_strtab_offset,
           (unsigned long long)sce_strtab_size);

    // Apply regular relocations (DT_SCE_RELA)
    process_rela_from_dynlib(sce_rela_offset, sce_rela_size,
                             sce_rela_ent, "RELA");

    // Apply PLT jump slot relocations (DT_SCE_JMPREL)
    process_rela_from_dynlib(sce_jmprel_offset, sce_jmprel_size,
                             sizeof(Elf64_Rela), "JMPREL");
  } else if (std_rela_addr != 0 && std_rela_size != 0) {
    // Standard ELF path: read relocations from mapped memory
    printf("[ElfLoader] Using standard ELF dynamic tags for relocations\n");
    printf("[ElfLoader] Applying relocations: addr=0x%llX, size=0x%llX\n",
           (unsigned long long)std_rela_addr, (unsigned long long)std_rela_size);

    uint64_t count = std_rela_size / std_rela_ent;
    for (uint64_t i = 0; i < count; ++i) {
      Elf64_Rela rela;
      std::vector<uint8_t> rela_bytes(static_cast<size_t>(std_rela_ent));
      memory->Read(std_rela_addr + (i * std_rela_ent), rela_bytes.data(),
                   static_cast<size_t>(std_rela_ent));
      std::memcpy(&rela, rela_bytes.data(), sizeof(Elf64_Rela));
      apply_one_rela(rela);
    }
  } else {
    printf("[ElfLoader] No relocations found (this may be normal for some ELFs)\n");
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
