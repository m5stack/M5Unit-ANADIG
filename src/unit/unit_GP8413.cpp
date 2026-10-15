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
#include <M5HAL.hpp>
#include <m5_unit_component/adapter_i2c.hpp>
#if defined(ARDUINO)
#include <Wire.h>
#endif

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

  (These values match the range encoding described in the GP8211S datasheet.)
  *******************************************************************************************************
*/
constexpr uint8_t mode_nibble_table[] = {0x05, 0x07};

/*
  Store sequence (datasheet 3.3.6). It contains 3-bit frames and frames without a device address, which the
  hardware I2C controller cannot generate, so the sequence is bit-banged on the SDA/SCL pins.
  Timing follows DFRobot_GP8XXX_IIC::store().
*/
constexpr uint8_t store_head{0x02};  // "010" (3 bits, no ACK)
constexpr uint8_t store_cmd_addr{0x10};
constexpr uint8_t store_cmd_enter{0x03};
constexpr uint8_t store_cmd_exit{0x00};
constexpr uint8_t store_data_count{8};
constexpr uint32_t store_wait_ms{10};  // At least 7 ms

constexpr uint32_t cycle_total_us{5};
constexpr uint32_t cycle_before_us{1};
constexpr uint32_t cycle_after_us{2};
constexpr uint32_t ack_timeout_us{250};

using m5::hal::interface::gpio::Pin;
using PinMode = m5::hal::types::gpio_mode_t;

void bb_start(Pin* sda, Pin* scl)
{
    scl->writeHigh();
    sda->writeHigh();
    m5::utility::delayMicroseconds(cycle_before_us);
    sda->writeLow();
    m5::utility::delayMicroseconds(cycle_after_us);
    scl->writeLow();
    m5::utility::delayMicroseconds(cycle_total_us);
}

void bb_stop(Pin* sda, Pin* scl)
{
    sda->writeLow();
    m5::utility::delayMicroseconds(cycle_before_us);
    scl->writeHigh();
    m5::utility::delayMicroseconds(cycle_total_us);
    sda->writeHigh();
    m5::utility::delayMicroseconds(cycle_total_us);
}

// Returns true if SDA is at the expected level at the ACK slot (waits up to ack_timeout_us)
bool bb_ack(Pin* sda, Pin* scl, const bool level)
{
    sda->setMode(PinMode::Input);
    m5::utility::delayMicroseconds(cycle_before_us);
    scl->writeHigh();
    m5::utility::delayMicroseconds(cycle_after_us);
    for (uint32_t t = 0; sda->read() != level && t < ack_timeout_us; ++t) {
        m5::utility::delayMicroseconds(1);
    }
    const bool matched = sda->read() == level;
    m5::utility::delayMicroseconds(cycle_before_us);
    scl->writeLow();
    m5::utility::delayMicroseconds(cycle_after_us);
    sda->setMode(PinMode::Output);
    return matched;
}

bool bb_write(Pin* sda, Pin* scl, const uint8_t data, const bool ack_level, const uint8_t bits = 8,
              const bool with_ack = true)
{
    for (int_fast8_t i = bits - 1; i >= 0; --i) {
        sda->write(data & (1U << i));
        m5::utility::delayMicroseconds(cycle_before_us);
        scl->writeHigh();
        m5::utility::delayMicroseconds(cycle_total_us);
        scl->writeLow();
        m5::utility::delayMicroseconds(cycle_after_us);
    }
    if (with_ack) {
        return bb_ack(sda, scl, ack_level);
    }
    sda->writeLow();
    scl->writeHigh();
    return true;
}

// Command frame: "010" (3 bits, no ACK), then the command address and value, each ACKed (low) by the chip
bool bb_command(Pin* sda, Pin* scl, const uint8_t cmd)
{
    bb_start(sda, scl);
    bb_write(sda, scl, store_head, false, 3, false);
    bb_stop(sda, scl);
    bb_start(sda, scl);
    const bool ack_addr = bb_write(sda, scl, store_cmd_addr, false);
    const bool ack_cmd  = bb_write(sda, scl, cmd, false);
    bb_stop(sda, scl);
    return ack_addr && ack_cmd;
}

// Returns false if the chip does not ACK the enter/exit commands (e.g. not connected)
bool bb_store_sequence(Pin* sda, Pin* scl, const uint8_t addr)
{
    sda->setMode(PinMode::Output);
    scl->setMode(PinMode::Output);
    scl->writeHigh();
    sda->writeHigh();

    if (!bb_command(sda, scl, store_cmd_enter)) {
        return false;
    }

    // The ACK slots of this frame stay high by design, so they are only waited for (not checked)
    bb_start(sda, scl);
    bb_write(sda, scl, static_cast<uint8_t>(addr << 1), true);
    for (uint_fast8_t i = 0; i < store_data_count; ++i) {
        bb_write(sda, scl, 0x00, true);
    }
    bb_stop(sda, scl);

    m5::utility::delay(store_wait_ms);

    return bb_command(sda, scl, store_cmd_exit);
}
}  // namespace

