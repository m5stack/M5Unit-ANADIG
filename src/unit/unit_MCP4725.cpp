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
// EEPROM write time: typ 25 ms, max 50 ms
constexpr uint32_t eeprom_write_typ_ms{25};
constexpr uint32_t eeprom_ready_timeout_ms{25};

// Read status layout: [0]:status [1-2]:DAC register [3-4]:EEPROM
void parse_dac_register(const uint8_t rbuf[5], PowerDown& pd, uint16_t& raw)
{
    pd  = static_cast<PowerDown>((rbuf[0] >> 1) & 0x03);
    raw = (static_cast<uint16_t>(rbuf[1]) << 4) | static_cast<uint16_t>((rbuf[2] >> 4) & 0x0F);
}

void parse_eeprom(const uint8_t rbuf[5], PowerDown& pd, uint16_t& raw)
{
    pd  = static_cast<PowerDown>((rbuf[3] >> 5) & 0x03);
    raw = (static_cast<uint16_t>(rbuf[3] & 0x0F) << 8) | static_cast<uint16_t>(rbuf[4]);
}
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

    uint8_t rbuf[5]{};
    if (!read_status(rbuf)) {
        M5_LIB_LOGE("Can not detect MCP4725");
        return false;
    }
    if (_cfg.using_eeprom_settings) {
        // Apply the EEPROM settings to the DAC register
        parse_eeprom(rbuf, _power_down, _last_value);
        return writeVoltage(_last_value);
    }
    // Keep the current output and take over the DAC register as the inner state.
    // (It equals the EEPROM settings after power-on, or the last written value after an MCU-only reset)
    parse_dac_register(rbuf, _power_down, _last_value);
    return true;
}

bool UnitMCP4725::writeVoltageAndEEPROM(const uint16_t raw, const bool blocking)
{
    if (write_voltage(Command::WriteDACAndEEPROM, raw)) {
        if (blocking) {
            m5::utility::delay(eeprom_write_typ_ms);
            const auto start_at = m5::utility::millis();
            do {
                if (is_eeprom_ready()) {
                    return true;
                }
                m5::utility::delay(1);
            } while (!m5::utility::hasElapsed(start_at, eeprom_ready_timeout_ms));
            return false;  // timeout
        }
        return true;  // non-blocking
    }
    return false;
}

bool UnitMCP4725::write_voltage(const Command cmd, const uint16_t raw)
{
    const uint16_t v{static_cast<uint16_t>(raw & RESOLUTION)};
    uint8_t buf[3]{};
    const uint32_t len = make_buffer(buf, v, cmd);
    if (writeWithTransaction(buf, len) == m5::hal::error::error_t::OK) {
        _last_value = v;
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
    return readDACRegister(_power_down, _last_value);
}

bool UnitMCP4725::writePowerDown(const mcp4725::PowerDown pd)
{
    _power_down = pd;
    return writeVoltage(_last_value);
}

bool UnitMCP4725::readDACRegister(mcp4725::PowerDown& pd, uint16_t& raw)
{
    uint8_t rbuf[5]{};
    if (read_status(rbuf)) {
        parse_dac_register(rbuf, pd, raw);
        return true;
    }
    return false;
}

bool UnitMCP4725::readEEPROM(mcp4725::PowerDown& pd, uint16_t& raw)
{
    uint8_t rbuf[5]{};
    if (read_status(rbuf)) {
        parse_eeprom(rbuf, pd, raw);
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
uint32_t UnitMCP4725::make_buffer(uint8_t buf[3], const uint16_t raw, const Command cmd) const
{
    if (!buf) {
        return 0;
    }

    uint32_t len{};
    uint8_t pwd{m5::stl::to_underlying(_power_down)};
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
