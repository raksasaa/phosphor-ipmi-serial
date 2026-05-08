// Runtime statistics for IPMI serial daemon
#pragma once

#include <cstdint>
#include <chrono>
#include <atomic>
#include <map>
#include <mutex>
#include <string>
#include <memory>

namespace serialipmi {
namespace debug {

struct CommandStat {
    uint64_t count = 0;
    uint64_t errors = 0;
    uint64_t totalLatencyUs = 0;
    uint64_t maxLatencyUs = 0;
};

class Statistics {
public:
    static Statistics& get();

    void recordFrameReceived();
    void recordFrameSent();
    void recordFrameError(const std::string& reason);
    void recordDbusCall(uint64_t latencyUs);
    void recordSessionEvent(const std::string& event);
    void recordCommandExecution(uint8_t netFn, uint8_t cmd, uint64_t latencyUs, uint8_t completionCode);
    void printSummary();
    void reset();

private:
    Statistics();
    ~Statistics() = default;
    Statistics(const Statistics&) = delete;
    Statistics& operator=(const Statistics&) = delete;

    // frame counters (atomic)
    std::atomic<uint64_t> framesReceived_{0};
    std::atomic<uint64_t> framesSent_{0};
    std::atomic<uint64_t> frameErrors_{0};

    // DBus call timing stats
    std::atomic<uint64_t> dbusCallCount_{0};
    std::atomic<uint64_t> dbusCallTotalLatencyUs_{0};
    std::atomic<uint64_t> dbusCallMaxLatencyUs_{0};

    // Per-command stats map
    std::mutex cmdStatsMutex_;
    std::map<uint16_t, CommandStat> commandStats_;

    // Uptime reference
    std::chrono::steady_clock::time_point startTime_;
};

} // namespace debug
} // namespace serialipmi
