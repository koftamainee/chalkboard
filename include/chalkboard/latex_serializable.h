#pragma once

#include <string>

template <typename T>
concept LatexSerializable = requires(const T& obj) {
  { obj.to_latex() } -> std::convertible_to<std::string>;
};
