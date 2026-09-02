# Cube-Sat-Relay-System
# 3-Node Arduino CubeSat Relay System

A robust, fault-tolerant telemetry and relay network linking ground environmental monitoring, an intermediate orbital relay node (CubeSat simulation), and a ground reception station interfaced to LabVIEW via serial telemetry.

```
+------------------------+      Link A (RF)      +------------------------+      Link B (RF)      +------------------------+      USB Serial      +------------------------+
|    Station A: Soil     | --------------------> |    CubeSat (Relay)     | --------------------> |   Station B: Receiver  | -------------------> |    LabVIEW Ground      |
|    (Arduino Nano)      |     2.4 GHz nRF24     |     (Arduino UNO)      |     2.4 GHz nRF24     |     (Arduino Nano)     |     115200 Baud      |   Telemetry Station    |
+------------------------+                       +------------------------+                       +------------------------+                      +------------------------+
```

---

## 1. System Overview & Architecture

The system coordinates three autonomous Arduino nodes implementing a multi-hop wireless sensor relay chain:

- **Station A ("Soil") — Arduino Nano**:
  - Acts as a dedicated field sensor node.
  - Collects ambient and soil parameters: Temperature and Relative Humidity (DHT11), Rain level (Analog), Soil Moisture (Analog), and Ambient Light (Digital LDR) at intervals defined by `SENSOR_INTERVAL_MS`.
  - Packages telemetry into a packed 14-byte `SoilPacket` and transmits it via 2.4 GHz nRF24L01 every `RF_SEND_INTERVAL_MS`.
  - Operates strictly in **Transmit-Only (TX)** mode with zero reception overhead.

- **CubeSat ("Cube") — Arduino UNO**:
  - Simulates a low-earth orbit CubeSat relay platform equipped with an environmental and attitude monitoring payload.
  - Features a **single nRF24L01 radio transceiver operating in time-multiplexed RX/TX mode**.
  - **RX Phase (Default)**: Continuously listens for incoming `SoilPacket` frames from Station A and maintains a cached copy of the latest valid payload.
  - **Payload Acquisition**: Periodically reads an MPU6050 6-DOF IMU (raw Accelerometer and Gyroscope registers via raw I2C) and a BMP180 barometric pressure/altitude sensor via Adafruit drivers every `SENSOR_INTERVAL_MS`.
  - **TX Phase**: Every `RELAY_TX_INTERVAL_MS`, switches the radio to transmit mode, broadcasts the unmodified cached `SoilPacket` followed by its own packed 22-byte `CubePacket`, and immediately returns to listening.

- **Station B ("Ground Receiver") — Arduino Nano**:
  - Acts as the primary ground telemetry sink node.
  - Operates strictly as a **Receive-Only (RX)** node listening on Link B.
  - Demultiplexes incoming frames based on `packetId`, routing data into dedicated last-valid-value cache registers (`SoilPacket` cache and `CubePacket` cache).
  - Emits a strictly deterministic, single-line telemetry string to USB Serial at 115200 baud every `PRINT_INTERVAL_MS`.
  - Zero debug text, status prompts, or asynchronous logs are output on the serial link, guaranteeing seamless parsing by LabVIEW or automated SCADA systems.

---

## 2. RF Protocol & Distribution Architecture

The network utilizes Nordic Semiconductor nRF24L01+ transceivers on two independent wireless hops:
- **Link A**: Station A $\rightarrow$ CubeSat
- **Link B**: CubeSat $\rightarrow$ Station B

```
                 Link A                                  Link B
Station A (TX) ----------> CubeSat (RX -> TX) --------------------------> Station B (RX)
[SoilPacket]               [Cache SoilPacket]                             [Demux & Cache]
                           [Sample MPU6050 + BMP180]                      [Output deterministic
                           [Send SoilPacket + CubePacket]                  Serial to LabVIEW]
```

### 2.1 Distribution Modes
Both links configure their distribution mode independently via an `enum class RfDistributionMode : uint8_t` in `radio_config.h`:

| Mode | Target Count | Hardware ACKs | Destination Tracking | Best Used For |
|---|---|---|---|---|
| `ONE_TO_ONE` | 1 | Enabled (Automatic Retries) | Per-destination success/fail counters | Direct point-to-point links with delivery confirmation |
| `SHARED_MULTI_RECEIVER` | Broadcast ($\ge 1$) | Disabled (`NO_ACK` flag) | Anonymous broadcast | Omnidirectional/downlink broadcast to multiple passive listening stations |
| `EXPLICIT_MULTI_RECEIVER` | $N$ Distinct Nodes | Enabled (Automatic Retries) | Independent tracking per destination address | Selective, multi-node point-to-point addressing |

