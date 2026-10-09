/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
  Example of using UnitADC11 / UnitDAC / UnitDAC2 via UnitPaHub (PaHub2)

  Device ---> PaHub2 (0x70)
                |- ch:0 UnitADC11
                |- ch:1 UnitDAC
                `- ch:2 UnitDAC2

  UnitDAC and UnitDAC2 output a sawtooth wave, and UnitADC11 measures its input.
  Connecting a DAC output to the UnitADC11 input lets you see the output on the ADC side.

  Each PaHub channel is an independent I2C bus, so units with the same address can also be connected.
  For example, to use a second UnitDAC, declare another instance and add it to a free channel
  (e.g. hub.add(dac_2nd, 3)).
*/
#include <M5Unified.h>
#include <M5UnitUnified.h>
#include <M5UnitUnifiedHUB.h>
#include <M5UnitUnifiedANADIG.h>
#include <wiring/m5_unit_unified_wiring.hpp>  // Board-aware connection helpers (include last)

namespace {
auto& lcd = M5.Display;
m5::unit::UnitUnified Units;

// The default address of PaHub2 is 0x70 (changeable with the A0-A2 pins (v2.0) or the DIP switch (v2.1))
// https://docs.m5stack.com/en/unit/pahub2
m5::unit::UnitPaHub2 hub{};

m5::unit::UnitADC11 adc{};
m5::unit::UnitDAC dac{};
m5::unit::UnitDAC2 dac2{};

// PaHub channels used by each unit
constexpr uint8_t CH_ADC{0};
constexpr uint8_t CH_DAC{1};
constexpr uint8_t CH_DAC2{2};

// Sawtooth wave: STEPS steps per period, one step every STEP_INTERVAL_MS
constexpr uint32_t STEPS{50};
constexpr uint32_t STEP_INTERVAL_MS{100};

}  // namespace

void setup()
{
    M5.begin();
    M5.setTouchButtonHeightByRatio(100);
    // The screen shall be in landscape mode
    if (lcd.height() > lcd.width()) {
        lcd.setRotation(1);
    }

    if (!hub.add(adc, CH_ADC) || !hub.add(dac, CH_DAC) || !hub.add(dac2, CH_DAC2)) {
        M5_LOGE("Failed to add");
        m5::unit::wiring::failStop();
    }

    // The hub is the I2C device on the port
    // NessoN1 -> SoftwareI2C (M5HAL), NanoC6 / NanoH2 -> M5.Ex_I2C, others -> Wire
    if (!m5::unit::wiring::addI2C(Units, hub) || !Units.begin()) {
        M5_LOGE("Failed to begin");
        M5_LOGW("%s", Units.debugInfo().c_str());
        m5::unit::wiring::failStop();
    }

    // UnitDAC2: 0-5V on both channels
    dac2.writeOutputRange(m5::unit::gp8413::Output::Range5V, m5::unit::gp8413::Output::Range5V);

    M5_LOGI("M5UnitUnified initialized");
    M5_LOGI("%s", Units.debugInfo().c_str());
    lcd.fillScreen(TFT_DARKGREEN);
}

void loop()
{
    static uint32_t step{};
    static m5::utility::elapsed_time_t last_ms{m5::utility::millis()};

    M5.update();
    Units.update();

    // ADC (periodic measurement)
    if (adc.updated()) {
        M5.Log.printf(">ADC(mV):%.2f\n", adc.differentialVoltage());
    }

    // DAC / DAC2 (sawtooth wave)
    const m5::utility::elapsed_time_t now{m5::utility::millis()};
    if (m5::utility::elapsedSince(last_ms, now) >= STEP_INTERVAL_MS) {
        last_ms           = now;
        const float ratio = static_cast<float>(step) / (STEPS - 1);
        step              = (step + 1) % STEPS;

        const float dac_mv  = dac.config().saturation_voltage * ratio;
        const float dac2_mv = dac2.maximumVoltage(m5::unit::gp8413::Channel::Zero) * ratio;
        dac.writeVoltage(dac_mv);
        dac2.writeBothVoltage(dac2_mv);
        M5.Log.printf(">DAC(mV):%.2f\n>DAC2(mV):%.2f\n", dac_mv, dac2_mv);
    }
}
