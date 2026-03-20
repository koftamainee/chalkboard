#include <chalkboard/task.h>

#include <iostream>
#include <stdexcept>

Task::Task(const std::string& task_name)
    : m_reporter(std::make_unique<Reporter>(task_name)),
      m_publisher(std::make_unique<Publisher>("localhost", 8080)) {}

void Task::build_and_publish(const std::function<void(Reporter&)>& callback) const {
  try {
    callback(*m_reporter);
    std::string artifact_dir = m_reporter->build();
    std::cout << "Report built at: " << artifact_dir << "\n";
    m_publisher->publish(artifact_dir);
    std::cout << "Published to http://localhost:8080\n";
  } catch (const std::exception& e) {
    std::cerr << "error: " << e.what() << "\n";
  }
}