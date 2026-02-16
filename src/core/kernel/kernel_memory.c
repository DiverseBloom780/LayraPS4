// SPDX-FileCopyrightText: Copyright 2025 LayraPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "kernel_memory.h"
#include <stdio.h>
// Forced rebuild check
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

// Forward declarations for memory manager interface
// These functions should be implemented in your memory_manager.c
extern void *MemoryManager_GetInstance(void);
extern u64 MemoryManager_GetTotalDirectSize(void *memory);
extern PAddr MemoryManager_Allocate(void *memory, s64 searchStart,
                                    s64 searchEnd, u64 len, u64 alignment,
                                    s32 memoryType);
extern s32 MemoryManager_Free(void *memory, u64 start, u64 len, bool checked);
extern s32 MemoryManager_DirectQueryAvailable(void *memory, u64 searchStart,
                                              u64 searchEnd, u64 alignment,
                                              PAddr *physAddr, u64 *size);
extern s32 MemoryManager_VirtualQuery(void *memory, VAddr addr, s32 flags,
                                      OrbisVirtualQueryInfo *info);
extern s32 MemoryManager_MapMemory(void *memory, void **addr, VAddr in_addr,
                                   u64 len, s32 prot, s32 flags, s32 vma_type,
                                   const char *name, bool should_check,
                                   s64 phys_addr, u64 alignment);
extern s32 MemoryManager_QueryProtection(void *memory, VAddr addr, void **start,
                                         void **end, u32 *prot);
extern s32 MemoryManager_Protect(void *memory, VAddr addr, u64 size, s32 prot);
extern s32 MemoryManager_DirectMemoryQuery(void *memory, u64 offset,
                                           bool extended, OrbisQueryInfo *info);
extern u64 MemoryManager_GetAvailableFlexibleSize(void *memory);
extern s32 MemoryManager_GetDirectMemoryType(void *memory, u64 addr,
                                             s32 *typeOut, void **startOut,
                                             void **endOut);
extern s32 MemoryManager_IsStack(void *memory, VAddr addr, void **start,
                                 void **end);
extern void MemoryManager_SetDirectMemoryType(void *memory, VAddr addr, u64 len,
                                              s32 type);
extern void MemoryManager_NameVirtualRange(void *memory, VAddr addr, u64 len,
                                           const char *name);
extern u64 MemoryManager_GetTotalFlexibleSize(void *memory);
extern s32 MemoryManager_UnmapMemory(void *memory, VAddr addr, u64 len);
extern PAddr MemoryManager_PoolExpand(void *memory, u64 searchStart,
                                      u64 searchEnd, u64 len, u64 alignment);
extern s32 MemoryManager_PoolCommit(void *memory, VAddr addr, u64 len, s32 prot,
                                    s32 type);
extern s32 MemoryManager_PoolDecommit(void *memory, VAddr addr, u64 len);
extern void
MemoryManager_GetMemoryPoolStats(void *memory,
                                 OrbisKernelMemoryPoolBlockStats *stats);
extern s32 MemoryManager_MapFile(void *memory, void **addr, VAddr in_addr,
                                 u64 len, s32 prot, s32 flags, s32 fd,
                                 s64 offset);
extern void MemoryManager_SetPrtArea(void *memory, s32 id, VAddr address,
                                     u64 size);

// Alignment helpers
static inline bool Is16KBAligned(u64 value) {
  return (value & (PAGE_SIZE_16KB - 1)) == 0;
}

static inline bool Is64KBAligned(u64 value) {
  return (value & (PAGE_SIZE_64KB - 1)) == 0;
}

static inline bool Is2MBAligned(u64 value) {
  return (value & (PAGE_SIZE_2MB - 1)) == 0;
}

static inline bool IsPowerOfTwo(u64 value) {
  return value != 0 && (value & (value - 1)) == 0;
}

static inline u64 AlignDown(u64 value, u64 alignment) {
  return value & ~(alignment - 1);
}

static inline u64 AlignUp(u64 value, u64 alignment) {
  return (value + alignment - 1) & ~(alignment - 1);
}

// Global state
static s32 g_sdk_version = -1;
static bool g_alias_dmem = false;

// ============================================================================
// Core Memory Functions
// ============================================================================

u64 sceKernelGetDirectMemorySize(void) {
  printf("[Kernel_Memory] sceKernelGetDirectMemorySize called\n");
  void *memory = MemoryManager_GetInstance();
  return MemoryManager_GetTotalDirectSize(memory);
}

s32 sceKernelEnableDmemAliasing(void) {
  printf("[Kernel_Memory] sceKernelEnableDmemAliasing called\n");
  g_alias_dmem = true;
  return ORBIS_OK;
}

