// SPDX-FileCopyrightText: Copyright 2025 LayraPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later
// Original implementation for application content management.

#include "app_content.h"
#include <cstdio>
#include <cstring>

namespace Core {
namespace Libraries {
namespace AppContent {

static s32 PS4_SYSV_ABI sceAppContentInitialize(const void* initParam, void* bootParam) {
    printf("[AppContent] sceAppContentInitialize\n");
    return 0;
}

static s32 PS4_SYSV_ABI sceAppContentAppParamGetInt(u32 paramId, s32* value) {
    printf("[AppContent] sceAppContentAppParamGetInt: paramId=%u\n", paramId);
    if (value) *value = 0;
    return 0;
}

static s32 PS4_SYSV_ABI sceAppContentTemporaryDataMount2(u32 option, void* mountPoint) {
    printf("[AppContent] sceAppContentTemporaryDataMount2\n");
    if (mountPoint) {
        strncpy((char*)mountPoint, "/temp0", 6);
        ((char*)mountPoint)[6] = '\0';
    }
    return 0;
}

static s32 PS4_SYSV_ABI sceAppContentGetAddcontInfoList(u32 serviceLabel, void* list, u32 listNum, u32* hitNum) {
    printf("[AppContent] sceAppContentGetAddcontInfoList\n");
    if (hitNum) *hitNum = 0;
    return 0;
}

static s32 PS4_SYSV_ABI dummy() {
    return 0;
}

void RegisterAppContent(::Core::Kernel::ModuleManager *module_manager) {
    printf("[AppContent] Registering AppContent\n");

#define LIB_FUNCTION(nid, library, version, module, function)                  \
  module_manager->RegisterHLEExport(module, nid, #function,                    \
                                    reinterpret_cast<uint64_t>(function));

    LIB_FUNCTION("R9lA82OraNs", "libSceAppContent", 1, "libSceAppContentUtil", sceAppContentInitialize);
    LIB_FUNCTION("99b82IKXpH4", "libSceAppContent", 1, "libSceAppContentUtil", sceAppContentAppParamGetInt);
    LIB_FUNCTION("buYbeLOGWmA", "libSceAppContent", 1, "libSceAppContentUtil", sceAppContentTemporaryDataMount2);
    LIB_FUNCTION("xnd8BJzAxmk", "libSceAppContent", 1, "libSceAppContentUtil", sceAppContentGetAddcontInfoList);
    LIB_FUNCTION("AS45QoYHjc4", "libSceAppContent", 1, "libSceAppContentUtil", dummy);

#undef LIB_FUNCTION
}

} // namespace AppContent
} // namespace Libraries
} // namespace Core
