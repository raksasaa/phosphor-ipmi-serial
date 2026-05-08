#include "event_loop.hpp"
#include "main.hpp"
#include "debug/statistics.hpp"
#include "debug/frame_dumper.hpp"
#include "debug/statistics.hpp"
#include "event_loop.hpp"
#include "main.hpp"

#include <poll.h>
#include <unistd.h>

#include <phosphor-logging/lg2.hpp>

#include <cstring>

using namespace serialipmi::eventloop;

std::unique_ptr<EventLoop> EventLoop::instance = nullptr;

EventLoop& EventLoop::getInstance(sdbusplus::bus::bus& bus,
                                  uart::Channel& channel,
                                  std::shared_ptr<message::Handler> handler)
{
    if (!instance)
    {
        instance = std::make_unique<EventLoop>(bus, channel, handler);
    }
    return *instance;
}

EventLoop::EventLoop(sdbusplus::bus::bus& bus, uart::Channel& channel,
                     std::shared_ptr<message::Handler> handler) :
    bus(bus),
    uartChannel(channel), msgHandler(std::move(handler))
{}

void EventLoop::startEventLoop()
{
    lg2::info("Starting IPMI Serial event loop");

    struct pollfd fds[2];
    fds[0].fd = uartChannel.getFd();
    fds[0].events = POLLIN;
    fds[1].fd = bus.get_fd();
    fds[1].events = POLLIN;

    while (g_running)
    {
        int ret = poll(fds, 2, 5000);
        if (ret < 0)
        {
            lg2::error("Poll error: {ERROR}", "ERROR", strerror(errno));
            continue;
        }
        if (ret == 0)
        {
            continue;
        }

        if (fds[0].revents & POLLIN)
        {
            processSerialData();
            processFrames();
        }

        if (fds[1].revents & POLLIN)
        {
            bus.process_discard();
        }
    }

    lg2::info("IPMI Serial event loop stopped");
}

void EventLoop::processSerialData()
{
    if (!msgHandler)
    {
        return;
    }

    std::array<uint8_t, 512> readBuffer{};
    ssize_t bytesRead =
        read(uartChannel.getFd(), readBuffer.data(), readBuffer.size());

    if (bytesRead > 0)
    {
        readAccumulator.insert(readAccumulator.end(), readBuffer.begin(),
                               readBuffer.begin() + bytesRead);
    }
    else if (bytesRead < 0 && errno != EAGAIN && errno != EWOULDBLOCK)
    {
        lg2::error("UART read error: {ERROR}", "ERROR", strerror(errno));
    }
}

void EventLoop::processFrames()
{
    if (readAccumulator.empty())
    {
        return;
    }

    if (msgHandler)
    {
        std::vector<uint8_t> rxData = readAccumulator;
        readAccumulator.clear();

        for (auto byte : rxData)
        {
            msgHandler->processByte(byte);
        }
    }
}
