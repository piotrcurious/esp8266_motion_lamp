#include <Arduino.h>
#include <EEPROM.h>
#include <ESP8266WebServer.h>

// Define the pins for the motion sensor, the LED, and the analog input
#define D0 0
#define D1 1
#define MOTION_PIN D0 // The pin that can wake up esp8266 from deep sleep
#define LED_PIN D1 // The pin that controls the LED brightness
#define ANALOG_PIN A0 // The pin that measures the battery voltage

// Define the number of envelopes, steps, and voltage set points
#define NUM_ENVELOPES 5 // The number of battery voltage levels
#define NUM_STEPS 60 // The number of steps in each envelope

// Define the configuration mode flag and timeout
#define CONFIG_MODE_FLAG 0x55 // The flag value to indicate configuration mode
#define INIT_DONE_FLAG 0xAA // Flag to check if defaults are loaded
#ifndef MOCK_ARDUINO
#define CONFIG_MODE_TIMEOUT 30000 // The timeout value for configuration mode in milliseconds
#else
#define CONFIG_MODE_TIMEOUT 10
#endif

// Structure for each step in RAM
struct Step {
  byte brightness; // 0-255
  unsigned int duration; // ms
};

// Global variables
Step envelopes[NUM_ENVELOPES][NUM_STEPS];
float v_ranges[NUM_ENVELOPES][2]; // [min_v, max_v]
int loop_points[NUM_ENVELOPES];
byte config_mode_flag;

#ifndef MOCK_ARDUINO
ESP8266WebServer server(80);
#else
extern ESP8266WebServer server;
#endif

void saveToEEPROM() {
  int addr = 2; // addr 0 is config_mode, addr 1 is init_done
  for (int i = 0; i < NUM_ENVELOPES; i++) {
    for (int j = 0; j < NUM_STEPS; j++) {
      EEPROM.write(addr++, envelopes[i][j].brightness);
      EEPROM.write(addr++, envelopes[i][j].duration >> 8);
      EEPROM.write(addr++, envelopes[i][j].duration & 0xFF);
    }
    byte high_vmin = (int)(v_ranges[i][0] * 100) >> 8;
    byte low_vmin = (int)(v_ranges[i][0] * 100) & 0xFF;
    EEPROM.write(addr++, high_vmin);
    EEPROM.write(addr++, low_vmin);
    byte high_vmax = (int)(v_ranges[i][1] * 100) >> 8;
    byte low_vmax = (int)(v_ranges[i][1] * 100) & 0xFF;
    EEPROM.write(addr++, high_vmax);
    EEPROM.write(addr++, low_vmax);
    EEPROM.write(addr++, (byte)loop_points[i]);
  }
  EEPROM.commit();
}

void loadFromEEPROM() {
  int addr = 2;
  for (int i = 0; i < NUM_ENVELOPES; i++) {
    for (int j = 0; j < NUM_STEPS; j++) {
      envelopes[i][j].brightness = EEPROM.read(addr++);
      envelopes[i][j].duration = (EEPROM.read(addr++) << 8) | EEPROM.read(addr++);
    }
    v_ranges[i][0] = (float)((EEPROM.read(addr++) << 8) | EEPROM.read(addr++)) / 100.0;
    v_ranges[i][1] = (float)((EEPROM.read(addr++) << 8) | EEPROM.read(addr++)) / 100.0;
    loop_points[i] = (int)EEPROM.read(addr++);
  }
}

void loadDefaults() {
  // Clear all
  memset(envelopes, 0, sizeof(envelopes));

  // Env 0: smooth fade
  for (int i = 0; i < 10; i++) {
    envelopes[0][i] = {(byte)((i+1)*25), 100};
  }
  envelopes[0][10] = {255, 5000};
  for (int i = 0; i < 10; i++) {
    envelopes[0][11+i] = {(byte)(255 - (i+1)*25), 100};
  }
  v_ranges[0][0] = 4.0; v_ranges[0][1] = 5.0; loop_points[0] = 10;

  // Env 1: pulse
  for (int p = 0; p < 3; p++) {
    for (int i = 0; i < 5; i++) envelopes[1][p*10 + i] = {(byte)((i+1)*51), 100};
    for (int i = 0; i < 5; i++) envelopes[1][p*10 + 5 + i] = {(byte)(255 - (i+1)*51), 100};
  }
  v_ranges[1][0] = 3.7; v_ranges[1][1] = 4.0; loop_points[1] = 0;

  // Env 2: strobe
  for (int i = 0; i < 10; i++) {
    envelopes[2][i*4] = {255, 100}; envelopes[2][i*4+1] = {0, 100};
    envelopes[2][i*4+2] = {255, 100}; envelopes[2][i*4+3] = {0, 700};
  }
  v_ranges[2][0] = 3.4; v_ranges[2][1] = 3.7; loop_points[2] = 0;

  // Env 3: night light
  envelopes[3][0] = {32, 30000};
  v_ranges[3][0] = 3.2; v_ranges[3][1] = 3.4; loop_points[3] = 0;

  // Env 4: rapid
  for (int i = 0; i < 30; i++) {
    envelopes[4][i*2] = {255, 200}; envelopes[4][i*2+1] = {0, 200};
  }
  v_ranges[4][0] = 0.0; v_ranges[4][1] = 3.2; loop_points[4] = 0;

  saveToEEPROM();
  EEPROM.write(1, INIT_DONE_FLAG);
  EEPROM.commit();
}

