// SPDX-FileCopyrightText: Copyright 2025 LayraPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "gnmdriver.h"
#include "video/gnm_processor.h"
#include <cstdio>
#include <memory>

namespace Core {
namespace Libraries {
namespace GnmDriver {

// --- GnmDriver HLE Implementation ---
// These functions intercept PS4 GPU calls and forward command buffers
// to our GnmProcessor for parsing and eventual Vulkan translation.

static std::unique_ptr<Video::GnmProcessor> s_gnm_processor;

static Video::GnmProcessor& GetProcessor() {
  if (!s_gnm_processor) {
    s_gnm_processor = std::make_unique<Video::GnmProcessor>();
  }
  return *s_gnm_processor;
}

void SetVulkanContext(void* device, void* physDevice, void* gfxQueue, uint32_t queueFamily, void* renderPass, uint32_t width, uint32_t height) {
  auto& proc = GetProcessor();
  proc.SetVulkanContext(device, physDevice, gfxQueue, queueFamily, renderPass, width, height);
}

void RenderEmulatorFrame(VkCommandBuffer cmd) {
  if (!s_gnm_processor) return;
  // Let the VulkanBackend execute any pending draws into the host command buffer
  auto& proc = GetProcessor();
  const auto& stats = proc.GetStats();
  if (stats.draws_dispatched > 0) {
    proc.FlushToCommandBuffer(cmd);
  }
}


static s32 PS4_SYSV_ABI sceGnmSubmitCommandBuffers(u32 count,
                                                     void **dcbGpuAddrs,
                                                     u32 *dcbSizesInBytes,
                                                     void **ccbGpuAddrs,
                                                     u32 *ccbSizesInBytes) {
  static u64 submit_count = 0;
  if (submit_count++ % 120 == 0) {
    printf("[GnmDriver] sceGnmSubmitCommandBuffers: count=%u (total: %llu)\n",
           count, (unsigned long long)submit_count);
  }

  // Feed each DCB into our PM4 parser
  auto& proc = GetProcessor();
  for (u32 i = 0; i < count; i++) {
    if (dcbGpuAddrs && dcbGpuAddrs[i] && dcbSizesInBytes) {
      proc.ProcessCommandBuffer(
          reinterpret_cast<u32*>(dcbGpuAddrs[i]),
          dcbSizesInBytes[i]);
    }
  }
  return 0;
}

static s32 PS4_SYSV_ABI sceGnmSubmitAndFlipCommandBuffers(
    u32 count, void **dcbGpuAddrs, u32 *dcbSizesInBytes, void **ccbGpuAddrs,
    u32 *ccbSizesInBytes, s32 videoOutHandle, s32 bufferIndex, s32 flipMode,
    s64 flipArg) {
  static u64 flip_count = 0;
  if (flip_count++ % 60 == 0) {
    printf("[GnmDriver] SubmitAndFlip: buffer=%d (frame: %llu)\n", bufferIndex,
           (unsigned long long)flip_count);
  }

  // Parse command buffers then signal flip
  auto& proc = GetProcessor();
  for (u32 i = 0; i < count; i++) {
    if (dcbGpuAddrs && dcbGpuAddrs[i] && dcbSizesInBytes) {
      proc.ProcessCommandBuffer(
          reinterpret_cast<u32*>(dcbGpuAddrs[i]),
          dcbSizesInBytes[i]);
    }
  }
  return 0;
}

static void PS4_SYSV_ABI sceGnmSubmitDone() {
  // Signal that all submissions for this frame are complete
}

static u32 PS4_SYSV_ABI sceGnmDrawInitDefaultHardwareState(u32 *cmdbuf,
                                                            u32 size) {
  printf("[GnmDriver] DrawInitDefaultHardwareState: size=%u\n", size);
  if (size < 0x100) return 0;
  // Fill with NOPs
  for (u32 i = 0; i < size; i++) {
    cmdbuf[i] = 0xC0001000; // NOP packet
  }
  return size;
}

static u32 PS4_SYSV_ABI sceGnmDrawInitDefaultHardwareState175(u32 *cmdbuf,
                                                                u32 size) {
  return sceGnmDrawInitDefaultHardwareState(cmdbuf, size);
}

static u32 PS4_SYSV_ABI sceGnmDrawInitDefaultHardwareState200(u32 *cmdbuf,
                                                                u32 size) {
  return sceGnmDrawInitDefaultHardwareState(cmdbuf, size);
}

static u32 PS4_SYSV_ABI sceGnmDrawInitDefaultHardwareState350(u32 *cmdbuf,
                                                                u32 size) {
  return sceGnmDrawInitDefaultHardwareState(cmdbuf, size);
}

static u32 PS4_SYSV_ABI sceGnmDispatchInitDefaultHardwareState(u32 *cmdbuf,
                                                                 u32 size) {
  if (size < 0x100) return 0;
  for (u32 i = 0; i < size; i++) {
    cmdbuf[i] = 0xC0001000;
  }
  return size;
}

static s32 PS4_SYSV_ABI sceGnmDrawIndexAuto(u32 *cmdbuf, u32 size,
                                              u32 index_count, u32 flags) {
  return 0;
}

static s32 PS4_SYSV_ABI sceGnmDrawIndex(u32 *cmdbuf, u32 size,
                                          u32 index_count, uintptr_t index_addr,
                                          u32 flags, u32 type) {
  return 0;
}

static void PS4_SYSV_ABI sceGnmSetVsShader(u32 *cmdbuf, u32 size,
                                             const void *vs_regs,
                                             u32 shader_modifier) {}

static void PS4_SYSV_ABI sceGnmSetPsShader(u32 *cmdbuf, u32 size,
                                             const void *ps_regs) {}

static void PS4_SYSV_ABI sceGnmSetPsShader350(u32 *cmdbuf, u32 size,
                                                const void *ps_regs) {}

static s32 PS4_SYSV_ABI sceGnmUpdateVsShader(u32 *cmdbuf, u32 size,
                                               const void *vs_regs,
                                               u32 shader_modifier) {
  return 0;
}

static s32 PS4_SYSV_ABI sceGnmUpdatePsShader(u32 *cmdbuf, u32 size,
                                               const void *ps_regs) {
  return 0;
}

static s32 PS4_SYSV_ABI sceGnmUpdatePsShader350(u32 *cmdbuf, u32 size,
                                                  const void *ps_regs) {
  return 0;
}

static void PS4_SYSV_ABI sceGnmDingDong(u32 gnm_vqid, u32 next_offs_dw) {}

static s32 PS4_SYSV_ABI sceGnmMapComputeQueue(u32 pipe_id, u32 queue_id,
                                                void *ring_addr,
                                                u32 ring_size_dw,
                                                void *rptr_addr) {
  printf("[GnmDriver] MapComputeQueue: pipe=%u, queue=%u\n", pipe_id, queue_id);
  return 1; // gnm_vqid
}

static void PS4_SYSV_ABI sceGnmUnmapComputeQueue(u32 gnm_vqid) {}

static s32 PS4_SYSV_ABI sceGnmAreSubmitsAllowed() { return 1; }

static u64 PS4_SYSV_ABI sceGnmGetGpuCoreClockFrequency() {
  return 800'000'000ULL; // 800 MHz
}

static s32 PS4_SYSV_ABI sceGnmGetEqTimeStamp() { return 0; }

static s32 PS4_SYSV_ABI sceGnmAddEqEvent(s64 eq, u64 id, void *udata) {
  return 0;
}

static s32 PS4_SYSV_ABI sceGnmDeleteEqEvent(s64 eq, u64 id) { return 0; }

static u32 PS4_SYSV_ABI sceGnmDrawInitToDefaultContextState(u32 *cmdbuf,
                                                              u32 size) {
  if (size == 0) return 0;
  for (u32 i = 0; i < size; i++) {
    cmdbuf[i] = 0xC0001000;
  }
  return size;
}

static u32 PS4_SYSV_ABI sceGnmDrawInitToDefaultContextState400(u32 *cmdbuf,
                                                                  u32 size) {
  return sceGnmDrawInitToDefaultContextState(cmdbuf, size);
}

static s32 PS4_SYSV_ABI sceGnmInsertWaitFlipDone(u32 *cmdbuf, u32 size,
                                                    s32 videoOutHandle,
                                                    s32 bufferIndex) {
  return 0;
}

static s32 PS4_SYSV_ABI sceGnmRegisterOwner(void *handle,
                                              const char *name) {
  printf("[GnmDriver] RegisterOwner: %s\n", name ? name : "unknown");
  return 0;
}

static s32 PS4_SYSV_ABI sceGnmRegisterResource(void *ownerHandle,
                                                  void *memory, u64 size,
                                                  u32 type, void *userData) {
  return 0;
}

static s32 PS4_SYSV_ABI sceGnmGetTheTessellationFactorRingBufferBaseAddress(
    u64 *addr) {
  static u64 fake_addr = 0x80000000ULL;
  if (addr) *addr = fake_addr;
  return 0;
}

// Generic stub for unimplemented functions
static s32 PS4_SYSV_ABI gnm_stub() { return 0; }

void RegisterGnmDriver(::Core::Kernel::ModuleManager *module_manager) {
  printf("[GnmDriver] Registering GnmDriver Syscalls\n");

#define LIB_FUNCTION(nid, library, version, module, function)                  \
  module_manager->RegisterHLEExport(module, nid, #function,                    \
                                    reinterpret_cast<uint64_t>(function));

  // Core submit/flip
  LIB_FUNCTION("zwY0YV91TTI", "libSceGnmDriver", 1, "libSceGnmDriver",
               sceGnmSubmitCommandBuffers);
  LIB_FUNCTION("xbxNatawohc", "libSceGnmDriver", 1, "libSceGnmDriver",
               sceGnmSubmitAndFlipCommandBuffers);
  LIB_FUNCTION("yvZ73uQUqrk", "libSceGnmDriver", 1, "libSceGnmDriver",
               sceGnmSubmitDone);

  // HW init
  LIB_FUNCTION("gxcr+JzKu+g", "libSceGnmDriver", 1, "libSceGnmDriver",
               sceGnmDrawInitDefaultHardwareState);
  LIB_FUNCTION("mLIFhjMKOBI", "libSceGnmDriver", 1, "libSceGnmDriver",
               sceGnmDrawInitDefaultHardwareState175);
  LIB_FUNCTION("HlTPoZ-oY7Y", "libSceGnmDriver", 1, "libSceGnmDriver",
               sceGnmDrawInitDefaultHardwareState200);
  LIB_FUNCTION("QhnyReteJ+M", "libSceGnmDriver", 1, "libSceGnmDriver",
               sceGnmDrawInitDefaultHardwareState350);
  LIB_FUNCTION("jg33rEKLfVs", "libSceGnmDriver", 1, "libSceGnmDriver",
               sceGnmDispatchInitDefaultHardwareState);

  // Draw
  LIB_FUNCTION("GGsn7jMTxw4", "libSceGnmDriver", 1, "libSceGnmDriver",
               sceGnmDrawIndexAuto);
  LIB_FUNCTION("ED7ylFjGEJo", "libSceGnmDriver", 1, "libSceGnmDriver",
               sceGnmDrawIndex);

  // Shader setup
  LIB_FUNCTION("V31V01UiScY", "libSceGnmDriver", 1, "libSceGnmDriver",
               sceGnmUpdateVsShader);
  LIB_FUNCTION("4MgRw-bVNQU", "libSceGnmDriver", 1, "libSceGnmDriver",
               sceGnmUpdatePsShader);
  LIB_FUNCTION("mLVL7N7BVBg", "libSceGnmDriver", 1, "libSceGnmDriver",
               sceGnmUpdatePsShader350);

  // Compute queue
  LIB_FUNCTION("bX5IbRvECXk", "libSceGnmDriver", 1, "libSceGnmDriver",
               sceGnmMapComputeQueue);
  LIB_FUNCTION("ArSg-TGinhk", "libSceGnmDriver", 1, "libSceGnmDriver",
               sceGnmUnmapComputeQueue);
  LIB_FUNCTION("SfWMD1xBbQY", "libSceGnmDriver", 1, "libSceGnmDriver",
               sceGnmDingDong);

  // Misc
  LIB_FUNCTION("b08AgtPlHPg", "libSceGnmDriver", 1, "libSceGnmDriver",
               sceGnmAreSubmitsAllowed);
  LIB_FUNCTION("Fwvh++m9IQI", "libSceGnmGetGpuCoreClockFrequency", 1,
               "libSceGnmDriver", sceGnmGetGpuCoreClockFrequency);

  // Event queue
  LIB_FUNCTION("iBt3Oe00Kvc", "libSceGnmDriver", 1, "libSceGnmDriver",
               sceGnmAddEqEvent);
  LIB_FUNCTION("Kx-h-nWMJ8A", "libSceGnmDriver", 1, "libSceGnmDriver",
               sceGnmDeleteEqEvent);

  // Context init
  LIB_FUNCTION("nF6bFRUBRAo", "libSceGnmDriver", 1, "libSceGnmDriver",
               sceGnmDrawInitToDefaultContextState);
  LIB_FUNCTION("yFVnQAGYz18", "libSceGnmDriver", 1, "libSceGnmDriver",
               sceGnmDrawInitToDefaultContextState400);
  LIB_FUNCTION("QFb0LjilIbc", "libSceGnmDriver", 1, "libSceGnmDriver",
               sceGnmInsertWaitFlipDone);

  // Resource tracking
  LIB_FUNCTION("t-vEFz13kNk", "libSceGnmDriver", 1, "libSceGnmDriver",
               sceGnmRegisterOwner);
  LIB_FUNCTION("gAhM3k80+y0", "libSceGnmDriver", 1, "libSceGnmDriver",
               sceGnmRegisterResource);

  // Tessellation
  LIB_FUNCTION("jg33rEKLfVs", "libSceGnmDriver", 1, "libSceGnmDriver",
               sceGnmGetTheTessellationFactorRingBufferBaseAddress);

#undef LIB_FUNCTION
}

} // namespace GnmDriver
} // namespace Libraries
} // namespace Core
