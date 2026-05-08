#include "statistics.hpp"

#include <chrono>
#include <iomanip>
#include <sstream>
#include <phosphor-logging/lg2.hpp>

namespace serialipmi {
namespace debug {

Statistics& Statistics::get()
{
    static Statistics instance;
    return instance;
}

Statistics::Statistics()
    : startTime_(std::chrono::steady_clock::now())
{
}

void Statistics::recordFrameReceived()
{
    framesReceived_.fetch_add(1, std::memory_order_relaxed);
}

void Statistics::recordFrameSent()
{
    framesSent_.fetch_add(1, std::memory_order_relaxed);
}

void Statistics::recordFrameError(const std::string& /*reason*/)
{
    frameErrors_.fetch_add(1, std::memory_order_relaxed);
}

void Statistics::recordDbusCall(uint64_t latencyUs)
{
    dbusCallCount_.fetch_add(1, std::memory_order_relaxed);
    dbusCallTotalLatencyUs_.fetch_add(latencyUs, std::memory_order_relaxed);
    // update max latency
    uint64_t currentMax = dbusCallMaxLatencyUs_.load(std::memory_order_relaxed);
    while (latencyUs > currentMax && !dbusCallMaxLatencyUs_.compare_exchange_weak(currentMax, latencyUs)) {
        // retry until updated
    }
}

void Statistics::recordSessionEvent(const std::string& event)
{
    (void)event; // For now, just a hook for future extension
}

void Statistics::recordCommandExecution(uint8_t netFn, uint8_t cmd, uint64_t latencyUs, uint8_t completionCode)
{
    uint16_t key = (static_cast<uint16_t>(netFn) << 8) | static_cast<uint16_t>(cmd);
    std::lock_guard<std::mutex> lk(cmdStatsMutex_);
    auto &stat = commandStats_[key];
    stat.count += 1;
    if (completionCode != 0x00)
    {
        stat.errors += 1;
    }
    stat.totalLatencyUs += latencyUs;
    if (latencyUs > stat.maxLatencyUs)
    {
        stat.maxLatencyUs = latencyUs;
    }
}

void Statistics::printSummary()
{
    auto uptimeSeconds = static_cast<uint32_t>(
        std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now() - startTime_).count());

    lg2::info("Stats frames: rx={RX}, tx={TX}, err={ERR}",
              "RX", static_cast<uint32_t>(framesReceived_.load()),
              "TX", static_cast<uint32_t>(framesSent_.load()),
              "ERR", static_cast<uint32_t>(frameErrors_.load()));

    auto dbusCalls = dbusCallCount_.load();
    uint32_t avgLatency = 0;
    if (dbusCalls > 0)
    {
        avgLatency = static_cast<uint32_t>(
            dbusCallTotalLatencyUs_.load() / dbusCalls);
    }

    lg2::info("Stats dbus: calls={DBUS}, avgUs={AVG}, maxUs={MAX}",
              "DBUS", static_cast<uint32_t>(dbusCalls),
              "AVG", avgLatency,
              "MAX", static_cast<uint32_t>(dbusCallMaxLatencyUs_.load()));

    lg2::info("Stats uptime: seconds={UP}", "UP", uptimeSeconds);
}

void Statistics::reset()
{
    framesReceived_.store(0);
    framesSent_.store(0);
    frameErrors_.store(0);
    dbusCallCount_.store(0);
    dbusCallTotalLatencyUs_.store(0);
    dbusCallMaxLatencyUs_.store(0);
    std::lock_guard<std::mutex> lk(cmdStatsMutex_);
    commandStats_.clear();
    startTime_ = std::chrono::steady_clock::now();
}

// End namespace
} // namespace debug
} // namespace serialipmi
