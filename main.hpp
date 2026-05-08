#pragma once

#include <memory>
#include <string>
#include <systemd/sd-bus.h>

namespace sdbusplus { namespace bus { class bus; } }

extern volatile sig_atomic_t g_running;

namespace serialipmi {
namespace uart { class Channel; }
namespace eventloop {
class EventLoop;
}

sd_bus* ipmid_get_sd_bus_connection();
std::shared_ptr<sdbusplus::bus::bus> getSdBus();
bool isDebugFrames();
bool isVerbose();

} // namespace serialipmi
