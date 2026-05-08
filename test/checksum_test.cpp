#include <gtest/gtest.h>
#include "../basic_mode_parser.hpp"

using serialipmi::basic::Parser;

TEST(Checksum, TwoComplementIdentity) {
    std::vector<uint8_t> bytes = {0x20, 0x18};
    uint8_t cs = Parser::checksum(bytes);
    uint16_t total = 0x20 + 0x18 + cs;
    EXPECT_EQ(total & 0xFF, 0);
}

TEST(Checksum, SingleByte) {
    std::vector<uint8_t> bytes = {0xFF};
    uint8_t cs = Parser::checksum(bytes);
    uint16_t total = 0xFF + cs;
    EXPECT_EQ(total & 0xFF, 0);
}

TEST(Checksum, MultipleBytes) {
    std::vector<uint8_t> bytes = {0x01, 0x02, 0x03, 0x04};
    uint8_t cs = Parser::checksum(bytes);
    uint32_t total = 0x01 + 0x02 + 0x03 + 0x04 + cs;
    EXPECT_EQ(total & 0xFF, 0);
}

TEST(Checksum, AllZeros) {
    std::vector<uint8_t> bytes = {0x00, 0x00, 0x00};
    uint8_t cs = Parser::checksum(bytes);
    EXPECT_EQ(cs, 0);
}

TEST(Checksum, AllFF) {
    std::vector<uint8_t> bytes = {0xFF, 0xFF, 0xFF};
    uint8_t cs = Parser::checksum(bytes);
    uint32_t total = 0xFF + 0xFF + 0xFF + cs;
    EXPECT_EQ(total & 0xFF, 0);
}

TEST(Checksum, FullIPMBValidation) {
    std::vector<uint8_t> msg = {0x20, 0x18, 0xC8, 0x81, 0x8C, 0x01, 0xF2};
    EXPECT_TRUE(Parser::validateChecksum(msg));
}

TEST(Checksum, InvalidFirstChecksum) {
    std::vector<uint8_t> msg = {0x20, 0x18, 0x00, 0x81, 0x8C, 0x01, 0xF2};
    EXPECT_FALSE(Parser::validateChecksum(msg));
}

TEST(Checksum, InvalidSecondChecksum) {
    std::vector<uint8_t> msg = {0x20, 0x18, 0xC8, 0x81, 0x8C, 0x01, 0x00};
    EXPECT_FALSE(Parser::validateChecksum(msg));
}

TEST(Checksum, TooShortMessage) {
    std::vector<uint8_t> msg = {0x20, 0x18};
    EXPECT_FALSE(Parser::validateChecksum(msg));
}
