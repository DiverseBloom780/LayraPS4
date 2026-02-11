// SPDX-FileCopyrightText: Copyright 2025 LayraPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"
#include <atomic>
#include <condition_variable>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

namespace Core {
namespace Kernel {

enum class KernelObjectType {
  Mutex,
  Semaphore,
  EventFlag,
  Thread,
};

struct KernelObject {
  s32 handle;
  KernelObjectType type;
  std::string name;

  virtual ~KernelObject() = default;
};

struct MutexObject : public KernelObject {
  std::recursive_mutex mtx;

  MutexObject() { type = KernelObjectType::Mutex; }
};

struct SemaphoreObject : public KernelObject {
  std::mutex mtx;
  std::condition_variable cv;
  int count;
  int maxCount;

  SemaphoreObject(int initial, int max) : count(initial), maxCount(max) {
    type = KernelObjectType::Semaphore;
  }
};

struct ThreadInfo {
  s32 handle;
  std::string name;
  u64 entry;
  bool running;
  bool exited;
};

struct ThreadObject : public KernelObject {
  std::thread hostThread;
  u64 entry;
  u64 arg;
  std::atomic<bool> running{false};
  std::atomic<bool> exited{false};
  int exitStatus = 0;

  ThreadObject() { type = KernelObjectType::Thread; }
  ~ThreadObject() override {
    if (hostThread.joinable()) {
      hostThread.detach();
    }
  }
};

class KernelManager {
public:
  KernelManager();
  ~KernelManager();

  // Mutex management
  s32 CreateMutex(const std::string &name, u32 attr);
  s32 LockMutex(s32 handle, u32 timeout_usec);
  s32 UnlockMutex(s32 handle);
  s32 DeleteMutex(s32 handle);

  // Semaphore management
  s32 CreateSemaphore(const std::string &name, u32 attr, int initialCount,
                      int maxCount);
  s32 WaitSemaphore(s32 handle, int count, u32 timeout_usec);
  s32 SignalSemaphore(s32 handle, int count);
  s32 DeleteSemaphore(s32 handle);

  // Thread management
  s32 CreateThread(const std::string &name, u64 entry, u64 arg, u32 stackSize,
                   int priority);
  s32 StartThread(s32 handle);
  void ExitThread(int status);
  s32 JoinThread(s32 handle, int *status);

  std::vector<ThreadInfo> GetThreadList();

private:
  std::map<s32, std::shared_ptr<KernelObject>> objects;
  std::mutex managerMutex;
  s32 nextHandle = 0x1000;

  s32 AllocateHandle();
};

} // namespace Kernel
} // namespace Core
