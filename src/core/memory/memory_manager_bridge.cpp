// SPDX-FileCopyrightText: Copyright 2025 LayraPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later
//
// C bridge for kernel_memory.c → C++ MemoryManager
// This file provides the extern "C" functions that kernel_memory.c
// expects to call. Each function delegates to the C++ singleton.

#include "../kernel/kernel_memory.h"
#include "memory_manager.h"
#include <cstdio>


using MM = Core::Memory::MemoryManager;

extern "C" {

void *MemoryManager_GetInstance(void) {
  return static_cast<void *>(MM::GetInstance());
}

u64 MemoryManager_GetTotalDirectSize(void *memory) {
  if (!memory)
    return 0;
  return static_cast<MM *>(memory)->GetTotalDirectSize();
}

PAddr MemoryManager_Allocate(void *memory, s64 searchStart, s64 searchEnd,
                             u64 len, u64 alignment, s32 memoryType) {
  if (!memory)
    return static_cast<PAddr>(-1);
  return static_cast<MM *>(memory)->Allocate(searchStart, searchEnd, len,
                                             alignment, memoryType);
}

s32 MemoryManager_Free(void *memory, u64 start, u64 len, bool checked) {
  if (!memory)
    return 0x80020016;
  return static_cast<MM *>(memory)->Free(start, len, checked);
}

s32 MemoryManager_DirectQueryAvailable(void *memory, u64 searchStart,
                                       u64 searchEnd, u64 alignment,
                                       PAddr *physAddr, u64 *size) {
  if (!memory)
    return 0x80020016;
  return static_cast<MM *>(memory)->DirectQueryAvailable(
      searchStart, searchEnd, alignment, physAddr, size);
}

s32 MemoryManager_VirtualQuery(void *memory, VAddr addr, s32 flags,
                               OrbisVirtualQueryInfo *info) {
  if (!memory)
    return 0x80020016;
  return static_cast<MM *>(memory)->VirtualQuery(addr, flags, info);
}

s32 MemoryManager_MapMemory(void *memory, void **addr, VAddr in_addr, u64 len,
                            s32 prot, s32 flags, s32 vma_type, const char *name,
                            bool should_check, s64 phys_addr, u64 alignment) {
  if (!memory)
    return 0x80020016;
  return static_cast<MM *>(memory)->MapMemory(addr, in_addr, len, prot, flags,
                                              vma_type, name, should_check,
                                              phys_addr, alignment);
}

s32 MemoryManager_QueryProtection(void *memory, VAddr addr, void **start,
                                  void **end, u32 *prot) {
  if (!memory)
    return 0x80020016;
  return static_cast<MM *>(memory)->QueryProtection(addr, start, end, prot);
}

s32 MemoryManager_Protect(void *memory, VAddr addr, u64 size, s32 prot) {
  if (!memory)
    return 0x80020016;
  return static_cast<MM *>(memory)->Protect(addr, size, prot);
}

s32 MemoryManager_DirectMemoryQuery(void *memory, u64 offset, bool extended,
                                    OrbisQueryInfo *info) {
  if (!memory)
    return 0x80020016;
  return static_cast<MM *>(memory)->DirectMemoryQuery(offset, extended, info);
}

u64 MemoryManager_GetAvailableFlexibleSize(void *memory) {
  if (!memory)
    return 0;
  return static_cast<MM *>(memory)->GetAvailableFlexibleSize();
}

s32 MemoryManager_GetDirectMemoryType(void *memory, u64 addr, s32 *typeOut,
                                      void **startOut, void **endOut) {
  if (!memory)
    return 0x80020016;
  return static_cast<MM *>(memory)->GetDirectMemoryType(addr, typeOut, startOut,
                                                        endOut);
}

s32 MemoryManager_IsStack(void *memory, VAddr addr, void **start, void **end) {
  if (!memory)
    return 0x80020016;
  return static_cast<MM *>(memory)->IsStack(addr, start, end);
}

void MemoryManager_SetDirectMemoryType(void *memory, VAddr addr, u64 len,
                                       s32 type) {
  if (!memory)
    return;
  static_cast<MM *>(memory)->SetDirectMemoryType(addr, len, type);
}

void MemoryManager_NameVirtualRange(void *memory, VAddr addr, u64 len,
                                    const char *name) {
  if (!memory)
    return;
  static_cast<MM *>(memory)->NameVirtualRange(addr, len, name);
}

u64 MemoryManager_GetTotalFlexibleSize(void *memory) {
  if (!memory)
    return 0;
  return static_cast<MM *>(memory)->GetTotalFlexibleSize();
}

s32 MemoryManager_UnmapMemory(void *memory, VAddr addr, u64 len) {
  if (!memory)
    return 0x80020016;
  return static_cast<MM *>(memory)->UnmapMemory(addr, len);
}

PAddr MemoryManager_PoolExpand(void *memory, u64 searchStart, u64 searchEnd,
                               u64 len, u64 alignment) {
  if (!memory)
    return static_cast<PAddr>(-1);
  return static_cast<MM *>(memory)->PoolExpand(searchStart, searchEnd, len,
                                               alignment);
}

s32 MemoryManager_PoolCommit(void *memory, VAddr addr, u64 len, s32 prot,
                             s32 type) {
  if (!memory)
    return 0x80020016;
  return static_cast<MM *>(memory)->PoolCommit(addr, len, prot, type);
}

s32 MemoryManager_PoolDecommit(void *memory, VAddr addr, u64 len) {
  if (!memory)
    return 0x80020016;
  return static_cast<MM *>(memory)->PoolDecommit(addr, len);
}

void MemoryManager_GetMemoryPoolStats(void *memory,
                                      OrbisKernelMemoryPoolBlockStats *stats) {
  if (!memory)
    return;
  static_cast<MM *>(memory)->GetMemoryPoolStats(stats);
}

s32 MemoryManager_MapFile(void *memory, void **addr, VAddr in_addr, u64 len,
                          s32 prot, s32 flags, s32 fd, s64 offset) {
  if (!memory)
    return 0x80020016;
  return static_cast<MM *>(memory)->MapFile(addr, in_addr, len, prot, flags, fd,
                                            offset);
}

void MemoryManager_SetPrtArea(void *memory, s32 id, VAddr address, u64 size) {
  if (!memory)
    return;
  static_cast<MM *>(memory)->SetPrtArea(id, address, size);
}

} // extern "C"
