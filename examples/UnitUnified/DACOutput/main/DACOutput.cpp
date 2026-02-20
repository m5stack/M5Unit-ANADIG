/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
  Example using M5UnitUnified for UnitDAC2/HatDAC2
*/
#include <M5Unified.h>
#include <M5UnitUnified.h>
#include <M5UnitUnifiedANADIG.h>
#include <M5Utility.h>
#include <M5HAL.hpp>
#include <cmath>

// *************************************************************
// Choose one define symbol to match the unit you are using
// *************************************************************
#if !defined(USING_UNIT_DAC) && !defined(USING_UNIT_DAC2) && !defined(USING_HAT_DAC2)
// For UnitDAC
// #define USING_UNIT_DAC
// For UnitDAC2
// #define USING_UNIT_DAC2
// For HatDAC2
// #define USING_HAT_DAC2
#endif
// *************************************************************

namespace {
auto& lcd = M5.Display;
m5::unit::UnitUnified Units;
#if defined(USING_UNIT_DAC)
m5::unit::UnitDAC unit;
#elif defined(USING_UNIT_DAC2)
m5::unit::UnitDAC2 unit;
#elif defined(USING_HAT_DAC2)
m5::unit::HatDAC2 unit;
#else
#error Please choose unit or hat!
#endif
uint32_t counter{};
bool has_display{};

constexpr inline float deg2rad(const float deg)
{
    return ((deg) / 180.0f * M_PI);
}

float sin_curve(const uint32_t counter, const float maxMv)
{
    float rad = deg2rad(counter % 360);
    float v   = sinf(rad);
    return maxMv * (v + 1.0f) * 0.5f;
}

float sawtooth_wave(const uint32_t counter, const float maxMv)
{
    float rad = (counter % 360) / 360.f;
    float v   = rad - floor(rad);
    return v * maxMv;
}

float square_wave(const uint32_t counter, const float maxMv)
{
    float rad = deg2rad(counter % 360);
    float v   = (sinf(rad) >= 0.0f) ? 1.0f : -1.0f;
    return maxMv * (v + 1.0f) * 0.5f;
}

float triangle_wave(const uint32_t counter, const float maxMv)
{
    float rad = deg2rad(counter % 360);
    float v   = asinf(sinf(rad));
    return maxMv * (v + M_PI / 2) / M_PI;
}

using function = float (*)(const uint32_t, const float);

constexpr function func_table[4] = {
    sin_curve,
    sawtooth_wave,
    triangle_wave,
    square_wave,

};
const char* func_name_table[4] = {
    "SinCurve",
    "SawtoothWave",
    "TriangleWave",
    "SquareWave",
};

uint32_t fidx{};
function func = func_table[fidx];

#if defined(USING_HAT_DAC2)
struct I2cPins {
    int sda;
    int scl;
};

I2cPins get_hat_i2c_pins(const m5::board_t board)
{
    switch (board) {
        case m5::board_t::board_M5StickC:
        case m5::board_t::board_M5StickCPlus:
        case m5::board_t::board_M5StickCPlus2:
            return {0, 26};
        case m5::board_t::board_M5StickS3:
            return {8, 0};
        case m5::board_t::board_M5StackCoreInk:
            return {25, 26};
        case m5::board_t::board_ArduinoNessoN1:
            return {6, 7};
        default:
            return {-1, -1};
    }
}
#endif

}  // namespace

using namespace m5::unit::gp8413;

void setup()
{
    delay(1500);

    auto m5cfg = M5.config();
#if defined(USING_HAT_DAC2)
    m5cfg.pmic_button  = false;  // Disable BtnPWR
    m5cfg.internal_imu = false;  // Disable internal IMU
    m5cfg.internal_rtc = false;  // Disable internal RTC
#endif

    M5.begin(m5cfg);
    M5.setTouchButtonHeightByRatio(100);
    const auto board = M5.getBoard();

    // The screen shall be in landscape mode if exists
    if (lcd.height() > lcd.width()) {
        lcd.setRotation(1);
    }

    // ePaper panels are not suitable for continuous redraw
    has_display = !lcd.isEPD() && (lcd.width() > 0 && lcd.height() > 0);

#if defined(USING_HAT_DAC2)
    const auto pins = get_hat_i2c_pins(board);
    M5_LOGI("getHatPin: SDA:%u SCL:%u", pins.sda, pins.scl);
    if (pins.sda < 0 || pins.scl < 0) {
        M5_LOGE("Illegal pin number");
        lcd.fillScreen(TFT_RED);
        while (true) {
            m5::utility::delay(10000);
        }
    }
    auto& wire = (board == m5::board_t::board_ArduinoNessoN1) ? Wire1 : Wire;
    wire.end();
    wire.begin(pins.sda, pins.scl, 400 * 1000U);
    if (!Units.add(unit, wire) || !Units.begin()) {
        M5_LOGE("Failed to begin");
        lcd.fillScreen(TFT_RED);
        while (true) {
            m5::utility::delay(10000);
        }
    }
#else
    auto pin_num_sda = M5.getPin(m5::pin_name_t::port_a_sda);
    auto pin_num_scl = M5.getPin(m5::pin_name_t::port_a_scl);
    if (board == m5::board_t::board_ArduinoNessoN1) {
        // Port A of the NessoN1 is QWIIC, then use portB (GROVE)
        pin_num_sda = M5.getPin(m5::pin_name_t::port_b_out);
        pin_num_scl = M5.getPin(m5::pin_name_t::port_b_in);
        M5_LOGI("getPin(NessoN1): SDA:%u SCL:%u", pin_num_sda, pin_num_scl);

        // Wire is used internally, so SoftwareI2C handles the unit
        m5::hal::bus::I2CBusConfig i2c_cfg;
        i2c_cfg.pin_sda = m5::hal::gpio::getPin(pin_num_sda);
        i2c_cfg.pin_scl = m5::hal::gpio::getPin(pin_num_scl);
        auto i2c_bus    = m5::hal::bus::i2c::getBus(i2c_cfg);

        if (!Units.add(unit, i2c_bus ? i2c_bus.value() : nullptr) || !Units.begin()) {
            M5_LOGE("Failed to begin");
            lcd.fillScreen(TFT_RED);
            while (true) {
                m5::utility::delay(10000);
            }
        }
    } else {
        M5_LOGI("getPin: SDA:%u SCL:%u", pin_num_sda, pin_num_scl);
        Wire.end();
        Wire.begin(pin_num_sda, pin_num_scl, 400 * 1000U);
        if (!Units.add(unit, Wire) || !Units.begin()) {
            M5_LOGE("Failed to begin");
            lcd.fillScreen(TFT_RED);
            while (true) {
                m5::utility::delay(10000);
            }
        }
    }
#endif

#if !defined(USING_UNIT_DAC)
    unit.writeOutputRange(Output::Range5V, Output::Range5V);
    unit.writeBothVoltage(0U, 0U);
#endif

    M5_LOGI("M5UnitUnified has been begun");
    M5_LOGI("%s", Units.debugInfo().c_str());

    if (has_display) {
        lcd.setFont(lcd.width() > 240 ? &fonts::Font4 : &fonts::Font2);
        lcd.fillScreen(TFT_BLACK);
        lcd.setTextDatum(middle_center);
        lcd.drawString(func_name_table[fidx], lcd.width() >> 1, lcd.height() >> 1);
        lcd.setTextDatum(top_left);
    }
    M5.Log.printf("Output:%s\n", func_name_table[fidx]);
}