s32 sceKernelAllocateDirectMemory(s64 searchStart, s64 searchEnd, u64 len,
                                  u64 alignment, s32 memoryType,
                                  s64 *physAddrOut) {
  // Validation
  if (searchStart < 0 || searchEnd < 0) {
    fprintf(stderr, "[Kernel_Memory] ERROR: Invalid parameters!\n");
    return ORBIS_KERNEL_ERROR_EINVAL;
  }
  if (len <= 0 || !Is16KBAligned(len)) {
    fprintf(stderr, "[Kernel_Memory] ERROR: Length 0x%lx is invalid!\n", len);
    return ORBIS_KERNEL_ERROR_EINVAL;
  }
  if (alignment != 0 && !Is16KBAligned(alignment)) {
    fprintf(stderr, "[Kernel_Memory] ERROR: Alignment 0x%lx is invalid!\n",
            alignment);
    return ORBIS_KERNEL_ERROR_EINVAL;
  }
  if (memoryType > 10) {
    fprintf(stderr, "[Kernel_Memory] ERROR: Memory type 0x%x is invalid!\n",
            memoryType);
    return ORBIS_KERNEL_ERROR_EINVAL;
  }
  if (physAddrOut == NULL) {
    fprintf(
        stderr,
        "[Kernel_Memory] ERROR: Result physical address pointer is null!\n");
    return ORBIS_KERNEL_ERROR_EINVAL;
  }

  bool is_in_range = (searchEnd - searchStart) >= (s64)len;
  if (searchEnd <= searchStart || searchEnd < (s64)len || !is_in_range) {
    fprintf(stderr,
            "[Kernel_Memory] ERROR: Provided address range is too small! "
            "searchStart = 0x%lx, searchEnd = 0x%lx, length = 0x%lx\n",
            searchStart, searchEnd, len);
    return ORBIS_KERNEL_ERROR_EAGAIN;
  }

  void *memory = MemoryManager_GetInstance();
  PAddr phys_addr = MemoryManager_Allocate(memory, searchStart, searchEnd, len,
                                           alignment, memoryType);
  if (phys_addr == (PAddr)-1) {
    return ORBIS_KERNEL_ERROR_EAGAIN;
  }

  *physAddrOut = (s64)phys_addr;

  printf("[Kernel_Memory] sceKernelAllocateDirectMemory: searchStart = 0x%lx, "
         "searchEnd = 0x%lx, "
         "len = 0x%lx, alignment = 0x%lx, memoryType = 0x%x, physAddrOut = "
         "0x%lx\n",
         searchStart, searchEnd, len, alignment, memoryType, phys_addr);

  return ORBIS_OK;
}

s32 sceKernelAllocateMainDirectMemory(u64 len, u64 alignment, s32 memoryType,
                                      s64 *physAddrOut) {
  s64 searchEnd = (s64)sceKernelGetDirectMemorySize();
  return sceKernelAllocateDirectMemory(0, searchEnd, len, alignment, memoryType,
                                       physAddrOut);
}

s32 sceKernelCheckedReleaseDirectMemory(u64 start, u64 len) {
  printf("[Kernel_Memory] sceKernelCheckedReleaseDirectMemory: start = 0x%lx, "
         "len = 0x%lx\n",
         start, len);
  if (!Is16KBAligned(start) || !Is16KBAligned(len)) {
    fprintf(stderr,
            "[Kernel_Memory] ERROR: Misaligned start or length, start = 0x%lx, "
            "length = 0x%lx\n",
            start, len);
    return ORBIS_KERNEL_ERROR_EINVAL;
  }
  if (len == 0) {
    return ORBIS_OK;
  }
  void *memory = MemoryManager_GetInstance();
  return MemoryManager_Free(memory, start, len, true);
}

s32 sceKernelReleaseDirectMemory(u64 start, u64 len) {
  printf("[Kernel_Memory] sceKernelReleaseDirectMemory: start = 0x%lx, len = "
         "0x%lx\n",
         start, len);
  if (!Is16KBAligned(start) || !Is16KBAligned(len)) {
    fprintf(stderr,
            "[Kernel_Memory] ERROR: Misaligned start or length, start = 0x%lx, "
            "length = 0x%lx\n",
            start, len);
    return ORBIS_KERNEL_ERROR_EINVAL;
  }
  if (len == 0) {
    return ORBIS_OK;
  }
  void *memory = MemoryManager_GetInstance();
  MemoryManager_Free(memory, start, len, false);
  return ORBIS_OK;
}

s32 sceKernelAvailableDirectMemorySize(u64 searchStart, u64 searchEnd,
                                       u64 alignment, u64 *physAddrOut,
                                       u64 *sizeOut) {
  printf("[Kernel_Memory] sceKernelAvailableDirectMemorySize: searchStart = "
         "0x%lx, "
         "searchEnd = 0x%lx, alignment = 0x%lx\n",
         searchStart, searchEnd, alignment);

  if (physAddrOut == NULL || sizeOut == NULL) {
    return ORBIS_KERNEL_ERROR_EINVAL;
  }

  void *memory = MemoryManager_GetInstance();
  PAddr physAddr = 0;
  u64 size = 0;
  s32 result = MemoryManager_DirectQueryAvailable(
      memory, searchStart, searchEnd, alignment, &physAddr, &size);

  if (size == 0) {
    return ORBIS_KERNEL_ERROR_ENOMEM;
  }

  *physAddrOut = (u64)physAddr;
  *sizeOut = size;

  return result;
}

