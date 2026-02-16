// SPDX-FileCopyrightText: Copyright 2025 LayraPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <cstdint>

// Forward declarations
namespace Core::Memory {
class MemoryManager;
}
namespace Core::Kernel {
class KernelManager;
class ModuleManager;
} // namespace Core::Kernel
namespace Core::FileSys {
class MntPoints;
class HandleTable;
} // namespace Core::FileSys
namespace Core::Services {
class ServiceManager;
}

namespace Core::OS {

class OrbisSystem {
public:
  OrbisSystem();
  ~OrbisSystem();

  // Initialize the system with subsystem references
  bool Initialize(Memory::MemoryManager *memory, Kernel::KernelManager *kernel,
                  Kernel::ModuleManager *modules,
                  Services::ServiceManager *services,
                  FileSys::MntPoints *mnt_points,
                  FileSys::HandleTable *handle_table);

  // Accessors
  FileSys::MntPoints *GetMntPoints() { return mnt_points_; }
  FileSys::HandleTable *GetHandleTable() { return handle_table_; }

  // Get firmware version string
  const char *GetFirmwareVersion() const { return firmware_version_; }

  // Get system software version (numeric)
  uint32_t GetSdkVersion() const { return sdk_version_; }

  // System state
  bool IsInitialized() const { return initialized_; }

private:
  Memory::MemoryManager *memory_ = nullptr;
  Kernel::KernelManager *kernel_ = nullptr;
  Kernel::ModuleManager *modules_ = nullptr;
  Services::ServiceManager *services_ = nullptr;
  FileSys::MntPoints *mnt_points_ = nullptr;
  FileSys::HandleTable *handle_table_ = nullptr;

  bool initialized_ = false;
  static constexpr const char *firmware_version_ = "11.00";
  static constexpr uint32_t sdk_version_ = 0x0B000000; // 11.00
};

} // namespace Core::OS
