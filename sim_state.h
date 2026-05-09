// sim_state.h — per-sensor simulation override flags, persisted to NVS.
//
// Flags themselves live in g_sensors.simulate_air / simulate_water /
// simulate_light (see state.h). These functions handle loading them
// from NVS at boot and saving them when the user toggles via POST /sim.
//
// NVS namespace: "cyd-sim". Keys: "air", "water", "light" (bool).
//
// Adapted from cores3-hydro's sim_state — same behaviour, different
// namespace ("sim" → "cyd-sim") to keep the two firmwares' NVS state
// separate when the same chip is reflashed back and forth.

#pragma once

// Load all three flags from NVS into g_sensors. Missing keys default
// to false (real hardware reads). Call once during setup() after
// state_init().
void sim_state_load();

// Save the corresponding flag's current g_sensors value to NVS. Called
// after each successful POST /sim that flipped the flag. No-op if NVS
// write fails.
void sim_state_save_air();
void sim_state_save_water();
void sim_state_save_light();
