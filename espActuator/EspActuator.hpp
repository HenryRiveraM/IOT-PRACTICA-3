// EspActuator.hpp
#pragma once
#include "Mqtt.hpp"
#include "ServoController.hpp"
#include <ArduinoJson.h>

class EspActuator {
private:
    ServoController*  servoController;
    MqttClient*       mqtt;
    NetworkConfig*    networkConfig;
    NetworkHandler*   net;
    MqttConfig*       mqttConfig;
    static EspActuator* instance;
    const char* publishTopic;
    const char* subscribeTopic;

    // Callback estático de MQTT
    static void mqttCallback(char* topic, uint8_t* payload, unsigned int length) {
        if (instance) instance->handleMessage(topic, payload, length);
    }

    // Manejo del mensaje que llega de AWS IoT
    void handleMessage(char* topic, uint8_t* payload, unsigned int length) {
        Serial.print("Mensaje recibido [");
        Serial.print(topic);
        Serial.print("]: ");

        // Mostrar el JSON recibido
        String msg;
        for (unsigned int i = 0; i < length; i++) msg += (char)payload[i];
        Serial.println(msg);

        StaticJsonDocument<512> doc;
        DeserializationError error = deserializeJson(doc, payload, length); // usa length
        if (error) {
            Serial.print("Error parseando JSON: ");
            Serial.println(error.c_str());
            return;
        }

        // AWS IoT Shadow puede mandar:
        // delta:  { "state": { "interiorDoor": "OPEN" }, ... }
        // o a veces: { "state": { "desired": { "interiorDoor": "OPEN" } } }
        const char* doorState = nullptr;

        if (doc["state"]["interiorDoor"]) {
            doorState = doc["state"]["interiorDoor"];
        } else if (doc["state"]["desired"]["interiorDoor"]) {
            doorState = doc["state"]["desired"]["interiorDoor"];
        }

        if (!doorState) {
            Serial.println("No viene interiorDoor en el JSON");
            return;
        }

        Serial.print("Nuevo estado interiorDoor = ");
        Serial.println(doorState);

        // Mover el servo
        if (strcmp(doorState, "OPEN") == 0) {
            servoController->open();
        } else if (strcmp(doorState, "CLOSE") == 0) {
            servoController->close();
        }

        // Actualizar REPORTED en el shadow
        StaticJsonDocument<256> responseDoc;
        responseDoc["state"]["reported"]["interiorDoor"] = doorState;

        String out;
        serializeJson(responseDoc, out);
        mqtt->publish(publishTopic, out.c_str());

        Serial.print("Shadow report (interiorDoor): ");
        Serial.println(out);
    }

public:
    EspActuator(byte actuatorPin,
                const char* ssid,
                const char* password,
                const char* server,
                int port,
                const char* publishTopic,
                const char* subscribeTopic,
                const char* clientId) {

        instance             = this;
        this->publishTopic   = publishTopic;
        this->subscribeTopic = subscribeTopic;

        networkConfig   = new NetworkConfig(ssid, password);
        net             = new NetworkHandler(networkConfig);
        servoController = new ServoController(actuatorPin);
        mqttConfig      = new MqttConfig(server, clientId, &mqttCallback, port);
        mqtt            = new MqttClient(mqttConfig, net);
    }

    void setup() {
        Serial.begin(115200);
        servoController->begin();   // mueve el servo a posición inicial
        mqtt->initialize();
        mqtt->subscribe(subscribeTopic);
    }

    void loop() {
        if (!mqtt->connected()) {
            mqtt->reconnect();
            mqtt->subscribe(subscribeTopic);
        }
        mqtt->loop();
    }
};

EspActuator* EspActuator::instance = nullptr;