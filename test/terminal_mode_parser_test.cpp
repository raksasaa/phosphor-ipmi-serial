#include <gtest/gtest.h>
#include "../terminal_mode_parser.hpp"

using serialipmi::terminal::Parser;

TEST(TerminalModeParser, ParseSingleNormalFrame) {
    Parser parser;
    std::string frame = "[b0040000]";
    parser.feedData(std::vector<uint8_t>(frame.begin(), frame.end()));
    ASSERT_TRUE(parser.hasCompleteMessage());
    auto payload = parser.getNextMessagePayload();
    ASSERT_EQ(payload.size(), 4u);
    EXPECT_EQ(payload[0], 0xb0);
    EXPECT_EQ(payload[1], 0x04);
    EXPECT_EQ(payload[2], 0x00);
    EXPECT_EQ(payload[3], 0x00);
}

TEST(TerminalModeParser, ParseSpacedFrame) {
    Parser parser;
    std::string frame = "[b0 04 00 00]";
    parser.feedData(std::vector<uint8_t>(frame.begin(), frame.end()));
    ASSERT_TRUE(parser.hasCompleteMessage());
    auto payload = parser.getNextMessagePayload();
    ASSERT_EQ(payload.size(), 4u);
    EXPECT_EQ(payload[0], 0xb0);
    EXPECT_EQ(payload[1], 0x04);
    EXPECT_EQ(payload[2], 0x00);
    EXPECT_EQ(payload[3], 0x00);
}

TEST(TerminalModeParser, ParseMultipleConsecutiveFrames) {
    Parser parser;
    std::string frames = "[b0040000]\r\n[b0080000]\r\n";
    parser.feedData(std::vector<uint8_t>(frames.begin(), frames.end()));
    ASSERT_TRUE(parser.hasCompleteMessage());
    auto msg1 = parser.getNextMessagePayload();
    ASSERT_EQ(msg1.size(), 4u);
    EXPECT_EQ(msg1[1], 0x04);
    ASSERT_TRUE(parser.hasCompleteMessage());
    auto msg2 = parser.getNextMessagePayload();
    ASSERT_EQ(msg2.size(), 4u);
    EXPECT_EQ(msg2[1], 0x08);
}

TEST(TerminalModeParser, CrossReadFrameStitching) {
    Parser parser;
    std::string part1 = "[b004";
    std::string part2 = "0000]\r\n";
    parser.feedData(std::vector<uint8_t>(part1.begin(), part1.end()));
    EXPECT_FALSE(parser.hasCompleteMessage());
    parser.feedData(std::vector<uint8_t>(part2.begin(), part2.end()));
    ASSERT_TRUE(parser.hasCompleteMessage());
    auto payload = parser.getNextMessagePayload();
    ASSERT_EQ(payload.size(), 4u);
    EXPECT_EQ(payload[0], 0xb0);
    EXPECT_EQ(payload[1], 0x04);
}

TEST(TerminalModeParser, InvalidHexChars) {
    Parser parser;
    std::string frame = "[ZZZZ]";
    parser.feedData(std::vector<uint8_t>(frame.begin(), frame.end()));
    EXPECT_FALSE(parser.hasCompleteMessage());
    EXPECT_GT(parser.stats.framesError, 0u);
}

TEST(TerminalModeParser, FrameOverflow) {
    Parser parser;
    std::string frame = "[";
    for (int i = 0; i < 200; i++) {
        frame += "a";
    }
    frame += "]";
    parser.feedData(std::vector<uint8_t>(frame.begin(), frame.end()));
    EXPECT_FALSE(parser.hasCompleteMessage());
    EXPECT_GT(parser.stats.framesError, 0u);
}

TEST(TerminalModeParser, EncodeResponse) {
    Parser parser;
    std::vector<uint8_t> payload = {0x1c, 0x00, 0x01, 0x00};
    auto encoded = parser.encodeResponse(payload);
    EXPECT_EQ(encoded, "[1C 00 01 00]\r\n");
}

TEST(TerminalModeParser, EncodeError) {
    auto err = Parser::encodeError(0x01);
    EXPECT_EQ(err, "[ERR 01]\r\n");
}

TEST(TerminalModeParser, MixedDataWithGarbage) {
    Parser parser;
    std::string data = "garbage[b0040000]morejunk[b0080000]";
    parser.feedData(std::vector<uint8_t>(data.begin(), data.end()));
    ASSERT_TRUE(parser.hasCompleteMessage());
    auto msg1 = parser.getNextMessagePayload();
    ASSERT_EQ(msg1.size(), 4u);
    EXPECT_EQ(msg1[1], 0x04);
    ASSERT_TRUE(parser.hasCompleteMessage());
    auto msg2 = parser.getNextMessagePayload();
    ASSERT_EQ(msg2.size(), 4u);
    EXPECT_EQ(msg2[1], 0x08);
}

TEST(TerminalModeParser, EmptyFrame) {
    Parser parser;
    std::string frame = "[]";
    parser.feedData(std::vector<uint8_t>(frame.begin(), frame.end()));
    EXPECT_FALSE(parser.hasCompleteMessage());
}
