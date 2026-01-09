# LoRa Ground Station Setup Guide

Complete guide for building, programming, and operating the LoRa ground station.

## 📋 Table of Contents

1. [Hardware Assembly](#hardware-assembly)
2. [Firmware Upload](#firmware-upload)
3. [Power On and Verification](#power-on-and-verification)
4. [Usage](#usage)
5. [Troubleshooting](#troubleshooting)

---

## 🔧 Hardware Assembly

### Required Components

1. **Teensy 4.1** (second board, separate from the probe)
2. **RFM95W LoRa module** (same model as on probe)
3. **LoRa antenna** (433/868 MHz, same as probe)
4. **USB cable** for programming and serial communication
5. **Breadboard and jumper wires** (optional, for prototyping)

### Wiring

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

### Antenna Connection

- Connect the LoRa antenna to the RFM95W module
- Ensure antenna is properly seated (screw-on or SMA connector)
- **Never power on LoRa module without antenna** - can damage the module

---

## 💻 Firmware Upload

### Step 1: Configure Team ID

1. Open `src/main.cpp` in this repository
2. Find the line:
   ```cpp
   #define TARGET_TEAM_ID 0x0
   ```
3. Change `0x0` to match your probe's Team ID (0x0-0xF)
4. Save the file

### Step 2: Connect Teensy

1. Connect Teensy 4.1 to computer via USB cable
2. Press the program button on Teensy (if needed)

### Step 3: Build and Upload

```bash
pio run -t upload
```

**Expected output:**
```
Building...
Uploading...
```

### Step 4: Verify Upload

If upload fails:
- Check USB connection
- Press Teensy program button
- Verify correct board selected in PlatformIO
- Check for driver issues

---

## 🔌 Power On and Verification

### Step 1: Open Serial Monitor

```bash
pio device monitor --baud 115200
```

### Step 2: Verify Initialization

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

### Step 3: Verify LoRa Parameters

Check that displayed parameters match probe configuration:
- Frequency: 868.30 MHz
- Spreading Factor: 8
- Bandwidth: 250 kHz
- Coding Rate: 4/6
- TX Power: 14 dBm

**If parameters don't match:** Check `src/main.cpp` configuration constants.

---

## 🚀 Usage

### Serial Commands

Type commands in the serial monitor:

| Command | Action |
|---------|--------|
| `t` | Request telemetry only |
| `s` | Request scientific data only |
| `b` | Request both telemetry and scientific |
| `r` | Display last received telemetry |
| `h` | Show help |

### Sending Commands

1. Type command letter (`t`, `s`, or `b`) in serial monitor
2. Press Enter
3. Ground station will display:
   ```
   [TX] Sending command: TEL=1, SCI=0, Team=0x0
   [TX] Command sent (2 bytes)
   ```

### Receiving Responses

**Telemetry packet (16 bytes):**
```
[RX] Received 16 bytes
[RX] Telemetry parsed successfully:
  Lat: 45.123456°
  Lon: 7.654321°
  Alt: 123.4 m
  V_vert: -0.50 m/s
  Timestamp: 1234567890
```

**Scientific packet (47-82 bytes):**
```
[RX] Received 47 bytes
[RX] Scientific packet received:
  Size: 47 bytes
  Timestamp: 1234567890
  Bitmap: 0x03
  Flags: DET=1, SEC=1, LOC=0, TEL=0
```

---

## 🐛 Troubleshooting

### Problem: No Response from Probe

**Check:**
1. **Team ID** - Verify `TARGET_TEAM_ID` matches probe's `TEAM_ID`
2. **LoRa parameters** - Frequency, SF, BW, CR must match exactly
3. **Antenna** - Both devices must have antennas connected
4. **Distance** - Start with devices < 1 meter apart
5. **Probe state** - Probe must be in READY, DESCENT, or LANDED state

**Verify on probe Serial Monitor:**
- Should see `[LoRa RX] Received 2 bytes`
- Should see `[LoRa RX] Command for this probe!`
- Should see `[LoRa TX] Sending telemetry...`

### Problem: "Failed to start LoRa"

**Check:**
1. Wiring connections (especially CS, RST, INT pins)
2. Power supply (3.3V to RFM95W)
3. Antenna connected
4. SPI pins correct (MISO, MOSI, SCK)

### Problem: Invalid Packet Received

**Check:**
1. CRC errors in telemetry packets
2. Packet size matches expected format
3. Try sending command again
4. Check for interference on 868.3 MHz

### Problem: Ground Station Not Receiving

**Check:**
1. Probe Serial Monitor shows `[LoRa TX]` messages
2. Wait a few seconds after sending command
3. Ensure line-of-sight between antennas
4. Check antenna connections on both devices

---

## ✅ Pre-Flight Checklist

Before testing with probe:

- [ ] Ground station Teensy programmed and connected
- [ ] Probe Teensy programmed and running
- [ ] Both devices have LoRa antennas connected
- [ ] Team ID matches on both devices
- [ ] LoRa parameters match exactly (frequency, SF, BW, CR)
- [ ] Serial monitors open on both devices (115200 baud)
- [ ] Devices within 1-2 meters for initial testing
- [ ] Probe is in READY state (or DESCENT/LANDED)

---

## 📝 Notes

- **Duty Cycle:** LoRa has duty cycle restrictions (1% in EU). The probe firmware handles this automatically.
- **Timing:** The probe waits ~100ms before responding to commands. Be patient.
- **Multiple Packets:** Telemetry requests may return multiple packets if there's pending data.
- **Testing in READY State:** You can test LoRa communication while the probe is in READY state (before drop detection).
