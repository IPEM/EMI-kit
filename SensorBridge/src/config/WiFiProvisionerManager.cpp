#include "WiFiProvisionerManager.h"
#include <WiFiProvisioner.h>

static WiFiProvisioner* provisioner = nullptr;

WiFiProvisionerManager::WiFiProvisionerManager() 
    : provisioningActive(false) {
}

void WiFiProvisionerManager::begin() {
    // Nothing specific to initialize here
}

void WiFiProvisionerManager::startProvisioning(ProvisioningCallback callback) {
    if (provisioningActive) {
        Serial.println("Provisioning already active");
        return;
    }
    
    onComplete = callback;
    provisioningActive = true;
    
    Serial.println("Starting WiFi provisioning...");
    
    // Create provisioner instance
    if (provisioner != nullptr) {
        delete provisioner;
    }
    provisioner = new WiFiProvisioner();
    
    // Configure provisioner
    provisioner->getConfig().SHOW_INPUT_FIELD = false;
    provisioner->getConfig().SHOW_RESET_FIELD = false;
    
    // Set callbacks using lambda functions that capture 'this'
    provisioner->onSuccess([this](const char* ssid, const char* password, const char* input) {
        this->onProvisioningSuccess(ssid, password, input);
    });
    
    // Start provisioning
    provisioner->startProvisioning();
    Serial.println("WiFi provisioning started. Connect to the AP to configure.");
}

bool WiFiProvisionerManager::isProvisioningActive() {
    return provisioningActive;
}

void WiFiProvisionerManager::handleProvisioning() {
    // This method can be used for any provisioning loop handling if needed
    // Currently, the WiFiProvisioner library handles everything internally
}

void WiFiProvisionerManager::onProvisioningSuccess(const char* ssid, const char* password, const char* input) {
    Serial.printf("Provisioning successful! SSID: %s\n", ssid);
    
    // Create config with provisioned values
    DeviceConfig config;
    config.wifi_ssid = String(ssid);
    config.wifi_password = String(password ? password : "");
    
    provisioningActive = false;
    
    // Clean up provisioner
    if (provisioner != nullptr) {
        delete provisioner;
        provisioner = nullptr;
    }
    
    // Return configuration via callback
    if (onComplete) {
        onComplete(config);
    }
}

void WiFiProvisionerManager::onProvisioningError(const char* error) {
    Serial.printf("Provisioning error: %s\n", error);
    
    provisioningActive = false;
    
    // Clean up provisioner
    if (provisioner != nullptr) {
        delete provisioner;
        provisioner = nullptr;
    }
    
    // Return empty configuration on error
    if (onComplete) {
        DeviceConfig emptyConfig; // Empty config indicates failure
        onComplete(emptyConfig);
    }
}
