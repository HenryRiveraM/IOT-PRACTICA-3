// MagneticSensor.hpp
#pragma once
#include <Arduino.h>

//==========================================================================
// MagneticSensor
// -------------------------------------------------------------------------
// This class encapsulates a magnetic reed switch used to detect whether
// a door/window is OPEN or CLOSED. The logic level depends on wiring:
//  - LOW  -> sensor triggered (magnet far, door open)
//  - HIGH -> sensor idle (magnet near, door closed)
// The class tracks state changes and exposes the last known state.
//==========================================================================

class MagneticSensor {
private:
    int  pin;         // GPIO where the magnetic switch is connected
    bool lastState;   // Cached state: true = OPEN, false = CLOSE

public:
    // Constructor stores GPIO and initializes lastState to "CLOSE" by default
    MagneticSensor(int pin) : pin(pin), lastState(false) {}

    //------------------------------------------------------------------------
    // begin()
    // Initializes the pin as INPUT_PULLUP since reed switches are
    // typically wired to short to ground when activated.
    // Also stores the initial state as the baseline for change detection.
    //------------------------------------------------------------------------
    void begin() {
        pinMode(pin, INPUT_PULLUP);
        lastState = isOpen();
    }

    //------------------------------------------------------------------------
    // isOpen()
    // Reads the sensor and returns true if the door is OPEN.
    // Logic depends on wiring; here LOW = OPEN.
    //------------------------------------------------------------------------
    bool isOpen() {
        return digitalRead(pin) == LOW;
    }

    //------------------------------------------------------------------------
    // hasStateChanged()
    // Returns true when the sensor transitions between OPEN/CLOSE.
    // Updates lastState so future calls detect only new changes.
    //------------------------------------------------------------------------
    bool hasStateChanged() {
        bool currentState = isOpen();
        if (currentState != lastState) {
            lastState = currentState;
            return true;
        }
        return false;
    }

    //------------------------------------------------------------------------
    // getLastState()
    // Returns the last known value without re-reading the sensor.
    // Useful after detecting a change.
    //------------------------------------------------------------------------
    bool getLastState() {
        return lastState;
    }
};