### 2.2 Dual-Mode Time-Multiplexing on CubeSat
The CubeSat uses a single radio module to service both Link A and Link B without requiring radio re-initialization or global ACK configuration toggling:
- When transmitting under `SHARED_MULTI_RECEIVER`, the driver utilizes the nRF24 per-packet hardware `NO_ACK` flag (`radio.write(buf, len, true)`).
- This flag instructs the receiver's hardware not to generate an ACK packet, regardless of whether auto-acknowledgment is globally configured on the RX pipe.
- As a result, the CubeSat can listen with auto-ack enabled on Link A and relay downlink packets anonymously over Link B without runtime re-binding or pipe reconfiguration overhead.

---

## 3. Telemetry Packet Structure & Transport Envelope

### 3.1 Memory Layout & Alignment
All network packets enforce strict 1-byte alignment using `#pragma pack(push, 1)` and are protected by compile-time assertions (`static_assert`) to eliminate compiler padding discrepancies across different GCC architectures.

```cpp
#pragma pack(push, 1)
// Struct definitions...
#pragma pack(pop)

static_assert(sizeof(SoilPacket) <= RF_PAYLOAD_SIZE, "SoilPacket exceeds RF payload limit!");
static_assert(sizeof(CubePacket) <= RF_PAYLOAD_SIZE, "CubePacket exceeds RF payload limit!");
```

### 3.2 Packet Definitions

#### `SoilPacket` (14 Bytes)
| Offset | Field | Type | Scale / Unit | Description |
|---|---|---|---|---|
| 0 | `packetId` | `uint8_t` | — | Constant `PACKET_SOIL` identifier |
| 1–2 | `dhtTempC_x10` | `int16_t` | $	imes 10$ ($^\circ\text{C}$) | DHT11 Temperature ($25.4^\circ\text{C} \rightarrow 254$) |
| 3–4 | `dhtHumidity_x10` | `uint16_t` | $	imes 10$ (%RH) | DHT11 Relative Humidity ($60.5\% \rightarrow 605$) |
| 5–6 | `rainRaw` | `uint16_t` | Raw ADC (0–1023) | Rain sensor analog input voltage |
| 7 | `rainState` | `uint8_t` | Enum | `0 = RAIN_NONE`, `1 = RAIN_DETECTED` |
| 8–9 | `soilRaw` | `uint16_t` | Raw ADC (0–1023) | Soil moisture analog input voltage |
| 10 | `soilPct` | `uint8_t` | 0–100 (%) | Calibrated & clamped soil moisture percentage |
| 11 | `soilState` | `uint8_t` | Enum | `0 = SOIL_DRY`, `1 = SOIL_NORMAL`, `2 = SOIL_OVERWATERED` |
| 12 | `lightState` | `uint8_t` | Enum | `0 = LIGHT_DARK`, `1 = LIGHT_DETECTED` (Digital sensor) |
| 13 | `statusFlags` | `uint8_t` | Bitmask | Sensor health flags (Bits 0..3) |

**Soil Status Bitmask (`statusFlags`):**
- `Bit 0` (`0x01`): `DHT_VALID` — DHT11 read successfully
- `Bit 1` (`0x02`): `RAIN_VALID` — Rain sensor channel active
- `Bit 2` (`0x04`): `SOIL_VALID` — Soil moisture channel active
- `Bit 3` (`0x08`): `LIGHT_VALID` — LDR sensor channel active

#### `CubePacket` (22 Bytes)
| Offset | Field | Type | Scale / Unit | Description |
|---|---|---|---|---|
| 0 | `packetId` | `uint8_t` | — | Constant `PACKET_CUBE` identifier |
| 1–2 | `accelX` | `int16_t` | Raw register | MPU6050 X-axis acceleration |
| 3–4 | `accelY` | `int16_t` | Raw register | MPU6050 Y-axis acceleration |
| 5–6 | `accelZ` | `int16_t` | Raw register | MPU6050 Z-axis acceleration |
| 7–8 | `gyroX` | `int16_t` | Raw register | MPU6050 X-axis angular rate |
| 9–10 | `gyroY` | `int16_t` | Raw register | MPU6050 Y-axis angular rate |
| 11–12 | `gyroZ` | `int16_t` | Raw register | MPU6050 Z-axis angular rate |
| 13–14 | `bmpTempC_x10` | `int16_t` | $	imes 10$ ($^\circ\text{C}$) | BMP180 barometric temperature |
| 15–18 | `bmpPressurePa` | `uint32_t` | Pascals (Pa) | BMP180 barometric atmospheric pressure |
| 19–20 | `bmpAltitudeM_x10` | `int16_t` | $	imes 10$ (m) | BMP180 relative altitude |
| 21 | `statusFlags` | `uint8_t` | Bitmask | Sensor health flags (Bits 0..1) |

