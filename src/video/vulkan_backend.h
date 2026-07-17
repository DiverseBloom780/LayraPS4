// src/video/vulkan_backend.h
// SPDX-FileCopyrightText: Copyright 2025 LayraPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Layra's own GPU abstraction layer. Consumes parsed Gnm register
// state from GnmProcessor and issues real Vulkan draw commands.

#pragma once

#include "common/types.h"
#include <map>
#include <vector>
#include <vulkan/vulkan.h>

namespace Video {

// Mirrors a subset of the GCN pipeline state that we track
struct GuestPipelineState {
  // Vertex shader registers (SPI_SHADER_PGM_LO_VS / HI)
  u64 vs_program_addr = 0;
  // Pixel shader registers  (SPI_SHADER_PGM_LO_PS / HI)
  u64 ps_program_addr = 0;

  // Primitive topology inferred from VGT_PRIMITIVE_TYPE
  u32 primitive_type = 0; // 0 = points, 4 = trilist, 5 = tristrip ...

  // Viewport scissor (PA_SC_VPORT_SCISSOR_0_TL / BR)
  u32 viewport_x = 0;
  u32 viewport_y = 0;
  u32 viewport_w = 1920;
  u32 viewport_h = 1080;

  // Render target (CB_COLOR0_BASE, CB_COLOR0_INFO)
  u64 color_target_addr = 0;
  u32 color_format = 0;
};

// Tracks a single guest-registered buffer (textures, vertex data, etc.)
struct GpuResource {
  u64 guest_addr;
  u64 size;
  VkBuffer vk_buffer;
  VkDeviceMemory vk_memory;
  bool dirty;
};

class VulkanBackend {
public:
  VulkanBackend();
  ~VulkanBackend();

  // One-time setup using host Vulkan context
  bool Initialize(VkDevice device, VkPhysicalDevice physDevice,
                   VkQueue gfxQueue, u32 queueFamily,
                   VkRenderPass renderPass, VkExtent2D extent);

  void Shutdown();

  // Called by GnmProcessor after it has finished parsing a command buffer
  void CommitState(const GuestPipelineState &state);

  // Execute the pending draw
  void ExecuteDraw(VkCommandBuffer cmd, u32 vertexCount);

  // Clear the current render target
  void ExecuteClear(VkCommandBuffer cmd, float r, float g, float b, float a);

private:
  VkDevice device_ = VK_NULL_HANDLE;
  VkPhysicalDevice physDevice_ = VK_NULL_HANDLE;
  VkQueue gfxQueue_ = VK_NULL_HANDLE;
  u32 queueFamily_ = 0;
  VkRenderPass renderPass_ = VK_NULL_HANDLE;
  VkExtent2D extent_{};

  // Fallback pipeline that draws solid colour (used until shader
  // recompiler is online)
  VkPipeline fallbackPipeline_ = VK_NULL_HANDLE;
  VkPipelineLayout fallbackLayout_ = VK_NULL_HANDLE;

  bool CreateFallbackPipeline();

  // Current committed state
  GuestPipelineState committed_{};
  bool stateValid_ = false;
};

} // namespace Video
