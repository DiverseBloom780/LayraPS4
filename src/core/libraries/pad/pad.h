// SPDX-FileCopyrightText: Copyright 2025 LayraPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later
// Original implementation for controller input emulation.

#pragma once

#include "common/types.h"
#include "core/kernel/module_manager.h"

namespace Core {
namespace Libraries {
namespace Pad {

// Pad button bit flags (public PS4 SDK)
constexpr u32 ORBIS_PAD_BUTTON_L3 = 0x00000002;
constexpr u32 ORBIS_PAD_BUTTON_R3 = 0x00000004;
constexpr u32 ORBIS_PAD_BUTTON_OPTIONS = 0x00000008;
constexpr u32 ORBIS_PAD_BUTTON_UP = 0x00000010;
constexpr u32 ORBIS_PAD_BUTTON_RIGHT = 0x00000020;
constexpr u32 ORBIS_PAD_BUTTON_DOWN = 0x00000040;
constexpr u32 ORBIS_PAD_BUTTON_LEFT = 0x00000080;
constexpr u32 ORBIS_PAD_BUTTON_L2 = 0x00000100;
constexpr u32 ORBIS_PAD_BUTTON_R2 = 0x00000200;
constexpr u32 ORBIS_PAD_BUTTON_L1 = 0x00000400;
constexpr u32 ORBIS_PAD_BUTTON_R1 = 0x00000800;
constexpr u32 ORBIS_PAD_BUTTON_TRIANGLE = 0x00001000;
constexpr u32 ORBIS_PAD_BUTTON_CIRCLE = 0x00002000;
constexpr u32 ORBIS_PAD_BUTTON_CROSS = 0x00004000;
constexpr u32 ORBIS_PAD_BUTTON_SQUARE = 0x00008000;
constexpr u32 ORBIS_PAD_BUTTON_TOUCH_PAD = 0x00100000;

struct OrbisPadAnalogStick {
  u8 x;
  u8 y;
};

struct OrbisPadAnalogButtons {
  u8 l2;
  u8 r2;
  u8 padding[2];
};

struct OrbisPadTouch {
  u16 x;
  u16 y;
  u8 id;
  u8 padding[3];
};

struct OrbisPadTouchData {
  u8 touchNum;
  u8 padding[3];
  u32 reserved;
  OrbisPadTouch touch[2];
};

struct OrbisPadVec3f {
  float x, y, z;
};

struct OrbisPadVec4f {
  float x, y, z, w;
};

struct OrbisPadData {
  u32 buttons;
  OrbisPadAnalogStick leftStick;
  OrbisPadAnalogStick rightStick;
  OrbisPadAnalogButtons analogButtons;
  OrbisPadVec4f orientation;
  OrbisPadVec3f acceleration;
  OrbisPadVec3f angularVelocity;
  OrbisPadTouchData touchData;
  bool connected;
  u64 timestamp;
  u32 connectedCount;
  u8 padding[12];
};

void RegisterPad(::Core::Kernel::ModuleManager *module_manager);

} // namespace Pad
} // namespace Libraries
} // namespace Core
