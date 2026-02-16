// SPDX-FileCopyrightText: Copyright 2025 LayraPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "memory_manager.h"
#include "../kernel/kernel_memory.h" // For OrbisVirtualQueryInfo, OrbisKernelMemoryPoolBlockStats
#include <algorithm>
#include <cstdio>
#include <cstring>

namespace Core::Memory {

// Singleton
MemoryManager *MemoryManager::s_instance_ = nullptr;

MemoryManager::MemoryManager() {
  printf("[MemoryManager] Initialized (direct: %llu MB, flexible: %llu MB)\n",
         TOTAL_DIRECT_SIZE / (1024 * 1024),
         TOTAL_FLEXIBLE_SIZE / (1024 * 1024));

  // Initialize free physical memory region covering the entire direct memory
  // space
  PhysicalMemoryArea free_area{};
  free_area.base = 0;
  free_area.size = TOTAL_DIRECT_SIZE;
  free_area.memory_type = 0;
  free_area.allocated = false;
  phys_map_[0] = free_area;

  // Initialize AddressSpace backend
  address_space_ = std::make_unique<AddressSpace>();

  if (!address_space_->GetBase()) {
    fprintf(stderr,
            "[MemoryManager] FATAL: Failed to initialize AddressSpace!\n");
  }

  // Set singleton
  s_instance_ = this;
}

MemoryManager::~MemoryManager() {
  printf("[MemoryManager] Shutdown\n");
  if (s_instance_ == this) {
    s_instance_ = nullptr;
  }
}

MemoryManager *MemoryManager::Instance() { return s_instance_; }

bool MemoryManager::Initialize() {
  base_addr_ = static_cast<uint8_t *>(address_space_->GetBase());
  if (base_addr_) {
    printf("[MemoryManager] AddressSpace base: %p\n", base_addr_);
  } else {
    fprintf(stderr, "[MemoryManager] CRITICAL: AddressSpace has no base!\n");
  }
  return base_addr_ != nullptr;
}

// --- Simple read/write (used by gui_debugger) ---

void MemoryManager::Read(uint64_t vaddr, void *dest, size_t size) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto it = vma_map_.upper_bound(vaddr);
  if (it != vma_map_.begin()) {
    it--;
    if (vaddr >= it->second.base &&
        vaddr + size <= it->second.base + it->second.size) {
      if (base_addr_) {
        void *host_ptr = GetHostPtr(vaddr);
        if (host_ptr) {
          memcpy(dest, host_ptr, size);
          return;
        }
      }
    }
  }
  std::memset(dest, 0, size);
}

void MemoryManager::Write(uint64_t vaddr, const void *src, size_t size) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto it = vma_map_.upper_bound(vaddr);
  if (it != vma_map_.begin()) {
    it--;
    if (vaddr >= it->second.base &&
        vaddr + size <= it->second.base + it->second.size) {
      if (it->second.prot & 2) { // Write permission
        if (base_addr_) {
          void *host_ptr = GetHostPtr(vaddr);
          if (host_ptr) {
            memcpy(host_ptr, src, size);
          }
        }
      }
    }
  }
}

void MemoryManager::Map(uint64_t vaddr, uint64_t size, uint32_t flags,
                        const char *name) {
  MapMemory(nullptr, vaddr, size, flags, 0, 0, name, false, -1, 0);
}

// --- PS4 Direct Memory ---

