#pragma once
#include <WiFiClientSecure.h>
#include <WiFi.h>
#include "certificates.h"

//=============================================================================
// NetworkConfig
// -----------------------------------------------------------------------------
// Handles secure credential configuration required to establish a TLS connection
// with AWS IoT Core. This class encapsulates:
//
//   • WiFi SSID + password
//   • AWS IoT Root CA certificate
//   • Device private key
//   • Device certificate
//
// It produces a fully configured WiFiClientSecure instance used by MQTT.
//=============================================================================
class NetworkConfig {
    public:
        const char* ssid;                 // WiFi network name
        const char* password;             // WiFi password
        WiFiClientSecure* client;         // TLS-enabled network client

        //-------------------------------------------------------------------------
        // Constructor: loads certificates from compile-time constants.
        //-------------------------------------------------------------------------
        NetworkConfig(const char* ssid, const char* password)
            : ssid(ssid), password(password) {

            client = new WiFiClientSecure();
            client->setCACert(AWS_ROOT_CA_CERTIFICATE);      // AWS Root CA
            client->setPrivateKey(AWS_PRIVATE_KEY);          // Device private key
            client->setCertificate(AWS_CLIENT_CERTIFICATE);  // Device certificate
        }

        //-------------------------------------------------------------------------
        // Constructor overload: accepts certificates dynamically (optional use case).
        //-------------------------------------------------------------------------
        NetworkConfig(const char* ssid,
                      const char* password,
                      const char* root_ca,
                      const char* private_key,
                      const char* client_cert)
            : ssid(ssid), password(password) {

            client = new WiFiClientSecure();
            client->setCACert(root_ca);
            client->setPrivateKey(private_key);
            client->setCertificate(client_cert);
        }

        // Cleanup allocated secure client
        ~NetworkConfig() {
            delete client;
        }
};

//=============================================================================
// NetworkHandler
// -----------------------------------------------------------------------------
// Manages WiFi connectivity for the ESP32, including:
//
//   ✔ Initialization of WiFi parameters
//   ✔ Connection to the access point
//   ✔ Auto-reconnect logic
//   ✔ Providing the secure client for MQTT
//
// Designed to maintain a stable connection required by AWS IoT Core.
//=============================================================================
class NetworkHandler {
    public:
        NetworkConfig* config;     // Pointer to certificate + WiFi settings

        //-------------------------------------------------------------------------
        // Constructor: injects configuration dependency.
        //-------------------------------------------------------------------------
        NetworkHandler(NetworkConfig* config) : config(config) {}

        //-------------------------------------------------------------------------
        // connect()
        // Establishes WiFi connection. Loops until the ESP32 successfully joins
        // the access point. Prints connection attempts for debugging.
        //-------------------------------------------------------------------------
        void connect() {
            WiFi.begin(config->ssid, config->password);

            while (WiFi.status() != WL_CONNECTED) {
                delay(1000);
                Serial.println("Connecting to WiFi...");
            }

            Serial.println("Connected to WiFi");
        }

        //-------------------------------------------------------------------------
        // getClient()
        // Returns the TLS-secured WiFiClientSecure instance used by MQTT.
        //-------------------------------------------------------------------------
        WiFiClientSecure* getClient() {
            return config->client;
        }

        //-------------------------------------------------------------------------
        // reconnect()
        // Ensures WiFi stays connected. If a disconnection occurs, this method
        // restarts the connection process and blocks until reconnection is restored.
        //-------------------------------------------------------------------------
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

        //-------------------------------------------------------------------------
        // initialize()
        // Configures WiFi in station mode, disables sleep (improves MQTT stability),
        // and enables automatic reconnection at the hardware level.
        //-------------------------------------------------------------------------
        void initialize() {
            WiFi.mode(WIFI_STA);            // Client mode
            WiFi.setSleep(false);           // Prevent WiFi power saving
            WiFi.setAutoReconnect(true);    // Automatic reconnect
            WiFi.persistent(true);          // Save WiFi config to flash
        }
};