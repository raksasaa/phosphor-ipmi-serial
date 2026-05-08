#include "sessions_manager.hpp"

namespace serialipmi {
namespace session {

Manager& Manager::get()
{
    static Manager instance;
    return instance;
}

void Manager::managerInit(const std::string& channelName)
{
    channelName_ = channelName;
    maxSessions_ = 1; // serial limitation
}

uint32_t Manager::createSession()
{
    if (sessions_.size() >= maxSessions_) {
        lg2::error("Maximum sessions reached for channel: {CHANNEL}", "CHANNEL", channelName_);
        return 0;
    }

    static std::uniform_int_distribution<uint32_t> dist(1, UINT32_MAX);
    uint32_t sid;
    int attempts = 0;
    do
    {
        sid = dist(rng);
        attempts++;
    } while (sessions_.count(sid) && attempts < 100);

    if (attempts >= 100)
    {
        lg2::error("Failed to generate unique session ID");
        return 0;
    }

    auto s = std::make_shared<Session>(sid);
    s->isActive = true;
    s->lastActive = std::chrono::steady_clock::now();
    sessions_.emplace(sid, s);
    lg2::info("Created session: ID={SID}", "SID", sid);
    return sid;
}

bool Manager::activateSession(uint32_t sessionId)
{
    auto s = getSession(sessionId);
    if (!s) return false;
    s->isActive = true;
    s->lastActive = std::chrono::steady_clock::now();
    lg2::info("Activated session: ID={SID}", "SID", sessionId);
    return true;
}

bool Manager::closeSession(uint32_t sessionId)
{
    auto s = getSession(sessionId);
    if (!s) return false;
    s->isActive = false;
    lg2::info("Closed session: ID={SID}", "SID", sessionId);
    return true;
}

std::shared_ptr<Session> Manager::getSession(uint32_t sessionId)
{
    auto it = sessions_.find(sessionId);
    if (it == sessions_.end()) return nullptr;
    return it->second;
}

void Manager::checkTimeouts(int timeoutSeconds)
{
    auto now = std::chrono::steady_clock::now();
    auto it = sessions_.begin();
    while (it != sessions_.end())
    {
        auto s = it->second;
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
            now - s->lastActive).count();
        if (elapsed > timeoutSeconds)
        {
            lg2::info("Session timed out: ID={SID}, idle={TIMEOUT}s",
                       "SID", it->first, "TIMEOUT", elapsed);
            it = sessions_.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

size_t Manager::getActiveSessionCount() const
{
    size_t count = 0;
    for (const auto& kv : sessions_)
        if (kv.second && kv.second->isActive)
            ++count;
    return count;
}

} // namespace session
} // namespace serialipmi
