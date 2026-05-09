// simulation.h — random-value generators used when a sensor's per-sensor
// simulation override is set (g_sensors.simulate_air / simulate_water /
// simulate_light). Verbatim port from cores3-hydro; ranges match the
// CoreS3 sensor spec so a hydro-dash unit polling either firmware sees
// equivalent value distributions.

#pragma once

#include <stdint.h>

float    sim_air_temp();    // 18.0 - 30.0 °C
float    sim_humidity();    // 40 - 80 %
float    sim_water_temp();  // 20.0 - 28.0 °C
uint16_t sim_light();       // 0 - 1000 (CoreS3 reports lux; on CYD this is "raw-ish" — see ldr.cpp)
