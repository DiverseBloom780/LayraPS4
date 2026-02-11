// SPDX-FileCopyrightText: Copyright 2025 LayraPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"
#include <map>
#include <mutex>
#include <string>
#include <vector>

namespace Core {
namespace Kernel {

struct SymbolInfo {
  std::string name;
  u64 address;
  u32 nid; // PS4 Name ID (hash of the name)
};

struct ModuleInfo {
  s32 handle;
  std::string name;
  u64 baseAddress;
  u64 size;
  std::map<std::string, SymbolInfo> exports;
  std::vector<std::string> dependencies;
};

class ModuleManager {
public:
  ModuleManager();
  ~ModuleManager();

  // Register a module after it's been loaded into memory
  s32 RegisterModule(const std::string &name, u64 base, u64 size);

  // Unregister a module
  void UnregisterModule(s32 handle);

  // Add an export to a module
  void AddExport(s32 handle, const std::string &name, u64 address, u32 nid = 0);

  // Resolve a symbol address across all loaded modules
  u64 ResolveSymbol(const std::string &moduleName,
                    const std::string &symbolName);
  u64 ResolveSymbolByNid(const std::string &moduleName, u32 nid);

  // Get module information
  std::vector<ModuleInfo> GetLoadedModules();
  ModuleInfo *GetModule(s32 handle);
  ModuleInfo *GetModuleByName(const std::string &name);

private:
  std::map<s32, ModuleInfo> modules;
  std::mutex mutex;
  s32 nextHandle = 0x2000;

  s32 AllocateHandle();
};

} // namespace Kernel
} // namespace Core
