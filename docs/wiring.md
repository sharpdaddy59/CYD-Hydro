# cyd-hydro — sensor wiring harness

This doc describes how to build a single wiring harness that connects
both sensors (Grove DHT20 air-temp/humidity + Grove DS18B20 water-temp
probe) to the Sunton CYD board. Once built, it's plug-and-play: two
1.25 mm JST plugs into the CYD, two Grove plugs into the sensors.

You only need to build this once per station. The complete harness has:

- **Two CYD-side connectors:** a 4-pin 1.25 mm JST (mates CN1) and a
  2-pin 1.25 mm JST (mates the Speaker JST).
- **Two sensor-side connectors:** two 4-pin Grove (mate the DHT20 and
  DS18B20 modules respectively).
- **One internal 3V3 Y-splice** so both sensors share CN1's 3V3 rail.

If you'd rather not build it yourself, see the [alternatives section](#alternatives)
at the end.

---

## Bill of materials

| Item | Qty | Notes |
|---|---|---|
| Grove DHT20 module + 4-pin Grove cable | 1 | Cable is the standard "wires-on-one-end, Grove-on-the-other" pigtail Seeed ships |
| Grove DS18B20 waterproof probe + 4-pin Grove cable | 1 | Has the 4.7 kΩ pull-up integrated on the module — no external resistor needed |
| 1.25 mm JST 4-pin male pigtail | 1 | Mates CN1. Search "JST GH 1.25mm 4-pin connector with wire" |
| 1.25 mm JST 2-pin male pigtail | 1 | Mates the Speaker JST. Same JST GH 1.25mm family |
| Heat-shrink (1.5 mm and 5 mm) | a few cm of each | 1.5 mm for individual wire splices; 5 mm for the bundle strain relief |
| Soldering iron + solder | — | The 3V3 splice and the JST crimps need it |

The two CYD-side JST pigtails come pre-crimped if you buy them as
"with wire" assemblies — you just trim them to length and solder the
free ends to the Grove cable wires. If you have crimping tools and
loose JST GH crimps, you can build them yourself, but the pigtails are
~$0.50 each on AliExpress and a lot less fiddly.

---

## Pin map

| CYD connector / pin | Wire colour (Grove) | Sensor / pin | Notes |
|---|---|---|---|
| CN1 pin 1 (GND) | Black | DHT20 pin 4 (GND) | straight-through |
| CN1 pin 2 (GPIO 22) | Yellow | DHT20 pin 1 (SDA) | straight-through |
| CN1 pin 3 (GPIO 27) | White | DHT20 pin 2 (SCL) | straight-through |
| CN1 pin 4 (3V3) | Red | DHT20 pin 3 (VCC) **AND** DS18B20 pin 3 (VCC) | **Y-splice** — 3V3 feeds both sensors |
| Speaker JST pin 1 (GND) | Black | DS18B20 pin 4 (GND) | straight-through |
| Speaker JST pin 2 (GPIO 26) | Yellow | DS18B20 pin 1 (DATA) | straight-through |
| — | DS18B20 White (pin 2) | NC | cut short or leave un-crimped |

---

## Topology

```
       CYD side                                       Sensor side
  (1.25 mm JST male                                   (Grove 4-pin
   pigtails on cable)                                  on the existing
                                                       sensor cables)

  CN1 (4-pin)                                  DHT20 Grove (4-pin)
  ┌──────────────┐                            ┌──────────────────────┐
  │ 1: GND   ────┼──── Black ────────────────►│ 4: GND    (Black)    │
  │ 2: GPIO22────┼──── Yellow ───────────────►│ 1: SDA    (Yellow)   │
  │ 3: GPIO27────┼──── White ────────────────►│ 2: SCL    (White)    │
  │ 4: 3V3   ────┼──── Red ──────┬───────────►│ 3: VCC    (Red)      │
  └──────────────┘               │            └──────────────────────┘
                                 │
                          Y-splice (solder
                          both Red wires
                          to a single trunk
                          wire from CN1 pin
                          4; heat-shrink)
                                 │
                                 │             DS18B20 Grove (4-pin)
                                 │            ┌──────────────────────┐
                                 └───────────►│ 3: VCC    (Red)      │
                                              │ 2: NC     (White —   │
                                              │           cut off)   │
  Speaker JST (2-pin)                         │                      │
  ┌──────────────┐                            │                      │
  │ 1: GND   ────┼──── Black ────────────────►│ 4: GND    (Black)    │
  │ 2: GPIO26────┼──── Yellow ───────────────►│ 1: DATA   (Yellow)   │
  └──────────────┘                            └──────────────────────┘
```

---

## Pin numbering note

- **JST 1.25 mm:** pin 1 is marked on the cable housing with a small
  triangle / dot, and on the CYD's silkscreen near each connector.
  Verify with the silkscreen before crimping — getting 3V3 backwards
  onto GPIO 22 would smoke the LDR divider and possibly the ESP32.
