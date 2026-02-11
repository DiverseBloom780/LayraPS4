// SPDX-FileCopyrightText: Copyright 2025 LayraPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"
#include <cstdint>
#include <map>
#include <string>
#include <vector>


namespace Core {
namespace Memory {
class MemoryManager;
}
namespace Services {
class ServiceManager;
}
namespace Kernel {
class KernelManager;
}

namespace OS {

class OrbisSystem {
public:
  OrbisSystem();
  ~OrbisSystem();

  bool Initialize(Memory::MemoryManager *memoryManager,
                  Services::ServiceManager *serviceManager,
                  Kernel::KernelManager *kernelManager);
  void Shutdown();

  // HLE Syscall Dispatcher
  int64_t HandleSyscall(uint32_t syscall_id, const std::vector<uint64_t> &args);

  uint32_t GetCurrentPID() const { return currentPid; }
  void SetCurrentPID(uint32_t pid) { currentPid = pid; }

private:
  uint32_t currentPid = 1;
  Memory::MemoryManager *memory = nullptr;
  Services::ServiceManager *services = nullptr;
  Kernel::KernelManager *kernel = nullptr;

  // File System Translation
  std::map<std::string, std::string> mountPoints;
  std::string TranslatePath(const std::string &orbisPath);
  void SetupMounts();
};

} // namespace OS
} // namespace Core
