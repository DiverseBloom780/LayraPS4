#include "core/cpu/jaguar_core.h"
#include "core/os/orbis_system.h"

// In your CPU execution loop:
void JaguarCore::ExecuteInstruction() {
    // Decode instruction
    // ...
    
    // Check for SYSCALL/SYSRET instructions
    if (current_opcode == 0x0F05) { // SYSCALL
        HandleSyscall();
    }
}

void JaguarCore::HandleSyscall() {
    // Get syscall number from RAX
    uint32_t syscall_id = static_cast<uint32_t>(regs.gpr[0]); // RAX
    
    // Get arguments from registers (System V AMD64 ABI)
    uint64_t args[8] = {
        regs.gpr[7],  // RDI - arg1
        regs.gpr[6],  // RSI - arg2
        regs.gpr[2],  // RDX - arg3
        regs.gpr[1],  // RCX - arg4
        regs.gpr[8],  // R8  - arg5
        regs.gpr[9],  // R9  - arg6
        0, 0          // Stack args handled separately
    };
    
    // Get stack arguments if needed
    if (syscall_id >= 512) { // PS4 syscalls use stack for >6 args
        // Read from stack pointer
    }
    
    // Call OS emulation layer
    uint64_t result = g_orbis_system->HandleSyscall(syscall_id, 
        args[0], args[1], args[2], args[3],
        args[4], args[5], args[6], args[7]);
    
    // Store result in RAX
    regs.gpr[0] = result;
}
