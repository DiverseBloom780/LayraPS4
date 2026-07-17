// src/core/loader/cpu_patcher.h
// SPDX-FileCopyrightText: Copyright 2025 LayraPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"
#include <cstddef>

namespace Core::Loader {

/**
 * @brief Applies "Lite" instruction patching to guest code.
 *
 * This version uses pattern matching to redirect FS segment accesses
 * (common for TLS/stack canary in PS4 binaries) which conflict with
 * Windows TEB usage.
 */
void ApplyLitePatches(uint8_t *base, size_t size);

} // namespace Core::Loader