namespace m5 {
namespace unit {

const char UnitGP8413::name[] = "UnitGP8413";
const types::uid_t UnitGP8413::uid{"UnitGP8413"_mmh3};
const types::attr_t UnitGP8413::attr{attribute::AccessI2C};

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
    const uint16_t v{static_cast<uint16_t>(raw & RESOLUTION)};
    const uint8_t buf[2]{static_cast<uint8_t>(v & 0xFF), static_cast<uint8_t>(v >> 8)};
    return write_voltage(channel_reg_table[m5::stl::to_underlying(channel)], buf, 2);
}

bool UnitGP8413::writeBothVoltage(const uint16_t raw0, const uint16_t raw1)
{
    const uint16_t v0{static_cast<uint16_t>(raw0 & RESOLUTION)};
    const uint16_t v1{static_cast<uint16_t>(raw1 & RESOLUTION)};
    const uint8_t buf[4]{static_cast<uint8_t>(v0 & 0xFF), static_cast<uint8_t>(v0 >> 8),
                         static_cast<uint8_t>(v1 & 0xFF), static_cast<uint8_t>(v1 >> 8)};
    return write_voltage(channel_reg_table[0], buf, 4);
}

uint16_t UnitGP8413::voltage_to_raw(const Channel channel, const float mv) const
{
    const float max_mv = maximumVoltage(channel);
    const float val    = m5::stl::clamp(mv, 0.0f, max_mv);
    return static_cast<uint16_t>((val / max_mv) * RESOLUTION);
}

bool UnitGP8413::write_voltage(const uint8_t reg, const uint8_t* buf, const uint32_t len)
{
    return buf && writeRegister(reg, buf, len);
}

bool UnitGP8413::storeBothVoltage()
{
    auto ad = asAdapter<AdapterI2C>(Adapter::Type::I2C);
    if (!ad || ad->sda() < 0 || ad->scl() < 0) {
        M5_LIB_LOGE("Store needs an I2C bus with known SDA/SCL pins");
        return false;
    }
    // Via a hub (PaHub etc.), route the parent channels to this unit first.
    // The hub is an analog switch, so the bit-banged frames pass through it
    if (!selectChannel(channel())) {
        M5_LIB_LOGE("Failed to select the channel");
        return false;
    }

    // Release the bus, bit-bang on the same pins, then restore the bus
    const auto type = ad->implType();
    switch (type) {
#if defined(ARDUINO) && defined(WIRE_HAS_END)
        case AdapterI2C::ImplType::TwoWire: {
            auto wire = ad->impl()->getWire();
            if (!wire || !wire->end()) {
                return false;
            }
            break;
        }
#endif
        case AdapterI2C::ImplType::I2CClass:
            if (!ad->end()) {
                return false;
            }
            break;
        case AdapterI2C::ImplType::Bus:  // Software I2C on GPIO; nothing to release
            break;
#if defined(ESP_PLATFORM) && __has_include(<driver/i2c_master.h>)
        case AdapterI2C::ImplType::ESPIDFMasterBus:
#elif defined(ESP_PLATFORM)
        case AdapterI2C::ImplType::ESPIDFLegacyBus:
#endif
#if defined(ESP_PLATFORM)
            // Detach the pins from the controller, keeping the driver installed
            // (fails on pins not routed through the GPIO matrix, e.g. LP I2C)
            if (!ad->releasePins()) {
                M5_LIB_LOGE("Store is not supported on this I2C bus (SDA/SCL are not routed through the GPIO matrix)");
                return false;
            }
            break;
#endif
        default:
            M5_LIB_LOGE("Store is not supported on this I2C backend:%u", m5::stl::to_underlying(type));
            return false;
    }

    auto sda          = m5::hal::gpio::getPin(ad->sda());
    auto scl          = m5::hal::gpio::getPin(ad->scl());
    const bool stored = sda && scl && bb_store_sequence(sda, scl, address());

    bool restored{};
    switch (type) {
#if defined(ARDUINO) && defined(WIRE_HAS_END)
        case AdapterI2C::ImplType::TwoWire:
            restored = ad->impl()->getWire()->begin(ad->sda(), ad->scl(), ad->clock());
            break;
#endif
        case AdapterI2C::ImplType::I2CClass:
            restored = ad->begin();
            break;
        case AdapterI2C::ImplType::Bus:  // Back to the idle state of the software I2C (open-drain, released)
            if (!sda || !scl) {
                break;
            }
            sda->setMode(PinMode::Output_OpenDrain);
            scl->setMode(PinMode::Output_OpenDrain);
            scl->writeHigh();
            sda->writeHigh();
            restored = true;
            break;
#if defined(ESP_PLATFORM) && __has_include(<driver/i2c_master.h>)
        case AdapterI2C::ImplType::ESPIDFMasterBus:
#elif defined(ESP_PLATFORM)
        case AdapterI2C::ImplType::ESPIDFLegacyBus:
#endif
#if defined(ESP_PLATFORM)
            restored = ad->restorePins();
            break;
#endif
        default:
            break;
    }
    if (!sda || !scl) {
        M5_LIB_LOGE("Failed to get SDA/SCL pins");
    } else if (!stored) {
        M5_LIB_LOGE("The chip did not acknowledge the store sequence");
    }
    if (!restored) {
        M5_LIB_LOGE("Failed to restore the I2C bus");
    }
    return stored && restored;
}

}  // namespace unit
}  // namespace m5
