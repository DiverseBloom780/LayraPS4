// SPDX-FileCopyrightText: Copyright 2025 LayraPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/loader/symbols.h"
#include "sysv_abi_wrapper.h"
#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace Core::Kernel {

struct SymbolInfo {
  uint64_t address;
};

struct ModuleInfo {
  uint32_t handle;
  std::string name;
  uint64_t baseAddress;
  std::map<std::string, SymbolInfo> exports;
};

class ModuleManager {
public:
  ModuleManager();
  ~ModuleManager();

  std::vector<ModuleInfo> GetLoadedModules() const;
  uintptr_t ResolveSymbol(const std::string &module, const std::string &symbol);

  // Registers a loaded module
  uint32_t RegisterModule(const std::string &name, uint64_t baseAddress,
                          uint64_t size, uint64_t entryPoint);

  // Registers an HLE module function
  void RegisterHLEExport(const std::string &moduleName, const std::string &nid,
                         const std::string &name, uint64_t hostAddress);

  // Resolves an HLE symbol by NID
  uint64_t ResolveHLEExport(const std::string &moduleName,
                            const std::string &nid);

private:
  struct ModulePrivate;
  std::vector<ModuleInfo> loaded_modules;
  // Map of Module Name -> List of HLE Exports
  std::map<std::string, std::vector<Core::Loader::HLEExport>> hle_modules;
  AbiWrapperManager abi_wrapper_;
};

} // namespace Core::Kernel
