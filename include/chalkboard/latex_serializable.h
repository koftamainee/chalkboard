#pragma once

#include <string>

namespace chalkboard {
  template <typename T>
  concept LatexSerializable = requires(const T& obj) {
    { obj.to_latex() } -> std::convertible_to<std::string>;
  };
}
