#include "Arduino.h"
#include <iostream>

void setup();
void loop();
void triggerInterrupt(uint8_t interruptNum);
void setAnalogRead(uint8_t pin, int val);

int main() {
    std::cout << "Starting Edge Case Test for motion_envelope_led.ino" << std::endl;
    setup();

    std::cout << "--- Test 1: Re-triggering at different steps ---" << std::endl;
    setAnalogRead(A0, 860); // 4.2V
    triggerInterrupt(2); // Start envelope 0

    for (int i = 0; i < 50; ++i) {
        loop();
        if (i == 5) {
            std::cout << "Re-triggering at step 5" << std::endl;
            triggerInterrupt(2);
        }
        if (i == 20) {
            std::cout << "Re-triggering at step 20" << std::endl;
            triggerInterrupt(2);
        }
    }

    std::cout << "--- Test 2: Battery Voltage Transitions ---" << std::endl;
    std::cout << "Setting battery to 3.5V (analog 716)" << std::endl;
    setAnalogRead(A0, 716);
    triggerInterrupt(2);
    for (int i = 0; i < 50; ++i) {
        loop();
    }

    std::cout << "Test completed" << std::endl;
    return 0;
}
