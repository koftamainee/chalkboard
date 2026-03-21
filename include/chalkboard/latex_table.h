#pragma once

#include <string>
#include <vector>

#include <chalkboard/latex_serializable.h>
#include <chalkboard/report_object.h>

namespace chalkboard {
  class LatexTable : public IReportObject {
  public:
    explicit LatexTable(std::vector<std::string> headers);

    template<CellRenderable... Args>
    LatexTable& row(Args&&... cols) {
      std::vector<std::string> r;
      (r.push_back(render_cell(std::forward<Args>(cols))), ...);
      m_rows.push_back(std::move(r));
      return *this;
    }

    [[nodiscard]] std::string to_html() const override;

  private:
    std::vector<std::string>              m_headers;
    std::vector<std::vector<std::string>> m_rows;
  };
}