// SPDX-FileCopyrightText: Copyright 2025 LayraPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <vector>

namespace Core::Memory {

enum class Protection : uint32_t {
  NoAccess = 0,
  Read = 1 << 0,
  Write = 1 << 1,
  Execute = 1 << 2,
  ReadWrite = Read | Write,
  ReadExecute = Read | Execute,
  ReadWriteExecute = Read | Write | Execute
};

struct MemoryRegion {
  uint64_t base;
  uint64_t size;
  uint64_t phys_offset; // -1 if no physical backing
  bool is_mapped;
  Protection prot;
};

class AddressSpace {
public:
  AddressSpace();
  ~AddressSpace();

  // Direct access to base pointers
  uint8_t *GetBase() const { return base_ptr_; }
  uint64_t GetSize() const { return total_size_; }

  // Map memory at specific virtual address
  // If phys_offset is -1, maps anonymous memory
  // If phys_offset is valid, maps from the backing file
  void *Map(uint64_t vaddr, uint64_t size, uint64_t phys_offset,
            Protection prot);

  // Unmap memory at specific virtual address
  void Unmap(uint64_t vaddr, uint64_t size);

  // Change protection
  void Protect(uint64_t vaddr, uint64_t size, Protection prot);

  // Physical backing file management
  void ResizeBacking(uint64_t new_size);

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;

  uint8_t *base_ptr_ = nullptr;
  uint64_t total_size_ = 0;
};

} // namespace Core::Memory
