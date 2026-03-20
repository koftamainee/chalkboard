#pragma once

#include <format>
#include <string>
#include <vector>

#include <chalkboard/latex_serializable.h>
#include <chalkboard/report_object.h>

template <LatexSerializable T>
struct std::formatter<T> {
  constexpr auto parse(auto& ctx) { return ctx.begin(); }
  auto format(const T& obj, auto& ctx) const {
    return std::format_to(ctx.out(), "\\({}\\)", obj.to_latex());
  }
};

class LatexList : public IReportObject {
public:
  enum class Kind { Itemize, Enumerate };

  static LatexList itemize();
  static LatexList enumerate();

  template <typename... Args>
  LatexList& item(std::format_string<Args...> fmt, Args&&... args) {
    m_items.push_back(std::format(fmt, std::forward<Args>(args)...));
    return *this;
  }

  std::string to_html() const override;

private:
  explicit LatexList(Kind kind);

  Kind m_kind;
  std::vector<std::string> m_items;
};