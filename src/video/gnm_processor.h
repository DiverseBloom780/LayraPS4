// src/video/gnm_processor.h
// SPDX-FileCopyrightText: Copyright 2025 LayraPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later
//
// PM4 command buffer parser for PS4's GCN GPU.
// Register offsets sourced from AMD's public "Sea Islands Register Reference".

#pragma once

#include "common/types.h"
#include "vulkan_backend.h"
#include <map>
#include <memory>
#include <vector>

namespace Video {

// PM4 Packet types (bits [31:30] of header)
enum class PacketType : u8 {
  Type0 = 0, // Write to consecutive registers
  Type1 = 1, // Reserved
  Type2 = 2, // NOP filler
  Type3 = 3  // Opcode-based commands
};

// Type 3 Opcodes — from AMD GCN PM4 specification
enum class Type3Opcode : u8 {
  NOP                    = 0x10,
  SET_BASE               = 0x11,
  INDEX_BUFFER_SIZE      = 0x0A,
  DRAW_INDEX_AUTO        = 0x2D,
  DRAW_INDEX_2           = 0x27,
  DRAW_INDEX_OFFSET_2    = 0x35,
  INDEX_TYPE             = 0x2A,
  NUM_INSTANCES          = 0x2F,
  DISPATCH_DIRECT        = 0x15,
  EVENT_WRITE            = 0x46,
  EVENT_WRITE_EOP        = 0x47,
  EVENT_WRITE_EOS        = 0x48,
  RELEASE_MEM            = 0x49,
  WAIT_REG_MEM           = 0x3C,
  WRITE_DATA             = 0x37,
  ACQUIRE_MEM            = 0x58,
  SET_CONFIG_REG         = 0x68,
  SET_CONTEXT_REG        = 0x69,
  SET_SH_REG             = 0x76,
  SET_UCONFIG_REG        = 0x79,
  INDIRECT_BUFFER        = 0x3F,
  COND_EXEC              = 0x22,
  DMA_DATA               = 0x50,
};

// GCN context register offsets (relative to 0xA000 base)
// From AMD Sea Islands register map
namespace CtxReg {
  constexpr u32 DB_RENDER_CONTROL      = 0x000;
  constexpr u32 PA_SC_VPORT_SCISSOR_TL = 0x094;
  constexpr u32 PA_SC_VPORT_SCISSOR_BR = 0x095;
  constexpr u32 VGT_PRIMITIVE_TYPE     = 0x242;
  constexpr u32 CB_COLOR0_BASE         = 0x318;
  constexpr u32 CB_COLOR0_INFO         = 0x31C;
  constexpr u32 CB_COLOR0_ATTRIB       = 0x31D;
  constexpr u32 CB_COLOR0_PITCH        = 0x319;
  constexpr u32 CB_COLOR0_SLICE        = 0x31A;
  constexpr u32 CB_COLOR0_FMASK        = 0x31E;
  constexpr u32 PA_CL_VPORT_XSCALE_0   = 0x10F;
  constexpr u32 PA_CL_VPORT_YSCALE_0   = 0x111;
}

// GCN SH register offsets (relative to 0x2C00 base)
namespace ShReg {
  constexpr u32 SPI_SHADER_PGM_LO_VS  = 0x048;
  constexpr u32 SPI_SHADER_PGM_HI_VS  = 0x049;
  constexpr u32 SPI_SHADER_PGM_LO_PS  = 0x008;
  constexpr u32 SPI_SHADER_PGM_HI_PS  = 0x009;
  constexpr u32 SPI_SHADER_PGM_LO_ES  = 0x0C8;
  constexpr u32 SPI_SHADER_PGM_HI_ES  = 0x0C9;
  constexpr u32 SPI_SHADER_PGM_LO_GS  = 0x088;
  constexpr u32 SPI_SHADER_PGM_HI_GS  = 0x089;
}

// Statistics for diagnostics
struct ProcessorStats {
  u64 packets_parsed = 0;
  u64 draws_dispatched = 0;
  u64 regs_written = 0;
  u64 nop_packets = 0;
  u64 unknown_opcodes = 0;
};

class GnmProcessor {
public:
  GnmProcessor();
  ~GnmProcessor();

  void ProcessCommandBuffer(u32 *cmdbuf, u32 sizeInBytes);

  void SetVulkanContext(void *device, void *physDevice, void *gfxQueue,
                        u32 queueFamily, void *renderPass, u32 width,
                        u32 height);

  // Read current GPU state for external consumers
  const ProcessorStats &GetStats() const { return stats_; }

  // Read a specific register value
  u32 ReadContextReg(u32 offset) const;
  u32 ReadShReg(u32 offset) const;

  // Execute pending draws into a host command buffer (called during
  // the main render pass so emulator output appears on screen)
  void FlushToCommandBuffer(VkCommandBuffer cmd);

private:
  void HandleType3(u32 header, u32 *payload, u16 count);

  // Dispatch handlers
  void HandleDrawIndexAuto(u32 *payload);
  void HandleDrawIndex2(u32 *payload, u16 count);
  void HandleEventWriteEop(u32 *payload, u16 count);
  void HandleWaitRegMem(u32 *payload, u16 count);
  void HandleDmaData(u32 *payload, u16 count);

  // Build GuestPipelineState from current register values
  GuestPipelineState BuildPipelineState() const;

  // Register banks
  std::map<u32, u32> config_regs_;
  std::map<u32, u32> context_regs_;
  std::map<u32, u32> sh_regs_;
  std::map<u32, u32> uconfig_regs_;

  // Draw state
  u32 index_type_ = 0;      // VGT_DMA_INDEX_TYPE value
  u32 num_instances_ = 1;   // Number of instances for instanced drawing
  u32 primitive_type_ = 4;  // Default: triangle list

  ProcessorStats stats_{};
  std::unique_ptr<VulkanBackend> vulkan_backend_;
};

} // namespace Video
