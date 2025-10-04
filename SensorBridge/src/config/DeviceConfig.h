#ifndef DEVICE_CONFIG_H
#define DEVICE_CONFIG_H

#include <Arduino.h>

struct DeviceConfig {
    // WiFi Configuration (active)
    String wifi_ssid;
    String wifi_password;
    
    // Future configuration values can be added here
    String device_name;
    // String device_id;
    // int sensor_update_interval;
    // bool led_enabled;
    // float threshold_tilt;
    // float threshold_rotation;
    
    // Default constructor
    DeviceConfig() {
        wifi_ssid = "";
        wifi_password = "";
        device_name = "SensorBridge01";
    }
    
    // Check if WiFi config is valid
    bool hasValidWiFiConfig() const {
        return wifi_ssid.length() > 0;
    }
    
    // Check if config is empty
    bool isEmpty() const {
        return wifi_ssid.length() == 0 && wifi_password.length() == 0;
    }
};

#endif
