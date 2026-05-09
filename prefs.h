// prefs.h — NVS-backed user preferences.
//
// Single device → no manual host list, no per-device aliases. Just
// brightness mode, screen rotation (pinned to 4 in practice but stored
// for future use), and touch calibration.

#pragma once

#include <Arduino.h>

enum BrightnessMode : uint8_t {
  BRIGHTNESS_AUTO = 0,
  BRIGHTNESS_FULL = 1,
  BRIGHTNESS_DIM  = 2,
};

void prefs_load();
void prefs_save();

BrightnessMode prefs_brightness_mode();
void           prefs_set_brightness_mode(BrightnessMode m);

uint8_t prefs_rotation();   // 0..3 (LovyanGFX setRotation)
void    prefs_set_rotation(uint8_t r);

// Touch calibration matrix (XPT2046 raw -> screen coords).
struct TouchCal { int16_t x_min, x_max, y_min, y_max; };
bool prefs_load_touch_cal(TouchCal& out);
void prefs_save_touch_cal(const TouchCal& cal);
