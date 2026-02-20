/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
  Example using M5UnitUnified for UnitADC11/HatADC11/HatADC
*/
#include <M5Unified.h>
#include <M5UnitUnified.h>
#include <M5UnitUnifiedANADIG.h>
#include <M5Utility.h>
#include <M5HAL.hpp>

// *************************************************************
// Choose one define symbol to match the unit you are using
// *************************************************************
#if !defined(USING_UNIT_ADC11) && !defined(USING_HAT_ADC11) && !defined(USING_HAT_ADC)
// For UnitADC11
// #define USING_UNIT_ADC11
// For HatADC11
// #define USING_HAT_ADC11
// For HatADC
// #define USING_HAT_ADC
#endif
// *************************************************************

namespace {
auto& lcd = M5.Display;
m5::unit::UnitUnified Units;

#if defined(USING_UNIT_ADC11)
m5::unit::UnitADC11 unit;
#elif defined(USING_HAT_ADC11)
m5::unit::UnitADC11 unit;
#elif defined(USING_HAT_ADC)
m5::unit::HatADC unit;
#else
#error Please choose unit!
#endif
LGFX_Sprite sprite{};
bool has_display{};

#if defined(USING_HAT_ADC11) || defined(USING_HAT_ADC)
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

#if defined(USING_UNIT_ADC11)
using namespace m5::unit::ads1110;
#elif defined(USING_HAT_ADC11) || defined(USING_HAT_ADC)
using namespace m5::unit::ads1100;
#else
#endif

void setup()
{
    auto m5cfg = M5.config();
#if defined(USING_HAT_ADC11) || defined(USING_HAT_ADC)
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

#if defined(USING_HAT_ADC11) || defined(USING_HAT_ADC)
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
        M5_LOGI("Bus:%d", i2c_bus.has_value());
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

    M5_LOGI("M5UnitUnified has been begun with %s", unit.deviceName());
    M5_LOGI("%s", Units.debugInfo().c_str());

    has_display = (lcd.width() > 0 && lcd.height() > 0);
    if (has_display) {
        constexpr RGBColor palettes[4] = {RGBColor(0, 0, 0), RGBColor(0, 0, 255), RGBColor(255, 0, 0),
                                          RGBColor(255, 255, 255)};
        sprite.setPsram(false);
        sprite.setColorDepth(2);  // 4 colors
        auto ptr = sprite.createSprite(lcd.width(), lcd.height());
        assert(ptr);
        sprite.setFont(lcd.width() > 240 ? &fonts::Font4 : &fonts::Font2);
        auto pal = sprite.getPalette();
        for (auto&& p : palettes) {
            *pal++ = p;
        }
        lcd.fillScreen(TFT_BLACK);
    }
}

void loop()
{
    M5.update();

    // Periodic
    Units.update();
    if (unit.updated()) {
        auto mv = unit.differentialVoltage();
        M5.Log.printf(">Raw:%d\n>Voltage(mV):%.2f\n", unit.differentialValue(), mv);

        const bool negative = (mv < 0.0f);
        auto abs_mv         = negative ? -mv : mv;
        auto ratio          = abs_mv / 12000.f;  // 0-12V (magnitude)
        if (ratio > 1.0f) {
            ratio = 1.0f;
        }
        int32_t deg = 300 * ratio;
        static int32_t prev_deg{-1};
        static bool prev_negative{};

        if (has_display && (deg != prev_deg || negative != prev_negative)) {
            auto cx                 = lcd.width() >> 1;
            auto cy                 = lcd.height() >> 1;
            const uint8_t arc_color = negative ? 2 : 1;  // blue:positive red:negative

            sprite.clear();
            sprite.fillArc(cx, cy, lcd.height() >> 1, (lcd.height() >> 1) - 16, 30, 330, 0);
            sprite.fillArc(cx, cy, lcd.height() >> 1, (lcd.height() >> 1) - 16, 30, 30 + 300 * ratio, arc_color);
            sprite.drawArc(cx, cy, lcd.height() >> 1, (lcd.height() >> 1) - 16, 30, 330, 3);
            sprite.setCursor(cx, cy);
            sprite.printf("%.2fmV", mv);

            lcd.startWrite();
            sprite.pushSprite(&lcd, 0, 0);
            lcd.endWrite();
            prev_deg      = deg;
            prev_negative = negative;
        }
    }

    // Toggle Single/Periodic
    if (M5.BtnA.wasClicked()) {
        static bool single{};
        single = !single;
        if (single) {
            unit.stopPeriodicMeasurement();
            Data d{};
            if (unit.measureSingleshot(d)) {
                M5.Log.printf("Single: %d/%f\n", d.differentialValue(), d.differentialVoltage());
            }
        } else {
            unit.startPeriodicMeasurement();
        }
    }
}