s32 sceKernelVirtualQuery(const void *addr, s32 flags,
                          OrbisVirtualQueryInfo *info, u64 infoSize) {
  printf("[Kernel_Memory] sceKernelVirtualQuery: addr = %p, flags = 0x%x\n",
         addr, flags);
  void *memory = MemoryManager_GetInstance();
  return MemoryManager_VirtualQuery(memory, (VAddr)addr, flags, info);
}

s32 sceKernelReserveVirtualRange(void **addr, u64 len, s32 flags,
                                 u64 alignment) {
  printf(
      "[Kernel_Memory] sceKernelReserveVirtualRange: addr = %p, len = 0x%lx, "
      "flags = 0x%x, alignment = 0x%lx\n",
      *addr, len, flags, alignment);

  if (addr == NULL) {
    fprintf(stderr, "[Kernel_Memory] ERROR: Address is invalid!\n");
    return ORBIS_KERNEL_ERROR_EINVAL;
  }
  if (len == 0 || !Is16KBAligned(len)) {
    fprintf(stderr, "[Kernel_Memory] ERROR: Map size is either zero or not "
                    "16KB aligned!\n");
    return ORBIS_KERNEL_ERROR_EINVAL;
  }
  if (alignment != 0) {
    if (!IsPowerOfTwo(alignment) && !Is16KBAligned(alignment)) {
      fprintf(stderr, "[Kernel_Memory] ERROR: Alignment value is invalid!\n");
      return ORBIS_KERNEL_ERROR_EINVAL;
    }
  }

  void *memory = MemoryManager_GetInstance();
  VAddr in_addr = (VAddr)*addr;

  // VMAType::Reserved = 0, MemoryProt::NoAccess = 0
  s32 result = MemoryManager_MapMemory(memory, addr, in_addr, len, 0, flags, 0,
                                       "anon", false, -1, alignment);
  if (result == 0) {
    printf("[Kernel_Memory] out_addr = %p\n", *addr);
  }
  return result;
}

s32 sceKernelMapNamedDirectMemory(void **addr, u64 len, s32 prot, s32 flags,
                                  s64 phys_addr, u64 alignment,
                                  const char *name) {
  printf("[Kernel_Memory] sceKernelMapNamedDirectMemory: in_addr = %p, len = "
         "0x%lx, "
         "prot = 0x%x, flags = 0x%x, phys_addr = 0x%lx, alignment = 0x%lx, "
         "name = '%s'\n",
         *addr, len, prot, flags, phys_addr, alignment, name);

  if (len == 0 || !Is16KBAligned(len)) {
    fprintf(stderr, "[Kernel_Memory] ERROR: Map size is either zero or not "
                    "16KB aligned!\n");
    return ORBIS_KERNEL_ERROR_EINVAL;
  }

  if (!Is16KBAligned(phys_addr)) {
    fprintf(stderr,
            "[Kernel_Memory] ERROR: Start address is not 16KB aligned!\n");
    return ORBIS_KERNEL_ERROR_EINVAL;
  }

  if (alignment != 0) {
    if (!IsPowerOfTwo(alignment) && !Is16KBAligned(alignment)) {
      fprintf(stderr, "[Kernel_Memory] ERROR: Alignment value is invalid!\n");
      return ORBIS_KERNEL_ERROR_EINVAL;
    }
  }

  if (strlen(name) >= ORBIS_KERNEL_MAXIMUM_NAME_LENGTH) {
    fprintf(stderr, "[Kernel_Memory] ERROR: name exceeds 32 bytes!\n");
    return ORBIS_KERNEL_ERROR_ENAMETOOLONG;
  }

  // Check for executable permissions (not allowed for direct memory)
  if (prot & 0x4) { // CPU_EXEC bit
    fprintf(stderr,
            "[Kernel_Memory] ERROR: Executable permissions are not allowed.\n");
    return ORBIS_KERNEL_ERROR_EACCES;
  }

  VAddr in_addr = (VAddr)*addr;
  void *memory = MemoryManager_GetInstance();

  bool should_check = false;
  if (g_sdk_version >= 0x2500000 &&
      !(flags & 0x200)) { // FW 2.5+, not Stack flag
    should_check = !g_alias_dmem;
  }

  // VMAType::Direct = 1
  s32 ret = MemoryManager_MapMemory(memory, addr, in_addr, len, prot, flags, 1,
                                    name, should_check, phys_addr, alignment);

  printf("[Kernel_Memory] out_addr = %p\n", *addr);
  return ret;
}

s32 sceKernelMapDirectMemory(void **addr, u64 len, s32 prot, s32 flags,
                             s64 phys_addr, u64 alignment) {
  printf("[Kernel_Memory] sceKernelMapDirectMemory (redirected to Named "
         "version)\n");
  return sceKernelMapNamedDirectMemory(addr, len, prot, flags, phys_addr,
                                       alignment, "anon");
}

