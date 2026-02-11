#include "kernel_manager.h"
#include <chrono>
#include <iostream>

namespace Core {
namespace Kernel {

KernelManager::KernelManager() {
  std::cout << "[Kernel] Kernel Manager initialized.\n";
}

KernelManager::~KernelManager() {
  std::lock_guard<std::mutex> lock(managerMutex);
  objects.clear();
}

s32 KernelManager::AllocateHandle() { return nextHandle++; }

s32 KernelManager::CreateMutex(const std::string &name, u32 attr) {
  std::lock_guard<std::mutex> lock(managerMutex);
  s32 handle = AllocateHandle();
  auto obj = std::make_shared<MutexObject>();
  obj->handle = handle;
  obj->name = name;
  objects[handle] = obj;

  std::cout << "[Kernel] Created Mutex: " << name << " (handle=0x" << std::hex
            << handle << std::dec << ")\n";
  return handle;
}

s32 KernelManager::LockMutex(s32 handle, u32 timeout_usec) {
  std::shared_ptr<MutexObject> mtxObj;
  {
    std::lock_guard<std::mutex> lock(managerMutex);
    auto it = objects.find(handle);
    if (it == objects.end() || it->second->type != KernelObjectType::Mutex) {
      return -1;
    }
    mtxObj = std::static_pointer_cast<MutexObject>(it->second);
  }

  if (timeout_usec == 0xFFFFFFFF) {
    mtxObj->mtx.lock();
    return 0;
  } else {
    if (mtxObj->mtx.try_lock()) {
      return 0;
    }
    return -1;
  }
}

s32 KernelManager::UnlockMutex(s32 handle) {
  std::lock_guard<std::mutex> lock(managerMutex);
  auto it = objects.find(handle);
  if (it == objects.end() || it->second->type != KernelObjectType::Mutex) {
    return -1;
  }
  auto mtxObj = std::static_pointer_cast<MutexObject>(it->second);
  mtxObj->mtx.unlock();
  return 0;
}

s32 KernelManager::DeleteMutex(s32 handle) {
  std::lock_guard<std::mutex> lock(managerMutex);
  return objects.erase(handle) > 0 ? 0 : -1;
}

s32 KernelManager::CreateSemaphore(const std::string &name, u32 attr,
                                   int initialCount, int maxCount) {
  std::lock_guard<std::mutex> lock(managerMutex);
  s32 handle = AllocateHandle();
  auto obj = std::make_shared<SemaphoreObject>(initialCount, maxCount);
  obj->handle = handle;
  obj->name = name;
  objects[handle] = obj;

  std::cout << "[Kernel] Created Semaphore: " << name << " (handle=0x"
            << std::hex << handle << std::dec << ")\n";
  return handle;
}

s32 KernelManager::WaitSemaphore(s32 handle, int count, u32 timeout_usec) {
  std::shared_ptr<SemaphoreObject> semObj;
  {
    std::lock_guard<std::mutex> lock(managerMutex);
    auto it = objects.find(handle);
    if (it == objects.end() ||
        it->second->type != KernelObjectType::Semaphore) {
      return -1;
    }
    semObj = std::static_pointer_cast<SemaphoreObject>(it->second);
  }

  std::unique_lock<std::mutex> lock(semObj->mtx);
  auto condition = [&] { return semObj->count >= count; };

  if (timeout_usec == 0xFFFFFFFF) {
    semObj->cv.wait(lock, condition);
    semObj->count -= count;
    return 0;
  } else {
    if (semObj->cv.wait_for(lock, std::chrono::microseconds(timeout_usec),
                            condition)) {
      semObj->count -= count;
      return 0;
    }
    return -1;
  }
}

s32 KernelManager::SignalSemaphore(s32 handle, int count) {
  std::shared_ptr<SemaphoreObject> semObj;
  {
    std::lock_guard<std::mutex> lock(managerMutex);
    auto it = objects.find(handle);
    if (it == objects.end() ||
        it->second->type != KernelObjectType::Semaphore) {
      return -1;
    }
    semObj = std::static_pointer_cast<SemaphoreObject>(it->second);
  }

  {
    std::lock_guard<std::mutex> lock(semObj->mtx);
    semObj->count = std::min(semObj->maxCount, semObj->count + count);
  }
  semObj->cv.notify_all();
  return 0;
}

s32 KernelManager::DeleteSemaphore(s32 handle) {
  std::lock_guard<std::mutex> lock(managerMutex);
  return objects.erase(handle) > 0 ? 0 : -1;
}

s32 KernelManager::CreateThread(const std::string &name, u64 entry, u64 arg,
                                u32 stackSize, int priority) {
  std::lock_guard<std::mutex> lock(managerMutex);
  s32 handle = AllocateHandle();
  auto obj = std::make_shared<ThreadObject>();
  obj->handle = handle;
  obj->name = name;
  obj->entry = entry;
  obj->arg = arg;
  objects[handle] = obj;

  std::cout << "[Kernel] Created Thread: " << name << " (entry=0x" << std::hex
            << entry << ", handle=0x" << handle << std::dec << ")\n";
  return handle;
}

s32 KernelManager::StartThread(s32 handle) {
  std::shared_ptr<ThreadObject> threadObj;
  {
    std::lock_guard<std::mutex> lock(managerMutex);
    auto it = objects.find(handle);
    if (it == objects.end() || it->second->type != KernelObjectType::Thread) {
      return -1;
    }
    threadObj = std::static_pointer_cast<ThreadObject>(it->second);
  }

  if (threadObj->running)
    return -1;

  threadObj->running = true;
  threadObj->hostThread = std::thread([threadObj]() {
    std::cout << "[Kernel] Thread '" << threadObj->name
              << "' starting execution at 0x" << std::hex << threadObj->entry
              << std::dec << "\n";

    // In a full emulator, this would jump into the CPU recompiler/interpreter
    // For now, we simulate execution
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    threadObj->running = false;
    threadObj->exited = true;
    std::cout << "[Kernel] Thread '" << threadObj->name << "' finished.\n";
  });

  return 0;
}

void KernelManager::ExitThread(int status) {
  // Current host thread needs to identify itself.
  // In a real HLE, we'd have a thread-local pointer to the ThreadObject.
  std::cout << "[Kernel] ExitThread(" << status << ") called.\n";
}

s32 KernelManager::JoinThread(s32 handle, int *status) {
  std::shared_ptr<ThreadObject> threadObj;
  {
    std::lock_guard<std::mutex> lock(managerMutex);
    auto it = objects.find(handle);
    if (it == objects.end() || it->second->type != KernelObjectType::Thread) {
      return -1;
    }
    threadObj = std::static_pointer_cast<ThreadObject>(it->second);
  }

  if (threadObj->hostThread.joinable()) {
    threadObj->hostThread.join();
  }

  if (status)
    *status = threadObj->exitStatus;
  return 0;
}

std::vector<ThreadInfo> KernelManager::GetThreadList() {
  std::lock_guard<std::mutex> lock(managerMutex);
  std::vector<ThreadInfo> list;
  for (const auto &[handle, obj] : objects) {
    if (obj->type == KernelObjectType::Thread) {
      auto thread = std::static_pointer_cast<ThreadObject>(obj);
      list.push_back({handle, thread->name, thread->entry,
                      thread->running.load(), thread->exited.load()});
    }
  }
  return list;
}

} // namespace Kernel
} // namespace Core
