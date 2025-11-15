// EspSensor.hpp
#pragma once
#include "Mqtt.hpp"
#include "MagneticSensor.hpp"
#include <ArduinoJson.h>

class EspSensor {
private:
    MagneticSensor* doorSensor;
    MqttClient*     mqtt;
    NetworkConfig*  networkConfig;
    NetworkHandler* net;
    MqttConfig*     mqttConfig;
    static EspSensor* instance;
    const char*     publishTopic;
    const char*     subscribeTopic;

public:
    EspSensor(int sensorPin,
              const char* ssid,
              const char* password,
              const char* server,
              int port,
              const char* clientId,
              const char* publishTopic,
              const char* subscribeTopic) {

        instance            = this;
        this->publishTopic   = publishTopic;
        this->subscribeTopic = subscribeTopic;

        networkConfig = new NetworkConfig(ssid, password);
        net           = new NetworkHandler(networkConfig);
        doorSensor    = new MagneticSensor(sensorPin);

        mqttConfig = new MqttConfig(
            server,
            clientId,
            [](char* topic, uint8_t* payload, unsigned int length) {
                Serial.print("Mensaje recibido [");
                Serial.print(topic);
                Serial.print("]: ");
                for (unsigned int i = 0; i < length; i++) Serial.print((char)payload[i]);
                Serial.println();
            },
            port
        );

        mqtt = new MqttClient(mqttConfig, net);
    }

    void setup() {
        Serial.begin(115200);
        doorSensor->begin();
        mqtt->initialize();
        // si quisieras escuchar algo, podrías usar: mqtt->subscribe(subscribeTopic);
    }

    void loop() {
        mqtt->loop();

        if (doorSensor->hasStateChanged()) {
            bool isOpen = doorSensor->getLastState();
            Serial.print("Cambio en puerta exterior -> ");
            Serial.println(isOpen ? "OPEN" : "CLOSE");

            // AQUÍ debe ir REPORTED porque es el estado REAL del sensor
            StaticJsonDocument<256> doc;
            doc["state"]["reported"]["exteriorDoor"] = isOpen ? "OPEN" : "CLOSE";

            String out;
            serializeJson(doc, out);
            mqtt->publish(publishTopic, out.c_str());

            Serial.print("Shadow report (exteriorDoor): ");
            Serial.println(out);
        }
    }
};
EspSensor* EspSensor::instance = nullptr;