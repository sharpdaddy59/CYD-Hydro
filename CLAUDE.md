# CLAUDE.md — cyd-hydro project notes for Claude Code

Merged self-display sensor station for the Sunton ESP32-2432S028R (CYD).
Reads its own DHT20 + DS18B20 (+ optional LDR) and renders the values on
its on-board LCD. Serves the same `/sensors` JSON contract as
cores3-hydro so a hydro-dash unit can discover and poll it
interchangeably.

**Sister projects (deliberate convention overlap):**
- `../cores3-hydro` — original CoreS3-based sensor firmware. Source of
  truth for the sensor JSON contract, sim-flag semantics, and OTA
  approach.
- `../hydro-dash` — desktop dashboard. Source of truth for the CYD
  panel config (LovyanGFX), backlight auto-dim with the polarity fix,
  WiFiManager onboarding, and per-MAC hostname helper.

**Authoritative design doc:** `docs/cyd-hydro-spec.md`. Read it before
non-trivial work.

## Build / flash / monitor

PowerShell, from the project root:

```powershell
.\build.ps1                  # compile only
.\build.ps1 -Upload          # compile + auto-detect port + flash
.\build.ps1 -Upload -Monitor # ... + serial @ 115200
.\build.ps1 -Strict          # warnings=all
```

One-time setup: `.\setup.ps1` installs arduino-cli, the mainstream
`esp32:esp32` core, and required libraries (LovyanGFX, WiFiManager,
ArduinoJson). DHT20 / DS18B20 / OneWire libraries land in setup.ps1
when the sensor drivers are ported in Phase 2.

**Why mainstream esp32:esp32 not M5Stack's fork:** the CYD is a plain
WROOM-32 board with no OPI PSRAM. The PSRAM-init fragility that pushed
cores3-hydro to M5Stack's fork doesn't apply, and the mainstream core
has better long-term library compatibility.

## Hardware map

See `docs/cyd-hydro-spec.md` for the full pinout. Critical pins
duplicated for fast lookup:

| Subsystem | Pin / detail |
|-----------|--------------|
| ILI9341 TFT (HSPI) | MOSI 13, MISO 12, SCLK 14, CS 15, DC 2, BL 21 |
| XPT2046 touch (VSPI) | MOSI 32, MISO 39, SCLK 25, CS 33, IRQ 36 |
| LDR (auto-dim) | GPIO 34 (sometimes — `USE_LDR` toggle) |
| RGB LED (active LOW) | R 4, G 16, B 17 |
| DHT20 (I²C) | SDA 22, SCL 27 — CN1 connector (3V3+GND on same connector) |
| DS18B20 (1-Wire) | DATA 26 — Speaker JST; needs external 4.7 kΩ pull-up |

Reference: https://randomnerdtutorials.com/esp32-cheap-yellow-display-cyd-pinout-esp32-2432s028r/

## Architecture pointers

- **Boot order** (`cyd-hydro.ino`): `state_init` → `prefs_load` →
  `backlight_begin` → `ui_begin` → `touch_begin` → `wifi_setup_begin`
  (WiFiManager) → (Phase 2) sensor tasks → mDNS advertise →
  `http_server_begin` → (Phase 3) `ota_begin`.
- **Concurrency:** sensor tasks (DHT20, DS18B20) write atomics in the
  single `LocalSensors` struct in `state.h`; consumers (UI, HTTP
  server) read without locking. Same convention as hydro-dash but with
  a single record instead of an array.
- **HTTP server** (Phase 2): synchronous `WebServer` polled from
  `loop()` via `http_server_loop()`. Same pattern as hydro-dash and
  cores3-hydro.
- **NVS** is split into per-feature namespaces: `cyd-ui` (brightness,
  rotation), `cyd-touch` (XPT2046 calibration). WiFiManager owns its
  own keys separately. Sensor sim flags will land in a `cyd-sim`
  namespace when sensor drivers are ported.
- **mDNS** advertises only — no browse. Hostname:
  `cyd-hydro-<last4mac>` per `device_id.cpp`.

## Critical gotchas (inherited from hydro-dash)

1. **CYD-S028R LovyanGFX panel config is fiddly and non-obvious.** The
   working combination, found empirically, is in `ui.cpp::LGFX_CYD`:
   `panel_width=320, panel_height=240` (swapped from chip-native
   240×320), `offset_y=80`, `setRotation(4)`, and `rgb_order=true`
   (CYD's LCD is BGR-wired; without this MADCTL bit, R/B channels
   swap). Don't switch to `LGFX_AUTODETECT` — its runtime probe gives
   a white screen on this board. Re-run the rotation/color test if you
   change the swap or offset.
2. **GPIO 21 is shared.** Backlight and the P3 expansion header both
   use it. If you wire something to P3 pin 4, the panel goes dark.
3. **GPIO 22 is "share-ish" with the touch CS pin in some board revs.**
   Bench-verify DHT20 on CN1 (which uses GPIO 22 for SDA) on your
   actual unit before designing the enclosure cutout.
