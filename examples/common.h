#pragma once

#include <string>
#include <format>

inline std::string to_latex(int v) { return std::to_string(v); }
inline std::string to_latex(unsigned int v) { return std::to_string(v); }
inline std::string to_latex(long v) { return std::to_string(v); }
inline std::string to_latex(unsigned long v) { return std::to_string(v); }
inline std::string to_latex(long long v) { return std::to_string(v); }
inline std::string to_latex(unsigned long long v) { return std::to_string(v); }
inline std::string to_latex(float v) { return std::format("{:.3}", v); }
inline std::string to_latex(double v) { return std::format("{:.3}", v); }
inline std::string to_latex(long double v) { return std::format("{:.3}", v); }
inline std::string to_latex(bool v) {
  return v ? "\\text{true}" : "\\text{false}";
}