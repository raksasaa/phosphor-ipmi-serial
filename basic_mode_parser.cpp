#include "basic_mode_parser.hpp"
#include "message.hpp"

#include <chrono>

namespace serialipmi {
namespace basic {

Parser::Parser()
{
    reset();
}

void Parser::reset()
{
    state = ParseState::MsgNone;
    msgBuffer.clear();
    while (!completedMessages.empty()) {
        completedMessages.pop();
    }
    stats = Stats{};
}

void Parser::feed(uint8_t ch)
{
    switch (state) {
    case ParseState::MsgNone:
        if (ch == BMC_START) {
            msgBuffer.clear();
            state = ParseState::MsgInProgress;
        }
        break;

    case ParseState::MsgInProgress:
        if (ch == BMC_ESCAPE) {
            state = ParseState::MsgEscape;
        }
        else if (ch == BMC_STOP) {
            if (validateChecksum(msgBuffer)) {
                completedMessages.push(msgBuffer);
                stats.framesReceived++;
                stats.framesValid++;
                stats.bytesReceived += msgBuffer.size();
            }
            else {
                stats.framesReceived++;
                stats.framesChecksumError++;
            }
            msgBuffer.clear();
            state = ParseState::MsgDone;
        }
        else if (ch == BMC_START) {
            msgBuffer.clear();
        }
        else {
            msgBuffer.push_back(ch);
        }
        break;

    case ParseState::MsgEscape:
        msgBuffer.push_back(ch ^ 0x10);
        state = ParseState::MsgInProgress;
        break;

    case ParseState::MsgDone:
        if (ch == BMC_START) {
            msgBuffer.clear();
            state = ParseState::MsgInProgress;
        }
        else {
            state = ParseState::MsgNone;
        }
        break;
    }
}

void Parser::feedData(const std::vector<uint8_t>& data)
{
    for (const auto& byte : data) {
        feed(byte);
    }
}

bool Parser::hasCompleteMessage() const
{
    return !completedMessages.empty();
}

message::Message Parser::getNextMessage()
{
    message::Message msg;
    msg.mode = message::Message::Mode::Basic;
    msg.receiveTime = std::chrono::steady_clock::now();

    if (completedMessages.empty()) {
        return msg;
    }

    auto raw = completedMessages.front();
    completedMessages.pop();

    msg.rawFrame = raw;

    if (raw.size() < 7) {
        return msg;
    }

    msg.rsSA = raw[0];
    msg.netFn = raw[1] >> 2;
    msg.lun = raw[1] & 0x03;
    msg.rqSA = raw[3];
    msg.seqNum = raw[4] >> 2;
    msg.cmd = raw[5];

    if (raw.size() > 7) {
        msg.data.assign(raw.begin() + 6, raw.end() - 1);
    }

    return msg;
}

std::vector<uint8_t> Parser::encodeResponse(const std::vector<uint8_t>& payload) const
{
    std::vector<uint8_t> out;
    out.push_back(BMC_START);
    for (const auto& byte : payload) {
        switch (byte) {
        case BMC_START:
        case BMC_STOP:
        case BMC_HANDSHAKE:
        case BMC_ESCAPE:
            out.push_back(BMC_ESCAPE);
            out.push_back(byte ^ 0x10);
            break;
        default:
            out.push_back(byte);
        }
    }
    out.push_back(BMC_STOP);
    return out;
}

std::vector<uint8_t> Parser::encodeHandshake() const
{
    return {BMC_HANDSHAKE};
}

uint8_t Parser::checksum(const std::vector<uint8_t>& bytes)
{
    uint32_t sum = 0;
    for (auto b : bytes) {
        sum += b;
    }
    return static_cast<uint8_t>((~sum + 1) & 0xFF);
}

bool Parser::validateChecksum(const std::vector<uint8_t>& m)
{
    if (m.size() < 7) {
        return false;
    }

    uint8_t rsSA = m[0];
    uint8_t netfn_rslun = m[1];
    uint8_t csum1 = m[2];
    uint8_t rqSA = m[3];
    uint8_t rqSeq_rqlun = m[4];
    uint8_t cmd = m[5];

    uint16_t sum1 = rsSA + netfn_rslun + csum1;
    if ((sum1 & 0xFF) != 0) {
        return false;
    }

    uint16_t sum2 = rqSA + rqSeq_rqlun + cmd;
    for (size_t i = 6; i + 1 < m.size(); ++i) {
        sum2 += m[i];
    }
    uint8_t csum2 = m.back();

    return ((sum2 + csum2) & 0xFF) == 0;
}

} // namespace basic
} // namespace serialipmi
