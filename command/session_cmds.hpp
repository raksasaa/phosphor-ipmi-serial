// IPMI session command handlers (local, APP NetFn)
#pragma once

#include <cstdint>
#include <vector>
#include <memory>

namespace serialipmi {
namespace message {
class Handler;
} // namespace message
} // namespace serialipmi

namespace serialipmi {
namespace command {

    // 0x39: Get Session Challenge
    std::vector<uint8_t> getSessionChallenge(const std::vector<uint8_t>& request,
                                             std::shared_ptr<message::Handler>& handler);

    // 0x3A: Activate Session
    std::vector<uint8_t> activateSession(const std::vector<uint8_t>& request,
                                       std::shared_ptr<message::Handler>& handler);

    // 0x3B: Set Session Privilege Level
    std::vector<uint8_t> setSessionPrivilegeLevel(const std::vector<uint8_t>& request,
                                                std::shared_ptr<message::Handler>& handler);

    // 0x3C: Close Session
    std::vector<uint8_t> closeSession(const std::vector<uint8_t>& request,
                                    std::shared_ptr<message::Handler>& handler);
}
}
