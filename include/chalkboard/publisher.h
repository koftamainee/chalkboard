#pragma once

#include <string>

class Publisher {
public:
  explicit Publisher(std::string host, int port);

  void publish(const std::string& artifact_dir) const;

private:
  std::string m_host;
  int m_port;
};