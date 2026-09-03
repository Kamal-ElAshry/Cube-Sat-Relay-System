# 3-Node Arduino CubeSat Relay System — Design & Reference

Three independent Arduino projects: `Station_A_Soil/`, `CubeSat/`, `Station_B/`.
Each folder is a complete, independently-compilable Arduino sketch (open the
`.ino` file directly in the Arduino IDE).

```
Station A (Nano)  --Link A-->  CubeSat (UNO)  --Link B-->  Station B (Nano)  --USB 115200-->  LabVIEW
   "Soil"                     "Cube" (relay)                    "B"
```

---

## 1. Architecture

- **Station A**: reads DHT11 + rain + soil moisture + LDR every `SENSOR_INTERVAL_MS`,
  transmits one `SoilPacket` every `RF_SEND_INTERVAL_MS`. Transmit-only, no RX role.
- **CubeSat**: single nRF24 radio, time-multiplexed. Continuously listens for
  `SoilPacket`s and caches the latest valid one; independently reads MPU6050 +
  BMP180 every `SENSOR_INTERVAL_MS`; every `RELAY_TX_INTERVAL_MS` it briefly
  switches to TX to relay the cached Soil packet (unmodified) plus its own
  `CubePacket`, then returns to listening.
- **Station B**: pure receiver. Dispatches on `packet.id`, keeps two independent
  last-valid-value caches (Soil, Cube), prints exactly one deterministic line
  to `Serial` per `PRINT_INTERVAL_MS`. No debug text ever reaches that port.

## 2. RF protocol — two independent links, one mechanism

Each link (`SOIL_TO_CUBE_MODE` in Station A / CubeSat, `CUBE_TO_STATIONB_MODE`
in CubeSat / Station B) is independently set to one of:

```cpp
enum class RfDistributionMode : uint8_t {
    ONE_TO_ONE,             // 1 destination, ACK enabled, tracked
    SHARED_MULTI_RECEIVER,  // shared address, ACK disabled, anonymous listeners
    EXPLICIT_MULTI_RECEIVER // N destinations, ACK enabled, each tracked independently
};
```

`ONE_TO_ONE` and `EXPLICIT_MULTI_RECEIVER` share one code path in `radio.cpp`
(`_distributeSend`/the address-cycling loop) — `ONE_TO_ONE` is simply a
destination count of 1. `SHARED_MULTI_RECEIVER` uses the nRF24's per-packet
**NO_ACK** flag (`radio.write(buf, len, true)`), which is honored by the
receiving hardware regardless of that pipe's own ACK setting. This is what
lets the CubeSat's single physical radio run **two different modes on its
two links simultaneously** without any global ACK-toggling when it switches
between listening (Link A) and transmitting (Link B).

**To change a link's mode**: edit exactly one `constexpr RfDistributionMode …`
line in that link's `radio_config.h` (on *both* ends of the link — see
section 6). No other file needs to change.

Because `SHARED_MULTI_RECEIVER` never receives a per-receiver ACK, the
application only ever learns "the radio finished transmitting," never
"receiver X got it." `ONE_TO_ONE`/`EXPLICIT_MULTI_RECEIVER` track a
`ReceiverStatus{lastSuccess, failureCount, lastAttemptMs}` per destination,
re-attempted next cycle — never immediately retried in a blocking loop.

## 3. Packet definitions

### SoilPacket — 14 bytes (verified via compile, see below)

| field | type | scale | meaning |
|---|---|---|---|
| packetId | uint8_t | — | `PACKET_SOIL` |
| dhtTempC_x10 | int16_t | ×10 | DHT11 temperature |
| dhtHumidity_x10 | uint16_t | ×10 | DHT11 humidity |
| rainRaw | uint16_t | — | raw ADC 0–1023 |
| rainState | uint8_t | — | `RAIN_NONE` / `RAIN_DETECTED` |
| soilRaw | uint16_t | — | raw ADC 0–1023 |
| soilPct | uint8_t | — | 0–100, clamped |
| soilState | uint8_t | — | `SOIL_DRY` / `SOIL_NORMAL` / `SOIL_OVERWATERED` |
| lightState | uint8_t | — | `LIGHT_DARK` / `LIGHT_DETECTED` (digital sensor, no raw value) |
| statusFlags | uint8_t | — | bit0 DHT_VALID, bit1 RAIN_VALID, bit2 SOIL_VALID, bit3 LIGHT_VALID |

