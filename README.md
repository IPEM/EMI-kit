# EMI-Kit - The Embodied Music Interface Kit

EMI-Kit is a low-cost, open-source system for controlling music production software through body movement. Designed for creative music practice and embodied music interaction research. EMI-Kit can be used as a starting platform to control music parameters with a variety of sensors with some custom programming.

## Introduction

EMI-Kit enables musicians, producers, and researchers to use natural body movements to control Digital Audio Workstation (DAW) parameters. The system consists of two ESP32-based microcontrollers:

- **Sender** - A wearable device that captures orientation (pitch/yaw/roll) and tap gestures from an IMU sensor
- **Receiver** - A USB-connected device that converts sensor data to MIDI messages for your DAW

Unlike expensive commercial alternatives, EMI-Kit is:
- **Affordable** - Built with accessible M5Stack hardware (~€30-50 total)
- **Open source** - Fully customizable and transparent
- **Modular** -  With some programming, other sensors and sensor mappings can be adapted to your workflow
- **Low latency** - Wireless communication (ESP-NOW, or OSC over WiFi) with USB MIDI output


## Sender

### Hardware
- **Device**: M5StickC Plus2
- **Microcontroller**: ESP32-PICO-V3-02
- **Sensors**: 6-axis IMU (accelerometer + gyroscope)
- **Display**: 1.14" LCD (135x240 pixels)
- **Battery**: Built-in rechargeable Li-Po
- **Connectivity**: WiFi 802.11 b/g/n

**Purchase**: [M5Stack Store](https://shop.m5stack.com/products/m5stickc-plus2-esp32-mini-iot-development-kit)

> The PlatformIO environments currently build with `board = m5stick-c`; the custom board file `SensorBridge/boards/m5stickc_plus2.json` is not used yet.

### Software

The sender firmware (`SensorBridge/src/main.cpp`) handles:

**Sensor Processing:**
- AHRS (Attitude and Heading Reference System) for orientation calculation
- Automatic gyroscope calibration on boot
- Tap detection via accelerometer magnitude threshold
- 200Hz IMU sampling with 50Hz MIDI transmission

**Communication** (see [Transports](#transports)):
- ESP-NOW: MIDI messages broadcast directly to the receiver, no WiFi setup needed
- WiFi + OSC:
  - WiFi provisioning via web interface: starts automatically when no WiFi is configured, or with a long press on button B
  - Automatic receiver discovery via mDNS/DNS-SD
  - OSC message transmission over UDP port 8888

**User Interface:**
- LCD shows connection status, streaming mode, and orientation values
- Button A (short press): cycle through streaming modes
  - All sensors (pitch + yaw + roll + tap)
  - Individual sensors
  - Off
- Button A (long press): set zero point for orientation
- Button B (long press): enter WiFi provisioning mode (WiFi + OSC build only)

**MIDI Mappings:**
- CC 80: Pitch (-90° to +90° → 0-127)
- CC 81: Yaw (-90° to +90° → 0-127)  
- CC 82: Roll (-90° to +90° → 0-127)
- Note C3 (60): Tap detection (velocity 100)

## Receiver



### Hardware
- **Device**: M5Stack STAMP S3
- **Microcontroller**: ESP32-S3A
- **Connectivity**: WiFi 802.11 b/g/n, USB-C
- **LED**: WS2812B RGB LED for status indication

**Purchase**: [M5Stack Store](https://shop.m5stack.com/products/m5stamp-esp32s3a-module)

### Software

The receiver firmware (`OscToMidi/src/main.cpp`) handles:


**MIDI over ESP-NOW** (ESP-NOW build):
- Listens for ESP-NOW broadcasts on WiFi channel 1
- Drops malformed packets

**OSC to MIDI Translation** (WiFi + OSC build):
- WiFi Access Point mode (SSID: `OSC-to-MIDI-XX` where XX is device ID)
- mDNS service advertisement (`osc-to-midi._osc._udp.local`)
- Receives OSC messages at address `/midi`

**USB MIDI Output:**
- TinyUSB MIDI implementation
- Virtual cable support for multi-channel routing
- Compatible with Windows, macOS, Linux, Android

## Transports

Sender and receiver must be built for the same transport. Each is selected by a PlatformIO environment:

| Transport | Receiver (`OscToMidi`) | Sender (`SensorBridge`) |
|---|---|---|
| ESP-NOW | `pio run -e esp32-s3-devkitc-1-espnow` | `pio run -e m5stick-c-espnow` |
| WiFi + OSC | `pio run -e esp32-s3-devkitc-1` | `pio run -e m5stick-c` |

Add `-t upload` to flash. The ESP-NOW protocol is defined once in `shared/espnow_protocol/espnow_protocol.h`.

**ESP-NOW** needs no WiFi network or provisioning and has the lowest latency. Senders *broadcast* on channel 1 and there is no pairing, so **every ESP-NOW receiver in range plays the messages of every ESP-NOW sender**. When several kits share a room, use the WiFi + OSC build, which gives each receiver its own network.

**WiFi + OSC**: the receiver creates its own access point and the sender connects to it, then sends `/midi` OSC messages over UDP.

To print MIDI on the receiver's Serial port instead of acting as a USB MIDI device, remove `-DUSE_USB_MIDI` from the environment's `build_flags`.

Run the receiver's unit tests on the host with `pio test -e native` (in `OscToMidi`).

## Getting Started

1. **Flash firmware** to both devices using PlatformIO, choosing the same [transport](#transports) for both
2. **Power on receiver** - WiFi + OSC: creates WiFi network `OSC-to-MIDI-XX`
3. **Power on sender** - ESP-NOW: starts sending immediately; WiFi + OSC: connects to the receiver (provision the WiFi network on first use)
4. **Connect receiver** to computer via USB
5. **Configure DAW** to receive MIDI from "OSC to MIDI Converter"
6. **Map MIDI CCs** to desired parameters in your DAW
7. **Move and create!**

## Debugging Tools

- **mot** - MIDI and OSC command-line tools for debugging and monitoring
  - [GitHub: JorenSix/mot](https://github.com/JorenSix/mot)

## References

**Hardware:**
- [M5StickC Plus2 Documentation](https://shop.m5stack.com/products/m5stickc-plus2-esp32-mini-iot-development-kit)
- [M5Stack STAMP S3 Documentation](https://shop.m5stack.com/products/m5stamp-esp32s3a-module)
- [M5Stack Getting Started Guide](https://github.com/Edinburgh-College-of-Art/m5stickc-plus-introduction)

**Software:**
- [M5Unified Library](https://registry.platformio.org/libraries/m5stack/M5Unified)
- [TinyUSB MIDI](https://github.com/hathach/tinyusb)
- [MicroOsc](https://github.com/thomasfredericks/MicroOsc)

## Credits

**Development by:**
- Bart Moens - [IPEM, Ghent University](https://www.ugent.be/lw/kunstwetenschappen/ipem/en), [XRHIL](https://xrhil.ugent.be), Ghent University
- Joren Six - Ghent Centre for Digital Humanities (GhentCDH), Ghent University

Partly funded by **MuTechLab**, __a series of workshops funded by the Luxembourgish National Research Fund (FNR, PSP-Classic)__.
