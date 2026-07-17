// SPDX-License-Identifier: GPL-2.0-or-later

#include "syscall_handler.h"
#include "kernel_manager.h"
#include "../memory/memory_manager.h"
#include <cstdio>
#include <cstring>

namespace Core::Kernel {

SyscallHandler::SyscallHandler(KernelManager *kernel) : kernel(kernel) {
  // Register default syscalls
  RegisterSyscall(1,
                  std::bind(&SyscallHandler::sys_exit, this,
                            std::placeholders::_1, std::placeholders::_2),
                  "sys_exit");
  RegisterSyscall(4,
                  std::bind(&SyscallHandler::sys_write, this,
                            std::placeholders::_1, std::placeholders::_2),
                  "sys_write");
  RegisterSyscall(20,
                  std::bind(&SyscallHandler::sys_getpid, this,
                            std::placeholders::_1, std::placeholders::_2),
                  "sys_getpid");

  printf("[SyscallHandler] Initialized with default syscalls\n");
}

SyscallHandler::~SyscallHandler() {}

void SyscallHandler::RegisterSyscall(int id, SyscallFn handler,
                                     const std::string &name) {
  syscall_table[id] = {handler, name, id};
}

uint64_t SyscallHandler::Dispatch(int id, void *context, uint64_t *args) {
  auto it = syscall_table.find(id);
  if (it != syscall_table.end()) {
    // printf("[SyscallHandler] Dispatching syscall %d (%s)\n", id,
    // it->second.name.c_str());
    return it->second.handler(context, args);
  } else {
    printf("[SyscallHandler] WARNING: Unknown syscall %d\n", id);
    return -1; // ENOSYS
  }
}

// =================================================================================
// Implementations
// =================================================================================

uint64_t SyscallHandler::sys_exit(void *context, uint64_t *args) {
  int status = (int)args[0];
  printf("[Syscall] sys_exit called via Dispatch! Status: %d\n", status);
  // TODO: Terminate thread
  return 0;
}

uint64_t SyscallHandler::sys_write(void *context, uint64_t *args) {
  int fd = (int)args[0];
  uint64_t buf_ptr = args[1];
  size_t count = (size_t)args[2];

  auto *mem_manager = Core::Memory::MemoryManager::Instance();
  const char *host_ptr = nullptr;
  if (mem_manager) {
    host_ptr = reinterpret_cast<const char *>(mem_manager->GetHostPtr(buf_ptr));
  }

  if (!host_ptr) {
    printf("[Syscall] sys_write failed: invalid guest pointer 0x%llx\n",
           (unsigned long long)buf_ptr);
    return static_cast<uint64_t>(-1);
  }

  if (fd == 1) {
    fwrite(host_ptr, 1, count, stdout);
    fflush(stdout);
  } else if (fd == 2) {
    fwrite(host_ptr, 1, count, stderr);
    fflush(stderr);
  } else {
    printf("[Syscall] sys_write unsupported fd=%d\n", fd);
  }

  return count;
}

uint64_t SyscallHandler::sys_getpid(void *context, uint64_t *args) {
  return 1337; // Dummy PID
}

} // namespace Core::Kernel
