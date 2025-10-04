#include <Arduino.h>
#include <ESPmDNS.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <MicroOscUdp.h>
#include <FastLED.h> 

// For debugging purposes; 
// Use this switch to enable USB MIDI functionality
#define USE_USB_MIDI

// Some midi receivers (like DAWs) do not like to receive the same MIDI message twice in a row.
// For example, sending the same cc message with identical values multiple times in a row can cause issues.
const boolean filterDuplicateMessages = true; // Set to true to filter out duplicate messages
// Structure to store MIDI message history
struct MidiMessage {
  uint8_t command_and_channel;
  uint8_t parameter1;
  uint8_t parameter2;
};
const size_t midi_duplicate_filter_size = 5; // Number of previous messages to remember
static MidiMessage midi_duplicate_filter_history[midi_duplicate_filter_size];
static int midi_duplicate_filter_history_index = 0;

// WiFi Access Point credentials
const int device_id = 6;
const char* ap_ssid_format = "OSC-to-MIDI-%02d"; 
char ap_ssid[32];
const char* ap_password = "midi1234";
const char ap_channel = (device_id * 3) % 10 + 1; //avoids that device 1 and two are on a neighboring channel

// LED configuration forM5Stack STAMP-S3
#define PIN_LED    21
#define NUM_LEDS   1
CRGB leds[NUM_LEDS];
#define LED_EN     38

// UDP and OSC setup
WiFiUDP myUdp;
unsigned int myReceivePort = 8888;  // Port to receive OSC messages
IPAddress mySendIp(0, 0, 0, 0);  // Placeholder IP (not used for receiving only)
unsigned int mySendPort = 0;  // Placeholder port (not used for receiving only)

#ifdef USE_USB_MIDI
  boolean enableSerial = false;
#else
  boolean enableSerial = true;
#endif

// MicroOsc instance with 1024 bytes buffer for incoming messages
MicroOscUdp<1024> myMicroOsc(&myUdp, mySendIp, mySendPort);

// MIDI constants
#define PITCH_BEND 0xE0
static uint8_t const cable_num = 0; // MIDI jack associated with USB endpoint
static uint8_t const channel = 0;   // 0 for channel 1

#ifdef USE_USB_MIDI
  #include "USB.h"
  #include "esp32-hal-tinyusb.h"
  // TinyUSB MIDI descriptor
  extern "C" uint16_t tusb_midi_load_descriptor(uint8_t *dst, uint8_t *itf) {
    uint8_t str_index = tinyusb_add_string_descriptor("OSC to MIDI Converter");
    uint8_t ep_num = tinyusb_get_free_duplex_endpoint();
    TU_VERIFY(ep_num != 0);
    uint8_t descriptor[TUD_MIDI_DESC_LEN] = {
        TUD_MIDI_DESCRIPTOR(*itf, str_index, ep_num, (uint8_t)(0x80 | ep_num), 64)
    };
    *itf += 2;//Two interfaces: MIDI and Audio Control for windows compatibility
    memcpy(dst, descriptor, TUD_MIDI_DESC_LEN);
    return TUD_MIDI_DESC_LEN;
  }
#endif

// Function declarations
void setupWiFi();
void setupUDP();
void setupMDNS();
void setupUSBMIDI();
void setupHeartbeatLed();
bool isDuplicateMIDIMessage(uint8_t command_and_channel, uint8_t parameter1, uint8_t parameter2);
void sendMidiCC(uint8_t controller, uint8_t value);
void myOnOscMessageReceived(MicroOscMessage& receivedOscMessage);
void handleMidiMessage(MicroOscMessage& message);

// Setup USB MIDI
#ifdef USE_USB_MIDI
  void setupUSBMIDI() {
      tinyusb_enable_interface(USB_INTERFACE_MIDI, TUD_MIDI_DESC_LEN, tusb_midi_load_descriptor);
      USB.begin();
      if (enableSerial) Serial.println("USB MIDI initialized");
  }
#else
  // Dummy function when USB MIDI is not enabled
  void setupUSBMIDI() {
    if (enableSerial) Serial.println("USB MIDI initialization called - but USB MIDI is disabled");
  }
#endif


