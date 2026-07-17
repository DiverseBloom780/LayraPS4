// SPDX-FileCopyrightText: Copyright 2025 LayraPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later
// Original implementation. Provides system-level service stubs
// that PS4 games query during initialization.

#include "systemservice.h"
#include <cstdio>
#include <cstring>

namespace Core {
namespace Libraries {
namespace SystemService {

// --- LayraPS4 original implementations ---

static s32 PS4_SYSV_ABI sceSystemServiceGetStatus(s32 *status) {
  if (!status)
    return -1;
  *status = 0; // Normal operation
  return 0;
}

static s32 PS4_SYSV_ABI sceSystemServiceGetAppIdOfRunningBigApp() {
  return 0; // This app is the big app
}

static s32 PS4_SYSV_ABI sceSystemServiceGetAppType(s32 appId, s32 *appType) {
  if (!appType)
    return -1;
  *appType = 4; // Game application
  return 0;
}

static s32 PS4_SYSV_ABI sceSystemServiceGetDisplaySafeAreaInfo(
    void *safeAreaInfo) {
  if (!safeAreaInfo)
    return -1;
  // Safe area is full screen: ratio = 1.0f
  struct SafeAreaInfo {
    float ratio;
  };
  auto *info = static_cast<SafeAreaInfo *>(safeAreaInfo);
  info->ratio = 1.0f;
  return 0;
}

static s32 PS4_SYSV_ABI sceSystemServiceParamGetInt(s32 paramId,
                                                      s32 *value) {
  if (!value)
    return -1;
  switch (paramId) {
  case 1: // LANGUAGE
    *value = 1; // English (US)
    break;
  case 2: // DATE_FORMAT
    *value = 0; // YYYY/MM/DD
    break;
  case 3: // TIME_FORMAT
    *value = 0; // 12-hour
    break;
  case 4: // TIME_ZONE
    *value = 0;
    break;
  case 5: // SUMMERTIME (DST)
    *value = 1;
    break;
  case 6: // SYSTEM_VERSION
    *value = 0x05050000; // 5.05
    break;
  case 7: // GAME_PARENTAL_LEVEL
    *value = 11; // No restriction
    break;
  case 1000: // ENTER_BUTTON_ASSIGN
    *value = 0; // Cross = confirm
    break;
  default:
    printf("[SystemService] ParamGetInt: unknown paramId=%d\n", paramId);
    *value = 0;
    break;
  }
  return 0;
}

static s32 PS4_SYSV_ABI sceSystemServiceParamGetString(s32 paramId,
                                                         char *buf,
                                                         u64 bufSize) {
  if (!buf || bufSize == 0)
    return -1;
  // Only known string param is title id
  strncpy(buf, "", bufSize - 1);
  buf[bufSize - 1] = '\0';
  return 0;
}

static s32 PS4_SYSV_ABI sceSystemServiceHideSplashScreen() {
  printf("[SystemService] HideSplashScreen\n");
  return 0;
}

static s32 PS4_SYSV_ABI sceSystemServiceLoadExec(const char *path,
                                                   const char *const *argv) {
  printf("[SystemService] LoadExec: %s\n", path ? path : "null");
  return 0;
}

static s32 PS4_SYSV_ABI sceSystemServiceReceiveEvent(void *event) {
  // No system events pending
  return -1;
}

static s32 PS4_SYSV_ABI sceSystemServiceGetAppContentInitParam(void *param) {
  if (!param)
    return -1;
  memset(param, 0, 512); // Zero out the param struct
  return 0;
}

static s32 PS4_SYSV_ABI sceSystemServiceDisableMusicPlayer() { return 0; }

static s32 PS4_SYSV_ABI sceSystemServiceReenableMusicPlayer() { return 0; }

static s32 PS4_SYSV_ABI sceSystemServiceIsAppSuspended(s32 appId,
                                                         s32 *suspended) {
  if (suspended)
    *suspended = 0;
  return 0;
}

static s32 PS4_SYSV_ABI sceSystemServiceGetParentSocket(s32 status) {
  return -1; // No parent socket
}

static s32 PS4_SYSV_ABI sceSystemServiceGetParentSocketForPlatformPrivacy(
    s32 *fd) {
  if (fd)
    *fd = -1;
  return 0;
}

// Generic stub for unimplemented system service calls
static s32 PS4_SYSV_ABI systemservice_stub() { return 0; }

void RegisterSystemService(::Core::Kernel::ModuleManager *module_manager) {
  printf("[SystemService] Registering SystemService\n");

#define LIB_FUNCTION(nid, library, version, module, function)                  \
  module_manager->RegisterHLEExport(module, nid, #function,                    \
                                    reinterpret_cast<uint64_t>(function));

  // System params — almost every game calls these
  LIB_FUNCTION("Vo5V8KAwCmk", "libSceSystemService", 1,
               "libSceSystemService", sceSystemServiceParamGetInt);
  LIB_FUNCTION("bIG2TERI9C0", "libSceSystemService", 1,
               "libSceSystemService", sceSystemServiceParamGetString);

  // App lifecycle
  LIB_FUNCTION("J7dYaoXGEQk", "libSceSystemService", 1,
               "libSceSystemService", sceSystemServiceHideSplashScreen);
  LIB_FUNCTION("UfG05wfEoHs", "libSceSystemService", 1,
               "libSceSystemService", sceSystemServiceReceiveEvent);
  LIB_FUNCTION("7NeytMAliBs", "libSceSystemService", 1,
               "libSceSystemService", sceSystemServiceGetStatus);
  LIB_FUNCTION("5k1ECwddMRA", "libSceSystemService", 1,
               "libSceSystemService", sceSystemServiceLoadExec);

  // Display
  LIB_FUNCTION("WzLbJVpEYQs", "libSceSystemService", 1,
               "libSceSystemService", sceSystemServiceGetDisplaySafeAreaInfo);

  // App content
  LIB_FUNCTION("QHG3GJaGejI", "libSceSystemService", 1,
               "libSceSystemService", sceSystemServiceGetAppContentInitParam);

  // Music
  LIB_FUNCTION("FI+VqGdttvI", "libSceSystemService", 1,
               "libSceSystemService", sceSystemServiceDisableMusicPlayer);
  LIB_FUNCTION("ec72vt3WEQo", "libSceSystemService", 1,
               "libSceSystemService", sceSystemServiceReenableMusicPlayer);

#undef LIB_FUNCTION
}

} // namespace SystemService
} // namespace Libraries
} // namespace Core
