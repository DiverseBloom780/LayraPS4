// SPDX-FileCopyrightText: Copyright 2025 LayraPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <vector>

namespace Core::Kernel {

class KernelManager;

// Standard syscall signature: returns u64, takes generic context (e.g., thread
// state)
using SyscallFn = std::function<uint64_t(void *context, uint64_t *args)>;

class SyscallHandler {
public:
  SyscallHandler(KernelManager *kernel);
  ~SyscallHandler();

  void RegisterSyscall(int id, SyscallFn handler, const std::string &name);
  uint64_t Dispatch(int id, void *context, uint64_t *args);

  // Default implementations
  uint64_t sys_exit(void *context, uint64_t *args);
  uint64_t sys_write(void *context, uint64_t *args);
  uint64_t sys_getpid(void *context, uint64_t *args);

private:
  KernelManager *kernel;
  struct SyscallEntry {
    SyscallFn handler;
    std::string name;
  };
  std::map<int, SyscallEntry> syscall_table;
};

} // namespace Core::Kernel
