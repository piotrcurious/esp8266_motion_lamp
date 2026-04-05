#include "Arduino.h"
#include <iostream>

void setup();
void loop();
void triggerInterrupt(uint8_t interruptNum);
void setAnalogRead(uint8_t pin, int val);
void digitalWrite(uint8_t pin, uint8_t val);

int main() {
    std::cout << "Starting ESP8266 Reset-on-Wake Test for simple_predefined_esp8266.ino" << std::endl;

    std::cout << "--- Phase 1: PIR active during boot ---" << std::endl;
    digitalWrite(2, 1); // PIR_PIN = HIGH
    setup();

    for (int i = 0; i < 50; ++i) {
        loop();
    }

    std::cout << "Test completed" << std::endl;
    return 0;
}
