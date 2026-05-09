// ldr.h — LDR sample task.
// Reads GPIO 34 every LDR_INTERVAL_MS, writes raw 0..4095 to
// g_sensors.light. No-op when USE_LDR=0 in config.h (in which case
// the /sensors JSON omits the light field entirely, matching the
// cores3-hydro contract for "no light sensor present").

#pragma once

void ldr_start();
