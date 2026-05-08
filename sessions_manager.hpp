#ifndef SERIALIPMI_SESSIONS_MANAGER_HPP
#define SERIALIPMI_SESSIONS_MANAGER_HPP

#include <memory>
#include <phosphor-logging/lg2.hpp>
#include <unordered_map>
#include <chrono>
#include <string>
#include <cstdint>
#include <random>

namespace serialipmi {
namespace session {
  class Session;
  class Manager;
}
}
namespace serialipmi { namespace message { class Message; } }

namespace serialipmi {
namespace session {

class Session {
public:
  uint32_t id;
  bool isActive{false};
  std::chrono::steady_clock::time_point lastActive;
  Session(uint32_t sid) : id(sid), lastActive(std::chrono::steady_clock::now()) {}
};

class Manager {
public:
  static Manager& get();

  void managerInit(const std::string& channelName);

  uint32_t createSession();

  bool activateSession(uint32_t sessionId);

  bool closeSession(uint32_t sessionId);

  std::shared_ptr<Session> getSession(uint32_t sessionId);

  void checkTimeouts(int timeoutSeconds);

  size_t getActiveSessionCount() const;

private:
  Manager() = default;
  std::unordered_map<uint32_t, std::shared_ptr<Session>> sessions_;
  std::mt19937 rng{std::random_device{}()};
  size_t maxSessions_{1};
  std::string channelName_;
};

} // namespace session
} // namespace serialipmi

#endif // SERIALIPMI_SESSIONS_MANAGER_HPP
