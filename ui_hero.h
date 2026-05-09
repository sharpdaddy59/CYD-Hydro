// ui_hero.h — single-device hero view for cyd-hydro.
//
// This board reads its own sensors (no remote polling), so the layout
// is much simpler than hydro-dash's: no auto-cycle, no per-device
// status-dot strip. Just four big sensor rows with per-row colour
// tinting from the sim flags, and a footer with hostname + IP for
// diagnosability.
//
// Tap → toggles brightness mode (long-press still opens Settings,
// dispatched in ui.cpp). Settings screen owns the brightness cycle UX
// already; the tap-on-hero behavior is just convenience for someone
// who doesn't want to long-press into settings just to dim the screen.

#pragma once

#include <Arduino.h>

void ui_hero_draw();
void ui_hero_handle_touch(int16_t x, int16_t y);

// Tick hook for ui.cpp — kept for parity with hydro-dash even though
// there's no auto-cycle to advance. No-op for now; reserved for
// future per-frame animations (e.g. sample-fresh pulse).
void ui_hero_tick();
