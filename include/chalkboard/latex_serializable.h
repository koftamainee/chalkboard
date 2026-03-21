#pragma once

#include <string>

namespace chalkboard {
  template<typename T>
  concept LatexSerializable = requires(const T& obj) {
    { to_latex(obj) } -> std::convertible_to<std::string>;
  };

  template<typename T>
  concept CellRenderable = LatexSerializable<T> || std::convertible_to<T, std::string>;

  template<CellRenderable T>
  std::string render_cell(const T& v) {
    if constexpr (LatexSerializable<T>) {
      return to_latex(v);
    } else {
      return std::string(v);
    }
  }
}