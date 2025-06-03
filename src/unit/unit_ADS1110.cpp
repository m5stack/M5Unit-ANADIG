/*
 * SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file unit_ADS1110.cpp
  @brief ADS1110 Unit for M5UnitUnified
*/
#include "unit_ADS1110.hpp"
#include <M5Utility.hpp>

using namespace m5::utility::mmh3;
using namespace m5::unit::types;
using namespace m5::unit::ads1110;

namespace {
constexpr uint32_t interval_table[] = {
    1000 / 250,
    1000 / 60 + 1,
    1000 / 30 + 1,
    1000 / 15 + 1,
};

}  // namespace

namespace m5 {
namespace unit {

const char UnitADS1110::name[] = "UnitADS1110";
const types::uid_t UnitADS1110::uid{"UnitADS1110"_mmh3};
const types::attr_t UnitADS1110::attr{attribute::AccessI2C};

bool UnitADS1110::begin()
{
    _factor = _cfg.factor;
    return UnitADS11XX::begin() && _cfg.start_periodic ? startPeriodicMeasurement(_cfg.sampling_rate, _cfg.pga)
                                                       : stopPeriodicMeasurement();
}

bool UnitADS1110::start_periodic_measurement(const ads1110::Sampling rate, const ads1110::PGA pga)
{
    Config c{};
    c.rate(m5::stl::to_underlying(rate));
    c.pga(pga);
    return UnitADS11XX::start_periodic_measurement(c.value);
}

bool UnitADS1110::measureSingleshot(ads1110::Data& data, const ads1110::Sampling rate, const ads1110::PGA pga)
{
    Config c{};
    c.rate(m5::stl::to_underlying(rate));
    c.pga(pga);
    return measure_singleshot(data, c.value);
}

bool UnitADS1110::generalReset()
{
    if (UnitADS11XX::generalReset()) {
        return stop_periodic_measurement();
    }
    return false;
}

bool UnitADS1110::readSamplingRate(ads1110::Sampling& rate)
{
    Config c{};
    if (read_config(c.value)) {
        rate = static_cast<Sampling>(c.rate());
        return true;
    }
    return false;
}

bool UnitADS1110::writeSamplingRate(const ads1110::Sampling rate)
{
    if (inPeriodic()) {
        M5_LIB_LOGD("Periodic measurements are running");
        return false;
    }

    Config c{};
    if (read_config(c.value)) {
        c.rate(m5::stl::to_underlying(rate));
        return write_config(c.value);
    }
    return false;
}

uint32_t UnitADS1110::get_interval(const uint8_t rate)
{
    return interval_table[rate & 0x03];
}

}  // namespace unit
}  // namespace m5
