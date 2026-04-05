#include "Arduino.h"
#include "WebMock.h"
#include <chrono>
#include <thread>

SerialMock Serial;
LowPowerMock LowPower;
ESPMock ESP;
EEPROMMock EEPROM;
ESP8266WebServer server(80);
WiFiMock WiFi;

static uint8_t pin_modes[256];
static int pin_values[256];
static int analog_values[256];
static voidFuncPtr interrupts[256];

void pinMode(uint8_t pin, uint8_t mode) { pin_modes[pin] = mode; }
void digitalWrite(uint8_t pin, uint8_t val) {
    pin_values[pin] = val;
}
int digitalRead(uint8_t pin) { return pin_values[pin]; }
int analogRead(uint8_t pin) { return analog_values[pin]; }
void analogWrite(uint8_t pin, int val) {
    analog_values[pin] = val;
    std::cout << "[analogWrite] pin " << (int)pin << " = " << val << std::endl;
}

static auto start_time = std::chrono::steady_clock::now();
unsigned long millis() {
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time).count();
}

void delay(unsigned long ms) {
}

void yield() {
}

void attachInterrupt(uint8_t interruptNum, voidFuncPtr userFunc, int mode) {
    interrupts[interruptNum] = userFunc;
}

uint8_t digitalPinToInterrupt(uint8_t pin) { return pin; }

long map(long x, long in_min, long in_max, long out_min, long out_max) {
    if (in_max == in_min) return out_min;
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

long constrain(long x, long a, long b) {
    if (x < a) return a;
    if (x > b) return b;
    return x;
}

void triggerInterrupt(uint8_t interruptNum) {
    if (interrupts[interruptNum]) {
        interrupts[interruptNum]();
    }
}

void setAnalogRead(uint8_t pin, int val) {
    analog_values[pin] = val;
}
