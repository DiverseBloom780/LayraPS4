// SPDX-FileCopyrightText: Copyright 2025 LayraPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <cstddef>
#include <type_traits>

namespace Common {

template <typename T>
constexpr T AlignUp(T value, size_t alignment) {
    return static_cast<T>((static_cast<uintptr_t>(value) + alignment - 1) & ~(alignment - 1));
}

template <typename T>
constexpr T AlignDown(T value, size_t alignment) {
    return static_cast<T>(static_cast<uintptr_t>(value) & ~(alignment - 1));
}

template <typename T>
constexpr bool IsAligned(T value, size_t alignment) {
    return (static_cast<uintptr_t>(value) & (alignment - 1)) == 0;
}

template <typename T>
constexpr size_t AlignOffset(T value, size_t alignment) {
    return alignment - (static_cast<uintptr_t>(value) & (alignment - 1));
}

} // namespace Common

#define ALIGN_UP(value, alignment) Common::AlignUp(value, alignment)
#define ALIGN_DOWN(value, alignment) Common::AlignDown(value, alignment)
#define IS_ALIGNED(value, alignment) Common::IsAligned(value, alignment)