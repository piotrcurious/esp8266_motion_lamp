#include "Arduino.h"
#include "WebMock.h"
#include <fstream>

void setup();
void handleRoot();
extern ESP8266WebServer server;

int main() {
    setup(); // Load defaults
    // Simulate handleRoot to get HTML
    handleRoot();

    std::ofstream out("web_ui_preview.html");
    out << server.getLatestContent();
    out.close();

    return 0;
}
