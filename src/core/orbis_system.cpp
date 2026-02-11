#include "orbis_system.h"
#include "core/kernel/kernel_manager.h"
#include "core/memory/memory_manager.h"
#include "core/services/audio_service.h"
#include "core/services/pad_service.h"
#include "core/services/service_manager.h"
#include "gui/gui_debugger.h"
#include <filesystem>
#include <fstream>
#include <iostream>

namespace Core {
namespace OS {

OrbisSystem::OrbisSystem() { SetupMounts(); }

OrbisSystem::~OrbisSystem() {}

bool OrbisSystem::Initialize(Memory::MemoryManager *memoryManager,
                             Services::ServiceManager *serviceManager,
                             Kernel::KernelManager *kernelManager) {
  std::cout << "[Orbis] Initializing OS Layer...\n";
  memory = memoryManager;
  services = serviceManager;
  kernel = kernelManager;
  return true;
}

void OrbisSystem::SetupMounts() {
  // Basic PS4 mount points mapped to host
  mountPoints["/app0/"] = "emu/host/app/";
  mountPoints["/data/"] = "emu/host/data/";
  mountPoints["/system/"] = "emu/host/system/";

  // Ensure directories exist
  for (auto const &[orbis, host] : mountPoints) {
    std::filesystem::create_directories(host);
  }
}

std::string OrbisSystem::TranslatePath(const std::string &orbisPath) {
  for (auto const &[orbis, host] : mountPoints) {
    if (orbisPath.find(orbis) == 0) {
      return host + orbisPath.substr(orbis.length());
    }
  }
  // Fallback: map root to app0 for now
  return mountPoints["/app0/"] + (orbisPath.empty() || orbisPath[0] == '/'
                                      ? orbisPath.substr(1)
                                      : orbisPath);
}

void OrbisSystem::Shutdown() {
  std::cout << "[Orbis] Shutting down OS Layer...\n";
}

int64_t OrbisSystem::HandleSyscall(uint32_t syscall_id,
                                   const std::vector<uint64_t> &args) {
  // Log to GUI Debugger
  std::stringstream ss;
  ss << "[Syscall] ID: " << syscall_id;
  Gui::GuiDebugger::GetInstance().AddLog(ss.str(),
                                         ImVec4(0.4f, 0.8f, 1.0f, 1.0f));

  switch (syscall_id) {
  case 1: // exit
    std::cout << "[Orbis] syscall: exit(" << args[0] << ")\n";
    return 0;
  case 20: // getpid
    return GetCurrentPID();

  // File System Syscalls
  case 5: // sceKernelOpen
  {
    char path[512];
    memory->Read(args[0], path, 512);
    std::string hostPath = TranslatePath(path);
    int flags = (int)args[1];
    int mode = (int)args[2];

    std::cout << "[Orbis] sceKernelOpen('" << path << "' -> '" << hostPath
              << "', flags=" << flags << ")\n";

    // Use standard fopen/open or similar. For now, dummy fd.
    return 100; // Fake FD
  }
  case 6: // sceKernelClose
  {
    std::cout << "[Orbis] sceKernelClose(fd=" << args[0] << ")\n";
    return 0;
  }
  case 3: // read
  {
    // args[0]=fd, args[1]=buf, args[2]=len
    std::cout << "[Orbis] syscall: read(fd=" << args[0] << ", buf=0x"
              << std::hex << args[1] << ", len=0x" << args[2] << std::dec
              << ")\n";
    return 0;
  }
  case 4: // write
    std::cout << "[Orbis] syscall: write(fd=" << args[0] << ", buf=0x"
              << std::hex << args[1] << ", len=0x" << args[2] << std::dec
              << ")\n";
    return 0;

  case 54: // ioctl
    std::cout << "[Orbis] syscall: ioctl(fd=" << args[0] << ", req=0x"
              << std::hex << args[1] << std::dec << ")\n";
    return 0;

  case 587: // sceKernelAllocateMainDirectMemory
  {
    uint64_t size = args[0];
    uint64_t alignment = args[1];
    uint32_t type = static_cast<uint32_t>(args[2]);
    uint64_t out_addr_ptr = args[3];

    if (memory) {
      uint64_t addr = memory->Map(0, size, 0x3, "MainDirectMemory");
      if (addr != 0) {
        memory->Write(out_addr_ptr, &addr, sizeof(addr));
        return 0;
      }
    }
    return -1;
  }
  case 477: // mmap
  {
    uint64_t addr = args[0];
    uint64_t len = args[1];
    uint32_t prot = static_cast<uint32_t>(args[2]);
    if (memory) {
      return memory->Map(addr, len, prot, "Anonymous");
    }
    return 0;
  }
  // Mutex syscalls
  case 655: // sceKernelCreateMutex
  {
    if (kernel)
      return kernel->CreateMutex("unnamed_mutex", (u32)args[1]);
    return -1;
  }
  case 656: // sceKernelLockMutex
  {
    if (kernel)
      return kernel->LockMutex((s32)args[0], (u32)args[1]);
    return -1;
  }
  case 657: // sceKernelUnlockMutex
  {
    if (kernel)
      return kernel->UnlockMutex((s32)args[0]);
    return -1;
  }
  // Threading syscalls
  case 499: // sceKernelCreateThread
  {
    if (kernel)
      return kernel->CreateThread("unnamed_thread", args[1], args[2],
                                  (u32)args[3], (int)args[4]);
    return -1;
  }
  case 500: // sceKernelStartThread
  {
    if (kernel)
      return kernel->StartThread((s32)args[0]);
    return -1;
  }
  case 501: // sceKernelWaitThreadEnd
  {
    if (kernel)
      return kernel->JoinThread((s32)args[0], nullptr);
    return -1;
  }
  // sceAudioOut syscalls
  case 164: // sceAudioOutOpen
  {
    if (services) {
      auto audio = static_cast<Services::AudioService *>(
          services->GetService("sceAudio"));
      if (audio)
        return audio->AudioOutOpen(args[0], args[1], args[2], args[3]);
    }
    return -1;
  }
  case 168: // sceAudioOutOutput
  {
    if (services) {
      auto audio = static_cast<Services::AudioService *>(
          services->GetService("sceAudio"));
      if (audio) {
        std::vector<uint8_t> audio_data(256 * 2 * 2);
        memory->Read(args[1], audio_data.data(), audio_data.size());
        return audio->AudioOutOutput(static_cast<int>(args[0]),
                                     audio_data.data());
      }
    }
    return -1;
  }
  // scePad syscalls
  case 533: // scePadInit
  {
    if (services) {
      auto pad =
          static_cast<Services::PadService *>(services->GetService("scePad"));
      if (pad)
        return pad->PadInit();
    }
    return -1;
  }
  case 534: // scePadOpen
  {
    if (services) {
      auto pad =
          static_cast<Services::PadService *>(services->GetService("scePad"));
      if (pad)
        return pad->PadOpen(args[0], args[1], args[2], nullptr);
    }
    return -1;
  }
  case 535: // scePadRead
  {
    if (services) {
      auto pad =
          static_cast<Services::PadService *>(services->GetService("scePad"));
      if (pad) {
        Services::OrbisPadData data;
        int res = pad->PadRead((int)args[0], &data, (int)args[2]);
        if (res == 0) {
          memory->Write(args[1], &data, sizeof(data));
        }
        return res;
      }
    }
    return -1;
  }
  default:
    std::cout << "[Orbis] Unknown syscall: " << syscall_id << "\n";
    return -1;
  }
}

} // namespace OS
} // namespace Core
