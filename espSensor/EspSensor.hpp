// EspSensor.hpp
#pragma once
#include "Mqtt.hpp"
#include "MagneticSensor.hpp"
#include <ArduinoJson.h>

//==========================================================================
// EspSensor
// -------------------------------------------------------------------------
// This class represents the ESP32 device responsible for *reading the
// exterior magnetic door sensor* and reporting its real physical state
// (OPEN / CLOSE) to AWS IoT Core via the Shadow "reported" attribute.
//
// Key responsibilities:
//  - Initialize WiFi + MQTT secure connection (TLS)
//  - Continuously monitor a reed switch sensor
//  - Detect state changes on the door (with debouncing behavior inside MagneticSensor)
//  - Publish state updates to the AWS IoT Device Shadow
//
// This device does NOT modify the desired state. It ONLY reports the real one.
//==========================================================================
class EspSensor {
private:
    MagneticSensor* doorSensor;   // Handles hardware state of the magnetic sensor
    MqttClient*     mqtt;         // MQTT wrapper for AWS IoT Core
    NetworkConfig*  networkConfig;
    NetworkHandler* net;
    MqttConfig*     mqttConfig;
    static EspSensor* instance;   // Allows static MQTT callback (if needed)
    const char*     publishTopic; // Topic used to publish Shadow "reported" states
    const char*     subscribeTopic;

public:

    //-------------------------------------------------------------------------
    // Constructor: prepares all network, MQTT and sensor objects
    //-------------------------------------------------------------------------
    EspSensor(int sensorPin,
              const char* ssid,
              const char* password,
              const char* server,
              int port,
              const char* clientId,
              const char* publishTopic,
              const char* subscribeTopic) {

        instance              = this;
        this->publishTopic    = publishTopic;    // Usually: $aws/things/<thing>/shadow/update
        this->subscribeTopic  = subscribeTopic;  // (Not used, but provided for completeness)

        // Setup secure WiFi config (certificates are loaded from NetworkConfig)
        networkConfig = new NetworkConfig(ssid, password);
        net           = new NetworkHandler(networkConfig);

        // Initialize the magnetic reed sensor interface
        doorSensor    = new MagneticSensor(sensorPin);

        // Setup MQTT/IoT Core configuration
        mqttConfig = new MqttConfig(
            server,
            clientId,
            // --------------------------------------------------------------
            // MQTT callback (not used here, but logs anything received)
            // --------------------------------------------------------------
            [](char* topic, uint8_t* payload, unsigned int length) {
                Serial.print("Received MQTT message [");
                Serial.print(topic);
                Serial.print("]: ");
                for (unsigned int i = 0; i < length; i++) Serial.print((char)payload[i]);
                Serial.println();
            },
            port
        );

        mqtt = new MqttClient(mqttConfig, net);
    }

    //-------------------------------------------------------------------------
    // setup(): initializes everything needed by the sensor device
    //-------------------------------------------------------------------------
    void setup() {
        Serial.begin(115200);

        // Initialize GPIO and read initial state
        doorSensor->begin();

        // Establish WiFi + MQTT secure connection
        mqtt->initialize();

        // If needed, we could subscribe:
        // mqtt->subscribe(subscribeTopic);
    }

    //-------------------------------------------------------------------------
    // loop(): main execution loop
    // - Polls MQTT client
    // - Detects door state changes
    // - Publishes Shadow "reported" attribute updates to AWS
    //-------------------------------------------------------------------------
    void loop() {
        mqtt->loop();

        // Check if physical state has changed since last loop
        if (doorSensor->hasStateChanged()) {

            bool isOpen = doorSensor->getLastState();

            Serial.print("Exterior door state changed -> ");
            Serial.println(isOpen ? "OPEN" : "CLOSE");

            // --------------------------------------------------------------
            // Create AWS IoT Shadow "reported" JSON payload:
            //
            // {
            //   "state": {
            //     "reported": {
            //       "exteriorDoor": "OPEN"
            //     }
            //   }
            // }
            //
            // This tells AWS the REAL hardware state.
            // --------------------------------------------------------------
            StaticJsonDocument<256> doc;
            doc["state"]["reported"]["exteriorDoor"] = isOpen ? "OPEN" : "CLOSE";

            String out;
            serializeJson(doc, out);

            // Publish state update to AWS IoT Shadow
            mqtt->publish(publishTopic, out.c_str());

            Serial.print("Shadow report (exteriorDoor): ");
            Serial.println(out);
        }
    }
};

// Static instance pointer
EspSensor* EspSensor::instance = nullptr;