s32 sceKernelMapDirectMemory2(void **addr, u64 len, s32 type, s32 prot,
                              s32 flags, s64 phys_addr, u64 alignment) {
  printf(
      "[Kernel_Memory] sceKernelMapDirectMemory2: in_addr = %p, len = 0x%lx, "
      "prot = 0x%x, flags = 0x%x, phys_addr = 0x%lx, alignment = 0x%lx\n",
      *addr, len, prot, flags, phys_addr, alignment);

  if (len == 0 || !Is16KBAligned(len)) {
    fprintf(stderr, "[Kernel_Memory] ERROR: Map size is either zero or not "
                    "16KB aligned!\n");
    return ORBIS_KERNEL_ERROR_EINVAL;
  }

  if (!Is16KBAligned(phys_addr)) {
    fprintf(stderr,
            "[Kernel_Memory] ERROR: Start address is not 16KB aligned!\n");
    return ORBIS_KERNEL_ERROR_EINVAL;
  }

  if (alignment != 0) {
    if (!IsPowerOfTwo(alignment) && !Is16KBAligned(alignment)) {
      fprintf(stderr, "[Kernel_Memory] ERROR: Alignment value is invalid!\n");
      return ORBIS_KERNEL_ERROR_EINVAL;
    }
  }

  if (prot & 0x4) { // CPU_EXEC bit
    fprintf(stderr,
            "[Kernel_Memory] ERROR: Executable permissions are not allowed.\n");
    return ORBIS_KERNEL_ERROR_EINVAL;
  }

  VAddr in_addr = (VAddr)*addr;
  void *memory = MemoryManager_GetInstance();

  // VMAType::Direct = 1
  s32 ret =
      MemoryManager_MapMemory(memory, addr, in_addr, len, prot, flags, 1,
                              "anon", !g_alias_dmem, phys_addr, alignment);

  if (ret == 0) {
    VAddr out_addr = (VAddr)*addr;
    MemoryManager_SetDirectMemoryType(memory, out_addr, len, type);
    printf("[Kernel_Memory] out_addr = 0x%lx\n", out_addr);
  }
  return ret;
}

s32 sceKernelMapNamedFlexibleMemory(void **addr_in_out, u64 len, s32 prot,
                                    s32 flags, const char *name) {
  printf("[Kernel_Memory] sceKernelMapNamedFlexibleMemory: in_addr = %p, len = "
         "0x%lx, "
         "prot = 0x%x, flags = 0x%x, name = '%s'\n",
         *addr_in_out, len, prot, flags, name);

  if (len == 0 || !Is16KBAligned(len)) {
    fprintf(stderr, "[Kernel_Memory] ERROR: len is 0 or not 16kb multiple\n");
    return ORBIS_KERNEL_ERROR_EINVAL;
  }

  if (name == NULL) {
    fprintf(stderr, "[Kernel_Memory] ERROR: name is invalid!\n");
    return ORBIS_KERNEL_ERROR_EFAULT;
  }

  if (strlen(name) >= ORBIS_KERNEL_MAXIMUM_NAME_LENGTH) {
    fprintf(stderr, "[Kernel_Memory] ERROR: name exceeds 32 bytes!\n");
    return ORBIS_KERNEL_ERROR_ENAMETOOLONG;
  }

  VAddr in_addr = (VAddr)*addr_in_out;
  void *memory = MemoryManager_GetInstance();

  // VMAType::Flexible = 2
  s32 ret = MemoryManager_MapMemory(memory, addr_in_out, in_addr, len, prot,
                                    flags, 2, name, false, -1, 0);
  printf("[Kernel_Memory] out_addr = %p\n", *addr_in_out);
  return ret;
}

s32 sceKernelMapFlexibleMemory(void **addr_in_out, u64 len, s32 prot,
                               s32 flags) {
  return sceKernelMapNamedFlexibleMemory(addr_in_out, len, prot, flags, "anon");
}

s32 sceKernelQueryMemoryProtection(void *addr, void **start, void **end,
                                   u32 *prot) {
  void *memory = MemoryManager_GetInstance();
  return MemoryManager_QueryProtection(memory, (VAddr)addr, start, end, prot);
}

s32 sceKernelMprotect(const void *addr, u64 size, s32 prot) {
  printf("[Kernel_Memory] sceKernelMprotect: addr = %p, size = 0x%lx, prot = "
         "0x%x\n",
         addr, size, prot);

  // Align addr and size to the nearest page boundary
  VAddr in_addr = (VAddr)addr;
  VAddr aligned_addr = AlignDown(in_addr, PAGE_SIZE_16KB);
  u64 aligned_size = AlignUp(size + in_addr - aligned_addr, PAGE_SIZE_16KB);

  if (aligned_size == 0) {
    return ORBIS_OK;
  }

  void *memory = MemoryManager_GetInstance();
  return MemoryManager_Protect(memory, aligned_addr, aligned_size, prot);
}

s32 posix_mprotect(const void *addr, u64 size, s32 prot) {
  s32 result = sceKernelMprotect(addr, size, prot);
  if (result < 0) {
    // Set errno here if needed
    return -1;
  }
  return result;
}

