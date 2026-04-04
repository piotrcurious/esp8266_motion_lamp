#include <Arduino.h>
#include <EEPROM.h>
#include <ESP8266WebServer.h>

// Define the pins for the motion sensor, the LED, and the analog input
#define D0 16
#define D1 5
#define D2 4
#define D3 0
#define MOTION_PIN D0 // The pin that can wake up esp8266 from deep sleep (XPD_DCDC to RST)
#define LED_PIN D1    // The pin that controls the LED brightness
#define ANALOG_PIN A0 // The pin that measures the battery voltage
#define CONFIG_PIN D3 // Pin to force config mode during boot (e.g., Flash button on many boards)

// Define the number of envelopes, steps, and voltage set points
#define NUM_ENVELOPES 5 // The number of battery voltage levels
#define NUM_STEPS 60 // The number of steps in each envelope

// Define the configuration mode flag and timeout
#define CONFIG_MODE_FLAG 0x55 // The flag value to indicate configuration mode
#define INIT_DONE_FLAG 0xAA // Flag to check if defaults are loaded
#ifndef MOCK_ARDUINO
#define CONFIG_MODE_TIMEOUT 60000 // Extended timeout to 60s
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
bool test_mode_active = false;
int test_env_idx = -1;

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
  memset(envelopes, 0, sizeof(envelopes));
  // Env 0: smooth fade
  for (int i = 0; i < 10; i++) envelopes[0][i] = {(byte)((i+1)*25), 100};
  envelopes[0][10] = {255, 5000};
  for (int i = 0; i < 10; i++) envelopes[0][11+i] = {(byte)(255 - (i+1)*25), 100};
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

String getVisual(int envIdx) {
  String visual = "";
  for (int j = 0; j < NUM_STEPS; j++) {
    byte b = envelopes[envIdx][j].brightness;
    if (b == 0) visual += ".";
    else if (b < 64) visual += "░";
    else if (b < 128) visual += "▒";
    else if (b < 192) visual += "▓";
    else visual += "█";
  }
  return visual;
}

void handleRoot() {
  float v = (float)analogRead(ANALOG_PIN) * 3.3 / 1024.0 * 2.0;
  String response = "<html><head><meta name='viewport' content='width=device-width, initial-scale=1'><style>";
  response += "body{background:#111;color:#0f0;font-family:monospace;padding:20px;}";
  response += "table{border-collapse:collapse;width:100%;margin-bottom:20px;border:1px solid #444;}";
  response += "th,td{border:1px solid #444;padding:8px;text-align:left;}";
  response += "th{background:#222;color:#f0f;}";
  response += "input,textarea{background:#000;color:#0ff;border:1px solid #0aa;font-family:monospace;padding:5px;width:100%;}";
  response += "input[type=submit],button{background:#0aa;color:#fff;cursor:pointer;font-weight:bold;margin-top:10px; border:none; padding:10px;}";
  response += "input[type=submit]:hover,button:hover{background:#0ff;}";
  response += ".visual{color:#ff0;letter-spacing:-1px;font-size:12px;display:block;margin:5px 0;word-break:break-all;}";
  response += "h1{color:#0ff;text-shadow:0 0 10px #0aa;}";
  response += "h3{color:#f0f;border-bottom:1px solid #f0f;}";
  response += ".dash{border:2px solid #0ff;padding:10px;margin-bottom:20px;background:#001;}";
  response += "</style></head><body><h1>> MOTION_LAMP_OS v1.3</h1>";

  response += "<div class='dash'><h3>[SYSTEM_DASHBOARD]</h3>";
  response += "BATTERY_VOLTAGE: " + String(v) + "V<br>";
  #ifndef MOCK_ARDUINO
  response += "FREE_HEAP: " + String(ESP.getFreeHeap()) + " bytes<br>";
  #endif
  response += "UPTIME: " + String(millis()/1000) + "s<br>";
  response += "FORCE_CONFIG_PIN: GPIO" + String(CONFIG_PIN) + "</div>";

  response += "<form method='post' action='/config'>";
  response += "<table><tr><th>Env</th><th>Min V</th><th>Max V</th><th>Loop Pt</th><th>Brightness Profile</th><th>Test</th></tr>";

  for (int i = 0; i < NUM_ENVELOPES; i++) {
    response += "<tr><td>" + String(i) + "</td>";
    response += "<td><input type='text' name='vmin" + String(i) + "' value='" + String(v_ranges[i][0]) + "'></td>";
    response += "<td><input type='text' name='vmax" + String(i) + "' value='" + String(v_ranges[i][1]) + "'></td>";
    response += "<td><input type='text' name='lp" + String(i) + "' value='" + String(loop_points[i]) + "'></td>";
    response += "<td><span id='vis" + String(i) + "' class='visual'>" + getVisual(i) + "</span></td>";
    response += "<td><button type='button' onclick=\"location.href='/test?env=" + String(i) + "'\">RUN</button></td></tr>";
  }
  response += "</table>";

  for (int i = 0; i < NUM_ENVELOPES; i++) {
    response += "<h3>[ENVELOPE_" + String(i) + "_DATA]</h3>";
    response += "<textarea name='steps" + String(i) + "' rows='3' oninput='updateVis(" + String(i) + ", this.value)'>";
    for (int j = 0; j < NUM_STEPS; j++) {
      response += String(envelopes[i][j].brightness) + "," + String(envelopes[i][j].duration);
      if (j < NUM_STEPS - 1) response += ";";
    }
    response += "</textarea>";
  }
  response += "<br><input type='submit' value='COMMIT_CHANGES_TO_ROM'></form>";

  response += "<script>function updateVis(idx, val){ let vis = ''; let steps = val.split(';'); steps.forEach(s => { let b = parseInt(s.split(',')[0]); if(isNaN(b) || b==0) vis+='.'; else if(b<64) vis+='░'; else if(b<128) vis+='▒'; else if(b<192) vis+='▓'; else vis+='█'; }); document.getElementById('vis'+idx).innerText = vis; }</script>";
  response += "</body></html>";
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
  server.send(200, "text/html", "<html><body style='background:#111;color:#0f0;font-family:monospace;padding:50px;'><h1>ROM_WRITE_SUCCESS</h1><p>REBOOT REQUIRED. <a href='/restart' style='color:#0ff;'>[RESTART_SYSTEM]</a></p></body></html>");
}

