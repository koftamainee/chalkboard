#pragma once

#include <string>

namespace chalkboard {
  template<typename T>
  concept LatexSerializable = requires(const T& obj) {
    { to_latex(obj) } -> std::convertible_to<std::string>;
  };
}
