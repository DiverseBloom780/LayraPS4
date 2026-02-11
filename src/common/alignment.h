#pragma once
#include <cstddef>
#include <cstdint>

namespace Common {

template <typename T> constexpr T AlignUp(T value, size_t alignment) {
  return (T)(((size_t)value + alignment - 1) & ~(alignment - 1));
}

template <typename T> constexpr T AlignDown(T value, size_t alignment) {
  return (T)((size_t)value & ~(alignment - 1));
}

template <typename T> constexpr bool IsAligned(T value, size_t alignment) {
  return ((size_t)value & (alignment - 1)) == 0;
}

} // namespace Common
