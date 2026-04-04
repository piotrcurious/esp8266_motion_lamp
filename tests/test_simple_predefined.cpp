#include "Arduino.h"
#include <iostream>

void setup();
void loop();
void triggerInterrupt(uint8_t interruptNum);
void setAnalogRead(uint8_t pin, int val);

int main() {
    std::cout << "Starting Test for simple_predefined.ino" << std::endl;
    setup();

    std::cout << "Setting battery to 4.2V (analog 840)" << std::endl;
    setAnalogRead(A0, 840);

    std::cout << "Triggering Motion" << std::endl;
    triggerInterrupt(2);

    for (int i = 0; i < 150; ++i) {
        loop();
    }

    std::cout << "Test completed" << std::endl;
    return 0;
}
