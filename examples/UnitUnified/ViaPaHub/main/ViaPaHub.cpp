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
  The screen plots the DAC (yellow), DAC2 (cyan) and ADC (red) values. With UnitDAC2 VOUT0 connected to
  the UnitADC11 input, the ADC trace overlaps the DAC2 trace.

  Each PaHub channel is an independent I2C bus, so units with the same address can also be connected.
  For example, to use a second UnitDAC, declare another instance and add it to a free channel
  (e.g. hub.add(dac_2nd, 3)).
*/
#include <algorithm>

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

// Plot: DAC / DAC2 / ADC traces sweep from left to right (the ADC trace overlaps DAC2 when looped back)
constexpr uint16_t trace_colors[3]{TFT_YELLOW, TFT_CYAN, TFT_RED};  // DAC, DAC2, ADC (RGB565)
constexpr const char* trace_labels[3]{"DAC", "DAC2", "ADC"};
constexpr int32_t SWEEP_STEPS{160};  // Steps per screen width
constexpr int32_t CURSOR_GAP{4};     // Blank columns ahead of the sweep

bool has_display{};
float plot_max_mv{};
int32_t plot_x{}, plot_top{}, plot_h{}, plot_dx{};
int32_t prev_y[3]{};
float adc_mv{};

int32_t to_y(const float mv)
{
    const float v = m5::stl::clamp(mv, 0.0f, plot_max_mv);
    return plot_top + plot_h - 1 - static_cast<int32_t>(v * (plot_h - 1) / plot_max_mv);
}

void draw_header(const float (&mv)[3])
{
    const int32_t w = lcd.width() / 3;
    lcd.fillRect(0, 0, lcd.width(), plot_top, TFT_BLACK);
    for (int i = 0; i < 3; ++i) {
        lcd.setTextColor(trace_colors[i], TFT_BLACK);
        lcd.drawString(m5::utility::formatString("%s:%4.0f", trace_labels[i], mv[i]).c_str(), w * i + 2, 2);
    }
}

void plot(const float (&mv)[3])
{
    int32_t y[3]{};
    for (int i = 0; i < 3; ++i) {
        y[i] = to_y(mv[i]);
    }

    lcd.startWrite();
    draw_header(mv);
    const int32_t x0 = plot_x;
    const int32_t x1 = plot_x + plot_dx;
    if (x1 >= lcd.width()) {
        // Wrap around: restart from the left edge
        plot_x = 0;
        lcd.fillRect(0, plot_top, plot_dx + CURSOR_GAP, plot_h, TFT_BLACK);
    } else {
        lcd.fillRect(x0 + 1, plot_top, plot_dx + CURSOR_GAP, plot_h, TFT_BLACK);
        for (int i = 0; i < 3; ++i) {
            lcd.drawLine(x0, prev_y[i], x1, y[i], trace_colors[i]);
        }
        plot_x = x1;
    }
    lcd.endWrite();

    for (int i = 0; i < 3; ++i) {
        prev_y[i] = y[i];
    }
}

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

    has_display = !lcd.isEPD() && (lcd.width() > 0 && lcd.height() > 0);
    if (has_display) {
        lcd.setTextSize(std::max<int32_t>(1, lcd.width() / 320));
        lcd.setFont(&fonts::Font2);
        plot_top    = lcd.fontHeight() + 4;
        plot_h      = lcd.height() - plot_top;
        plot_dx     = std::max<int32_t>(1, lcd.width() / SWEEP_STEPS);
        plot_max_mv = dac2.maximumVoltage(m5::unit::gp8413::Channel::Zero);
        for (auto& y : prev_y) {
            y = to_y(0.0f);
        }
        lcd.fillScreen(TFT_BLACK);
    }
}

void loop()
{
    static uint32_t step{};
    static m5::utility::elapsed_time_t last_ms{m5::utility::millis()};

    M5.update();
    Units.update();

    // ADC (periodic measurement)
    if (adc.updated()) {
        adc_mv = adc.differentialVoltage();
        M5.Log.printf(">ADC(mV):%.2f\n", adc_mv);
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

        if (has_display) {
            const float mv[3]{dac_mv, dac2_mv, adc_mv};
            plot(mv);
        }
    }
}

#if !defined(ARDUINO)
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_timer.h>

#if CONFIG_FREERTOS_UNICORE
// Single-core SoCs: run loop() back-to-back, but every ~2 s yield a 5 ms slice so the IDLE task
// runs and feeds the task watchdog (default 5 s).
static inline void feedIdleTaskPeriodically(void)
{
    constexpr uint32_t FEED_INTERVAL_MS{2000};
    constexpr TickType_t FEED_SLEEP_TICKS{pdMS_TO_TICKS(5)};
    static uint32_t s_last_feed_ms{};
    const uint32_t now_ms{static_cast<uint32_t>(esp_timer_get_time() / 1000)};
    if (now_ms - s_last_feed_ms >= FEED_INTERVAL_MS) {
        s_last_feed_ms = now_ms;
        vTaskDelay(FEED_SLEEP_TICKS);
    }
}
#endif

extern "C" void app_main(void)
{
    setup();
    for (;;) {
#if CONFIG_FREERTOS_UNICORE
        feedIdleTaskPeriodically();
#endif
        loop();
    }
}
#endif
