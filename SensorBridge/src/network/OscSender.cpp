#include "OscSender.h"

OscSender::OscSender(const char* targetIPStr, unsigned int targetPort, unsigned int localPort) 
    : osc(&udp, IPAddress(), targetPort), targetPort(targetPort), localPort(localPort) {
    
    // Parse IP address string
    targetIP.fromString(targetIPStr);
    
    // Set the destination in the OSC object
    osc.setDestination(targetIP, targetPort);
}

bool OscSender::begin() {

    // Start UDP
    udp.begin(localPort);
    Serial.print("UDP started on port: ");
    Serial.println(localPort);
    Serial.print("Sending to: ");
    Serial.print(targetIP);
    Serial.print(":");
    Serial.println(targetPort);
    
    return true;
}

void OscSender::sendInt(const char* address, int32_t value) {
    osc.sendInt(address, value);
    Serial.print("Sent OSC message: ");
    Serial.print(address);
    Serial.print(" ");
    Serial.println(value);
}

void OscSender::sendInt(int32_t value) {
    sendInt("/value", value);
}

bool OscSender::isConnected() {
    return WiFi.status() == WL_CONNECTED;
}

void OscSender::setDestination(const char* targetIPStr, unsigned int port) {
    targetIP.fromString(targetIPStr);
    targetPort = port;
    osc.setDestination(targetIP, targetPort);
    
    Serial.print("Destination updated to: ");
    Serial.print(targetIP);
    Serial.print(":");
    Serial.println(targetPort);
}