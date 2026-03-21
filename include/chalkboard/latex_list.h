#pragma once

#include <string>
#include <vector>

#include <chalkboard/latex_serializable.h>
#include <chalkboard/report_object.h>

namespace chalkboard {
  class LatexList : public IReportObject {
  public:
    enum class Kind { Itemize, Enumerate };

    static LatexList itemize();
    static LatexList enumerate();

    template<CellRenderable... Args>
    LatexList& item(Args&&... args) {
      std::string result;
      ((result += render_cell(std::forward<Args>(args))), ...);
      m_items.push_back(std::move(result));
      return *this;
    }

    [[nodiscard]] std::string to_html() const override;

  private:
    explicit LatexList(Kind kind);

    Kind                     m_kind;
    std::vector<std::string> m_items;
  };
}