/*
 * SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file unit_MCP4725.cpp
  @brief MCP4725 Unit for M5UnitUnified
*/
#include "unit_MCP4725.hpp"
#include <M5Utility.hpp>

using namespace m5::utility::mmh3;
using namespace m5::unit::types;
using namespace m5::unit::mcp4725;

namespace {
}  // namespace

namespace m5 {
namespace unit {

const char UnitMCP4725::name[] = "UnitMCP4725";
const types::uid_t UnitMCP4725::uid{"UnitMCP4725"_mmh3};
const types::attr_t UnitMCP4725::attr{attribute::AccessI2C};

bool UnitMCP4725::begin()
{
    if (_cfg.supply_voltage <= 0.0f) {
        M5_LIB_LOGE("Invalid supply voltage %f", _cfg.supply_voltage);
        return false;
    }

    // Read EEPROM settings
    PowerDown pd{};
    uint16_t raw{};
    if (!readEEPROM(pd, raw)) {
        M5_LIB_LOGE("Can not detect MCP4725");
        return false;
    }
    if (_cfg.using_eeprom_settings) {
        _powerDown = pd;
        _lastValue = raw;
    }
    return _cfg.using_eeprom_settings ? writeVoltage(_lastValue) : true;
}

bool UnitMCP4725::writeVoltageAndEEPROM(const uint16_t raw, const bool blocking)
{
    if (write_voltage(Command::WriteDACAndEEPROM, raw)) {
        bool done{!blocking};
        if (blocking) {
            m5::utility::delay(25);  // typ:25 max:50
            auto timeout_at = m5::utility::millis() + 25;
            do {
                done = is_eeprom_ready();
                if (done) {
                    break;
                };
                m5::utility::delay(1);
            } while (!done && m5::utility::millis() <= timeout_at);
        }
        return done;
    }
    return false;
}

bool UnitMCP4725::write_voltage(const Command cmd, const uint16_t raw)
{
    uint8_t buf[3]{};
    uint32_t len = make_buffer(buf, raw, cmd);
    if (writeWithTransaction(buf, len) == m5::hal::error::error_t::OK) {
        _lastValue = raw;
        return true;
    }
    return false;
}

bool UnitMCP4725::generalReset()
{
    uint8_t cmd{0x06};  // reset command
    // Reset does not return ACK, which is an error, but should be ignored
    generalCall(&cmd, 1);
    m5::utility::delay(50);
    return readDACRegister(_powerDown, _lastValue);
}

bool UnitMCP4725::writePowerDown(const mcp4725::PowerDown pd)
{
    _powerDown = pd;
    return writeVoltage(_lastValue);
}

bool UnitMCP4725::readDACRegister(mcp4725::PowerDown& pd, uint16_t& raw)
{
    uint8_t rbuf[5]{};
    if (read_status(rbuf)) {
        pd  = static_cast<PowerDown>((rbuf[0] >> 1) & 0x03);
        raw = ((uint16_t)rbuf[1] << 4) | (uint16_t)((rbuf[2] >> 4) & 0x0F);
        return true;
    }
    return false;
}

bool UnitMCP4725::readEEPROM(mcp4725::PowerDown& pd, uint16_t& raw)
{
    uint8_t rbuf[5]{};
    if (read_status(rbuf)) {
        pd  = static_cast<PowerDown>((rbuf[3] >> 5) & 0x03);
        raw = ((uint16_t)(rbuf[3] & 0x0F) << 8) | (uint16_t)rbuf[4];
        return true;
    }
    return false;
}

bool UnitMCP4725::is_eeprom_ready()
{
    uint8_t rbuf[5]{};
    return read_status(rbuf) && (rbuf[0] & 0x80);
}

// return: buffer length
uint32_t UnitMCP4725::make_buffer(uint8_t buf[3], const uint16_t raw, const Command cmd)
{
    if (!buf) {
        return 0;
    }

    uint32_t len{};
    uint8_t pwd{m5::stl::to_underlying(_powerDown)};
    switch (cmd) {
        case Command::FastMode:
            buf[0] = ((raw >> 8) & 0x0F) | (pwd << 4);
            buf[1] = (raw & 0xFF);
            len    = 2;
            break;
        default:
            buf[0] = ((cmd == Command::WriteDAC) ? 0x40 : 0x60) | (pwd << 1);
            buf[1] = (raw >> 4) & 0xFF;
            buf[2] = (raw << 4) & 0xF0;
            len    = 3;
            break;
    }
    return len;
}

bool UnitMCP4725::read_status(uint8_t rbuf[5])
{
    return rbuf && (readWithTransaction(rbuf, 5) == m5::hal::error::error_t::OK);
}

}  // namespace unit
}  // namespace m5
