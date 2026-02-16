// SPDX-FileCopyrightText: Copyright 2025 LayraPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "address_space.h"
#include <cstdio>
#include <memoryapi.h>
#include <windows.h>


// Link with OneCore.lib for VirtualAlloc2 etc. usually, or load dynamically
// For simplicity in this project, we'll try to use standard APIs where possible
// or load function pointers if needed.

namespace Core::Memory {

// Helper to convert internal Protection enum to Windows PAGE_ flags
static DWORD ToWindowsProt(Protection prot) {
  switch (prot) {
  case Protection::NoAccess:
    return PAGE_NOACCESS;
  case Protection::Read:
    return PAGE_READONLY;
  case Protection::Write:
    return PAGE_READWRITE; // Windows doesn't support Write-Only usually
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
  HANDLE backing_file_handle;
  HANDLE mapping_handle;
  uint8_t *backing_base; // Base of the physical backing view
  uint64_t backing_size;

  // We reserve a massive contiguous region for the emulated user space
  // 0x200000000 start roughly
  static constexpr uint64_t USER_SPACE_START = 0x200000000ULL;
  static constexpr uint64_t USER_SPACE_SIZE =
      0x1000000000ULL; // 64GB range? Or PS4 is smaller?
  // ShadPS4 uses massive ranges. Let's start with 512GB reservation to be safe.
  static constexpr uint64_t RESERVATION_SIZE = 0x8000000000ULL; // 512 GB

  Impl() {
    process_handle = GetCurrentProcess();
    backing_file_handle = INVALID_HANDLE_VALUE;
    mapping_handle = NULL;
    backing_base = nullptr;
    backing_size = 0;
  }

  ~Impl() {
    if (backing_base)
      UnmapViewOfFile(backing_base);
    if (mapping_handle)
      CloseHandle(mapping_handle);
    if (backing_file_handle != INVALID_HANDLE_VALUE)
      CloseHandle(backing_file_handle);
  }

  // Dynamically load VirtualAlloc2 if needed, or use VirtualAlloc.
  // Since we are targeting Windows 10/11, VirtualAlloc2 might be available.
  // But standard VirtualAlloc can reserve 512GB on 64-bit easily too if headers
  // are tricky.
};

AddressSpace::AddressSpace() : impl_(std::make_unique<Impl>()) {
  // 1. Create a backing file (swap) for physical memory
  // PS4 has 8GB shared GDDR5. Let's create an 8GB backing file.
  // Using 8GB + 512MB flexible + extra. Let's start with 9GB.
  uint64_t initial_backing = 9ULL * 1024 * 1024 * 1024;

  // Create unnamed swap file
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

  // Map the backing store into the host process so we can write to it (DMA
  // emulation)
  impl_->backing_base =
      (uint8_t *)MapViewOfFile(impl_->mapping_handle, FILE_MAP_ALL_ACCESS, 0, 0,
                               0 // Map entire object
      );

  if (!impl_->backing_base) {
    fprintf(stderr, "[AddressSpace] Failed to map backing store: %lu\n",
            GetLastError());
    return;
  }
  impl_->backing_size = initial_backing;

  // 2. Reserve the user virtual address space
  // We want a fixed base if possible to mimic PS4 pointers, e.g. 0x200000000
  // VirtualAlloc can accept a specific address.

  void *reserved =
      VirtualAlloc((void *)Impl::USER_SPACE_START, Impl::RESERVATION_SIZE,
                   MEM_RESERVE, PAGE_NOACCESS);

  // If exact specific address failed, try letting OS pick (but this breaks
  // fixed pointer assumptions) For an emulator, we really want fixed if we can.
  if (!reserved) {
    fprintf(stderr,
            "[AddressSpace] Failed to reserve user space at 0x%llx, trying "
            "arbitrary...\n",
            Impl::USER_SPACE_START);
    reserved =
        VirtualAlloc(NULL, Impl::RESERVATION_SIZE, MEM_RESERVE, PAGE_NOACCESS);
  }

  if (!reserved) {
    fprintf(
        stderr,
        "[AddressSpace] CRITICAL: Failed to reserve address space! error=%lu\n",
        GetLastError());
    return;
  }

  base_ptr_ = (uint8_t *)reserved;
  total_size_ = Impl::RESERVATION_SIZE;

  printf("[AddressSpace] Reserved %llu GB at %p\n",
         total_size_ / (1024 * 1024 * 1024), base_ptr_);
  printf("[AddressSpace] Backing store: %llu GB at %p\n",
         impl_->backing_size / (1024 * 1024 * 1024), impl_->backing_base);
}

AddressSpace::~AddressSpace() {
  if (base_ptr_) {
    VirtualFree(base_ptr_, 0, MEM_RELEASE);
  }
}

void *AddressSpace::Map(uint64_t vaddr, uint64_t size, uint64_t phys_offset,
                        Protection prot) {
  if (!base_ptr_)
    return nullptr;

  // Ensure vaddr is within our reserved range
  if (vaddr < (uint64_t)base_ptr_ ||
      vaddr + size > (uint64_t)base_ptr_ + total_size_) {
    fprintf(stderr, "[AddressSpace] Map out of bounds: 0x%llx size 0x%llx\n",
            vaddr, size);
    return nullptr;
  }

  DWORD win_prot = ToWindowsProt(prot);

  if (phys_offset != (uint64_t)-1) {
    // Map from backing file (Physical Memory)
    // We use MapViewOfFile3/2/Ex to map a section of the backing handle into
    // the reserved region. N.B. Windows VirtualAlloc cannot map a file view.
    // implementing "Map" on top of reserved memory usually mandates using
    // MapViewOfFileEx with the handle. But we already reserved it with
    // VirtualAlloc. To MapView over it, we must VirtualFree that slice first OR
    // use placeholders (VirtualAlloc2).

    // Since we are using standard VirtualAlloc for simplicity (avoiding
    // extensive API definition code), we will use the "Free-then-Map" strategy
    // for this slice. Unmap the reservation for this slice so MapViewOfFileEx
    // can take it.

    // Note: This creates a hole if MapViewOfFileEx fails, but for an emulator
    // it's usually fatal anyway.
    VirtualFree((void *)vaddr, size, MEM_RELEASE);

    // Offset high/low
    DWORD offset_high = (DWORD)(phys_offset >> 32);
    DWORD offset_low = (DWORD)(phys_offset & 0xFFFFFFFF);

    void *result = MapViewOfFileEx(
        impl_->mapping_handle,
        FILE_MAP_ALL_ACCESS |
            FILE_MAP_EXECUTE, // Protections are usually refined later
        offset_high, offset_low, size, (void *)vaddr);

    if (!result) {
      fprintf(stderr, "[AddressSpace] MapViewOfFileEx failed at 0x%llx: %lu\n",
              vaddr, GetLastError());
      // Attempt to recover reservation?
      return nullptr;
    }

    // Apply correct protection
    DWORD old;
    VirtualProtect(result, size, win_prot, &old);

    return result;

  } else {
    // Anonymous memory (VirtualAlloc COMMIT)
    // Since we reserved with VM, we can just Commit.
    void *result = VirtualAlloc((void *)vaddr, size, MEM_COMMIT, win_prot);
    if (!result) {
      fprintf(stderr,
              "[AddressSpace] VirtualAlloc COMMIT failed at 0x%llx: %lu\n",
              vaddr, GetLastError());
    }
    return result;
  }
}

void AddressSpace::Unmap(uint64_t vaddr, uint64_t size) {
  if (!base_ptr_)
    return;

  // If it was a file mapping (View), we must UnmapViewOfFile.
  // If it was VirtualAlloc committed, we should Decommit.
  // Tracking which is which is hard without metadata.
  // HOWEVER, for a reserved region, UnmapViewOfFile usually fails if it's not a
  // view. Robust strategy: Try UnmapViewOfFile. If it fails, try
  // VirtualFree(DECOMMIT). Wait, if we used the "Free-then-Map" strategy, the
  // UnmapViewOfFile releases the address. We then need to re-reserve it to keep
  // the region contiguous? Maintaining a fully reserved contiguous block is
  // hard with mixed APIs without placeholders (VirtualAlloc2).

  // For this implementation, we will assume:
  // 1. Try UnmapViewOfFile. If success, we have a hole. We should
  // VirtualAlloc(RESERVE) it back.
  // 2. If valid, VirtualFree(DECOMMIT).

  if (UnmapViewOfFile((void *)vaddr)) {
    // We successfully unmapped a view. Now the address is free.
    // Re-reserve it to keep our "Total Size" logic consistent.
    VirtualAlloc((void *)vaddr, size, MEM_RESERVE, PAGE_NOACCESS);
  } else {
    // Maybe it was just committed RAM. Decommit it.
    VirtualFree((void *)vaddr, size, MEM_DECOMMIT);
  }
}

void AddressSpace::Protect(uint64_t vaddr, uint64_t size, Protection prot) {
  DWORD win_prot = ToWindowsProt(prot);
  DWORD old;
  VirtualProtect((void *)vaddr, size, win_prot, &old);
}

void AddressSpace::ResizeBacking(uint64_t new_size) {
  // TODO: Implement resize (requires closing handle, creating new mapping)
  // For now, fixed startup size is enough for Phase 1.
}

} // namespace Core::Memory
