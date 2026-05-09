#include "state.h"
#include <cmath>

LocalSensors          g_sensors;
std::atomic<uint32_t> g_state_version{0};

void state_bump_version() {
  g_state_version.fetch_add(1, std::memory_order_relaxed);
}

void state_init() {
  g_sensors.water_temp.store(NAN);
  g_sensors.air_temp.store(NAN);
  g_sensors.humidity.store(NAN);
  g_sensors.light.store(NAN);
  g_sensors.ss_boot_water.store(0);
  g_sensors.ss_boot_air.store(0);
  g_sensors.ss_boot_light.store(0);
  g_sensors.sim_water.store(false);
  g_sensors.sim_air.store(false);
  g_sensors.sim_light.store(false);
  g_sensors.has_data.store(false);
}
