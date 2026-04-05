#ifndef WEB_MOCK_H
#define WEB_MOCK_H

#include <string>
#include <map>
#include <functional>
#include "Arduino.h"

class EEPROMMock {
    uint8_t data[4096]; // Increased size
public:
    EEPROMMock() { memset(data, 0, 4096); }
    void begin(int size) { }
    uint8_t read(int addr) { return data[addr]; }
    void write(int addr, uint8_t val) { data[addr] = val; }
    void commit() {}
    template<typename T> void get(int addr, T &t) {
        if (addr + sizeof(T) > 4096) return;
        memcpy(&t, &data[addr], sizeof(T));
    }
    template<typename T> void put(int addr, const T &t) {
        if (addr + sizeof(T) > 4096) return;
        memcpy(&data[addr], &t, sizeof(T));
    }
};

extern EEPROMMock EEPROM;

class ESP8266WebServer {
    std::map<std::string, std::function<void()>> handlers;
    std::map<std::string, String> args;
public:
    ESP8266WebServer(int port) {}
    void on(const char* path, std::function<void()> handler) {
        handlers[path] = handler;
    }
    void begin() {}
    void stop() {}
    void handleClient() {}
    void send(int code, const char* type, String content) {
        std::cout << "[WebServer] send " << code << " " << type << std::endl;
    }
    bool hasArg(String name) { return args.count((std::string)name) > 0; }
    String arg(String name) { return args[(std::string)name]; }
    void setArg(String name, String value) { args[(std::string)name] = value; }
    void trigger(const char* path) { if (handlers.count(path)) handlers[path](); }
};

class WiFiMock {
public:
    void softAP(const char* ssid, const char* pass) { std::cout << "[WiFi] softAP " << ssid << std::endl; }
    const char* softAPIP() { return "192.168.4.1"; }
};
extern WiFiMock WiFi;

#endif
