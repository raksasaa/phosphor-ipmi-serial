#include "message_parsers.hpp"
#include "terminal_mode_parser.hpp"
#include "basic_mode_parser.hpp"

#include <chrono>

namespace serialipmi {
namespace parser {

class TerminalParserWrapper : public IParser {
public:
    void feedData(const std::vector<uint8_t>& data) override {
        for (const auto& byte : data) {
            parser.feed(byte);
        }
    }

    bool hasCompleteMessage() const override {
        return parser.hasCompleteMessage();
    }

    message::Message getNextMessage() override {
        auto payload = parser.getNextMessagePayload();
        auto msg = parseTerminalPayload(payload);
        msg.receiveTime = std::chrono::steady_clock::now();
        msg.rawFrame = payload;
        return msg;
    }

    std::vector<uint8_t> encodeResponse(const std::vector<uint8_t>& payload) const override {
        return parser.encodeResponse(payload);
    }

    void reset() override {
        parser.reset();
    }

private:
    terminal::Parser parser;
};

class BasicParserWrapper : public IParser {
public:
    void feedData(const std::vector<uint8_t>& data) override {
        parser.feedData(data);
    }

    bool hasCompleteMessage() const override {
        return parser.hasCompleteMessage();
    }

    message::Message getNextMessage() override {
        return parser.getNextMessage();
    }

    std::vector<uint8_t> encodeResponse(const std::vector<uint8_t>& payload) const override {
        return parser.encodeResponse(payload);
    }

    void reset() override {
        parser.reset();
    }

private:
    basic::Parser parser;
};

std::unique_ptr<IParser> createParser(ProtocolMode mode)
{
    if (mode == ProtocolMode::Terminal) {
        return std::make_unique<TerminalParserWrapper>();
    }
    return std::make_unique<BasicParserWrapper>();
}

message::Message parseTerminalPayload(const std::vector<uint8_t>& payload)
{
    message::Message m;
    m.mode = message::Message::Mode::Terminal;

    if (payload.size() < 3) {
        return m;
    }

    uint8_t nf_lun = payload[0];
    m.netFn = nf_lun >> 2;
    m.lun = nf_lun & 0x03;
    m.seqNum = payload[1];
    m.cmd = payload[2];

    if (payload.size() > 3) {
        m.data.assign(payload.begin() + 3, payload.end());
    }

    return m;
}

message::Message parseBasicPayload(const std::vector<uint8_t>& payload)
{
    message::Message m;
    m.mode = message::Message::Mode::Basic;

    if (payload.size() < 7) {
        return m;
    }

    m.rsSA = payload[0];
    m.netFn = payload[1] >> 2;
    m.lun = payload[1] & 0x03;
    m.rqSA = payload[3];
    m.seqNum = payload[4] >> 2;
    m.cmd = payload[5];

    if (payload.size() > 7) {
        m.data.assign(payload.begin() + 6, payload.end() - 1);
    }

    return m;
}

} // namespace parser
} // namespace serialipmi
