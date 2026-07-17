// SPDX-FileCopyrightText: Copyright 2025 LayraPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"
#include "core/kernel/module_manager.h"

namespace Core {
namespace Libraries {
namespace VideoOut {

constexpr s32 SCE_VIDEO_OUT_BUS_TYPE_MAIN = 0;
constexpr s32 SCE_VIDEO_OUT_REFRESH_RATE_59_94HZ = 3;
constexpr s32 SCE_VIDEO_OUT_PIXEL_FORMAT_A8R8G8B8_SRGB = 0x80000000;

struct BufferAttribute {
  s32 pixel_format;
  s32 tiling_mode;
  s32 aspect_ratio;
  u32 width;
  u32 height;
  u32 pitch_in_pixel;
  s32 option;
};

struct FlipStatus {
  u64 count = 0;
  u64 process_time = 0;
  u64 tsc = 0;
  s64 flip_arg = -1;
  u64 submit_tsc = 0;
  u64 reserved0 = 0;
  s32 gc_queue_num = 0;
  s32 flip_pending_num = 0;
  s32 current_buffer = -1;
  u32 reserved1 = 0;
};

struct SceVideoOutResolutionStatus {
  s32 full_width = 1920;
  s32 full_height = 1080;
  s32 pane_width = 1920;
  s32 pane_height = 1080;
  u64 refresh_rate = SCE_VIDEO_OUT_REFRESH_RATE_59_94HZ;
  float screen_size_in_inch = 50;
  u16 flags = 0;
  u16 reserved0 = 0;
  u32 reserved1[3] = {0};
};

struct SceVideoOutVblankStatus {
  u64 count = 0;
  u64 process_time = 0;
  u64 tsc = 0;
  u64 reserved[1] = {0};
  u8 flags = 0;
  u8 pad1[7] = {};
};

// Syscalls
s32 PS4_SYSV_ABI sceVideoOutOpen(s32 userId, s32 busType, s32 index,
                                  const void *param);
s32 PS4_SYSV_ABI sceVideoOutClose(s32 handle);
void PS4_SYSV_ABI sceVideoOutSetBufferAttribute(BufferAttribute *attribute,
                                                  s32 pixelFormat,
                                                  u32 tilingMode,
                                                  u32 aspectRatio, u32 width,
                                                  u32 height, u32 pitchInPixel);
s32 PS4_SYSV_ABI sceVideoOutRegisterBuffers(s32 handle, s32 startIndex,
                                             void *const *addresses,
                                             s32 bufferNum,
                                             const BufferAttribute *attribute);
s32 PS4_SYSV_ABI sceVideoOutSubmitFlip(s32 handle, s32 bufferIndex,
                                        s32 flipMode, s64 flipArg);
s32 PS4_SYSV_ABI sceVideoOutGetFlipStatus(s32 handle, FlipStatus *status);
s32 PS4_SYSV_ABI sceVideoOutGetResolutionStatus(
    s32 handle, SceVideoOutResolutionStatus *status);
s32 PS4_SYSV_ABI sceVideoOutGetVblankStatus(s32 handle,
                                              SceVideoOutVblankStatus *status);
s32 PS4_SYSV_ABI sceVideoOutSetFlipRate(s32 handle, s32 rate);
s32 PS4_SYSV_ABI sceVideoOutIsFlipPending(s32 handle);
s32 PS4_SYSV_ABI sceVideoOutAddFlipEvent(s64 eq, s32 handle, void *udata);
s32 PS4_SYSV_ABI sceVideoOutAddVblankEvent(s64 eq, s32 handle, void *udata);
s32 PS4_SYSV_ABI sceVideoOutWaitVblank(s32 handle);

void RegisterVideoOut(::Core::Kernel::ModuleManager *module_manager);

} // namespace VideoOut
} // namespace Libraries
} // namespace Core
