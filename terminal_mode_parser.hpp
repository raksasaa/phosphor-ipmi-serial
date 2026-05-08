#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "message.hpp"

namespace serialipmi {
namespace terminal {

// Parser for ASCII-hex Terminal Mode frames
class Parser {
public:
    struct Stats {
        uint64_t framesReceived = 0;
        uint64_t framesValid = 0;
        uint64_t framesError = 0;
        uint64_t bytesReceived = 0;
        uint64_t bytesSent = 0;
    } stats;

    Parser();
    void reset();
    // Feed a single byte into the state machine
    void feed(uint8_t ch);
    void feedData(const std::vector<uint8_t>& data) {
        for (const auto& byte : data) {
            feed(byte);
        }
    }
    bool hasCompleteMessage() const;
    // Retrieve next complete decoded payload (NetFn/LUN, Seq, Cmd, Data...)
    // Returns an ASCII-hex decoded payload already converted to bytes.
    std::vector<uint8_t> getNextMessagePayload();

    // Encoding helpers
    std::vector<uint8_t> encodeResponse(const std::vector<uint8_t>& payload) const; // [XX XX ...]\r\n
    static std::vector<uint8_t> encodeError(uint8_t code); // [ERR XX]\r\n
private:
    enum class ParseState {
        Idle,
        ReceivingHex,
        Complete,
        Error
    } state;

    std::string hexBuffer; // collects ASCII hex digits
    std::vector<std::vector<uint8_t>> completed; // decoded frames awaiting retrieval
    static inline uint8_t hexNibble(char c) {
        if ('0' <= c && c <= '9') return static_cast<uint8_t>(c - '0');
        if ('a' <= c && c <= 'f') return static_cast<uint8_t>(c - 'a' + 10);
        if ('A' <= c && c <= 'F') return static_cast<uint8_t>(c - 'A' + 10);
        return 0xFF; // invalid
    }
};

} // namespace terminal
} // namespace serialipmi