### CubePacket — 22 bytes

| field | type | scale | meaning |
|---|---|---|---|
| packetId | uint8_t | — | `PACKET_CUBE` |
| accelX/Y/Z | int16_t ×3 | raw | MPU6050 raw registers |
| gyroX/Y/Z | int16_t ×3 | raw | MPU6050 raw registers |
| bmpTempC_x10 | int16_t | ×10 | BMP180 temperature |
| bmpPressurePa | uint32_t | raw | BMP180 pressure, Pascals |
| bmpAltitudeM_x10 | int16_t | ×10 | BMP180 altitude, relative to `BMP_REFERENCE_PRESSURE_PA` |
| statusFlags | uint8_t | — | bit0 MPU_VALID, bit1 BMP_VALID |

Both are validated at compile time:
```cpp
static_assert(sizeof(SoilPacket) <= RF_PAYLOAD_SIZE, ...); // 14 <= 32
static_assert(sizeof(CubePacket) <= RF_PAYLOAD_SIZE, ...); // 22 <= 32
```
Both structs use `#pragma pack(push, 1)` so no compiler can silently insert
padding, and both were confirmed to compile to exactly 14 and 22 bytes
(see "Verification" below).

### Fixed 32-byte RF envelope

The nRF24 is configured with a fixed, non-dynamic `setPayloadSize(32)`. Every
transmit zero-pads a local buffer to 32 bytes before copying the logical
packet into its front; every receive reads a full 32-byte buffer and only
`memcpy`s `sizeof(the matching struct)` bytes back out after checking
`buffer[0]` against the expected packet ID. Padding bytes are never
interpreted as telemetry. This keeps the transport size fixed even if a
logical packet's fields change later, as long as it stays ≤32 bytes.

## 4. Pin tables

| | Station A (Nano) | CubeSat (UNO) | Station B (Nano) |
|---|---|---|---|
| SPI (hardware) | MOSI D11 / MISO D12 / SCK D13 | MOSI D11 / MISO D12 / SCK D13 | MOSI D11 / MISO D12 / SCK D13 |
| nRF24 CE / CSN | D9 / D10 | D9 / D10 | D9 / D10 |
| I2C (hardware) | — | SDA A4 / SCL A5 (MPU6050 @0x68, BMP180 @0x77) | — |
| DHT11 | D2 | — | — |
| Rain sensor AO | A0 | — | — |
| Rain sensor DO | D3 (wired, unused — analog preferred per spec) | — | — |
| Soil moisture AO | A1 | — | — |
| LDR DO | D4 | — | — |
| USB Serial | — | — | TX0/RX0 @ 115200 |

No SPI, I2C, analog, or digital pin conflicts on any of the three boards.

## 5. Folder / module structure

```
Project/
├── DESIGN.md
├── Station_A_Soil/
│   ├── Station_A_Soil.ino
│   ├── config.h                 pins, calibration, thresholds
│   ├── radio_config.h           Link A mode + addresses + RF params
│   ├── packet_types.h           shared protocol definitions (identical in all 3 projects)
│   ├── radio.h / radio.cpp      RF transport abstraction
│   ├── dht_sensor.h/.cpp
│   ├── rain_sensor.h/.cpp
│   ├── soil_moisture.h/.cpp
│   ├── light_sensor.h/.cpp      digital-only LDR
│   └── soil_telemetry.h/.cpp    aggregates the 4 sensors into a SoilPacket
├── CubeSat/
│   ├── CubeSat.ino
│   ├── config.h
│   ├── radio_config.h           BOTH links' modes + addresses + RF params
│   ├── packet_types.h
│   ├── radio.h / radio.cpp      RX/TX time-multiplexing state
│   ├── imu.h/.cpp                raw MPU6050 I2C register reads
│   ├── barometer.h/.cpp          Adafruit_BMP085-based BMP180 driver
│   └── telemetry.h/.cpp          Soil cache + CubePacket collection
└── Station_B/
    ├── Station_B.ino
    ├── config.h
    ├── radio_config.h           Link B mode + addresses + RF params
    ├── packet_types.h
    ├── radio.h / radio.cpp      RX-only
    ├── telemetry.h/.cpp          Soil + Cube caches, zero/last-valid/replace semantics
    └── serial_output.h/.cpp      the ONLY file allowed to touch Serial
```