PAddr MemoryManager::Allocate(int64_t searchStart, int64_t searchEnd,
                              uint64_t len, uint64_t alignment,
                              int32_t memoryType) {
  std::lock_guard<std::mutex> lock(mutex_);

  if (alignment == 0)
    alignment = 0x4000; // 16KB default

  // Simple bump allocator
  PAddr addr = (next_phys_addr_ + alignment - 1) & ~(alignment - 1);

  if (addr + len > TOTAL_DIRECT_SIZE) {
    fprintf(stderr, "[MemoryManager] ERROR: Out of direct memory\n");
    return static_cast<PAddr>(-1);
  }

  PhysicalMemoryArea pma{};
  pma.base = addr;
  pma.size = len;
  pma.memory_type = memoryType;
  pma.allocated = true;
  phys_map_[addr] = pma;

  next_phys_addr_ = addr + len;

  printf("[MemoryManager] Allocated direct memory: 0x%llx, size: 0x%llx, type: "
         "%d\n",
         static_cast<unsigned long long>(addr),
         static_cast<unsigned long long>(len), memoryType);
  return addr;
}

int32_t MemoryManager::Free(uint64_t start, uint64_t len, bool checked) {
  std::lock_guard<std::mutex> lock(mutex_);

  auto it = phys_map_.find(start);
  if (it != phys_map_.end()) {
    it->second.allocated = false;
    printf("[MemoryManager] Freed direct memory: 0x%llx, size: 0x%llx\n",
           static_cast<unsigned long long>(start),
           static_cast<unsigned long long>(len));
    return 0; // ORBIS_OK
  }

  if (checked) {
    return 0x80020016; // ORBIS_KERNEL_ERROR_EINVAL
  }
  return 0; // ORBIS_OK
}

int32_t MemoryManager::DirectQueryAvailable(uint64_t searchStart,
                                            uint64_t searchEnd,
                                            uint64_t alignment, PAddr *physAddr,
                                            uint64_t *size) {
  std::lock_guard<std::mutex> lock(mutex_);

  uint64_t available = TOTAL_DIRECT_SIZE > next_phys_addr_
                           ? TOTAL_DIRECT_SIZE - next_phys_addr_
                           : 0;
  PAddr addr = (next_phys_addr_ + alignment - 1) & ~(alignment - 1);

  if (physAddr)
    *physAddr = addr;
  if (size)
    *size = available;
  return 0;
}

int32_t MemoryManager::DirectMemoryQuery(uint64_t offset, bool extended,
                                         void *info) {
  if (info)
    std::memset(info, 0, 24);
  return 0;
}

int32_t MemoryManager::GetDirectMemoryType(uint64_t addr, int32_t *typeOut,
                                           void **startOut, void **endOut) {
  std::lock_guard<std::mutex> lock(mutex_);
  for (auto &[base, pma] : phys_map_) {
    if (addr >= base && addr < base + pma.size) {
      if (typeOut)
        *typeOut = pma.memory_type;
      if (startOut)
        *startOut = reinterpret_cast<void *>(base);
      if (endOut)
        *endOut = reinterpret_cast<void *>(base + pma.size);
      return 0;
    }
  }
  return 0x80020016;
}

void MemoryManager::SetDirectMemoryType(VAddr addr, uint64_t len,
                                        int32_t type) {
  std::lock_guard<std::mutex> lock(mutex_);
  for (auto &[base, pma] : phys_map_) {
    if (addr >= base && addr < base + pma.size) {
      pma.memory_type = type;
      return;
    }
  }
}

// --- Virtual Memory ---

int32_t MemoryManager::VirtualQuery(VAddr addr, int32_t flags, void *info) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (info)
    std::memset(info, 0, sizeof(OrbisVirtualQueryInfo));
  return 0;
}

