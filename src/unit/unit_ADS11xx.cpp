/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file unit_ADS11XX.cpp
  @brief ADS11XX Unit for M5UnitUnified
*/
#include "unit_ADS11xx.hpp"
#include <M5Utility.hpp>

using namespace m5::unit::ads11xx;
using namespace m5::utility::mmh3;
using namespace m5::unit::types;

namespace {
constexpr uint8_t DEFAULT_CONFIG_VALUE{0x8C};
}  // namespace

namespace m5 {
namespace unit {

namespace ads11xx {
const int32_t Data::min_code_table[4] = {-2048, -8192, -16384, -32768};
}  // namespace ads11xx

const char UnitADS11XX::name[] = "UnitADS11XX";
const types::uid_t UnitADS11XX::uid{"UnitADS11XX"_mmh3};
const types::attr_t UnitADS11XX::attr{attribute::AccessI2C};

bool UnitADS11XX::begin()
{
    auto ssize = stored_size();
    assert(ssize && "stored_size must be greater than zero");
    if (ssize != _data->capacity()) {
        _data.reset(new m5::container::CircularBuffer<Data>(ssize));
        if (!_data) {
            M5_LIB_LOGE("Failed to allocate");
            return false;
        }
    }

    UnitADS11XX::generalReset();

    Config c{};
    if (!read_config(c.value) || c.value != DEFAULT_CONFIG_VALUE) {
        M5_LIB_LOGE("Can not detect ADS11XX %02X", c.value);
        return false;
    }
    _pga  = c.pga();
    _rate = c.rate();
    return true;
}

void UnitADS11XX::update(const bool force)
{
    _updated = false;
    if (inPeriodic()) {
        elapsed_time_t at{m5::utility::millis()};
        if (force || !_latest || at >= _latest + _interval) {
            Data d{};
            _updated = read_if_ready_in_periodic(d.raw.data());
            if (_updated) {
                d.pga    = _pga;
                d.rate   = _rate;
                d.vdd    = _vdd;
                d.factor = _factor;
                _data->push_back(d);
                _latest = at;
            }
        }
    }
}

bool UnitADS11XX::start_periodic_measurement(const uint8_t cfg_value)
{
    if (inPeriodic()) {
        M5_LIB_LOGD("Periodic measurements are running");
        return false;
    }

    Config c{};
    c.value = cfg_value;
    c.continuous(true);

    _periodic = write_config(c.value);
    if (_periodic) {
        _interval = get_interval(c.rate());
        _latest   = 0;
        read_config(c.value);
    }
    return _periodic;
}

bool UnitADS11XX::start_periodic_measurement()
{
    uint8_t c{};
    return read_config(c) && start_periodic_measurement(c);
}

bool UnitADS11XX::stop_periodic_measurement()
{
    Config c{};
    if (read_config(c.value)) {
        // Periodic measurement stops is substituted by a change to single mode
        c.single(true);
        if (write_config(c.value)) {
            _periodic = false;
            return true;
        }
    }
    return false;
}

bool UnitADS11XX::measure_singleshot(ads11xx::Data& data, const uint8_t cfg_value)
{
    if (inPeriodic()) {
        M5_LIB_LOGD("Periodic measurements are running");
        return false;
    }

    Config c{};
    c.value = cfg_value;
    c.st(true);
    c.single(true);
    if (write_config(c.value)) {
        auto timeout_at = m5::utility::millis() + 1000;
        do {
            if (is_data_ready() && read_measurement(data.raw.data())) {
                data.pga    = _pga;
                data.rate   = _rate;
                data.vdd    = _vdd;
                data.factor = _factor;
                return true;
            }
        } while (m5::utility::millis() <= timeout_at);
    }
    return false;
}

bool UnitADS11XX::measure_singleshot(ads11xx::Data& data)
{
    Config c{};
    return read_config(c.value) && measure_singleshot(data, c.value);
}

bool UnitADS11XX::readPGA(ads11xx::PGA& pga)
{
    Config c{};
    if (read_config(c.value)) {
        pga = c.pga();
        return true;
    }
    return false;
}

bool UnitADS11XX::writePGA(const ads11xx::PGA pga)
{
    if (inPeriodic()) {
        M5_LIB_LOGD("Periodic measurements are running");
        return false;
    }

    Config c{};
    if (read_config(c.value)) {
        c.pga(pga);
        return write_config(c.value);
    }
    return false;
}

bool UnitADS11XX::generalReset()
{
    uint8_t cmd{0x06};  // reset command
    // Reset does not return ACK, which is an error, but should be ignored
    generalCall(&cmd, 1);

    Config c{};
    auto timeout_at = m5::utility::millis() + 100;
    do {
        if (read_config(c.value) && c.value == DEFAULT_CONFIG_VALUE) {
            return true;
        }
        m5::utility::delay(1);

    } while (m5::utility::millis() <= timeout_at);
    return false;
}

bool UnitADS11XX::read_config(uint8_t& v)
{
    uint8_t rbuf[3]{};  // [0,]:data [2]:config
    if ((writeWithTransaction(nullptr, 0U) == m5::hal::error::error_t::OK) &&
        (readWithTransaction(rbuf, 3) == m5::hal::error::error_t::OK)) {
        v = rbuf[2];
        return true;
    }
    return false;
}

bool UnitADS11XX::write_config(const uint8_t v)
{
    if (writeWithTransaction(&v, 1) == m5::hal::error::error_t::OK) {
        Config c{};
        c.value = v;
        _pga    = c.pga();
        _rate   = c.rate();
        return true;
    }
    return false;
}

bool UnitADS11XX::read_measurement(uint8_t v[2])
{
    return (writeWithTransaction(nullptr, 0U) == m5::hal::error::error_t::OK) &&
           (readWithTransaction(v, 2) == m5::hal::error::error_t::OK);
}

bool UnitADS11XX::is_data_ready()
{
    Config c{};
    return read_config(c.value) && ((c.st()) == 0);
}

bool UnitADS11XX::read_if_ready_in_periodic(uint8_t v[2])
{
    return is_data_ready() && read_measurement(v);
}

}  // namespace unit
}  // namespace m5
