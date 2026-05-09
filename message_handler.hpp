#ifndef SERIALIPMI_MESSAGE_HANDLER_HPP
#define SERIALIPMI_MESSAGE_HANDLER_HPP

#include <memory>
#include <optional>
#include <vector>
#include <cstdint>
#include <string>

#include "message.hpp"

#include <sdbusplus/slot.hpp>

namespace serialipmi {
namespace uart { class Channel; }
namespace parser { class IParser; }
}

namespace serialipmi {
namespace message {

class Handler : public std::enable_shared_from_this<Handler> {
public:
  Handler(std::shared_ptr<uart::Channel> chan,
          std::shared_ptr<parser::IParser> parser,
          const std::string& channelName);

  void processByte(uint8_t byte);
  void flushFrames();
  void sendResponse(const Message& resp);

  std::optional<Message> getResponse();
  void setResponsePayload(const std::vector<uint8_t>& payload);

private:
  void tryParseMessages();
  void processIncoming(Message& inMsg);
  void forwardToIpmid(uint8_t netFn, uint8_t lun, uint8_t cmd,
                      const std::vector<uint8_t>& data, uint32_t sessionId);

  std::shared_ptr<uart::Channel> channel_;
  std::shared_ptr<parser::IParser> parser_;
  std::vector<uint8_t> rxBuffer_;
  std::optional<Message> outMessage_;
  Message currentMsg_;
  std::string channelName_;
  sdbusplus::slot::slot asyncSlot_;
};

} // namespace message
} // namespace serialipmi

#endif
