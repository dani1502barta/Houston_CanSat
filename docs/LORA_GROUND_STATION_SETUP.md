# LoRa Ground Station Setup Guide

This guide explains how to set up a second Teensy 4.1 as a LoRa ground station to test communication with the CanSat probe.

## 📋 Table of Contents

1. [Overview](#overview)
2. [Hardware Requirements](#hardware-requirements)
3. [Software Setup](#software-setup)
4. [Configuration](#configuration)
5. [Usage](#usage)
6. [Testing Procedures](#testing-procedures)
7. [Troubleshooting](#troubleshooting)

---

## 🎯 Overview

The ground station is a simplified version of the CanSat firmware that:
- **Sends command packets** to request telemetry (TEL) or scientific data (SCI)
- **Receives and displays** telemetry and scientific packets from the probe
- **Provides a serial interface** for interactive testing

### Command Packet Format

The ground station sends 2-byte command packets:

```
Byte 0: 0x00 (Command Packet ID)
Byte 1: [TEL bit 5] [SCI bit 4] [TEAM_ID bits 3-0]
```

**Examples:**
- Request telemetry only: `0x00 0x20` (TEL=1, SCI=0, Team=0)
- Request scientific only: `0x00 0x10` (TEL=0, SCI=1, Team=0)
- Request both: `0x00 0x30` (TEL=1, SCI=1, Team=0)
- Request for Team 5: `0x00 0x25` (TEL=1, SCI=0, Team=5)

---

## 🔧 Hardware Requirements

### Required Components

1. **Teensy 4.1** (second board, separate from the probe)
2. **RFM95W LoRa module** (same model as on probe)
3. **LoRa antenna** (433/868 MHz, same as probe)
4. **USB cable** for programming and serial communication
5. **Breadboard and jumper wires** (optional, for prototyping)

### Wiring Diagram

Connect the RFM95W to Teensy 4.1:

```
RFM95W Pin    →    Teensy 4.1 Pin
─────────────────────────────────
VCC           →    3.3V
GND           →    GND
MISO          →    Pin 12 (SPI MISO)
MOSI          →    Pin 11 (SPI MOSI)
SCK           →    Pin 13 (SPI SCK)
NSS (CS)      →    Pin 10
DIO0 (INT)    →    Pin 14
RST           →    Pin 15
```

**Important:** Use the same pin assignments as the probe to ensure compatibility.

---

## 💻 Software Setup

### Step 1: Create Ground Station Project

1. **Create a new PlatformIO project:**
   ```bash
   # In a new directory (e.g., ../LORA_GROUND_STATION)
   pio project init --board teensy41
   ```

2. **Or manually create the project structure:**
   ```
   LORA_GROUND_STATION/
   ├── platformio.ini
   ├── src/
   │   └── main.cpp
   └── README.md
   ```

### Step 2: Install Dependencies

The ground station uses the same LoRa library as the probe:
- `sandeepmistry/LoRa@^0.8.0`

This will be automatically installed when you build the project.

### Step 3: Copy Configuration

Copy the `platformio.ini` and `src/main.cpp` files from the `ground_station/` directory in this repository (see `GROUND_STATION_CODE.md` for the complete code).

---

## ⚙️ Configuration

### LoRa Parameters

The ground station **must** use the same LoRa parameters as the probe:

| Parameter | Value | Notes |
|-----------|-------|-------|
| Frequency | 868.3 MHz | Must match exactly |
| Spreading Factor | 8 | SF8 |
| Coding Rate | 4/6 | CR 6 |
| Bandwidth | 250 kHz | 250E3 Hz |
| TX Power | 14 dBm | Max 25mW |

These are configured in `src/main.cpp` and should match `lib/include/config.h` in the probe firmware.

### Team ID

Set the target Team ID in `src/main.cpp`:

```cpp
#define TARGET_TEAM_ID 0x0  // Change to your team's ID (0x0-0xF)
```

This must match the `TEAM_ID` defined in the probe's `lib/include/config.h`.

---

## 🚀 Usage

### Step 1: Upload Firmware

1. Connect the ground station Teensy via USB
2. Build and upload:
   ```bash
   pio run -t upload
   ```

### Step 2: Open Serial Monitor

```bash
pio device monitor --baud 115200
```

You should see:
```
======================================
   LoRa Ground Station
======================================
Team ID: 0x0
[INIT] Initializing LoRa...
LoRa initialized
LoRa ready and listening...
Freq: 868.30 MHz, SF: 8, BW: 250 kHz, CR: 4/6, Power: 14 dBm

Ground Station Ready!
Commands:
  't' - Send TEL request
  's' - Send SCI request
  'b' - Send both (TEL + SCI)
  'r' - Show last received packet
  'h' - Show this help
```

### Step 3: Send Commands

Type commands in the serial monitor:

- **`t`** - Request telemetry only
- **`s`** - Request scientific data only
- **`b`** - Request both telemetry and scientific
- **`r`** - Display last received packet (if any)
- **`h`** - Show help

### Step 4: Observe Responses

When you send a command:
1. The ground station transmits the command packet
2. The probe receives it (visible in probe's Serial Monitor)
3. The probe processes and responds
4. The ground station receives and displays the response

---

## 🧪 Testing Procedures

### Test 1: Basic Communication

**Objective:** Verify LoRa link is working

1. Power on both devices (probe and ground station)
2. Wait for both to initialize
3. Send a telemetry request (`t`) from ground station
4. **Verify on probe Serial Monitor:**
   ```
   [LoRa RX] Received 2 bytes
   [LoRa RX] Valid command: Team=0x0, TEL=1, SCI=0
   [LoRa RX] Command for this probe!
   [LoRa TX] Processing command...
   [LoRa TX] Sending telemetry...
   ```
5. **Verify on ground station Serial Monitor:**
   ```
   [TX] Sending command: TEL=1, SCI=0, Team=0x0
   [TX] Command sent (2 bytes)
   [RX] Received 16 bytes (Telemetry packet)
   [RX] Parsed telemetry: lat=XX.XXXXX, lon=XX.XXXXX, alt=XXX.Xm
   ```

### Test 2: Telemetry Request (TEL)

**Objective:** Verify telemetry packet reception

1. Ensure probe is in READY or DESCENT state
2. Send `t` from ground station
3. **Expected on ground station:**
   - Command sent confirmation
   - Telemetry packet received (16 bytes)
   - Parsed data displayed (lat, lon, alt, velocity, timestamp)

### Test 3: Scientific Request (SCI)

**Objective:** Verify scientific packet reception

1. Ensure probe has collected some data (BMP, UV readings)
2. Send `s` from ground station
3. **Expected on ground station:**
   - Command sent confirmation
   - Scientific packet received (47-82 bytes)
   - Packet size and bitmap flags displayed

### Test 4: Combined Request (TEL + SCI)

**Objective:** Verify both packet types in sequence

1. Send `b` from ground station
2. **Expected on ground station:**
   - Command sent confirmation
   - Telemetry packet received first
   - Scientific packet received second (after ~100ms delay)

### Test 5: Wrong Team ID

**Objective:** Verify probe ignores commands for other teams

1. Temporarily change `TARGET_TEAM_ID` in ground station to `0xF`
2. Send a command
3. **Expected on probe:**
   ```
   [LoRa RX] Received 2 bytes
   [LoRa RX] Valid command: Team=0xF, TEL=1, SCI=0
   [LoRa RX] Command for different team
   ```
4. **Expected on ground station:**
   - Command sent, but no response received

---

## 🔍 Packet Format Reference

### Command Packet (2 bytes)

```
Byte 0: 0x00 (PACKET_ID_COMMAND)
Byte 1: [Bit 7: Reserved]
        [Bit 6: Reserved]
        [Bit 5: TEL request (0x20)]
        [Bit 4: SCI request (0x10)]
        [Bits 3-0: Team ID (0x0F)]
```

### Telemetry Packet (16 bytes)

```
Byte 0:   Packet ID (0xA0-0xAF) | Team ID
Bytes 1-3:   Latitude (signed 24-bit, degrees × 10^5)
Bytes 4-6:   Longitude (signed 24-bit, degrees × 10^5)
Bytes 7-8:   Vertical velocity (signed 16-bit, cm/s)
Bytes 9-10:  Altitude (unsigned 16-bit, meters × 10)
Bytes 11-14: Timestamp (unsigned 32-bit, Unix epoch)
Byte 15:     CRC8 checksum
```

### Scientific Packet (47-82 bytes)

See `lib/telemetry/telemetry.h` and `lib/telemetry/telemetry.cpp` in the probe firmware for complete format.

---

## 🐛 Troubleshooting

### Problem: No Response from Probe

**Possible Causes:**
1. **Wrong Team ID** - Check that `TARGET_TEAM_ID` matches probe's `TEAM_ID`
2. **Different LoRa parameters** - Verify frequency, SF, BW, CR match exactly
3. **Antenna not connected** - Check both devices have antennas
4. **Out of range** - LoRa range is typically 1-5 km line-of-sight, but can be much less indoors
5. **Probe not in correct state** - Probe must be in READY, DESCENT, or LANDED state

**Solutions:**
- Verify Team ID on both devices
- Check Serial Monitor on probe to see if command is received
- Move devices closer together (start with < 1 meter)
- Ensure antennas are properly connected

### Problem: Invalid Packet Received

**Possible Causes:**
1. **Interference** - Other LoRa devices on same frequency
2. **Corrupted transmission** - Poor signal quality
3. **Wrong packet format** - Probe sending unexpected data

**Solutions:**
- Check CRC in telemetry packets
- Verify packet size matches expected format
- Try sending command again

### Problem: Command Not Parsed by Probe

**Check probe Serial Monitor:**
- If you see `[LoRa RX] Received X bytes` but `[LoRa RX] Invalid command packet`, the packet format is wrong
- Verify command packet is exactly 2 bytes: `0x00` followed by command byte

### Problem: Ground Station Not Receiving

**Possible Causes:**
1. **Probe not transmitting** - Check probe Serial Monitor for TX messages
2. **Timing issue** - Probe may transmit before ground station is ready
3. **Range/obstacles** - Physical barriers blocking signal

**Solutions:**
- Wait a few seconds after sending command before expecting response
- Check probe Serial Monitor for `[LoRa TX]` messages
- Ensure line-of-sight between antennas

---

## 📝 Notes

- **Duty Cycle:** LoRa has duty cycle restrictions (1% in EU). The probe firmware handles this automatically.
- **Timing:** The probe waits ~100ms before responding to commands. Be patient.
- **Multiple Packets:** Telemetry requests may return multiple packets if there's pending data. The ground station will receive all of them.
- **Testing in READY State:** You can test LoRa communication while the probe is in READY state (before drop detection). This avoids landing detection issues during testing.

---

## 📚 Additional Resources

- **Probe Firmware:** See `lib/telemetry/telemetry.cpp` for packet building/parsing
- **LoRa Library:** https://github.com/sandeepmistry/arduino-LoRa
- **PlatformIO Docs:** https://docs.platformio.org/

---

## ✅ Checklist for First Test

Before your first test, verify:

- [ ] Ground station Teensy programmed and connected
- [ ] Probe Teensy programmed and running
- [ ] Both devices have LoRa antennas connected
- [ ] Both devices use same LoRa parameters (frequency, SF, BW, CR)
- [ ] Team ID matches on both devices
- [ ] Serial monitors open on both devices (115200 baud)
- [ ] Devices are within 1-2 meters for initial testing
- [ ] Probe is in READY state (or DESCENT/LANDED)

---

**Last Updated:** Based on firmware from `Comms` branch

