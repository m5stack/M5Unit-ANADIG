/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
  UnitTest for UnitADS1100
*/
#include <gtest/gtest.h>
#include <Wire.h>
#include <M5Unified.h>
#include <M5UnitUnified.hpp>
#include <googletest/test_template.hpp>
#include <googletest/test_helper.hpp>
#include <m5_unit_component/adapter_i2c.hpp>
#include <unit/unit_ADS1100.hpp>
#include <cmath>

using namespace m5::unit::googletest;
using namespace m5::unit;
using namespace m5::unit::ads1100;

constexpr uint32_t STORED_SIZE{8};

#if defined(USING_HAT_ADC) || defined(USING_HAT_ADC11)
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

class TestADS1100 : public I2CComponentTestBase<UnitADS1100> {
protected:
#if defined(USING_HAT_ADC) || defined(USING_HAT_ADC11)
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
    virtual UnitADS1100* get_instance() override
    {
        auto ptr         = new m5::unit::UnitADS1100();
        auto ccfg        = ptr->component_config();
        ccfg.stored_size = STORED_SIZE;
        ptr->component_config(ccfg);
        return ptr;
    }
};

namespace {
constexpr Sampling rate_table[] = {
    Sampling::Rate128,
    Sampling::Rate32,
    Sampling::Rate16,
    Sampling::Rate8,
};
constexpr PGA pga_table[] = {PGA::Gain1, PGA::Gain2, PGA::Gain4, PGA::Gain8};

}  // namespace

TEST_F(TestADS1100, Settings)
{
    SCOPED_TRACE(ustr);

    EXPECT_TRUE(unit->inPeriodic());

    // Failed in periodic
    for (auto&& r : rate_table) {
        EXPECT_FALSE(unit->writeSamplingRate(r));
    }
    for (auto&& p : pga_table) {
        EXPECT_FALSE(unit->writePGA(p));
    }

    EXPECT_TRUE(unit->stopPeriodicMeasurement());
    EXPECT_FALSE(unit->inPeriodic());

    for (auto&& r : rate_table) {
        EXPECT_TRUE(unit->writeSamplingRate(r));
        Sampling sr{};
        EXPECT_TRUE(unit->readSamplingRate(sr));
        EXPECT_EQ(sr, r);
    }
    for (auto&& p : pga_table) {
        EXPECT_TRUE(unit->writePGA(p));
        PGA pga{};
        EXPECT_TRUE(unit->readPGA(pga));
        EXPECT_EQ(pga, p);
    }
}

TEST_F(TestADS1100, Reset)
{
    SCOPED_TRACE(ustr);

    // I2C_Class hangs on generalReset (bus stuck after general call reset)
    auto ad          = unit->asAdapter<m5::unit::AdapterI2C>(m5::unit::Adapter::Type::I2C);
    bool is_i2cclass = ad && ad->implType() == m5::unit::AdapterI2C::ImplType::I2CClass;
    if (is_i2cclass) {
        M5_LOGW("Skip Reset: I2C_Class does not recover from general call reset");
        GTEST_SKIP();
    }

    EXPECT_TRUE(unit->stopPeriodicMeasurement());
    EXPECT_FALSE(unit->inPeriodic());

    EXPECT_TRUE(unit->writeSamplingRate(Sampling::Rate32));
    EXPECT_TRUE(unit->writePGA(PGA::Gain4));

    EXPECT_TRUE(unit->generalReset());

    EXPECT_FALSE(unit->inPeriodic());
    Sampling sr{};
    EXPECT_TRUE(unit->readSamplingRate(sr));
    EXPECT_EQ(sr, Sampling::Rate8);
    PGA pga{};
    EXPECT_TRUE(unit->readPGA(pga));
    EXPECT_EQ(pga, PGA::Gain1);

    //
    EXPECT_TRUE(unit->writeSamplingRate(Sampling::Rate32));
    EXPECT_TRUE(unit->writePGA(PGA::Gain4));
    EXPECT_TRUE(unit->startPeriodicMeasurement());

    EXPECT_TRUE(unit->generalReset());

    EXPECT_FALSE(unit->inPeriodic());
    EXPECT_TRUE(unit->readSamplingRate(sr));
    EXPECT_EQ(sr, Sampling::Rate8);
    EXPECT_TRUE(unit->readPGA(pga));
    EXPECT_EQ(pga, PGA::Gain1);
}

TEST_F(TestADS1100, Singleshot)
{
    SCOPED_TRACE(ustr);
    Data d{};

    EXPECT_FALSE(unit->measureSingleshot(d));
    EXPECT_TRUE(unit->stopPeriodicMeasurement());
    EXPECT_FALSE(unit->inPeriodic());

    for (auto&& r : rate_table) {
        for (auto&& p : pga_table) {
            auto s = m5::utility::formatString("Rate:%u PGA:%u", r, p);
            SCOPED_TRACE(s);

            uint32_t cnt{8};
            while (cnt--) {
                EXPECT_TRUE(unit->measureSingleshot(d, r, p));
                EXPECT_TRUE(std::isfinite(d.differentialVoltage()));
            }
        }
    }
}

TEST_F(TestADS1100, Periodic)
{
    SCOPED_TRACE(ustr);

    EXPECT_TRUE(unit->inPeriodic());
    EXPECT_FALSE(unit->startPeriodicMeasurement());
    EXPECT_TRUE(unit->stopPeriodicMeasurement());
    EXPECT_FALSE(unit->inPeriodic());

    for (auto&& r : rate_table) {
        for (auto&& p : pga_table) {
            auto s = m5::utility::formatString("Rate:%u PGA:%u", r, p);
            SCOPED_TRACE(s);

            EXPECT_TRUE(unit->startPeriodicMeasurement(r, p));
            EXPECT_TRUE(unit->inPeriodic());

            auto result = collect_periodic_measurements(unit.get(), STORED_SIZE, unit->interval() * STORED_SIZE * 3);

            EXPECT_TRUE(unit->stopPeriodicMeasurement());
            EXPECT_FALSE(unit->inPeriodic());

            EXPECT_FALSE(result.timed_out);
            EXPECT_EQ(result.update_count, STORED_SIZE);
            EXPECT_LE(result.median(), result.expected_interval + result.expected_interval * 15 / 100 + 1);

            //
            EXPECT_EQ(unit->available(), STORED_SIZE);
            EXPECT_FALSE(unit->empty());
            EXPECT_TRUE(unit->full());

            uint32_t cnt{STORED_SIZE / 2};
            while (cnt-- && unit->available()) {
                EXPECT_TRUE(std::isfinite(unit->differentialVoltage()));
                EXPECT_EQ(unit->differentialValue(), unit->oldest().differentialValue());
                EXPECT_FLOAT_EQ(unit->differentialVoltage(), unit->oldest().differentialVoltage());
                EXPECT_FALSE(unit->empty());
                unit->discard();
            }
            EXPECT_EQ(unit->available(), STORED_SIZE / 2);
            EXPECT_FALSE(unit->empty());
            EXPECT_FALSE(unit->full());

            unit->flush();
            EXPECT_EQ(unit->available(), 0);
            EXPECT_TRUE(unit->empty());
            EXPECT_FALSE(unit->full());

            EXPECT_FALSE(std::isfinite(unit->differentialVoltage()));
        }
    }
}
