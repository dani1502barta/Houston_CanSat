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

// Scientific Packet Bitmap Flags
#define BITMAP_TELEMETRY_FLAG        0x08  // Bit 3
#define BITMAP_LOCALISATION_FLAG    0x04  // Bit 2
#define BITMAP_SECONDARY_PAYLOAD_FLAG 0x02  // Bit 1
#define BITMAP_DETONATION_EVENT_FLAG 0x01  // Bit 0

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
    
    // Verify CRC (last byte)
    uint8_t receivedCRC = buffer[length - 1];
    uint8_t calculatedCRC = calculateCRC8(buffer, length - 1);
    
    if (receivedCRC != calculatedCRC) {
        Serial.printf("[RX] CRC mismatch: received=0x%02X, calculated=0x%02X\n",
                      receivedCRC, calculatedCRC);
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
                  (bitmap & BITMAP_DETONATION_EVENT_FLAG) ? 1 : 0,
                  (bitmap & BITMAP_SECONDARY_PAYLOAD_FLAG) ? 1 : 0,
                  (bitmap & BITMAP_LOCALISATION_FLAG) ? 1 : 0,
                  (bitmap & BITMAP_TELEMETRY_FLAG) ? 1 : 0);
    
    // Parse Secondary Payload (bytes 26-45)
    if (bitmap & BITMAP_SECONDARY_PAYLOAD_FLAG && length >= 46) {
        int16_t uvMean = unpack16bitSigned(&buffer[26]);
        int16_t uvStd = unpack16bitSigned(&buffer[28]);
        
        Serial.println("  Secondary Payload:");
        Serial.printf("    UV Mean: %.2f\n", uvMean / 100.0f);
        Serial.printf("    UV Std: %.2f\n", uvStd / 100.0f);
        
        // BMP samples (4 samples, 4 bytes each: pressure + temperature)
        Serial.println("    BMP Samples:");
        for (int i = 0; i < 4 && (30 + i*4 + 3) < length; i++) {
            uint16_t pressure = unpack16bit(&buffer[30 + i*4]);
            int16_t temperature = unpack16bitSigned(&buffer[30 + i*4 + 2]);
            Serial.printf("      [%d] P=%.1f Pa, T=%.2f°C\n", 
                         i, pressure / 10.0f, temperature / 100.0f);
        }
    }
    
    // Parse Detonation Event (bytes 6-25) - simplified display
    if (bitmap & BITMAP_DETONATION_EVENT_FLAG && length >= 26) {
        Serial.println("  Detonation Event: (20 bytes encoded)");
        Serial.print("    Raw: ");
        for (int i = 0; i < 20 && (6 + i) < length; i++) {
            Serial.printf("%02X ", buffer[6 + i]);
        }
        Serial.println();
    }
    
    // Parse Localisation (bytes 46-70, if present)
    if (bitmap & BITMAP_LOCALISATION_FLAG && length >= 71) {
        int32_t latScaled = unpack24bitSigned(&buffer[46]);
        int32_t lonScaled = unpack24bitSigned(&buffer[49]);
        
        Serial.println("  Localisation:");
        Serial.printf("    Probe Lat: %.6f°\n", latScaled / 100000.0);
        Serial.printf("    Probe Lon: %.6f°\n", lonScaled / 100000.0);
        
        // Detonation positions (distance + azimuth pairs)
        int locIdx = 52;
        int detCount = 0;
        while (locIdx < length - 1 && detCount < 4) {
            uint16_t distance = unpack16bit(&buffer[locIdx]);
            int16_t azimuthScaled = unpack16bitSigned(&buffer[locIdx + 2]);
            float azimuth = azimuthScaled / 1000.0f;
            
            Serial.printf("    Detonation [%d]: dist=%.1fm, azim=%.3f°\n",
                         detCount, (float)distance, azimuth);
            locIdx += 4;
            detCount++;
        }
    }
    
    // Parse Mini Telemetry (if present, at the end before CRC)
    if (bitmap & BITMAP_TELEMETRY_FLAG && length >= 77) {
        Serial.println("  Mini Telemetry: (10 bytes)");
        Serial.print("    Raw: ");
        size_t telStart = length - 11;  // 10 bytes + 1 CRC
        for (int i = 0; i < 10 && (telStart + i) < length - 1; i++) {
            Serial.printf("%02X ", buffer[telStart + i]);
        }
        Serial.println();
    }
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

