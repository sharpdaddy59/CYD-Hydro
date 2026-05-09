// http_server.cpp — /sensors, /status, /sim, /wifi/reset handlers.
//
// Adapted from cores3-hydro/http_server.cpp. Differences:
//   - /snapshot dropped (no camera).
//   - /ota and ota_register dropped (Phase 3 work).
//   - /hostname dropped — v0.1.0 has no runtime rename feature.
//   - /status drops M5.Power (battery), PSRAM (none on WROOM), and
//     NTP fields (no NTP sync; ss_boot_* values are seconds-since-
//     boot which the agent can interpret without wall-clock).
//   - Landing page rewritten — much smaller, since we have far fewer
//     features to expose.
//   - g_state.* renamed g_sensors.* throughout (different struct
//     name; same field semantics).
//
// JSON contract for /sensors is preserved verbatim so a hydro-dash
// unit can poll a cyd-hydro device interchangeably with a CoreS3
// hydro device.

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>

#include <cmath>

#include "http_server.h"
#include "state.h"
#include "config.h"
#include "wifi_setup.h"
#include "sim_state.h"
#include "device_id.h"

static WebServer s_server(HTTP_PORT);

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
static void emit_float_or_null(JsonDocument &doc, const char *key, float v, bool fresh) {
  if (!fresh || std::isnan(v)) doc[key] = nullptr;
  else                          doc[key] = v;
}

// ---------------------------------------------------------------------------
// /sensors — JSON contract preserved verbatim from cores3-hydro
// ---------------------------------------------------------------------------
static void handle_sensors() {
  uint32_t now = now_seconds_since_boot();

  bool fresh_air   = reading_is_fresh(g_sensors.seconds_since_boot_air.load(),   now);
  bool fresh_water = reading_is_fresh(g_sensors.seconds_since_boot_water.load(), now);
  bool fresh_light = reading_is_fresh(g_sensors.seconds_since_boot_light.load(), now);

  JsonDocument doc;
  emit_float_or_null(doc, "water_temp", g_sensors.water_temp.load(), fresh_water);
  emit_float_or_null(doc, "air_temp",   g_sensors.air_temp.load(),   fresh_air);

  float h = g_sensors.humidity.load();
  if (fresh_air && !std::isnan(h)) doc["humidity"] = (int)h;
  else                              doc["humidity"] = nullptr;

#if USE_LDR
  if (fresh_light) doc["light"] = g_sensors.light.load();
  else             doc["light"] = nullptr;
#else
  // USE_LDR=0 — omit the field entirely (matches the cyd-port-plan
  // contract: "omit or null if USE_LDR=false"). We choose omit so a
  // hydro-dash polling consumer can render the row as "--".
#endif

  doc["rssi"]          = (int)WiFi.RSSI();
  doc["ss_boot_water"] = g_sensors.seconds_since_boot_water.load();
  doc["ss_boot_air"]   = g_sensors.seconds_since_boot_air.load();
#if USE_LDR
  doc["ss_boot_light"] = g_sensors.seconds_since_boot_light.load();
#endif
  doc["wifi_ok"]       = (WiFi.status() == WL_CONNECTED);

  // Per-sensor sim flags so the consumer knows which fields it can
  // trust. True = value came from sim_*() generators; false = real
  // hardware read.
  JsonObject sim = doc["simulated"].to<JsonObject>();
  sim["air"]   = g_sensors.simulate_air.load();
  sim["water"] = g_sensors.simulate_water.load();
#if USE_LDR
  sim["light"] = g_sensors.simulate_light.load();
#endif

  String out;
  serializeJson(doc, out);
  s_server.send(200, "application/json", out);
}

// ---------------------------------------------------------------------------
// /status — system health
// ---------------------------------------------------------------------------
static void handle_status() {
  JsonDocument doc;
  doc["uptime"]     = now_seconds_since_boot();
  doc["rssi"]       = (int)WiFi.RSSI();
  doc["heap_free"]  = ESP.getFreeHeap();
  doc["wifi_ok"]    = (WiFi.status() == WL_CONNECTED);
  doc["fw_version"] = FW_VERSION;
  doc["hostname"]   = device_hostname();

  String out;
  serializeJson(doc, out);
  s_server.send(200, "application/json", out);
}

// ---------------------------------------------------------------------------
// /sim — per-sensor simulation override management.
//
//   GET  /sim                              → JSON of current flags
//   POST /sim?air=on&water=off&light=auto  → set one or more flags
//
// Accepted values for each flag (case-insensitive):
//   on, true,  1, sim   → simulate
//   off, false, 0, auto → real hardware
//
// Adapted verbatim from cores3-hydro.
// ---------------------------------------------------------------------------
static int parse_sim_value(const String &v) {
  String s = v;
  s.toLowerCase();
  if (s == "on"  || s == "true"  || s == "1" || s == "sim")  return 1;
  if (s == "off" || s == "false" || s == "0" || s == "auto") return 0;
  return -1;
}

static void emit_sim_state(int code) {
  JsonDocument doc;
  doc["air"]   = g_sensors.simulate_air.load();
  doc["water"] = g_sensors.simulate_water.load();
  doc["light"] = g_sensors.simulate_light.load();
  String out;
  serializeJson(doc, out);
  s_server.send(code, "application/json", out);
}