## 6. Required Arduino libraries

Install via Library Manager (Sketch → Include Library → Manage Libraries):

- **RF24** by TMRh20 — all three projects
- **DHT sensor library** by Adafruit, plus its dependency **Adafruit Unified Sensor** — Station A only
- **Adafruit BMP085 Library** by Adafruit (register-compatible with BMP180) — CubeSat only

MPU6050 uses raw `Wire` (I2C) calls directly — no external library needed.

## 7. Wiring notes

- **nRF24L01 power**: it needs a clean **3.3 V** supply, not 5 V. Current
  draw spikes during TX (tens of mA), which can brown out an onboard 3.3 V
  regulator, especially at `RF24_PA_HIGH`/`RF24_PA_MAX`. This project
  defaults every board to `RF24_PA_LOW` specifically to reduce that risk.
  Put a decoupling capacitor (10–100 µF electrolytic + 0.1 µF ceramic)
  directly across the module's VCC/GND, close to the module. If you see
  intermittent radio init failures or resets during TX, use a dedicated
  3.3 V regulator (e.g. AMS1117-3.3) rather than the Arduino's onboard pin.
- I2C bus on the CubeSat is shared (SDA/SCL) between MPU6050 and BMP180 —
  this is fine, they have different addresses (0x68 vs 0x77); no extra
  wiring is needed beyond the standard pull-ups most breakout boards
  already include.
- **Rain, soil, LDR module variants**: this code assumes the LDR module is
  digital-output only per your product description. If your physical rain
  or soil module's polarity doesn't match the defaults in `config.h`
  (`RAIN_TRIGGERS_BELOW_THRESHOLD`, `SOIL_ADC_DRY`/`SOIL_ADC_WET`,
  `LDR_ACTIVE_LEVEL`), flip the corresponding constant — do not change
  application logic, only the calibration values.

## 8. RF configuration instructions

1. Decide each link's mode independently: `SOIL_TO_CUBE_MODE` and
   `CUBE_TO_STATIONB_MODE`.
2. For each link, set the mode identically on **both ends**:
   - Station A's `SOIL_TO_CUBE_MODE` ⇔ CubeSat's `SOIL_TO_CUBE_MODE`
   - CubeSat's `CUBE_TO_STATIONB_MODE` ⇔ Station B's `CUBE_TO_STATIONB_MODE`
3. If a link is `SHARED_MULTI_RECEIVER`: only the shared address constant
   matters; it must match on both ends.
4. If a link is `ONE_TO_ONE` or `EXPLICIT_MULTI_RECEIVER`: the transmitting
   side's address table (`CUBESAT_ADDRESSES[]` or `STATIONB_ADDRESSES[]`)
   must contain every receiving unit's address, and each receiving unit's
   `MY_ADDRESS_FROM_*` constant must exactly match its own entry in that
   table. Each physical unit gets a distinct `MY_ADDRESS_FROM_*` value.
5. `RF_CHANNEL`, `RF_DATA_RATE`, `RF_PA_LEVEL`, and `RF_ADDRESS_WIDTH` must
   match across every node on the same link (channel/data-rate mismatches
   are a common cause of "radio inits fine but nothing is ever received").

## 9. Test procedure

**Station A**
- Boot with DHT11 disconnected → `statusFlags` bit0 clear, temp/humidity report 0, rest of packet unaffected.
- Cover/uncover rain sensor → `rainState` toggles at the configured threshold.
- Dip/dry the soil sensor → `soilPct`/`soilState` move through Dry → Normal → Overwatered.
- Cover/uncover the LDR → `lightState` toggles.
- Disconnect the nRF24 → sensors keep updating; `radio_isAvailable()` false; `radio_maintain()` retries every 5s.

