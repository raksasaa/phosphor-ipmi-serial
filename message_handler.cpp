#include "message_handler.hpp"
#include "main.hpp"
#include "message.hpp"
#include "uart_channel.hpp"
#include "command_table.hpp"
#include "debug/frame_dumper.hpp"
#include "debug/statistics.hpp"
#include "main.hpp"
#include "message.hpp"
#include "message_handler.hpp"
#include "message_parsers.hpp"
#include "sessions_manager.hpp"
#include "uart_channel.hpp"

#include <sdbusplus/bus.hpp>
#include <sdbusplus/exception.hpp>
#include <sdbusplus/message.hpp>

#include <chrono>
#include <cstdint>
#include <optional>
#include <vector>

using DbusVariant = std::variant<uint8_t, uint16_t, uint32_t, int16_t, int32_t,
                                 bool, std::string, std::vector<uint8_t>>;

using namespace serialipmi::message;

Handler::Handler(std::shared_ptr<uart::Channel> chan,
                 std::shared_ptr<parser::IParser> parser) :
    channel_(std::move(chan)),
    parser_(std::move(parser))
{
    lg2::info("serialipmi::message::Handler constructed");
}

void Handler::processByte(uint8_t byte)
{
    rxBuffer_.push_back(byte);
    tryParseMessages();
}

void Handler::tryParseMessages()
{
    parser_->feedData(rxBuffer_);

    while (parser_->hasCompleteMessage())
    {
        auto msg = parser_->getNextMessage();
        processIncoming(msg);
    }

    rxBuffer_.clear();
}

void Handler::flushFrames()
{
    if (parser_ && !rxBuffer_.empty())
    {
        parser_->feedData(rxBuffer_);
        while (parser_->hasCompleteMessage())
        {
            auto msg = parser_->getNextMessage();
            processIncoming(msg);
        }
        rxBuffer_.clear();
    }
}

void Handler::processIncoming(Message& inMsg)
{
    currentMsg_ = inMsg;
    lg2::debug("processIncoming: netFn={NETFN}, cmd={CMD}", "NETFN", lg2::hex,
               inMsg.netFn, "CMD", lg2::hex, inMsg.cmd);

    if (isDebugFrames())
    {
        debug::FrameDumper::dumpMessage(inMsg, "RX");
    }

    serialipmi::command::CommandID id{inMsg.netFn, inMsg.cmd};
    auto self = shared_from_this();

    auto respDataOpt =
        serialipmi::command::Table::get().executeCommand(id, inMsg.data, self);

    if (respDataOpt.has_value())
    {
        Message resp;
        resp.netFn = inMsg.netFn | 0x01;
        resp.lun = inMsg.lun;
        resp.seqNum = inMsg.seqNum;
        resp.cmd = inMsg.cmd;
        resp.completionCode = 0x00;
        resp.data = respDataOpt.value();
        resp.sessionId = inMsg.sessionId;
        outMessage_ = resp;
        lg2::debug("Local command executed: NetFn={NETFN}, Cmd={CMD}, CC={CC}",
                   "NETFN", lg2::hex, resp.netFn, "CMD", lg2::hex, resp.cmd,
                   "CC", lg2::hex, resp.completionCode);
        sendResponse(resp);
        return;
    }

    forwardToIpmid(inMsg.netFn, inMsg.lun, inMsg.cmd, inMsg.data,
                   inMsg.sessionId);
}

void Handler::sendResponse(const Message& resp)
{
    std::vector<uint8_t> respPayload;
    respPayload.push_back(
        static_cast<uint8_t>((resp.netFn << 2) | (resp.lun & 0x03)));
    respPayload.push_back(resp.seqNum);
    respPayload.push_back(resp.cmd);
    respPayload.push_back(resp.completionCode);
    respPayload.insert(respPayload.end(), resp.data.begin(), resp.data.end());

    auto encoded = parser_->encodeResponse(respPayload);
    if (!encoded.empty())
    {
        std::vector<uint8_t> txData(encoded.begin(), encoded.end());
        if (channel_)
        {
            channel_->write(txData);
            debug::Statistics::get().recordFrameSent();
            if (isDebugFrames())
            {
                debug::FrameDumper::dumpSent(txData, "TX");
            }
        }
    }
}

void Handler::forwardToIpmid(uint8_t netFn, uint8_t lun, uint8_t cmd,
                             const std::vector<uint8_t>& data,
                             uint32_t sessionId)
{
    lg2::info("Forwarding to ipmid: NetFn={NETFN}, Lun={LUN}, Cmd={CMD}",
              "NETFN", lg2::hex, netFn, "LUN", lg2::hex, lun, "CMD", lg2::hex,
              cmd);

    auto bus = getSdBus();
    if (!bus)
    {
        lg2::error("D-Bus connection not available");
        return;
    }

    auto method = bus->new_method_call("xyz.openbmc_project.Ipmi.Host",
                                       "/xyz/openbmc_project/Ipmi",
                                       "xyz.openbmc_project.Ipmi.Server",
                                       "execute");

    method.append(netFn, lun, cmd, data,
                  std::map<std::string, DbusVariant>{});

    auto seqNum = currentMsg_.seqNum;

    [[maybe_unused]] auto slot = method.call_async(
        [this, netFn, lun, cmd, sessionId,
         seqNum](sdbusplus::message::message&& reply) {
        if (reply.is_method_error())
        {
            lg2::error("D-Bus call failed");
            Message resp;
            resp.netFn = netFn | 0x01;
            resp.lun = lun;
            resp.cmd = cmd;
            resp.completionCode = 0xCC;
            resp.sessionId = sessionId;
            sendResponse(resp);
            return;
        }

        uint8_t retNetFn, retLun, retCmd, cc;
        std::vector<uint8_t> retData;
        reply.read(retNetFn, retLun, retCmd, cc, retData);

        Message resp;
        resp.netFn = retNetFn;
        resp.lun = retLun;
        resp.seqNum = seqNum;
        resp.cmd = retCmd;
        resp.completionCode = cc;
        resp.data = std::move(retData);
        resp.sessionId = sessionId;
        sendResponse(resp);
    });
}

std::optional<Message> Handler::getResponse()
{
    if (outMessage_)
    {
        auto m = std::move(*outMessage_);
        outMessage_.reset();
        return m;
    }
    return std::nullopt;
}

void Handler::setResponsePayload(const std::vector<uint8_t>& payload)
{
    Message resp;
    resp.netFn = currentMsg_.netFn | 0x01;
    resp.lun = currentMsg_.lun;
    resp.seqNum = currentMsg_.seqNum;
    resp.cmd = currentMsg_.cmd;
    resp.completionCode = 0x00;
    resp.data = payload;
    outMessage_ = resp;
}

