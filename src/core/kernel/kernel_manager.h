// SPDX-FileCopyrightText: Copyright 2025 LayraPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace Core::Kernel {

struct ThreadInfo {
  uint32_t handle;
  std::string name;
  uint64_t entry;
  bool running;
  bool exited;
};

class KernelManager {
public:
  KernelManager();
  ~KernelManager();

  std::vector<ThreadInfo> GetThreadList() const;

  // Thread management
  uint32_t CreateThread(const std::string &name, uint64_t entryPoint,
                        uint64_t priority, uint64_t stackSize = 0,
                        uint64_t arg = 0);
  void StartThread(uint32_t handle);
  void ExitThread(uint32_t handle, int exitCode);
  void JoinThread(uint32_t handle);

  class SyscallHandler *GetSyscallHandler() const { return syscall_handler; }

private:
  class SyscallHandler *syscall_handler = nullptr;
};

} // namespace Core::Kernel
