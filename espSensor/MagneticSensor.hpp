// MagneticSensor.hpp
#pragma once
#include <Arduino.h>

class MagneticSensor {
private:
    int  pin;
    bool lastState;   // true = OPEN, false = CLOSE

public:
    MagneticSensor(int pin) : pin(pin), lastState(false) {}

    void begin() {
        pinMode(pin, INPUT_PULLUP);    // reed normalmente cerrado
        lastState = isOpen();
    }

    bool isOpen() {
        // LOW = abierto (según tu conexión)
        return digitalRead(pin) == LOW;
    }

    bool hasStateChanged() {
        bool currentState = isOpen();
        if (currentState != lastState) {
            lastState = currentState;
            return true;
        }
        return false;
    }

    bool getLastState() {
        return lastState;
    }
};