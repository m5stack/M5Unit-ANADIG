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
#include <wiring/m5_unit_unified_wiring.hpp>  // Board-aware I2C wiring helpers

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

    // The screen shall be in landscape mode if exists
    if (lcd.height() > lcd.width()) {
        lcd.setRotation(1);
    }

#if defined(USING_HAT_ADC11) || defined(USING_HAT_ADC)
    // Hat: board-aware Hat I2C (StickC series Wire / NessoN1 Wire1)
    const bool unit_ready = m5::unit::wiring::addHatI2C(Units, unit) && Units.begin();
#else
    // Board-aware I2C: NessoN1 -> SoftwareI2C(port_b), NanoC6/NanoH2 -> Ex_I2C, others -> Wire
    const bool unit_ready = m5::unit::wiring::addI2C(Units, unit) && Units.begin();
#endif
    if (!unit_ready) {
        M5_LOGE("Failed to begin");
        M5_LOGW("%s", Units.debugInfo().c_str());
        m5::unit::wiring::failStop();
    }

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
