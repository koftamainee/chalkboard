#pragma once

#include <functional>
#include <memory>
#include <string>

#include <chalkboard/publisher.h>
#include <chalkboard/reporter.h>

class Task {
public:
  explicit Task(const std::string& task_name);

  void build_and_publish(const std::function<void(Reporter&)>& callback) const;

private:
  std::unique_ptr<Reporter> m_reporter;
  std::unique_ptr<Publisher> m_publisher;
};