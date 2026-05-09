// dht20.cpp — DHT20 reader task.
//
// Adapted from cores3-hydro/dht20.cpp. Differences:
//   - Wire bus initialised on the CYD's CN1 pins (SDA 22, SCL 27)
//     instead of CoreS3's internal I2C.
//   - No g_wire_mutex. The CYD has no other I2C devices on this bus
//     (touch is SPI), so contention is impossible.
//   - No ota_in_progress gate (Phase 3 concern).
//   - g_state.* renamed to g_sensors.* (different struct name; same
//     field semantics).
//
// Inline driver — no external library dependency. The DHT20 is
// internally an AHT20 with a fixed I²C address (0x38) and a dead-simple
// protocol (datasheet §5.4):
//
//   Trigger:  W [0xAC, 0x33, 0x00]
//   Wait:     ~75 ms (status bit 7 = 1 means busy)
//   Read:     R 7 bytes: [status, h_hi, h_mid, h_lo|t_hi, t_mid, t_lo, crc]
//   Decode:   raw_h = (b1<<12) | (b2<<4) | (b3>>4)             — 20 bits
//             raw_t = ((b3 & 0x0F)<<16) | (b4<<8) | b5          — 20 bits
//             humidity_pct = raw_h * 100 / 2^20
//             temp_c       = raw_t * 200 / 2^20 - 50

#include <Arduino.h>
#include <Wire.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "dht20.h"
#include "state.h"
#include "config.h"
#include "simulation.h"

static constexpr uint8_t DHT20_ADDR = 0x38;

static bool dht20_trigger() {
  Wire.beginTransmission(DHT20_ADDR);
  Wire.write(0xAC);
  Wire.write(0x33);
  Wire.write(0x00);
  return Wire.endTransmission() == 0;
}

static bool dht20_read_result(float &temp_c, float &humidity_pct) {
  uint8_t buf[7];
  if (Wire.requestFrom(DHT20_ADDR, (uint8_t)7) != 7) return false;
  for (int i = 0; i < 7; ++i) {
    if (!Wire.available()) return false;
    buf[i] = Wire.read();
  }
  if (buf[0] & 0x80) return false;   // sensor still busy

  uint32_t raw_h =  ((uint32_t)buf[1] << 12)
                  | ((uint32_t)buf[2] <<  4)
                  | ((uint32_t)buf[3] >>  4);
  uint32_t raw_t =  ((uint32_t)(buf[3] & 0x0F) << 16)
                  | ((uint32_t)buf[4] <<  8)
                  | ((uint32_t)buf[5]);

  humidity_pct = (float)raw_h * (100.0f / 1048576.0f);
  temp_c       = (float)raw_t * (200.0f / 1048576.0f) - 50.0f;
  return true;
}

// One-shot presence probe. Returns true if a device ACKs at 0x38.
static bool dht20_probe() {
  Wire.beginTransmission(DHT20_ADDR);
  return Wire.endTransmission() == 0;
}

static void dht20_task(void *param) {
  (void)param;
  Serial.println("[dht20] task starting");

  // Probe once before entering the loop. If the sensor isn't there,
  // idle the hardware-read path — keep advancing the timestamp so
  // /sensors shows the task is alive, but don't keep hammering an
  // empty I²C address (which can wedge the bus driver after repeated
  // NACKs). Re-probe every 5 min in case the user plugs the unit in
  // later. Note this only matters when sim mode is OFF.
  bool present = dht20_probe();
  Serial.printf("[dht20] sensor present: %s\n", present ? "yes" : "no");

  static const uint32_t REPROBE_CYCLES = 60;   // 60 * 5s = 5 min
  uint32_t cycles_since_reprobe = 0;

  for (;;) {
    float t = NAN;
    float h = NAN;

    if (g_sensors.simulate_air.load()) {
      t = sim_air_temp();
      h = sim_humidity();
    } else if (present) {
      if (dht20_trigger()) {
        vTaskDelay(pdMS_TO_TICKS(85));   // conversion window — yields & feeds WDT
        float tt, hh;
        if (dht20_read_result(tt, hh)) {
          t = tt;
          h = hh;
        }
      }
    } else if (++cycles_since_reprobe >= REPROBE_CYCLES) {
      cycles_since_reprobe = 0;
      present = dht20_probe();
      if (present) Serial.println("[dht20] sensor detected on reprobe");
    }

    g_sensors.air_temp.store(t);
    g_sensors.humidity.store(h);
    g_sensors.seconds_since_boot_air.store(now_seconds_since_boot());
    if (!isnan(t)) g_sensors.has_data.store(true);
    state_bump_version();

    vTaskDelay(pdMS_TO_TICKS(DHT20_INTERVAL_MS));
  }
}

void dht20_start() {
  // Bring up the I²C bus on the CYD's CN1 pins. Wire.begin is idempotent;
  // calling it here keeps the wire ownership co-located with its consumer.
  Wire.begin(DHT20_SDA, DHT20_SCL);

  // 8 KB stack — Wire + Serial.printf paths can be deep. Cheap insurance.
  xTaskCreatePinnedToCore(dht20_task, "dht20", 8192, nullptr, 2, nullptr, 1);
}
