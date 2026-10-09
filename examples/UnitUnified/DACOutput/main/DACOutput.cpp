/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
  Example using M5UnitUnified for UnitDAC/UnitDAC2/HatDAC2
*/
#include <M5Unified.h>
#include <M5UnitUnified.h>
#include <M5UnitUnifiedANADIG.h>
#include <M5Utility.h>
#include <wiring/m5_unit_unified_wiring.hpp>  // Board-aware I2C wiring helpers
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
LGFX_Sprite sprite{};
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

constexpr int MARGIN{4};
constexpr int BAR_H{12};
constexpr int CH_BLOCK_H{30};  // 16(voltage) + 2(gap) + 12(bar)
constexpr int FONT_H{16};

void draw_channel(const int y, const int ch, const float v, const float maxV, const uint8_t bar_color)
{
    int bar_w = sprite.width() - MARGIN * 2;

    sprite.setCursor(MARGIN, y);
    sprite.printf("Ch:%d %.2f mV", ch, v);

    int bar_y  = y + FONT_H + 2;
    int fill_w = (int)((bar_w - 2) * (v / maxV));
    sprite.drawRect(MARGIN, bar_y, bar_w, BAR_H, 3);
    if (fill_w > 0) {
        sprite.fillRect(MARGIN + 1, bar_y + 1, fill_w, BAR_H - 2, bar_color);
    }
}

}  // namespace

using namespace m5::unit::gp8413;

void setup()
{
    auto m5cfg = M5.config();
#if defined(USING_HAT_DAC2)
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

    // ePaper panels are not suitable for continuous redraw
    has_display = !lcd.isEPD() && (lcd.width() > 0 && lcd.height() > 0);

#if defined(USING_HAT_DAC2)
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

#if !defined(USING_UNIT_DAC)
    unit.writeOutputRange(Output::Range5V, Output::Range5V);
    unit.writeBothVoltage(0U, 0U);
#endif

    M5_LOGI("M5UnitUnified has been begun");
    M5_LOGI("%s", Units.debugInfo().c_str());

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
    M5.Log.printf("Output:%s\n", func_name_table[fidx]);
}

void loop()
{
    static float pv0{}, pv1{};
#if defined(USING_UNIT_DAC)
    static float max_0{unit.config().saturation_voltage};
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

    if (has_display && (pv0 != v0 || pv1 != v1)) {
        pv0 = v0;
        pv1 = v1;

#if defined(USING_UNIT_DAC)
        constexpr int num_ch = 1;
#else
        constexpr int num_ch = 2;
#endif
        int total_h = FONT_H + 2 + CH_BLOCK_H * num_ch + (num_ch - 1) * 4;
        int y       = (lcd.height() - total_h) / 2;

        sprite.clear();
        sprite.setCursor(MARGIN, y);
        sprite.printf("%s", func_name_table[fidx]);

        int ch_y = y + FONT_H + 2;
        draw_channel(ch_y, 0, v0, max_0, 2);
#if !defined(USING_UNIT_DAC)
        draw_channel(ch_y + CH_BLOCK_H + 4, 1, v1, max_1, 1);
#endif

        lcd.startWrite();
        sprite.pushSprite(&lcd, 0, 0);
        lcd.endWrite();
    }

    // Change output function
    if (M5.BtnA.wasClicked()) {
        fidx    = (fidx + 1) % m5::stl::size(func_table);
        func    = func_table[fidx];
        counter = 0;

        pv0 = -1.f;
        pv1 = -1.f;
        M5.Speaker.tone(2000, 20);
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
}
