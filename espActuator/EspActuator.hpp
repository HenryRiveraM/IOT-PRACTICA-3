// EspActuator.hpp
#pragma once
#include "Mqtt.hpp"
#include "ServoController.hpp"
#include <ArduinoJson.h>

class EspActuator {
private:
    // Controls the physical servo that opens/closes the interior door
    ServoController*  servoController;

    // MQTT client wrapper used to communicate with AWS IoT Core
    MqttClient*       mqtt;

    // Wi-Fi / TLS configuration and handler
    NetworkConfig*    networkConfig;
    NetworkHandler*   net;

    // MQTT connection configuration (endpoint, clientId, callback, port)
    MqttConfig*       mqttConfig;

    // Static instance pointer used by the static MQTT callback
    static EspActuator* instance;

    // Topics used to publish reported state and subscribe to delta/desired updates
    const char* publishTopic;
    const char* subscribeTopic;

    /**
     * Static MQTT callback required by PubSubClient.
     * Delegates the handling of the message to the singleton instance.
     */
    static void mqttCallback(char* topic, uint8_t* payload, unsigned int length) {
        if (instance) instance->handleMessage(topic, payload, length);
    }

    /**
     * Process messages coming from AWS IoT (shadow updates / deltas).
     *  - Logs the raw JSON payload.
     *  - Parses the JSON and extracts the requested door state.
     *  - Moves the servo accordingly.
     *  - Publishes a "reported" state back to the device shadow.
     */
    void handleMessage(char* topic, uint8_t* payload, unsigned int length) {
        Serial.print("Mensaje recibido [");
        Serial.print(topic);
        Serial.print("]: ");

        // Dump raw JSON payload for debugging
        String msg;
        for (unsigned int i = 0; i < length; i++) msg += (char)payload[i];
        Serial.println(msg);

        StaticJsonDocument<512> doc;
        // IMPORTANT: use the provided length when deserializing
        DeserializationError error = deserializeJson(doc, payload, length);
        if (error) {
            Serial.print("Error parseando JSON: ");
            Serial.println(error.c_str());
            return;
        }

        // AWS IoT Shadow may send:
        //  - delta:   { "state": { "interiorDoor": "OPEN" }, ... }
        //  - desired: { "state": { "desired": { "interiorDoor": "OPEN" } } }
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

        // Drive the servo according to the requested state
        if (strcmp(doorState, "OPEN") == 0) {
            servoController->open();
        } else if (strcmp(doorState, "CLOSE") == 0) {
            servoController->close();
        }

        // Build and publish the REPORTED state back to the shadow
        StaticJsonDocument<256> responseDoc;
        responseDoc["state"]["reported"]["interiorDoor"] = doorState;

        String out;
        serializeJson(responseDoc, out);
        mqtt->publish(publishTopic, out.c_str());

        Serial.print("Shadow report (interiorDoor): ");
        Serial.println(out);
    }

public:
    /**
     * Constructor wires together:
     *  - Wi-Fi/TLS configuration
     *  - MQTT client configuration and callback
     *  - Servo controller
     *  - Topics used for shadow update / delta
     */
    EspActuator(byte actuatorPin,
                const char* ssid,
                const char* password,
                const char* server,
                int port,
                const char* publishTopic,
                const char* subscribeTopic,
                const char* clientId) {

        // Set singleton instance so the static callback can delegate here
        instance             = this;
        this->publishTopic   = publishTopic;
        this->subscribeTopic = subscribeTopic;

        networkConfig   = new NetworkConfig(ssid, password);
        net             = new NetworkHandler(networkConfig);
        servoController = new ServoController(actuatorPin);
        mqttConfig      = new MqttConfig(server, clientId, &mqttCallback, port);
        mqtt            = new MqttClient(mqttConfig, net);
    }

    /**
     * Initializes serial logging, servo, Wi-Fi connection and MQTT subscription.
     */
    void setup() {
        Serial.begin(115200);
        servoController->begin();   // move servo to initial position
        mqtt->initialize();
        mqtt->subscribe(subscribeTopic);
    }

    /**
     * Main loop:
     *  - Ensures MQTT connection is alive (reconnects if needed).
     *  - Processes incoming MQTT messages.
     */
    void loop() {
        if (!mqtt->connected()) {
            mqtt->reconnect();
            mqtt->subscribe(subscribeTopic);
        }
        mqtt->loop();
    }
};

// Static member initialization
EspActuator* EspActuator::instance = nullptr;