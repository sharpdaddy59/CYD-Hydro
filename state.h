// state.h — local sensor readings + atomics.
//
// Single-device firmware: this CYD reads its own DHT20 + DS18B20 + LDR
// and renders the values on its own screen. So there's just one
// LocalSensors record (not the DeviceRecord array hydro-dash uses for
// its remote-polling role). Producers (sensor tasks) write atomics;
// consumers (UI, HTTP server) read without locking.

#pragma once

#include <atomic>
#include <Arduino.h>

struct LocalSensors {
  // Latest readings (NaN until first successful read).
  std::atomic<float> water_temp;     // DS18B20, °C
  std::atomic<float> air_temp;       // DHT20, °C
  std::atomic<float> humidity;       // DHT20, %
  std::atomic<float> light;          // LDR raw 0..4095, NaN if USE_LDR=0

  // Sample-stamps in millis() since boot. The /sensors JSON exposes
  // these as ss_boot_water / ss_boot_air / ss_boot_light so consumers
  // can detect staleness without needing NTP. 0 = never sampled.
  std::atomic<uint32_t> ss_boot_water;
  std::atomic<uint32_t> ss_boot_air;
  std::atomic<uint32_t> ss_boot_light;

  // Per-sensor sim flags (drive UI colour tinting + advertised in
  // /sensors so a hydro-dash unit polling us renders them too).
  std::atomic<bool> sim_water;
  std::atomic<bool> sim_air;
  std::atomic<bool> sim_light;

  // True once any sensor has produced a value.
  std::atomic<bool> has_data;
};

extern LocalSensors g_sensors;

// Bumped whenever something display-relevant changes (new sample, sim
// flag flip, screen change). The UI loop skips a redraw when the
// version hasn't moved since last frame — kills idle flicker without
// per-element dirty-tracking. Mirrors hydro-dash's convention.
extern std::atomic<uint32_t> g_state_version;

void state_init();
void state_bump_version();