int32_t MemoryManager::MapMemory(void **addr, VAddr in_addr, uint64_t len,
                                 int32_t prot, int32_t flags, int32_t vma_type,
                                 const char *name, bool should_check,
                                 int64_t phys_addr, uint64_t alignment) {
  std::lock_guard<std::mutex> lock(mutex_);

  VAddr target_addr = in_addr;

  if (target_addr == 0) {
    if (alignment == 0)
      alignment = 0x4000;
    target_addr = FindFreeVirtualRange(len, alignment);
  }

  VirtualMemoryArea vma{};
  vma.base = target_addr;
  vma.size = len;
  vma.type = vma_type;
  vma.prot = prot;
  vma.name = name ? name : "anon";
  vma.phys_addr = phys_addr;
  vma_map_[target_addr] = vma;

  if (vma_type == 3) {
    flexible_used_ += len;
  }

  Protection p = Protection::NoAccess;
  if (prot & 1)
    p = (Protection)((uint32_t)p | (uint32_t)Protection::Read);
  if (prot & 2)
    p = (Protection)((uint32_t)p | (uint32_t)Protection::Write);
  if (prot & 4)
    p = (Protection)((uint32_t)p | (uint32_t)Protection::Execute);

  // Calculate Host Address relative to Base
  // AddressSpace Map expects a HOST ADDRESS if reserved.
  // Wait, if AddressSpace::Map expects a host address (as VAddr), does it check
  // if it falls in range? Yes. So we pass base_addr_ + target_addr

  uint64_t host_vaddr = (uint64_t)base_addr_ + target_addr;

  void *result = address_space_->Map(host_vaddr, len, (uint64_t)phys_addr, p);

  if (!result) {
    fprintf(stderr,
            "[MemoryManager] AddressSpace Map failed! Guest: 0x%llx, Host: "
            "0x%llx\n",
            target_addr, host_vaddr);
    // Clean up VMA?
    vma_map_.erase(target_addr);
    return 0x80020016; // ENOMEM
  }

  if (addr)
    *addr = result; // Return Host Pointer

  printf("[MemoryManager] Mapped: Guest 0x%llx -> Host 0x%llx, size: 0x%llx, "
         "type: %d, name: '%s'\n",
         static_cast<unsigned long long>(target_addr),
         static_cast<unsigned long long>(host_vaddr),
         static_cast<unsigned long long>(len), vma_type, name);
  return 0;
}

int32_t MemoryManager::UnmapMemory(VAddr addr, uint64_t len) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto it = vma_map_.find(addr);
  if (it != vma_map_.end()) {
    if (it->second.type == 3) {
      flexible_used_ = flexible_used_ > len ? flexible_used_ - len : 0;
    }
    vma_map_.erase(it);

    // Address Translation
    if (base_addr_) {
      address_space_->Unmap((uint64_t)base_addr_ + addr, len);
    }
    return 0;
  }
  return 0;
}

int32_t MemoryManager::QueryProtection(VAddr addr, void **start, void **end,
                                       uint32_t *prot) {
  std::lock_guard<std::mutex> lock(mutex_);
  for (auto &[base, vma] : vma_map_) {
    if (addr >= base && addr < base + vma.size) {
      // Return GUEST addresses for query
      if (start)
        *start = reinterpret_cast<void *>(base);
      if (end)
        *end = reinterpret_cast<void *>(base + vma.size);
      if (prot)
        *prot = static_cast<uint32_t>(vma.prot);
      return 0;
    }
  }
  return 0x80020016;
}

int32_t MemoryManager::Protect(VAddr addr, uint64_t size, int32_t prot) {
  std::lock_guard<std::mutex> lock(mutex_);
  for (auto &[base, vma] : vma_map_) {
    if (addr >= base && addr < base + vma.size) {
      vma.prot = prot;
      Protection p = Protection::NoAccess;
      if (prot & 1)
        p = (Protection)((uint32_t)p | (uint32_t)Protection::Read);
      if (prot & 2)
        p = (Protection)((uint32_t)p | (uint32_t)Protection::Write);
      if (prot & 4)
        p = (Protection)((uint32_t)p | (uint32_t)Protection::Execute);

      // Address Translation
      if (base_addr_) {
        address_space_->Protect((uint64_t)base_addr_ + base, vma.size, p);
      }
      return 0;
    }
  }
  return 0;
}

