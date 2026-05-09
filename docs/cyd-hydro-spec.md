# cyd-hydro — design spec

Merged self-display sensor station for the Sunton ESP32-2432S028R (CYD).
A single ~$12 board reads its own DHT20 + DS18B20 (+ optional LDR) and
renders the values on its on-board 2.8" LCD. Serves the same `/sensors`
JSON contract as cores3-hydro so a hydro-dash unit can discover and poll
it interchangeably.

This spec is the source of truth for design decisions. If behaviour
diverges from the spec, update the spec.

## Changelog

- **v0.1.0 (Phase 1 scaffold):** repo skeleton + display + WiFiManager
  + brightness auto-dim ported from hydro-dash with the v0.1.5 LDR
  polarity fix included. Hero view shows hostname banner, four sensor
  rows (all `--` until Phase 2), and IP/RSSI footer. Settings screen
  cycles brightness mode (Auto / Full / Dim) on tap. NVS namespaces
  renamed `dash-*` → `cyd-*`. Hostname prefix changed `hydro-dash-` →
  `cyd-hydro-`. No sensors / mDNS / HTTP / OTA yet — TODO markers in
  the boot orchestration in `cyd-hydro.ino`.

## Why this exists

The growing-station node was previously a CoreS3 + DIN base (~$70). The
CYD trades the camera and ambient-light sensor (LTR-553ALS) reliability
for ~5× cost reduction and a larger built-in display — which removes
the need for a separate hydro-dash unit per station.

Background analysis: [`../../cores3-hydro/docs/cyd-port-plan.md`](../../cores3-hydro/docs/cyd-port-plan.md).

## Hardware

- **Sunton ESP32-2432S028R** (CYD): ESP32-WROOM-32, 2.8" 320×240
  ILI9341, XPT2046 resistive touch, on-board LDR (per-unit lottery),
  RGB LED, microSD, two USB ports.
- **DHT20** module (I²C): air temp + humidity. Wires to **CN1** —
  GND / GPIO 22 (SDA) / GPIO 27 (SCL) / 3V3.
- **DS18B20** waterproof probe (1-Wire): water temp. Wires to the
  **Speaker JST** — GND / GPIO 26 (DATA). Needs an external
  **4.7 kΩ pull-up** from DATA to 3V3.

### Pin map

| Subsystem | Pin(s) | Connector | Notes |
|-----------|--------|-----------|-------|
| ILI9341 TFT (HSPI) | MOSI 13, MISO 12, SCLK 14, CS 15, DC 2, BL 21 | on-board | from hydro-dash |
| XPT2046 touch (VSPI) | MOSI 32, MISO 39, SCLK 25, CS 33, IRQ 36 | on-board | from hydro-dash |
| LDR (auto-dim, optional) | GPIO 34 | on-board | `USE_LDR` toggle in config.h |
| DHT20 (I²C) | SDA 22, SCL 27 | CN1 | 3V3 + GND on same connector |
| DS18B20 (1-Wire) | DATA 26 | Speaker JST | external 4.7 kΩ pull-up to 3V3 |
| RGB LED | R 4, G 16, B 17 | on-board | active LOW |

GPIO 35 left free for a future input-only sensor (e.g. flow pulse
counter).

## Architecture

Single device. **No** discovery, **no** remote polling, **no**
DeviceRecord array. Local sensor tasks write atomics; UI reads them;
HTTP server exposes them.

### Boot order (`cyd-hydro.ino`)

```
state_init           // initialize LocalSensors atomics to NaN/false
prefs_load           // NVS — brightness mode, rotation, touch cal
backlight_begin      // PWM + LDR ADC config
ui_begin             // LovyanGFX panel init + initial render
touch_begin          // XPT2046 (config came up with the panel)
wifi_setup_begin     // WiFiManager — blocks until creds or AP timeout
sensor_tasks_begin   // (Phase 2) DHT20 + DS18B20 + LDR FreeRTOS tasks
mdns_begin           // (Phase 2) advertise hostname; no browse
http_server_begin    // (Phase 2) /sensors, /status, /wifi/reset
ota_begin            // (Phase 3) streaming HTTP OTA + ArduinoOTA push
```

### Concurrency

Sensor tasks (DHT20, DS18B20, LDR) write atomics in the single
`LocalSensors` struct in `state.h`. Consumers (UI, HTTP server) read
without locks. No mutex needed — atomics suffice for single-writer/
multi-reader on these primitive types.

### `/sensors` JSON contract

Identical to cores3-hydro's `/sensors` response. From the port plan:

```json
{
  "water_temp": 22.5,
  "air_temp":   24.1,
  "humidity":   55,
  "light":      320,
  "rssi":       -45,
  "wifi_ok":    true,
  "ss_boot_water": 9120,
  "ss_boot_air":   9123,
  "ss_boot_light": 9100,
  "simulated":  { "air": false, "water": false, "light": false }
}
```

