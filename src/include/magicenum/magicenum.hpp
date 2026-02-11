#pragma once
#include <optional>
#include <string_view>


namespace magic_enum {
template <typename E> constexpr std::string_view enum_name(E value) {
  return "Unknown";
}

template <typename E>
constexpr std::optional<E> enum_cast(std::string_view value) {
  return std::nullopt;
}
} // namespace magic_enum