void handleTest() {
  if (server.hasArg("env")) {
    test_env_idx = server.arg("env").toInt();
    test_mode_active = true;
    server.send(200, "text/html", "<html><body style='background:#111;color:#0f0;font-family:monospace;padding:50px;'><h1>TEST_MODE_INITIALIZED</h1><p>Executing envelope " + String(test_env_idx) + "... <a href='/'>[BACK]</a></p></body></html>");
  } else {
    server.send(400, "text/plain", "Missing env arg");
  }
}

void handleRestart() {
  server.send(200, "text/html", "<html><body style='background:#111;color:#0f0;font-family:monospace;padding:50px;'><h1>RESTARTING...</h1></body></html>");
  delay(1000);
  ESP.restart();
}

void runEnvelope(int envIdx) {
  if (envIdx < 0 || envIdx >= NUM_ENVELOPES) return;
  bool motionWasHigh = false;
  for (int j = 0; j < NUM_STEPS; j++) {
    analogWrite(LED_PIN, envelopes[envIdx][j].brightness);
    unsigned long stepStart = millis();
    while (millis() - stepStart < envelopes[envIdx][j].duration) {
       bool motionNow = (digitalRead(MOTION_PIN) == HIGH);
       if (motionNow && !motionWasHigh) { // Rising edge detected
         if (loop_points[envIdx] != -1) {
           j = loop_points[envIdx] - 1;
           motionWasHigh = true;
           break;
         }
       }
       if (!motionNow) motionWasHigh = false;
       #ifndef MOCK_ARDUINO
       server.handleClient(); // Keep server responsive during test or config
       #endif
       yield();
    }
    if (envelopes[envIdx][j].brightness == 0 && j > 0) break;
  }
  analogWrite(LED_PIN, 0);
}

void setup() {
  Serial.begin(9600);
  EEPROM.begin(1024);

  if (EEPROM.read(1) != INIT_DONE_FLAG) loadDefaults();
  else loadFromEEPROM();

  pinMode(MOTION_PIN, INPUT);
  pinMode(CONFIG_PIN, INPUT_PULLUP);

  // Check for configuration mode entry: hold CONFIG_PIN LOW for 5 seconds during boot
  // Using active-low since it's common for buttons with internal pull-up
  bool forceConfig = true;
  unsigned long bootCheckStart = millis();
  while (millis() - bootCheckStart < 5000) {
    if (digitalRead(CONFIG_PIN) == HIGH) { // Button released
      forceConfig = false;
      break;
    }
    delay(10);
  }

  config_mode_flag = EEPROM.read(0);

  if (config_mode_flag == CONFIG_MODE_FLAG || forceConfig) {
    Serial.println("Config mode active");
    EEPROM.write(0, 0);
    EEPROM.commit();
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH); // Signal config mode
    delay(500);
    digitalWrite(LED_PIN, LOW);

    server.on("/", handleRoot);
    server.on("/config", handleConfig);
    server.on("/restart", handleRestart);
    server.on("/test", handleTest);
    server.begin();

    unsigned long configStart = millis();
    while (millis() - configStart < CONFIG_MODE_TIMEOUT || test_mode_active) {
      server.handleClient();
      if (test_mode_active) {
        runEnvelope(test_env_idx);
        test_mode_active = false;
        configStart = millis(); // Reset timeout after test
      }
      yield();
    }
    server.stop();
    ESP.deepSleep(0);
  } else {
    Serial.println("Normal mode");
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
      if (envIdx != -1) runEnvelope(envIdx);
    }
    digitalWrite(LED_PIN, LOW);
    ESP.deepSleep(0);
  }
}

void loop() {}