**Cube Status Bitmask (`statusFlags`):**
- `Bit 0` (`0x01`): `MPU_VALID` — MPU6050 communication and self-test OK
- `Bit 1` (`0x02`): `BMP_VALID` — BMP180 communication and sampling OK

### 3.3 Fixed 32-Byte RF Transport Framing
The nRF24 radio operates with a static hardware payload size (`setPayloadSize(32)`):
- **Transmit**: A 32-byte staging buffer is zero-cleared. The logical packet struct (`SoilPacket` or `CubePacket`) is copied to offset 0. The remaining bytes serve as zero padding.
- **Receive**: Exactly 32 bytes are pulled from the radio FIFO. The node inspects byte 0 (`packetId`) and unpacks only `sizeof(TargetPacket)` bytes into memory via `memcpy`. Padding bytes are discarded, preventing undefined behavior.

---

## 4. Hardware Pinout & Wiring Specifications

### 4.1 Master Pin Assignment Table

| Subsystem / Interface | Station A (Nano) | CubeSat (UNO) | Station B (Nano) |
|---|---|---|---|
| **SPI MOSI** | D11 | D11 | D11 |
| **SPI MISO** | D12 | D12 | D12 |
| **SPI SCK** | D13 | D13 | D13 |
| **nRF24L01 CE** | D9 | D9 | D9 |
| **nRF24L01 CSN** | D10 | D10 | D10 |
| **I2C SDA** | — | A4 (MPU6050 @ `0x68`, BMP180 @ `0x77`) | — |
| **I2C SCL** | — | A5 (MPU6050 @ `0x68`, BMP180 @ `0x77`) | — |
| **DHT11 Data** | D2 | — | — |
| **Rain Sensor AO** | A0 | — | — |
| **Rain Sensor DO** | D3 *(wired, reserved)* | — | — |
| **Soil Moisture AO**| A1 | — | — |
| **LDR Sensor DO** | D4 | — | — |
| **Telemetry Out** | — | — | TX0 / RX0 (USB Serial @ 115200) |

### 4.2 Power Distribution & Decoupling Recommendations
- **nRF24L01 Power Rail**:
  - Must be powered from a dedicated **3.3V** supply rail. Connecting to 5V will permanently destroy the transceiver.
  - Transceiver peak currents exceed 30 mA during RF bursts. Onboard Arduino 3.3V LDOs frequently drop voltage, causing radio brownouts and SPI lockups.
  - Solder a **10 µF – 100 µF electrolytic capacitor in parallel with a 0.1 µF ceramic capacitor** directly across the VCC and GND pins of each nRF24 module.
  - For field deployment, use an external 3.3V regulator (e.g., AMS1117-3.3).
- **I2C Bus on CubeSat**:
  - The MPU6050 and BMP180 share the standard hardware I2C bus (A4/A5).
  - Distinct hardware I2C addresses (`0x68` and `0x77`) allow conflict-free operation on the same bus without external multiplexers.

---

## 5. Software Architecture & Directory Layout

The codebase is organized into three independent, decoupled Arduino IDE projects sharing a standard protocol header:

