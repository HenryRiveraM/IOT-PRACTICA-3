#include "EspActuator.hpp"

EspActuator* espActuator = new EspActuator(
    12,
    "RIVERA WIFI 2.4",
    "2880203CB.",
    "a1acybki981kqw-ats.iot.us-east-2.amazonaws.com",
    8883,
    "$aws/things/iot_thing/shadow/update",       // publish (reported)
    "$aws/things/iot_thing/shadow/update/delta",// subscribe (delta)
    "ESP_CLIENT_Actuator"
);

void setup() {
    espActuator->setup();
}

void loop() {
    espActuator->loop();
}