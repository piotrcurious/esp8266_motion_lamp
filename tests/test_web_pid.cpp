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
    server.setArg("v00", "3.0");
    server.setArg("v01", "4.5");
    server.setArg("v02", "10"); // looping point
    server.setArg("b00", "255");
    server.setArg("b01", "128");
    server.setArg("b02", "0");
    server.trigger("/config");

    // Phase 2: Normal mode
    std::cout << "--- Phase 2: Normal Mode ---" << std::endl;
    // EEPROM flag was cleared by setup
    setAnalogRead(A0, 600); // ~3.8V
    digitalWrite(0, 1); // MOTION_PIN = HIGH
    setup();

    std::cout << "Test completed" << std::endl;
    return 0;
}
