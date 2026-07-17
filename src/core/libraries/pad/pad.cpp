// SPDX-FileCopyrightText: Copyright 2025 LayraPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later
// Original controller input implementation for LayraPS4.

#include "pad.h"
#include <chrono>
#include <cstdio>
#include <cstring>

namespace Core {
namespace Libraries {
namespace Pad {

static constexpr s32 kMaxPadHandles = 4;

struct PadState {
  bool is_open = false;
  s32 userId = -1;
  OrbisPadData current{};
};

static PadState g_pads[kMaxPadHandles];
static bool g_pad_initialized = false;

static void InitDefaultPadData(OrbisPadData *data) {
  memset(data, 0, sizeof(OrbisPadData));
  data->leftStick.x = 128;  // Center
  data->leftStick.y = 128;
  data->rightStick.x = 128;
  data->rightStick.y = 128;
  data->orientation.w = 1.0f;
  data->connected = true;
  data->timestamp = std::chrono::duration_cast<std::chrono::microseconds>(
                        std::chrono::high_resolution_clock::now()
                            .time_since_epoch())
                        .count();
}

static s32 PS4_SYSV_ABI scePadInit() {
  printf("[Pad] scePadInit\n");
  g_pad_initialized = true;
  return 0;
}

static s32 PS4_SYSV_ABI scePadOpen(s32 userId, s32 type, s32 index,
                                    const void *param) {
  printf("[Pad] scePadOpen: userId=%d, type=%d, index=%d\n", userId, type,
         index);
  for (int i = 0; i < kMaxPadHandles; i++) {
    if (!g_pads[i].is_open) {
      g_pads[i].is_open = true;
      g_pads[i].userId = userId;
      InitDefaultPadData(&g_pads[i].current);
      return i; // Handle
    }
  }
  return -1;
}

static s32 PS4_SYSV_ABI scePadClose(s32 handle) {
  if (handle < 0 || handle >= kMaxPadHandles)
    return -1;
  g_pads[handle].is_open = false;
  return 0;
}

static s32 PS4_SYSV_ABI scePadReadState(s32 handle, OrbisPadData *data) {
  if (!data || handle < 0 || handle >= kMaxPadHandles)
    return -1;
  if (!g_pads[handle].is_open)
    return -1;

  // Return current pad state with updated timestamp
  *data = g_pads[handle].current;
  data->timestamp = std::chrono::duration_cast<std::chrono::microseconds>(
                        std::chrono::high_resolution_clock::now()
                            .time_since_epoch())
                        .count();
  return 0;
}

static s32 PS4_SYSV_ABI scePadRead(s32 handle, OrbisPadData *data,
                                     s32 num) {
  if (!data || num < 1)
    return -1;
  // Fill the first entry, set rest to same
  s32 ret = scePadReadState(handle, &data[0]);
  if (ret < 0)
    return ret;
  for (s32 i = 1; i < num; i++)
    data[i] = data[0];
  return num;
}

static s32 PS4_SYSV_ABI scePadGetHandle(s32 userId, s32 type, s32 index) {
  for (int i = 0; i < kMaxPadHandles; i++) {
    if (g_pads[i].is_open && g_pads[i].userId == userId)
      return i;
  }
  return -1;
}

static s32 PS4_SYSV_ABI scePadSetMotionSensorState(s32 handle,
                                                      s32 enable) {
  return 0;
}

static s32 PS4_SYSV_ABI scePadSetLightBar(s32 handle, const void *param) {
  return 0;
}

static s32 PS4_SYSV_ABI scePadResetLightBar(s32 handle) { return 0; }

static s32 PS4_SYSV_ABI scePadSetVibration(s32 handle, const void *param) {
  return 0;
}

static s32 PS4_SYSV_ABI scePadGetControllerInformation(s32 handle,
                                                         void *info) {
  if (!info)
    return -1;
  // Minimal controller info: connected, standard controller
  struct PadControllerInfo {
    u8 touchPadInfo_resolution_x[2]; // u16
    u8 touchPadInfo_resolution_y[2]; // u16
    u8 colorInfo[4];
    u8 connectionType;
    u8 connectedCount;
    u8 connected;
    u8 deviceClass;
  };
  memset(info, 0, 64); // Zero out
  auto *ci = static_cast<PadControllerInfo *>(info);
  ci->connected = 1;
  ci->connectedCount = 1;
  ci->connectionType = 0; // Standard
  return 0;
}

void RegisterPad(::Core::Kernel::ModuleManager *module_manager) {
  printf("[Pad] Registering Pad\n");

#define LIB_FUNCTION(nid, library, version, module, function)                  \
  module_manager->RegisterHLEExport(module, nid, #function,                    \
                                    reinterpret_cast<uint64_t>(function));

  LIB_FUNCTION("2JM+4AAFJmo", "libScePad", 1, "libScePad", scePadInit);
  LIB_FUNCTION("7+IEBx3JKmE", "libScePad", 1, "libScePad", scePadOpen);
  LIB_FUNCTION("q1cHNfGycLo", "libScePad", 1, "libScePad", scePadClose);
  LIB_FUNCTION("YndgXqQVV7c", "libScePad", 1, "libScePad", scePadReadState);
  LIB_FUNCTION("fm1r2vv0mz8", "libScePad", 1, "libScePad", scePadRead);
  LIB_FUNCTION("clVvL4ZDntw", "libScePad", 1, "libScePad",
               scePadSetMotionSensorState);
  LIB_FUNCTION("4tpS1bIHvrk", "libScePad", 1, "libScePad",
               scePadSetLightBar);
  LIB_FUNCTION("o+8Hk3MdMtk", "libScePad", 1, "libScePad",
               scePadResetLightBar);
  LIB_FUNCTION("5v-dsCAyJEI", "libScePad", 1, "libScePad",
               scePadSetVibration);
  LIB_FUNCTION("gjP9-KQzoUk", "libScePad", 1, "libScePad",
               scePadGetControllerInformation);
  LIB_FUNCTION("1DmZmgVzlnI", "libScePad", 1, "libScePad",
               scePadGetHandle);

#undef LIB_FUNCTION
}

} // namespace Pad
} // namespace Libraries
} // namespace Core
