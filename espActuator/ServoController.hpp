#pragma once
#include <ESP32Servo.h>

class ServoController {
  private:
    int pin;
    Servo servo;

  public:
    ServoController(int pin){
        this->pin = pin;
    };

    void begin(){
        servo.setPeriodHertz(50);
        servo.attach(pin, 500, 2500);
        close();   // posición inicial
    };

    void open() {
        Serial.println("→ Servo: OPEN (30°)");
        servo.write(30);
    };

    void close() {
        Serial.println("→ Servo: CLOSE (120°)");
        servo.write(120);
    };
};