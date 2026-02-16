// SPDX-FileCopyrightText: Copyright 2025 LayraPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cmath>
#include <cstdio>
#include <iostream>

#include "core/file_sys/fs.h"
#include "core/kernel/kernel_manager.h"
#include "core/kernel/module_manager.h"
#include "core/kernel/syscalls.h"
#include "core/libraries/kernel/libkernel.h"
#include "core/libraries/libc/libc.h"
#include "core/loader/elf_loader.h"
#include "core/memory/memory_manager.h"
#include "core/orbis_system.h"
#include "core/services/service_manager.h"
#include "emulator.h"


namespace Core {

Emulator::Emulator() : state(EmulatorState::Stopped) {
  printf("[Emulator] Instance created\n");
}

Emulator::~Emulator() {
  printf("[Emulator] Instance destroyed\n");
  if (state != EmulatorState::Stopped) {
    Stop();
  }
}

bool Emulator::Initialize() {
  printf("[Emulator] Initializing...\n");

  if (state != EmulatorState::Stopped) {
    fprintf(stderr, "[Emulator] ERROR: Already initialized or running\n");
    return false;
  }

  try {
    // Initialize Memory Manager
    printf("[Emulator] Initializing Memory Manager...\n");
    memory = std::make_unique<Memory::MemoryManager>();
    if (!memory) {
      fprintf(stderr, "[Emulator] ERROR: Failed to create Memory Manager\n");
      return false;
    }

    // Initialize Kernel Manager
    printf("[Emulator] Initializing Kernel Manager...\n");
    kernel = std::make_unique<Kernel::KernelManager>();
    if (!kernel) {
      fprintf(stderr, "[Emulator] ERROR: Failed to create Kernel Manager\n");
      return false;
    }

    // Initialize Module Manager
    printf("[Emulator] Initializing Module Manager...\n");
    module_manager = std::make_unique<Kernel::ModuleManager>();
    if (!module_manager) {
      fprintf(stderr, "[Emulator] ERROR: Failed to create Module Manager\n");
      return false;
    }

    // Register HLE Libraries
    Core::Libraries::Kernel::RegisterLibKernel(module_manager.get());
    Core::Libraries::Libc::RegisterLibc(module_manager.get());

    // Initialize Service Manager
    printf("[Emulator] Initializing Service Manager...\n");
    services = std::make_unique<Services::ServiceManager>();
    if (!services) {
      fprintf(stderr, "[Emulator] ERROR: Failed to create Service Manager\n");
      return false;
    }

    // Initialize FileSystem Components
    printf("[Emulator] Initializing FileSystem...\n");
    mnt_points = std::make_unique<FileSys::MntPoints>();
    handle_table = std::make_unique<FileSys::HandleTable>();
    if (!mnt_points || !handle_table) {
      fprintf(stderr,
              "[Emulator] ERROR: Failed to create FileSystem components\n");
      return false;
    }

    // Create standard handles (stdin, stdout, stderr)
    handle_table->CreateStdHandles();

    // Initialize Orbis System
    printf("[Emulator] Initializing Orbis System...\n");
    system = std::make_unique<OS::OrbisSystem>();
    if (!system) {
      fprintf(stderr, "[Emulator] ERROR: Failed to create Orbis System\n");
      return false;
    }

    // Initialize ELF Loader
    printf("[Emulator] Initializing ELF Loader...\n");
    loader = std::make_unique<Loader::ElfLoader>();
    if (!loader) {
      fprintf(stderr, "[Emulator] ERROR: Failed to create ELF Loader\n");
      return false;
    }

    // Wire up the Orbis System with all subsystem pointers
    printf("[Emulator] Initializing Orbis System subsystems...\n");
    if (!system->Initialize(memory.get(), kernel.get(), module_manager.get(),
                            services.get(), mnt_points.get(),
                            handle_table.get())) {
      fprintf(stderr, "[Emulator] ERROR: Failed to initialize Orbis System\n");
      return false;
    }

    printf("[OrbisSystem] Orbis OS layer initialized successfully\n");

  } catch (const std::exception &e) {
    fprintf(stderr, "[Emulator] ERROR: Exception during initialization: %s\n",
            e.what());
    return false;
  }

  state = EmulatorState::Stopped;
  printf("[Emulator] Initialization complete\n");
  return true;
}

bool Emulator::LoadExecutable(const std::string &path) {
  printf("[Emulator] Loading executable: %s\n", path.c_str());

  if (state != EmulatorState::Stopped) {
    fprintf(
        stderr,
        "[Emulator] ERROR: Cannot load executable while emulator is running\n");
    return false;
  }

  if (!loader) {
    fprintf(stderr, "[Emulator] ERROR: ELF Loader not initialized\n");
    return false;
  }

  if (!memory) {
    fprintf(stderr, "[Emulator] ERROR: Memory Manager not initialized\n");
    return false;
  }

  try {
    // TODO: Determine if it's a PKG or ELF file
    // For now, assume it's an ELF

    // Use the ELF loader to load the executable
    auto result = loader->Load(path);
    if (!result.success) {
      fprintf(stderr, "[Emulator] ERROR: ELF Loader failed for %s: %s\n",
              path.c_str(), result.error_msg.c_str());
      return false;
    }

    printf("[Emulator] Executable loaded successfully. Entry: 0x%llx, Base: "
           "0x%llx\n",
           result.entry_point, result.load_base);

    // Register module with ModuleManager
    if (module_manager) {
      module_manager->RegisterModule(path, result.load_base, result.image_size,
                                     result.entry_point);
    }

    // Create initial thread with KernelManager
    if (kernel) {
      uint32_t handle =
          kernel->CreateThread("main", result.entry_point, 100 /* priority */);
      kernel->StartThread(handle);
    }

    return true;

  } catch (const std::exception &e) {
    fprintf(stderr, "[Emulator] ERROR: Failed to load executable: %s\n",
            e.what());
    return false;
  }
}

void Emulator::Run() {
  printf("[Emulator] Starting...\n");

  if (state == EmulatorState::Running) {
    printf("[Emulator] WARNING: Already running\n");
    return;
  }

  if (state != EmulatorState::Stopped && state != EmulatorState::Paused) {
    fprintf(stderr, "[Emulator] ERROR: Cannot run emulator in current state\n");
    return;
  }

  state = EmulatorState::Running;
  printf("[Emulator] Started\n");
}

void Emulator::Pause() {
  printf("[Emulator] Pausing...\n");

  if (state != EmulatorState::Running) {
    printf("[Emulator] WARNING: Not running, cannot pause\n");
    return;
  }

  state = EmulatorState::Paused;
  printf("[Emulator] Paused\n");
}

void Emulator::Stop() {
  printf("[Emulator] Stopping...\n");

  if (state == EmulatorState::Stopped) {
    printf("[Emulator] WARNING: Already stopped\n");
    return;
  }

  state = EmulatorState::Stopping;

  // Clean up resources
  // Note: unique_ptrs will automatically clean up when reset or destroyed

  state = EmulatorState::Stopped;
  printf("[Emulator] Stopped\n");
}

void Emulator::Step() {
  // Only step if running
  if (state != EmulatorState::Running) {
    return;
  }

  // Execute one CPU cycle/instruction
  // This is where you'd:
  // 1. Fetch the next instruction from memory
  // 2. Decode the instruction
  // 3. Execute the instruction
  // 4. Update program counter
  // 5. Handle interrupts/exceptions

  // TODO: Implement actual CPU stepping
  // This requires:
  // - CPU state (registers, PC, flags)
  // - Instruction decoder
  // - Instruction executor
  // - Memory interface
}

} // namespace Core