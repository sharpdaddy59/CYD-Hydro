// simulation.h — random-value generators used when a sensor's per-sensor
// simulation override is set (g_sensors.simulate_air / simulate_water /
// simulate_light). Air/water/humidity ranges ported verbatim from
// cores3-hydro. Light range diverges: cyd-hydro publishes raw ADC
// (0..4095, higher = darker), so sim_light() matches that scale and
// direction rather than cores3-hydro's lux scale (0..1000, higher =
// brighter). See ldr.cpp + docs/cyd-hydro-spec.md §`/sensors`.

#pragma once

#include <stdint.h>

float    sim_air_temp();    // 18.0 - 30.0 °C
float    sim_humidity();    // 40 - 80 %
float    sim_water_temp();  // 20.0 - 28.0 °C
uint16_t sim_light();       // 0 - 4095 raw ADC (uncalibrated, higher = darker)
