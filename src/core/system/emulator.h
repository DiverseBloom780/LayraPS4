#pragma once
#include <memory>
#include <vector>
#include "core/cpu/jaguar_core.h"
#include "core/memory/memory_manager.h"

namespace PS4 {

class EmulatorCore {
private:
    // Hardware components
    std::vector<std::unique_ptr<CPU::JaguarCore>> cpu_cores;
    std::unique_ptr<Memory::MemoryManager> memory;
    
    // System state
    bool is_running;
    bool is_paused;
    
public:
    EmulatorCore();
    ~EmulatorCore();
    
    // Core lifecycle
    void Initialize();
    void Run();
    void Pause();
    void Stop();
    
    // CPU management
    void StepCPU();          // Execute one instruction on all cores
    void RunCPU(uint64_t cycles);  // Run for specified cycles
    
    // Memory access (for debugging/GUI)
    uint64_t ReadMemory(uint64_t addr, size_t size);
    void WriteMemory(uint64_t addr, uint64_t value, size_t size);
    
    // Get component references (for GUI)
    Memory::MemoryManager* GetMemoryManager() { return memory.get(); }
    
private:
    void InitializeCPU();
    void InitializeMemory();
    void InitializeDevices();
};

} // namespace PS4