#ifdef USE_USB_MIDI
// Send MIDI standard message over USB MIDI
void sendMidiMessage(uint8_t command_and_channel, uint8_t parameter1, uint8_t parameter2,uint8_t virtual_cable_num) {

  if (filterDuplicateMessages && isDuplicateMIDIMessage(command_and_channel, parameter1, parameter2)) {
    if(enableSerial) Serial.printf("Duplicate MIDI message detected, not sending. Command: %d, Parameter 1: %d, Parameter 2: %d\n", command_and_channel, parameter1, parameter2);
    return;
  }

  uint8_t code_index = (command_and_channel >> 4) & 0x0F;  // Extract upper nibble
  uint8_t header = (virtual_cable_num << 4) | code_index;  // Combine cable + code index

  if (tud_midi_mounted()) {
    // Create 4-byte MIDI packet: [cable_num + code_index, MIDI_0, MIDI_1, MIDI_2]
    uint8_t packet[4] = {
      header,  // Cable 0 + Control Change code index (0xB)
      command_and_channel,  // Control Change + channel
      parameter1,  // Controller number
      parameter2        // Controller value
    };
    tud_midi_packet_write(packet);

    if (enableSerial) Serial.printf("Sent MIDI Message: command %d, parameter 1 %d, parameter 2 %d\n", command_and_channel, parameter1, parameter2);
  } else {
    if (enableSerial) Serial.println("MIDI not mounted, cannot send message");
  }
}
#else
// Dummy function when USB MIDI is not enabled
void sendMidiMessage(uint8_t command_and_channel, uint8_t parameter1, uint8_t parameter2,uint8_t virtual_cable_num) {

  if (enableSerial) {
    // Parse command type from upper nibble
    uint8_t command_type = command_and_channel & 0xF0;
    uint8_t channel_from_command = command_and_channel & 0x0F;

    Serial.printf("MIDI Debug [Cable:%d] [Ch:%d] ", virtual_cable_num, channel_from_command + 1);

    if (filterDuplicateMessages && isDuplicateMIDIMessage(command_and_channel, parameter1, parameter2)) {
      if(enableSerial) Serial.printf("Duplicate MIDI message detected, not sending. Command: %d, Parameter 1: %d, Parameter 2: %d\n", command_and_channel, parameter1, parameter2);
      return;
    }
    
    switch (command_type) {
      case 0x80:
        Serial.printf("Note OFF: Note=%d, Velocity=%d", parameter1, parameter2);
        break;
      case 0x90:
        Serial.printf("Note ON: Note=%d, Velocity=%d", parameter1, parameter2);
        break;
      case 0xB0:
        Serial.printf("Control Change: CC=%d, Value=%d", parameter1, parameter2);
        break;
      case 0xC0:
        Serial.printf("Program Change: Program=%d", parameter1);
        break;
      case 0xE0:
        Serial.printf("Pitch Bend: LSB=%d, MSB=%d (Value=%d)", parameter1, parameter2, (parameter2 << 7) | parameter1);
        break;
      default:
        Serial.printf("Unknown Command: 0x%02X, Data1=%d, Data2=%d", command_and_channel, parameter1, parameter2);
        break;
    }
    Serial.println();
  }
}
#endif



bool isDuplicateMIDIMessage(uint8_t command_and_channel, uint8_t parameter1, uint8_t parameter2) {
  // Search through history to find an entry with matching command_and_channel
  for (size_t i = 0; i < midi_duplicate_filter_size; i++) {
    // Found an entry with the same command and channel
    if (midi_duplicate_filter_history[i].command_and_channel == command_and_channel) {
      // Check if all parameters match (this would be a duplicate)
      if (midi_duplicate_filter_history[i].parameter1 == parameter1 && 
          midi_duplicate_filter_history[i].parameter2 == parameter2) {
        return true; // It's a duplicate - same command/channel with same parameters
      } else {
        // Same command/channel but different parameters - update and not a duplicate
        midi_duplicate_filter_history[i].parameter1 = parameter1;
        midi_duplicate_filter_history[i].parameter2 = parameter2;
        return false;
      }
    }
  }
  
  // command_and_channel not found in history - need to add it
  // Find an empty slot or use the oldest slot (LRU replacement)
  int slot_to_use = -1;
  
  // First, try to find an empty slot (command_and_channel == 0)
  for (size_t i = 0; i < midi_duplicate_filter_size; i++) {
    if (midi_duplicate_filter_history[i].command_and_channel == 0) {
      slot_to_use = i;
      break;
    }
  }
  
  // If no empty slot, use a simple rotating index (oldest entry)
  if (slot_to_use == -1) {
    slot_to_use = midi_duplicate_filter_history_index % midi_duplicate_filter_size;
    midi_duplicate_filter_history_index++;
  }
  
  // Add current message to the slot
  midi_duplicate_filter_history[slot_to_use].command_and_channel = command_and_channel;
  midi_duplicate_filter_history[slot_to_use].parameter1 = parameter1;
  midi_duplicate_filter_history[slot_to_use].parameter2 = parameter2;
  
  return false; // Not a duplicate (new command_and_channel)
}


void setupHeartbeatLed(){
   pinMode(LED_EN, OUTPUT);
   digitalWrite(LED_EN, HIGH);
  
// Initialize FastLED for STAMP-S3 compatible LED
  FastLED.addLeds<WS2812B, PIN_LED, GRB>(leds, NUM_LEDS);

  // Set initial color (red for "ready")
  leds[0] = CRGB::Red;
  FastLED.show();
  
  if (enableSerial) Serial.println("Heartbeat LED initialized on pin 21");
}

