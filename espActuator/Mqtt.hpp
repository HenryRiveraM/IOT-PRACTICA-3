#pragma once
#include <PubSubClient.h>
#include "Network.hpp"

//-----------------------------------------------
// MQTT Configuration
//-----------------------------------------------
// Holds server settings, client ID, callback,
// and port used by the MQTT client.
class MqttConfig {
    public:
        const char* server;   // MQTT broker endpoint (AWS IoT Core)
        const char* clientId; // Unique MQTT client ID
        void (*callback)(char*, uint8_t*, unsigned int); // Message callback handler
        int port;             // MQTT port (AWS uses 8883 TLS)

        MqttConfig(const char* server, const char* clientId,
                   void (*callback)(char*, uint8_t*, unsigned int),
                   int port)
            : server(server), clientId(clientId), callback(callback), port(port) {}
};

//-----------------------------------------------
// MQTT Client Wrapper
//-----------------------------------------------
// Encapsulates PubSubClient and manages:
// - MQTT initialization
// - MQTT reconnection logic
// - Subscriptions
// - Publishing messages
// - Processing MQTT loop
//-----------------------------------------------
class MqttClient {
    public:
        MqttConfig* config;            // Pointer to MQTT configuration
        PubSubClient* client;          // PubSubClient instance (MQTT engine)
        NetworkHandler* networkHandler;// Handles Wi-Fi/TLS connection

        // Constructor with config and network provider
        MqttClient(MqttConfig* config, NetworkHandler* networkHandler)
            : config(config), networkHandler(networkHandler) {
            // Bind PubSubClient to TLS-enabled network client
            client = new PubSubClient(*networkHandler->getClient());
            client->setServer(config->server, config->port);
            client->setCallback(config->callback);
        }

        // Default constructor (optional)
        MqttClient() {}

        // Allows setting objects after construction
        void setAll(MqttConfig* config, NetworkHandler* networkHandler) {
            this->config = config;
            this->networkHandler = networkHandler;
            client = new PubSubClient(*networkHandler->getClient());
            client->setServer(config->server, config->port);
            client->setCallback(config->callback);
        }

        // Destructor releases dynamically allocated MQTT client
        ~MqttClient() {
            delete client;
        }

        //-----------------------------------------------
        // Initializes Wi-Fi/TLS connection and MQTT
        //-----------------------------------------------
        void initialize() {
            networkHandler->initialize();  // Setup Wi-Fi + certificates
            networkHandler->connect();     // Connect to Wi-Fi
            reconnect();                   // Ensure MQTT connection
        }

        //-----------------------------------------------
        // Reconnects to the MQTT broker if disconnected
        //-----------------------------------------------
        void reconnect() {
            if (!client->connected()) {
                Serial.print("Attempting MQTT connection...");
                if (client->connect(config->clientId)) {
                    Serial.println("connected");
                } else {
                    Serial.print("failed, rc=");
                    Serial.print(client->state());
                    Serial.println(" — retrying in 5 seconds");
                    delay(5000);
                }
            }
        }

        //-----------------------------------------------
        // Returns true if MQTT is connected
        //-----------------------------------------------
        bool connected() {
            return client->connected();
        }

        //-----------------------------------------------
        // Publish MQTT message to topic
        //-----------------------------------------------
        void publish(const char* topic, const char* payload) {
            if (client->connected()) {
                client->publish(topic, payload);
            } else {
                Serial.println("MQTT client not connected. Reconnecting...");
                reconnect();
            }
        }

        //-----------------------------------------------
        // Subscribe to an MQTT topic
        //-----------------------------------------------
        void subscribe(const char* topic) {
            if (client->connected()) {
                client->subscribe(topic);
            } else {
                Serial.println("MQTT client not connected. Reconnecting...");
                reconnect();
            }
        }

        //-----------------------------------------------
        // Processes incoming MQTT messages and maintains connection
        //-----------------------------------------------
        void loop() {
            if (client->connected()) {
                client->loop();
            } else {
                Serial.println("MQTT client not connected. Reconnecting...");
                reconnect();
            }
        }
};