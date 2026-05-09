// cyd-hydro.ino — entry point.
//
// Merged self-display sensor station running on the Sunton ESP32-2432S028R
// (CYD). One board reads its own DHT20 + DS18B20 (+ optional LDR) and
// renders the values on its on-board LCD. Same `/sensors` JSON contract
// as cores3-hydro so an external hydro-dash unit on the LAN can discover
// and poll us interchangeably.
//
// Pin map in config.h. Full design in docs/cyd-hydro-spec.md.
//
// Boot sequence is intentionally explicit — same convention as
// cores3-hydro and hydro-dash. Order matters: NVS first (UI prefs feed
// later steps), display before WiFi (so the user sees a "connecting"
// screen), sensors after WiFi (mDNS + HTTP server land on top).
//
// Phase 1 scaffold: display + WiFiManager only. Sensor tasks, mDNS
// advertise, HTTP server (/sensors, /status), and OTA arrive in
// subsequent commits — placeholders kept in the boot order with TODO
// markers so the structure is visible.

#include "config.h"
#include "state.h"
#include "prefs.h"
#include "backlight.h"
#include "ui.h"
#include "touch.h"
#include "wifi_setup.h"
#include "device_id.h"

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.printf("[boot] cyd-hydro %s on %s\n", FW_VERSION, device_hostname());

  state_init();
  prefs_load();

  backlight_begin();
  ui_begin();
  ui_set_status("Connecting to WiFi...");

  touch_begin();

  // Blocks until WiFi up. WiFiManager opens an AP if no creds saved;
  // SSID is `<hostname>-setup` so multiple units onboarded at once
  // produce distinguishable networks.
  wifi_setup_begin();

  // TODO Phase 2: sensor_tasks_begin() — DHT20 + DS18B20 + (optional) LDR
  //                read tasks; write LocalSensors atomics + bump version.
  // TODO Phase 2: mdns_begin() — advertise hostname (no browse, single device).
  // TODO Phase 2: http_server_begin() — /sensors + /status + /wifi/reset.
  // TODO Phase 3: ota_begin() — streaming HTTP OTA + ArduinoOTA push.

  ui_set_status("");  // clear the connecting message
}

void loop() {
  ui_loop();
  touch_loop();
  backlight_loop();
  // TODO Phase 2: http_server_loop();
  delay(10);
}
