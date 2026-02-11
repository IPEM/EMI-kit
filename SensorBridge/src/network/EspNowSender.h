#ifndef ESPNOW_SENDER_H
#define ESPNOW_SENDER_H

#ifdef TRANSPORT_ESPNOW

#include <Arduino.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <WiFi.h>
#include <espnow_protocol.h>

class EspNowSender {
public:
    bool begin(uint8_t channel = ESPNOW_CHANNEL);
    void sendMidi(uint8_t command_and_channel, uint8_t param1, uint8_t param2);

private:
    static void onSendCallback(const uint8_t *mac_addr, esp_now_send_status_t status);
    static uint8_t broadcastAddress[6];
};

#endif // TRANSPORT_ESPNOW
#endif // ESPNOW_SENDER_H
