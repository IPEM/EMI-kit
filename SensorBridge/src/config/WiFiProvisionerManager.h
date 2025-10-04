#ifndef WIFI_PROVISIONER_MANAGER_H
#define WIFI_PROVISIONER_MANAGER_H

#include <Arduino.h>
#include <functional>
#include "DeviceConfig.h"

typedef std::function<void(const DeviceConfig& config)> ProvisioningCallback;

class WiFiProvisionerManager {
public:
    WiFiProvisionerManager();
    
    void begin();
    void startProvisioning(ProvisioningCallback callback = nullptr);
    bool isProvisioningActive();
    void handleProvisioning(); // Call this in loop if provisioning is active
    
private:
    bool provisioningActive;
    ProvisioningCallback onComplete;
    
    void onProvisioningSuccess(const char* ssid, const char* password, const char* input);
    void onProvisioningError(const char* error);
};
#endif