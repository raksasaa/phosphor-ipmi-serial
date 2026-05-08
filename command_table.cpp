#include "command_table.hpp"
#include "sessions_manager.hpp"
#include "command/channel_auth.hpp"
#include "command/session_cmds.hpp"
#include "message_handler.hpp"
#include <phosphor-logging/lg2.hpp>

using namespace serialipmi::command;

void Table::registerSessionCommands()
{
  auto bind = [](NetIpmidEntry::Fn fn) {
    return std::make_unique<NetIpmidEntry>(fn);
  };

  constexpr uint8_t netFnApp = 0x06;

  // 0x38: Get Channel Authentication Capabilities
  CommandID id1{netFnApp, 0x38};
  registerCommand(id1, bind([](const std::vector<uint8_t>& req,
                             std::shared_ptr<message::Handler>& handler) {
    return command::getChannelAuthCap(req, handler);
  }));

  // 0x39: Get Session Challenge
  CommandID id2{netFnApp, 0x39};
  registerCommand(id2, bind([](const std::vector<uint8_t>& req,
                             std::shared_ptr<message::Handler>& handler) {
    return command::getSessionChallenge(req, handler);
  }));

  // 0x3A: Activate Session
  CommandID id3{netFnApp, 0x3A};
  registerCommand(id3, bind([](const std::vector<uint8_t>& req,
                             std::shared_ptr<message::Handler>& handler) {
    return command::activateSession(req, handler);
  }));

  // 0x3B: Set Session Privilege Level
  CommandID id4{netFnApp, 0x3B};
  registerCommand(id4, bind([](const std::vector<uint8_t>& req,
                             std::shared_ptr<message::Handler>& handler) {
    return command::setSessionPrivilegeLevel(req, handler);
  }));

  // 0x3C: Close Session
  CommandID id5{netFnApp, 0x3C};
  registerCommand(id5, bind([](const std::vector<uint8_t>& req,
                             std::shared_ptr<message::Handler>& handler) {
    return command::closeSession(req, handler);
  }));
}
