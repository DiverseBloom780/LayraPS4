// SPDX-FileCopyrightText: Copyright 2025 LayraPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "orbis_system.h"
#include "file_sys/fs.h"
#include "kernel/kernel_manager.h"
#include "kernel/module_manager.h"
#include "memory/memory_manager.h"
#include "services/service_manager.h"
#include <cstdio>

namespace Core {
namespace OS {

OrbisSystem::OrbisSystem() { printf("[OrbisSystem] Created\n"); }

OrbisSystem::~OrbisSystem() { printf("[OrbisSystem] Shutdown\n"); }

bool OrbisSystem::Initialize(Memory::MemoryManager *memory,
                             Kernel::KernelManager *kernel,
                             Kernel::ModuleManager *modules,
                             Services::ServiceManager *services,
                             FileSys::MntPoints *mnt_points,
                             FileSys::HandleTable *handle_table) {
  printf("[OrbisSystem] Initializing Orbis OS layer...\n");

  memory_ = memory;
  kernel_ = kernel;
  modules_ = modules;
  services_ = services;
  mnt_points_ = mnt_points;
  handle_table_ = handle_table;

  // Validate all subsystems are available
  if (!memory_ || !kernel_ || !modules_ || !services_ || !mnt_points_ ||
      !handle_table_) {
    fprintf(stderr, "[OrbisSystem] ERROR: One or more subsystems are null\n");
    return false;
  }

  // Report system configuration
  printf("[OrbisSystem] Firmware: %s (SDK 0x%08X)\n", firmware_version_,
         sdk_version_);
  printf("[OrbisSystem] Direct Memory: %llu MB\n",
         static_cast<unsigned long long>(memory_->GetTotalDirectSize() /
                                         (1024 * 1024)));
  printf("[OrbisSystem] Flexible Memory: %llu MB\n",
         static_cast<unsigned long long>(memory_->GetTotalFlexibleSize() /
                                         (1024 * 1024)));

  initialized_ = true;
  printf("[OrbisSystem] Orbis OS layer initialized successfully\n");
  return true;
}

} // namespace OS
} // namespace Core
