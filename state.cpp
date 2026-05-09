#include "state.h"
#include "config.h"
#include <cmath>

LocalSensors          g_sensors;
std::atomic<uint32_t> g_state_version{0};

// Reading is "fresh" if its sample-stamp is within SENSOR_STALE_S of
// the current uptime. 0 (never sampled) is always stale. Mirrors
// cores3-hydro's helper of the same name.
static constexpr uint32_t SENSOR_STALE_S = 120;

void state_bump_version() {
  g_state_version.fetch_add(1, std::memory_order_relaxed);
}

void state_init() {
  g_sensors.water_temp.store(NAN);
  g_sensors.air_temp.store(NAN);
  g_sensors.humidity.store(NAN);
  g_sensors.light.store(0);
  g_sensors.seconds_since_boot_water.store(0);
  g_sensors.seconds_since_boot_air.store(0);
  g_sensors.seconds_since_boot_light.store(0);
  g_sensors.simulate_air.store(false);
  g_sensors.simulate_water.store(false);
  g_sensors.simulate_light.store(false);
  g_sensors.has_data.store(false);
}

uint32_t now_seconds_since_boot() {
  return millis() / 1000;
}

bool reading_is_fresh(uint32_t sensor_ss_boot, uint32_t now_ss_boot) {
  if (sensor_ss_boot == 0) return false;
  return (now_ss_boot - sensor_ss_boot) <= SENSOR_STALE_S;
}
