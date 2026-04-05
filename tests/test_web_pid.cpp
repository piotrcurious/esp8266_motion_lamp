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

    std::cout << "--- Phase 1: Default Init ---" << std::endl;
    // EEPROM is empty, setup should load defaults
    setup();

    std::cout << "--- Phase 2: Update Config (SSID & V_MULT) ---" << std::endl;
    // Re-run setup in config mode by setting flag
    EEPROM.write(0, 0x55);
    server.setArg("ssid", "NewSSID");
    server.setArg("vmult", "3.14");
    server.setArg("vmin0", "3.0");
    server.setArg("vmax0", "4.5");
    server.setArg("steps0", "100,500; 200,1000");
    setup();
    server.trigger("/config");

    std::cout << "--- Phase 3: Normal Mode (Motion Trigger) ---" << std::endl;
    setAnalogRead(A0, 500); // 500 * (3.3/1024) * 3.14 = 5.06V -> Env 0
    digitalWrite(16, 1); // MOTION_PIN HIGH
    setup();

    std::cout << "Test completed" << std::endl;
    return 0;
}
