# cyd-hydro — design spec

Merged self-display sensor station for the Sunton ESP32-2432S028R (CYD).
A single ~$12 board reads its own DHT20 + DS18B20 (+ optional LDR) and
renders the values on its on-board 2.8" LCD. Serves the same `/sensors`
JSON contract as cores3-hydro so a hydro-dash unit can discover and poll
it interchangeably.

This spec is the source of truth for design decisions. If behaviour
diverges from the spec, update the spec.

## Changelog

- **v0.1.2:** Stop interpreting the `light` value. v0.1.1 inverted the
  raw ADC (`4095 - raw`) to make the published number rise with
  brightness, ostensibly to match the direction of cores3-hydro's lux
  output. But the LDR is uncalibrated, the magnitudes were never
  comparable to lux anyway, and the inversion just added a layer of
  pseudo-meaning on top of an arbitrary number. Now we publish the raw
  12-bit ADC value directly (0..4095, **higher = darker** because of
  the CYD's high-pull-up wiring). Display matches JSON exactly.
  Consumers that want a label (Bright/Dim/Dark) or a normalised
  percentage can derive one — we don't pretend to. `sim_light()`
  range adjusted from 0..1000 to 0..4095 so sim-mode and real-mode
  values are at least directionally consistent.
- **v0.1.1 (Phase 2 — sensors + HTTP):** DHT20 (CN1, GPIO 22/27),
  DS18B20 (Speaker JST, GPIO 26 + external 4.7 kΩ pull-up), and the
  on-board LDR all read on FreeRTOS tasks. Each writes its atomic in
  `g_sensors` and bumps `g_state_version` so the hero view re-renders
  on each fresh sample. mDNS service advertise on
  `cyd-hydro-<last4mac>.local` so a hydro-dash unit on the LAN can
  discover us via shape-probing. Synchronous `WebServer` exposes
  `/sensors`, `/status`, `/sim` (GET + POST per-sensor toggle), and
  `/wifi/reset` — JSON shape on `/sensors` matches cores3-hydro
  verbatim so dashboards can poll cyd-hydro and CoreS3-hydro
  interchangeably. `simulation.{cpp,h}` and `sim_state.{cpp,h}`
  ported from cores3-hydro (sim_state under namespace `cyd-sim` to
  keep separate from a CoreS3 firmware reflashed onto the same chip).
  Note: the `light` field is published as 4095 minus the raw ADC so
  the value goes UP with brightness (matching cores3-hydro's lux
  direction) — but the magnitudes aren't comparable since the LDR is
  uncalibrated and CoreS3 reports calibrated lux from the LTR-553ALS.
  Sketch size at 1.19 MB / 1.31 MB (90% — Phase 3 OTA will need a
  partition scheme switch).
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
  "light":      3850,
  "rssi":       -45,
  "wifi_ok":    true,
  "ss_boot_water": 9120,
  "ss_boot_air":   9123,
  "ss_boot_light": 9100,
  "simulated":  { "air": false, "water": false, "light": false }
}
```

Field semantics:

- `water_temp` / `air_temp`: degrees Celsius (float). `null` when the
  sensor has never produced a reading or its last reading is stale
  (>120 s old).
- `humidity`: percent (integer). Same null semantics, gated on the
  DHT20's freshness because air + humidity share the device.
- `light`: **raw 12-bit ADC value, 0..4095. Higher = darker** because
  of the CYD's high-pull-up wiring (R10 1MΩ to 3V3, LDR to GND, GPIO
  34 between them). The LDR is uncalibrated — this is *not* lux, *not*
  a percentage, and **not directionally consistent with cores3-hydro's
  `light` field** (which reports calibrated lux from the LTR-553ALS,
  higher = brighter). A polling consumer that wants to label or
  normalise the value can do so based on its own thresholds; we
  publish the raw read and stay out of the interpretation business.
- `rssi`: live `WiFi.RSSI()` integer, dBm.
- `ss_boot_*`: seconds-since-boot at the moment of the most recent
  sample for that sensor (0 = never sampled). Consumers detect
  staleness from these without needing NTP.
- `wifi_ok`: live `WiFi.status() == WL_CONNECTED`.
- `simulated`: per-sensor sim-mode flags, true if the corresponding
  value came from `simulation.cpp` instead of real hardware.

`light`, `ss_boot_light`, and `simulated.light` are omitted entirely
when `USE_LDR=0`.

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
