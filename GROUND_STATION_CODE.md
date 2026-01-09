# Ground Station Code

This document contains the complete code for the LoRa Ground Station. Copy these files to create a new PlatformIO project.

## Project Structure

```
LORA_GROUND_STATION/
├── platformio.ini
└── src/
    └── main.cpp
```

---

## platformio.ini

```ini
; PlatformIO Project Configuration File
; Ground Station for CanSat LoRa Communication Testing
; Board: Teensy 4.1

[env:teensy41]
platform  = teensy
board     = teensy41
framework = arduino

; Library dependencies
lib_deps =
    sandeepmistry/LoRa@^0.8.0

; Build options
build_flags =
    -O2
    -std=gnu++17

; Serial / Upload
monitor_speed = 115200
upload_speed  = 921600
```

---

## src/main.cpp

```cpp
#include <Arduino.h>
#include <LoRa.h>

// ===== CONFIGURATION =====

// LoRa Pin Configuration (must match probe)
#define RFM95_CS   10
#define RFM95_INT  14
#define RFM95_RST  15

// LoRa Radio Parameters (must match probe)
#define LORA_FREQ           868.3E6  // 868.30 MHz
#define LORA_TX_POWER       14       // dBm
#define LORA_SPREADING_FACTOR 8      // SF8
#define LORA_CODING_RATE    6        // 4/6
#define LORA_BANDWIDTH      250E3    // 250 kHz

// Target Team ID (must match probe's TEAM_ID)
#define TARGET_TEAM_ID      0x0      // Change to your team ID (0x0-0xF)

// Packet Type Identifiers
#define PACKET_ID_COMMAND        0x00
#define PACKET_ID_TELEMETRY_BASE 0xA0
#define PACKET_ID_SCIENTIFIC_BASE 0x10

// Command Flags
#define CMD_TELEMETRY_REQUEST  0x20   // Bit 5
#define CMD_SCIENTIFIC_REQUEST 0x10  // Bit 4
#define CMD_TEAM_ID_MASK      0x0F   // Bits 3-0

// ===== GLOBAL STATE =====

struct TelemetryData {
    double latitude;
    double longitude;
    float altitude;
    float verticalVel;
    uint32_t timestamp;
    bool valid;
};

TelemetryData lastTelemetry = {0};

uint8_t lastScientificPacket[82] = {0};
size_t lastScientificSize = 0;
bool hasScientificPacket = false;

// ===== CRC-8-CCITT =====

uint8_t calculateCRC8(const uint8_t* data, size_t length) {
    uint8_t crc = 0x00;
    for (size_t i = 0; i < length; i++) {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x80) {
                crc = (crc << 1) ^ 0x07;
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}

// ===== COMMAND PACKET BUILDING =====

void buildCommandPacket(uint8_t* buffer, bool requestTelemetry, bool requestScientific) {
    buffer[0] = PACKET_ID_COMMAND;
    
    uint8_t cmdByte = 0;
    if (requestTelemetry) {
        cmdByte |= CMD_TELEMETRY_REQUEST;
    }
    if (requestScientific) {
        cmdByte |= CMD_SCIENTIFIC_REQUEST;
    }
    cmdByte |= (TARGET_TEAM_ID & CMD_TEAM_ID_MASK);
    
    buffer[1] = cmdByte;
}

bool sendCommand(bool requestTelemetry, bool requestScientific) {
    uint8_t cmdPacket[2];
    buildCommandPacket(cmdPacket, requestTelemetry, requestScientific);
    
    Serial.print("[TX] Sending command: ");
    if (requestTelemetry) Serial.print("TEL=1 ");
    else Serial.print("TEL=0 ");
    if (requestScientific) Serial.print("SCI=1 ");
    else Serial.print("SCI=0 ");
    Serial.printf("Team=0x%X\n", TARGET_TEAM_ID);
    
    LoRa.beginPacket();
    LoRa.write(cmdPacket, 2);
    bool result = LoRa.endPacket(true);  // async
    
    if (result) {
        Serial.println("[TX] Command sent (2 bytes)");
        LoRa.receive();  // Switch back to receive mode
        return true;
    } else {
        Serial.println("[TX] Failed to send command");
        LoRa.receive();
        return false;
    }
}

// ===== TELEMETRY PACKET PARSING =====

int32_t unpack24bitSigned(const uint8_t* buffer) {
    // Unpack 24-bit signed value (Big Endian)
    int32_t value = ((int32_t)buffer[0] << 16) | ((int32_t)buffer[1] << 8) | buffer[2];
    // Sign extend if negative
    if (value & 0x00800000) {
        value |= 0xFF000000;
    }
    return value;
}

int16_t unpack16bitSigned(const uint8_t* buffer) {
    // Unpack 16-bit signed value (Big Endian)
    return ((int16_t)buffer[0] << 8) | buffer[1];
}

uint16_t unpack16bit(const uint8_t* buffer) {
    // Unpack 16-bit unsigned value (Big Endian)
    return ((uint16_t)buffer[0] << 8) | buffer[1];
}

uint32_t unpack32bit(const uint8_t* buffer) {
    // Unpack 32-bit unsigned value (Big Endian)
    return ((uint32_t)buffer[0] << 24) | ((uint32_t)buffer[1] << 16) |
           ((uint32_t)buffer[2] << 8) | buffer[3];
}

bool parseTelemetryPacket(const uint8_t* buffer, size_t length, TelemetryData& data) {
    if (length < 16) {
        Serial.println("[RX] Telemetry packet too short");
        return false;
    }
    
    // Verify packet ID
    uint8_t packetId = buffer[0];
    uint8_t teamId = packetId & 0x0F;
    uint8_t baseId = packetId & 0xF0;
    
    if (baseId != PACKET_ID_TELEMETRY_BASE) {
        Serial.printf("[RX] Invalid telemetry packet ID: 0x%02X\n", packetId);
        return false;
    }
    
    if (teamId != TARGET_TEAM_ID) {
        Serial.printf("[RX] Telemetry from different team: 0x%X\n", teamId);
        return false;
    }
    
    // Verify CRC
    uint8_t receivedCRC = buffer[15];
    uint8_t calculatedCRC = calculateCRC8(buffer, 15);
    
    if (receivedCRC != calculatedCRC) {
        Serial.printf("[RX] CRC mismatch: received=0x%02X, calculated=0x%02X\n",
                      receivedCRC, calculatedCRC);
        return false;
    }
    
    // Parse data
    int32_t latScaled = unpack24bitSigned(&buffer[1]);
    int32_t lonScaled = unpack24bitSigned(&buffer[4]);
    int16_t velScaled = unpack16bitSigned(&buffer[7]);
    uint16_t altScaled = unpack16bit(&buffer[9]);
    uint32_t timestamp = unpack32bit(&buffer[11]);
    
    // Convert to physical units
    data.latitude = (double)latScaled / 100000.0;
    data.longitude = (double)lonScaled / 100000.0;
    data.altitude = (float)altScaled / 10.0f;
    data.verticalVel = (float)velScaled / 100.0f;  // cm/s to m/s
    data.timestamp = timestamp;
    data.valid = true;
    
    return true;
}

// ===== SCIENTIFIC PACKET PARSING =====

void parseScientificPacket(const uint8_t* buffer, size_t length) {
    if (length < 47) {
        Serial.println("[RX] Scientific packet too short");
        return;
    }
    
    uint8_t packetId = buffer[0];
    uint8_t teamId = packetId & 0x0F;
    uint8_t baseId = packetId & 0xF0;
    
    if (baseId != PACKET_ID_SCIENTIFIC_BASE) {
        Serial.printf("[RX] Invalid scientific packet ID: 0x%02X\n", packetId);
        return;
    }
    
    if (teamId != TARGET_TEAM_ID) {
        Serial.printf("[RX] Scientific from different team: 0x%X\n", teamId);
        return;
    }
    
    // Store packet for later analysis
    size_t copySize = (length < 82) ? length : 82;
    memcpy(lastScientificPacket, buffer, copySize);
    lastScientificSize = copySize;
    hasScientificPacket = true;
    
    // Parse basic info
    uint32_t timestamp = unpack32bit(&buffer[1]);
    uint8_t bitmap = buffer[5];
    
    Serial.println("[RX] Scientific packet received:");
    Serial.printf("  Size: %d bytes\n", length);
    Serial.printf("  Timestamp: %u\n", timestamp);
    Serial.printf("  Bitmap: 0x%02X\n", bitmap);
    Serial.printf("  Flags: DET=%d, SEC=%d, LOC=%d, TEL=%d\n",
                  (bitmap & 0x01) ? 1 : 0,
                  (bitmap & 0x02) ? 1 : 0,
                  (bitmap & 0x04) ? 1 : 0,
                  (bitmap & 0x08) ? 1 : 0);
    
    // Note: Full parsing of scientific data requires understanding the encoding
    // This is a simplified version that just displays packet info
}

// ===== PACKET RECEPTION =====

void handleReceivedPacket() {
    int packetSize = LoRa.parsePacket();
    if (packetSize == 0) return;
    
    uint8_t buffer[82];
    size_t bytesRead = 0;
    
    while (LoRa.available() && bytesRead < sizeof(buffer)) {
        buffer[bytesRead++] = LoRa.read();
    }
    
    Serial.printf("[RX] Received %d bytes\n", bytesRead);
    
    // Determine packet type
    uint8_t packetId = buffer[0];
    
    if (packetId == PACKET_ID_TELEMETRY_BASE || 
        (packetId & 0xF0) == PACKET_ID_TELEMETRY_BASE) {
        // Telemetry packet
        TelemetryData tel;
        if (parseTelemetryPacket(buffer, bytesRead, tel)) {
            lastTelemetry = tel;
            Serial.println("[RX] Telemetry parsed successfully:");
            Serial.printf("  Lat: %.6f°\n", tel.latitude);
            Serial.printf("  Lon: %.6f°\n", tel.longitude);
            Serial.printf("  Alt: %.1f m\n", tel.altitude);
            Serial.printf("  V_vert: %.2f m/s\n", tel.verticalVel);
            Serial.printf("  Timestamp: %u\n", tel.timestamp);
        }
    } else if ((packetId & 0xF0) == PACKET_ID_SCIENTIFIC_BASE) {
        // Scientific packet
        parseScientificPacket(buffer, bytesRead);
    } else {
        Serial.printf("[RX] Unknown packet type: 0x%02X\n", packetId);
        Serial.print("[RX] Raw bytes: ");
        for (size_t i = 0; i < bytesRead && i < 20; i++) {
            Serial.printf("%02X ", buffer[i]);
        }
        Serial.println();
    }
}

// ===== SERIAL COMMAND INTERFACE =====

void printHelp() {
    Serial.println();
    Serial.println("Commands:");
    Serial.println("  't' - Send TEL request (telemetry only)");
    Serial.println("  's' - Send SCI request (scientific only)");
    Serial.println("  'b' - Send both (TEL + SCI)");
    Serial.println("  'r' - Show last received telemetry");
    Serial.println("  'h' - Show this help");
    Serial.println();
}

void showLastTelemetry() {
    if (lastTelemetry.valid) {
        Serial.println();
        Serial.println("Last Telemetry:");
        Serial.printf("  Lat: %.6f°\n", lastTelemetry.latitude);
        Serial.printf("  Lon: %.6f°\n", lastTelemetry.longitude);
        Serial.printf("  Alt: %.1f m\n", lastTelemetry.altitude);
        Serial.printf("  V_vert: %.2f m/s\n", lastTelemetry.verticalVel);
        Serial.printf("  Timestamp: %u\n", lastTelemetry.timestamp);
        Serial.println();
    } else {
        Serial.println("No telemetry received yet");
    }
}

void processSerialCommand() {
    if (Serial.available() > 0) {
        char cmd = Serial.read();
        
        switch (cmd) {
            case 't':
            case 'T':
                sendCommand(true, false);
                break;
                
            case 's':
            case 'S':
                sendCommand(false, true);
                break;
                
            case 'b':
            case 'B':
                sendCommand(true, true);
                break;
                
            case 'r':
            case 'R':
                showLastTelemetry();
                break;
                
            case 'h':
            case 'H':
            case '?':
                printHelp();
                break;
                
            case '\n':
            case '\r':
                // Ignore newlines
                break;
                
            default:
                Serial.printf("Unknown command: '%c'. Type 'h' for help.\n", cmd);
                break;
        }
    }
}

// ===== SETUP =====

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    Serial.println("======================================");
    Serial.println("   LoRa Ground Station");
    Serial.println("======================================");
    Serial.printf("Team ID: 0x%X\n", TARGET_TEAM_ID);
    Serial.println();
    
    // Initialize LoRa
    Serial.println("[INIT] Initializing LoRa...");
    LoRa.setPins(RFM95_CS, RFM95_RST, RFM95_INT);
    
    if (!LoRa.begin(LORA_FREQ)) {
        Serial.println("[ERROR] Failed to start LoRa!");
        Serial.println("[ERROR] Check wiring and connections");
        while (1) {
            delay(1000);
        }
    }
    
    Serial.println("LoRa initialized");
    
    // Configure LoRa parameters
    LoRa.setTxPower(LORA_TX_POWER);
    LoRa.setSpreadingFactor(LORA_SPREADING_FACTOR);
    LoRa.setSignalBandwidth(LORA_BANDWIDTH);
    LoRa.setCodingRate4(LORA_CODING_RATE);
    
    Serial.println("LoRa ready and listening...");
    Serial.printf("Freq: %.2f MHz, SF: %d, BW: %.0f kHz, CR: 4/%d, Power: %d dBm\n",
                  LORA_FREQ / 1E6, LORA_SPREADING_FACTOR,
                  LORA_BANDWIDTH / 1E3, LORA_CODING_RATE, LORA_TX_POWER);
    Serial.println();
    
    Serial.println("Ground Station Ready!");
    printHelp();
}

// ===== LOOP =====

void loop() {
    // Check for received packets
    if (LoRa.parsePacket() > 0) {
        handleReceivedPacket();
    }
    
    // Process serial commands
    processSerialCommand();
    
    // Small delay to prevent CPU spinning
    delay(10);
}
```

