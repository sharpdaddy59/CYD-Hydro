#include "ui_hero.h"
#include "ui.h"
#include "state.h"
#include "config.h"
#include "device_id.h"
#include <WiFi.h>
#include <cmath>
#include <cstring>

// Layout (single device, no cycling, no per-peer dot strip):
//   y= 0..23   Header — hostname banner.
//   y=24..199  Hero region — 4 sensor rows at text size 3.
//   y=200..239 Footer — IP + RSSI at text size 2.
static constexpr int HEADER_H      = 24;
static constexpr int HEADER_TEXT_Y = 4;
static constexpr int HERO_TOP      = HEADER_H + 8;
static constexpr int HERO_LINE0_Y  = HERO_TOP + 4;
static constexpr int HERO_LINE_DY  = 38;
static constexpr int RIGHT_PAD     = 8;

// uint16_t (not uint32_t) so LovyanGFX's color path treats values as
// RGB565. Same gotcha hydro-dash hit at v0.1.4 — uint32_t dispatches
// to the RGB888 overload and the bytes get reinterpreted, turning
// TFT_GREEN into red.
//
// Staleness threshold lives in state.cpp::reading_is_fresh (same
// SENSOR_STALE_S that the /sensors HTTP handler uses, so the colour
// in the UI agrees with the null-vs-value decision in the JSON).
static uint16_t row_color(uint32_t ss_boot, bool simulated) {
  if (!reading_is_fresh(ss_boot, now_seconds_since_boot())) return 0x7BEF;
  if (simulated)                                            return TFT_YELLOW;
  return TFT_GREEN;
}

static void draw_header() {
  auto& g = ui_gfx();
  int W = g.width();
  g.fillRect(0, FY(0, HEADER_H), W, HEADER_H, TFT_BLACK);
  g.drawFastHLine(0, FY(HEADER_H - 1), W, 0x4208);

  g.setTextColor(TFT_WHITE, TFT_BLACK);
  g.setTextSize(2);
  g.setCursor(8, FY(HEADER_TEXT_Y, 16));
  g.print(device_hostname());
}

static void draw_readings() {
  auto& g = ui_gfx();
  int W = g.width();
  int H = g.height();

  g.fillRect(0, FY(HEADER_H, H - HEADER_H), W, H - HEADER_H, TFT_BLACK);

  g.setTextSize(3);
  g.setTextWrap(false);
  const int LINE_H = 24;

  auto reading = [&](int line, const char* label, float v, const char* unit,
                     uint32_t ss_boot, bool simulated) {
    int y = HERO_LINE0_Y + line * HERO_LINE_DY;
    uint16_t color = row_color(ss_boot, simulated);
    g.setTextColor(color, (uint16_t)TFT_BLACK);
    g.setCursor(8, FY(y, LINE_H));
    g.print(label);

    char buf[16];
    if (isnan(v))                     snprintf(buf, sizeof(buf), "--");
    else if (strcmp(unit, "%") == 0)  snprintf(buf, sizeof(buf), "%.0f%s", v, unit);
    else if (unit[0] == '\0')         snprintf(buf, sizeof(buf), "%.0f", v);
    else                              snprintf(buf, sizeof(buf), "%.1f%s", v, unit);
    int tw = g.textWidth(buf);
    g.setCursor(W - RIGHT_PAD - tw, FY(y, LINE_H));
    g.print(buf);
  };

  // Air and Humidity share simulate_air — same DHT20 device on the sensor side.
  reading(0, "Water",    g_sensors.water_temp.load(),     "C",
          g_sensors.seconds_since_boot_water.load(), g_sensors.simulate_water.load());
  reading(1, "Air",      g_sensors.air_temp.load(),       "C",
          g_sensors.seconds_since_boot_air.load(),   g_sensors.simulate_air.load());
  reading(2, "Humidity", g_sensors.humidity.load(),       "%",
          g_sensors.seconds_since_boot_air.load(),   g_sensors.simulate_air.load());
  reading(3, "Light",    (float)g_sensors.light.load(),   "",
          g_sensors.seconds_since_boot_light.load(), g_sensors.simulate_light.load());

  // Footer: IP + RSSI in dim grey for low visual weight.
  const int FOOTER_LINE_H = 16;
  int footer_y = H - FOOTER_LINE_H - 4;
  g.setTextSize(2);
  g.setTextColor(TFT_DARKGREY, TFT_BLACK);
  g.setCursor(8, FY(footer_y, FOOTER_LINE_H));
  if (WiFi.status() == WL_CONNECTED) {
    g.printf("%s  RSSI %d", WiFi.localIP().toString().c_str(), (int)WiFi.RSSI());
  } else {
    g.print("WiFi: not connected");
  }
}

void ui_hero_draw() {
  draw_header();
  draw_readings();
}

void ui_hero_handle_touch(int16_t /*x*/, int16_t /*y*/) {
  // Single device → no focus, no pause, no cycle. Long-press still
  // dispatches to Settings via ui.cpp. Tap on the hero pane is a
  // no-op for now; reserved for a possible "tap to cycle brightness
  // mode without going through settings" shortcut.
  state_bump_version();
}

void ui_hero_tick() {
  // No-op. Kept for parity with hydro-dash's ui.cpp dispatch table —
  // reserved for future per-frame animations (sample-fresh pulse, etc.).
}
