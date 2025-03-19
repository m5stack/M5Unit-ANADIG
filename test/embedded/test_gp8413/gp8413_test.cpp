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
#include <cmath>
#include <random>

using namespace m5::unit::googletest;
using namespace m5::unit;
using namespace m5::unit::gp8413;

const ::testing::Environment* global_fixture = ::testing::AddGlobalTestEnvironment(new GlobalFixture<400000U>());

class TestGP8413 : public ComponentTestBase<UnitGP8413, bool> {
protected:
    virtual UnitGP8413* get_instance() override
    {
        auto ptr = new m5::unit::UnitGP8413();
        return ptr;
    }
    virtual bool is_using_hal() const override
    {
        return GetParam();
    };
};

// INSTANTIATE_TEST_SUITE_P(ParamValues, TestGP8413, ::testing::Values(false, true));
// INSTANTIATE_TEST_SUITE_P(ParamValues, TestGP8413, ::testing::Values(true));
INSTANTIATE_TEST_SUITE_P(ParamValues, TestGP8413, ::testing::Values(false));

namespace {
constexpr Output output_table[] = {Output::Range5V, Output::Range10V};
constexpr float voltage_table[] = {5000.f, 10000.f};

}  // namespace

TEST_P(TestGP8413, Settings)
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

TEST_P(TestGP8413, Output)
{
    SCOPED_TRACE(ustr);

    EXPECT_TRUE(unit->writeOutputRange(Output::Range5V, Output::Range10V));
    EXPECT_TRUE(unit->writeBothVoltage(10000.f));

    m5::utility::delay(3 * 1000);

    EXPECT_TRUE(unit->writeChannel0Voltage(5000.f));
    EXPECT_TRUE(unit->writeChannel1Voltage(5000.f));
}

TEST_P(TestGP8413, Store)
{
    SCOPED_TRACE(ustr);
    auto start_at = m5::utility::millis();
    EXPECT_TRUE(unit->storeBothVoltage());
    auto duration = m5::utility::millis() - start_at;
    EXPECT_GE(duration, 7);  // Need wait at least 7ms for store
}