void loop()
{
    static float pv0{}, pv1{};
#if defined(USING_UNIT_DAC)
    static float max_0{m5::unit::UnitDAC::MAXIMUM_VOLTAGE};
#else
    static float max_0{unit.maximumVoltage(Channel::Zero)};
    static float max_1{unit.maximumVoltage(Channel::One)};
#endif

    M5.update();
    Units.update();

#if defined(USING_UNIT_DAC)
    auto v0 = func(counter, max_0);
    auto v1 = 0.0f;
    unit.writeVoltage(v0);
    M5.Log.printf("Voltage:%.2f\n", v0);
#else
    auto v0 = func(counter, max_0);
    auto v1 = func(counter, max_1);
    unit.writeBothVoltage(v0, v1);
    M5.Log.printf("Voltage:%.2f / %.2f\n", v0, v1);
#endif
    counter += 6;

    if (has_display) {
        auto bwid = lcd.width() >> 3;

        lcd.startWrite();

        if (pv0 != v0 || pv1 != v1) {
            lcd.fillRect(bwid, (lcd.height() >> 1) + 24, lcd.width() - bwid * 2, (lcd.height() >> 1) - 24, TFT_BLACK);
            lcd.drawString(m5::utility::formatString("< Ch0:%.2f", v0).c_str(), bwid * 2, (lcd.height() >> 1) + 24);
#if !defined(USING_UNIT_DAC)
            lcd.drawString(m5::utility::formatString("> Ch1:%.2f", v1).c_str(), bwid * 2, (lcd.height() >> 1) + 24 * 2);
#endif
        }

        // Channel 0
        if (pv0 != v0) {
            pv0       = v0;
            auto vhgt = lcd.height() * (v0 / max_0);
            lcd.fillRect(0, 0, bwid, lcd.height() - vhgt, TFT_BLACK);
            lcd.fillRect(0, lcd.height() - vhgt, bwid, vhgt, TFT_RED);
        }
#if !defined(USING_UNIT_DAC)
        // Channel 1
        if (pv1 != v1) {
            pv1       = v1;
            auto vhgt = lcd.height() * (v1 / max_1);
            lcd.fillRect(lcd.width() - bwid, 0, bwid, lcd.height() - vhgt, TFT_BLACK);
            lcd.fillRect(lcd.width() - bwid, lcd.height() - vhgt, bwid, vhgt, TFT_BLUE);
        }
#endif

        lcd.endWrite();
    }

    // Change output function
    if (M5.BtnA.wasClicked()) {
        fidx    = (fidx + 1) % m5::stl::size(func_table);
        func    = func_table[fidx];
        counter = 0;

        M5.Speaker.tone(2000, 20);
        if (has_display) {
            lcd.fillScreen(TFT_BLACK);
            lcd.setTextDatum(top_center);
            lcd.drawString(func_name_table[fidx], lcd.width() >> 1, lcd.height() >> 1);
            lcd.setTextDatum(top_left);
        }
        M5.Log.printf("==== Output:%s\n", func_name_table[fidx]);
    }

#if !defined(USING_UNIT_DAC)
    // Change output range(DAC2)
    if (M5.BtnA.wasHold()) {
        static uint32_t range_mode{};
        M5.Speaker.tone(4000, 50);

        range_mode = (range_mode + 1) & 0x03;  // 0-3
        unit.writeOutputRange((range_mode & 0x01) ? Output::Range10V : Output::Range5V,
                              (range_mode & 0x02) ? Output::Range10V : Output::Range5V);
        max_0 = unit.maximumVoltage(Channel::Zero);
        max_1 = unit.maximumVoltage(Channel::One);

        M5.Log.printf("---- Range V0:%uV V1:%uV\n", (int)(max_0 / 1000), (int)(max_1 / 1000));
    }
#endif

    m5::utility::delay(1);
}
