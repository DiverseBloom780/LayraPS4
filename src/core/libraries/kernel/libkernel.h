// SPDX-FileCopyrightText: Copyright 2025 LayraPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/kernel/module_manager.h"
#include <string>

namespace Core {
namespace Libraries {
namespace Kernel {

void RegisterLibKernel(::Core::Kernel::ModuleManager *module_manager);

// Types
typedef void *PthreadAttrT;
typedef void *PthreadT;
typedef uint32_t OrbisKernelSema;
typedef void *PthreadMutexT;
typedef void *PthreadMutexAttrT;
typedef void *PthreadCondT;
typedef void *PthreadCondAttrT;
typedef void *OrbisKernelEventFlag;

struct OrbisKernelEventFlagOptParam {
  size_t size;
};

struct OrbisVirtualQueryInfo {
  void *start;
  void *end;
  uint64_t offset;
  uint32_t prot;
  uint32_t type;
  uint32_t flags;
  char name[32];
};

// Threading
int32_t scePthreadAttrInit(PthreadAttrT *attr);
int32_t scePthreadAttrDestroy(PthreadAttrT *attr);
int32_t scePthreadAttrSetstacksize(PthreadAttrT *attr, size_t stacksize);
int32_t scePthreadCreate(PthreadT *thread, const PthreadAttrT *attr,
                         void *(*entry)(void *), void *arg, const char *name);
PthreadT scePthreadSelf();
void scePthreadExit(void *value);
int32_t scePthreadJoin(PthreadT thread, void **value_ptr);

// Mutexes
int32_t scePthreadMutexInit(PthreadMutexT *mutex, const PthreadMutexAttrT *attr,
                            const char *name);
int32_t posix_pthread_mutex_init(PthreadMutexT *mutex,
                                 const PthreadMutexAttrT *attr);
int32_t posix_pthread_mutex_lock(PthreadMutexT *mutex);
int32_t posix_pthread_mutex_unlock(PthreadMutexT *mutex);
int32_t posix_pthread_mutex_destroy(PthreadMutexT *mutex);

// Condition Variables
int32_t scePthreadCondInit(PthreadCondT *cond, const PthreadCondAttrT *attr,
                           const char *name);
int32_t posix_pthread_cond_init(PthreadCondT *cond,
                                const PthreadCondAttrT *attr);
int32_t posix_pthread_cond_wait(PthreadCondT *cond, PthreadMutexT *mutex);
int32_t posix_pthread_cond_signal(PthreadCondT *cond);
int32_t posix_pthread_cond_broadcast(PthreadCondT *cond);
int32_t posix_pthread_cond_destroy(PthreadCondT *cond);

// Event Flags
int32_t sceKernelCreateEventFlag(OrbisKernelEventFlag *ef, const char *pName,
                                 uint32_t attr, uint64_t initPattern,
                                 const OrbisKernelEventFlagOptParam *pOptParam);
int32_t sceKernelDeleteEventFlag(OrbisKernelEventFlag ef);
int32_t sceKernelSetEventFlag(OrbisKernelEventFlag ef, uint64_t bitPattern);
int32_t sceKernelClearEventFlag(OrbisKernelEventFlag ef, uint64_t bitPattern);
int32_t sceKernelWaitEventFlag(OrbisKernelEventFlag ef, uint64_t bitPattern,
                               uint32_t waitMode, uint64_t *pResultPat,
                               uint32_t *pTimeout);
int32_t sceKernelPollEventFlag(OrbisKernelEventFlag ef, uint64_t bitPattern,
                               uint32_t waitMode, uint64_t *pResultPat);
int32_t sceKernelCancelEventFlag(OrbisKernelEventFlag ef, uint64_t setPattern,
                                 int32_t *pNumWaitThreads);

// Memory Management
uint64_t sceKernelGetDirectMemorySize();
int32_t sceKernelAllocateDirectMemory(int64_t searchStart, int64_t searchEnd,
                                      uint64_t len, uint64_t alignment,
                                      int32_t memoryType, int64_t *physAddrOut);
int32_t sceKernelAllocateMainDirectMemory(uint64_t len, uint64_t alignment,
                                          int32_t memoryType,
                                          int64_t *physAddrOut);
int32_t sceKernelReleaseDirectMemory(uint64_t start, uint64_t len);
int32_t sceKernelMapNamedDirectMemory(void **addr, uint64_t len, int32_t prot,
                                      int32_t flags, int64_t phys_addr,
                                      uint64_t alignment, const char *name);
int32_t sceKernelMapDirectMemory(void **addr, uint64_t len, int32_t prot,
                                 int32_t flags, int64_t phys_addr,
                                 uint64_t alignment);
int32_t sceKernelReserveVirtualRange(void **addr, uint64_t len, int32_t flags,
                                     uint64_t alignment);
int32_t sceKernelVirtualQuery(const void *addr, int32_t flags,
                              OrbisVirtualQueryInfo *info, uint64_t infoSize);
int32_t sceKernelMunmap(void *addr, uint64_t len);
int32_t sceKernelMprotect(const void *addr, uint64_t size, int32_t prot);

// Semaphores
int32_t sceKernelCreateSema(OrbisKernelSema *sem, const char *name,
                            uint32_t attr, int32_t initCount, int32_t maxCount,
                            const void *pOptParam);
int32_t sceKernelWaitSema(OrbisKernelSema sem, int32_t needCount,
                          uint32_t *pTimeout);
int32_t sceKernelSignalSema(OrbisKernelSema sem, int32_t signalCount);

// Macros to simplify registration
#define LIB_FUNCTION(nid, library, version, module, function)                  \
  module_manager->RegisterHLEExport(module, nid, #function,                    \
                                    reinterpret_cast<uint64_t>(function));

void ErrSceToPosix(int32_t result);
int32_t ErrnoToSceKernelError(int32_t e);
int32_t *__Error();

} // namespace Kernel
} // namespace Libraries
} // namespace Core