int32_t MemoryManager::IsStack(VAddr addr, void **start, void **end) {
  std::lock_guard<std::mutex> lock(mutex_);
  for (auto &[base, vma] : vma_map_) {
    if (addr >= base && addr < base + vma.size && vma.type == 6) {
      if (start)
        *start = reinterpret_cast<void *>(base);
      if (end)
        *end = reinterpret_cast<void *>(base + vma.size);
      return 0;
    }
  }
  return 0x80020016;
}

void MemoryManager::NameVirtualRange(VAddr addr, uint64_t len,
                                     const char *name) {
  std::lock_guard<std::mutex> lock(mutex_);
  for (auto &[base, vma] : vma_map_) {
    if (addr >= base && addr < base + vma.size) {
      vma.name = name ? name : "";
      return;
    }
  }
}

// --- Flexible Memory ---

uint64_t MemoryManager::GetAvailableFlexibleSize() const {
  return TOTAL_FLEXIBLE_SIZE > flexible_used_
             ? TOTAL_FLEXIBLE_SIZE - flexible_used_
             : 0;
}

// --- Memory Pool ---

PAddr MemoryManager::PoolExpand(uint64_t searchStart, uint64_t searchEnd,
                                uint64_t len, uint64_t alignment) {
  return Allocate(static_cast<int64_t>(searchStart),
                  static_cast<int64_t>(searchEnd), len, alignment, 0);
}

int32_t MemoryManager::PoolCommit(VAddr addr, uint64_t len, int32_t prot,
                                  int32_t type) {
  std::lock_guard<std::mutex> lock(mutex_);
  for (auto &[base, vma] : vma_map_) {
    if (addr >= base && addr < base + vma.size) {
      vma.prot = prot;
      vma.type = 4; // Pooled
      pool_used_ += len;
      return 0;
    }
  }
  return 0x80020016;
}

int32_t MemoryManager::PoolDecommit(VAddr addr, uint64_t len) {
  std::lock_guard<std::mutex> lock(mutex_);
  for (auto &[base, vma] : vma_map_) {
    if (addr >= base && addr < base + vma.size) {
      vma.prot = 0;
      pool_used_ = pool_used_ > len ? pool_used_ - len : 0;
      return 0;
    }
  }
  return 0x80020016;
}

void MemoryManager::GetMemoryPoolStats(void *stats) {
  if (!stats)
    return;
  auto *s = static_cast<OrbisKernelMemoryPoolBlockStats *>(stats);
  s->size = TOTAL_DIRECT_SIZE;
  s->free_size = TOTAL_DIRECT_SIZE > next_phys_addr_
                     ? TOTAL_DIRECT_SIZE - next_phys_addr_
                     : 0;
  s->committed_size = pool_used_;
}

// --- File Mapping ---

int32_t MemoryManager::MapFile(void **addr, VAddr in_addr, uint64_t len,
                               int32_t prot, int32_t flags, int32_t fd,
                               int64_t offset) {
  return MapMemory(addr, in_addr, len, prot, flags, 8, "file", false, -1, 0);
}

// --- PRT ---

void MemoryManager::SetPrtArea(int32_t id, VAddr address, uint64_t size) {
  if (id >= 0 && id < MAX_PRT_AREAS) {
    prt_areas_[id].start = address;
    prt_areas_[id].size = size;
    prt_areas_[id].mapped = true;
  }
}

// --- Helpers ---

VAddr MemoryManager::FindFreeVirtualRange(uint64_t size, uint64_t alignment) {
  constexpr VAddr SEARCH_START = 0x200000000ULL;
  VAddr candidate = SEARCH_START;

  if (alignment > 0) {
    candidate = (candidate + alignment - 1) & ~(alignment - 1);
  }

  for (auto &[base, vma] : vma_map_) {
    VAddr vma_end = base + vma.size;
    if (candidate + size > base) {
      candidate = (vma_end + alignment - 1) & ~(alignment - 1);
    }
  }

  return candidate;
}

} // namespace Core::Memory