**CubeSat**
- Disconnect MPU6050 → `CUBE_STATUS_MPU_VALID` clears, accel/gyro fields report 0, BMP180 fields unaffected.
- Disconnect BMP180 → same, mirrored.
- Power Station A off after CubeSat has received at least one Soil packet → CubeSat keeps relaying the last cached Soil packet.
- Power Station A off before CubeSat ever receives a packet → CubeSat sends only its `CubePacket` (no fabricated Soil data).
- Move CubeSat out of Station B range → `radio_relayAndSend` failures recorded per-destination (if addressed mode); CubeSat keeps running.

**Station B**
- Power on with nothing transmitting → prints the deterministic zero-state line every `PRINT_INTERVAL_MS`, nothing else.
- Soil-only traffic → Cube fields stay at zero/last-valid; Soil fields update live.
- Cube-only traffic → mirrored.
- Both present → single combined line updates live.
- Interrupt CubeSat mid-stream → Station B keeps printing the last valid values for whichever source went quiet, not zeros.
- Confirm nothing but telemetry lines ever appears on the serial monitor at 115200 baud.

## 10. Known limitations

- **Bounded (not eliminated) blocking during ACK-based TX.** `RF24::write()`
  without the NO_ACK flag blocks until an ACK arrives or the configured
  retries are exhausted — bounded by `RF_RETRY_DELAY`/`RF_RETRY_COUNT`
  (worst case here: 15 × (15×250 µs + ~500 µs) ≈ 64 ms per destination).
  In `EXPLICIT_MULTI_RECEIVER` mode with N destinations this can add up to
  roughly N × 64 ms during the brief TX window. It is not an infinite or
  arbitrary-length block, and `SHARED_MULTI_RECEIVER` sends are
  effectively instantaneous (no ACK wait at all), but if you configure a
  large destination list, keep `RELAY_TX_INTERVAL_MS` comfortably larger
  than the worst-case cumulative TX time.
- **Multiple simultaneously-transmitting CubeSats are out of scope.** The
  current architecture assumes one CubeSat (or several CubeSats that never
  transmit at literally the same instant). If you later add multiple
  CubeSats transmitting on the same shared address concurrently, they can
  collide on-air; that requires an explicit collision-avoidance/backoff
  scheme this project does not implement.
- **Analog sensors (rain, soil) are always reported "valid."** A
  disconnected analog sensor reads floating noise, not a detectable
  fault — this is a property of analog inputs, not a gap in this code.
  Only DHT11 (digital protocol) and the I2C sensors (which report bus
  errors) can be positively detected as failed.
- **`RF24::begin()` return type.** This code assumes a current version of
  the TMRh20 RF24 library where `begin()` returns `bool`. Very old library
  versions had `begin()` return `void`; if you're pinned to such a
  version, `radio_init()` needs a one-line adjustment.
- **BMP180 altitude accuracy** depends entirely on how current/accurate
  `BMP_REFERENCE_PRESSURE_PA` is; it is not a GPS-grade altitude.

## Verification performed in this environment

- Compiled `packet_types.h` standalone with g++ (not avr-gcc, since no
  Arduino toolchain is available here) and confirmed via `sizeof()`:
  `SoilPacket = 14 bytes`, `CubePacket = 22 bytes`, both ≤ the 32-byte
  `RF_PAYLOAD_SIZE`, matching the tables above exactly.
- Searched the entire tree for `delay(`, `String`, infinite `while(true)`
  loops, and leftover "mission counter" references — none found (the only
  matches were comments explicitly stating their absence, and unrelated
  substrings like `beginTransmission`).
- Full avr-gcc compilation against real RF24/DHT/Adafruit_BMP085 library
  headers was not possible in this sandbox (no Arduino toolchain / network
  access to library sources here) — compile each sketch in the Arduino IDE
  before flashing, as a final check on library API compatibility with
  whatever exact library versions you install.
