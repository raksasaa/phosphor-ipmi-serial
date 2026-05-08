// Channel authentication capability implementation
#include "channel_auth.hpp"
#include "message_handler.hpp"

#include <vector>
#include <memory>
#include <cstdint>

#include <phosphor-logging/lg2.hpp>

namespace serialipmi {
namespace command {

std::vector<uint8_t> getChannelAuthCap(const std::vector<uint8_t>& request,
                                   [[maybe_unused]] std::shared_ptr<message::Handler>& handler)
{
    // Per spec: Request Data[0]=channel, Data[1]=max privilege (ignored for response here)
    uint8_t channel = 0x0E; // default per spec (current channel)
    if (request.size() >= 1)
    {
        channel = request[0];
    }

    lg2::info("Get Channel Auth Cap: channel={CHAN}", "CHAN", lg2::hex, channel);

    // Response layout (data only):
    // - Byte 0: channel (echo back)
    // - Byte 1: auth type support bitmap = 0x01 (None only, bit0)
    // - Byte 2: auth status = 0x0C (per-message auth disabled, user-level auth disabled)
    // - Byte 3: oem id = 0x00
    // - Byte 4-6: OEM continuation = 0x00 0x00 0x00
    // - Byte 7: oem auxiliary = 0x00
    std::vector<uint8_t> resp = {
        channel,
        0x01,
        0x0C,
        0x00,
        0x00, 0x00, 0x00,
        0x00
    };
    return resp;
}

} // namespace command
} // namespace serialipmi
