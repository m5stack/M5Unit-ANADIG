/*
 * SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
  UnitTest for UnitMCP4725
*/
#include <gtest/gtest.h>
#include <Wire.h>
#include <M5Unified.h>
#include <M5UnitUnified.hpp>
#include <googletest/test_template.hpp>
#include <googletest/test_helper.hpp>
#include <m5_unit_component/adapter_i2c.hpp>
#include <unit/unit_MCP4725.hpp>

using namespace m5::unit::googletest;
using namespace m5::unit;
using namespace m5::unit::mcp4725;

class TestMCP4725 : public I2CComponentTestBase<UnitMCP4725> {
protected:
    virtual UnitMCP4725* get_instance() override
    {
        auto ptr = new m5::unit::UnitMCP4725();
        return ptr;
    }
};

namespace {
constexpr PowerDown pd_table[] = {PowerDown::OHM_1K, PowerDown::OHM_100K, PowerDown::OHM_500K, PowerDown::Normal};

}  // namespace

TEST_F(TestMCP4725, Settings)
{
    SCOPED_TRACE(ustr);

    auto ad          = unit->asAdapter<m5::unit::AdapterI2C>(m5::unit::Adapter::Type::I2C);
    bool is_i2cclass = ad && ad->implType() == m5::unit::AdapterI2C::ImplType::I2CClass;

    EXPECT_EQ(unit->lastValue(), 0U);
    EXPECT_EQ(unit->powerDown(), PowerDown::Normal);

    // only DAC
    for (auto&& pd : pd_table) {
        EXPECT_TRUE(unit->writePowerDown(pd));

        EXPECT_EQ(unit->lastValue(), 0U);
        EXPECT_EQ(unit->powerDown(), pd);

        PowerDown pwd{};
        uint16_t raw{};
        EXPECT_TRUE(unit->readDACRegister(pwd, raw));
        EXPECT_EQ(pwd, pd);
        EXPECT_EQ(raw, 0U);
    }

    // DAC and EEPROM
    for (auto&& pd : pd_table) {
        EXPECT_TRUE(unit->writePowerDown(pd));
        EXPECT_TRUE(unit->writeVoltageAndEEPROM(unit->lastValue()));

        EXPECT_EQ(unit->lastValue(), 0U);
        EXPECT_EQ(unit->powerDown(), pd);

        PowerDown pwd{};
        uint16_t raw{};
        EXPECT_TRUE(unit->readDACRegister(pwd, raw));
        EXPECT_EQ(pwd, pd);
        EXPECT_EQ(raw, 0U);
        EXPECT_TRUE(unit->readEEPROM(pwd, raw));
        EXPECT_EQ(pwd, pd);
        EXPECT_EQ(raw, 0U);

        // I2C_Class hangs on generalReset (bus stuck after general call reset)
        if (!is_i2cclass) {
            // reset
            uint8_t v = (uint8_t)pd;
            v         = (v + 1) & 0x03;
            EXPECT_TRUE(unit->writePowerDown((PowerDown)v));
            EXPECT_TRUE(unit->writeVoltage(100U));

            EXPECT_TRUE(unit->readDACRegister(pwd, raw));
            EXPECT_EQ(pwd, (PowerDown)v);
            EXPECT_EQ(raw, 100U);

            EXPECT_TRUE(unit->generalReset());

            EXPECT_TRUE(unit->readDACRegister(pwd, raw));
            EXPECT_EQ(pwd, pd);
            EXPECT_EQ(raw, 0U);
            EXPECT_TRUE(unit->readEEPROM(pwd, raw));
            EXPECT_EQ(pwd, pd);
            EXPECT_EQ(raw, 0U);
        }
    }
}

TEST_F(TestMCP4725, Output)
{
    SCOPED_TRACE(ustr);

    EXPECT_EQ(unit->lastValue(), 0U);
    EXPECT_EQ(unit->powerDown(), PowerDown::Normal);
    EXPECT_TRUE(unit->writeVoltageAndEEPROM(0U));

    auto supply_voltage = unit->config().supply_voltage;

    PowerDown pwd{};
    uint16_t raw{};
    const float near = UnitMCP4725::MAXIMUM_VOLTAGE / UnitMCP4725::RESOLUTION;

    //
    EXPECT_TRUE(unit->writeVoltage(1234.56f));
    EXPECT_TRUE(unit->readDACRegister(pwd, raw));
    // M5_LOGI("%f", raw_to_voltage(raw, supply_voltage));
    EXPECT_NEAR(UnitMCP4725::raw_to_voltage(raw, supply_voltage), 1234.56f, near);

    EXPECT_TRUE(unit->writeVoltage(3333.33f));
    EXPECT_TRUE(unit->readDACRegister(pwd, raw));
    EXPECT_EQ(raw, UnitMCP4725::voltage_to_raw(UnitMCP4725::MAXIMUM_VOLTAGE, supply_voltage));

    EXPECT_FALSE(unit->writeVoltage(-1234.56f));

    //
    EXPECT_TRUE(unit->writeVoltageAndEEPROM(1234.56f));
    EXPECT_TRUE(unit->readDACRegister(pwd, raw));
    EXPECT_NEAR(UnitMCP4725::raw_to_voltage(raw, supply_voltage), 1234.56f, near);
    EXPECT_TRUE(unit->readEEPROM(pwd, raw));
    EXPECT_NEAR(UnitMCP4725::raw_to_voltage(raw, supply_voltage), 1234.56f, near);

    EXPECT_TRUE(unit->writeVoltageAndEEPROM(3333.33f));
    EXPECT_TRUE(unit->readDACRegister(pwd, raw));
    EXPECT_EQ(raw, UnitMCP4725::voltage_to_raw(UnitMCP4725::MAXIMUM_VOLTAGE, supply_voltage));
    EXPECT_TRUE(unit->readEEPROM(pwd, raw));
    EXPECT_EQ(raw, UnitMCP4725::voltage_to_raw(UnitMCP4725::MAXIMUM_VOLTAGE, supply_voltage));

    EXPECT_FALSE(unit->writeVoltageAndEEPROM(-1234.56f));

    //
    EXPECT_EQ(unit->powerDown(), PowerDown::Normal);
    EXPECT_TRUE(unit->writeVoltageAndEEPROM(0U));
}
