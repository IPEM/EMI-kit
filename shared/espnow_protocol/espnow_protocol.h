#ifndef ESPNOW_PROTOCOL_H
#define ESPNOW_PROTOCOL_H

#include <stdint.h>

// WiFi channel used by both sender and receiver (must match)
#define ESPNOW_CHANNEL 1

// Message type identifiers
#define ESPNOW_MSG_MIDI 0x4D // 'M'

// MIDI message over ESP-NOW (4 bytes)
struct EspNowMidiMessage {
    uint8_t type;                // ESPNOW_MSG_MIDI
    uint8_t command_and_channel; // Status byte: 0x80-0xFF
    uint8_t parameter1;          // Data byte 1: 0-127
    uint8_t parameter2;          // Data byte 2: 0-127
} __attribute__((packed));

#endif
