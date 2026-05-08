#include "terminal_mode_parser.hpp"

#include <phosphor-logging/lg2.hpp>
#include <iomanip>
#include <sstream>

namespace serialipmi {
namespace terminal {

Parser::Parser() {
    reset();
}

void Parser::reset() {
    state = ParseState::Idle;
    hexBuffer.clear();
    completed.clear();
    stats = Stats{};
}

static uint8_t hexValue(char c) {
    if ('0' <= c && c <= '9') return static_cast<uint8_t>(c - '0');
    if ('a' <= c && c <= 'f') return static_cast<uint8_t>(c - 'a' + 10);
    if ('A' <= c && c <= 'F') return static_cast<uint8_t>(c - 'A' + 10);
    return 0xFF;
}

static std::vector<uint8_t> hexStringToBytes(const std::string& hex) {
    std::vector<uint8_t> out;
    try {
        std::string s;
        for (char c : hex) {
            if (c == ' ' || c == '\n' || c == '\r' || c == '\t') continue;
            s.push_back(c);
        }
        for (size_t i = 0; i + 1 < s.size(); i += 2) {
            uint8_t hi = hexValue(s[i]);
            uint8_t lo = hexValue(s[i + 1]);
            if (hi == 0xFF || lo == 0xFF) break;
            out.push_back((hi << 4) | lo);
        }
    } catch (const std::exception& e) {
        lg2::error("Invalid Hex input: {ERROR}", "ERROR", e.what());
    }
    return out;
}

void Parser::feed(uint8_t ch) {
    switch (state) {
        case ParseState::Idle:
            if (ch == 0x5B) { // '['
                hexBuffer.clear();
                state = ParseState::ReceivingHex;
            } else if (ch == '\r' || ch == '\n') {
                // 帧分隔符，忽略
            }
            // ignore other chars
            break;
        case ParseState::ReceivingHex:
            if (ch == 0x5D) { // ']'
                // decode
                auto bytes = hexStringToBytes(hexBuffer);
                if (!bytes.empty()) {
                    completed.push_back(bytes);
                    stats.framesReceived++;
                    if (!bytes.empty()) stats.bytesReceived += bytes.size();
                } else {
                    stats.framesError++;
                }
                hexBuffer.clear();
                state = ParseState::Complete;
            } else if (ch == '\r' || ch == '\n') {
                // 帧分隔符，完成当前帧
                auto bytes = hexStringToBytes(hexBuffer);
                if (!bytes.empty()) {
                    completed.push_back(bytes);
                    stats.framesReceived++;
                    stats.bytesReceived += bytes.size();
                } else if (!hexBuffer.empty()) {
                    stats.framesError++;
                }
                hexBuffer.clear();
                state = ParseState::Complete;
            } else if (ch == 0x20) {
                // ignore spaces
            } else if ((ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F')) {
                hexBuffer.push_back(static_cast<char>(ch));
                if (hexBuffer.size() > 160) {
                    state = ParseState::Error;
                    hexBuffer.clear();
                    stats.framesError++;
                }
            } else {
                state = ParseState::Error;
                hexBuffer.clear();
                stats.framesError++;
            }
            break;
        case ParseState::Complete:
            // after a complete frame, go back to idle waiting for next
            state = ParseState::Idle;
            if (ch == 0x5B) {
                hexBuffer.clear();
                state = ParseState::ReceivingHex;
            }
            break;
        case ParseState::Error:
            // reset on error
            state = ParseState::Idle;
            hexBuffer.clear();
            break;
    }
}

bool Parser::hasCompleteMessage() const {
    return !completed.empty();
}

std::vector<uint8_t> Parser::getNextMessagePayload() {
    if (completed.empty()) return {};
    auto v = completed.front();
    completed.erase(completed.begin());
    // track bytes sent for response size later if needed
    return v;
}

std::vector<uint8_t> Parser::encodeResponse(const std::vector<uint8_t>& payload) const {
    std::ostringstream oss;
    oss << "[";
    for (size_t i = 0; i < payload.size(); ++i) {
        if (i) oss << " ";
        oss << std::uppercase << std::setfill('0') << std::setw(2) << std::hex << static_cast<int>(payload[i]);
    }
    oss << "]\r\n";
    std::string str = oss.str();
    return std::vector<uint8_t>(str.begin(), str.end());
}

std::vector<uint8_t> Parser::encodeError(uint8_t code) {
    std::ostringstream oss;
    oss << "[ERR ";
    oss << std::uppercase << std::setw(2) << std::setfill('0') << std::hex << static_cast<int>(code);
    oss << "]\r\n";
    std::string str = oss.str();
    return std::vector<uint8_t>(str.begin(), str.end());
}

} // namespace terminal
} // namespace serialipmi
