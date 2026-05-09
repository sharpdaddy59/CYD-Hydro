// sim_state.cpp — NVS-backed persistence for per-sensor sim override flags.
//
// Adapted from cores3-hydro's sim_state.cpp. Two changes:
//   - NVS namespace renamed "sim" → "cyd-sim" so the two firmwares
//     don't trample each other's state when reflashed back and forth.
//   - g_state.simulate_* renamed to g_sensors.simulate_* (different
//     struct name in cyd-hydro; same field semantics).

#include <Arduino.h>
#include <Preferences.h>

#include "sim_state.h"
#include "state.h"

static const char *NS_SIM = "cyd-sim";

void sim_state_load() {
  Preferences prefs;
  if (!prefs.begin(NS_SIM, /*readOnly=*/true)) {
    Serial.println("[sim] no saved sim state; defaulting all to real hardware");
    return;
  }
  bool air   = prefs.getBool("air",   false);
  bool water = prefs.getBool("water", false);
  bool light = prefs.getBool("light", false);
  prefs.end();

  g_sensors.simulate_air.store(air);
  g_sensors.simulate_water.store(water);
  g_sensors.simulate_light.store(light);

  Serial.printf("[sim] loaded: air=%s water=%s light=%s\n",
                air   ? "sim" : "real",
                water ? "sim" : "real",
                light ? "sim" : "real");
}

static void save_one(const char *key, bool value) {
  Preferences prefs;
  if (!prefs.begin(NS_SIM, /*readOnly=*/false)) {
    Serial.printf("[sim] save %s FAILED: NVS open error\n", key);
    return;
  }
  prefs.putBool(key, value);
  prefs.end();
}

void sim_state_save_air()   { save_one("air",   g_sensors.simulate_air.load());   }
void sim_state_save_water() { save_one("water", g_sensors.simulate_water.load()); }
void sim_state_save_light() { save_one("light", g_sensors.simulate_light.load()); }
