# LoRa Ground Station

Ground station firmware for testing LoRa communication with the CanSat probe.

## Overview

This firmware runs on a second Teensy 4.1 that acts as a LoRa ground station. It can:
- **Send command packets** to request telemetry (TEL) or scientific data (SCI) from the probe
- **Receive and parse** telemetry and scientific packets from the probe
- **Provide a serial interface** for interactive testing

## Hardware Requirements

- **Teensy 4.1** (second board, separate from the probe)
- **RFM95W LoRa module** (same model as on probe)
- **LoRa antenna** (433/868 MHz, same as probe)
- **USB cable** for programming and serial communication

### Pin Connections

| RFM95W Pin | Teensy 4.1 Pin |
|------------|----------------|
| CS         | 10             |
| RST        | 15             |
| INT (DIO0) | 14             |
| MOSI       | 11             |
| MISO       | 12             |
| SCK        | 13             |
| VCC        | 3.3V           |
| GND        | GND            |

## Quick Start

### Step 1: Configure Team ID

1. Open `src/main.cpp` in this repository
2. Find the line near the top (around line 20-30):
   ```cpp
   #define TARGET_TEAM_ID 0x0
   ```
3. Change `0x0` to your probe's Team ID (must be 0x0-0xF, e.g., `0x5` for Team 5)
4. Save the file

**Important:** This Team ID must match the `TEAM_ID` defined in your probe's `lib/include/config.h`.

### Step 2: Connect Hardware

1. Connect Teensy 4.1 to computer via USB cable
2. Ensure RFM95W LoRa module is wired correctly (see [Hardware Requirements](#hardware-requirements))
3. **Connect LoRa antenna to RFM95W module**
   - Ensure antenna is properly seated (screw-on or SMA connector)
   - **⚠️ CRITICAL: Never power on LoRa module without antenna** - can damage the module
4. Press Teensy program button if needed

### Step 3: Build and Upload Firmware

1. Open terminal/command prompt in this repository directory
2. Run:
   ```bash
   pio run -t upload
   ```
3. Wait for compilation and upload to complete
4. If upload fails, press Teensy program button and try again

### Step 4: Open Serial Monitor

1. In the same terminal, run:
   ```bash
   pio device monitor --baud 115200
   ```
2. You should see initialization messages:
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

### Step 5: Verify LoRa Parameters

Check that displayed parameters match probe configuration:
- **Frequency:** 868.30 MHz
- **Spreading Factor:** 8
- **Bandwidth:** 250 kHz
- **Coding Rate:** 4/6
- **TX Power:** 14 dBm

**If parameters don't match:** Check `src/main.cpp` configuration constants.

### Step 6: Send Commands

Once the ground station is ready, type commands in the serial monitor:

- **`t`** + Enter → Send telemetry request (TEL only)
- **`s`** + Enter → Send scientific request (SCI only)
- **`b`** + Enter → Send both requests (TEL + SCI)
- **`r`** + Enter → Show last received telemetry
- **`h`** + Enter → Show help menu

## Serial Commands

| Command | Action |
|---------|--------|
| `t` | Send telemetry request (TEL only) |
| `s` | Send scientific request (SCI only) |
| `b` | Send both requests (TEL + SCI) |
| `r` | Show last received telemetry |
| `h` | Show help |

## Configuration

### LoRa Parameters

These **must match exactly** with the probe configuration:
- **Frequency:** 868.3 MHz
- **Spreading Factor:** 8 (SF8)
- **Bandwidth:** 250 kHz
- **Coding Rate:** 4/6
- **TX Power:** 14 dBm

### Team ID

The `TARGET_TEAM_ID` in `src/main.cpp` must match the `TEAM_ID` in the probe's `lib/include/config.h`.

## Documentation

- **`src/main.cpp`** - Complete ground station source code

## Pre-Flight Checklist

Before testing with probe:

- [ ] Ground station Teensy programmed and connected
- [ ] Probe Teensy programmed and running
- [ ] Both devices have LoRa antennas connected
- [ ] Team ID matches on both devices
- [ ] LoRa parameters match exactly (frequency, SF, BW, CR)
- [ ] Serial monitors open on both devices (115200 baud)
- [ ] Devices within 1-2 meters for initial testing
- [ ] Probe is in READY state (or DESCENT/LANDED)

## Testing

1. Start with devices close (< 1 meter) for initial testing
2. Verify Team ID matches on both devices
3. Check probe Serial Monitor to see if command was received
4. Verify LoRa parameters match exactly
5. Ensure antennas are connected on both devices

## Troubleshooting

If you don't receive responses:
1. Verify Team ID matches on both devices
2. Check probe Serial Monitor to see if command was received
3. Verify LoRa parameters match exactly
4. Ensure antennas are connected on both devices
5. Start with devices very close (< 1 meter) for initial testing
