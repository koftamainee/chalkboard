#pragma once

#include <string>
#include <format>

namespace chalkboard {
  template <typename T>
  concept LatexSerializable = requires(const T& obj) {
    { to_latex(obj) } -> std::convertible_to<std::string>;
  };

  template <typename T>
  auto maybe_latex(T&& value) {
    if constexpr (chalkboard::LatexSerializable<std::decay_t<T>>) {
      return std::format("\\({}\\)", to_latex(value));
    }
    else {
      return std::forward<T>(value);
    }
  }
}
