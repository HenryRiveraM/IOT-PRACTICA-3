#pragma once
#include <PubSubClient.h>
#include "Network.hpp"

class MqttConfig {
    public:
        const char* server;
        const char* clientId;
        void (*callback)(char*, uint8_t*, unsigned int);
        int port;
        MqttConfig(const char* server, const char* clientId, void (*callback)(char*, uint8_t*, unsigned int), int port) : server(server), clientId(clientId), callback(callback), port(port) {}
};

class MqttClient{
    public:
        MqttConfig* config;
        PubSubClient* client;
        NetworkHandler* networkHandler;
        MqttClient(MqttConfig* config, NetworkHandler* networkHandler) : config(config), networkHandler(networkHandler) {
            client = new PubSubClient(*networkHandler->getClient());
            client->setServer(config->server, config->port);
            client->setCallback(config->callback);
        }
        MqttClient() {}
        void setAll(MqttConfig* config, NetworkHandler* networkHandler) {
            this->config = config;
            this->networkHandler = networkHandler;
            client = new PubSubClient(*networkHandler->getClient());
            client->setServer(config->server, config->port);
            client->setCallback(config->callback);
        }
        ~MqttClient() {
            delete client;
        }
        void initialize() {
            networkHandler->initialize();
            networkHandler->connect();
            reconnect();
        }
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
        bool connected() {
            return client->connected();
        }
        void publish(const char* topic, const char* payload) {
            if (client->connected()) {
                client->publish(topic, payload);
            } else {
                Serial.println("MQTT client not connected. Reconnecting...");
                reconnect();
            }
        }
        void subscribe(const char* topic) {
            if (client->connected()) {
                client->subscribe(topic);
            } else {
                Serial.println("MQTT client not connected. Reconnecting...");
                reconnect();
            }
        }
        void loop() {
            if (client->connected()) {
                client->loop();
            } else {
                Serial.println("MQTT client not connected. Reconnecting...");
                reconnect();
            }
        }
};