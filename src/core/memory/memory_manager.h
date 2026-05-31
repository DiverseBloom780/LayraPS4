// SPDX-FileCopyrightText: Copyright 2025 LayraPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <cstddef>
#include <cstdint>
#include <map>
#include <mutex>
#include <string>
#include <vector>

#include "address_space.h"

namespace Core {
namespace Memory {

// Address types (matching kernel_memory.h)
using VAddr = uint64_t;
using PAddr = uint64_t;

// Virtual Memory Area tracking
struct VirtualMemoryArea {
  VAddr base = 0;
  uint64_t size = 0;
  int32_t type = 0; // VMAType
  int32_t prot = 0; // Protection flags
  std::string name;
  int64_t phys_addr = -1; // Backing physical address (-1 = none)
};

// Physical Memory Area tracking
struct PhysicalMemoryArea {
  PAddr base = 0;
  uint64_t size = 0;
  int32_t memory_type = 0;
  bool allocated = false;
};

// PRT Aperture
struct PrtArea {
  VAddr start = 0;
  uint64_t size = 0;
  bool mapped = false;
};

class MemoryManager {
public:
  MemoryManager();
  ~MemoryManager();

  // --- Simple read/write (used by gui_debugger) ---
  void Read(uint64_t vaddr, void *dest, size_t size);
  void Write(uint64_t vaddr, const void *src, size_t size);
  bool Map(uint64_t vaddr, uint64_t size, uint32_t flags, const char *name);

  // --- PS4 Memory Management API (called by kernel_memory bridge) ---

  // Direct memory
  uint64_t GetTotalDirectSize() const { return TOTAL_DIRECT_SIZE; }
  PAddr Allocate(int64_t searchStart, int64_t searchEnd, uint64_t len,
                 uint64_t alignment, int32_t memoryType);
  int32_t Free(uint64_t start, uint64_t len, bool checked);
  int32_t DirectQueryAvailable(uint64_t searchStart, uint64_t searchEnd,
                               uint64_t alignment, PAddr *physAddr,
                               uint64_t *size);

  int32_t DirectMemoryQuery(uint64_t offset, bool extended, void *info);
  int32_t GetDirectMemoryType(uint64_t addr, int32_t *typeOut, void **startOut,
                              void **endOut);
  void SetDirectMemoryType(VAddr addr, uint64_t len, int32_t type);

  // Virtual memory
  int32_t VirtualQuery(VAddr addr, int32_t flags, void *info);
  int32_t MapMemory(void **addr, VAddr in_addr, uint64_t len, int32_t prot,
                    int32_t flags, int32_t vma_type, const char *name,
                    bool should_check, int64_t phys_addr, uint64_t alignment);
  int32_t UnmapMemory(VAddr addr, uint64_t len);
  int32_t QueryProtection(VAddr addr, void **start, void **end, uint32_t *prot);
  int32_t Protect(VAddr addr, uint64_t size, int32_t prot);

  int32_t IsStack(VAddr addr, void **start, void **end);
  void NameVirtualRange(VAddr addr, uint64_t len, const char *name);

  // Flexible memory
  uint64_t GetTotalFlexibleSize() const { return TOTAL_FLEXIBLE_SIZE; }
  uint64_t GetAvailableFlexibleSize() const;

  // Memory pool
  PAddr PoolExpand(uint64_t searchStart, uint64_t searchEnd, uint64_t len,
                   uint64_t alignment);
  int32_t PoolCommit(VAddr addr, uint64_t len, int32_t prot, int32_t type);
  int32_t PoolDecommit(VAddr addr, uint64_t len);
  void GetMemoryPoolStats(void *stats);

  // File mapping
  int32_t MapFile(void **addr, VAddr in_addr, uint64_t len, int32_t prot,
                  int32_t flags, int32_t fd, int64_t offset);

  // PRT
  void SetPrtArea(int32_t id, VAddr address, uint64_t size);

  // Singleton access
  static MemoryManager *Instance();

  // Bridge helper
  static MemoryManager *GetInstance() { return Instance(); }

  // Core functionality
  bool Initialize();

  // Address Space Access
  AddressSpace &GetAddressSpace() { return *address_space_; }

  // Translate guest virtual address to host pointer. Returns nullptr if the
  // address is invalid or memory has not been initialized.
  inline void *GetHostPtr(VAddr guest_addr) {
    if (!base_addr_)
      return nullptr;
    return static_cast<void *>(base_addr_ + guest_addr);
  }

private:
  static constexpr uint64_t TOTAL_DIRECT_SIZE = 0x160000000ULL;  // 5.5 GB
  static constexpr uint64_t TOTAL_FLEXIBLE_SIZE = 0x1DC00000ULL; // 448 MB
  static constexpr int MAX_PRT_AREAS = 3;

  std::mutex mutex_;
  std::unique_ptr<AddressSpace> address_space_; // Backing implementation

  // Tracking structures
  std::map<VAddr, VirtualMemoryArea> vma_map_;
  std::map<PAddr, PhysicalMemoryArea> phys_map_;
  PrtArea prt_areas_[MAX_PRT_AREAS] = {};

  uint64_t next_phys_addr_ = 0;
  uint64_t flexible_used_ = 0;
  uint64_t pool_used_ = 0;

  // Global singleton
  static MemoryManager *s_instance_;

  // Helpers
  VAddr FindFreeVirtualRange(uint64_t size, uint64_t alignment);

  uint8_t *base_addr_ = nullptr; // Base address of the reserved user space
};

} // namespace Memory
} // namespace Core