```
Project/
├── DESIGN.md                         # Detailed architecture and engineering rationale
├── README.md                         # Project documentation and deployment guide
│
├── Station_A_Soil/                   # [Node 1] Environmental Sensing Transmitter
│   ├── Station_A_Soil.ino            # Main sketch loop & initialization
│   ├── config.h                      # Hardware pins, sampling rates & sensor calibration
│   ├── radio_config.h                # Link A RF channel, addresses, and distribution mode
│   ├── packet_types.h                # Shared binary telemetry structures & protocol enums
│   ├── radio.h / radio.cpp           # Non-blocking RF24 transport abstraction
│   ├── dht_sensor.h / .cpp           # Adafruit DHT11 wrapper & error check
│   ├── rain_sensor.h / .cpp          # Analog rain reading & threshold detection
│   ├── soil_moisture.h / .cpp        # Soil moisture calibration, ADC map & clamping
│   ├── light_sensor.h / .cpp         # Digital LDR comparator state reader
│   └── soil_telemetry.h / .cpp       # Aggregator compiling sensors into SoilPacket
│
├── CubeSat/                          # [Node 2] Relay & Attitude Satellite Node
│   ├── CubeSat.ino                   # Main relay state machine
│   ├── config.h                      # Timing intervals & reference pressure calibration
│   ├── radio_config.h                # Dual-link (Link A & Link B) RF configuration
│   ├── packet_types.h                # Shared binary telemetry structures & protocol enums
│   ├── radio.h / radio.cpp           # Time-multiplexed RX/TX state driver
│   ├── imu.h / .cpp                  # Direct Wire I2C register driver for MPU6050
│   ├── barometer.h / .cpp            # Adafruit BMP180 atmospheric pressure & altitude
│   └── telemetry.h / .cpp            # SoilPacket cache manager & CubePacket builder
│
└── Station_B/                        # [Node 3] Telemetry Sink & LabVIEW Bridge
    ├── Station_B.ino                 # Main receiver execution loop
    ├── config.h                      # Serial output formatting & print intervals
    ├── radio_config.h                # Link B RF channel, addresses, and distribution mode
    ├── packet_types.h                # Shared binary telemetry structures & protocol enums
    ├── radio.h / radio.cpp           # Dedicated RX-only RF24 listener driver
    ├── telemetry.h / .cpp            # Cache tables with zero-fill / last-valid semantics
    └── serial_output.h / .cpp        # Pure deterministic Serial streaming interface
```

---

## 6. Station B Deterministic Serial Protocol (LabVIEW Interface)

To prevent parsing errors, buffer overruns, and frame misalignment in automated LabVIEW VIS or SCADA parsers, **Station B enforces strict serial streaming rules**:

1. **Dedicated Output Stream**: Only `serial_output.cpp` is permitted to access the `Serial` hardware interface. No debug logging, boot banners, or human-readable strings are emitted.
2. **Fixed Refresh Interval**: Serial strings are emitted exactly once every `PRINT_INTERVAL_MS` regardless of RF traffic state.
3. **Deterministic Fail-Safe (Zero / Last-Valid Semantics)**:
   - If no packets have been received since boot, fields report initial zero/invalid states.
   - If Link A or Link B drops mid-mission, Station B persists the **last known valid telemetry values** for the dropped node, preventing telemetry flicker.
4. **Baud Rate**: `115200` baud, 8 data bits, no parity, 1 stop bit (8-N-1).

---

## 7. Required Arduino Libraries

Install the following dependencies via the Arduino IDE Library Manager (**Sketch $\rightarrow$ Include Library $\rightarrow$ Manage Libraries**):

| Library Name | Author | Applicable Projects | Notes |
|---|---|---|---|
| **RF24** | TMRh20 | Station A, CubeSat, Station B | Optimized nRF24L01 driver |
| **DHT sensor library** | Adafruit | Station A | Temperature & humidity sensing |
| **Adafruit Unified Sensor**| Adafruit | Station A | Core sensor abstraction dependency |
| **Adafruit BMP085 Library**| Adafruit | CubeSat | Register-compatible with BMP180 sensor |
| **Wire** | Built-in | CubeSat | Native AVR I2C library (MPU6050 registers) |

---

## 8. Configuration & Setup Guide

### Step 1: Configure Radio Channel and Parameters
Open `radio_config.h` across the sketches and ensure common RF physical layer parameters match:
- `RF_CHANNEL`: Must match on both nodes of a link (e.g., 76).
- `RF_DATA_RATE`: Must match (e.g., `RF24_1MBPS`).
- `RF_PA_LEVEL`: Default is set to `RF24_PA_LOW` to prevent power rail collapse.
- `RF_ADDRESS_WIDTH`: Set to 5 bytes.

### Step 2: Establish Distribution Modes & Addresses
- **Link A (Station A $\leftrightarrow$ CubeSat)**:
  - Match `SOIL_TO_CUBE_MODE` between `Station_A_Soil/radio_config.h` and `CubeSat/radio_config.h`.
- **Link B (CubeSat $\leftrightarrow$ Station B)**:
  - Match `CUBE_TO_STATIONB_MODE` between `CubeSat/radio_config.h` and `Station_B/radio_config.h`.
