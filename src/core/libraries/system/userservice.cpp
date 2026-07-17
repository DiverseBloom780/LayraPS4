// SPDX-FileCopyrightText: Copyright 2025 LayraPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later
// Original implementation. NID values from public firmware analysis.

#include "userservice.h"
#include <cstdio>
#include <cstring>

namespace Core {
namespace Libraries {
namespace UserService {

// LayraPS4 emulates a single local user with ID 1
static constexpr s32 kDefaultUserId = 1;
static bool g_initialized = false;

// --- Original function implementations ---

static s32 PS4_SYSV_ABI sceUserServiceInitialize(
    const OrbisUserServiceInitializeParams *params) {
  printf("[UserService] Initialize (priority=%d)\n",
         params ? params->priority : 0);
  g_initialized = true;
  return 0;
}

static s32 PS4_SYSV_ABI sceUserServiceInitialize2() {
  printf("[UserService] Initialize2\n");
  g_initialized = true;
  return 0;
}

static s32 PS4_SYSV_ABI sceUserServiceTerminate() {
  printf("[UserService] Terminate\n");
  g_initialized = false;
  return 0;
}

static s32 PS4_SYSV_ABI sceUserServiceGetInitialUser(s32 *userId) {
  if (!userId)
    return -1;
  *userId = kDefaultUserId;
  return 0;
}

static s32 PS4_SYSV_ABI sceUserServiceGetLoginUserIdList(
    OrbisUserServiceLoginUserIdList *list) {
  if (!list)
    return -1;
  // Slot 0 = our user, rest invalid
  list->user_id[0] = kDefaultUserId;
  for (int i = 1; i < ORBIS_USER_SERVICE_MAX_LOGIN_USERS; i++)
    list->user_id[i] = ORBIS_USER_SERVICE_USER_ID_INVALID;
  return 0;
}

static s32 PS4_SYSV_ABI sceUserServiceGetUserName(s32 userId, char *name,
                                                    u64 nameSize) {
  if (!name || nameSize == 0)
    return -1;
  const char *defaultName = "LayraUser";
  strncpy(name, defaultName, nameSize - 1);
  name[nameSize - 1] = '\0';
  return 0;
}

static s32 PS4_SYSV_ABI sceUserServiceGetUserColor(s32 userId,
                                                     s32 *color) {
  if (!color)
    return -1;
  *color = 0; // Blue
  return 0;
}

static s32 PS4_SYSV_ABI sceUserServiceGetEvent(void *event) {
  // No pending events
  return -1; // ORBIS_USER_SERVICE_ERROR_NO_EVENT
}

static s32 PS4_SYSV_ABI sceUserServiceGetForegroundUser(s32 *userId) {
  if (!userId)
    return -1;
  *userId = kDefaultUserId;
  return 0;
}

static s32 PS4_SYSV_ABI sceUserServiceGetNpAccountId(s32 userId,
                                                       u64 *accountId) {
  if (!accountId)
    return -1;
  *accountId = 0x0123456789ABCDEFULL; // Fake account ID
  return 0;
}

// Generic no-op stub for the hundreds of get/set functions
static s32 PS4_SYSV_ABI userservice_stub() { return 0; }

void RegisterUserService(::Core::Kernel::ModuleManager *module_manager) {
  printf("[UserService] Registering UserService\n");

#define LIB_FUNCTION(nid, library, version, module, function)                  \
  module_manager->RegisterHLEExport(module, nid, #function,                    \
                                    reinterpret_cast<uint64_t>(function));

  // Core lifecycle
  LIB_FUNCTION("j0YkvJTdBJo", "libSceUserService", 1, "libSceUserService",
               sceUserServiceInitialize);
  LIB_FUNCTION("PC7jLHdFTME", "libSceUserService", 1, "libSceUserService",
               sceUserServiceInitialize2);
  LIB_FUNCTION("nTRMRBT6TAk", "libSceUserService", 1, "libSceUserService",
               sceUserServiceTerminate);

  // User queries
  LIB_FUNCTION("fPhymKNvK-A", "libSceUserService", 1, "libSceUserService",
               sceUserServiceGetInitialUser);
  LIB_FUNCTION("FG-HkkEMjPI", "libSceUserService", 1, "libSceUserService",
               sceUserServiceGetLoginUserIdList);
  LIB_FUNCTION("JieijM3pFv4", "libSceUserService", 1, "libSceUserService",
               sceUserServiceGetUserName);
  LIB_FUNCTION("MU8D+4Jjfpg", "libSceUserService", 1, "libSceUserService",
               sceUserServiceGetUserColor);
  LIB_FUNCTION("HHssHbEiLqI", "libSceUserService", 1, "libSceUserService",
               sceUserServiceGetEvent);
  LIB_FUNCTION("4zRn3TFGNR0", "libSceUserService", 1, "libSceUserService",
               sceUserServiceGetForegroundUser);
  LIB_FUNCTION("JB4YhMIa2gY", "libSceUserService", 1, "libSceUserService",
               sceUserServiceGetNpAccountId);

#undef LIB_FUNCTION
}

} // namespace UserService
} // namespace Libraries
} // namespace Core