`light` and `ss_boot_light` are omitted when `USE_LDR=0`. `ss_boot_*`
values are millis()-since-boot at the moment of the most recent
sample — consumers detect staleness without needing NTP.

## Hero view

Single-device layout, no auto-cycle, no per-peer dot strip.

```
┌─────────────────────────────────────┐
│ cyd-hydro-a3f2                      │  ← header (size 2)
├─────────────────────────────────────┤
│ Water                       22.5C   │  ← row 0 (size 3)
│ Air                         24.1C   │  ← row 1
│ Humidity                      55%   │  ← row 2
│ Light                        320    │  ← row 3
│                                     │
│ 192.168.1.42  RSSI -45              │  ← footer (size 2, dim grey)
└─────────────────────────────────────┘
```

Per-row colour:

| Colour | Meaning |
|--------|---------|
| Green | Fresh real reading |
| Yellow | Fresh but the upstream sensor is in sim mode |
| Dim grey | Never sampled, or last sample > 30 s old |

Same semantics as hydro-dash so a user moving between cyd-hydro and
hydro-dash screens sees consistent meaning.

Air and Humidity share the `sim_air` flag — both come from the same
DHT20 device.

## Settings screen

Long-press → opens Settings. On Settings:
- Tap anywhere → cycles brightness mode (Auto → Full → Dim → Auto)
- Long-press → returns to hero

Auto mode honors the LDR via `backlight.cpp`. Full and Dim pin the
backlight at `BL_MAX_DUTY` / `BL_MIN_DUTY`.

Future additions (not in v0.1.0): WiFi reset action, touch
recalibration entry point.

## NVS schema

Per-feature namespaces — wiping one doesn't disturb others.

| Namespace | Owner | Keys |
|-----------|-------|------|
| `cyd-ui` | `prefs.cpp` | `mode`, `rot`, `schema` |
| `cyd-touch` | `prefs.cpp` | `xmin`, `xmax`, `ymin`, `ymax` |
| `cyd-sim` *(Phase 2)* | sensor sim flags | `air`, `water`, `light` |
| WiFiManager-internal | WiFiManager | SSID, password |

`PREFS_SCHEMA = 1` for v0.1.0. Bump if the meaning of stored keys
changes in a backwards-incompatible way (e.g. rotation pinning).

## Open work

### Phase 2 — sensors + HTTP

- Port `dht20.{cpp,h}` from cores3-hydro; repin to GPIO 22/27.
- Port `ds18b20.{cpp,h}` from cores3-hydro; repin to GPIO 26.
- Port `sim_state.{cpp,h}` from cores3-hydro (verbatim).
- Add LDR sample task (write `g_sensors.light` if `USE_LDR=1`).
- Port `http_server.{cpp,h}`; serve `/sensors`, `/status`,
  `/wifi/reset`. Reuse hydro-dash's synchronous-`WebServer`-from-loop
  pattern.
- Add mDNS advertisement on the device hostname.
- Verify a hydro-dash unit on the LAN discovers + polls cyd-hydro
  via shape probing.

### Phase 3 — OTA

- Port `ota.{cpp,h}` from cores3-hydro, **streaming variant** (write
  each upload chunk to flash as it arrives — no PSRAM buffer on the
  WROOM-32).
- Keep ArduinoOTA push as a second path.
- Add `ui_ota.{cpp,h}` — full-screen progress display during upload.
- Verify partition scheme leaves room for two ~1.3 MB app slots on
  the WROOM's 4 MB flash (`min_spiffs` or custom CSV).

### Phase 4 — polish

- Adapt hydro-dash's parametric SCAD enclosure to add a cable gland
  for the sensor leads.
- Touch recalibration UI flow (replaces the placeholder values in
  `ui.cpp`).
- WiFi reset action on the Settings screen.
- Optional: `USE_LDR` runtime detection (read 16 samples at all four
  attenuations; if all zero, mark dead and force backlight = Full).

## Don'ts (architectural guardrails)

- **No alerting.** Stateless by design — interpretation is the
  upstream agent's job.
- **No hardcoded WiFi credentials.** WiFiManager is the one true
  onboarding path.
- **No discovery / polling.** This firmware is single-device. If
  you find yourself porting `discovery.{cpp,h}` from hydro-dash,
  you're on the wrong codebase.
- **Don't break the `/sensors` JSON contract.** Field names and
  types must match cores3-hydro's `/sensors` verbatim.
- **No OS abstraction layer.** ArduinoJson + WebServer + LovyanGFX +
  WiFiManager + DHT20 + DallasTemperature/OneWire is the surface;
  keep it small.
