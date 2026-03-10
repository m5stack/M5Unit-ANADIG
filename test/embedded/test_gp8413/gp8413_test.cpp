/*
 * SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
  UnitTest for UnitGP8413
*/
#include <gtest/gtest.h>
#include <Wire.h>
#include <M5Unified.h>
#include <M5UnitUnified.hpp>
#include <googletest/test_template.hpp>
#include <googletest/test_helper.hpp>
#include <unit/unit_GP8413.hpp>

using namespace m5::unit::googletest;
using namespace m5::unit;
using namespace m5::unit::gp8413;

#if defined(USING_HAT_DAC2)
namespace hat {
struct I2cPins {
    int sda, scl;
};

I2cPins get_hat_pins(const m5::board_t board)
{
    switch (board) {
        case m5::board_t::board_M5StickC:
        case m5::board_t::board_M5StickCPlus:
        case m5::board_t::board_M5StickCPlus2:
            return {0, 26};
        case m5::board_t::board_M5StickS3:
            return {8, 0};
        case m5::board_t::board_M5StackCoreInk:
            return {25, 26};
        case m5::board_t::board_ArduinoNessoN1:
            return {6, 7};
        default:
            return {-1, -1};
    }
}
}  // namespace hat
#endif

class TestGP8413 : public I2CComponentTestBase<UnitGP8413> {
protected:
#if defined(USING_HAT_DAC2)
    virtual bool begin() override
    {
        auto board      = M5.getBoard();
        const auto pins = hat::get_hat_pins(board);
        // NessoN1: Wire is used by M5Unified In_I2C; use Wire1 for Hat port
        auto& wire = (board == m5::board_t::board_ArduinoNessoN1) ? Wire1 : Wire;
        wire.end();
        wire.begin(pins.sda, pins.scl, unit->component_config().clock);
        return Units.add(*unit, wire) && Units.begin();
    }
#endif
    virtual UnitGP8413* get_instance() override
    {
        auto ptr = new m5::unit::UnitGP8413();
        return ptr;
    }
};

namespace {
constexpr Output output_table[] = {Output::Range5V, Output::Range10V};
constexpr float voltage_table[] = {5000.f, 10000.f};

}  // namespace

TEST_F(TestGP8413, Settings)
{
    SCOPED_TRACE(ustr);

    EXPECT_EQ(unit->range(Channel::Zero), Output::Range10V);
    EXPECT_EQ(unit->range(Channel::One), Output::Range10V);
    EXPECT_EQ(unit->maximumVoltage(Channel::Zero), 10000.f);
    EXPECT_EQ(unit->maximumVoltage(Channel::One), 10000.f);

    for (auto&& or0 : output_table) {
        for (auto&& or1 : output_table) {
            auto s = m5::utility::formatString("OR0:%u OR1:%u", or0, or1);
            SCOPED_TRACE(s);

            EXPECT_TRUE(unit->writeOutputRange(or0, or1));
            EXPECT_EQ(unit->range(Channel::Zero), or0);
            EXPECT_EQ(unit->range(Channel::One), or1);
            EXPECT_EQ(unit->maximumVoltage(Channel::Zero), voltage_table[(uint8_t)or0]);
            EXPECT_EQ(unit->maximumVoltage(Channel::One), voltage_table[(uint8_t)or1]);
        }
    }
}

TEST_F(TestGP8413, Output)
{
    SCOPED_TRACE(ustr);

    EXPECT_TRUE(unit->writeOutputRange(Output::Range5V, Output::Range10V));
    EXPECT_TRUE(unit->writeBothVoltage(10000.f));

    m5::utility::delay(3 * 1000);

    EXPECT_TRUE(unit->writeChannel0Voltage(5000.f));
    EXPECT_TRUE(unit->writeChannel1Voltage(5000.f));

    // Per-channel raw write
    EXPECT_TRUE(unit->writeChannel0Voltage((uint16_t)0x0000));
    EXPECT_TRUE(unit->writeChannel1Voltage((uint16_t)0x0000));
    EXPECT_TRUE(unit->writeChannel0Voltage((uint16_t)UnitGP8413::RESOLUTION));
    EXPECT_TRUE(unit->writeChannel1Voltage((uint16_t)UnitGP8413::RESOLUTION));

    // writeBothVoltage with raw
    EXPECT_TRUE(unit->writeBothVoltage((uint16_t)0x4000, (uint16_t)0x2000));
    EXPECT_TRUE(unit->writeBothVoltage((uint16_t)0x0000));
}

TEST_F(TestGP8413, Boundary)
{
    SCOPED_TRACE(ustr);

    // Negative voltage rejected
    EXPECT_FALSE(unit->writeChannel0Voltage(-1.0f));
    EXPECT_FALSE(unit->writeChannel1Voltage(-1.0f));
    EXPECT_FALSE(unit->writeBothVoltage(-1.0f));

    // Over-range voltage clamped (not rejected)
    for (auto&& or0 : output_table) {
        for (auto&& or1 : output_table) {
            auto s = m5::utility::formatString("OR0:%u OR1:%u", or0, or1);
            SCOPED_TRACE(s);

            EXPECT_TRUE(unit->writeOutputRange(or0, or1));
            float max0 = unit->maximumVoltage(Channel::Zero);
            float max1 = unit->maximumVoltage(Channel::One);

            // Voltage at maximum succeeds
            EXPECT_TRUE(unit->writeChannel0Voltage(max0));
            EXPECT_TRUE(unit->writeChannel1Voltage(max1));

            // Voltage above maximum is clamped (still succeeds)
            EXPECT_TRUE(unit->writeChannel0Voltage(max0 + 1000.f));
            EXPECT_TRUE(unit->writeChannel1Voltage(max1 + 1000.f));
        }
    }

    // Raw value masking (values > RESOLUTION are masked to 15 bits)
    EXPECT_TRUE(unit->writeChannel0Voltage((uint16_t)0xFFFF));
    EXPECT_TRUE(unit->writeChannel1Voltage((uint16_t)0xFFFF));
    EXPECT_TRUE(unit->writeBothVoltage((uint16_t)0xFFFF));
}

TEST_F(TestGP8413, Store)
{
    SCOPED_TRACE(ustr);
    auto start_at = m5::utility::millis();
    EXPECT_TRUE(unit->storeBothVoltage());
    auto duration = m5::utility::millis() - start_at;
    EXPECT_GE(duration, 7);  // Need wait at least 7ms for store
}
