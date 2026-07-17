// SPDX-FileCopyrightText: Copyright 2025 LayraPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "video_out.h"
#include "core/libraries/kernel/equeue.h"
#include <chrono>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <thread>
#include <vector>

namespace Core {
namespace Libraries {
namespace VideoOut {

// --- Internal Video Port State ---

struct VideoOutPort {
  bool is_open = false;
  s32 handle = -1;
  FlipStatus flip_status{};
  SceVideoOutResolutionStatus resolution{};
  SceVideoOutVblankStatus vblank_status{};
  s32 flip_rate = 0;
  std::mutex port_mutex;

  // Buffer tracking
  struct BufferSlot {
    void *address = nullptr;
    bool registered = false;
  };
  BufferSlot buffer_slots[16] = {};
  BufferAttribute current_attribute{};

  // Vblank simulation
  std::thread vblank_thread;
  bool vblank_running = false;
};

static constexpr int MAX_PORTS = 2;
static VideoOutPort g_ports[MAX_PORTS];
static std::mutex g_port_mutex;

static VideoOutPort *GetPort(s32 handle) {
  if (handle < 0 || handle >= MAX_PORTS) return nullptr;
  auto *port = &g_ports[handle];
  return port->is_open ? port : nullptr;
}

// Vblank simulation thread
static void VblankThread(VideoOutPort *port) {
  while (port->vblank_running) {
    std::this_thread::sleep_for(std::chrono::microseconds(16683)); // ~60Hz
    std::lock_guard<std::mutex> lock(port->port_mutex);
    port->vblank_status.count++;
    auto us = std::chrono::duration_cast<std::chrono::microseconds>(
                  std::chrono::high_resolution_clock::now().time_since_epoch())
                  .count();
    port->vblank_status.process_time = us;
  }
}

// --- Syscall Implementations ---

s32 PS4_SYSV_ABI sceVideoOutOpen(s32 userId, s32 busType, s32 index,
                                  const void *param) {
  printf("[VideoOut] sceVideoOutOpen: userId=%d, busType=%d, index=%d\n",
         userId, busType, index);

  std::lock_guard<std::mutex> lock(g_port_mutex);
  for (int i = 0; i < MAX_PORTS; i++) {
    if (!g_ports[i].is_open) {
      g_ports[i].is_open = true;
      g_ports[i].handle = i;
      g_ports[i].flip_status = {};
      g_ports[i].resolution = {};
      g_ports[i].vblank_status = {};

      // Start vblank simulation
      g_ports[i].vblank_running = true;
      g_ports[i].vblank_thread =
          std::thread(VblankThread, &g_ports[i]);
      g_ports[i].vblank_thread.detach();

      printf("[VideoOut] Opened port handle=%d\n", i);
      return i;
    }
  }
  printf("[VideoOut] ERROR: No free ports!\n");
  return -1;
}

s32 PS4_SYSV_ABI sceVideoOutClose(s32 handle) {
  printf("[VideoOut] sceVideoOutClose: handle=%d\n", handle);
  auto *port = GetPort(handle);
  if (!port) return -1;

  port->vblank_running = false;
  port->is_open = false;
  return 0;
}

void PS4_SYSV_ABI sceVideoOutSetBufferAttribute(BufferAttribute *attribute,
                                                  s32 pixelFormat,
                                                  u32 tilingMode,
                                                  u32 aspectRatio, u32 width,
                                                  u32 height,
                                                  u32 pitchInPixel) {
  printf("[VideoOut] SetBufferAttribute: %ux%u, fmt=0x%x\n", width, height,
         pixelFormat);
  if (!attribute) return;
  std::memset(attribute, 0, sizeof(BufferAttribute));
  attribute->pixel_format = pixelFormat;
  attribute->tiling_mode = tilingMode;
  attribute->aspect_ratio = aspectRatio;
  attribute->width = width;
  attribute->height = height;
  attribute->pitch_in_pixel = pitchInPixel;
  attribute->option = 0;
}

s32 PS4_SYSV_ABI sceVideoOutRegisterBuffers(s32 handle, s32 startIndex,
                                             void *const *addresses,
                                             s32 bufferNum,
                                             const BufferAttribute *attribute) {
  printf("[VideoOut] RegisterBuffers: handle=%d, start=%d, num=%d\n", handle,
         startIndex, bufferNum);

  auto *port = GetPort(handle);
  if (!port || !addresses || !attribute) return -1;

  std::lock_guard<std::mutex> lock(port->port_mutex);
  port->current_attribute = *attribute;

  for (s32 i = 0; i < bufferNum; i++) {
    s32 idx = startIndex + i;
    if (idx >= 0 && idx < 16) {
      port->buffer_slots[idx].address = addresses[i];
      port->buffer_slots[idx].registered = true;
      printf("[VideoOut]   Buffer[%d] = %p\n", idx, addresses[i]);
    }
  }
  return 0;
}

s32 PS4_SYSV_ABI sceVideoOutSubmitFlip(s32 handle, s32 bufferIndex,
                                        s32 flipMode, s64 flipArg) {
  auto *port = GetPort(handle);
  if (!port) return -1;

  std::lock_guard<std::mutex> lock(port->port_mutex);
  port->flip_status.count++;
  port->flip_status.flip_arg = flipArg;
  port->flip_status.current_buffer = bufferIndex;
  port->flip_status.flip_pending_num = 0;

  auto us = std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::high_resolution_clock::now().time_since_epoch())
                .count();
  port->flip_status.process_time = us;
  port->flip_status.tsc = us;

