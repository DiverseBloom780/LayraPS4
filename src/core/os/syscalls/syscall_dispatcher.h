#pragma once
#include <cstdint>

namespace PS4 {
namespace OS {
namespace Syscalls {

// PS4 uses FreeBSD-style syscalls but with Sony extensions
class SyscallDispatcher {
private:
    // Registered syscall handlers
    using SyscallHandler = uint64_t(*)(uint64_t, uint64_t, uint64_t, 
                                      uint64_t, uint64_t, uint64_t,
                                      uint64_t, uint64_t);
    SyscallHandler handlers[1024];
    
public:
    SyscallDispatcher();
    
    // Register a syscall handler
    void RegisterSyscall(uint32_t number, SyscallHandler handler);
    
    // Dispatch a syscall
    uint64_t Dispatch(uint32_t number, uint64_t arg1, uint64_t arg2,
                     uint64_t arg3, uint64_t arg4, uint64_t arg5,
                     uint64_t arg6, uint64_t arg7, uint64_t arg8);
    
    // Initialize with default syscalls
    void InitializeDefaultHandlers();
};

// Example syscall implementations
uint64_t sys_write(uint64_t fd, uint64_t buf, uint64_t count, 
                   uint64_t arg4, uint64_t arg5, uint64_t arg6,
                   uint64_t arg7, uint64_t arg8);

uint64_t sys_read(uint64_t fd, uint64_t buf, uint64_t count,
                  uint64_t arg4, uint64_t arg5, uint64_t arg6,
                  uint64_t arg7, uint64_t arg8);

// Sony-specific syscalls
uint64_t sceKernelGetLibkernelTextLocation(uint64_t unknown);
uint64_t sceKernelLoadStartModule(uint64_t name, uint64_t args,
                                  uint64_t argp, uint64_t flags,
                                  uint64_t arg4, uint64_t arg5,
                                  uint64_t arg6, uint64_t arg7);

} // namespace Syscalls
} // namespace OS
} // namespace PS4
