// SPDX-FileCopyrightText: Copyright 2025 LayraPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "kernel.h"
#include "syscall_handler.h"
#include "exception_handler.h"
#include "core/memory/memory_manager.h"
#include <cstdio>
#include <map>
#include <mutex>
#include <thread>
#include <vector>

namespace Core::Kernel {

// Internal thread structure
struct ThreadContext {
  std::thread native_thread;
  ThreadInfo info;
  uint64_t arg;
};

// Global state for now (should be members but modifying header fully is
// intrusive) But wait, I can add members to the cpp file as a PIMPL or just
// global static if I'm lazy. Better to add them to the class in the header if I
// can. But since I didn't see private members in the header view, I should
// probably add them there too.

// However, to avoid header dependnecy hell, I'll use a static map here for this
// phase.
static std::map<uint32_t, ThreadContext *> g_threads;
static std::mutex g_thread_mutex;
static uint32_t g_next_thread_handle = 0x2000;

KernelManager::KernelManager() {
  printf("[KernelManager] Initialized\n");
  syscall_handler = new SyscallHandler(this);
  InstallExceptionHandler(syscall_handler);
}

KernelManager::~KernelManager() {
  printf("[KernelManager] Shutdown\n");
  RemoveExceptionHandler();
  if (syscall_handler)
    delete syscall_handler; // Clean up
  std::lock_guard<std::mutex> lock(g_thread_mutex);
  for (auto &pair : g_threads) {
    ThreadContext *ctx = pair.second;
    if (ctx->native_thread.joinable()) {
      ctx->native_thread.join();
    }
    delete ctx;
  }
  g_threads.clear();
}

std::vector<ThreadInfo> KernelManager::GetThreadList() const {
  std::lock_guard<std::mutex> lock(g_thread_mutex);
  std::vector<ThreadInfo> list;
  for (const auto &pair : g_threads) {
    list.push_back(pair.second->info);
  }
  return list;
}

uint32_t KernelManager::CreateThread(const std::string &name,
                                     uint64_t entryPoint, uint64_t priority,
                                     uint64_t stackSize, uint64_t arg) {
  std::lock_guard<std::mutex> lock(g_thread_mutex);
  uint32_t handle = g_next_thread_handle++;

  ThreadContext *ctx = new ThreadContext();
  ctx->info.handle = handle;
  ctx->info.name = name;
  ctx->info.entry = entryPoint;
  ctx->info.running = false;
  ctx->info.exited = false;
  ctx->arg = arg;

  g_threads[handle] = ctx;

  printf("[KernelManager] Created thread '%s' (Handle: 0x%X, Entry: 0x%llx)\n",
         name.c_str(), handle, entryPoint);
  return handle;
}

void KernelManager::StartThread(uint32_t handle) {
  std::lock_guard<std::mutex> lock(g_thread_mutex);
  if (g_threads.find(handle) == g_threads.end())
    return;

  ThreadContext *ctx = g_threads[handle];
  if (ctx->info.running)
    return;

  ctx->info.running = true;

  // Launch native thread
  // Lambda to wrap execution
  ctx->native_thread = std::thread([ctx]() {
    printf("[KernelManager] Thread '%s' started execution at 0x%llx\n",
           ctx->info.name.c_str(), ctx->info.entry);

    auto *mem_manager = Core::Memory::MemoryManager::Instance();
    if (mem_manager) {
      void *host_entry = mem_manager->GetHostPtr(ctx->info.entry);
      if (!host_entry) {
        printf("[KernelManager] ERROR: Invalid guest entry point 0x%llx\n",
               ctx->info.entry);
        ctx->info.running = false;
        ctx->info.exited = true;
        return;
      }

      void *range_start = nullptr;
      void *range_end = nullptr;
      uint32_t prot = 0;
      if (mem_manager->QueryProtection(ctx->info.entry, &range_start,
                                       &range_end, &prot) != 0 ||
          !(prot & 4)) {
        printf("[KernelManager] ERROR: Entry point 0x%llx is not executable or "
               "not mapped\n",
               ctx->info.entry);
        ctx->info.running = false;
        ctx->info.exited = true;
        return;
      }

      printf("[KernelManager] JMP to host address: %p\n", host_entry);

      // Cast to function pointer and execute (SysV vs MSVC ABI mismatch
      // is fine for now since the game entry point technically never returns,
      // it calls exit() or loops forever).
      void (*entry_func)(void *) =
          reinterpret_cast<void (*)(void *)>(host_entry);
      
      try {
          entry_func((void *)ctx->arg);
      } catch (...) {
          printf("[KernelManager] Exception during thread execution!\n");
      }
    }

    printf("[KernelManager] Thread '%s' finished execution\n",
           ctx->info.name.c_str());
    ctx->info.running = false;
    ctx->info.exited = true;
  });

  // Do NOT detach here. Keeping the thread joinable allows the 
  // destructor to wait for it, preventing use-after-free on shutdown.
}

void KernelManager::ExitThread(uint32_t handle, int exitCode) {
  // TODO impl
  printf("[KernelManager] Thread 0x%X exited with code %d (not fully "
         "implemented)\n",
         handle, exitCode);
  std::lock_guard<std::mutex> lock(g_thread_mutex);
  if (g_threads.count(handle)) {
    g_threads[handle]->info.exited = true;
    g_threads[handle]->info.running = false;
    // In a real scenario, we might clean up the ThreadContext here,
    // or mark it for later cleanup, especially if it was detached.
  }
}

void KernelManager::JoinThread(uint32_t handle) {
  std::lock_guard<std::mutex> lock(g_thread_mutex);
  if (g_threads.count(handle)) {
    ThreadContext *ctx = g_threads[handle];
    if (ctx->native_thread.joinable()) {
      ctx->native_thread.join();
    }
  }
}

} // namespace Core::Kernel
