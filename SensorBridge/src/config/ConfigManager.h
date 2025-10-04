#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Arduino.h>
#include "DeviceConfig.h"

class ConfigManager {
public:
    ConfigManager();
    void begin();
    
    // Configuration management
    DeviceConfig getConfig();
    void setConfig(const DeviceConfig& config);
    void clearConfig();
    bool hasValidConfig();
    
    // Debug helper
    void printConfig();

private:
    static const char* NAMESPACE;
    
    void loadConfig();
    void saveConfig(const DeviceConfig& config);
    
    // Key names for preferences
    static const char* WIFI_SSID_KEY;
    static const char* WIFI_PASSWORD_KEY;

    static const char* DEVICE_NAME_KEY;
};

#endif