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
    1000 / 60,
    1000 / 30,
    1000 / 15,
};

constexpr uint8_t CONFIG_DEFAULT_VALUE{0x8C};

}  // namespace

struct Config {
    inline Sampling rate() const
    {
        return static_cast<Sampling>((value >> 2) & 0x03);
    }
    inline PGA pga() const
    {
        return static_cast<PGA>(value & 0x03);
    }
    inline bool continuous() const
    {
        return value & (1U << 4);
    }
    inline bool single() const
    {
        return !continuous();
    }
    inline bool ready() const
    {
        return (value & 0x80) == 0;
    }

    inline void rate(const Sampling rate)
    {
        value = (value & ~(0x03 << 2)) | (m5::stl::to_underlying(rate) << 2);
    }
    inline void pga(const PGA pga)
    {
        value = (value & ~0x03) | m5::stl::to_underlying(pga);
    }
    inline void continuous(bool enable)
    {
        value = (value & ~(1U << 4)) | ((enable ? 0 : 1) << 4);
    }
    inline void single(bool enable)
    {
        continuous(false);
    }
    inline void ready(const bool b)
    {
        value = (value & ~0x80) | (b ? 0x80 : 0x00);
    }

    uint8_t value{};
};

namespace m5 {
namespace unit {

namespace ads1110 {
const float Data::gain_table[4] = {2.048f, 1.024f, 0.512f, 0.256f};
}

const char UnitADS1110::name[] = "UnitADS1110";
const types::uid_t UnitADS1110::uid{"UnitADS1110"_mmh3};
const types::uid_t UnitADS1110::attr{0};

bool UnitADS1110::begin()
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

    uint8_t c{};
    if (!read_config(c)) {
        M5_LIB_LOGE("Can not detect ADS1110 %02X", c);
        return false;
    }
    return _cfg.start_periodic ? startPeriodicMeasurement(_cfg.sampling_rate, _cfg.pga) : stopPeriodicMeasurement();
}

void UnitADS1110::update(const bool force)
{
    _updated = false;
    if (inPeriodic()) {
        elapsed_time_t at{m5::utility::millis()};
        if (force || !_latest || at >= _latest + _interval) {
            Data d{};
            _updated = is_data_ready() && read_measurement(d.raw.data());
            if (_updated) {
                d.pga   = _pga;
                _latest = at;
                _data->push_back(d);
            }
        }
    }
}

bool UnitADS1110::start_periodic_measurement(const ads1110::Sampling rate, const ads1110::PGA pga)
{
    if (inPeriodic()) {
        return false;
    }

    Config c{};
    if (read_config(c.value)) {
        c.rate(rate);
        c.pga(pga);
        c.continuous(true);
        _periodic = write_config(c.value);
        if (_periodic) {
            _pga      = c.pga();
            _interval = interval_table[m5::stl::to_underlying(rate)];
            _latest   = 0;
        }
    }
    return _periodic;
}

bool UnitADS1110::start_periodic_measurement()
{
    Config c{};
    return read_config(c.value) && start_periodic_measurement(c.rate(), c.pga());
}

bool UnitADS1110::stop_periodic_measurement()
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

bool UnitADS1110::measureSingleshot(ads1110::Data& data, const ads1110::Sampling rate, const ads1110::PGA pga)
{
    if (inPeriodic()) {
        M5_LIB_LOGD("Periodic measurements are running");
        return false;
    }

    Config c{};
    if (read_config(c.value)) {
        c.ready(true);
        c.rate(rate);
        c.pga(pga);
        c.single(true);
        if (write_config(c.value)) {
            auto timeout_at = m5::utility::millis() + 100;
            do {
                if (is_data_ready() && read_measurement(data.raw.data())) {
                    data.pga = pga;
                    return true;
                }
            } while (m5::utility::millis() <= timeout_at);
        }
    }
    return false;
}

bool UnitADS1110::measureSingleshot(ads1110::Data& data)
{
    Config c{};
    return read_config(c.value) && measureSingleshot(data, c.rate(), c.pga());
}

bool UnitADS1110::generalReset()
{
    uint8_t cmd{0x06};  // reset command
    // Reset does not return ACK, which is an error, but should be ignored
    generalCall(&cmd, 1);

    Config c{};
    auto timeout_at = m5::utility::millis() + 100;
    do {
        if (read_config(c.value) && c.value == CONFIG_DEFAULT_VALUE) {
            return stop_periodic_measurement();
        }
        m5::utility::delay(1);

    } while (m5::utility::millis() <= timeout_at);
    return false;
}

bool UnitADS1110::readSamplingRate(ads1110::Sampling& rate)
{
    Config c{};
    if (read_config(c.value)) {
        rate = c.rate();
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
        c.rate(rate);
        return write_config(c.value);
    }
    return false;
}

bool UnitADS1110::readPGA(ads1110::PGA& pga)
{
    Config c{};
    if (read_config(c.value)) {
        pga = c.pga();
        return true;
    }
    return false;
}

bool UnitADS1110::writePGA(const ads1110::PGA pga)
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

//
bool UnitADS1110::read_config(uint8_t& v)
{
    uint8_t rbuf[3]{};  // [0,]:data [2]:config
    if ((writeWithTransaction(nullptr, 0U) == m5::hal::error::error_t::OK) &&
        (readWithTransaction(rbuf, 3) == m5::hal::error::error_t::OK)) {
        v = rbuf[2];
        return true;
    }
    return false;
}

bool UnitADS1110::write_config(const uint8_t v)
{
    return (writeWithTransaction(&v, 1) == m5::hal::error::error_t::OK);
}

bool UnitADS1110::read_measurement(uint8_t* v)
{
    return (writeWithTransaction(nullptr, 0U) == m5::hal::error::error_t::OK) &&
           (readWithTransaction(v, 2) == m5::hal::error::error_t::OK);
}

bool UnitADS1110::is_data_ready()
{
    Config c{};
    return read_config(c.value) && c.ready();
}

}  // namespace unit
}  // namespace m5
