#pragma once

#include <memory>
#include <vector>
#include <string>
#include <stdint.h>

namespace Core::Kernel {

class SyscallHandler;

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

    bool Initialize();
    
    std::vector<ThreadInfo> GetThreadList() const;

    // Thread management
    uint32_t CreateThread(const std::string &name, uint64_t entryPoint,
                        uint64_t priority, uint64_t stackSize = 0,
                        uint64_t arg = 0);
    void StartThread(uint32_t handle);
    void ExitThread(uint32_t handle, int exitCode);
    void JoinThread(uint32_t handle);

    SyscallHandler* GetSyscallHandler() const { return syscall_handler; }

private:
    // Owned by the Manager, responsible for Orbis OS syscall dispatching
    SyscallHandler* syscall_handler = nullptr;
    // Memory management and module loading would also live here in later phases
};

} // namespace Core::Kernel