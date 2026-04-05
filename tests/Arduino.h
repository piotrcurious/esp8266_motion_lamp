#ifndef ARDUINO_H
#define ARDUINO_H

#include <iostream>
#include <chrono>
#include <map>
#include <vector>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <string>
#include <algorithm>

typedef uint8_t byte;
typedef bool boolean;

#define HIGH 0x1
#define LOW  0x0

#define INPUT 0x0
#define OUTPUT 0x1
#define INPUT_PULLUP 0x2

#define RISING 0x01
#define FALLING 0x02
#define CHANGE 0x03

#define A0 0

#define PROGMEM
#define pgm_read_byte(addr) (*(const uint8_t*)(addr))
#define pgm_read_word(addr) (*(const uint16_t*)(addr))
#define pgm_read_ptr(addr) (*(const void**)(addr))
#define memcpy_P(dest, src, size) memcpy(dest, src, size)

void pinMode(uint8_t pin, uint8_t mode);
void digitalWrite(uint8_t pin, uint8_t val);
int digitalRead(uint8_t pin);
int analogRead(uint8_t pin);
void analogWrite(uint8_t pin, int val);
unsigned long millis();
void delay(unsigned long ms);
void yield();

typedef void (*voidFuncPtr)(void);
void attachInterrupt(uint8_t interruptNum, voidFuncPtr userFunc, int mode);
uint8_t digitalPinToInterrupt(uint8_t pin);

class String : public std::string {
public:
    String() : std::string("") {}
    String(const char* s) : std::string(s) {}
    String(const std::string& s) : std::string(s) {}
    String(int n) : std::string(std::to_string(n)) {}
    String(unsigned int n) : std::string(std::to_string(n)) {}
    String(long n) : std::string(std::to_string(n)) {}
    String(unsigned long n) : std::string(std::to_string(n)) {}
    String(float f) : std::string(std::to_string(f)) {}
    String(double f) : std::string(std::to_string(f)) {}

    float toFloat() const {
        try { return std::stof(*this); } catch(...) { return 0; }
    }
    int toInt() const {
        try { return std::stoi(*this); } catch(...) { return 0; }
    }
    int indexOf(char c, int start = 0) const {
        size_t pos = find(c, start);
        return (pos == std::string::npos) ? -1 : (int)pos;
    }
    String substring(int start, int end = -1) const {
        if (end == -1) return String(substr(start));
        return String(substr(start, end - start));
    }
    void trim() {
        erase(begin(), find_if(begin(), end(), [](unsigned char ch) { return !isspace(ch); }));
        erase(find_if(rbegin(), rend(), [](unsigned char ch) { return !isspace(ch); }).base(), end());
    }

    String operator+(const String& other) const {
        return String((std::string)*this + (std::string)other);
    }
    String operator+(const char* other) const {
        return String((std::string)*this + std::string(other));
    }
    String operator+(const std::string& other) const {
        return String((std::string)*this + other);
    }
    String& operator+=(const String& other) {
        this->append(other);
        return *this;
    }
    String& operator+=(const char* other) {
        this->append(other);
        return *this;
    }
    String& operator+=(const std::string& other) {
        this->append(other);
        return *this;
    }
};

inline String operator+(const char* a, const String& b) {
    return String(std::string(a) + (std::string)b);
}

inline String operator+(const std::string& a, const String& b) {
    return String(a + (std::string)b);
}

class SerialMock {
public:
    void begin(unsigned long baud) {}
    void print(const char* s) { std::cout << s; }
    void print(int n) { std::cout << n; }
    void print(float f) { std::cout << f; }
    void print(String s) { std::cout << (std::string)s; }
    void println(const char* s) { std::cout << s << std::endl; }
    void println(int n) { std::cout << n << std::endl; }
    void println(float f) { std::cout << f << std::endl; }
    void println(String s) { std::cout << (std::string)s << std::endl; }
    void println() { std::cout << std::endl; }
};

extern SerialMock Serial;

long map(long x, long in_min, long in_max, long out_min, long out_max);
long constrain(long x, long a, long b);

// LowPower mock
class LowPowerMock {
public:
    void powerDown(int period, int adc, int bod) {
        // std::cout << "[LowPower] powerDown called" << std::endl;
    }
};
extern LowPowerMock LowPower;
#define SLEEP_FOREVER 0
#define ADC_OFF 0
#define BOD_OFF 0

// ESP mock
class ESPMock {
public:
    void deepSleep(unsigned long us) {
        std::cout << "[ESP] deepSleep called" << std::endl;
    }
    void restart() {
        std::cout << "[ESP] restart called" << std::endl;
    }
    unsigned long getFreeHeap() { return 40000; }
};
extern ESPMock ESP;

// AVR sleep mock
#define SLEEP_MODE_PWR_DOWN 0
inline void set_sleep_mode(int mode) {}
inline void sleep_enable() {}
inline void sleep_disable() {}
inline void sleep_bod_disable() {}
inline void sleep_cpu() {
    // std::cout << "[AVR] sleep_cpu called" << std::endl;
}
inline void cli() {}
inline void sei() {}

#endif
