/*
 * SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
  Example using M5UnitUnified for UnitDAC2
*/
#include <M5Unified.h>
#include <M5UnitUnified.h>
#include <M5UnitUnifiedANADIG.h>
#include <M5Utility.h>
#include <cmath>

namespace {
auto& lcd = M5.Display;
m5::unit::UnitUnified Units;
m5::unit::UnitDAC2 unit;

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

    auto pin_num_sda = M5.getPin(m5::pin_name_t::port_a_sda);
    auto pin_num_scl = M5.getPin(m5::pin_name_t::port_a_scl);
    M5_LOGI("getPin: SDA:%u SCL:%u", pin_num_sda, pin_num_scl);
    Wire.begin(pin_num_sda, pin_num_scl, 400 * 1000U);

    if (!Units.add(unit, Wire) || !Units.begin()) {
        M5_LOGE("Failed to begin");
        lcd.clear(TFT_RED);
        while (true) {
            m5::utility::delay(10000);
        }
    }

    // channel0: 0-5V channel1:0-10V
    unit.writeOutputRange(Output::Range5V, Output::Range10V);
    unit.writeBothVoltage(0U, 0U);

    M5_LOGI("M5UnitUnified has been begun");
    M5_LOGI("%s", Units.debugInfo().c_str());

    lcd.clear(TFT_DARKGREEN);
    lcd.setCursor(16, lcd.height() / 2);
    lcd.printf("%s", func_name_table[fidx]);
    M5_LOGI("%s", func_name_table[fidx]);
}

void loop()
{
    M5.update();
    Units.update();

    unit.writeBothVoltage(func(counter, unit.maximumVoltage(Channel::Zero)),
                          func(counter, unit.maximumVoltage(Channel::One)));
    m5::utility::delay(1);

    counter += 2;
    if (counter > 360 * 32) {
        fidx = (fidx + 1) % m5::stl::size(func_table);
        func = func_table[fidx];

        counter = 0;
        // M5.Speaker.tone(2000, 20);
        lcd.clear();
        lcd.setCursor(16, lcd.height() / 2);
        lcd.printf("%s", func_name_table[fidx]);
        M5_LOGI("%s", func_name_table[fidx]);
    }
}
