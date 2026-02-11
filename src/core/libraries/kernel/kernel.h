// SPDX-FileCopyrightText: Copyright 2025 LayraPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string>

#include "common/log.h"
#include "common/types.h"

namespace Libraries::Kernel {

// PS4 Kernel Error Codes
constexpr int SCE_KERNEL_ERROR_EINVAL = 0x80010003;
constexpr int SCE_KERNEL_ERROR_ENOMEM = 0x8001000C;
constexpr int SCE_KERNEL_ERROR_EACCES = 0x80010009;
constexpr int SCE_KERNEL_ERROR_ENOENT = 0x80010002;
constexpr int SCE_KERNEL_ERROR_EAGAIN = 0x8001000B;
constexpr int SCE_KERNEL_ERROR_EBUSY = 0x8001000A;
constexpr int SCE_KERNEL_ERROR_EEXIST = 0x80010011;
constexpr int SCE_KERNEL_ERROR_ENOTDIR = 0x80010014;
constexpr int SCE_KERNEL_ERROR_EISDIR = 0x80010015;
// SCE_KERNEL_ERROR_EINVAL already defined at the top

// Thread Management
struct SceKernelThreadInfo {
  u32 size;
  char name[32];
  u32 attr;
  u32 status;
  u32 entry;
  u32 stack;
  u32 stackSize;
  u32 initPriority;
  u32 currentPriority;
  u32 initAffinity;
  u32 currentAffinity;
  u32 exitStatus;
  u32 runClocks[2];
  u32 interruptCounter;
  u32 lastInterruptClocks[2];
  u32 numInterrupts;
};

// Memory Management
struct SceKernelMemoryInfo {
  u32 base;
  u32 size;
  u32 memoryType;
  u32 access;
  u32 initialProtection;
  u32 currentProtection;
  u32 numPageFaults;
  u32 numPageOuts;
  u32 numPageIns;
  u32 numPageFaultsRequiringIo;
  u32 numCopyOnWriteFaults;
  u32 numZeroFilledPages;
  u32 numReclaimedPages;
  u32 numPagesShared;
  u32 numPagesSharedNow;
  u32 numBlocksAllocated;
  u32 numBlocksFreed;
  u32 numBytesAllocated;
  u32 numBytesFreed;
  u32 largestFreeBlockSize;
  u32 smallestFreeBlockSize;
  u32 totalFreeBlockSize;
  u32 averageFreeBlockSize;
};

// File System
struct SceKernelStat {
  u32 st_mode;
  u32 st_ino;
  u32 st_dev;
  u32 st_rdev;
  u32 st_nlink;
  u32 st_uid;
  u32 st_gid;
  u32 st_size;
  u32 st_atime;
  u32 st_mtime;
  u32 st_ctime;
  u32 st_blksize;
  u32 st_blocks;
  u32 st_attr;
};

// Time Management
struct SceKernelTimeval {
  u32 tv_sec;
  u32 tv_usec;
};

struct SceKernelTimezone {
  int tz_minuteswest;
  int tz_dsttime;
};

// Module Management
struct SceKernelModuleInfo {
  u32 size;
  u32 attribute;
  u32 version;
  char name[28];
  void *segmentAddr[4];
  u32 segmentSize[4];
  u32 segmentProt[4];
  u32 segmentType[4];
  u32 startEntry;
  u32 stopEntry;
};

class KernelContext {
public:
  KernelContext() = default;
  ~KernelContext() = default;

  bool Initialize();
  void Shutdown();

  // Memory Management
  u32 AllocateMemory(u32 size, u32 type, u32 protection);
  void FreeMemory(u32 address);
  u32 MapMemory(u32 address, u32 size, u32 protection);
  void UnmapMemory(u32 address, u32 size);

  // Thread Management
  s32 CreateThread(const char *name, u32 entry, u32 arg, u32 stackSize,
                   u32 priority, u32 affinity);
  s32 StartThread(s32 threadId);
  s32 DeleteThread(s32 threadId);
  void ExitThread(s32 exitStatus);
  s32 GetThreadInfo(s32 threadId, SceKernelThreadInfo *info);

  // File System
  s32 Open(const char *path, s32 flags, s32 mode);
  s32 Close(s32 fd);
  s32 Read(s32 fd, void *buf, u32 size);
  s32 Write(s32 fd, const void *buf, u32 size);
  s32 Lseek(s32 fd, s32 offset, s32 whence);
  s32 Stat(const char *path, SceKernelStat *stat);

  // Time Management
  s32 Gettimeofday(SceKernelTimeval *tp, SceKernelTimezone *tzp);
  u32 GetProcessTime();

  // Module Management
  s32 LoadModule(const char *path);
  s32 UnloadModule(s32 moduleId);
  s32 GetModuleInfo(s32 moduleId, SceKernelModuleInfo *info);

  // Error Handling
  void SetError(s32 error);
  s32 GetError() const { return lastError_; }

private:
  s32 lastError_ = 0;
  bool isInitialized_ = false;
};

// Global kernel instance
extern KernelContext g_kernel;

// Utility Functions
inline void KernelError(const std::string &msg) {
  LOG_ERROR("Kernel", "Kernel Error: {}", msg);
}

inline void KernelWarning(const std::string &msg) {
  LOG_WARNING("Kernel", "Kernel Warning: {}", msg);
}

// Thread-local error definition
inline s32 &_Error() {
  static thread_local s32 thread_error = 0;
  return thread_error;
}
inline s32 &Error() { return _Error(); }

} // namespace Libraries::Kernel