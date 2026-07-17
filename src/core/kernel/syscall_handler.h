#pragma once

#include <stdint.h>
#include <functional>
#include <string>
#include <unordered_map>

namespace Core::Kernel {

class KernelManager;

// Unified Syscall function signature used in syscalls.cpp
using SyscallFn = std::function<uint64_t(void*, uint64_t*)>;

struct SyscallEntry {
    SyscallFn handler;
    std::string name;
    int id;
};

class SyscallHandler {
public:
    SyscallHandler(KernelManager* kernel);
    ~SyscallHandler();
    
    void RegisterSyscall(int id, SyscallFn handler, const std::string &name);
    uint64_t Dispatch(int id, void *context, uint64_t *args);

private:
    std::unordered_map<int, SyscallEntry> syscall_table;
    KernelManager* kernel;

    // HLE Syscall Implementations
    uint64_t sys_exit(void* context, uint64_t* args);
    uint64_t sys_write(void* context, uint64_t* args);
    uint64_t sys_getpid(void* context, uint64_t* args);
};

} // namespace Core::Kernel