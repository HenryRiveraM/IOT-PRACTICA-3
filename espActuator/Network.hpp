#pragma once
#include <WiFiClientSecure.h>
#include <WiFi.h>
#include "certificates.h"

//=====================================================
// NetworkConfig
// ----------------------------------------------------
// This class handles Wi-Fi credentials and prepares a
// TLS-secured WiFiClientSecure instance loaded with the
// certificates required to authenticate with AWS IoT.
//=====================================================
class NetworkConfig {
    public:
        const char* ssid;       // Wi-Fi network SSID
        const char* password;   // Wi-Fi network password
        WiFiClientSecure* client; // TLS-secured network client

        // Constructor using certificates defined in certificates.h
        NetworkConfig(const char* ssid, const char* password)
            : ssid(ssid), password(password) 
        {
            client = new WiFiClientSecure();

            // Load the AWS IoT Root CA certificate
            client->setCACert(AWS_ROOT_CA_CERTIFICATE);

            // Load the device private key
            client->setPrivateKey(AWS_PRIVATE_KEY);

            // Load the device certificate
            client->setCertificate(AWS_CLIENT_CERTIFICATE);
        }

        // Constructor allowing custom certificate inputs
        NetworkConfig(const char* ssid, const char* password,
                      const char* root_ca, const char* private_key, const char* client_cert)
            : ssid(ssid), password(password)
        {
            client = new WiFiClientSecure();
            client->setCACert(root_ca);
            client->setPrivateKey(private_key);
            client->setCertificate(client_cert);
        }

        // Destructor cleans the secure client instance
        ~NetworkConfig() {
            delete client;
        }
};

//=====================================================
// NetworkHandler
// ----------------------------------------------------
// Manages Wi-Fi connectivity behavior, including:
// - Wi-Fi initialization
// - Auto-reconnect handling
// - Returning the TLS client for MQTT operations
//=====================================================
class NetworkHandler {
    public:
        NetworkConfig* config;  // Reference to network configuration

        // Bind handler to config object
        NetworkHandler(NetworkConfig* config) : config(config) {}

        //-----------------------------------------------------
        // Connects to Wi-Fi and blocks until connection succeeds
        //-----------------------------------------------------
        void connect() {
            WiFi.begin(config->ssid, config->password);

            while (WiFi.status() != WL_CONNECTED) {
                delay(1000);
                Serial.println("Connecting to WiFi...");
            }

            Serial.println("Connected to WiFi");
        }

        //-----------------------------------------------------
        // Returns the secure WiFiClient used by MQTT/TLS
        //-----------------------------------------------------
        WiFiClientSecure* getClient() {
            return config->client;
        }

        //-----------------------------------------------------
        // Ensures Wi-Fi reconnection if connection is lost
        //-----------------------------------------------------
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

        //-----------------------------------------------------
        // Initial Wi-Fi setup for stable IoT operation:
        // - Station mode
        // - Disable sleep (avoids MQTT disconnects)
        // - Auto-reconnect enabled
        //-----------------------------------------------------
        void initialize() {
            WiFi.mode(WIFI_STA);        // Use station mode only
            WiFi.setSleep(false);       // Avoid sleep to prevent TLS drops
            WiFi.setAutoReconnect(true);// Enable automatic reconnect
            WiFi.persistent(true);      // Save Wi-Fi config to flash
        }
};