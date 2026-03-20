#pragma once

#include <format>
#include <string>

#include <chalkboard/latex_serializable.h>


  template <chalkboard::LatexSerializable T>
  struct std::formatter<T> {
    constexpr auto parse(auto& ctx) { return ctx.begin(); }

    auto format(const T& obj, auto& ctx) const {
      return std::format_to(ctx.out(), "\\({}\\)", obj.to_latex());
    }
  };