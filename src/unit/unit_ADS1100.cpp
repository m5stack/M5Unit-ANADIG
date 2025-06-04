/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file unit_ADS1100.cpp
  @brief ADS1100 Unit for M5UnitUnified
*/
#include "unit_ADS1100.hpp"
#include <M5Utility.hpp>

using namespace m5::utility::mmh3;
using namespace m5::unit::types;
using namespace m5::unit::ads1100;

namespace {
constexpr uint32_t interval_table[] = {
    1000 / 128 + 1,
    1000 / 32 + 1,
    1000 / 16 + 1,
    1000 / 8,
};

}  // namespace

namespace m5 {
namespace unit {

const char UnitADS1100::name[] = "UnitADS1100";
const types::uid_t UnitADS1100::uid{"UnitADS1100"_mmh3};
const types::attr_t UnitADS1100::attr{attribute::AccessI2C};

bool UnitADS1100::begin()
{
    _vdd    = _cfg.vdd;
    _factor = _cfg.factor;
    return UnitADS11XX::begin() && _cfg.start_periodic ? startPeriodicMeasurement(_cfg.sampling_rate, _cfg.pga)
                                                       : stopPeriodicMeasurement();
}

bool UnitADS1100::start_periodic_measurement(const ads1100::Sampling rate, const ads1100::PGA pga)
{
    Config c{};
    c.rate(m5::stl::to_underlying(rate));
    c.pga(pga);
    return UnitADS11XX::start_periodic_measurement(c.value);
}

bool UnitADS1100::measureSingleshot(ads1100::Data& data, const ads1100::Sampling rate, const ads1100::PGA pga)
{
    Config c{};
    c.rate(m5::stl::to_underlying(rate));
    c.pga(pga);
    return measure_singleshot(data, c.value);
}

bool UnitADS1100::generalReset()
{
    if (UnitADS11XX::generalReset()) {
        return stop_periodic_measurement();
    }
    return false;
}

bool UnitADS1100::readSamplingRate(ads1100::Sampling& rate)
{
    Config c{};
    if (read_config(c.value)) {
        rate = static_cast<Sampling>(c.rate());
        return true;
    }
    return false;
}

bool UnitADS1100::writeSamplingRate(const ads1100::Sampling rate)
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

uint32_t UnitADS1100::get_interval(const uint8_t rate)
{
    return interval_table[rate & 0x03];
}

bool UnitADS1100::read_if_ready_in_periodic(uint8_t v[2])
{
    // ADS1100 don't have data ready status for periodic
    return /*is_data_ready() && */ read_measurement(v);
}

}  // namespace unit
}  // namespace m5