4. **VSPI is shared with the SD slot.** Currently no SD use. If SD is
   added, the LovyanGFX touch driver needs `bus_shared = true` and
   explicit lock management.
5. **GPIO 35 is input-only.** Don't try to drive it as an output.
   Reserved here for a future flow-pulse counter.
6. **GPIO 26 (DS18B20)** needs an external 4.7 kΩ pull-up to 3V3.
   Many waterproof DS18B20 probes ship without one.
7. **CYD LDR polarity is inverted vs. naive divider assumption.** R10
   1MΩ pull-up to 3V3, LDR to GND, GPIO 34 between them — bright =
   LOW raw ADC, dark = HIGH. `backlight.cpp` and `BL_LDR_*` constants
   in `config.h` already handle this; don't re-derive the math.
8. **Touch calibration ships with placeholder values** in `ui.cpp`.
   First press on a fresh unit will be visibly off-axis until the
   recalibration flow lands.
9. **WiFiManager blocks** in `wifi_setup_begin()` until creds are
   submitted or `AP_TIMEOUT_S` (default 180 s) expires. On timeout we
   reboot.

## Conventions for new work

- **New screen:** add `ui_<name>.{cpp,h}` mirroring `ui_hero` /
  `ui_settings`. Wire in via `ui.cpp`'s `ui_loop` switch and
  `ui_set_screen`.
- **New sensor:** add `<sensor>.{cpp,h}` with a FreeRTOS task that
  writes the atomic in `g_sensors` and bumps `g_state_version`.
  Mirror cores3-hydro's task structure. Update `LocalSensors` in
  `state.h`, render in `ui_hero.cpp`, expose in `/sensors` (Phase 2).
- **New NVS-persisted state:** mirror existing namespaces in
  `prefs.cpp`. Separate `Preferences` namespace, load in `prefs_load`,
  save inline on change.
- **User-facing changes:** bump `FW_VERSION` in `config.h`, add a
  changelog entry to the top of `docs/cyd-hydro-spec.md`.
- **Spec doc is the source of truth** for design decisions. If
  behavior diverges, update the spec.

## Don'ts

- **Don't add alerting.** Stateless by design — same stance as the
  sister projects. The upstream agent does interpretation.
- **Don't hardcode WiFi credentials.** WiFiManager AP-mode onboarding
  is the one true path.
- **Don't add discovery / polling.** This firmware is single-device.
  If you find yourself porting `discovery.{cpp,h}` from hydro-dash,
  you're confused about which board you're on — that lives on the
  dashboard, not the sensor station.
- **Don't break the `/sensors` JSON contract.** It's shared with
  cores3-hydro and hydro-dash polls both interchangeably. Field names
  and types must match cores3-hydro's `/sensors` response verbatim.
- **Don't bump `FW_VERSION` without updating the spec changelog.**
- **Don't drag in an OS abstraction layer.** ArduinoJson + WebServer +
  LovyanGFX + WiFiManager + DHT20 + DallasTemperature/OneWire is the
  surface area; keep it small.

## Recent state

- **v0.1.2 (current):** Stop interpreting the `light` value. v0.1.1
  inverted raw ADC (`4095 - raw`) to make the published number rise
  with brightness; v0.1.2 publishes the raw 0..4095 directly with
  documented "higher = darker" semantics. The LDR is uncalibrated, so
  any normalisation was arbitrary — let the consumer decide.
  `sim_light()` range adjusted from 0..1000 to 0..4095 to match.
- **v0.1.1 (Phase 2 — sensors + HTTP):** DHT20 + DS18B20 + LDR sample
  tasks live; mDNS advertise; `/sensors`, `/status`, `/sim`,
  `/wifi/reset` HTTP endpoints. JSON shape on `/sensors` matches
  cores3-hydro (with the `light` direction caveat noted in v0.1.2).
  Sketch at 90% of the default partition's app slot — Phase 3 OTA
  needs partition scheme switch.
- **v0.1.0 (Phase 1 scaffold):** display + WiFiManager + auto-dim.
  No sensors yet. Boot orchestration in `cyd-hydro.ino` has TODO
  markers for the missing pieces. Hero view shows hostname + four
  `--` rows + IP/RSSI footer.

## Where to look first

- `cyd-hydro.ino` — boot orchestration, current Phase markers
- `docs/cyd-hydro-spec.md` — full design + contract + open work
- `config.h` — central tunables (intervals, pins, version)
- `state.h` / `state.cpp` — LocalSensors struct, atomics
- `ui.cpp` — LovyanGFX panel config (pin numbers come from `config.h`)
- `../cores3-hydro/` — sister project, source of truth for sensor
  drivers + JSON contract + sim semantics
- `../hydro-dash/` — sister project, source of truth for the CYD UI
  layer (panel config, backlight, WiFiManager, prefs, hostname)