- **Grove 4-pin:** pin 1 (yellow / SIG) is on the keyed side of the
  connector. Universally consistent across Seeed Grove modules.

---

## Build sequence

1. **Cut the bare-wire ends** of both Grove pigtails to ~10 cm length.
2. **CN1 side.** Take the DHT20 Grove cable's 4 bare wires and crimp
   the 1.25 mm JST 4-pin housing onto them. Wire colours per the pin
   map above (Black → pin 1, Yellow → pin 2, White → pin 3, Red → pin 4).
   **Verify pin 1 location against the CYD silkscreen before
   committing.**
3. **Speaker JST side.** Take the DS18B20 Grove cable's Black and
   Yellow wires and crimp the 1.25 mm JST 2-pin housing onto them
   (Black → pin 1, Yellow → pin 2). Cut the DS18B20 Grove cable's
   White wire short and heat-shrink the stub (it's NC on the DS18B20
   module).
4. **3V3 Y-splice.** Splice the DS18B20 Grove cable's Red wire into the
   DHT20 Grove cable's Red wire, a few cm back from the CN1-side JST.
   Heat-shrink each splice individually, then a larger piece of
   heat-shrink over the whole splice area for strain relief.
5. **Bundle.** Tidy up the loose lengths between the splice point and
   the two Grove ends with cable management of your choice (sleeve,
   spiral wrap, or just tape).

---

## Verification (before plugging into the CYD)

Beep these out with a multimeter at the four connector ends:

| Continuity check | Expected |
|---|---|
| DHT20 pin 4 (Black) ↔ CN1 pin 1 | continuous |
| DHT20 pin 1 (Yellow) ↔ CN1 pin 2 | continuous |
| DHT20 pin 2 (White) ↔ CN1 pin 3 | continuous |
| DHT20 pin 3 (Red) ↔ CN1 pin 4 | continuous |
| DS18B20 pin 3 (Red) ↔ CN1 pin 4 | continuous (via Y-splice) |
| DS18B20 pin 4 (Black) ↔ Speaker JST pin 1 | continuous |
| DS18B20 pin 1 (Yellow) ↔ Speaker JST pin 2 | continuous |
| DHT20 Yellow ↔ DS18B20 Yellow | **open** (different signals) |
| Any Red ↔ any Black | **open** (no power short!) |
| CN1 pin 4 (3V3) ↔ Speaker JST pin 1 (GND) | **open** |

If all ten checks pass, plug the four connectors in and power on. The
Water / Air / Humidity rows on the hero screen should turn green
within ~5 seconds. The Light row is independent of this harness — it
reads the on-board LDR.

---

## Troubleshooting

- **Air + Humidity rows stay grey.** DHT20 not detected. Check the
  CN1-side wiring: Yellow → GPIO 22 (SDA), White → GPIO 27 (SCL). On
  serial monitor (`arduino-cli monitor -p COMx -c baudrate=115200`)
  look for `[dht20] sensor present: no` — that confirms the I²C probe
  failed.
- **Water row stays grey.** DS18B20 not detected. Check the Yellow wire
  goes from Speaker JST pin 2 to DS18B20 pin 1 (DATA). The Grove
  DS18B20 has the 4.7 kΩ pull-up on the module, so if power is
  reaching it, DATA should pull cleanly.
- **Both sensor groups go grey simultaneously.** Likely a 3V3 issue —
  verify the Y-splice is intact and that CN1 pin 4 actually has 3.3 V
  at the JST end of your harness (not soldered to GND by mistake).
- **CYD won't boot or smokes.** Power-supply short — disconnect
  immediately and re-do the CN1-side beep tests. Reversing 3V3 and
  GND in the JST will damage the LDR divider and possibly the ESP32.

---

## Alternatives

If this build is too much, the cyd-hydro spec discusses two other
approaches:

- **Add a small breakout PCB** with the 1.25 mm JSTs on one side and
  Grove sockets on the other. Same wiring; cleaner result; needs PCB
  design + fab.
- **Add a DS2484 I²C-to-1-Wire bridge chip** so both sensors run
  entirely off CN1's I²C bus. Genuinely "everything on one connector"
  but adds a discrete chip per station and a library dependency. See
  the v0.1.x discussion in [`cyd-hydro-spec.md`](cyd-hydro-spec.md).

---

## See also

- [`cyd-hydro-spec.md`](cyd-hydro-spec.md) — full design + JSON
  contract + open work
- [`config.h`](../config.h) — pin assignments (search for
  `DHT20_SDA`, `DHT20_SCL`, `DS18B20_PIN`)
- [cores3-hydro/docs/cyd-port-plan.md](https://github.com/sharpdaddy59/cores3-hydro/blob/main/docs/cyd-port-plan.md)
  — original port plan including the pin-map analysis that picked
  these specific GPIOs out of the limited free-pin pool on the CYD
