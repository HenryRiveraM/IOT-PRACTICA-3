#include "EspSensor.hpp"

EspSensor* espSensor = new EspSensor(
    4,
    "RIVERA WIFI 2.4",
    "2880203CB.",
    "a1acybki981kqw-ats.iot.us-east-2.amazonaws.com",
    8883,
    "ESP_CLIENT_SENSOR",
    "$aws/things/iot_thing/shadow/update",        // publish (reported)
    "$aws/things/iot_thing/shadow/update/delta"  // (no lo usas de momento)
);

void setup() {
    espSensor->setup();
}

void loop() {
    espSensor->loop();
}