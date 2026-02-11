#include "memory_manager.h"
#include <algorithm>
#include <cstring>
#include <iostream>

namespace Core {
namespace Memory {

MemoryManager::MemoryManager() {
  std::cout << "[Memory] Memory Manager initialized.\n";
}

MemoryManager::~MemoryManager() {
  std::lock_guard<std::mutex> lock(mutex);
  for (auto &[addr, region] : regions) {
    region.data.clear();
    region.data.shrink_to_fit();
  }
}

uint64_t MemoryManager::Map(uint64_t vaddr, uint64_t size, uint32_t prot,
                            const std::string &name) {
  std::lock_guard<std::mutex> lock(mutex);

  if (vaddr == 0) {
    vaddr = FindGap(size);
  }

  // Robust overlap check
  for (const auto &[addr, existing_region] : regions) {
    if ((vaddr >= addr && vaddr < addr + existing_region.size) ||
        (vaddr + size > addr && vaddr + size <= addr + existing_region.size) ||
        (addr >= vaddr && addr < vaddr + size)) {
      std::cerr << "[Memory] ERROR: Region '" << name << "' at 0x" << std::hex
                << vaddr << " size 0x" << size
                << " overlaps with existing region '" << existing_region.name
                << "' at 0x" << addr << " size 0x" << existing_region.size
                << std::dec << "\n";
      return 0; // Allocation failed
    }
  }

  std::cout << "[Memory] Mapping region: " << name << " at 0x" << std::hex
            << vaddr << " size 0x" << size << " prot 0x" << prot << std::dec
            << "\n";

  MemoryRegion region;
  region.vaddr = vaddr;
  region.size = size;
  region.prot = prot;
  region.name = name;
  region.data.resize(size, 0);

  regions[vaddr] = std::move(region);
  return vaddr;
}

void MemoryManager::Unmap(uint64_t vaddr, uint64_t size) {
  std::lock_guard<std::mutex> lock(mutex);
  auto it = regions.find(vaddr);
  if (it != regions.end()) {
    std::cout << "[Memory] Unmapping region at 0x" << std::hex << vaddr << " ("
              << it->second.name << ")" << std::dec << "\n";
    it->second.data.clear();
    it->second.data.shrink_to_fit();
    regions.erase(it);
  }
}

void MemoryManager::Protect(uint64_t vaddr, uint64_t size, uint32_t prot) {
  std::lock_guard<std::mutex> lock(mutex);
  if (regions.count(vaddr)) {
    regions[vaddr].prot = prot;
  }
}

void MemoryManager::Read(uint64_t vaddr, void *data, uint64_t size) {
  std::lock_guard<std::mutex> lock(mutex);
  for (auto &[addr, region] : regions) {
    if (vaddr >= addr && vaddr + size <= addr + region.size) {
      std::memcpy(data, region.data.data() + (vaddr - addr), size);
      return;
    }
  }
  std::cerr << "[Memory] Read violation at 0x" << std::hex << vaddr << std::dec
            << "\n";
}

void MemoryManager::Write(uint64_t vaddr, const void *data, uint64_t size) {
  std::lock_guard<std::mutex> lock(mutex);
  for (auto &[addr, region] : regions) {
    if (vaddr >= addr && vaddr + size <= addr + region.size) {
      std::memcpy(region.data.data() + (vaddr - addr), data, size);
      return;
    }
  }
  std::cerr << "[Memory] Write violation at 0x" << std::hex << vaddr << std::dec
            << "\n";
}

uint64_t MemoryManager::FindGap(uint64_t size) {
  uint64_t start = 0x100000000;
  for (const auto &[addr, region] : regions) {
    if (addr >= start + size)
      return start;
    start = std::max(start, addr + region.size);
  }
  return start;
}

} // namespace Memory
} // namespace Core
