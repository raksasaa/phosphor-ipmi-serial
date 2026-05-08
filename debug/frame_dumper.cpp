// Frame dumper implementation
#include "frame_dumper.hpp"

#include <vector>
#include <string>
#include <iomanip>
#include <sstream>
#include <chrono>
#include <ctime>
#include <cstdint>

#include <phosphor-logging/lg2.hpp>

namespace serialipmi {
namespace debug {

bool FrameDumper::enabled = false;

static std::string currentTimeForLog()
{
    using namespace std::chrono;
    auto now = system_clock::now();
    auto now_time_t = system_clock::to_time_t(now);
    auto ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;
    std::stringstream ss;
    std::tm tm{};
    // localtime may be thread-unsafe in some environments; this is a lightweight helper
    localtime_r(&now_time_t, &tm);
    ss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    ss << "." << std::setfill('0') << std::setw(3) << ms.count();
    return ss.str();
}

static void dumpBytes(const uint8_t* data, size_t len, const std::string& prefix, [[maybe_unused]] bool addNewline = true)
{
    std::stringstream ss;
    ss << "[" << prefix << "] " << currentTimeForLog() << " (" << len << " bytes)\n";
    size_t idx = 0;
    while (idx < len) {
        ss << std::setw(4) << std::setfill('0') << idx << ":";
        for (size_t i = 0; i < 16 && idx < len; ++i, ++idx) {
            ss << " " << std::uppercase << std::hex << std::setw(2) << std::setfill('0')
               << static_cast<int>(data[idx]);
        }
        ss << std::dec << std::setw(0) << std::setfill(' ');
        ss << "\n";
    }
    lg2::info("Frame dump: {DUMP}", "DUMP", ss.str());
}

void FrameDumper::hexdump(const uint8_t* data, size_t len, const std::string& prefix)
{
    if (!enabled || data == nullptr || len == 0)
    {
        return;
    }
    dumpBytes(data, len, prefix);
}

void FrameDumper::dumpReceived(const std::vector<uint8_t>& raw, const std::string& label)
{
    if (!enabled) return;
    dumpBytes(raw.data(), raw.size(), label);
}

void FrameDumper::dumpSent(const std::vector<uint8_t>& raw, const std::string& label)
{
    if (!enabled) return;
    dumpBytes(raw.data(), raw.size(), label);
}

void FrameDumper::dumpMessage(const message::Message& msg, const std::string& direction)
{
    if (!enabled) return;
    lg2::info("FrameDumper::dumpMessage called: DIR={DIRECTION}", "DIRECTION", direction.c_str());
}

} // namespace debug
} // namespace serialipmi
