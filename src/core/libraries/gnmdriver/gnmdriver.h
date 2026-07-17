// SPDX-FileCopyrightText: Copyright 2025 LayraPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"
#include "core/kernel/module_manager.h"

typedef struct VkCommandBuffer_T *VkCommandBuffer;

namespace Core {
namespace Libraries {
namespace GnmDriver {

void RegisterGnmDriver(::Core::Kernel::ModuleManager *module_manager);

void SetVulkanContext(void* device, void* physDevice, void* gfxQueue,
                      uint32_t queueFamily, void* renderPass,
                      uint32_t width, uint32_t height);

// Called from the main render loop to let VulkanBackend issue draws
// into the active command buffer during the render pass.
void RenderEmulatorFrame(VkCommandBuffer cmd);

} // namespace GnmDriver
} // namespace Libraries
} // namespace Core

