// SPDX-FileCopyrightText: Copyright 2025 LayraPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "address_space.h"
#include <cstdio>
#include <windows.h>

namespace Core::Memory {

// Helper to convert internal Protection enum to Windows PAGE_ flags
static DWORD ToWindowsProt(Protection prot) {
  switch (prot) {
  case Protection::NoAccess:
    return PAGE_NOACCESS;
  case Protection::Read:
    return PAGE_READONLY;
  case Protection::Write:
    return PAGE_READWRITE; // Windows doesn't support Write-Only
  case Protection::ReadWrite:
    return PAGE_READWRITE;
  case Protection::Execute:
    return PAGE_EXECUTE;
  case Protection::ReadExecute:
    return PAGE_EXECUTE_READ;
  case Protection::ReadWriteExecute:
    return PAGE_EXECUTE_READWRITE;
  default:
    return PAGE_NOACCESS;
  }
}

struct AddressSpace::Impl {
  HANDLE process_handle;
  HANDLE mapping_handle;

  Impl() {
    process_handle = GetCurrentProcess();
    mapping_handle = NULL;
  }

  ~Impl() {
    if (mapping_handle)
      CloseHandle(mapping_handle);
  }
};

AddressSpace::AddressSpace() : impl_(std::make_unique<Impl>()) {
  // Identity-mapped address space: we do NOT reserve a huge contiguous block.
  // Instead, we allocate memory at exact guest virtual addresses on demand.
  //
  // We still create a backing file for physical memory (DMA emulation).
  // PS4 has 8GB shared GDDR5. Create a 9GB backing.
  uint64_t initial_backing = 9ULL * 1024 * 1024 * 1024;

  // Create unnamed swap file for physical memory backing
  impl_->mapping_handle = CreateFileMappingA(
      INVALID_HANDLE_VALUE, // Use paging file
      NULL, PAGE_EXECUTE_READWRITE, (DWORD)(initial_backing >> 32),
      (DWORD)(initial_backing & 0xFFFFFFFF), NULL);

  if (!impl_->mapping_handle) {
    fprintf(stderr,
            "[AddressSpace] Failed to create backing file mapping: %lu\n",
            GetLastError());
    return;
  }

  // Map the backing store so we can write to it (DMA emulation)
  backing_base_ =
      (uint8_t *)MapViewOfFile(impl_->mapping_handle, FILE_MAP_ALL_ACCESS, 0, 0,
                               0 // Map entire object
      );

  if (!backing_base_) {
    fprintf(stderr, "[AddressSpace] Failed to map backing store: %lu\n",
            GetLastError());
    return;
  }
  backing_size_ = initial_backing;
  initialized_ = true;

  printf("[AddressSpace] Identity-mapped address space initialized\n");
  printf("[AddressSpace] Backing store: %llu GB at %p\n",
         backing_size_ / (1024 * 1024 * 1024), backing_base_);
}

AddressSpace::~AddressSpace() {
  if (backing_base_)
    UnmapViewOfFile(backing_base_);
}

void *AddressSpace::Map(uint64_t vaddr, uint64_t size, uint64_t phys_offset,
                        Protection prot) {
  if (!initialized_)
    return nullptr;

  DWORD win_prot = ToWindowsProt(prot);

  if (phys_offset != (uint64_t)-1) {
    // Physical-backed mapping: map from backing file at exact guest address.
    DWORD offset_high = (DWORD)(phys_offset >> 32);
    DWORD offset_low = (DWORD)(phys_offset & 0xFFFFFFFF);

    void *result = MapViewOfFileEx(
        impl_->mapping_handle,
        FILE_MAP_ALL_ACCESS | FILE_MAP_EXECUTE,
        offset_high, offset_low, size, (void *)vaddr);

    if (!result) {
      fprintf(stderr, "[AddressSpace] MapViewOfFileEx failed at 0x%llx: %lu, "
                      "falling back to anonymous\n",
              (unsigned long long)vaddr, GetLastError());
      // Fall through to anonymous mapping below
    } else {
      DWORD old;
      VirtualProtect(result, size, win_prot, &old);
      return result;
    }
  }

  // Anonymous memory: try to commit at the exact guest address.
  // First try MEM_COMMIT only (if the address was already reserved).
  void *result = VirtualAlloc((void *)vaddr, size, MEM_COMMIT, win_prot);
  if (result) {
    return result;
  }

  // If that failed, try MEM_RESERVE | MEM_COMMIT (new allocation).
  result = VirtualAlloc((void *)vaddr, size,
                        MEM_RESERVE | MEM_COMMIT, win_prot);
  if (!result) {
    fprintf(stderr,
            "[AddressSpace] VirtualAlloc at 0x%llx size 0x%llx failed: %lu\n",
            (unsigned long long)vaddr, (unsigned long long)size,
            GetLastError());
  }
  return result;
}

void AddressSpace::Unmap(uint64_t vaddr, uint64_t size) {
  if (!initialized_)
    return;

  // Try UnmapViewOfFile first (for physical-backed mappings).
  // If that fails, try VirtualFree (for anonymous mappings).
  if (!UnmapViewOfFile((void *)vaddr)) {
    VirtualFree((void *)vaddr, 0, MEM_RELEASE);
  }
}

void AddressSpace::Protect(uint64_t vaddr, uint64_t size, Protection prot) {
  DWORD win_prot = ToWindowsProt(prot);
  DWORD old;
  if (!VirtualProtect((void *)vaddr, size, win_prot, &old)) {
    fprintf(stderr,
            "[AddressSpace] VirtualProtect at 0x%llx size 0x%llx failed: %lu\n",
            (unsigned long long)vaddr, (unsigned long long)size,
            GetLastError());
  }
}

void AddressSpace::ResizeBacking(uint64_t new_size) {
  // TODO: Implement resize (requires closing handle, creating new mapping)
  // For now, fixed startup size is enough for Phase 1.
}

} // namespace Core::Memory