s32 sceKernelMtypeprotect(const void *addr, u64 size, s32 mtype, s32 prot) {
  printf("[Kernel_Memory] sceKernelMtypeprotect: addr = %p, size = 0x%lx, prot "
         "= 0x%x\n",
         addr, size, prot);

  VAddr in_addr = (VAddr)addr;
  VAddr aligned_addr = AlignDown(in_addr, PAGE_SIZE_16KB);
  u64 aligned_size = AlignUp(size + in_addr - aligned_addr, PAGE_SIZE_16KB);

  if (aligned_size == 0) {
    return ORBIS_OK;
  }

  void *memory = MemoryManager_GetInstance();
  s32 result = MemoryManager_Protect(memory, aligned_addr, aligned_size, prot);
  if (result == ORBIS_OK) {
    MemoryManager_SetDirectMemoryType(memory, aligned_addr, aligned_size,
                                      mtype);
  }
  return result;
}

s32 sceKernelDirectMemoryQuery(u64 offset, s32 flags,
                               OrbisQueryInfo *query_info, u64 infoSize) {
  printf("[Kernel_Memory] sceKernelDirectMemoryQuery: offset = 0x%lx, flags = "
         "0x%x\n",
         offset, flags);
  void *memory = MemoryManager_GetInstance();
  return MemoryManager_DirectMemoryQuery(memory, offset, flags == 1,
                                         query_info);
}

s32 sceKernelAvailableFlexibleMemorySize(u64 *out_size) {
  void *memory = MemoryManager_GetInstance();
  *out_size = MemoryManager_GetAvailableFlexibleSize(memory);
  printf("[Kernel_Memory] sceKernelAvailableFlexibleMemorySize: size = 0x%lx\n",
         *out_size);
  return ORBIS_OK;
}

void _sceKernelRtldSetApplicationHeapAPI(void *func[]) {
  printf("[Kernel_Memory] _sceKernelRtldSetApplicationHeapAPI called\n");
  // TODO: Implement linker heap API setting
}

s32 sceKernelGetDirectMemoryType(u64 addr, s32 *directMemoryTypeOut,
                                 void **directMemoryStartOut,
                                 void **directMemoryEndOut) {
  printf("[Kernel_Memory] sceKernelGetDirectMemoryType: addr = 0x%lx\n", addr);
  void *memory = MemoryManager_GetInstance();
  return MemoryManager_GetDirectMemoryType(memory, addr, directMemoryTypeOut,
                                           directMemoryStartOut,
                                           directMemoryEndOut);
}

s32 sceKernelIsStack(void *addr, void **start, void **end) {
  printf("[Kernel_Memory] sceKernelIsStack: addr = %p\n", addr);
  void *memory = MemoryManager_GetInstance();
  return MemoryManager_IsStack(memory, (VAddr)addr, start, end);
}

u32 sceKernelIsAddressSanitizerEnabled(void) {
  printf("[Kernel_Memory] sceKernelIsAddressSanitizerEnabled called\n");
  return 0; // Always disabled
}

s32 sceKernelBatchMap(OrbisKernelBatchMapEntry *entries, s32 numEntries,
                      s32 *numEntriesOut) {
  return sceKernelBatchMap2(entries, numEntries, numEntriesOut,
                            ORBIS_KERNEL_MAP_FIXED);
}

s32 sceKernelBatchMap2(OrbisKernelBatchMapEntry *entries, s32 numEntries,
                       s32 *numEntriesOut, s32 flags) {
  s32 result = ORBIS_OK;
  s32 processed = 0;

  for (s32 i = 0; i < numEntries; i++, processed++) {
    if (entries == NULL || entries[i].length == 0 || entries[i].operation > 4) {
      result = ORBIS_KERNEL_ERROR_EINVAL;
      break;
    }

    switch (entries[i].operation) {
    case ORBIS_KERNEL_MAP_OP_MAP_DIRECT:
      result = sceKernelMapNamedDirectMemory(
          &entries[i].start, entries[i].length, entries[i].protection, flags,
          (s64)entries[i].offset, 0, "anon");
      break;
    case ORBIS_KERNEL_MAP_OP_UNMAP:
      result = sceKernelMunmap(entries[i].start, entries[i].length);
      break;
    case ORBIS_KERNEL_MAP_OP_PROTECT:
      result = sceKernelMprotect(entries[i].start, entries[i].length,
                                 entries[i].protection);
      break;
    case ORBIS_KERNEL_MAP_OP_MAP_FLEXIBLE:
      result =
          sceKernelMapNamedFlexibleMemory(&entries[i].start, entries[i].length,
                                          entries[i].protection, flags, "anon");
      break;
    case ORBIS_KERNEL_MAP_OP_TYPE_PROTECT:
      result = sceKernelMtypeprotect(entries[i].start, entries[i].length,
                                     entries[i].type, entries[i].protection);
      break;
    default:
      fprintf(stderr, "[Kernel_Memory] ERROR: Unknown batch operation!\n");
      result = ORBIS_KERNEL_ERROR_EINVAL;
      break;
    }

    if (result != ORBIS_OK) {
      fprintf(stderr,
              "[Kernel_Memory] ERROR: Batch operation failed with error 0x%x\n",
              result);
      break;
    }
  }

  if (numEntriesOut != NULL) {
    *numEntriesOut = processed;
  }
  return result;
}

