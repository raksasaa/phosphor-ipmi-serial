// Get Message / Send Message interface (pattern)
#pragma once

#include <cstdint>
#include <vector>
#include <memory>

namespace message {
class Handler; // forward
class Message; // forward (for dump/debug if needed)
}

namespace serialipmi {
namespace command {

    // Get Message command (NetFn APP, Cmd 0x33)
    std::vector<uint8_t> getMessage(const std::vector<uint8_t>& request,
                                  std::shared_ptr<message::Handler>& handler);
}
}