---

## Quick Start Instructions

1. **Create new PlatformIO project:**
   ```bash
   mkdir LORA_GROUND_STATION
   cd LORA_GROUND_STATION
   pio project init --board teensy41
   ```

2. **Copy files:**
   - Copy `platformio.ini` content to `platformio.ini`
   - Copy `src/main.cpp` content to `src/main.cpp`

3. **Configure Team ID:**
   - Edit `src/main.cpp`
   - Change `#define TARGET_TEAM_ID 0x0` to your team's ID

4. **Build and upload:**
   ```bash
   pio run -t upload
   ```

5. **Open serial monitor:**
   ```bash
   pio device monitor --baud 115200
   ```

6. **Test:**
   - Type `t` to send telemetry request
   - Type `s` to send scientific request
   - Type `b` to send both
   - Type `r` to show last received telemetry
   - Type `h` for help

---

## Notes

- **Team ID:** Must match the probe's `TEAM_ID` in `lib/include/config.h`
- **LoRa Parameters:** Must match exactly (frequency, SF, BW, CR, power)
- **Range:** Start testing with devices < 1 meter apart, then increase distance
- **Timing:** Probe waits ~100ms before responding. Be patient.
- **Multiple Packets:** Telemetry requests may return multiple packets. Ground station will receive all of them.

---

## Troubleshooting

If you don't receive responses:
1. Verify Team ID matches on both devices
2. Check probe Serial Monitor to see if command was received
3. Verify LoRa parameters match exactly
4. Ensure antennas are connected on both devices
5. Start with devices very close (< 1 meter) for initial testing

