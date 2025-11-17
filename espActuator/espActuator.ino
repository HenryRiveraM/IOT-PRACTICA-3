#include "EspActuator.hpp"

// Create the global actuator instance.
// This object manages Wi-Fi, MQTT, TLS certificates, servo motor control,
// and communication with AWS IoT Device Shadow.
EspActuator* espActuator = new EspActuator(
    12,                                         // Servo control pin
    "RIVERA WIFI 2.4",                          // Wi-Fi SSID
    "2880203CB.",                               // Wi-Fi Password
    "a1acybki981kqw-ats.iot.us-east-2.amazonaws.com", // AWS IoT Core endpoint
    8883,                                       // MQTT port with TLS
    "$aws/things/iot_thing/shadow/update",      // Publish topic (reported state)
    "$aws/things/iot_thing/shadow/update/delta",// Subscribe topic (shadow delta updates)
    "ESP_CLIENT_Actuator"                       // MQTT client ID for this device
);

void setup() {
    // Initialize Wi-Fi, MQTT connection, TLS certificates,
    // subscribe to shadow delta, and set servo to initial position.
    espActuator->setup();
}

void loop() {
    // Process incoming MQTT messages and keep connection alive.
    espActuator->loop();
}