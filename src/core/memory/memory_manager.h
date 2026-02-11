// SPDX-FileCopyrightText: Copyright 2025 LayraPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"
#include <cstdint>
#include <map>
#include <mutex>
#include <vector>

namespace Core {
namespace Memory {

struct MemoryRegion {
  uint64_t vaddr;
  uint64_t size;
  uint32_t prot; // Protection flags (read/write/exec)
  std::string name;
  std::vector<uint8_t> data;
};

class MemoryManager {
public:
  MemoryManager();
  ~MemoryManager();

  // Map a region of memory
  uint64_t Map(uint64_t vaddr, uint64_t size, uint32_t prot,
               const std::string &name);

  // Unmap a region
  void Unmap(uint64_t vaddr, uint64_t size);

  // Protect a region
  void Protect(uint64_t vaddr, uint64_t size, uint32_t prot);

  // Read/Write helpers (stubs for physical backing)
  void Read(uint64_t vaddr, void *data, uint64_t size);
  void Write(uint64_t vaddr, const void *data, uint64_t size);

  // Get the list of mapped regions
  const std::map<uint64_t, MemoryRegion> &GetRegions() const { return regions; }

private:
  std::map<uint64_t, MemoryRegion> regions;
  std::mutex mutex;

  // Find a free gap in memory for anonymous mapping
  uint64_t FindGap(uint64_t size);
};

} // namespace Memory
} // namespace Core
