#pragma once

#include <memory>
#include <vector>
#include <cstdint>
#include <functional>

#include "message.hpp"

namespace serialipmi {
namespace parser {

enum class ProtocolMode {
    Terminal,
    Basic
};

class IParser {
public:
    virtual ~IParser() = default;
    virtual void feedData(const std::vector<uint8_t>& data) = 0;
    virtual bool hasCompleteMessage() const = 0;
    virtual message::Message getNextMessage() = 0; // returns a complete message
    virtual std::vector<uint8_t> encodeResponse(const std::vector<uint8_t>& payload) const = 0;
    virtual void reset() = 0;
};

std::unique_ptr<IParser> createParser(ProtocolMode mode);

// Payload parsers (convert decoded payload bytes to Message)
message::Message parseTerminalPayload(const std::vector<uint8_t>& payload);
message::Message parseBasicPayload(const std::vector<uint8_t>& payload);

} // namespace parser
} // namespace serialipmi
