// elf_loader.h
// SPDX-FileCopyrightText: Copyright 2025 LayraPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace Core {

namespace Memory {
class MemoryManager;
}

namespace Loader {

// ELF64 constants
constexpr uint32_t PT_LOAD = 1;
constexpr uint32_t PT_DYNAMIC = 2;
constexpr uint32_t PT_INTERP = 3;
constexpr uint32_t PT_NOTE = 4;
constexpr uint32_t PT_SHLIB = 5;
constexpr uint32_t PT_PHDR = 6;
constexpr uint32_t PT_TLS = 7;

// SCE Specific Program Header Types
constexpr uint32_t PT_SCE_RELA = 0x60000000;
constexpr uint32_t PT_SCE_DYNLIBDATA = 0x61000000;
constexpr uint32_t PT_SCE_PROCPARAM = 0x61000001;
constexpr uint32_t PT_SCE_MODULE_INFO = 0x61000002;
constexpr uint32_t PT_SCE_RELRO = 0x61000010;

// Relocation types
constexpr uint32_t R_X86_64_NONE = 0;
constexpr uint32_t R_X86_64_64 = 1;
constexpr uint32_t R_X86_64_PC32 = 2;
constexpr uint32_t R_X86_64_GOT32 = 3;
constexpr uint32_t R_X86_64_PLT32 = 4;
constexpr uint32_t R_X86_64_COPY = 5;
constexpr uint32_t R_X86_64_GLOB_DAT = 6;
constexpr uint32_t R_X86_64_JUMP_SLOT = 7;
constexpr uint32_t R_X86_64_RELATIVE = 8;

// PS4 SELF (Signed ELF) magic signature
constexpr uint32_t SELF_MAGIC = 0x1D3D154F;

// ── PS4 SELF header (based on shadPS4 reference) ──────────────────────
struct SelfHeader {
  uint32_t magic;          // 0x1D3D154F
  uint8_t  version;        // 0x00
  uint8_t  mode;           // 0x01
  uint8_t  endian;         // 0x01 = little-endian
  uint8_t  attributes;     // 0x12
  uint8_t  category;       // 0x01
  uint8_t  program_type;   // 0x01
  uint16_t padding1;
  uint16_t header_size;
  uint16_t meta_size;
  uint32_t file_size;
  uint32_t padding2;
  uint16_t segment_count;
  uint16_t unknown1a;      // always 0x22
  uint32_t padding3;
};

// ── SELF segment header ───────────────────────────────────────────────
struct SelfSegmentHeader {
  uint64_t flags;
  uint64_t file_offset;    // Where in the SELF file this segment's data lives
  uint64_t file_size;
  uint64_t memory_size;

  // A "blocked" segment contains loadable data mapped to a program header
  bool IsBlocked() const { return (flags & 0x800) != 0; }
  // The program-header index this segment corresponds to
  uint32_t GetId() const { return (flags >> 20u) & 0xFFFu; }
  // Check if segment data is encrypted (retail discs only)
  bool IsEncrypted() const { return (flags & 2) != 0; }
  // Check if segment data is compressed
  bool IsCompressed() const { return (flags & 8) != 0; }
};

} // namespace Loader

namespace Kernel {
class ModuleManager;
}

namespace Loader {

class ElfLoader {
public:
  ElfLoader(); // <-- added default constructor
  ElfLoader(Memory::MemoryManager *memoryManager,
            Kernel::ModuleManager *moduleManager);
  ~ElfLoader();

  struct LoadResult {
    bool success = false;
    uint64_t entry_point = 0;
    uint64_t load_base = 0;
    uint64_t image_size = 0;
    std::string error_msg;
  };

  LoadResult Load(const std::string &path);

private:
  Memory::MemoryManager *memory;
  Kernel::ModuleManager *module_manager;

  // ── SELF state ────────────────────────────────────────────────────
  bool is_self = false;
  SelfHeader self_header{};
  std::vector<SelfSegmentHeader> self_segments;
  size_t elf_header_offset = 0;  // Where the ELF header starts in file

  struct Elf64_Ehdr {
    uint8_t e_ident[16];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint64_t e_entry;
    uint64_t e_phoff;
    uint64_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
  };

  struct Elf64_Phdr {
    uint32_t p_type;
    uint32_t p_flags;
    uint64_t p_offset;
    uint64_t p_vaddr;
    uint64_t p_paddr;
    uint64_t p_filesz;
    uint64_t p_memsz;
    uint64_t p_align;
  };

  struct Elf64_Dyn {
    int64_t d_tag;
    union {
      uint64_t d_val;
      uint64_t d_ptr;
    } d_un;
  };

  struct Elf64_Rela {
    uint64_t r_offset;
    uint64_t r_info;
    int64_t r_addend;
  };

  struct Elf64_Sym {
    uint32_t st_name;
    uint8_t st_info;
    uint8_t st_other;
    uint16_t st_shndx;
    uint64_t st_value;
    uint64_t st_size;
  };

  bool DetectAndParseSelf(const std::vector<uint8_t> &data);
  bool ParseHeaders(const std::vector<uint8_t> &data, Elf64_Ehdr &ehdr,
                    std::vector<Elf64_Phdr> &phdrs);
  bool MapSegments(const std::vector<uint8_t> &data,
                   const std::vector<Elf64_Phdr> &phdrs, uint64_t load_base);
  bool ApplyRelocations(const std::vector<uint8_t> &data,
                        const Elf64_Ehdr &ehdr,
                        const std::vector<Elf64_Phdr> &phdrs,
                        uint64_t load_base);
  bool HandleImports(const std::vector<uint8_t> &data, const Elf64_Ehdr &ehdr,
                     const std::vector<Elf64_Phdr> &phdrs);

  // For SELF files: resolve the actual file offset for a given ELF segment
  // Returns the file offset of the data, or SIZE_MAX on failure
  size_t ResolveSegmentFileOffset(const std::vector<Elf64_Phdr> &phdrs,
                                  const std::vector<uint8_t> &data,
                                  uint64_t desired_offset,
                                  uint64_t desired_size) const;
};

} // namespace Loader
} // namespace Core
