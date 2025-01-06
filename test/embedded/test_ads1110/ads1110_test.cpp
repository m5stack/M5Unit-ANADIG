/*
 * SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
  UnitTest for UnitADS1110
*/
#include <gtest/gtest.h>
#include <Wire.h>
#include <M5Unified.h>
#include <M5UnitUnified.hpp>
#include <googletest/test_template.hpp>
#include <googletest/test_helper.hpp>
#include <unit/unit_ADS1110.hpp>
#include <cmath>
#include <random>

using namespace m5::unit::googletest;
using namespace m5::unit;
using namespace m5::unit::ads1110;
using m5::unit::types::elapsed_time_t;

const ::testing::Environment* global_fixture = ::testing::AddGlobalTestEnvironment(new GlobalFixture<400000U>());

constexpr uint32_t STORED_SIZE{8};

class TestADS1110 : public ComponentTestBase<UnitADS1110, bool> {
protected:
    virtual UnitADS1110* get_instance() override
    {
        auto ptr         = new m5::unit::UnitADS1110();
        auto ccfg        = ptr->component_config();
        ccfg.stored_size = STORED_SIZE;
        ptr->component_config(ccfg);
        return ptr;

        return ptr;
    }
    virtual bool is_using_hal() const override
    {
        return GetParam();
    };
};

// INSTANTIATE_TEST_SUITE_P(ParamValues, TestADS1110, ::testing::Values(false, true));
// INSTANTIATE_TEST_SUITE_P(ParamValues, TestADS1110, ::testing::Values(true));
INSTANTIATE_TEST_SUITE_P(ParamValues, TestADS1110, ::testing::Values(false));

namespace {
constexpr Sampling rate_table[] = {
    Sampling::Rate240,
    Sampling::Rate60,
    Sampling::Rate30,
    Sampling::Rate15,
};
constexpr PGA pga_table[] = {PGA::Gain1, PGA::Gain2, PGA::Gain4, PGA::Gain8};

constexpr uint32_t interval_table[] = {1000 / 240, 1000 / 60, 1000 / 30, 1000 / 15};

template <class U>
elapsed_time_t test_periodic(U* unit, const uint32_t times, const uint32_t measure_duration = 0)
{
    auto tm         = unit->interval();
    auto timeout_at = m5::utility::millis() + 10 * 1000;

    do {
        unit->update();
        if (unit->updated()) {
            break;
        }
        std::this_thread::yield();
    } while (!unit->updated() && m5::utility::millis() <= timeout_at);
    // timeout
    if (!unit->updated()) {
        return 0;
    }

    //
    uint32_t measured{};
    auto start_at = m5::utility::millis();
    timeout_at    = start_at + (times * (tm + measure_duration) * 2);

    do {
        unit->update();
        measured += unit->updated() ? 1 : 0;
        if (measured >= times) {
            break;
        }
        std::this_thread::yield();

    } while (measured < times && m5::utility::millis() <= timeout_at);
    return (measured == times) ? m5::utility::millis() - start_at : 0;
    //   return (measured == times) ? unit->updatedMillis() - start_at : 0;
}

}  // namespace

TEST_P(TestADS1110, Settings)
{
    SCOPED_TRACE(ustr);

    EXPECT_TRUE(unit->inPeriodic());

    // Faild in periodic
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

TEST_P(TestADS1110, Reset)
{
    SCOPED_TRACE(ustr);

    EXPECT_TRUE(unit->stopPeriodicMeasurement());
    EXPECT_FALSE(unit->inPeriodic());

    EXPECT_TRUE(unit->writeSamplingRate(Sampling::Rate60));
    EXPECT_TRUE(unit->writePGA(PGA::Gain4));

    EXPECT_TRUE(unit->generalReset());

    EXPECT_FALSE(unit->inPeriodic());
    Sampling sr{};
    EXPECT_TRUE(unit->readSamplingRate(sr));
    EXPECT_EQ(sr, Sampling::Rate15);
    PGA pga{};
    EXPECT_TRUE(unit->readPGA(pga));
    EXPECT_EQ(pga, PGA::Gain1);

    //
    EXPECT_TRUE(unit->writeSamplingRate(Sampling::Rate60));
    EXPECT_TRUE(unit->writePGA(PGA::Gain4));
    EXPECT_TRUE(unit->startPeriodicMeasurement());

    EXPECT_TRUE(unit->generalReset());

    EXPECT_FALSE(unit->inPeriodic());
    EXPECT_TRUE(unit->readSamplingRate(sr));
    EXPECT_EQ(sr, Sampling::Rate15);
    EXPECT_TRUE(unit->readPGA(pga));
    EXPECT_EQ(pga, PGA::Gain1);
}

TEST_P(TestADS1110, Singleshot)
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

TEST_P(TestADS1110, Periodic)
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

            auto tm      = interval_table[m5::stl::to_underlying(r)];
            auto elapsed = test_periodic(unit.get(), STORED_SIZE, tm);

            EXPECT_TRUE(unit->stopPeriodicMeasurement());
            EXPECT_FALSE(unit->inPeriodic());

            EXPECT_NE(elapsed, 0);
            EXPECT_GE(elapsed, STORED_SIZE * tm);

            // M5_LOGI("TM:%u IT:%u e:%ld", tm, unit->interval(), elapsed);
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
