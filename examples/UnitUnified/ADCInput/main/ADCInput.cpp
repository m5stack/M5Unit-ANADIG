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

constexpr int MARGIN{4};
constexpr int BAR_H{12};
constexpr int BLOCK_H{30};  // 16(voltage) + 2(gap) + 12(bar)

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
#elif defined(USING_HAT_ADC11)
using namespace m5::unit::ads1110;
#elif defined(USING_HAT_ADC)
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
    M5_LOGI("getHatPin: SDA:%d SCL:%d", pins.sda, pins.scl);
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
    // NessoN1: Arduino Wire (I2C_NUM_0) cannot be used for GROVE port.
    //   Wire is used by M5Unified In_I2C for internal devices (IOExpander etc.).
    //   Wire1 exists but is reserved for HatPort — cannot be used for GROVE.
    //   Reconfiguring Wire to GROVE pins breaks In_I2C, causing ESP_ERR_INVALID_STATE in M5.update().
    //   Solution: Use SoftwareI2C via M5HAL (bit-banging) for the GROVE port.
    // NanoC6: Wire.begin() on GROVE pins conflicts with m5::I2C_Class registered by Ex_I2C.setPort()
    //   on the same I2C_NUM_0, causing sporadic NACK errors.
    //   Solution: Use M5.Ex_I2C (m5::I2C_Class) directly instead of Arduino Wire.
    bool unit_ready{};
    if (board == m5::board_t::board_ArduinoNessoN1) {
        // NessoN1: GROVE is on port_b (GPIO 5/4), not port_a (which maps to Wire pins 8/10)
        auto pin_num_sda = M5.getPin(m5::pin_name_t::port_b_out);
        auto pin_num_scl = M5.getPin(m5::pin_name_t::port_b_in);
        M5_LOGI("getPin(M5HAL): SDA:%d SCL:%d", pin_num_sda, pin_num_scl);
        m5::hal::bus::I2CBusConfig i2c_cfg;
        i2c_cfg.pin_sda = m5::hal::gpio::getPin(pin_num_sda);
        i2c_cfg.pin_scl = m5::hal::gpio::getPin(pin_num_scl);
        auto i2c_bus    = m5::hal::bus::i2c::getBus(i2c_cfg);
        M5_LOGI("Bus:%d", i2c_bus.has_value());
        unit_ready = Units.add(unit, i2c_bus ? i2c_bus.value() : nullptr) && Units.begin();
    } else if (board == m5::board_t::board_M5NanoC6) {
        // NanoC6: Use M5.Ex_I2C (m5::I2C_Class, not Arduino Wire)
        M5_LOGI("Using M5.Ex_I2C");
        unit_ready = Units.add(unit, M5.Ex_I2C) && Units.begin();
    } else {
        auto pin_num_sda = M5.getPin(m5::pin_name_t::port_a_sda);
        auto pin_num_scl = M5.getPin(m5::pin_name_t::port_a_scl);
        M5_LOGI("getPin: SDA:%d SCL:%d", pin_num_sda, pin_num_scl);
        Wire.end();
        Wire.begin(pin_num_sda, pin_num_scl, 400 * 1000U);
        unit_ready = Units.add(unit, Wire) && Units.begin();
    }
    if (!unit_ready) {
        M5_LOGE("Failed to begin");
        lcd.fillScreen(TFT_RED);
        while (true) {
            m5::utility::delay(10000);
        }
    }
#endif

    M5_LOGI("M5UnitUnified has been begun with %s", unit.deviceName());
    M5_LOGI("%s", Units.debugInfo().c_str());

    // ePaper panels are not suitable for continuous redraw
    has_display = !lcd.isEPD() && (lcd.width() > 0 && lcd.height() > 0);
    if (has_display) {
        constexpr RGBColor palettes[4] = {RGBColor(0, 0, 0), RGBColor(0, 0, 255), RGBColor(255, 0, 0),
                                          RGBColor(255, 255, 255)};
        sprite.setPsram(false);
        sprite.setColorDepth(2);  // 4 colors
        sprite.createSprite(lcd.width(), lcd.height());
        sprite.setFont(&fonts::AsciiFont8x16);
        auto pal = sprite.getPalette();
        for (auto&& p : palettes) {
            *pal++ = p;
        }
        sprite.setTextColor(3, 0);
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

        static float prev_mv{NAN};
        if (has_display && prev_mv != mv) {
            prev_mv = mv;

            const bool negative = (mv < 0.0f);
            auto abs_mv         = negative ? -mv : mv;
            auto ratio          = abs_mv / 12000.f;  // 0-12V (magnitude)
            if (ratio > 1.0f) {
                ratio = 1.0f;
            }

            constexpr int FONT_H    = 16;
            constexpr int total_h   = FONT_H + 2 + BLOCK_H;  // title + gap + voltage + gap + bar
            int y                   = (lcd.height() - total_h) / 2;
            int bar_w               = sprite.width() - MARGIN * 2;
            int fill_w              = (int)((bar_w - 2) * ratio);
            const uint8_t bar_color = negative ? 2 : 1;  // red:negative blue:positive

            sprite.clear();
            sprite.setCursor(MARGIN, y);
            sprite.printf("ADC (raw:%d)", unit.differentialValue());
            sprite.setCursor(MARGIN, y + FONT_H + 2);
            sprite.printf("%.2f mV", mv);
            int bar_y = y + FONT_H + 2 + FONT_H + 2;
            sprite.drawRect(MARGIN, bar_y, bar_w, BAR_H, 3);
            if (fill_w > 0) {
                sprite.fillRect(MARGIN + 1, bar_y + 1, fill_w, BAR_H - 2, bar_color);
            }

            lcd.startWrite();
            sprite.pushSprite(&lcd, 0, 0);
            lcd.endWrite();
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
