#include <chalkboard/publisher.h>

#include <httplib.h>
#include <stdexcept>
#include <string>

namespace chalkboard {
  Publisher::Publisher(std::string host, const int port)
    : m_host(std::move(host)), m_port(port) {}

  void Publisher::publish(const std::string& artifact_dir) const {
    httplib::Client client(m_host, m_port);
    client.set_connection_timeout(2);
    client.set_read_timeout(2);

    auto res = client.Post("/publish", artifact_dir, "text/plain");
    if (res == nullptr) {
      throw std::runtime_error(
        "Publisher::publish: cannot connect to viewer at "
        + m_host + ":" + std::to_string(m_port)
      );
    }
    if (res->status != 200) {
      throw std::runtime_error(
        "Publisher::publish: viewer returned status "
        + std::to_string(res->status)
      );
    }
  }
}
