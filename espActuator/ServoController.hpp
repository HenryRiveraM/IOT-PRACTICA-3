#pragma once
#include <ESP32Servo.h>

//=====================================================
// ServoController
// ----------------------------------------------------
// This class encapsulates all logic required to control
// a servo motor used to physically move the interior door.
// It abstracts servo initialization, positioning, and
// provides simple "open" and "close" operations.
//
// Designed for 180° servos with custom calibrated angles.
//=====================================================
class ServoController {
  private:
    int pin;        // GPIO pin used by the servo signal wire
    Servo servo;    // ESP32Servo instance for PWM generation

  public:

    //-----------------------------------------------------
    // Constructor: stores the pin assigned to the servo
    //-----------------------------------------------------
    ServoController(int pin){
        this->pin = pin;
    };

    //-----------------------------------------------------
    // Initializes the servo:
    // - Sets PWM frequency
    // - Attaches the servo with calibrated pulse range
    // - Moves servo to initial "closed" position
    //-----------------------------------------------------
    void begin(){
        servo.setPeriodHertz(50);          // Standard servo PWM frequency (50 Hz)
        servo.attach(pin, 500, 2500);      // Microsecond range for ESP32 servo control
        close();                            // Move to default closed position
    };

    //-----------------------------------------------------
    // Moves the servo to the "open" position.
    // Angle 30° was calibrated manually for your hardware.
    //-----------------------------------------------------
    void open() {
        Serial.println("→ Servo: OPEN (30°)");
        servo.write(30);
    };

    //-----------------------------------------------------
    // Moves the servo to the "closed" position.
    // Angle 120° was calibrated manually for your hardware.
    //-----------------------------------------------------
    void close() {
        Serial.println("→ Servo: CLOSE (120°)");
        servo.write(120);
    };
};