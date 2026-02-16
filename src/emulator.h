// SPDX-FileCopyrightText: Copyright 2025 LayraPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"
#include <memory>
#include <string>
#include <vector>

namespace Core {

// Forward declarations
namespace OS {
class OrbisSystem;
}
namespace Memory {
class MemoryManager;
}
namespace Services {
class ServiceManager;
}
namespace FileSys {
class MntPoints;
class HandleTable;
} // namespace FileSys

namespace Loader {
class ElfLoader;
}
namespace Kernel {
class KernelManager;
class ModuleManager;
} // namespace Kernel

enum class EmulatorState { Stopped, Booting, Running, Paused, Stopping };

class Emulator {
public:
  // Constructor
  Emulator();

  // Destructor
  ~Emulator();

  // Initialize the emulator and OS
  bool Initialize();

  // Load an executable (ELF or PKG)
  bool LoadExecutable(const std::string &path);

  // Run the emulator
  void Run();

  // Pause the emulator
  void Pause();

  // Stop the emulator
  void Stop();

  // Step the CPU
  void Step();

  // Check if the emulator is running
  bool IsRunning() const { return state == EmulatorState::Running; }

  // Get the current state
  EmulatorState GetState() const { return state; }

  // Get OS system
  OS::OrbisSystem *GetSystem() const { return system.get(); }

  // Get Memory Manager
  Memory::MemoryManager *GetMemoryManager() const { return memory.get(); }

  // Get Service Manager
  Services::ServiceManager *GetServiceManager() const { return services.get(); }

  // Get Kernel Manager
  Kernel::KernelManager *GetKernelManager() const { return kernel.get(); }

  // Get Module Manager
  Kernel::ModuleManager *GetModuleManager() const {
    return module_manager.get();
  }

private:
  // Flag to indicate if the emulator is running
  EmulatorState state = EmulatorState::Stopped;

  // OS Emulation system
  std::unique_ptr<OS::OrbisSystem> system;

  // Memory Management system
  std::unique_ptr<Memory::MemoryManager> memory;

  // High-level service manager
  std::unique_ptr<Services::ServiceManager> services;

  // Kernel Management system
  std::unique_ptr<Kernel::KernelManager> kernel;

  // Module & Library Manager
  std::unique_ptr<Kernel::ModuleManager> module_manager;

  // ELF Loader
  std::unique_ptr<Loader::ElfLoader> loader;

  // FileSystem
  std::unique_ptr<FileSys::MntPoints> mnt_points;
  std::unique_ptr<FileSys::HandleTable> handle_table;
};

} // namespace Core
