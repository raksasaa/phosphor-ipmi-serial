#pragma once

#include <cstdint>
#include <vector>
#include <queue>

#include "message.hpp"

namespace serialipmi {
namespace basic {

constexpr uint8_t BMC_START     = 0xA0;
constexpr uint8_t BMC_STOP      = 0xA5;
constexpr uint8_t BMC_HANDSHAKE = 0xA6;
constexpr uint8_t BMC_ESCAPE    = 0xAA;

class Parser {
public:
    enum class ParseState {
        MsgNone,
        MsgInProgress,
        MsgEscape,
        MsgDone
    };

    Parser();

    void feed(uint8_t ch);
    void feedData(const std::vector<uint8_t>& data);
    void reset();
    bool hasCompleteMessage() const;
    message::Message getNextMessage();

    std::vector<uint8_t> encodeResponse(const std::vector<uint8_t>& payload) const;
    std::vector<uint8_t> encodeHandshake() const;

    struct Stats {
        uint64_t framesReceived = 0;
        uint64_t framesValid = 0;
        uint64_t framesChecksumError = 0;
        uint64_t framesError = 0;
        uint64_t handshakesSent = 0;
        uint64_t bytesReceived = 0;
        uint64_t bytesSent = 0;
    } stats;

private:
    ParseState state;
    std::vector<uint8_t> msgBuffer;
    std::queue<std::vector<uint8_t>> completedMessages;

    static uint8_t checksum(const std::vector<uint8_t>& bytes);
    static bool validateChecksum(const std::vector<uint8_t>& m);
};

} // namespace basic
} // namespace serialipmi
