// Local (APP NetFn) IPMI session command handlers
#include "session_cmds.hpp"
#include "message_handler.hpp"

#include <vector>
#include <memory>
#include <cstdint>

#include <phosphor-logging/lg2.hpp>

namespace serialipmi {
namespace command {

    std::vector<uint8_t> getSessionChallenge(const std::vector<uint8_t>& request,
                                             std::shared_ptr<message::Handler>& handler)
    {
        (void)handler;
        (void)request;
        lg2::info("getSessionChallenge invoked");
        // Response: 16-byte challenge (zeros) + 4-byte session ID
        std::vector<uint8_t> resp;
        // 16-byte challenge (zeros)
        for (int i = 0; i < 16; ++i)
        {
            resp.push_back(0x00);
        }
        // 4-byte session ID (default to 1, little-endian)
        uint32_t sid = 1;
        resp.push_back(static_cast<uint8_t>(sid & 0xFF));
        resp.push_back(static_cast<uint8_t>((sid >> 8) & 0xFF));
        resp.push_back(static_cast<uint8_t>((sid >> 16) & 0xFF));
        resp.push_back(static_cast<uint8_t>((sid >> 24) & 0xFF));
        return resp;
    }

    std::vector<uint8_t> activateSession(const std::vector<uint8_t>& request,
                                       std::shared_ptr<message::Handler>& handler)
    {
        (void)handler;
        (void)request;
        lg2::info("activateSession invoked");
        // Response: 4-byte session ID + 1-byte initial sequence number
        std::vector<uint8_t> resp;
        uint32_t sid = 1;
        resp.push_back(static_cast<uint8_t>(sid & 0xFF));
        resp.push_back(static_cast<uint8_t>((sid >> 8) & 0xFF));
        resp.push_back(static_cast<uint8_t>((sid >> 16) & 0xFF));
        resp.push_back(static_cast<uint8_t>((sid >> 24) & 0xFF));
        // Initial sequence number
        resp.push_back(0x01);
        return resp;
    }

    std::vector<uint8_t> setSessionPrivilegeLevel(const std::vector<uint8_t>& request,
                                                std::shared_ptr<message::Handler>& handler)
    {
        (void)handler;
        (void)request;
        lg2::info("setSessionPrivilegeLevel invoked");
        // Response: current privilege level (echoed back)
        std::vector<uint8_t> resp;
        uint8_t privilege = 4; // Administrator by default
        resp.push_back(privilege);
        return resp;
    }

    std::vector<uint8_t> closeSession(const std::vector<uint8_t>& request,
                                    std::shared_ptr<message::Handler>& handler)
    {
        (void)handler;
        (void)request;
        lg2::info("closeSession invoked");
        // Response: empty (CC will be set to 0x00 by setResponsePayload)
        return {};
    }
}
}
