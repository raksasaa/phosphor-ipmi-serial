#include "message_handler.hpp"
#include "main.hpp"
#include "message.hpp"
#include "uart_channel.hpp"
#include "command_table.hpp"
#include "debug/frame_dumper.hpp"
#include "debug/statistics.hpp"
#include "message_parsers.hpp"
#include "sessions_manager.hpp"

#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/exception.hpp>
#include <sdbusplus/message.hpp>

#include <chrono>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <variant>
#include <vector>

using DbusVariant = std::variant<uint8_t, uint16_t, uint32_t, int16_t, int32_t,
                                 bool, std::string, std::vector<uint8_t>>;

using namespace serialipmi::message;

Handler::Handler(std::shared_ptr<uart::Channel> chan,
                 std::shared_ptr<parser::IParser> parser,
                 const std::string& channelName) :
    channel_(std::move(chan)),
    parser_(std::move(parser)),
    channelName_(channelName)
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
    
    uint8_t netfnLun = static_cast<uint8_t>((resp.netFn << 2) | (resp.lun & 0x03));
    uint8_t seqLun = static_cast<uint8_t>((resp.seqNum << 2) | (resp.lun & 0x03));
    
    uint8_t targetAddr = resp.rqSA;
    uint8_t sourceAddr = resp.rsSA;
    
    respPayload.push_back(targetAddr);
    respPayload.push_back(netfnLun);
    
    uint8_t csum1 = static_cast<uint8_t>(~(targetAddr + netfnLun) + 1);
    respPayload.push_back(csum1);
    
    respPayload.push_back(sourceAddr);
    respPayload.push_back(seqLun);
    respPayload.push_back(resp.cmd);
    respPayload.push_back(resp.completionCode);
    
    if (!resp.data.empty())
    {
        respPayload.insert(respPayload.end(), resp.data.begin(), resp.data.end());
    }
    
    uint16_t sum2 = sourceAddr + seqLun + resp.cmd + resp.completionCode;
    for (const auto& byte : resp.data)
    {
        sum2 += byte;
    }
    uint8_t csum2 = static_cast<uint8_t>(~sum2 + 1);
    respPayload.push_back(csum2);

    auto encoded = parser_->encodeResponse(respPayload);
    if (!encoded.empty())
    {
        std::vector<uint8_t> txData(encoded.begin(), encoded.end());
        if (channel_)
        {
            lg2::info("Response frame structure:");
            lg2::info("  Target Address (byte 0): {TGT}", "TGT", lg2::hex, targetAddr);
            lg2::info("  NetFn/LUN (byte 1): {NETFN_LUN}", "NETFN_LUN", lg2::hex, netfnLun);
            lg2::info("  Checksum1 (byte 2): {CSUM1}", "CSUM1", lg2::hex, csum1);
            lg2::info("  Source Address (byte 3): {SRC}", "SRC", lg2::hex, sourceAddr);
            lg2::info("  SeqNum/LUN (byte 4): {SEQ_LUN}", "SEQ_LUN", lg2::hex, seqLun);
            lg2::info("  Command (byte 5): {CMD}", "CMD", lg2::hex, resp.cmd);
            lg2::info("  Completion Code (byte 6): {CC}", "CC", lg2::hex, resp.completionCode);
            lg2::info("  Data length: {LEN}", "LEN", resp.data.size());
            lg2::info("  Checksum2 (last byte): {CSUM2}", "CSUM2", lg2::hex, csum2);
            
            std::string hexFrame;
            for (size_t i = 0; i < std::min(txData.size(), static_cast<size_t>(64)); ++i)
            {
                char buf[4];
                snprintf(buf, sizeof(buf), "%02X ", txData[i]);
                hexFrame += buf;
            }
            if (txData.size() > 64)
            {
                hexFrame += "...";
            }
            lg2::info("Full TX frame ({LEN} bytes): {FRAME}", "LEN", txData.size(), "FRAME", hexFrame);
            
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
    lg2::info("Forwarding to ipmid: NetFn={NETFN}, Lun={LUN}, Cmd={CMD}, Channel={CHANNEL}",
              "NETFN", lg2::hex, netFn, "LUN", lg2::hex, lun, "CMD", lg2::hex,
              cmd, "CHANNEL", channelName_);

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

    std::map<std::string, DbusVariant> options;
    options["channel"] = channelName_;
    options["userId"] = uint8_t(1);

    method.append(netFn, lun, cmd, data, options);

    auto seqNum = currentMsg_.seqNum;
    auto self = shared_from_this();

    asyncSlot_ = method.call_async(
        [self, netFn, lun, cmd, sessionId,
         seqNum](sdbusplus::message::message&& reply) {
        try
        {
            if (reply.is_method_error())
            {
                std::string errName;
                std::string errMsg;
                auto err = reply.get_error();
                if (err)
                {
                    errName = err->name;
                    errMsg = err->message;
                }
                lg2::error("D-Bus call failed: {ERR_NAME}: {ERR_MSG}",
                           "ERR_NAME", errName, "ERR_MSG", errMsg);
                Message resp;
                resp.netFn = netFn | 0x01;
                resp.lun = lun;
                resp.cmd = cmd;
                resp.completionCode = 0xCC;
                resp.seqNum = seqNum;
                resp.sessionId = sessionId;
                self->sendResponse(resp);
                return;
            }

            lg2::debug("Attempting to read D-Bus response...");
            
            std::tuple<uint8_t, uint8_t, uint8_t, uint8_t, std::vector<uint8_t>> respTuple;
            reply.read(respTuple);
            
            uint8_t retNetFn = std::get<0>(respTuple);
            uint8_t retLun = std::get<1>(respTuple);
            uint8_t retCmd = std::get<2>(respTuple);
            uint8_t cc = std::get<3>(respTuple);
            std::vector<uint8_t> retData = std::get<4>(respTuple);

            lg2::info("D-Bus response received: NetFn={NETFN}, Cmd={CMD}, CC={CC}",
                      "NETFN", lg2::hex, retNetFn,
                      "CMD", lg2::hex, retCmd,
                      "CC", lg2::hex, cc);
            
            lg2::debug("D-Bus response details: Lun={LUN}, DataLen={LEN}",
                       "LUN", lg2::hex, retLun,
                       "LEN", retData.size());

            if (!retData.empty())
            {
                std::string hexData;
                for (size_t i = 0; i < std::min(retData.size(), static_cast<size_t>(32)); ++i)
                {
                    char buf[4];
                    snprintf(buf, sizeof(buf), "%02X ", retData[i]);
                    hexData += buf;
                }
                if (retData.size() > 32)
                {
                    hexData += "...";
                }
                lg2::debug("Response data: {DATA}", "DATA", hexData);
            }

            Message resp;
            resp.netFn = retNetFn;
            resp.lun = retLun;
            resp.seqNum = seqNum;
            resp.cmd = retCmd;
            resp.completionCode = cc;
            resp.data = std::move(retData);
            resp.sessionId = sessionId;
            
            lg2::info("Sending response to serial: SeqNum={SEQ}, NetFn={NETFN}, Cmd={CMD}, CC={CC}",
                      "SEQ", lg2::hex, seqNum,
                      "NETFN", lg2::hex, resp.netFn,
                      "CMD", lg2::hex, resp.cmd,
                      "CC", lg2::hex, resp.completionCode);
            
            self->sendResponse(resp);
        }
        catch (const std::exception& e)
        {
            lg2::error("D-Bus callback exception: {ERROR}", "ERROR", e.what());
            Message resp;
            resp.netFn = netFn | 0x01;
            resp.lun = lun;
            resp.cmd = cmd;
            resp.completionCode = 0xFF;
            resp.seqNum = seqNum;
            resp.sessionId = sessionId;
            self->sendResponse(resp);
        }
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

