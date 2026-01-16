#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <memory>

namespace PS4 {
namespace OS {

// PS4 module formats
enum class ModuleFormat {
    PRX,    // Process Executable
    SPRX,   // Secure PRX
    SELF,   // Signed ELF
    ELF     // Unsigned ELF
};

// Module information structure
struct ModuleInfo {
    uint64_t handle;
    std::string name;
    ModuleFormat format;
    uint64_t base_address;
    uint64_t size;
    uint64_t entry_point;
    
    // Export/import tables
    std::vector<uint64_t> exports;
    std::vector<uint64_t> imports;
};

// Module loader for PRX/SPRX files
class ModuleLoader {
private:
    std::vector<ModuleInfo> loaded_modules;
    uint64_t next_handle;
    
public:
    ModuleLoader();
    
    // Load a module from file
    bool Load(const std::string& path, ModuleFormat format, uint64_t* handle);
    
    // Relocate module for specific address
    bool RelocateModule(ModuleInfo& module, uint64_t base_addr);
    
    // Resolve imports
    bool ResolveImports(ModuleInfo& module);
    
    // Get module by handle
    ModuleInfo* GetModule(uint64_t handle);
    
    // Get module by name
    ModuleInfo* GetModule(const std::string& name);
    
private:
    bool LoadPRX(const std::string& path, ModuleInfo* info);
    bool LoadSELF(const std::string& path, ModuleInfo* info);
    bool LoadELF(const std::string& path, ModuleInfo* info);
};

} // namespace OS
} // namespace PS4
