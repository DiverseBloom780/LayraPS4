// SPDX-FileCopyrightText: Copyright 2025 LayraPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

// Common Error Codes
constexpr s32 ORBIS_OK = 0;
constexpr s32 ORBIS_ERROR_NET_Base = 0x80410000;
#define ORBISNETERRORBASE ORBIS_ERROR_NET_Base

// Kernel Error Codes (if not in kernel.h)
constexpr s32 ORBIS_KERNEL_ERROR_EAGAIN = 0x8002000B;
