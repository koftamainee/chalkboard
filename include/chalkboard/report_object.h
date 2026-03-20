#pragma once

#include <string>

class IReportObject {
public:
  virtual std::string to_html() const = 0;
  virtual ~IReportObject() = default;
};