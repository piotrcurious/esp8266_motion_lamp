#ifndef WEB_MOCK_H
#define WEB_MOCK_H

#include <string>
#include <map>
#include <functional>
#include "Arduino.h"

class EEPROMMock {
    uint8_t data[1024];
public:
    EEPROMMock() { memset(data, 0, 1024); }
    void begin(int size) { }
    uint8_t read(int addr) { return data[addr]; }
    void write(int addr, uint8_t val) { data[addr] = val; }
    void commit() {}
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
    bool hasArg(String name) { return args.count(name) > 0; }
    String arg(String name) { return args[name]; }
    void setArg(String name, String value) { args[name] = value; }
    void trigger(const char* path) { if (handlers.count(path)) handlers[path](); }
};

#endif
