#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>

namespace serialipmi {
namespace uart {

class Channel {
public:
    using ByteVec = std::vector<uint8_t>;

    Channel(const std::string& devicePath, uint32_t baud);
    ~Channel();

    Channel(const Channel&) = delete;
    Channel& operator=(const Channel&) = delete;
    Channel(Channel&&) = default;
    Channel& operator=(Channel&&) = default;

    std::size_t write(const ByteVec& in);
    int getFd() const;
    void flush();
    void sendBreak();
    const std::string& getDevicePath() const { return devicePath; }

private:
    std::string devicePath;
    int fd{-1};

    static int baudRateToConstant(uint32_t baud);
};

} // namespace uart
} // namespace serialipmi
