#pragma once
#include <memory>
#include <unordered_map>
#include <vector>
#include <string>

namespace PS4 {
namespace OS {

// Orbis OS version and constants
struct OrbisConstants {
    static constexpr uint32_t ORBIS_OS_VERSION = 0x09400000;  // 9.4.0.0
    static constexpr uint32_t SDK_VERSION = 0x04500000;       // 4.50
    static constexpr size_t SYSTEM_RESERVED_MEMORY = 0xC0000000;  // 3GB
};

// System service types (like PS4 has)
enum class ServiceType {
    SCE_VIDEO_OUT,
    SCE_PAD,
    SCE_AUDIO_OUT,
    SCE_SAVE_DATA,
    SCE_NP,
    SCE_SYSTEM_SERVICE,
    // ... many more
};

// Main OS emulation class
class OrbisOS {
private:
    // Kernel state
    bool kernel_initialized;
    uint64_t kernel_base_address;
    
    // System services
    std::unordered_map<ServiceType, std::unique_ptr<class SystemService>> services;
    
    // Loaded modules
    std::vector<class PS4Module*> loaded_modules;
    
    // Process management
    class ProcessManager* process_manager;
    
    // System call table
    std::array<void*, 1024> syscall_table;
    
public:
    OrbisOS();
    ~OrbisOS();
    
    // OS lifecycle
    void Boot();            // Cold boot sequence
    void InitializeKernel(); // Initialize Orbis kernel
    void Shutdown();
    
    // System call handling
    uint64_t HandleSyscall(uint32_t syscall_id, uint64_t arg1, 
                          uint64_t arg2, uint64_t arg3, uint64_t arg4,
                          uint64_t arg5, uint64_t arg6, uint64_t arg7, uint64_t arg8);
    
    // Module loading (PRX/SRX files)
    bool LoadModule(const std::string& path, uint64_t* handle);
    bool UnloadModule(uint64_t handle);
    
    // Service management
    void* GetService(ServiceType type);
    
    // Process management
    uint32_t CreateProcess(const std::string& elf_path);
    void DestroyProcess(uint32_t pid);
    
    // Memory management for OS
    void* AllocateOSMemory(size_t size, size_t alignment);
    void FreeOSMemory(void* ptr);
    
private:
    void InitializeSyscallTable();
    void InitializeServices();
    void SetupVirtualFilesystem();
};

} // namespace OS
} // namespace PS4
