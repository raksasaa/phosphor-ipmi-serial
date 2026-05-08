#pragma once

#include <cstdint>
#include <vector>
#include <chrono>

namespace serialipmi {
namespace message {

// Message container for IPMI Serial protocol
struct Message {
    // Operational mode of the message
    enum class Mode {
        Terminal,
        Basic
    } mode;

    // Common IPMI fields
    uint8_t netFn = 0;      // NetFn for Terminal (upper 6 bits) / NetFn for Basic (from NetFn/rsLUN)
    uint8_t lun = 0;        // LUN (2 bits) for Terminal
    uint8_t seqNum = 0;     // Sequence number
    uint8_t cmd = 0;        // Command
    std::vector<uint8_t> data; // Data payload

    // Serial/IPMI addressing defaults
    uint8_t rsSA = 0x20;    // Recipient Slave Address (default)
    uint8_t rqSA = 0x81;    // Requestor Slave Address (default)

    uint32_t sessionId = 0; // Session identifier (if any)
    uint8_t completionCode = 0; // Completion code (for responses)

    // Raw/decoded framing information
    std::vector<uint8_t> rawFrame;
    std::chrono::steady_clock::time_point receiveTime;

    // Special invalid session constant
    static constexpr uint32_t MESSAGE_INVALID_SESSION_ID = 0;
};

} // namespace message
} // namespace serialipmi
