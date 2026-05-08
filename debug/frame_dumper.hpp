#pragma once

#include <vector>
#include <string>

#include "../message.hpp"

namespace serialipmi {
namespace debug {

class FrameDumper {
public:
    static bool enabled;

    static void dumpReceived(const std::vector<uint8_t>& raw, const std::string& label = "RX");
    static void dumpSent(const std::vector<uint8_t>& raw, const std::string& label = "TX");

    static void dumpMessage(const message::Message& msg, const std::string& direction);
    static void hexdump(const uint8_t* data, size_t len, const std::string& prefix);
};

} // namespace debug
} // namespace serialipmi
