// ds18b20.cpp — DS18B20 reader task.
//
// Adapted from cores3-hydro/ds18b20.cpp. Differences:
//   - Pin: DS18B20_PIN (GPIO 26, Speaker JST) instead of cores3-hydro's
//     PIN_DS18B20_DATA (GPIO 9, Port B).
//   - 4.7 kΩ pull-up to 3V3 must be added externally on the CYD —
//     unlike the Grove DS18B20 unit cores3-hydro uses, which has the
//     pull-up integrated. Document this in cyd-hydro/README.md.
//   - No ota_in_progress gate (Phase 3 concern).
//   - g_state.* renamed to g_sensors.* (different struct name; same
//     field semantics).
//
// The 750 ms conversion is handled with vTaskDelay() — yielding to the
// scheduler inherently feeds the task watchdog. Do not disable the WDT.

#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "ds18b20.h"
#include "state.h"
#include "config.h"
#include "simulation.h"

static OneWire           s_one_wire(DS18B20_PIN);
static DallasTemperature s_ds(&s_one_wire);

static void ds18b20_task(void *param) {
  (void)param;

  s_ds.begin();
  s_ds.setWaitForConversion(false);   // we manage the wait via vTaskDelay

  for (;;) {
    float t = NAN;

    if (g_sensors.simulate_water.load()) {
      t = sim_water_temp();
    } else {
      s_ds.requestTemperatures();
      vTaskDelay(pdMS_TO_TICKS(800));   // conversion window — yields & feeds WDT
      t = s_ds.getTempCByIndex(0);
      if (t == DEVICE_DISCONNECTED_C) t = NAN;
    }

    g_sensors.water_temp.store(t);
    g_sensors.seconds_since_boot_water.store(now_seconds_since_boot());
    if (!isnan(t)) g_sensors.has_data.store(true);
    state_bump_version();

    vTaskDelay(pdMS_TO_TICKS(DS18B20_INTERVAL_MS));
  }
}

void ds18b20_start() {
  xTaskCreatePinnedToCore(ds18b20_task, "ds18b20", 4096, nullptr, 2, nullptr, 1);
}
