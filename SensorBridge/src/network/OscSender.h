#ifndef OSCSENDER_H
#define OSCSENDER_H

#include <WiFi.h>
#include <WiFiUdp.h>
#include <MicroOsc.h>
#include <MicroOscUdp.h>

class OscSender {
private:
    WiFiUDP udp;
    MicroOscUdp<1024> osc;
    IPAddress targetIP;
    unsigned int targetPort;
    unsigned int localPort;

public:
    // Constructor
    OscSender(const char* targetIPStr = "192.168.88.137", 
              unsigned int targetPort = 7777, 
              unsigned int localPort = 8888);
    
    // Initialize WiFi and UDP
    bool begin();
    
    // Send an integer value
    void sendInt(const char* address, int32_t value);
    
    // Send an integer with default address "/value"
    void sendInt(int32_t value);
    
    // Check if connected
    bool isConnected();
    
    // Update target destination
    void setDestination(const char* targetIPStr, unsigned int port);
};

#endif