static void handle_sim() {
  if (s_server.method() == HTTP_GET) {
    emit_sim_state(200);
    return;
  }
  if (s_server.method() != HTTP_POST) {
    s_server.send(405, "text/plain", "use GET or POST");
    return;
  }

  bool changed_air = false, changed_water = false, changed_light = false;

  auto try_apply = [&](const char *name, std::atomic<bool> &flag, bool &changed) -> int {
    if (!s_server.hasArg(name)) return 0;
    int v = parse_sim_value(s_server.arg(name));
    if (v < 0) return -1;
    bool desired = (v == 1);
    if (flag.load() != desired) {
      flag.store(desired);
      changed = true;
    }
    return 0;
  };

  if (try_apply("air",   g_sensors.simulate_air,   changed_air)   < 0) {
    s_server.send(400, "text/plain", "bad value for air");
    return;
  }
  if (try_apply("water", g_sensors.simulate_water, changed_water) < 0) {
    s_server.send(400, "text/plain", "bad value for water");
    return;
  }
  if (try_apply("light", g_sensors.simulate_light, changed_light) < 0) {
    s_server.send(400, "text/plain", "bad value for light");
    return;
  }

  if (changed_air)   sim_state_save_air();
  if (changed_water) sim_state_save_water();
  if (changed_light) sim_state_save_light();
  if (changed_air || changed_water || changed_light) state_bump_version();

  emit_sim_state(200);
}

// ---------------------------------------------------------------------------
// POST /wifi/reset — wipe stored WiFi creds and reboot into AP mode.
// LAN-trusted, no auth. Useful to recover a unit that ended up on the
// wrong network without physical access.
// ---------------------------------------------------------------------------
static void handle_wifi_reset() {
  if (s_server.method() != HTTP_POST) {
    s_server.send(405, "text/plain", "use POST");
    return;
  }
  Serial.println("[http] /wifi/reset received; rebooting to AP setup");
  s_server.send(200, "text/plain",
                "Wi-Fi credentials cleared. Device will reboot in 2s "
                "and open the AP-setup network.");
  delay(500);
  wifi_setup_reset_and_reboot();
  // unreachable
}

// ---------------------------------------------------------------------------
// GET / — small landing page
// ---------------------------------------------------------------------------
static void handle_root() {
  static const char *kHome =
    "<!doctype html>\n"
    "<html lang=\"en\">\n"
    "<head>\n"
    "  <meta charset=\"utf-8\">\n"
    "  <title>cyd-hydro</title>\n"
    "  <meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">\n"
    "  <style>\n"
    "    body { font: 15px system-ui, sans-serif; max-width: 560px;\n"
    "           margin: 2em auto; padding: 0 1em; color:#222; }\n"
    "    h1 { margin: 0 0 0.2em; }\n"
    "    h2 { margin-top: 1.6em; border-bottom:1px solid #ddd; padding-bottom:0.2em; }\n"
    "    code { background:#eee; padding:2px 5px; border-radius:3px; }\n"
    "    a { color:#0366d6; text-decoration:none; }\n"
    "    a:hover { text-decoration:underline; }\n"
    "    li { margin: 0.4em 0; }\n"
    "    .muted { color:#666; }\n"
    "    button.danger { background:#d4332a; color:white; border:none;\n"
    "                    padding: 0.5em 1em; border-radius:4px; cursor:pointer;\n"
    "                    font: inherit; }\n"
    "    button.danger:hover { background:#a82822; }\n"
    "  </style>\n"
    "</head>\n"
    "<body>\n"
    "  <h1>cyd-hydro</h1>\n"
    "  <p class=\"muted\">Self-display sensor station on Sunton CYD. Firmware " FW_VERSION ".</p>\n"
    "\n"
    "  <h2>Live data</h2>\n"
    "  <ul>\n"
    "    <li><a href=\"/sensors\">/sensors</a> &mdash; latest readings (JSON)</li>\n"
    "    <li><a href=\"/status\">/status</a> &mdash; system health (JSON)</li>\n"
    "    <li><a href=\"/sim\">/sim</a> &mdash; per-sensor sim mode (JSON, GET/POST)</li>\n"
    "  </ul>\n"
    "\n"
    "  <h2>Admin</h2>\n"
    "  <p>\n"
    "    <button class=\"danger\" onclick=\"resetWifi()\">Reset Wi-Fi credentials</button>\n"
    "    <span class=\"muted\">&nbsp; &mdash; wipes saved Wi-Fi and reboots into AP setup mode.</span>\n"
    "  </p>\n"
    "  <p id=\"fb\" class=\"muted\"></p>\n"
    "\n"
    "  <script>\n"
    "  async function resetWifi() {\n"
    "    if (!confirm('Wipe Wi-Fi credentials and reboot the device into AP setup mode?')) return;\n"
    "    try {\n"
    "      await fetch('/wifi/reset', {method:'POST'});\n"
    "      document.getElementById('fb').textContent = 'Device is rebooting...';\n"
    "    } catch(e) { document.getElementById('fb').textContent = 'Failed: ' + e; }\n"
    "  }\n"
    "  </script>\n"
    "</body>\n"
    "</html>\n";
  s_server.send(200, "text/html", kHome);
}

// ---------------------------------------------------------------------------
// 404
// ---------------------------------------------------------------------------
static void handle_not_found() {
  s_server.send(404, "text/plain", "not found");
}

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------
void http_server_begin() {
  s_server.on("/",           handle_root);
  s_server.on("/sensors",    handle_sensors);
  s_server.on("/status",     handle_status);
  s_server.on("/sim",        handle_sim);
  s_server.on("/wifi/reset", handle_wifi_reset);

  s_server.onNotFound(handle_not_found);
  s_server.begin();
  Serial.printf("[http] server up on port %d\n", HTTP_PORT);
}

void http_server_loop() {
  s_server.handleClient();
}
