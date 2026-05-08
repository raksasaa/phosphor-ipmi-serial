#include <gtest/gtest.h>
#include "../sessions_manager.hpp"

using serialipmi::session::Manager;
using serialipmi::session::Session;

TEST(SessionManager, CreateSessionReturnsValidId) {
    Manager::get().managerInit("test_channel");
    uint32_t sid = Manager::get().createSession();
    EXPECT_GT(sid, 0u);
}

TEST(SessionManager, ActivateSessionSetsActive) {
    Manager::get().managerInit("test_channel");
    uint32_t sid = Manager::get().createSession();
    EXPECT_TRUE(Manager::get().activateSession(sid));
    auto s = Manager::get().getSession(sid);
    ASSERT_NE(s, nullptr);
    EXPECT_TRUE(s->isActive);
}

TEST(SessionManager, CloseSessionDeactivates) {
    Manager::get().managerInit("test_channel");
    uint32_t sid = Manager::get().createSession();
    Manager::get().activateSession(sid);
    EXPECT_TRUE(Manager::get().closeSession(sid));
    auto s = Manager::get().getSession(sid);
    ASSERT_NE(s, nullptr);
    EXPECT_FALSE(s->isActive);
}

TEST(SessionManager, GetSessionReturnsNullForInvalidId) {
    Manager::get().managerInit("test_channel");
    auto s = Manager::get().getSession(9999);
    EXPECT_EQ(s, nullptr);
}

TEST(SessionManager, CheckTimeoutsClosesInactive) {
    Manager::get().managerInit("test_channel");
    uint32_t sid = Manager::get().createSession();
    Manager::get().activateSession(sid);
    Manager::get().checkTimeouts(0);
    auto s = Manager::get().getSession(sid);
    ASSERT_NE(s, nullptr);
    EXPECT_FALSE(s->isActive);
}

TEST(SessionManager, ActiveSessionCount) {
    Manager::get().managerInit("test_channel");
    EXPECT_EQ(Manager::get().getActiveSessionCount(), 0u);
    uint32_t sid = Manager::get().createSession();
    EXPECT_EQ(Manager::get().getActiveSessionCount(), 1u);
    Manager::get().closeSession(sid);
    EXPECT_EQ(Manager::get().getActiveSessionCount(), 0u);
}

TEST(SessionManager, MaxSessionsEnforced) {
    Manager::get().managerInit("test_channel");
    uint32_t sid1 = Manager::get().createSession();
    EXPECT_GT(sid1, 0u);
    uint32_t sid2 = Manager::get().createSession();
    EXPECT_EQ(sid2, 0u);
}

TEST(SessionManager, SessionTimeoutKeepsActive) {
    Manager::get().managerInit("test_channel");
    uint32_t sid = Manager::get().createSession();
    Manager::get().activateSession(sid);
    Manager::get().checkTimeouts(9999);
    auto s = Manager::get().getSession(sid);
    ASSERT_NE(s, nullptr);
    EXPECT_TRUE(s->isActive);
}