void handleRoot() {
  String response = "<html><head><title>ESP8266 Motion Sensor Lamp Configuration</title></head><body><h1>ESP8266 Motion Sensor Lamp Configuration</h1><form method=\"post\" action=\"/config\"><table border=\"1\"><tr><th>Env</th><th>Min V</th><th>Max V</th><th>Loop Point</th></tr>";

  for (int i = 0; i < NUM_ENVELOPES; i++) {
    response += "<tr><td>" + String(i) + "</td>";
    response += "<td><input type=\"text\" name=\"vmin" + String(i) + "\" value=\"" + String(v_ranges[i][0]) + "\"></td>";
    response += "<td><input type=\"text\" name=\"vmax" + String(i) + "\" value=\"" + String(v_ranges[i][1]) + "\"></td>";
    response += "<td><input type=\"text\" name=\"lp" + String(i) + "\" value=\"" + String(loop_points[i]) + "\"></td></tr>";
  }
  response += "</table><p>Each envelope has 60 steps. Format: brightness,duration(ms);brightness,duration(ms);...</p>";
  for (int i = 0; i < NUM_ENVELOPES; i++) {
    response += "<h3>Envelope " + String(i) + " Steps</h3>";
    response += "<textarea name=\"steps" + String(i) + "\" rows=\"4\" cols=\"100\">";
    for (int j = 0; j < NUM_STEPS; j++) {
      response += String(envelopes[i][j].brightness) + "," + String(envelopes[i][j].duration);
      if (j < NUM_STEPS - 1) response += ";";
    }
    response += "</textarea>";
  }
  response += "<br><input type=\"submit\" value=\"Save\"></form></body></html>";
  server.send(200, "text/html", response);
}

void handleConfig() {
  for (int i = 0; i < NUM_ENVELOPES; i++) {
    if (server.hasArg("vmin" + String(i))) v_ranges[i][0] = server.arg("vmin" + String(i)).toFloat();
    if (server.hasArg("vmax" + String(i))) v_ranges[i][1] = server.arg("vmax" + String(i)).toFloat();
    if (server.hasArg("lp" + String(i))) loop_points[i] = server.arg("lp" + String(i)).toInt();

    if (server.hasArg("steps" + String(i))) {
      String stepsStr = server.arg("steps" + String(i));
      int start = 0;
      for (int j = 0; j < NUM_STEPS; j++) {
        int sep = stepsStr.indexOf(';', start);
        String step = (sep == -1) ? stepsStr.substring(start) : stepsStr.substring(start, sep);
        int comma = step.indexOf(',');
        if (comma != -1) {
          envelopes[i][j].brightness = step.substring(0, comma).toInt();
          envelopes[i][j].duration = step.substring(comma + 1).toInt();
        }
        if (sep == -1) break;
        start = sep + 1;
      }
    }
  }
  saveToEEPROM();
  server.send(200, "text/html", "Configuration saved. Please <a href=\"/restart\">restart</a>.");
}

void handleRestart() {
  server.send(200, "text/html", "Restarting...");
  delay(1000);
  ESP.restart();
}

void setup() {
  Serial.begin(9600);
  EEPROM.begin(1024);

  if (EEPROM.read(1) != INIT_DONE_FLAG) {
    loadDefaults();
  } else {
    loadFromEEPROM();
  }

  config_mode_flag = EEPROM.read(0);

  if (config_mode_flag == CONFIG_MODE_FLAG) {
    Serial.println("Config mode");
    EEPROM.write(0, 0);
    EEPROM.commit();

    pinMode(MOTION_PIN, INPUT);
    server.on("/", handleRoot);
    server.on("/config", handleConfig);
    server.on("/restart", handleRestart);
    server.begin();

    unsigned long start = millis();
    while (millis() - start < CONFIG_MODE_TIMEOUT) {
      server.handleClient();
      #ifndef MOCK_ARDUINO
      if (digitalRead(MOTION_PIN) == HIGH) break;
      #else
      break;
      #endif
    }
    server.stop();
    ESP.deepSleep(0);
  } else {
    Serial.println("Normal mode");
    pinMode(MOTION_PIN, INPUT);
    pinMode(LED_PIN, OUTPUT);

    if (digitalRead(MOTION_PIN) == HIGH) {
      float battery_v = (float)analogRead(ANALOG_PIN) * 3.3 / 1024.0 * 2.0;
      int envIdx = -1;
      for (int i = 0; i < NUM_ENVELOPES; i++) {
        if (battery_v >= v_ranges[i][0] && battery_v <= v_ranges[i][1]) {
          envIdx = i;
          break;
        }
      }

      if (envIdx != -1) {
        for (int j = 0; j < NUM_STEPS; j++) {
          analogWrite(LED_PIN, envelopes[envIdx][j].brightness);
          unsigned long stepStart = millis();
          #ifndef MOCK_ARDUINO
          while (millis() - stepStart < envelopes[envIdx][j].duration) {
             if (digitalRead(MOTION_PIN) == HIGH) {
               if (loop_points[envIdx] != -1) {
                 j = loop_points[envIdx] - 1;
                 break;
               }
             }
             yield();
          }
          #else
          if (digitalRead(MOTION_PIN) == HIGH && loop_points[envIdx] != -1) {
              // Simulating re-trigger logic for mock
          }
          #endif
          if (envelopes[envIdx][j].brightness == 0 && j > 0) break;
        }
      }
    }
    digitalWrite(LED_PIN, LOW);
    ESP.deepSleep(0);
  }
}

void loop() {}
