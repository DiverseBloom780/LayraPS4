// SPDX-FileCopyrightText: Copyright 2025 LayraPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "module_manager.h"
#include <algorithm>
#include <iostream>


namespace Core {
namespace Kernel {

ModuleManager::ModuleManager() {
  std::cout << "[Kernel] Module Manager initialized.\n";
}

ModuleManager::~ModuleManager() {
  std::lock_guard<std::mutex> lock(mutex);
  modules.clear();
}

s32 ModuleManager::AllocateHandle() { return nextHandle++; }

s32 ModuleManager::RegisterModule(const std::string &name, u64 base, u64 size) {
  std::lock_guard<std::mutex> lock(mutex);
  s32 handle = AllocateHandle();

  ModuleInfo info;
  info.handle = handle;
  info.name = name;
  info.baseAddress = base;
  info.size = size;

  modules[handle] = std::move(info);

  std::cout << "[Kernel] Registered Module: " << name << " at 0x" << std::hex
            << base << " (size=0x" << size << ")\n"
            << std::dec;

  return handle;
}

void ModuleManager::UnregisterModule(s32 handle) {
  std::lock_guard<std::mutex> lock(mutex);
  modules.erase(handle);
}

void ModuleManager::AddExport(s32 handle, const std::string &name, u64 address,
                              u32 nid) {
  std::lock_guard<std::mutex> lock(mutex);
  auto it = modules.find(handle);
  if (it != modules.end()) {
    it->second.exports[name] = {name, address, nid};
  }
}

u64 ModuleManager::ResolveSymbol(const std::string &moduleName,
                                 const std::string &symbolName) {
  std::lock_guard<std::mutex> lock(mutex);

  for (auto &[handle, mod] : modules) {
    if (mod.name == moduleName) {
      auto it = mod.exports.find(symbolName);
      if (it != mod.exports.end()) {
        return it->second.address;
      }
    }
  }

  return 0;
}

u64 ModuleManager::ResolveSymbolByNid(const std::string &moduleName, u32 nid) {
  std::lock_guard<std::mutex> lock(mutex);

  for (auto &[handle, mod] : modules) {
    if (mod.name == moduleName) {
      for (const auto &[name, sym] : mod.exports) {
        if (sym.nid == nid) {
          return sym.address;
        }
      }
    }
  }

  return 0;
}

std::vector<ModuleInfo> ModuleManager::GetLoadedModules() {
  std::lock_guard<std::mutex> lock(mutex);
  std::vector<ModuleInfo> list;
  for (const auto &[handle, mod] : modules) {
    list.push_back(mod);
  }
  return list;
}

ModuleInfo *ModuleManager::GetModule(s32 handle) {
  auto it = modules.find(handle);
  return (it != modules.end()) ? &it->second : nullptr;
}

ModuleInfo *ModuleManager::GetModuleByName(const std::string &name) {
  for (auto &[handle, mod] : modules) {
    if (mod.name == name) {
      return &mod;
    }
  }
  return nullptr;
}

} // namespace Kernel
} // namespace Core
