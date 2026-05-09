// state.h — local sensor readings, sim-mode flags, and atomics.
//
// Single-device firmware: this CYD reads its own DHT20 + DS18B20 (+
// optional LDR) and renders the values on its own screen. So there's
// just one LocalSensors record (not the DeviceRecord array hydro-dash
// uses for its remote-polling role). Producers (sensor tasks) write
// atomics; consumers (UI, HTTP server) read without locking.
//
// Field names + units mirror cores3-hydro's SensorState verbatim where
// they overlap, so ported sensor drivers need only g_state → g_sensors
// renaming. Sample-stamps are in SECONDS since boot (matching the
// `ss_boot_*` JSON contract field naming).

#pragma once

#include <atomic>
#include <Arduino.h>

struct LocalSensors {
  // Latest readings (NaN / 0 until first successful read).
  std::atomic<float>    water_temp;     // DS18B20, °C
  std::atomic<float>    air_temp;       // DHT20, °C
  std::atomic<float>    humidity;       // DHT20, %
  std::atomic<uint16_t> light;          // LDR raw 0..4095, 0 if USE_LDR=0

  // Sample-stamps in SECONDS since boot. JSON exposes these as
  // ss_boot_water / ss_boot_air / ss_boot_light. 0 = never sampled.
  std::atomic<uint32_t> seconds_since_boot_water;
  std::atomic<uint32_t> seconds_since_boot_air;
  std::atomic<uint32_t> seconds_since_boot_light;

  // Per-sensor simulation overrides. When true, the sensor task uses
  // simulated values from simulation.cpp instead of reading hardware.
  // Loaded from NVS at boot via sim_state_load(); toggled at runtime
  // via POST /sim. Default is real-hardware reads (false).
  std::atomic<bool>     simulate_air;     // DHT20 (air_temp + humidity)
  std::atomic<bool>     simulate_water;   // DS18B20 (water_temp)
  std::atomic<bool>     simulate_light;   // LDR (light)

  // True once any sensor has produced a value.
  std::atomic<bool>     has_data;
};

extern LocalSensors g_sensors;

// Bumped whenever something display-relevant changes (new sample, sim
// flag flip, screen change). The UI loop skips a redraw when the
// version hasn't moved since last frame — kills idle flicker without
// per-element dirty-tracking. Mirrors hydro-dash's convention.
extern std::atomic<uint32_t> g_state_version;

void state_init();
void state_bump_version();

// Time helpers — same shape as cores3-hydro's sensors.h.
uint32_t now_seconds_since_boot();
bool     reading_is_fresh(uint32_t sensor_ss_boot, uint32_t now_ss_boot);
