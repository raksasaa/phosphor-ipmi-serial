#include <gtest/gtest.h>
#include "../basic_mode_parser.hpp"

using serialipmi::basic::Parser;

TEST(BasicModeParser, ParseNormalFrameNoEscape) {
    Parser parser;
    std::vector<uint8_t> frame = {
        0xA0,
        0x20, 0x18, 0xC8, 0x81, 0x8C, 0x01, 0xF2,
        0xA5
    };
    parser.feedData(frame);
    ASSERT_TRUE(parser.hasCompleteMessage());
    auto msg = parser.getNextMessage();
    EXPECT_EQ(msg.mode, serialipmi::message::Message::Mode::Basic);
    EXPECT_EQ(msg.rsSA, 0x20);
    EXPECT_EQ(msg.netFn, 0x06);
    EXPECT_EQ(msg.rqSA, 0x81);
    EXPECT_EQ(msg.cmd, 0x01);
}

TEST(BasicModeParser, ParseFrameWithEscapeA0) {
    Parser parser;
    std::vector<uint8_t> frame = {
        0xA0,
        0x20, 0x18, 0xAA, 0xB0, 0x81, 0x8C, 0x01,
        0xA5
    };
    parser.feedData(frame);
    auto msg = parser.getNextMessage();
    ASSERT_GE(msg.rawFrame.size(), 3u);
    EXPECT_EQ(msg.rawFrame[2], 0xA0);
}

TEST(BasicModeParser, AllEscapeSequences) {
    Parser parser;
    std::vector<uint8_t> frame = {
        0xA0,
        0xAA, 0xB0,
        0xAA, 0xB5,
        0xAA, 0xB6,
        0xAA, 0xBA,
        0x00, 0x00,
        0xA5
    };
    parser.feedData(frame);
    ASSERT_TRUE(parser.hasCompleteMessage());
    auto msg = parser.getNextMessage();
    EXPECT_EQ(msg.rawFrame[0], 0xA0);
    EXPECT_EQ(msg.rawFrame[1], 0xA5);
    EXPECT_EQ(msg.rawFrame[2], 0xA6);
    EXPECT_EQ(msg.rawFrame[3], 0xAA);
    EXPECT_EQ(msg.rawFrame[4], 0x00);
    EXPECT_EQ(msg.rawFrame[5], 0x00);
}

TEST(BasicModeParser, ValidChecksum) {
    std::vector<uint8_t> msg = {0x20, 0x18, 0xC8, 0x81, 0x8C, 0x01, 0xF2};
    EXPECT_TRUE(Parser::validateChecksum(msg));
}

TEST(BasicModeParser, InvalidChecksum) {
    std::vector<uint8_t> msg = {0x20, 0x18, 0xC8, 0x81, 0x8C, 0x01, 0xFF};
    EXPECT_FALSE(Parser::validateChecksum(msg));
}

TEST(BasicModeParser, HandshakeGeneration) {
    Parser parser;
    auto hs = parser.encodeHandshake();
    ASSERT_EQ(hs.size(), 1u);
    EXPECT_EQ(hs[0], 0xA6);
}

TEST(BasicModeParser, ResponseEncodingWithEscape) {
    Parser parser;
    std::vector<uint8_t> payload = {0x20, 0xA0, 0x81};
    auto encoded = parser.encodeResponse(payload);
    ASSERT_EQ(encoded.size(), 5u);
    EXPECT_EQ(encoded[0], 0xA0);
    EXPECT_EQ(encoded[1], 0x20);
    EXPECT_EQ(encoded[2], 0xAA);
    EXPECT_EQ(encoded[3], 0xB0);
    EXPECT_EQ(encoded[4], 0xA5);
}

TEST(BasicModeParser, IncompleteFrame) {
    Parser parser;
    std::vector<uint8_t> frame = {0xA0, 0x20, 0x18, 0xC8};
    parser.feedData(frame);
    EXPECT_FALSE(parser.hasCompleteMessage());
}

TEST(BasicModeParser, MultipleConsecutiveFrames) {
    Parser parser;
    std::vector<uint8_t> data = {
        0xA0, 0x20, 0x18, 0xC8, 0x81, 0x8C, 0x01, 0xF2, 0xA5,
        0xA0, 0x20, 0x18, 0xC8, 0x81, 0x8C, 0x02, 0xF1, 0xA5
    };
    parser.feedData(data);
    ASSERT_TRUE(parser.hasCompleteMessage());
    parser.getNextMessage();
    ASSERT_TRUE(parser.hasCompleteMessage());
    parser.getNextMessage();
    EXPECT_FALSE(parser.hasCompleteMessage());
}

TEST(BasicModeParser, NewStartMidFrame) {
    Parser parser;
    std::vector<uint8_t> data = {0xA0, 0x20, 0x18, 0xA0, 0x20, 0x18, 0xC8, 0x81, 0x8C, 0x01, 0xF2, 0xA5};
    parser.feedData(data);
    ASSERT_TRUE(parser.hasCompleteMessage());
    auto msg = parser.getNextMessage();
    EXPECT_EQ(msg.rsSA, 0x20);
}
