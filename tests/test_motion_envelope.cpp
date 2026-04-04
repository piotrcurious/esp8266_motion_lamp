#include "Arduino.h"
#include <iostream>

void setup();
void loop();
void triggerInterrupt(uint8_t interruptNum);
void setAnalogRead(uint8_t pin, int val);

int main() {
    std::cout << "Starting Test for motion_envelope_led.ino" << std::endl;
    setup();

    std::cout << "Setting battery to 4.2V (analog 860)" << std::endl;
    setAnalogRead(A0, 860);

    std::cout << "Triggering Motion" << std::endl;
    triggerInterrupt(2);

    for (int i = 0; i < 100; ++i) {
        loop();
    }

    std::cout << "Test completed" << std::endl;
    return 0;
}
