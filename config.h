// config.h — central tunables for cyd-hydro.
//
// Pinout matches the Sunton ESP32-2432S028R (CYD) per Random Nerd's
// pinout reference. Sensor pins per cores3-hydro/docs/cyd-port-plan.md
// §Pin map: DHT20 on CN1 (3V3/GND/SDA22/SCL27), DS18B20 on the Speaker
// JST (GPIO 26, plus external 4.7 kΩ pull-up to 3V3).
//
// If you have a different CYD revision (S028C capacitive, or one of the
// silently-different clone variants), the pin map below is the place to fix.

#pragma once

#define FW_VERSION       "0.1.2"

// ---------------------------------------------------------------------------
// Display (ILI9341, HSPI bus, 240x320 portrait native -> rotated to 320x240)
// ---------------------------------------------------------------------------
#define TFT_MOSI         13
#define TFT_MISO         12
#define TFT_SCLK         14
#define TFT_CS           15
#define TFT_DC           2
#define TFT_RST          -1   // tied to ESP32 reset; software reset only
#define TFT_BL           21   // backlight, active HIGH, PWM-capable
#define TFT_BL_PWM_CH    0
#define TFT_BL_PWM_FREQ  5000
#define TFT_BL_PWM_BITS  8

#define TFT_W            320
#define TFT_H            240

// ---------------------------------------------------------------------------
// Touch (XPT2046, separate VSPI bus — shares VSPI with SD if SD is added)
// ---------------------------------------------------------------------------
#define TOUCH_MOSI       32
#define TOUCH_MISO       39
#define TOUCH_SCLK       25
#define TOUCH_CS         33
#define TOUCH_IRQ        36

// ---------------------------------------------------------------------------
// On-board sensors / indicators
// ---------------------------------------------------------------------------
#define LDR_PIN          34   // ADC1, input-only
#define LED_R_PIN        4    // active LOW
#define LED_G_PIN        16
#define LED_B_PIN        17

// ---------------------------------------------------------------------------
// Off-board sensors (per cyd-port-plan.md §Pin map)
// ---------------------------------------------------------------------------
#define USE_LDR          1    // compile-time toggle: 0 if your board's LDR is dead
#define DHT20_SDA        22   // CN1 connector
#define DHT20_SCL        27   // CN1 connector
#define DS18B20_PIN      26   // Speaker JST; external 4.7 kΩ pull-up to 3V3 required

#define DHT20_INTERVAL_MS    5000
#define DS18B20_INTERVAL_MS  5000
#define LDR_INTERVAL_MS      5000

#define HTTP_PORT            80

// ---------------------------------------------------------------------------
// WiFi onboarding (WiFiManager AP fallback when no creds saved)
// ---------------------------------------------------------------------------
#define AP_PASSWORD              ""    // open AP; only seen during setup
#define AP_TIMEOUT_S             180

// ---------------------------------------------------------------------------
// Backlight auto-dim
//
// CYD wiring (per the Sunton schematic): R10 1MΩ pull-up to 3V3, LDR
// between GPIO 34 and GND. So bright light drops the LDR's resistance,
// pulls the tap toward GND, and produces a LOW raw ADC value. Dark
// produces a HIGH raw value. Constants below are named for the room
// condition, not the raw direction — so BL_LDR_BRIGHT < BL_LDR_DARK
// numerically. See hydro-dash backlight.cpp for the polarity-fix story
// (v0.1.5).
// ---------------------------------------------------------------------------
#define BL_MAX_DUTY              255
#define BL_MIN_DUTY              30
#define BL_LDR_BRIGHT            150   // raw ADC when room is bright (low because of CYD wiring)
#define BL_LDR_DARK              550   // raw ADC when room is dark (high because of CYD wiring)
