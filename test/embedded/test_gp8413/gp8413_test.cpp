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
#include <m5_unit_component/adapter_i2c.hpp>
#include <unit/unit_GP8413.hpp>
#include <wiring/m5_unit_unified_wiring.hpp>

using namespace m5::unit::googletest;
using namespace m5::unit;
using namespace m5::unit::gp8413;

class TestGP8413 : public I2CComponentTestBase<UnitGP8413> {
protected:
#if defined(USING_HAT_DAC2)
    virtual bool begin() override
    {
        // Board-aware Hat I2C (StickC series Wire / NessoN1 Wire1)
        return m5::unit::wiring::addHatI2C(Units, *unit) && Units.begin();
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
            auto s = m5::utility::formatString("OR0:%u OR1:%u", (unsigned)or0, (unsigned)or1);
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
            auto s = m5::utility::formatString("OR0:%u OR1:%u", (unsigned)or0, (unsigned)or1);
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

    // The store sequence is bit-banged on SDA/SCL, so it is supported only on buses that can be released
    // and whose pins are known (see storeBothVoltage())
    auto ad              = unit->asAdapter<AdapterI2C>(Adapter::Type::I2C);
    const auto type      = ad ? ad->implType() : AdapterI2C::ImplType::Unknown;
    const bool supported = ad && ad->sda() >= 0 && ad->scl() >= 0 &&
                           (type == AdapterI2C::ImplType::TwoWire || type == AdapterI2C::ImplType::I2CClass ||
                            type == AdapterI2C::ImplType::Bus
#if defined(ESP_PLATFORM) && __has_include(<driver/i2c_master.h>) && !defined(CONFIG_IDF_TARGET_ESP32P4)
                            || type == AdapterI2C::ImplType::ESPIDFMasterBus
#elif defined(ESP_PLATFORM) && !defined(CONFIG_IDF_TARGET_ESP32P4)
                            || type == AdapterI2C::ImplType::ESPIDFLegacyBus
#endif
                           );
    M5_LOGI("Store on impl:%u SDA:%d SCL:%d", (unsigned)type, ad ? ad->sda() : -1, ad ? ad->scl() : -1);

    // Store 0V so that the unit outputs 0V at the next power-on
    EXPECT_TRUE(unit->writeBothVoltage(0.0f));

    const auto start_at = m5::utility::millis();
    const bool stored   = unit->storeBothVoltage();
    const auto duration = m5::utility::elapsedSince(start_at);
    if (supported) {
        EXPECT_TRUE(stored);
        EXPECT_GE(duration, 7U);  // Need wait at least 7ms for store
    } else {
        M5_LOGW("Store is not supported on this bus (impl:%u)", (unsigned)type);
        EXPECT_FALSE(stored);
    }

    // The bus is restored after the store
    EXPECT_TRUE(unit->writeBothVoltage(0.0f));
}
