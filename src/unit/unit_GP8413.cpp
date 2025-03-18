/*
 * SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file unit_GP8413.cpp
  @brief GP8413 Unit for M5UnitUnified
*/
#include "unit_GP8413.hpp"
#include <M5Utility.hpp>

using namespace m5::utility::mmh3;
using namespace m5::unit::types;
using namespace m5::unit::gp8413;
using namespace m5::unit::gp8413::command;

namespace {
constexpr uint8_t channel_reg_table[] = {
    OUTPUT_CHANNEL0_REG,
    OUTPUT_CHANNEL1_REG,
};
constexpr float max_mv_table[] = {
    5000.f,
    10000.f,
};

/*
  **** NOTICE *******************************************************************************************
  The datasheet says that when changing the output voltage range, use 0x00 for 5V and 0x01 for 10V.
  However, using these values requires a bit shift of the output value, and causes oscillation
  (especially channel 1 at 5V).
  With 0x5 (5V) and 0x7 (10V), no bit shift is required, and there is no oscillation.
  This is undocumented behavior.
  *******************************************************************************************************
*/
constexpr uint8_t mode_nibble_table[] = {0x05, 0x07};
}  // namespace

namespace m5 {
namespace unit {

const char UnitGP8413::name[] = "UnitGP8413";
const types::uid_t UnitGP8413::uid{"UnitGP8413"_mmh3};
const types::uid_t UnitGP8413::attr{0};

float UnitGP8413::maximumVoltage(const gp8413::Channel channel) const
{
    return max_mv_table[m5::stl::to_underlying(range(channel))];
}

bool UnitGP8413::begin()
{
    return writeOutputRange(_cfg.range0, _cfg.range1);
}

bool UnitGP8413::writeOutputRange(const gp8413::Output range0, const gp8413::Output range1)
{
    uint8_t v =
        mode_nibble_table[m5::stl::to_underlying(range0)] | (mode_nibble_table[m5::stl::to_underlying(range1)] << 4);
    if (writeRegister8(OUTPUT_RANGE_REG, v)) {
        _range[0] = range0;
        _range[1] = range1;
        return true;
    }
    return false;
}

bool UnitGP8413::writeVoltage(const gp8413::Channel channel, const uint16_t raw)
{
    uint8_t buf[2]{(uint8_t)(raw & 0xFF), (uint8_t)(raw >> 8)};
    return write_voltage(channel_reg_table[m5::stl::to_underlying(channel)], buf, 2);
}

bool UnitGP8413::writeBothVoltage(const uint16_t raw0, const uint16_t raw1)
{
    uint8_t buf[4]{(uint8_t)(raw0 & 0xFF), (uint8_t)(raw0 >> 8), (uint8_t)(raw1 & 0xFF), (uint8_t)(raw1 >> 8)};
    return write_voltage(channel_reg_table[0], buf, 4);
}

uint16_t UnitGP8413::voltage_to_raw(const Channel channel, const float mv)
{
    float maxMv = maximumVoltage(channel);
    float val   = std::fmin(std::fmax(mv, 0.0f), maxMv);
    return static_cast<uint16_t>((val / maxMv) * RESOLUTION);
}

bool UnitGP8413::write_voltage(const uint8_t reg, const uint8_t* buf, const uint32_t len)
{
    return buf && writeRegister(reg, buf, len);
}

}  // namespace unit
}  // namespace m5
