// SPDX-FileCopyrightText: Copyright 2025 LayraPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later
// Original implementation. Games call sceSysmoduleLoadModule to load
// PRX libraries; we HLE everything so just return success.

#include "sysmodule.h"
#include <cstdio>

namespace Core {
namespace Libraries {
namespace SysModule {

// Module IDs from public PS4 SDK docs
enum OrbisModuleId : u16 {
  ORBIS_SYSMODULE_FIBER = 0x0006,
  ORBIS_SYSMODULE_NET = 0x0009,
  ORBIS_SYSMODULE_HTTP = 0x000A,
  ORBIS_SYSMODULE_SSL = 0x000B,
  ORBIS_SYSMODULE_NP_COMMON = 0x000C,
  ORBIS_SYSMODULE_NP_TROPHY = 0x000E,
  ORBIS_SYSMODULE_NP_AUTH = 0x000F,
  ORBIS_SYSMODULE_SAVE_DATA = 0x0010,
  ORBIS_SYSMODULE_IME_DIALOG = 0x0016,
  ORBIS_SYSMODULE_AUDIO_OUT = 0x001B,
  ORBIS_SYSMODULE_PAD = 0x0024,
  ORBIS_SYSMODULE_FONT = 0x0033,
  ORBIS_SYSMODULE_FONT_FT = 0x0034,
  ORBIS_SYSMODULE_APP_CONTENT = 0x0042,
  ORBIS_SYSMODULE_NP_MANAGER = 0x0043,
  ORBIS_SYSMODULE_SHARE_PLAY = 0x008C,
  ORBIS_SYSMODULE_JSON = 0x0080,
  ORBIS_SYSMODULE_JSON2 = 0x00A4,
  ORBIS_SYSMODULE_RTCLOG = 0x0097,
};

static const char *GetModuleName(u16 id) {
  switch (id) {
  case ORBIS_SYSMODULE_FIBER: return "Fiber";
  case ORBIS_SYSMODULE_NET: return "Net";
  case ORBIS_SYSMODULE_HTTP: return "HTTP";
  case ORBIS_SYSMODULE_SSL: return "SSL";
  case ORBIS_SYSMODULE_NP_COMMON: return "NpCommon";
  case ORBIS_SYSMODULE_NP_TROPHY: return "NpTrophy";
  case ORBIS_SYSMODULE_NP_AUTH: return "NpAuth";
  case ORBIS_SYSMODULE_SAVE_DATA: return "SaveData";
  case ORBIS_SYSMODULE_IME_DIALOG: return "ImeDialog";
  case ORBIS_SYSMODULE_AUDIO_OUT: return "AudioOut";
  case ORBIS_SYSMODULE_PAD: return "Pad";
  case ORBIS_SYSMODULE_FONT: return "Font";
  case ORBIS_SYSMODULE_FONT_FT: return "FontFt";
  case ORBIS_SYSMODULE_APP_CONTENT: return "AppContent";
  case ORBIS_SYSMODULE_NP_MANAGER: return "NpManager";
  case ORBIS_SYSMODULE_JSON: return "Json";
  case ORBIS_SYSMODULE_JSON2: return "Json2";
  default: return "Unknown";
  }
}

static s32 PS4_SYSV_ABI sceSysmoduleLoadModule(u16 moduleId) {
  printf("[SysModule] LoadModule: 0x%04X (%s)\n", moduleId,
         GetModuleName(moduleId));
  return 0; // All modules are "loaded" via HLE
}

static s32 PS4_SYSV_ABI sceSysmoduleUnloadModule(u16 moduleId) {
  printf("[SysModule] UnloadModule: 0x%04X (%s)\n", moduleId,
         GetModuleName(moduleId));
  return 0;
}

static s32 PS4_SYSV_ABI sceSysmoduleIsLoaded(u16 moduleId) {
  return 0; // Always loaded
}

static s32 PS4_SYSV_ABI sceSysmoduleLoadModuleInternal(u32 moduleId) {
  printf("[SysModule] LoadModuleInternal: 0x%08X\n", moduleId);
  return 0;
}

static s32 PS4_SYSV_ABI sceSysmoduleLoadModuleByNameInternal(
    const char *name, u64 arg2, u64 arg3, u64 arg4, u64 arg5, u64 arg6) {
  printf("[SysModule] LoadModuleByNameInternal: %s\n",
         name ? name : "null");
  return 0;
}

static s32 PS4_SYSV_ABI sceSysmoduleIsLoadedInternal(u32 moduleId) {
  return 0;
}

void RegisterSysModule(::Core::Kernel::ModuleManager *module_manager) {
  printf("[SysModule] Registering SysModule\n");

#define LIB_FUNCTION(nid, library, version, module, function)                  \
  module_manager->RegisterHLEExport(module, nid, #function,                    \
                                    reinterpret_cast<uint64_t>(function));

  LIB_FUNCTION("g8cM39EUZ6o", "libSceSysmodule", 1, "libSceSysmodule",
               sceSysmoduleLoadModule);
  LIB_FUNCTION("MFWQ-MRoCnE", "libSceSysmodule", 1, "libSceSysmodule",
               sceSysmoduleUnloadModule);
  LIB_FUNCTION("H4nMOFPMhCE", "libSceSysmodule", 1, "libSceSysmodule",
               sceSysmoduleIsLoaded);
  LIB_FUNCTION("39iENBFzheQ", "libSceSysmodule", 1, "libSceSysmodule",
               sceSysmoduleLoadModuleInternal);
  LIB_FUNCTION("CY-IUDjjfnA", "libSceSysmodule", 1, "libSceSysmodule",
               sceSysmoduleLoadModuleByNameInternal);
  LIB_FUNCTION("aHR5L2rrJBQ", "libSceSysmodule", 1, "libSceSysmodule",
               sceSysmoduleIsLoadedInternal);

#undef LIB_FUNCTION
}

} // namespace SysModule
} // namespace Libraries
} // namespace Core
