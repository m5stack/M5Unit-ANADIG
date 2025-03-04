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

}  // namespace

#if defined(USING_UNIT_ADC11)
using namespace m5::unit::ads1110;
#elif defined(USING_HAT_ADC11) || defined(USING_HAT_ADC)
using namespace m5::unit::ads1100;
#else
#endif

void setup()
{
    M5.begin();
    // The screen shall be in landscape mode if exists
    if (lcd.height() > lcd.width()) {
        lcd.setRotation(1);
    }

#if defined(USING_HAT_ADC11) || defined(USING_HAT_ADC)
    Wire.end();
    Wire.begin(0, 26, 400 * 1000U);
#else
    auto pin_num_sda = M5.getPin(m5::pin_name_t::port_a_sda);
    auto pin_num_scl = M5.getPin(m5::pin_name_t::port_a_scl);
    M5_LOGI("getPin: SDA:%u SCL:%u", pin_num_sda, pin_num_scl);
    Wire.end();
    Wire.begin(pin_num_sda, pin_num_scl, 400 * 1000U);
#endif

    if (!Units.add(unit, Wire) || !Units.begin()) {
        M5_LOGE("Failed to begin");
        lcd.clear(TFT_RED);
        while (true) {
            m5::utility::delay(10000);
        }
    }
    M5_LOGI("M5UnitUnified has been begun with %s", unit.deviceName());
    M5_LOGI("%s", Units.debugInfo().c_str());

    //
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

    lcd.startWrite();
    lcd.clear(TFT_BLACK);
}

void loop()
{
    M5.update();
    auto touch = M5.Touch.getDetail();

    // Periodic
    Units.update();
    if (unit.updated()) {
        auto mv = unit.differentialVoltage();
        M5.Log.printf(">Raw:%d\n>Voltage(mV):%.2f\n", unit.differentialValue(), mv);

        auto ratio  = mv / 12000.f;  // 0-12V
        int32_t deg = 300 * ratio;
        static int32_t prev_deg{-1};

        if (deg != prev_deg) {
            auto cx = lcd.width() >> 1;
            auto cy = lcd.height() >> 1;

            sprite.clear();
            sprite.fillArc(cx, cy, lcd.height() >> 1, (lcd.height() >> 1) - 16, 30, 330, 0);
            sprite.fillArc(cx, cy, lcd.height() >> 1, (lcd.height() >> 1) - 16, 30, 30 + 300 * ratio, 1);
            sprite.drawArc(cx, cy, lcd.height() >> 1, (lcd.height() >> 1) - 16, 30, 330, 3);
            sprite.setCursor(cx, cy);
            sprite.printf("%.2fmV", mv);

            sprite.pushSprite(&lcd, 0, 0);
            prev_deg = deg;
        }
    }

    // Toggle Single/Periodic
    if (M5.BtnA.wasClicked() || touch.wasClicked()) {
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