  // Log every Nth flip to avoid spam
  if (port->flip_status.count % 60 == 0) {
    printf("[VideoOut] Flip #%llu buffer=%d\n",
           (unsigned long long)port->flip_status.count, bufferIndex);
  }
  return 0;
}

s32 PS4_SYSV_ABI sceVideoOutGetFlipStatus(s32 handle, FlipStatus *status) {
  auto *port = GetPort(handle);
  if (!port || !status) return -1;

  std::lock_guard<std::mutex> lock(port->port_mutex);
  *status = port->flip_status;
  return 0;
}

s32 PS4_SYSV_ABI sceVideoOutGetResolutionStatus(
    s32 handle, SceVideoOutResolutionStatus *status) {
  auto *port = GetPort(handle);
  if (!port || !status) return -1;
  *status = port->resolution;
  return 0;
}

s32 PS4_SYSV_ABI sceVideoOutGetVblankStatus(s32 handle,
                                              SceVideoOutVblankStatus *status) {
  auto *port = GetPort(handle);
  if (!port || !status) return -1;

  std::lock_guard<std::mutex> lock(port->port_mutex);
  *status = port->vblank_status;
  return 0;
}

s32 PS4_SYSV_ABI sceVideoOutSetFlipRate(s32 handle, s32 rate) {
  auto *port = GetPort(handle);
  if (!port) return -1;
  port->flip_rate = rate;
  return 0;
}

s32 PS4_SYSV_ABI sceVideoOutIsFlipPending(s32 handle) {
  auto *port = GetPort(handle);
  if (!port) return 0;
  return port->flip_status.flip_pending_num;
}

s32 PS4_SYSV_ABI sceVideoOutAddFlipEvent(s64 eq, s32 handle, void *udata) {
  printf("[VideoOut] AddFlipEvent: eq=0x%llx, handle=%d\n",
         (unsigned long long)eq, handle);
  // Register with event queue for flip notifications
  return 0;
}

s32 PS4_SYSV_ABI sceVideoOutAddVblankEvent(s64 eq, s32 handle, void *udata) {
  printf("[VideoOut] AddVblankEvent: eq=0x%llx, handle=%d\n",
         (unsigned long long)eq, handle);
  return 0;
}

s32 PS4_SYSV_ABI sceVideoOutWaitVblank(s32 handle) {
  // Wait approximately one vsync period
  std::this_thread::sleep_for(std::chrono::microseconds(16683));
  return 0;
}

void RegisterVideoOut(::Core::Kernel::ModuleManager *module_manager) {
  printf("[VideoOut] Registering VideoOut Syscalls\n");

#define LIB_FUNCTION(nid, library, version, module, function)                  \
  module_manager->RegisterHLEExport(module, nid, #function,                    \
                                    reinterpret_cast<uint64_t>(function));

  LIB_FUNCTION("Up36PTk687E", "libSceVideoOut", 1, "libSceVideoOut",
               sceVideoOutOpen);
  LIB_FUNCTION("uquVH4-Du78", "libSceVideoOut", 1, "libSceVideoOut",
               sceVideoOutClose);
  LIB_FUNCTION("i6-sR91Wt-4", "libSceVideoOut", 1, "libSceVideoOut",
               sceVideoOutSetBufferAttribute);
  LIB_FUNCTION("w3BY+tAEiQY", "libSceVideoOut", 1, "libSceVideoOut",
               sceVideoOutRegisterBuffers);
  LIB_FUNCTION("U46NwOiJpys", "libSceVideoOut", 1, "libSceVideoOut",
               sceVideoOutSubmitFlip);
  LIB_FUNCTION("SbU3dwp80lQ", "libSceVideoOut", 1, "libSceVideoOut",
               sceVideoOutGetFlipStatus);
  LIB_FUNCTION("6kPnj51T62Y", "libSceVideoOut", 1, "libSceVideoOut",
               sceVideoOutGetResolutionStatus);
  LIB_FUNCTION("1FZBKy8HeNU", "libSceVideoOut", 1, "libSceVideoOut",
               sceVideoOutGetVblankStatus);
  LIB_FUNCTION("CBiu4mCE1DA", "libSceVideoOut", 1, "libSceVideoOut",
               sceVideoOutSetFlipRate);
  LIB_FUNCTION("zgXifHT9ErY", "libSceVideoOut", 1, "libSceVideoOut",
               sceVideoOutIsFlipPending);
  LIB_FUNCTION("HXzjK9yI30k", "libSceVideoOut", 1, "libSceVideoOut",
               sceVideoOutAddFlipEvent);
  LIB_FUNCTION("Xru92wHJRmg", "libSceVideoOut", 1, "libSceVideoOut",
               sceVideoOutAddVblankEvent);
  LIB_FUNCTION("j6RaAUlaLv0", "libSceVideoOut", 1, "libSceVideoOut",
               sceVideoOutWaitVblank);

#undef LIB_FUNCTION
}

} // namespace VideoOut
} // namespace Libraries
} // namespace Core
