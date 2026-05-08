#pragma once

#include <memory>
#include <vector>
#include <cstdint>

#include "uart_channel.hpp"
#include <sdbusplus/bus.hpp>
#include "message_handler.hpp"
#include "uart_channel.hpp"

namespace serialipmi
{
namespace eventloop
{

class EventLoop
{
  public:
    EventLoop(sdbusplus::bus::bus& bus, uart::Channel& channel,
              std::shared_ptr<message::Handler> handler);
    ~EventLoop() = default;

    EventLoop(const EventLoop&) = delete;
    EventLoop& operator=(const EventLoop&) = delete;

    static EventLoop& getInstance(sdbusplus::bus::bus& bus,
                                  uart::Channel& channel,
                                  std::shared_ptr<message::Handler> handler);

    void startEventLoop();

  private:
    sdbusplus::bus::bus& bus;
    uart::Channel& uartChannel;
    std::shared_ptr<message::Handler> msgHandler;
    std::vector<uint8_t> readAccumulator;

    static std::unique_ptr<EventLoop> instance;

    void processSerialData();
    void processFrames();
};

} // namespace eventloop
} // namespace serialipmi
