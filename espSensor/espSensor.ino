#include "EspSensor.hpp"

//==========================================================================
// Global instance of the sensor controller
// -------------------------------------------------------------------------
// This ESP32 acts as the *exterior door sensor device*.
// It reads the magnetic reed switch and reports "OPEN"/"CLOSE" to AWS IoT Core
// via the Device Shadow (reported state).
//==========================================================================

EspSensor* espSensor = new EspSensor(
    4,                                          // GPIO pin where the magnetic sensor is connected
    "RIVERA WIFI 2.4",                          // WiFi SSID
    "2880203CB.",                               // WiFi password
    "a1acybki981kqw-ats.iot.us-east-2.amazonaws.com", // AWS IoT Core endpoint
    8883,                                       // Secure MQTT TLS port
    "ESP_CLIENT_SENSOR",                        // MQTT client ID for this device
    "$aws/things/iot_thing/shadow/update",      // Topic used to publish Shadow "reported" updates
    "$aws/things/iot_thing/shadow/update/delta" // Topic normally used for desired-state deltas (not used here)
);

//==========================================================================
// setup()
// -------------------------------------------------------------------------
// Initializes:
//  - Serial output
//  - WiFi connection
//  - TLS certificates
//  - MQTT client connection to AWS IoT Core
//  - Magnetic sensor initial state
//==========================================================================
void setup() {
    espSensor->setup();
}

//==========================================================================
// loop()
// -------------------------------------------------------------------------
// Main execution loop for the device:
//  - Keeps MQTT connection alive
//  - Checks if the door sensor changed state
//  - Publishes new "reported" state to AWS IoT Shadow when needed
//==========================================================================
void loop() {
    espSensor->loop();
}