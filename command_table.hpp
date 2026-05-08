#ifndef SERIALIPMI_COMMAND_TABLE_HPP
#define SERIALIPMI_COMMAND_TABLE_HPP

#include <memory>
#include <vector>
#include <unordered_map>
#include <functional>
#include <optional>
#include <cstdint>

namespace serialipmi {
namespace message { class Message; class Handler; }
namespace command {
  // Forward declaration of Handler to avoid header dependency loops
  class Table;
  class NetIpmidEntry;
}
}

namespace serialipmi {
namespace command {

// NetFn constants (subset used by this module)
struct NetFns {
  static constexpr uint8_t APP = 0x06;
  static constexpr uint8_t STORAGE = 0x0A;
  static constexpr uint8_t TRANSPORT = 0x0C;
  static constexpr uint8_t GROUP = 0x2C;
};

// Command identifier (netFn, cmd) with key-generation helper
struct CommandID {
  uint8_t netFn{0};
  uint8_t cmd{0};
  uint32_t toKey() const { return (static_cast<uint32_t>(netFn) << 8) | static_cast<uint32_t>(cmd); }
};

// Base class for executable command entries
class Entry {
public:
  virtual ~Entry() = default;
  virtual std::vector<uint8_t> execute(const std::vector<uint8_t>& request,
                                     std::shared_ptr<message::Handler>& handler) = 0;
};

// Entry that forwards to an IPMI/IPMID handler (local to the system) via a provided function
class NetIpmidEntry : public Entry {
public:
  using Fn = std::function<std::vector<uint8_t>(const std::vector<uint8_t>&,
                                             std::shared_ptr<message::Handler>&)>;
  NetIpmidEntry(Fn f) : fn_(f) {}
  std::vector<uint8_t> execute(const std::vector<uint8_t>& request,
                               std::shared_ptr<message::Handler>& handler) override {
    return fn_(request, handler);
  }
private:
  Fn fn_;
};

class ProviderEntry : public Entry {
public:
  ProviderEntry() = default;
  std::vector<uint8_t> execute(const std::vector<uint8_t>& /*request*/,
                               std::shared_ptr<message::Handler>& /*handler*/) override {
    return {};
  }
};

// Placeholder for commands that should be forwarded via DBus in the future

class Table {
public:
  static Table& get() {
    static Table t;
    return t;
  }

  void registerCommand(CommandID id, std::unique_ptr<Entry> entry) {
    commands_[id.toKey()] = std::move(entry);
  }

  std::optional<std::vector<uint8_t>> executeCommand(const CommandID& id,
                                                  const std::vector<uint8_t>& request,
                                                  std::shared_ptr<message::Handler>& handler) {
    auto key = id.toKey();
    auto it = commands_.find(key);
    if (it == commands_.end()) {
      return std::nullopt;
    }
    auto data = it->second->execute(request, handler);
    // 命令已注册即视为已处理，空返回值表示无附加数据（如 closeSession）
    return data;
  }

  // Register all session-related commands (per task requirements)
  void registerSessionCommands();

private:
  Table() = default;
  std::unordered_map<uint32_t, std::unique_ptr<Entry>> commands_;
};

} // namespace command
} // namespace serialipmi

#endif // SERIALIPMI_COMMAND_TABLE_HPP
