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
    float v   = sinf(rad) / fabs(sinf(rad));
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

}  // namespace

using namespace m5::unit::gp8413;

void setup()
{
    M5.begin();
    // The screen shall be in landscape mode if exists
    if (lcd.height() > lcd.width()) {
        lcd.setRotation(1);
    }

#if defined(USING_HAT_DAC2)
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
        lcd.fillScreen(TFT_RED);
        while (true) {
            m5::utility::delay(10000);
        }
    }

#if !defined(USING_UNIT_DAC)
    // channel0: 0-5V channel1:0-10V
    unit.writeOutputRange(Output::Range5V, Output::Range10V);
    unit.writeBothVoltage(0U, 0U);
#endif

    M5_LOGI("M5UnitUnified has been begun");
    M5_LOGI("%s", Units.debugInfo().c_str());

    lcd.setFont(lcd.width() > 240 ? &fonts::Font4 : &fonts::Font2);
    lcd.startWrite();

    lcd.fillScreen(TFT_BLACK);
    lcd.setTextDatum(middle_center);
    lcd.drawString(func_name_table[fidx], lcd.width() >> 1, lcd.height() >> 1);
    lcd.setTextDatum(top_left);
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
    auto touch = M5.Touch.getDetail();

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
    counter += 4;

    auto bwid = lcd.width() >> 3;

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

    // Change output function
    if (M5.BtnA.wasClicked() || touch.wasClicked()) {
        fidx    = (fidx + 1) % m5::stl::size(func_table);
        func    = func_table[fidx];
        counter = 0;

        M5.Speaker.tone(2000, 20);
        lcd.fillScreen(TFT_BLACK);
        lcd.setTextDatum(top_center);
        lcd.drawString(func_name_table[fidx], lcd.width() >> 1, lcd.height() >> 1);
        lcd.setTextDatum(top_left);
        M5.Log.printf("Output:%s\n", func_name_table[fidx]);
    }
    m5::utility::delay(8);
}
