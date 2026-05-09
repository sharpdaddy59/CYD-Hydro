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
// Phase 2: sensor tasks + mDNS advertise + HTTP server
// (/sensors, /status, /sim, /wifi/reset). Phase 3 (OTA) still TODO.

#include "config.h"
#include "state.h"
#include "prefs.h"
#include "sim_state.h"
#include "backlight.h"
#include "ui.h"
#include "touch.h"
#include "wifi_setup.h"
#include "device_id.h"
#include "dht20.h"
#include "ds18b20.h"
#include "ldr.h"
#include "mdns_advertise.h"
#include "http_server.h"

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.printf("[boot] cyd-hydro %s on %s\n", FW_VERSION, device_hostname());

  state_init();
  prefs_load();
  sim_state_load();   // restore per-sensor sim flags into g_sensors

  backlight_begin();
  ui_begin();
  ui_set_status("Connecting to WiFi...");

  touch_begin();

  // Blocks until WiFi up. WiFiManager opens an AP if no creds saved;
  // SSID is `<hostname>-setup` so multiple units onboarded at once
  // produce distinguishable networks.
  wifi_setup_begin();

  // Sensor tasks own their own data — start them before HTTP so
  // /sensors has values to return on the first request.
  dht20_start();
  ds18b20_start();
  ldr_start();   // no-op when USE_LDR=0

  // Advertise on mDNS so a hydro-dash unit on the LAN can discover us.
  mdns_advertise_begin();

  // Bind HTTP server. Must run after WiFiManager has released port 80
  // (which it does after autoConnect returns).
  http_server_begin();

  // TODO Phase 3: ota_begin() — streaming HTTP OTA + ArduinoOTA push.

  ui_set_status("");  // clear the connecting message
}

void loop() {
  ui_loop();
  touch_loop();
  backlight_loop();
  http_server_loop();
  delay(10);
}
