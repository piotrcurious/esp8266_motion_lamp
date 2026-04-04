#include "Arduino.h"
#include "WebMock.h"
#include <iostream>

void setup();
void loop();
void setAnalogRead(uint8_t pin, int val);
void digitalWrite(uint8_t pin, uint8_t val);

extern EEPROMMock EEPROM;
extern ESP8266WebServer server;

int main() {
    std::cout << "Starting Test for web_pid_led.ino" << std::endl;

    // First run: Set config mode flag in EEPROM
    std::cout << "--- Phase 1: Configuration ---" << std::endl;
    EEPROM.write(0, 0x55);

    setup(); // This should enter config mode

    // Simulate saving some config
    server.setArg("vmin0", "3.0");
    server.setArg("vmax0", "4.5");
    server.setArg("lp0", "1");
    server.setArg("steps0", "255,1000;128,500;0,0");
    server.trigger("/config");

    // Phase 2: Normal mode
    std::cout << "--- Phase 2: Normal Mode ---" << std::endl;
    setAnalogRead(A0, 600); // (600*3.3/1024)*2 = 3.86V
    digitalWrite(0, 1); // MOTION_PIN = HIGH (triggered)
    setup();

    std::cout << "Test completed" << std::endl;
    return 0;
}
