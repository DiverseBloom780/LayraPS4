#pragma once
#include <cstdint>

namespace Core::Kernel {

class SyscallHandler;

// Install the Windows Vectored Exception Handler that intercepts
// guest x86-64 syscall/int instructions and dispatches to HLE.
void InstallExceptionHandler(SyscallHandler* handler);
void RemoveExceptionHandler();

} // namespace Core::Kernel