s32 sceKernelSetVirtualRangeName(const void *addr, u64 len, const char *name) {
  if (name == NULL) {
    fprintf(stderr, "[Kernel_Memory] ERROR: name is invalid!\n");
    return ORBIS_KERNEL_ERROR_EFAULT;
  }

  if (strlen(name) >= ORBIS_KERNEL_MAXIMUM_NAME_LENGTH) {
    fprintf(stderr, "[Kernel_Memory] ERROR: name exceeds 32 bytes!\n");
    return ORBIS_KERNEL_ERROR_ENAMETOOLONG;
  }

  void *memory = MemoryManager_GetInstance();
  MemoryManager_NameVirtualRange(memory, (VAddr)addr, len, name);
  return ORBIS_OK;
}

// ============================================================================
// Memory Pool Functions
// ============================================================================

s32 sceKernelMemoryPoolExpand(u64 searchStart, u64 searchEnd, u64 len,
                              u64 alignment, u64 *physAddrOut) {
  if (searchStart < 0 || searchEnd <= searchStart) {
    fprintf(stderr,
            "[Kernel_Memory] ERROR: Provided address range is invalid!\n");
    return ORBIS_KERNEL_ERROR_EINVAL;
  }
  if (len <= 0 || !Is64KBAligned(len)) {
    fprintf(stderr,
            "[Kernel_Memory] ERROR: Provided length 0x%lx is invalid!\n", len);
    return ORBIS_KERNEL_ERROR_EINVAL;
  }
  if (alignment != 0 && !Is64KBAligned(alignment)) {
    fprintf(stderr, "[Kernel_Memory] ERROR: Alignment 0x%lx is invalid!\n",
            alignment);
    return ORBIS_KERNEL_ERROR_EINVAL;
  }
  if (physAddrOut == NULL) {
    fprintf(
        stderr,
        "[Kernel_Memory] ERROR: Result physical address pointer is null!\n");
    return ORBIS_KERNEL_ERROR_EINVAL;
  }

  bool is_in_range = searchEnd - searchStart >= len;
  if (searchEnd <= searchStart || searchEnd < len || !is_in_range) {
    fprintf(stderr,
            "[Kernel_Memory] ERROR: Provided address range is too small! "
            "searchStart = 0x%lx, searchEnd = 0x%lx, length = 0x%lx\n",
            searchStart, searchEnd, len);
    return ORBIS_KERNEL_ERROR_ENOMEM;
  }

  void *memory = MemoryManager_GetInstance();
  PAddr phys_addr =
      MemoryManager_PoolExpand(memory, searchStart, searchEnd, len, alignment);
  if (phys_addr == (PAddr)-1) {
    return ORBIS_KERNEL_ERROR_ENOMEM;
  }

  *physAddrOut = (s64)phys_addr;

  printf("[Kernel_Memory] sceKernelMemoryPoolExpand: searchStart = 0x%lx, "
         "searchEnd = 0x%lx, "
         "len = 0x%lx, alignment = 0x%lx, physAddrOut = 0x%lx\n",
         searchStart, searchEnd, len, alignment, phys_addr);
  return ORBIS_OK;
}

s32 sceKernelMemoryPoolReserve(void *addr_in, u64 len, u64 alignment, s32 flags,
                               void **addr_out) {
  printf(
      "[Kernel_Memory] sceKernelMemoryPoolReserve: addr_in = %p, len = 0x%lx, "
      "alignment = 0x%lx, flags = 0x%x\n",
      addr_in, len, alignment, flags);

  if (len == 0 || !Is2MBAligned(len)) {
    fprintf(
        stderr,
        "[Kernel_Memory] ERROR: Map size is either zero or not 2MB aligned!\n");
    return ORBIS_KERNEL_ERROR_EINVAL;
  }
  if (alignment != 0) {
    if (!IsPowerOfTwo(alignment) && !Is2MBAligned(alignment)) {
      fprintf(stderr, "[Kernel_Memory] ERROR: Alignment value is invalid!\n");
      return ORBIS_KERNEL_ERROR_EINVAL;
    }
  }

  void *memory = MemoryManager_GetInstance();
  VAddr in_addr = (VAddr)addr_in;
  u64 map_alignment = alignment == 0 ? PAGE_SIZE_2MB : alignment;

  // VMAType::PoolReserved = 3, MemoryProt::NoAccess = 0
  return MemoryManager_MapMemory(memory, addr_out, in_addr, len, 0, flags, 3,
                                 "anon", false, -1, map_alignment);
}

s32 sceKernelMemoryPoolCommit(void *addr, u64 len, s32 type, s32 prot,
                              s32 flags) {
  if (addr == NULL) {
    fprintf(stderr, "[Kernel_Memory] ERROR: Address is invalid!\n");
    return ORBIS_KERNEL_ERROR_EINVAL;
  }
  if (len == 0 || !Is64KBAligned(len)) {
    fprintf(stderr, "[Kernel_Memory] ERROR: Map size is either zero or not "
                    "64KB aligned!\n");
    return ORBIS_KERNEL_ERROR_EINVAL;
  }

  if (prot & 0x4) { // CPU_EXEC bit
    fprintf(stderr,
            "[Kernel_Memory] ERROR: Executable permissions are not allowed.\n");
    return ORBIS_KERNEL_ERROR_EINVAL;
  }

  printf("[Kernel_Memory] sceKernelMemoryPoolCommit: addr = %p, len = 0x%lx, "
         "type = 0x%x, prot = 0x%x, flags = 0x%x\n",
         addr, len, type, prot, flags);

  VAddr in_addr = (VAddr)addr;
  void *memory = MemoryManager_GetInstance();
  return MemoryManager_PoolCommit(memory, in_addr, len, prot, type);
}

