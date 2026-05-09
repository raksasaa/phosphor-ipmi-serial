#include <gtest/gtest.h>
#include "../message_handler.hpp"
#include "../message_parsers.hpp"
#include "../command_table.hpp"
#include "../sessions_manager.hpp"

TEST(MessageHandler, LocalCommandExecution) {
    serialipmi::session::Manager::get().managerInit("test");
    serialipmi::command::Table::get().registerSessionCommands();

    auto mode = serialipmi::parser::ProtocolMode::Terminal;
    auto parser = serialipmi::parser::createParser(mode);

    serialipmi::message::Message inMsg;
    inMsg.netFn = 0x06;
    inMsg.cmd = 0x38;
    inMsg.lun = 0;
    inMsg.data = {0x0E};

    auto handler = std::make_shared<serialipmi::message::Handler>(
        nullptr, parser, "test");

    handler->processIncoming(inMsg);
    auto resp = handler->getResponse();
    ASSERT_TRUE(resp.has_value());
    EXPECT_EQ(resp->netFn, 0x06);
    EXPECT_EQ(resp->cmd, 0x38);
    EXPECT_EQ(resp->completionCode, 0x00);
}

TEST(MessageHandler, NonLocalCommandReturnsNoResponse) {
    serialipmi::session::Manager::get().managerInit("test2");
    serialipmi::command::Table::get().registerSessionCommands();

    auto mode = serialipmi::parser::ProtocolMode::Terminal;
    auto parser = serialipmi::parser::createParser(mode);

    serialipmi::message::Message inMsg;
    inMsg.netFn = 0x06;
    inMsg.cmd = 0x01;
    inMsg.lun = 0;

    auto handler = std::make_shared<serialipmi::message::Handler>(
        nullptr, parser, "test2");

    handler->processIncoming(inMsg);
    auto resp = handler->getResponse();
    EXPECT_FALSE(resp.has_value());
}

TEST(MessageHandler, GetChannelAuthCap) {
    serialipmi::session::Manager::get().managerInit("test3");
    serialipmi::command::Table::get().registerSessionCommands();

    auto mode = serialipmi::parser::ProtocolMode::Terminal;
    auto parser = serialipmi::parser::createParser(mode);

    serialipmi::message::Message inMsg;
    inMsg.netFn = 0x06;
    inMsg.cmd = 0x38;
    inMsg.lun = 0;
    inMsg.data = {0x0E, 0x04};

    auto handler = std::make_shared<serialipmi::message::Handler>(
        nullptr, parser, "test3");

    handler->processIncoming(inMsg);
    auto resp = handler->getResponse();
    ASSERT_TRUE(resp.has_value());
    EXPECT_EQ(resp->data.size(), 8u);
    EXPECT_EQ(resp->data[0], 0x0E);
    EXPECT_EQ(resp->data[1], 0x01);
    EXPECT_EQ(resp->data[2], 0x0C);
}

TEST(MessageHandler, ActivateSession) {
    serialipmi::session::Manager::get().managerInit("test4");
    serialipmi::command::Table::get().registerSessionCommands();

    auto mode = serialipmi::parser::ProtocolMode::Terminal;
    auto parser = serialipmi::parser::createParser(mode);

    serialipmi::message::Message inMsg;
    inMsg.netFn = 0x06;
    inMsg.cmd = 0x3A;
    inMsg.lun = 0;
    inMsg.data = {0x01, 0x00, 0x00, 0x00};

    auto handler = std::make_shared<serialipmi::message::Handler>(
        nullptr, parser, "test4");

    handler->processIncoming(inMsg);
    auto resp = handler->getResponse();
    ASSERT_TRUE(resp.has_value());
    EXPECT_EQ(resp->completionCode, 0x00);
}
