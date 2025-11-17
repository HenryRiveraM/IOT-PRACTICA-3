#pragma once
#include <PubSubClient.h>
#include "Network.hpp"

//=============================================================================
// MqttConfig
// -----------------------------------------------------------------------------
// Holds all MQTT configuration parameters required to establish a connection
// with AWS IoT (or any MQTT broker). This includes server address, port,
// client ID, and the callback function that will handle incoming messages.
//=============================================================================
class MqttConfig {
    public:
        const char* server;                              // MQTT broker endpoint
        const char* clientId;                            // Unique client ID
        void (*callback)(char*, uint8_t*, unsigned int); // Incoming message handler
        int port;                                        // MQTT/TLS port (8883 for AWS)

        MqttConfig(const char* server,
                   const char* clientId,
                   void (*callback)(char*, uint8_t*, unsigned int),
                   int port)
            : server(server), clientId(clientId), callback(callback), port(port) {}
};

//=============================================================================
// MqttClient
// -----------------------------------------------------------------------------
// Wrapper class around PubSubClient that integrates:
//
//  ✔ TLS-secured WiFi connection (via NetworkHandler)
//  ✔ MQTT connection management (connect, reconnect, subscribe, publish)
//  ✔ Automatic reconnection if WiFi or MQTT drops
//
// This class ensures that the ESP32 maintains a stable connection to AWS IoT
// and sends/receives messages reliably.
//=============================================================================
class MqttClient {
    public:
        MqttConfig* config;           // Holds MQTT settings
        PubSubClient* client;         // Underlying MQTT client
        NetworkHandler* networkHandler;  // Manages WiFi/TLS network layer

        //-------------------------------------------------------------------------
        // Constructor: builds a PubSubClient based on the WiFi secure client
        // provided by NetworkHandler, then sets server and callback.
        //-------------------------------------------------------------------------
        MqttClient(MqttConfig* config, NetworkHandler* networkHandler)
            : config(config), networkHandler(networkHandler) {
            
            client = new PubSubClient(*networkHandler->getClient());
            client->setServer(config->server, config->port);
            client->setCallback(config->callback);
        }

        // Default constructor for optional delayed initialization
        MqttClient() {}

        //-------------------------------------------------------------------------
        // setAll(): allows late injection of config + network handler
        // Useful for cases where objects must be created before full configuration.
        //-------------------------------------------------------------------------
        void setAll(MqttConfig* config, NetworkHandler* networkHandler) {
            this->config = config;
            this->networkHandler = networkHandler;

            client = new PubSubClient(*networkHandler->getClient());
            client->setServer(config->server, config->port);
            client->setCallback(config->callback);
        }

        // Destructor cleans up allocated PubSubClient
        ~MqttClient() {
            delete client;
        }

        //-------------------------------------------------------------------------
        // initialize()
        // Initializes WiFi/TLS connection, ensures device is online,
        // and attempts the first MQTT connection.
        //-------------------------------------------------------------------------
        void initialize() {
            networkHandler->initialize();
            networkHandler->connect();
            reconnect();
        }

        //-------------------------------------------------------------------------
        // reconnect()
        // Tries to establish MQTT connection if it's not active.
        // Includes retry logging and error reporting for debugging.
        //-------------------------------------------------------------------------
        void reconnect() {
            if (!client->connected()) {
                Serial.print("Attempting MQTT connection...");

                if (client->connect(config->clientId)) {
                    Serial.println("connected");
                } else {
                    Serial.print("failed, rc=");
                    Serial.print(client->state());
                    Serial.println(" try again in 5 seconds");
                    delay(5000);
                }
            }
        }

        //-------------------------------------------------------------------------
        // connected()
        // Returns true if the MQTT client is currently connected.
        //-------------------------------------------------------------------------
        bool connected() {
            return client->connected();
        }

        //-------------------------------------------------------------------------
        // publish()
        // Sends a payload to a specific topic. If MQTT is disconnected,
        // the method tries to recover before retrying.
        //-------------------------------------------------------------------------
        void publish(const char* topic, const char* payload) {
            if (client->connected()) {
                client->publish(topic, payload);
            } else {
                Serial.println("MQTT client not connected. Reconnecting...");
                reconnect();
            }
        }

        //-------------------------------------------------------------------------
        // subscribe()
        // Subscribes to a topic. Handles reconnection if needed.
        //-------------------------------------------------------------------------
        void subscribe(const char* topic) {
            if (client->connected()) {
                client->subscribe(topic);
            } else {
                Serial.println("MQTT client not connected. Reconnecting...");
                reconnect();
            }
        }

        //-------------------------------------------------------------------------
        // loop()
        // Processes incoming MQTT messages and keeps TCP connection alive.
        // Called continuously in the ESP32 main loop().
        //-------------------------------------------------------------------------
        void loop() {
            if (client->connected()) {
                client->loop();
            } else {
                Serial.println("MQTT client not connected. Reconnecting...");
                reconnect();
            }
        }
};