s32 sceKernelMemoryPoolDecommit(void *addr, u64 len, s32 flags) {
  if (addr == NULL) {
    fprintf(stderr, "[Kernel_Memory] ERROR: Address is invalid!\n");
    return ORBIS_KERNEL_ERROR_EINVAL;
  }
  if (len == 0 || !Is64KBAligned(len)) {
    fprintf(stderr, "[Kernel_Memory] ERROR: Map size is either zero or not "
                    "64KB aligned!\n");
    return ORBIS_KERNEL_ERROR_EINVAL;
  }

  printf("[Kernel_Memory] sceKernelMemoryPoolDecommit: addr = %p, len = 0x%lx, "
         "flags = 0x%x\n",
         addr, len, flags);

  VAddr pool_addr = (VAddr)addr;
  void *memory = MemoryManager_GetInstance();
  return MemoryManager_PoolDecommit(memory, pool_addr, len);
}

s32 sceKernelMemoryPoolBatch(const OrbisKernelMemoryPoolBatchEntry *entries,
                             s32 count, s32 *num_processed, s32 flags) {
  if (entries == NULL) {
    return ORBIS_KERNEL_ERROR_EINVAL;
  }
  s32 result = ORBIS_OK;
  s32 processed = 0;

  for (s32 i = 0; i < count; i++, processed++) {
    OrbisKernelMemoryPoolBatchEntry entry = entries[i];
    switch (entry.opcode) {
    case ORBIS_MEMORY_POOL_COMMIT:
      result = sceKernelMemoryPoolCommit(
          entry.commit_params.addr, entry.commit_params.len,
          entry.commit_params.type, entry.commit_params.prot, entry.flags);
      break;
    case ORBIS_MEMORY_POOL_DECOMMIT:
      result = sceKernelMemoryPoolDecommit(
          entry.decommit_params.addr, entry.decommit_params.len, entry.flags);
      break;
    case ORBIS_MEMORY_POOL_PROTECT:
      result =
          sceKernelMprotect(entry.protect_params.addr, entry.protect_params.len,
                            entry.protect_params.prot);
      break;
    case ORBIS_MEMORY_POOL_TYPE_PROTECT:
      result = sceKernelMtypeprotect(
          entry.type_protect_params.addr, entry.type_protect_params.len,
          entry.type_protect_params.type, entry.type_protect_params.prot);
      break;
    case ORBIS_MEMORY_POOL_MOVE:
      fprintf(stderr,
              "[Kernel_Memory] ERROR: Unimplemented memory pool opcode Move\n");
      result = ORBIS_KERNEL_ERROR_EINVAL;
      break;
    default:
      result = ORBIS_KERNEL_ERROR_EINVAL;
      break;
    }

    if (result != ORBIS_OK) {
      break;
    }
  }

  if (num_processed != NULL) {
    *num_processed = processed;
  }
  return result;
}

s32 sceKernelMemoryPoolGetBlockStats(OrbisKernelMemoryPoolBlockStats *stats,
                                     u64 size) {
  printf("[Kernel_Memory] sceKernelMemoryPoolGetBlockStats called\n");
  void *memory = MemoryManager_GetInstance();
  OrbisKernelMemoryPoolBlockStats local_stats;
  MemoryManager_GetMemoryPoolStats(memory, &local_stats);

  u64 size_to_copy = size < sizeof(OrbisKernelMemoryPoolBlockStats)
                         ? size
                         : sizeof(OrbisKernelMemoryPoolBlockStats);

  // Assert that stats is not null (same check as original)
  if (stats == NULL && size != 0) {
    fprintf(stderr, "[Kernel_Memory] ERROR: Block stats cannot be null\n");
    return ORBIS_KERNEL_ERROR_EINVAL;
  }

  memcpy(stats, &local_stats, size_to_copy);
  return ORBIS_OK;
}

// ============================================================================
// POSIX mmap/munmap Functions
// ============================================================================

