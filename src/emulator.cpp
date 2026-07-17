// SPDX-FileCopyrightText: Copyright 2025 LayraPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cmath>
#include <cstdio>
#include <iostream>

#include "core/file_sys/fs.h"
#include "core/kernel/kernel_manager.h"
#include "core/kernel/module_manager.h"
#include "core/kernel/syscalls.h"
#include "layra_pkg.h"
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include "core/libraries/app_content/app_content.h"
#include "core/libraries/gnmdriver/gnmdriver.h"
#include "core/libraries/kernel/libkernel.h"
#include "core/libraries/libc/libc.h"
#include "core/libraries/pad/pad.h"
#include "core/libraries/sysmodule/sysmodule.h"
#include "core/libraries/system/systemservice.h"
#include "core/libraries/system/userservice.h"
#include "core/libraries/videoout/video_out.h"
#include "core/loader/elf_loader.h"
#include "core/memory/memory_manager.h"
#include "core/orbis_system.h"
#include "core/services/service_manager.h"
#include "core/libraries/nptrophy/nptrophy.h"
#include "core/libraries/savedata/savedata.h"
#include "core/libraries/audioout/audioout.h"
#include "emulator.h"

#include <array>
#include <optional>
#include <string>

namespace Core {

enum class ExecutableFormat {
  Unknown,
  Elf,
  Pkg,
  Self,
};

static ExecutableFormat DetectExecutableFormat(const std::string &path) {
  std::ifstream file(path, std::ios::binary);
  if (!file) {
    return ExecutableFormat::Unknown;
  }

  std::array<unsigned char, 4> header{};
  file.read(reinterpret_cast<char *>(header.data()), header.size());
  if (!file) {
    return ExecutableFormat::Unknown;
  }

  if (header == std::array<unsigned char, 4>{0x7F, 'E', 'L', 'F'}) {
    return ExecutableFormat::Elf;
  }

  if (header == std::array<unsigned char, 4>{0x7F, 'C', 'N', 'T'}) {
    return ExecutableFormat::Pkg;
  }

  if (header == std::array<unsigned char, 4>{0x1D, 0x3D, 0x15, 0x4F} ||
      header == std::array<unsigned char, 4>{0x4F, 0x15, 0x3D, 0x1D}) {
    return ExecutableFormat::Self;
  }

  return ExecutableFormat::Unknown;
}

static bool MountGameRoot(FileSys::MntPoints *mnt_points,
                          const std::filesystem::path &host_path) {
  if (!mnt_points) {
    return false;
  }

  if (host_path.empty() || !std::filesystem::exists(host_path) ||
      !std::filesystem::is_directory(host_path)) {
    return false;
  }

  mnt_points->Mount(host_path, "/app0", true);
  return true;
}

static std::optional<std::filesystem::path>
FindGameExecutableInDirectory(const std::filesystem::path &root) {
  if (root.empty() || !std::filesystem::exists(root) ||
      !std::filesystem::is_directory(root)) {
    return std::nullopt;
  }

  std::filesystem::path candidate = root / "eboot.bin";
  if (std::filesystem::is_regular_file(candidate)) {
    return candidate;
  }

  for (const auto &entry : std::filesystem::recursive_directory_iterator(root)) {
    if (!entry.is_regular_file()) {
      continue;
    }

    auto filename = entry.path().filename().string();
    auto extension = entry.path().extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(),
                   ::tolower);

    if (filename == "eboot.bin" || extension == ".elf" ||
        extension == ".self") {
      return entry.path();
    }
  }

