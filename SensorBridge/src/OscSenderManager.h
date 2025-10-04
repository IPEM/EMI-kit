#ifndef OSCSENDERMANAGER_H
#define OSCSENDERMANAGER_H

#include <WiFi.h>
#include <WiFiUdp.h>
#include <ESPmDNS.h>
#include <vector>
#include <MicroOsc.h>
#include <MicroOscUdp.h>

struct OscReceiver {
  IPAddress ip;
  uint16_t port;
  String name;
  unsigned long lastSeen;
};

class OscSenderManager {
private:
  std::vector<OscReceiver> receivers;
  WiFiUDP udp;
  MicroOscUdp<1024> osc;
  bool verbose = false;
  unsigned long lastDiscoveryTime;
  static const unsigned long DISCOVERY_INTERVAL = 3000; // 3 seconds

public:
  OscSenderManager();
  
  // Initialize the manager
  bool begin();
  
  // Discover OSC services via mDNS
  void discoverReceivers();
  
  // Send integer value to all receivers
  void sendIntToAll(const char* address, int32_t value);
  
  // Send integer value to all receivers (default address)
  void sendIntToAll(int32_t value);
  
  // Send MIDI message to all receivers
  void sendMidiToAll(const char* address, unsigned char* midi);
  
  // Send list of integers to all receivers
  void sendIntListToAll(const char* address, const std::vector<int32_t>& values);
  
  void sendIntArrayToAll(const char* address, const int* values, size_t count);

  void sendFloatArrayToAll(const char* address, const float* values, size_t count);

  // Get number of discovered receivers
  size_t getReceiverCount() const;
  
  // Print all receivers to serial
  void printReceivers() const;
  
  // Clean up old receivers (not seen for more than 30 seconds)
  void cleanupOldReceivers();

private:
  // Callback for mDNS service discovery
  static void serviceCallback(const char* serviceType, const char* proto, 
                             const char* name, IPAddress ip, unsigned short port, 
                             const char* txtContent);
  
  // Add or update receiver in the list
  void addOrUpdateReceiver(const char* name, IPAddress ip, uint16_t port);
};

#endif
