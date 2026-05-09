# cyd-hydro

Merged self-display sensor station firmware for the Sunton ESP32-2432S028R
(CYD). One ~$12 board reads its own DHT20 (air temp + humidity) and
DS18B20 (water temp), renders the values on its on-board 2.8" LCD, and
serves the same `/sensors` JSON contract as
[cores3-hydro](https://github.com/sharpdaddy59/cores3-hydro) so a
[hydro-dash](https://github.com/sharpdaddy59/Hydro-Desktop) unit can
discover and poll it interchangeably.

The firmware is **stateless and never alerts** — same stance as its
sibling projects. Interpretation is the upstream agent's job; this just
shows what's true right now.

## Why this exists

A growing-station node was previously a CoreS3 + DIN base (~$70) with a
camera, AXP2101 power management, and a 2" display. Replacing it with a
CYD trades the camera (used only for QR-based WiFi setup, replaceable
with WiFiManager) and ambient-light sensor reliability (CYD's LDR is a
per-unit lottery; toggleable via `USE_LDR`) for ~5× cost reduction and a
larger built-in display — which removes the need for a separate hydro-dash
unit per station.

Background: [cores3-hydro/docs/cyd-port-plan.md](https://github.com/sharpdaddy59/cores3-hydro/blob/main/docs/cyd-port-plan.md).

## Status

**Phase 1 scaffold.** Display + WiFiManager + brightness auto-dim. Sensor
drivers, mDNS advertisement, HTTP server (`/sensors`, `/status`,
`/wifi/reset`), and OTA arrive in subsequent phases — see
[`docs/cyd-hydro-spec.md`](docs/cyd-hydro-spec.md) for the changelog and
the open-work list.

## Hardware

- **Sunton ESP32-2432S028R** (CYD): ESP32-WROOM-32, 2.8" 320×240 ILI9341,
  XPT2046 resistive touch, on-board LDR (sometimes), RGB LED, microSD,
  two USB ports.
- **DHT20** module (I²C): air temp + humidity. Wires to **CN1** —
  GND / GPIO 22 (SDA) / GPIO 27 (SCL) / 3V3.
- **DS18B20** waterproof probe (1-Wire): water temp. Wires to the
  **Speaker JST** — GND / GPIO 26 (DATA). Needs an external **4.7 kΩ
  pull-up** from DATA to 3V3 if the probe doesn't ship with one.

> **Other CYD revisions:** the S028C (capacitive) and various clone
> variants have different pin maps. Update [`config.h`](config.h) and
> the panel config in [`ui.cpp`](ui.cpp) to match yours. The current
> values were dialed in empirically against the user's S028R units.

## Quick start

PowerShell on Windows:

```powershell
git clone <repo-url> cyd-hydro
cd cyd-hydro

# One-time setup: installs arduino-cli, esp32:esp32 core, and libraries.
.\setup.ps1

# Plug in the CYD over USB, then:
.\build.ps1 -Upload -Monitor
```

On first boot the device opens a `cyd-hydro-<last4mac>-setup` WiFi
network. Join it from a phone, the captive portal opens, enter your LAN
credentials, the device reboots into client mode and starts advertising
itself as `cyd-hydro-<last4mac>.local`.

## Project layout

```
cyd-hydro/
├── cyd-hydro.ino           Sketch entry, boot orchestration
├── config.h                Pins, intervals, FW_VERSION
├── state.{cpp,h}           LocalSensors struct, atomics, version-bump
├── device_id.{cpp,h}       Per-MAC unique hostname helper
├── prefs.{cpp,h}           NVS: brightness mode, rotation, touch calib
├── backlight.{cpp,h}       LDR-driven PWM auto-dim
├── ui.{cpp,h}              LovyanGFX panel config + screen state machine
├── ui_hero.{cpp,h}         Hero view (single device, four sensor rows)
├── ui_settings.{cpp,h}     Settings screen (brightness mode cycle)
├── touch.{cpp,h}           XPT2046 tap/long-press dispatch
├── wifi_setup.{cpp,h}      WiFiManager AP-mode onboarding
├── build.ps1, setup.ps1    arduino-cli wrappers
├── docs/cyd-hydro-spec.md  Full design + changelog + open work
└── CLAUDE.md               Notes for Claude Code agents
```

## License

MIT.