  return std::nullopt;
}

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
    if (!memory || !memory->IsInitialized()) {
      fprintf(stderr, "[Emulator] ERROR: Failed to initialize Memory Manager or it is not properly initialized\n");
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
    Core::Libraries::VideoOut::RegisterVideoOut(module_manager.get());
    Core::Libraries::GnmDriver::RegisterGnmDriver(module_manager.get());
    Core::Libraries::UserService::RegisterUserService(module_manager.get());
    Core::Libraries::SystemService::RegisterSystemService(module_manager.get());
    Core::Libraries::SysModule::RegisterSysModule(module_manager.get());
    Core::Libraries::Pad::RegisterPad(module_manager.get());
    Core::Libraries::AppContent::RegisterAppContent(module_manager.get());
    Core::Libraries::NpTrophy::RegisterNpTrophy(module_manager.get());
    Core::Libraries::SaveData::RegisterSaveData(module_manager.get());
    Core::Libraries::AudioOut::RegisterAudioOut(module_manager.get());

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

    // Wire filesystem pointers into libkernel HLE
    Core::Libraries::Kernel::SetFileSysPointers(mnt_points.get(),
                                                handle_table.get());

    // Initialize Orbis System
    printf("[Emulator] Initializing Orbis System...\n");
    system = std::make_unique<OS::OrbisSystem>();
    if (!system) {
      fprintf(stderr, "[Emulator] ERROR: Failed to create Orbis System\n");
      return false;
    }

    // Initialize ELF Loader
    printf("[Emulator] Initializing ELF Loader...\n");
    loader = std::make_unique<Loader::ElfLoader>(memory.get(),
                                                module_manager.get());
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

void Emulator::SetVulkanContext(void* device, void* physDevice, void* gfxQueue, uint32_t queueFamily, void* renderPass, uint32_t width, uint32_t height) {
  Core::Libraries::GnmDriver::SetVulkanContext(device, physDevice, gfxQueue, queueFamily, renderPass, width, height);
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
    std::filesystem::path source_path(path);
    std::string executable_path = path;
    std::string ext;
    bool is_pkg = false;
    bool mounted_game_root = false;

    if (std::filesystem::exists(source_path) &&
        std::filesystem::is_directory(source_path)) {
      auto game_executable = FindGameExecutableInDirectory(source_path);
      if (!game_executable.has_value()) {
        fprintf(stderr,
                "[Emulator] ERROR: Could not find eboot.bin, .elf, or .self in game directory %s\n",
                source_path.string().c_str());
        return false;
      }

      executable_path = game_executable->string();
      if (!MountGameRoot(mnt_points.get(), source_path)) {
        fprintf(stderr,
                "[Emulator] ERROR: Failed to mount game directory %s to /app0\n",
                source_path.string().c_str());
        return false;
      }
      mounted_game_root = true;
      printf("[Emulator] Mounted game directory %s as /app0\n",
             source_path.string().c_str());

      if (DetectExecutableFormat(executable_path) == ExecutableFormat::Self) {
        printf("[Emulator] SELF file detected in game directory (%s). Will parse SELF container.\n",
               executable_path.c_str());
      }
    }

    // Detect PKG by file extension or magic header.
    if (!mounted_game_root && source_path.has_extension()) {
      ext = source_path.extension().string();
      std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
      if (ext == ".pkg") {
        is_pkg = true;
      }
    }

    if (!mounted_game_root && !is_pkg) {
      ExecutableFormat format = DetectExecutableFormat(path);
      switch (format) {
      case ExecutableFormat::Elf:
        printf("[Emulator] Detected ELF executable: %s\n", path.c_str());
        if (source_path.has_parent_path()) {
          MountGameRoot(mnt_points.get(), source_path.parent_path());
        }
        break;
      case ExecutableFormat::Pkg:
        is_pkg = true;
        break;
      case ExecutableFormat::Self:
        printf("[Emulator] SELF file detected (%s). Will parse SELF container.\n",
               path.c_str());
        if (source_path.has_parent_path()) {
          MountGameRoot(mnt_points.get(), source_path.parent_path());
        }
        break;
      default:
        if (ext == ".elf") {
          fprintf(stderr,
                  "[Emulator] ERROR: .elf extension detected but file header is invalid ELF: %s\n",
                  path.c_str());
        } else {
          fprintf(stderr,
                  "[Emulator] ERROR: Unsupported executable format for %s. Only raw ELF and PKG-wrapped ELF are currently supported.\n",
                  path.c_str());
        }
        return false;
      }
    }

    if (is_pkg) {
      std::filesystem::path mount_point =
          std::filesystem::temp_directory_path() / "layra_pkg_vfs";
      if (!layra_pkg_open_and_mount(path.c_str(), mount_point.string().c_str())) {
        fprintf(stderr, "[Emulator] ERROR: Failed to mount PKG file %s\n", path.c_str());
        return false;
      }

      if (!MountGameRoot(mnt_points.get(), mount_point)) {
        fprintf(stderr,
                "[Emulator] ERROR: Failed to mount extracted PKG contents %s to /app0\n",
                mount_point.string().c_str());
        return false;
      }
      printf("[Emulator] Mounted PKG contents %s as /app0\n",
             mount_point.string().c_str());

      auto game_executable = FindGameExecutableInDirectory(mount_point);
      if (!game_executable.has_value()) {
        fprintf(stderr,
                "[Emulator] ERROR: No ELF executable found inside PKG %s\n",
                path.c_str());
        return false;
      }

      executable_path = game_executable->string();
      if (DetectExecutableFormat(executable_path) == ExecutableFormat::Self) {
        printf("[Emulator] SELF file detected inside PKG (%s). Will parse SELF container.\n",
               executable_path.c_str());
      }
    }

    // Use the ELF loader to load the executable
    auto result = loader->Load(executable_path);
    if (!result.success) {
      fprintf(stderr, "[Emulator] ERROR: ELF Loader failed for %s: %s\n",
              executable_path.c_str(), result.error_msg.c_str());
      return false;
    }

    printf("[Emulator] Executable loaded successfully. Entry: 0x%llx, Base: "
           "0x%llx\n",
           result.entry_point, result.load_base);

    // Register module with ModuleManager
    if (module_manager) {
      module_manager->RegisterModule(executable_path, result.load_base,
                                     result.image_size, result.entry_point);
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
  if (state != EmulatorState::Running) {
    return;
  }

  if (!kernel) {
    return;
  }

  auto threads = kernel->GetThreadList();
  bool any_active = false;
  for (const auto &thread : threads) {
    if (!thread.exited) {
      any_active = true;
      break;
    }
  }

  if (!any_active) {
    printf("[Emulator] All guest threads have exited, stopping emulator\n");
    Stop();
  }
}

} // namespace Core