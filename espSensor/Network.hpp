#pragma once
#include <WiFiClientSecure.h>
#include <WiFi.h>
#include "certificates.h"

class NetworkConfig{
    public:
        const char* ssid;
        const char* password;
        WiFiClientSecure* client;
        NetworkConfig(const char* ssid, const char* password) : ssid(ssid), password(password) {
            client = new WiFiClientSecure();
            client->setCACert(AWS_ROOT_CA_CERTIFICATE);
            client->setPrivateKey(AWS_PRIVATE_KEY);
            client->setCertificate(AWS_CLIENT_CERTIFICATE);
        }
        NetworkConfig(const char* ssid, const char* password, const char* root_ca, const char* private_key, const char* client_cert) : ssid(ssid), password(password) {
            client = new WiFiClientSecure();
            client->setCACert(root_ca);
            client->setPrivateKey(private_key);
            client->setCertificate(client_cert);
        }
        ~NetworkConfig() {
            delete client;
        }
};

class NetworkHandler {
    public:
        NetworkConfig* config;
        NetworkHandler(NetworkConfig* config) : config(config) {}
        void connect() {
            WiFi.begin(config->ssid, config->password);
            while (WiFi.status() != WL_CONNECTED) {
                delay(1000);
                Serial.println("Connecting to WiFi...");
            }
            Serial.println("Connected to WiFi");
        }
        WiFiClientSecure* getClient() {
            return config->client;
        }
        void reconnect() {
            if (WiFi.status() != WL_CONNECTED) {
                WiFi.disconnect();
                WiFi.begin(config->ssid, config->password);
                while (WiFi.status() != WL_CONNECTED) {
                    delay(1000);
                    Serial.println("Reconnecting to WiFi...");
                }
                Serial.println("Reconnected to WiFi");
            }
        }
        void initialize() {
            WiFi.mode(WIFI_STA);
            WiFi.setSleep(false);
            WiFi.setAutoReconnect(true);
            WiFi.persistent(true);
        }
};