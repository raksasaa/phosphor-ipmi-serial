// Get Message / Send Message interface implementation
// TODO: implement Get Message command for session-based messaging
#include "get_message.hpp"

#include <vector>
#include <memory>
#include <cstdint>

#include <phosphor-logging/lg2.hpp>

namespace serialipmi {
namespace command {
    // Forward declaration
    namespace message { class Handler; }

    std::vector<uint8_t> getMessage(const std::vector<uint8_t>& request,
                                  std::shared_ptr<message::Handler>& handler)
    {
        (void)request;
        (void)handler;
        lg2::info("getMessage invoked");
        // As a minimal implementation, return no data (CC will be set by caller)
        return {};
    }
}
}
