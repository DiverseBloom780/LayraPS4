// src/video/gnm_processor.cpp
// SPDX-FileCopyrightText: Copyright 2025 LayraPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later
//
// PM4 command buffer parser — extracts GPU register writes and draw
// commands, builds pipeline state, and dispatches to VulkanBackend.

#include "gnm_processor.h"
#include <cstdio>
#include <cstring>

namespace Video {

GnmProcessor::GnmProcessor() {
  printf("[GnmProcessor] Initialized\n");
  vulkan_backend_ = std::make_unique<VulkanBackend>();
}

GnmProcessor::~GnmProcessor() = default;

void GnmProcessor::SetVulkanContext(void *device, void *physDevice,
                                     void *gfxQueue, u32 queueFamily,
                                     void *renderPass, u32 width,
                                     u32 height) {
  if (!device || !physDevice || !gfxQueue || !renderPass) {
    fprintf(stderr, "[GnmProcessor] ERROR: Invalid Vulkan context parameters\n");
    fprintf(stderr, "  device=%p, physDevice=%p, gfxQueue=%p, renderPass=%p\n",
            device, physDevice, gfxQueue, renderPass);
    return;
  }
  
  if (width == 0 || height == 0) {
    fprintf(stderr, "[GnmProcessor] ERROR: Invalid render target dimensions: %ux%u\n",
            width, height);
    return;
  }
  
  if (!vulkan_backend_) {
    fprintf(stderr, "[GnmProcessor] ERROR: VulkanBackend not initialized\n");
    return;
  }
  
  VkExtent2D extent = {width, height};
  if (!vulkan_backend_->Initialize(
      static_cast<VkDevice>(device),
      static_cast<VkPhysicalDevice>(physDevice),
      static_cast<VkQueue>(gfxQueue), queueFamily,
      static_cast<VkRenderPass>(renderPass), extent)) {
    fprintf(stderr, "[GnmProcessor] Failed to initialize VulkanBackend\n");
  }
}

u32 GnmProcessor::ReadContextReg(u32 offset) const {
  auto it = context_regs_.find(offset);
  return it != context_regs_.end() ? it->second : 0;
}

u32 GnmProcessor::ReadShReg(u32 offset) const {
  auto it = sh_regs_.find(offset);
  return it != sh_regs_.end() ? it->second : 0;
}

void GnmProcessor::FlushToCommandBuffer(VkCommandBuffer cmd) {
  if (!vulkan_backend_ || cmd == VK_NULL_HANDLE) return;

  // Build current pipeline state from register shadow
  GuestPipelineState state = BuildPipelineState();
  vulkan_backend_->CommitState(state);

  // Issue draw with the committed state — VulkanBackend will bind the
  // fallback pipeline and set viewport/scissor from the state
  vulkan_backend_->ExecuteDraw(cmd, 3); // fullscreen triangle
}

// ─── PM4 Packet Parser ─────────────────────────────────────────

void GnmProcessor::ProcessCommandBuffer(u32 *cmdbuf, u32 sizeInBytes) {
  if (!cmdbuf || sizeInBytes < 4) return;

  u32 dwordCount = sizeInBytes / 4;
  u32 pos = 0;

  while (pos < dwordCount) {
    u32 header = cmdbuf[pos];
    u8 type = (header >> 30) & 0x3;

    switch (static_cast<PacketType>(type)) {
      case PacketType::Type0: {
        u16 count = ((header >> 16) & 0x3FFF) + 1;
        u32 baseReg = header & 0xFFFF;

        if (pos + 1 + count <= dwordCount) {
          for (u16 i = 0; i < count; i++) {
            context_regs_[baseReg + i] = cmdbuf[pos + 1 + i];
            stats_.regs_written++;
          }
        }
        pos += 1 + count;
        break;
      }

      case PacketType::Type2:
        // NOP filler — skip one dword
        stats_.nop_packets++;
        pos++;
        break;

      case PacketType::Type3: {
        u16 count = (header >> 16) & 0x3FFF;
        if (pos + 1 + count <= dwordCount) {
          HandleType3(header, &cmdbuf[pos + 1], count);
        }
        pos += 2 + count;
        break;
      }

      default:
        pos++;
        break;
    }

    stats_.packets_parsed++;
  }
}

// ─── Type 3 Dispatch ────────────────────────────────────────────

void GnmProcessor::HandleType3(u32 header, u32 *payload, u16 count) {
  u8 opcode = (header >> 8) & 0xFF;

  switch (static_cast<Type3Opcode>(opcode)) {
    case Type3Opcode::NOP:
      stats_.nop_packets++;
      break;

    case Type3Opcode::SET_CONFIG_REG: {
      u32 startReg = payload[0];
      for (u16 i = 0; i < count; i++) {
        config_regs_[startReg + i] = payload[i + 1];
        stats_.regs_written++;
      }
      break;
    }

    case Type3Opcode::SET_CONTEXT_REG: {
      u32 startReg = payload[0];
      for (u16 i = 0; i < count; i++) {
        context_regs_[startReg + i] = payload[i + 1];
        stats_.regs_written++;
      }

      // Track primitive type changes
      if (startReg <= CtxReg::VGT_PRIMITIVE_TYPE &&
          startReg + count > CtxReg::VGT_PRIMITIVE_TYPE) {
        u32 idx = CtxReg::VGT_PRIMITIVE_TYPE - startReg;
        primitive_type_ = payload[idx + 1];
      }
      break;
    }

    case Type3Opcode::SET_SH_REG: {
      u32 startReg = payload[0];
      for (u16 i = 0; i < count; i++) {
        sh_regs_[startReg + i] = payload[i + 1];
        stats_.regs_written++;
      }
      break;
    }

    case Type3Opcode::SET_UCONFIG_REG: {
      u32 startReg = payload[0];
      for (u16 i = 0; i < count; i++) {
        uconfig_regs_[startReg + i] = payload[i + 1];
        stats_.regs_written++;
      }
      break;
    }

    case Type3Opcode::INDEX_TYPE:
      index_type_ = payload[0] & 0x3; // 0=u16, 1=u32
      break;

    case Type3Opcode::NUM_INSTANCES:
      num_instances_ = payload[0];
      if (num_instances_ == 0) num_instances_ = 1;
      break;

    case Type3Opcode::DRAW_INDEX_AUTO:
      HandleDrawIndexAuto(payload);
      break;

    case Type3Opcode::DRAW_INDEX_2:
      HandleDrawIndex2(payload, count);
      break;

    case Type3Opcode::EVENT_WRITE_EOP:
      HandleEventWriteEop(payload, count);
      break;

    case Type3Opcode::WAIT_REG_MEM:
      HandleWaitRegMem(payload, count);
      break;

    case Type3Opcode::DMA_DATA:
      HandleDmaData(payload, count);
      break;

    case Type3Opcode::ACQUIRE_MEM:
      // Memory barrier — we don't need to do anything special yet
      // since we're not doing real async GPU work
      break;

    case Type3Opcode::RELEASE_MEM:
      // Signal completion — similar to EVENT_WRITE_EOP
      break;

    case Type3Opcode::WRITE_DATA:
      // Write data to memory or register — used for labels/fences
      break;

    default:
      stats_.unknown_opcodes++;
      break;
  }
}

// ─── Draw Dispatch ──────────────────────────────────────────────

GuestPipelineState GnmProcessor::BuildPipelineState() const {
  GuestPipelineState state{};

  // Extract VS program address from SH regs
  u32 vsLo = ReadShReg(ShReg::SPI_SHADER_PGM_LO_VS);
  u32 vsHi = ReadShReg(ShReg::SPI_SHADER_PGM_HI_VS);
  state.vs_program_addr = (static_cast<u64>(vsHi) << 32) | (static_cast<u64>(vsLo) << 8);

  // Extract PS program address from SH regs
  u32 psLo = ReadShReg(ShReg::SPI_SHADER_PGM_LO_PS);
  u32 psHi = ReadShReg(ShReg::SPI_SHADER_PGM_HI_PS);
  state.ps_program_addr = (static_cast<u64>(psHi) << 32) | (static_cast<u64>(psLo) << 8);

  // Primitive topology
  state.primitive_type = primitive_type_;

  // Viewport from context regs
  u32 scissorTL = ReadContextReg(CtxReg::PA_SC_VPORT_SCISSOR_TL);
  u32 scissorBR = ReadContextReg(CtxReg::PA_SC_VPORT_SCISSOR_BR);
  state.viewport_x = scissorTL & 0x7FFF;
  state.viewport_y = (scissorTL >> 16) & 0x7FFF;
  state.viewport_w = (scissorBR & 0x7FFF) - state.viewport_x;
  state.viewport_h = ((scissorBR >> 16) & 0x7FFF) - state.viewport_y;
  if (state.viewport_w == 0) state.viewport_w = 1920;
  if (state.viewport_h == 0) state.viewport_h = 1080;

  // Render target
  state.color_target_addr = static_cast<u64>(ReadContextReg(CtxReg::CB_COLOR0_BASE)) << 8;
  state.color_format = ReadContextReg(CtxReg::CB_COLOR0_INFO) & 0x3F;

  return state;
}

void GnmProcessor::HandleDrawIndexAuto(u32 *payload) {
  u32 indexCount = payload[0];

  // Build the full pipeline state from current register values
  GuestPipelineState state = BuildPipelineState();
  vulkan_backend_->CommitState(state);

  // Dispatch the draw — VulkanBackend will handle it
  // (currently logs until shader pipeline is complete)
  vulkan_backend_->ExecuteDraw(VK_NULL_HANDLE, indexCount);
  stats_.draws_dispatched++;
}

void GnmProcessor::HandleDrawIndex2(u32 *payload, u16 count) {
  if (count < 4) return;

  u32 maxSize = payload[0];
  u32 indexBaseLo = payload[1];
  u32 indexBaseHi = payload[2];
  u32 indexCount = payload[3];

  GuestPipelineState state = BuildPipelineState();
  vulkan_backend_->CommitState(state);
  vulkan_backend_->ExecuteDraw(VK_NULL_HANDLE, indexCount);
  stats_.draws_dispatched++;
}

// ─── GPU Sync Commands ──────────────────────────────────────────

void GnmProcessor::HandleEventWriteEop(u32 *payload, u16 count) {
  // EVENT_WRITE_EOP writes a value to memory when the GPU reaches this
  // point in the command stream. Used for frame completion fences.
  if (count < 4) return;

  u32 eventType = payload[0] & 0x3F;
  u64 address = (static_cast<u64>(payload[2]) << 32) | payload[1];
  u32 value = payload[3];

  // Write the completion value to guest memory
  // This signals to the game that the GPU has finished processing
  // For now we complete instantly since we're not doing async GPU work
  if (address != 0) {
    u32 *ptr = reinterpret_cast<u32 *>(address);
    // Only write if the address looks valid (in guest memory range)
    // TODO: validate against memory manager
  }
}

void GnmProcessor::HandleWaitRegMem(u32 *payload, u16 count) {
  // WAIT_REG_MEM: stall command processing until a memory location
  // contains the expected value. Used for GPU-CPU synchronization.
  if (count < 5) return;

  u32 function = payload[0] & 0x7;  // comparison function
  u64 address = (static_cast<u64>(payload[2]) << 32) | payload[1];
  u32 reference = payload[3];
  u32 mask = payload[4];

  // Since we process commands synchronously, the wait is always satisfied
  // immediately. Real async GPU execution would need actual polling here.
}

void GnmProcessor::HandleDmaData(u32 *payload, u16 count) {
  // DMA_DATA: copy data between memory locations or to/from registers.
  // Used for buffer uploads and register initialization.
  if (count < 5) return;

  u32 control = payload[0];
  u64 srcAddr = (static_cast<u64>(payload[2]) << 32) | payload[1];
  u64 dstAddr = (static_cast<u64>(payload[4]) << 32) | payload[3];
  u32 byteCount = payload[5] & 0x1FFFFF;

  // TODO: implement actual DMA copy when memory manager is wired up
}

} // namespace Video