void toggleHeartbeatLed(CRGB color) {
  if (leds[0] == CRGB::Black) {
    leds[0] = color;  // Use the provided color when active
  } else {
    leds[0] = CRGB::Black; // Off
  }
  if (enableSerial) Serial.println("Heartbeat LED toggled to " + String(leds[0] == CRGB::Black ? "Off" : "On"));
  FastLED.show();
}

// Setup WiFi Access Point
void setupWiFi() {  
  if (enableSerial) Serial.print("Creating WiFi Access Point: ");
  if (enableSerial) Serial.println(ap_ssid);

  // Set SSID based on device ID
  sprintf(ap_ssid, ap_ssid_format, device_id);
  
  // Configure Access Point
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_MODE_AP);
  delay(100); // Short delay to ensure mode is set
  WiFi.softAP(ap_ssid, ap_password,ap_channel);
  
  // Wait for AP to start
  delay(2000);
  
  if (enableSerial) Serial.println();
  if (enableSerial) Serial.println("WiFi Access Point created!");
  if (enableSerial) Serial.print("AP IP address: ");
  if (enableSerial) Serial.println(WiFi.softAPIP());
  if (enableSerial) Serial.printf("Connect to: %s\n", ap_ssid);
  if (enableSerial) Serial.printf("Password: %s\n", ap_password);
  if (enableSerial) Serial.printf("Channel: %d\n", ap_channel);
}

// Setup UDP server
void setupUDP() {
  myUdp.begin(myReceivePort);
  if (enableSerial) Serial.print("UDP server started on port: ");
  if (enableSerial) Serial.println(myReceivePort);
}

// Setup mDNS service discovery
void setupMDNS() {
  // Initialize mDNS
  if (!MDNS.begin("osc-to-midi")) {
    if (enableSerial) Serial.println("Error starting mDNS");
    return;
  }
  if (enableSerial) Serial.println("mDNS responder started");
  
  // Add OSC UDP service to mDNS-SD
  MDNS.addService("osc", "udp", myReceivePort);
  if (enableSerial) Serial.println("mDNS service registered: osc-to-midi._osc._udp.local on port 8888");
}

static unsigned long prevMillis = 0;
// Handle MIDI messages from OSC
void handleMidiMessage(MicroOscMessage& message) {  
  // Read the first integer as controller number, second as value
  int32_t command_and_channel = message.nextAsInt();
  int32_t parameter1 = message.nextAsInt();
  int32_t parameter2 = message.nextAsInt();
  uint8_t virtual_cable_num = 0; // Using cable 0 for simplicity
  
  // Constrain values to valid MIDI ranges
  // Command and channel: is between 0x7F and 0xF0
  command_and_channel = constrain(command_and_channel, 0x80, 0xFF);
  parameter1 = constrain(parameter1, 0, 127);
  parameter2 = constrain(parameter2, 0, 127);
  
  unsigned long currMillis = millis();
  unsigned long diff = currMillis - prevMillis;
  if (enableSerial) Serial.printf("[%lu ms] OSC to MIDI: Command and channel %d, Parameter 1 %d, Parameter 2 %d (Δ%lu ms)\n", currMillis, command_and_channel, parameter1, parameter2, diff);
  prevMillis = currMillis;
  
  // Send MIDI CC message
  sendMidiMessage((uint8_t)command_and_channel, (uint8_t)parameter1, (uint8_t)parameter2, virtual_cable_num);
}

// Function that will be called when an OSC message is received
void myOnOscMessageReceived(MicroOscMessage& receivedOscMessage) {  
  // Handle MIDI messages at address "/midi"
  if (receivedOscMessage.checkOscAddress("/midi")) {
    handleMidiMessage(receivedOscMessage);
  } else {
    if (enableSerial) Serial.println("Received OSC message with unhandled address");
  }
  toggleHeartbeatLed(CRGB::Green); // Blink green to indicate message received
}

void setupSerial(){
  Serial.begin(115200);
  while (!Serial) {
    ; // Wait for serial port to connect. Needed for native USB
  }
}

void setup() {

  if (enableSerial) setupSerial();
  delay(100); // Give some time for serial to initialize
  if (enableSerial) Serial.println("Starting OSC to MIDI converter...");
  
  
  setupUSBMIDI();
  setupWiFi();
  setupUDP();
  setupMDNS();
  setupHeartbeatLed();

  for (size_t i = i=0; i < 10; i++) {
    toggleHeartbeatLed(CRGB::Red); // Blink green to indicate ready state
    delay(500);
  }
  
  if (enableSerial) Serial.println("Ready to receive OSC messages and send MIDI!");
}

void loop() {
  // Check for incoming OSC messages and call the callback function for each received message
  myMicroOsc.onOscMessageReceived(myOnOscMessageReceived);

  #ifdef USE_USB_MIDI
    // Keep USB MIDI responsive
    //tud_task(); // Process USB stack for MIDI
  #endif
  
  // Small delay to prevent overwhelming the CPU
  delay(2);
}
