// SPDX-FileCopyrightText: Copyright 2025 LayraPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "module_manager.h"
#include <cstdio>
#include <map>
#include <vector>

namespace Core::Kernel {

ModuleManager::ModuleManager() { printf("[ModuleManager] Initialized\n"); }

ModuleManager::~ModuleManager() { printf("[ModuleManager] Shutdown\n"); }

std::vector<ModuleInfo> ModuleManager::GetLoadedModules() const { return {}; }

uintptr_t ModuleManager::ResolveSymbol(const std::string &module,
                                       const std::string &symbol) {
  // If a module name is provided, search specifically in that module
  if (!module.empty()) {
    // Search in HLE modules first
    auto hle_it = hle_modules.find(module);
    if (hle_it != hle_modules.end()) {
      for (const auto &exp : hle_it->second) {
        if (exp.name == symbol || exp.nid == symbol) {
          return static_cast<uintptr_t>(exp.host_address);
        }
      }
    }

    // Search in natively loaded modules (from ELF)
    for (const auto &mi : loaded_modules) {
      if (mi.name == module) {
        auto sym_it = mi.exports.find(symbol);
        if (sym_it != mi.exports.end()) {
          return static_cast<uintptr_t>(sym_it->second.address);
        }
      }
    }
  } else {
    // Global search across all HLE modules
    for (const auto &[mod_name, exports] : hle_modules) {
      for (const auto &exp : exports) {
        if (exp.name == symbol || exp.nid == symbol) {
          return static_cast<uintptr_t>(exp.host_address);
        }
      }
    }

    // Global search across all natively loaded modules
    for (const auto &mi : loaded_modules) {
      auto sym_it = mi.exports.find(symbol);
      if (sym_it != mi.exports.end()) {
        return static_cast<uintptr_t>(sym_it->second.address);
      }
    }
  }

  return 0;
}

uint32_t ModuleManager::RegisterModule(const std::string &name,
                                       uint64_t baseAddress, uint64_t size,
                                       uint64_t entryPoint) {
  // Simple handle generation
  static uint32_t next_handle = 0x1000;
  uint32_t handle = next_handle++;

  // Store module info
  // For now we just log it as we don't have a full member list in the class
  // private section yet Wait, the header didn't show private members. I should
  // check if I need to add them.
  printf("[ModuleManager] Registered module: %s (Handle: 0x%X, Base: 0x%llx, "
         "Size: 0x%llx, Entry: 0x%llx)\n",
         name.c_str(), handle, baseAddress, size, entryPoint);

  // Add to internal list
  ModuleInfo info;
  info.handle = handle;
  info.name = name;
  info.baseAddress = baseAddress;
  loaded_modules.push_back(info);

  return handle;
}

void ModuleManager::RegisterHLEExport(const std::string &moduleName,
                                      const std::string &nid,
                                      const std::string &name,
                                      uint64_t hostAddress) {
  Core::Loader::HLEExport exportInfo;
  exportInfo.name = name;
  exportInfo.nid = nid;
  exportInfo.host_address = hostAddress;

  hle_modules[moduleName].push_back(exportInfo);
  // printf("[ModuleManager] Registered HLE export: %s#%s (%s)\n",
  // moduleName.c_str(), nid.c_str(), name.c_str());
}

uint64_t ModuleManager::ResolveHLEExport(const std::string &moduleName,
                                         const std::string &nid) {
  if (hle_modules.find(moduleName) != hle_modules.end()) {
    const auto &exports = hle_modules[moduleName];
    for (const auto &exp : exports) {
      if (exp.nid == nid) {
        return exp.host_address;
      }
    }
  }
  return 0;
}

} // namespace Core::Kernel
