#include "OscSenderManager.h"

OscSenderManager::OscSenderManager() 
  : osc(&udp), lastDiscoveryTime(0) {
}

bool OscSenderManager::begin() {

  if(mdns_init()!= ESP_OK){
    if (verbose) Serial.println("mDNS failed to start");
    return false;
  }

  // Initialize UDP for sending
  udp.begin(0); // Use any available port for sending
  if(verbose) Serial.println("OSC Sender Manager initialized");
  return true;
}

void OscSenderManager::discoverReceivers() {
  if (verbose) Serial.printf("Browsing for service _osc._udp.local. ... ");


  int n = MDNS.queryService("osc", "udp");

  // Track which receivers are present in this discovery
  std::vector<std::pair<IPAddress, uint16_t>> foundReceivers;
  if (n == 0) {
    if (verbose) Serial.println("no services found");
  } else {
    if (verbose) {
      Serial.print(n);
      Serial.println(" service(s) found");
    }
    for (int i = 0; i < n; ++i) {
      if (verbose) {
        Serial.print("  ");
        Serial.print(i + 1);
        Serial.print(": ");
        String hostname = MDNS.hostname(i);
        int dotIndex = hostname.indexOf('.');
        String instanceName = (dotIndex > 0) ? hostname.substring(0, dotIndex) : hostname;
        Serial.print(instanceName);
        Serial.print(" - ");
        Serial.print(hostname);
        Serial.print(" (");
        Serial.print(MDNS.IP(i));
        Serial.print(":");
        Serial.print(MDNS.port(i));
        Serial.println(")");
      }
      // Add to our receiver list
      String hostname = MDNS.hostname(i);
      int dotIndex = hostname.indexOf('.');
      String instanceName = (dotIndex > 0) ? hostname.substring(0, dotIndex) : hostname;
      addOrUpdateReceiver(instanceName.c_str(), MDNS.IP(i), MDNS.port(i));
      foundReceivers.push_back({MDNS.IP(i), MDNS.port(i)});
    }
  }
  // Remove receivers not found in this discovery
  auto it = receivers.begin();
  while (it != receivers.end()) {
    bool stillPresent = false;
    for (const auto& found : foundReceivers) {
      if (it->ip == found.first && it->port == found.second) {
        stillPresent = true;
        break;
      }
    }
    if (!stillPresent) {
      if(verbose) Serial.printf("Removing unsubscribed receiver: %s (%s:%d)\n", it->name.c_str(), it->ip.toString().c_str(), it->port);
      it = receivers.erase(it);
    } else {
      ++it;
    }
  }
  if(verbose) Serial.println();
}

void OscSenderManager::sendIntToAll(const char* address, int32_t value) {
  for (const auto& receiver : receivers) {
    osc.setDestination(receiver.ip, receiver.port);
    osc.sendInt(address, value);
    if(verbose) Serial.printf("Sent to %s (%s:%d): %s %d\n",  receiver.name.c_str(), receiver.ip.toString().c_str(), receiver.port, address, value);
  }
}

void OscSenderManager::sendIntToAll(int32_t value) {
  sendIntToAll("/value", value);
}

void OscSenderManager::sendMidiToAll(const char* address, unsigned char* midi) {
  for (const auto& receiver : receivers) {
    osc.setDestination(receiver.ip, receiver.port);
    osc.sendMidi(address, midi);
    if(verbose) Serial.printf("Sent MIDI to %s (%s:%d): %s [%02X %02X %02X %02X]\n", receiver.name.c_str(), receiver.ip.toString().c_str(), receiver.port, address, midi[0], midi[1], midi[2], midi[3]);
  }
}

void OscSenderManager::sendIntListToAll(const char* address, const std::vector<int32_t>& values) {
  if (values.empty()) return;
  
  // Create format string with 'i' for each integer
  String format = "";
  for (size_t i = 0; i < values.size(); i++) {
    format += "i";
  }
  
  for (const auto& receiver : receivers) {
    osc.setDestination(receiver.ip, receiver.port);
    osc.beginMessage();
    osc.writeAddress(address);
    for (const auto& i : values) {
      osc.writeFormat("i");
      osc.writeInt(i);
    }  
    osc.endMessage();   
    
    if(verbose) {
      Serial.printf("Sent int list to %s (%s:%d): %s [%d values]\n",
                 receiver.name.c_str(),
                 receiver.ip.toString().c_str(),
                 receiver.port,
                 address,
                 values.size());
    }
      
  }
}

void OscSenderManager::sendIntArrayToAll(const char* address, const int* values, size_t count){
    for (const auto& receiver : receivers) {
    osc.setDestination(receiver.ip, receiver.port);
    osc.beginMessage();
    osc.writeAddress(address);
    // Build complete format string first
    String format = "";
    for (size_t i = 0; i < count; i++) {
      format += "i";
    }
    osc.writeFormat(format.c_str());
    
    // Then write all integer values
    for (size_t i = 0; i < count; i++) {
      osc.writeInt(values[i]);
    }
    osc.endMessage();
  }
  if (verbose) Serial.printf("Sent int array to %d receivers: %s [%d values]\n", receivers.size(), address, count);
}

void OscSenderManager::sendFloatArrayToAll(const char* address, const float* values, size_t count) {
    
  for (const auto& receiver : receivers) {
    osc.setDestination(receiver.ip, receiver.port);
    osc.beginMessage();
    osc.writeAddress(address);
    // Build complete format string first
    String format = "";
    for (size_t i = 0; i < count; i++) {
      format += "f";
    }
    osc.writeFormat(format.c_str());
    
    // Then write all float values
    for (size_t i = 0; i < count; i++) {
      osc.writeFloat(values[i]);
    }
    osc.endMessage();
  }
}

size_t OscSenderManager::getReceiverCount() const {
  return receivers.size();
}

void OscSenderManager::printReceivers() const {
  if (verbose) {
    Serial.printf("Discovered OSC receivers: %d\n", receivers.size());
    for (size_t i = 0; i < receivers.size(); i++) {
      Serial.printf("  %d: %s (%s:%d) - Last seen: %lu ms ago\n", 
                   i+1, 
                   receivers[i].name.c_str(), 
                   receivers[i].ip.toString().c_str(), 
                   receivers[i].port,
                   millis() - receivers[i].lastSeen);
    }
  }
}

void OscSenderManager::cleanupOldReceivers() {
  unsigned long currentTime = millis();
  auto it = receivers.begin();
  while (it != receivers.end()) {
    if (currentTime - it->lastSeen > 30000) { // 30 seconds
      if (verbose) Serial.printf("Removing old receiver: %s\n", it->name.c_str());
      it = receivers.erase(it);
    } else {
      ++it;
    }
  }
}

void OscSenderManager::addOrUpdateReceiver(const char* name, IPAddress ip, uint16_t port) {
  // Check if receiver already exists
  for (auto& receiver : receivers) {
    if (receiver.ip == ip && receiver.port == port) {
      receiver.lastSeen = millis();
      return;
    }
  }
  
  // Add new receiver
  OscReceiver newReceiver = {ip, port, String(name), millis()};
  receivers.push_back(newReceiver);
  if (verbose) Serial.printf("Added OSC receiver: %s (%s:%d)\n", name, ip.toString().c_str(), port);
}
