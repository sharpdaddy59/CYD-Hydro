// ldr.cpp — LDR sample task.
//
// Cadence is independent of backlight.cpp's own LDR sampling — that one
// runs at 2 Hz inside backlight_loop() to drive the auto-dim duty
// cycle, and only when brightness mode = AUTO. The /sensors `light`
// field needs to be available in all brightness modes, so this task
// does its own analogRead. Same pin, same attenuation; concurrent
// reads on ADC1 from two contexts are safe in arduino-esp32.

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "ldr.h"
#include "state.h"
#include "config.h"
#include "simulation.h"

#if USE_LDR

static void ldr_task(void *param) {
  (void)param;
  pinMode(LDR_PIN, INPUT);
  // Match backlight.cpp's ADC config so both readers see the same
  // voltage→count mapping.
  analogReadResolution(12);
  analogSetPinAttenuation(LDR_PIN, ADC_6db);

  for (;;) {
    uint16_t value;
    if (g_sensors.simulate_light.load()) {
      value = sim_light();
    } else {
      // Burst-of-3 + take last. The CYD's high-impedance LDR divider
      // (R10 = 1MΩ) means the ADC's S/H cap can take a couple samples
      // to settle to the divider voltage — discarding the first two
      // and keeping the third matches the pattern the diagnostic at
      // hydro-dash/docs/cyd-ldr-test settled on.
      analogRead(LDR_PIN);
      analogRead(LDR_PIN);
      value = (uint16_t)analogRead(LDR_PIN);
    }
    // Publish the raw 12-bit ADC value (0..4095) directly. We don't
    // invert, normalise, or pretend it's lux — this is an uncalibrated
    // LDR, and any "interpretation" would be arbitrary anyway.
    // Direction is determined by CYD wiring (R10 1MΩ pull-up to 3V3,
    // LDR to GND): bright light pulls the tap toward GND so raw goes
    // DOWN; dark lets the pull-up dominate so raw goes UP. Documented
    // in docs/cyd-hydro-spec.md §`/sensors` so consumers know.
    g_sensors.light.store(value);
    g_sensors.seconds_since_boot_light.store(now_seconds_since_boot());
    g_sensors.has_data.store(true);
    state_bump_version();

    vTaskDelay(pdMS_TO_TICKS(LDR_INTERVAL_MS));
  }
}

void ldr_start() {
  xTaskCreatePinnedToCore(ldr_task, "ldr", 4096, nullptr, 2, nullptr, 1);
}

#else

void ldr_start() {
  // USE_LDR=0 — no task, no analogRead. light stays 0 and
  // ss_boot_light stays at its initial 0, which makes /sensors emit
  // null for `light` (matches the cores3-hydro contract for "no light
  // sensor present").
}

#endif
