#pragma once
#include <cstdint>
#include <array>

namespace PS4 {
namespace CPU {

// Jaguar core registers (x86-64 plus PS4 extensions)
struct JaguarRegisters {
    // General purpose 64-bit registers
    std::array<uint64_t, 16> gpr;
    
    // Special registers
    uint64_t rip;       // Instruction pointer
    uint64_t rflags;    // Flags register
    
    // Model Specific Registers (MSRs) for PS4
    uint64_t apic_base;
    uint64_t efer;
    uint64_t star;
    uint64_t lstar;     // SYSCALL target
    
    // Floating point/vector
    std::array<uint8_t, 512> xmm;  // 16 x 32-byte registers
};

// Single Jaguar CPU core
class JaguarCore {
private:
    JaguarRegisters regs;
    uint32_t core_id;
    bool is_running;
    
public:
    JaguarCore(uint32_t id) : core_id(id), is_running(false) {}
    
    void Reset();
    void ExecuteInstruction();
    void HandleInterrupt(uint8_t vector);
    
    // Memory access methods
    uint64_t ReadMemory(uint64_t addr, size_t size);
    void WriteMemory(uint64_t addr, uint64_t value, size_t size);
    
    uint32_t GetCoreID() const { return core_id; }
};

} // namespace CPU
} // namespace PS4
