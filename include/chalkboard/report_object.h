#pragma once

#include <string>

namespace chalkboard {
  class IReportObject {
  public:
    [[nodiscard]] virtual std::string to_html() const = 0;
    virtual ~IReportObject() = default;
  };
}