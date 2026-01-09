# LoRa Ground Station - Quick Reference

This is a quick reference guide for the LoRa Ground Station. For detailed setup instructions, see `LORA_GROUND_STATION_SETUP.md`.

## Quick Start

1. **Create PlatformIO project:**
   ```bash
   mkdir LORA_GROUND_STATION && cd LORA_GROUND_STATION
   pio project init --board teensy41
   ```

2. **Copy code from `GROUND_STATION_CODE.md`:**
   - Copy `platformio.ini` → `platformio.ini`
   - Copy `src/main.cpp` → `src/main.cpp`

3. **Configure Team ID in `src/main.cpp`:**
   ```cpp
   #define TARGET_TEAM_ID 0x0  // Change to your team ID
   ```

4. **Build and upload:**
   ```bash
   pio run -t upload
   pio device monitor --baud 115200
   ```

## Serial Commands

| Command | Action |
|---------|--------|
| `t` | Send telemetry request (TEL only) |
| `s` | Send scientific request (SCI only) |
| `b` | Send both requests (TEL + SCI) |
| `r` | Show last received telemetry |
| `h` | Show help |

## Expected Output

### When sending command:
```
[TX] Sending command: TEL=1 SCI=0 Team=0x0
[TX] Command sent (2 bytes)
```

### When receiving telemetry:
```
[RX] Received 16 bytes
[RX] Telemetry parsed successfully:
  Lat: 45.123456°
  Lon: 7.654321°
  Alt: 123.4 m
  V_vert: -0.50 m/s
  Timestamp: 1234567890
```

### When receiving scientific:
```
[RX] Received 47 bytes
[RX] Scientific packet received:
  Size: 47 bytes
  Timestamp: 1234567890
  Bitmap: 0x03
  Flags: DET=1, SEC=1, LOC=0, TEL=0
```

## Troubleshooting

**No response from probe:**
- Check Team ID matches on both devices
- Verify LoRa parameters match exactly
- Check probe Serial Monitor to see if command was received
- Ensure antennas are connected
- Start with devices < 1 meter apart

**Invalid packet:**
- Check CRC errors in telemetry packets
- Verify packet size matches expected format
- Try sending command again

## Configuration Checklist

Before testing, verify:
- [ ] Team ID matches probe's `TEAM_ID`
- [ ] LoRa frequency: 868.3 MHz
- [ ] Spreading Factor: 8
- [ ] Coding Rate: 4/6
- [ ] Bandwidth: 250 kHz
- [ ] TX Power: 14 dBm
- [ ] Pin assignments match probe (CS=10, INT=14, RST=15)

## Files Reference

- **`LORA_GROUND_STATION_SETUP.md`** - Complete setup guide with detailed instructions
- **`GROUND_STATION_CODE.md`** - Complete source code for ground station
- **`COMPRESSED_CHECKLIST.md`** - Testing checklist that includes LoRa tests