- **Addressing**:
  - In `SHARED_MULTI_RECEIVER`, configure identical broadcast pipe addresses.
  - In `ONE_TO_ONE` or `EXPLICIT_MULTI_RECEIVER`, ensure transmitter address tables include the exact hardware node addresses defined by `MY_ADDRESS_FROM_*`.

### Step 3: Sensor Calibration (`config.h`)
- **Rain Sensor**: Set threshold via `RAIN_ADC_THRESHOLD` and polarity via `RAIN_TRIGGERS_BELOW_THRESHOLD`.
- **Soil Moisture**: Adjust `SOIL_ADC_DRY` and `SOIL_ADC_WET` based on physical soil calibration.
- **Barometer**: Set `BMP_REFERENCE_PRESSURE_PA` to current local mean sea-level pressure (e.g., `101325UL`) for accurate altitude calculations.

---

## 9. Hardware & System Verification Procedures

Execute these test scenarios after flashing the firmware:

### Station A Verification
1. **DHT11 Fault Isolation**: Disconnect the DHT11 signal line while running. Verify `statusFlags` Bit 0 clears, temperature/humidity values report 0, and analog rain/soil readings continue transmitting uninterrupted.
2. **Rain & Soil Calibration**: Expose rain sensor to water droplets $\rightarrow$ `rainState` switches from `RAIN_NONE` to `RAIN_DETECTED`. Place soil probe in dry vs. saturated soil $\rightarrow$ `soilPct` transitions linearly through Dry $\rightarrow$ Normal $\rightarrow$ Overwatered.
3. **RF Recovery**: Disconnect the nRF24 transceiver module. Verify sensor acquisition continues unhindered and `radio_maintain()` periodically attempts reconnection without hanging the MCU.

### CubeSat Verification
1. **I2C Sensor Disconnect**: Unplug MPU6050 during runtime $\rightarrow$ `statusFlags` Bit 0 (`MPU_VALID`) clears immediately while BMP180 continues logging pressure and altitude. Unplug BMP180 $\rightarrow$ Bit 1 (`BMP_VALID`) clears while IMU data continues logging.
2. **Relay Persistence**: Power off Station A after the CubeSat has received at least one packet. Confirm the CubeSat continues transmitting its own `CubePacket` along with the **last cached `SoilPacket`**.
3. **Cold Boot Isolation**: Boot CubeSat with Station A powered off. Confirm CubeSat relays **only** its `CubePacket` without injecting phantom or uninitialized soil data.

### Station B Verification
1. **Deterministic Boot**: Power Station B with no transmitters active. Verify that it strictly prints the deterministic baseline telemetry string on Serial at 115200 baud every `PRINT_INTERVAL_MS`.
2. **Partial Stream Interruption**: Power off Station A while keeping CubeSat running, or power off CubeSat. Verify Station B holds the last valid measurements in cache rather than dropping fields to zero or emitting error messages.
3. **LabVIEW Stream Integrity**: Confirm with an automated serial monitor that no ASCII debug strings, newline corruptions, or non-telemetry data appear on the USB COM port.

---

## 10. Design Considerations & Limitations

- **Bounded Non-Blocking ACK Transmission**:
  Under `ONE_TO_ONE` or `EXPLICIT_MULTI_RECEIVER` modes, missing receivers trigger hardware retries (`RF_RETRY_DELAY` $\times$ `RF_RETRY_COUNT`). This introduces a maximum bounded hardware delay of approximately 64 ms per missing node. To prevent timing jitter, ensure `RELAY_TX_INTERVAL_MS` includes adequate margin for all addressed receivers. Under `SHARED_MULTI_RECEIVER`, transmissions complete with zero ACK wait time.
- **Single CubeSat Topology**:
  The system is architected for a single relay node. Multiple unsynchronized CubeSats sharing the same RF channel and pipe address will experience on-air packet collisions unless an external time-slotted or CSMA scheme is introduced.
- **Analog Sensor Fault Diagnostics**:
  Disconnected analog pins register floating high-impedance voltage rather than electrical open-circuit faults. Consequently, analog sensor health flags indicate sampling activity rather than physical sensor presence. Digital interfaces (DHT11, MPU6050, BMP180) provide positive hardware fault detection.
- **Barometric Altitude vs. GNSS**:
  Altitude derived from the BMP180 depends on ambient atmospheric reference pressure (`BMP_REFERENCE_PRESSURE_PA`) and should be calibrated prior to flight simulations.

---

## 11. License & Attribution

Designed and engineered for modular Arduino CubeSat telemetry and environmental relay research. Built using the open-source **TMRh20 RF24** and **Adafruit Sensor** ecosystems.
