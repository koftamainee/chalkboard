#pragma once

#include <format>
#include <string>
#include <type_traits>

#include <chalkboard/latex_serializable.h>

template <chalkboard::LatexSerializable T>
  requires (!std::same_as<T, char> &&
    !std::same_as<T, const char*> &&
    !std::same_as<T, const void*>)
struct std::formatter<T> {
  constexpr auto parse(format_parse_context& ctx) {
    return ctx.begin();
  }

  auto format(const T& obj, auto& ctx) const {
    return std::format_to(ctx.out(), "\\({}\\)", to_latex(obj));
  }
}; // namespace std