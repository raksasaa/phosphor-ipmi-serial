// Channel authentication capability for local IPMI channel (no-auth mode)
// Implemented as per: serialipmi::command::getChannelAuthCap
#pragma once

#include <cstdint>
#include <memory>
#include <vector>

namespace serialipmi {
namespace message {
class Handler;
} // namespace message
} // namespace serialipmi

namespace serialipmi {
namespace command {

    // Returns an 8-byte data payload with channel, auth bitmap, status and OEM fields
    // CC (completion code) is always the first byte of the returned vector.
    std::vector<uint8_t> getChannelAuthCap(const std::vector<uint8_t>& request,
                                       std::shared_ptr<message::Handler>& handler);
}
}
