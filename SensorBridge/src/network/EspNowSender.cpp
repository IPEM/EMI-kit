#ifdef TRANSPORT_ESPNOW

#include "EspNowSender.h"

uint8_t EspNowSender::broadcastAddress[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

void EspNowSender::onSendCallback(const uint8_t *mac_addr, esp_now_send_status_t status) {
    if (status != ESP_NOW_SEND_SUCCESS) {
        Serial.println("ESP-NOW send failed");
    }
}

bool EspNowSender::begin(uint8_t channel) {
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();

    // Set WiFi channel
    esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);

    if (esp_now_init() != ESP_OK) {
        Serial.println("ESP-NOW init failed");
        return false;
    }

    esp_now_register_send_cb(onSendCallback);

    // Add broadcast peer
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, broadcastAddress, 6);
    peerInfo.channel = 0; // use current channel
    peerInfo.encrypt = false;

    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        Serial.println("ESP-NOW add broadcast peer failed");
        return false;
    }

    Serial.printf("ESP-NOW sender initialized on channel %d\n", channel);
    return true;
}

void EspNowSender::sendMidi(uint8_t command_and_channel, uint8_t param1, uint8_t param2) {
    EspNowMidiMessage msg;
    msg.type = ESPNOW_MSG_MIDI;
    msg.command_and_channel = command_and_channel;
    msg.parameter1 = param1;
    msg.parameter2 = param2;

    esp_now_send(broadcastAddress, (uint8_t *)&msg, sizeof(msg));
}

#endif // TRANSPORT_ESPNOW