void *posix_mmap(void *addr, u64 len, s32 prot, s32 flags, s32 fd,
                 s64 phys_addr) {
  printf("[Kernel_Memory] posix_mmap: addr = %p, len = 0x%lx, prot = 0x%x, "
         "flags = 0x%x, fd = %d, phys_addr = 0x%lx\n",
         addr, len, prot, flags, fd, phys_addr);

  if (len == 0) {
    // errno = EINVAL
    return (void *)-1;
  }

  void *addr_out;
  void *memory = MemoryManager_GetInstance();
  VAddr vaddr = (VAddr)addr;

  // Align address and size
  VAddr aligned_addr = AlignUp(vaddr, PAGE_SIZE_16KB);
  u64 aligned_size = AlignUp(len, PAGE_SIZE_16KB);

  if ((flags & ORBIS_KERNEL_MAP_FIXED) && vaddr != aligned_addr) {
    // errno = EINVAL
    return (void *)-1;
  }

  s32 result = ORBIS_OK;
  if (flags & 0x1000) { // MAP_ANON
    // VMAType::Flexible = 2
    result =
        MemoryManager_MapMemory(memory, &addr_out, aligned_addr, aligned_size,
                                prot, flags, 2, "anon", false, -1, 0);
  } else if (flags & 0x200) { // MAP_STACK
    // VMAType::Stack = 4
    result =
        MemoryManager_MapMemory(memory, &addr_out, aligned_addr, aligned_size,
                                prot, flags, 4, "anon", false, -1, 0);
  } else if (flags & 0x100) { // MAP_VOID
    // VMAType::Reserved = 0, NoAccess = 0
    result =
        MemoryManager_MapMemory(memory, &addr_out, aligned_addr, aligned_size,
                                0, flags, 0, "anon", false, -1, 0);
  } else {
    // File mapping
    result = MemoryManager_MapFile(memory, &addr_out, aligned_addr,
                                   aligned_size, prot, flags, fd, phys_addr);
  }

  if (result != ORBIS_OK) {
    // errno = convert(result)
    return (void *)-1;
  }

  return addr_out;
}

s32 sceKernelMmap(void *addr, u64 len, s32 prot, s32 flags, s32 fd,
                  s64 phys_addr, void **res) {
  void *addr_out = posix_mmap(addr, len, prot, flags, fd, phys_addr);

  if (addr_out == (void *)-1) {
    // Convert errno to kernel error
    return ORBIS_KERNEL_ERROR_EINVAL;
  }

  *res = addr_out;
  return ORBIS_OK;
}

s32 sceKernelConfiguredFlexibleMemorySize(u64 *sizeOut) {
  if (sizeOut == NULL) {
    return ORBIS_KERNEL_ERROR_EINVAL;
  }

  void *memory = MemoryManager_GetInstance();
  *sizeOut = MemoryManager_GetTotalFlexibleSize(memory);
  return ORBIS_OK;
}

s32 sceKernelMunmap(void *addr, u64 len) {
  printf("[Kernel_Memory] sceKernelMunmap: addr = %p, len = 0x%lx\n", addr,
         len);
  if (len == 0) {
    return ORBIS_KERNEL_ERROR_EINVAL;
  }
  void *memory = MemoryManager_GetInstance();
  return MemoryManager_UnmapMemory(memory, (VAddr)addr, len);
}

s32 posix_munmap(void *addr, u64 len) {
  s32 result = sceKernelMunmap(addr, len);
  if (result < 0) {
    fprintf(stderr, "[Kernel_Memory] ERROR: posix_munmap failed: 0x%x\n",
            result);
    // errno = convert(result)
    return -1;
  }
  return result;
}

// ============================================================================
// PRT (Page Remapping Table) Functions
// ============================================================================

#define MAX_PRT_APERTURES 3
#define PRT_AREA_START_ADDR 0x1000000000ULL
#define PRT_AREA_SIZE 0xec00000000ULL

static struct {
  VAddr address;
  u64 size;
} PrtApertures[MAX_PRT_APERTURES] = {0};

s32 sceKernelSetPrtAperture(s32 id, VAddr address, u64 size) {
  if (id < 0 || id >= MAX_PRT_APERTURES) {
    return ORBIS_KERNEL_ERROR_EINVAL;
  }

  if (address < PRT_AREA_START_ADDR ||
      address + size > PRT_AREA_START_ADDR + PRT_AREA_SIZE) {
    return ORBIS_KERNEL_ERROR_EINVAL;
  }

  if (address % 4096 != 0) {
    return ORBIS_KERNEL_ERROR_EINVAL;
  }

  printf("[Kernel_Memory] WARNING: PRT aperture id = %d, address = 0x%lx, size "
         "= 0x%lx "
         "is set but not used\n",
         id, address, size);

  void *memory = MemoryManager_GetInstance();
  MemoryManager_SetPrtArea(memory, id, address, size);

  PrtApertures[id].address = address;
  PrtApertures[id].size = size;
  return ORBIS_OK;
}

s32 sceKernelGetPrtAperture(s32 id, VAddr *address, u64 *size) {
  if (id < 0 || id >= MAX_PRT_APERTURES) {
    return ORBIS_KERNEL_ERROR_EINVAL;
  }

  *address = PrtApertures[id].address;
  *size = PrtApertures[id].size;
  return ORBIS_OK;
}

// ============================================================================
// Symbol Registration
// ============================================================================

void RegisterKernelMemory(void *symbol_resolver) {
  printf("[Kernel_Memory] Registering kernel memory syscalls\n");

  // TODO: Register all function symbols with the dynamic linker
  // This would use your symbol resolver to register each function
  // with its NID (Name ID) so games can call them

  // Example (you'll need to implement this based on your linker):
  // SymbolResolver_Register(symbol_resolver, "usHTMoFoBTM",
  // sceKernelEnableDmemAliasing); SymbolResolver_Register(symbol_resolver,
  // "rTXw65xmLIA", sceKernelAllocateDirectMemory);
  // ... etc for all functions
}
