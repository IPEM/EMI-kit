#include "ConfigManager.h"
#include <Preferences.h>

const char* ConfigManager::NAMESPACE = "device_config_";
const char* ConfigManager::WIFI_SSID_KEY = "wifi_ssid";
const char* ConfigManager::WIFI_PASSWORD_KEY = "wifi_pass";

static Preferences preferences;

ConfigManager::ConfigManager() {
}

void ConfigManager::begin() {
    preferences.begin(NAMESPACE, false);
    Serial.println("ConfigManager initialized");
}

void ConfigManager::loadConfig() {
    // This method is now private and called by getConfig()
}

DeviceConfig ConfigManager::getConfig() {
    DeviceConfig config;
    
    // Load configuration from preferences
    config.wifi_ssid = preferences.getString(WIFI_SSID_KEY, "");
    config.wifi_password = preferences.getString(WIFI_PASSWORD_KEY, "");
    
    return config;
}

void ConfigManager::setConfig(const DeviceConfig& config) {
    saveConfig(config);
    Serial.println("Configuration saved");
}

void ConfigManager::saveConfig(const DeviceConfig& config) {
    // Save WiFi config to preferences
    preferences.putString(WIFI_SSID_KEY, config.wifi_ssid);
    preferences.putString(WIFI_PASSWORD_KEY, config.wifi_password);
}

void ConfigManager::clearConfig() {
    preferences.remove(WIFI_SSID_KEY);
    preferences.remove(WIFI_PASSWORD_KEY);
    Serial.println("Configuration cleared");
}

bool ConfigManager::hasValidConfig() {
    DeviceConfig config = getConfig();
    return config.hasValidWiFiConfig();
}

void ConfigManager::printConfig() {
    DeviceConfig config = getConfig();
    
    Serial.println("=== Device Configuration ===");
    Serial.println("WiFi SSID: " + config.wifi_ssid);
    String passwordStatus = (config.wifi_password.length() > 0) ? "***" : "(not set)";
    Serial.println("WiFi Password: " + passwordStatus);
    Serial.println("Valid Config: " + String(config.hasValidWiFiConfig() ? "Yes" : "No"));
    Serial.println("============================");
}
