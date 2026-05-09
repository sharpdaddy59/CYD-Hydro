#include "prefs.h"
#include <Preferences.h>

// Per-feature NVS namespaces — wiping one doesn't disturb others.
static Preferences s_ui;       // namespace "cyd-ui"
static Preferences s_touch;    // namespace "cyd-touch"

// Bump this whenever the meaning of stored prefs changes in a way that
// existing values would be invalid (e.g. we change the panel config in
// ui.cpp and the old rotation value would produce garbled output).
// On boot, mismatched schema triggers a one-time reset to safe defaults.
static constexpr uint8_t PREFS_SCHEMA = 1;

static BrightnessMode s_mode = BRIGHTNESS_AUTO;
static uint8_t        s_rot  = 4;             // CYD landscape (panel swap + offset_y=80 in ui.cpp)

void prefs_load() {
  s_ui.begin("cyd-ui", true);
  uint8_t schema = s_ui.getUChar("schema", 0);
  s_ui.end();

  if (schema != PREFS_SCHEMA) {
    // First boot of this build (or a schema-incompatible upgrade).
    // Reset to safe defaults and write the new schema number.
    s_mode = BRIGHTNESS_AUTO;
    s_rot  = 4;
    s_ui.begin("cyd-ui", false);
    s_ui.putUChar("mode",   (uint8_t)s_mode);
    s_ui.putUChar("rot",    s_rot);
    s_ui.putUChar("schema", PREFS_SCHEMA);
    s_ui.end();
  } else {
    s_ui.begin("cyd-ui", true);
    s_mode = (BrightnessMode)s_ui.getUChar("mode", BRIGHTNESS_AUTO);
    s_rot  = s_ui.getUChar("rot", 4);
    s_ui.end();
  }
}

void prefs_save() {
  s_ui.begin("cyd-ui", false);
  s_ui.putUChar("mode", (uint8_t)s_mode);
  s_ui.putUChar("rot",  s_rot);
  s_ui.end();
}

BrightnessMode prefs_brightness_mode()                    { return s_mode; }
void           prefs_set_brightness_mode(BrightnessMode m) { s_mode = m; prefs_save(); }
uint8_t        prefs_rotation()                            { return s_rot;  }
void           prefs_set_rotation(uint8_t r)               { s_rot = r;  prefs_save(); }

bool prefs_load_touch_cal(TouchCal& out) {
  s_touch.begin("cyd-touch", true);
  bool ok = s_touch.isKey("xmin");
  if (ok) {
    out.x_min = s_touch.getShort("xmin", 300);
    out.x_max = s_touch.getShort("xmax", 3900);
    out.y_min = s_touch.getShort("ymin", 300);
    out.y_max = s_touch.getShort("ymax", 3900);
  }
  s_touch.end();
  return ok;
}
void prefs_save_touch_cal(const TouchCal& cal) {
  s_touch.begin("cyd-touch", false);
  s_touch.putShort("xmin", cal.x_min);
  s_touch.putShort("xmax", cal.x_max);
  s_touch.putShort("ymin", cal.y_min);
  s_touch.putShort("ymax", cal.y_max);
  s_touch.end();
}
