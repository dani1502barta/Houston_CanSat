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

1. **Configure Team ID in `src/main.cpp`:**
   ```cpp
   #define TARGET_TEAM_ID 0x0  // Change to match your probe's Team ID
   ```

2. **Build and upload:**
   ```bash
   pio run -t upload
   ```

3. **Open serial monitor:**
   ```bash
   pio device monitor --baud 115200
   ```

4. **Send commands:**
   - Type `t` to send telemetry request
   - Type `s` to send scientific request
   - Type `b` to send both
   - Type `r` to show last received telemetry
   - Type `h` for help

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

- **`docs/GROUND_STATION_README.md`** - Quick reference guide
- **`docs/LORA_GROUND_STATION_SETUP.md`** - Complete setup and usage guide
- **`src/main.cpp`** - Complete ground station source code

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
