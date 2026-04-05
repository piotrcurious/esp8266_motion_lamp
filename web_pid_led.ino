#include <Arduino.h>
#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

// Define the pins
#define D0 16
#define D1 5
#define D2 4
#define D3 0
#define MOTION_PIN D0
#define LED_PIN D1
#define ANALOG_PIN A0
#define CONFIG_PIN D3

#define NUM_ENVELOPES 5
#define NUM_STEPS 60

#define CONFIG_MODE_FLAG 0x55
#define INIT_DONE_FLAG 0xAB
#ifndef MOCK_ARDUINO
#define CONFIG_MODE_TIMEOUT 120000
#else
#define CONFIG_MODE_TIMEOUT 10
#endif

struct Step {
  byte brightness;
  unsigned int duration;
};

struct DeviceConfig {
  char ssid[32];
  char pass[32];
  float v_multiplier;
  float v_ranges[NUM_ENVELOPES][2];
  byte loop_points[NUM_ENVELOPES];
  Step envelopes[NUM_ENVELOPES][NUM_STEPS];
};

DeviceConfig cfg;
byte config_mode_flag;
bool test_mode_active = false;
int test_env_idx = -1;

#ifndef MOCK_ARDUINO
ESP8266WebServer server(80);
#else
extern ESP8266WebServer server;
#endif

void saveToEEPROM() {
  EEPROM.put(2, cfg);
  EEPROM.commit();
}

void loadFromEEPROM() {
  EEPROM.get(2, cfg);
}

void loadDefaults() {
  memset(&cfg, 0, sizeof(cfg));
  strncpy(cfg.ssid, "MotionLampAP", 31);
  strncpy(cfg.pass, "12345678", 31);
  cfg.v_multiplier = 2.0;
  for (int i = 0; i < 10; i++) cfg.envelopes[0][i] = {(byte)((i+1)*25), 100};
  cfg.envelopes[0][10] = {255, 5000};
  for (int i = 0; i < 10; i++) cfg.envelopes[0][11+i] = {(byte)(255 - (i+1)*25), 100};
  cfg.v_ranges[0][0] = 4.0; cfg.v_ranges[0][1] = 5.0; cfg.loop_points[0] = 10;
  for (int p = 0; p < 3; p++) {
    for (int i = 0; i < 5; i++) cfg.envelopes[1][p*10 + i] = {(byte)((i+1)*51), 100};
    for (int i = 0; i < 5; i++) cfg.envelopes[1][p*10 + 5 + i] = {(byte)(255 - (i+1)*51), 100};
  }
  cfg.v_ranges[1][0] = 3.7; cfg.v_ranges[1][1] = 4.0; cfg.loop_points[1] = 0;
  for (int i = 0; i < 10; i++) {
    cfg.envelopes[2][i*4] = {255, 100}; cfg.envelopes[2][i*4+1] = {0, 100};
    cfg.envelopes[2][i*4+2] = {255, 100}; cfg.envelopes[2][i*4+3] = {0, 700};
  }
  cfg.v_ranges[2][0] = 3.4; cfg.v_ranges[2][1] = 3.7; cfg.loop_points[2] = 0;
  cfg.envelopes[3][0] = {32, 30000};
  cfg.v_ranges[3][0] = 3.2; cfg.v_ranges[3][1] = 3.4; cfg.loop_points[3] = 0;
  for (int i = 0; i < 30; i++) {
    cfg.envelopes[4][i*2] = {255, 200}; cfg.envelopes[4][i*2+1] = {0, 200};
  }
  cfg.v_ranges[4][0] = 0.0; cfg.v_ranges[4][1] = 3.2; cfg.loop_points[4] = 0;
  saveToEEPROM();
  EEPROM.write(1, INIT_DONE_FLAG);
  EEPROM.commit();
}

String getVisual(int envIdx) {
  String visual = "";
  for (int j = 0; j < NUM_STEPS; j++) {
    byte b = cfg.envelopes[envIdx][j].brightness;
    String ch = ".";
    if (b > 0) {
      if (b < 64) ch = "░";
      else if (b < 128) ch = "▒";
      else if (b < 192) ch = "▓";
      else ch = "█";
    }
    visual += "<span id='v" + String(envIdx) + "_" + String(j) + "'>" + ch + "</span>";
  }
  return visual;
}

void handleRoot() {
  float v = (float)analogRead(ANALOG_PIN) * (3.3 / 1024.0) * cfg.v_multiplier;
  String response = "<html><head><meta name='viewport' content='width=device-width, initial-scale=1'><style>";
  response += "body{background:#111;color:#0f0;font-family:monospace;padding:20px;}";
  response += "table{border-collapse:collapse;width:100%;margin-bottom:20px;border:1px solid #444;}";
  response += "th,td{border:1px solid #444;padding:8px;text-align:left;}";
  response += "th{background:#222;color:#f0f;}";
  response += "input,textarea{background:#000;color:#0ff;border:1px solid #0aa;font-family:monospace;padding:5px;width:100%;}";
  response += "input[type=submit],button{background:#0aa;color:#fff;cursor:pointer;font-weight:bold;margin-top:10px; border:none; padding:10px;}";
  response += "input[type=submit]:hover,button:hover{background:#0ff;}";
  response += ".visual{color:#ff0;letter-spacing:-1px;font-size:12px;display:block;margin:5px 0;word-break:break-all;cursor:crosshair;}";
  response += ".visual span.sel{background:#f00;color:#fff;}";
  response += "h1{color:#0ff;text-shadow:0 0 10px #0aa;}";
  response += "h3{color:#f0f;border-bottom:1px solid #f0f;}";
  response += ".dash{border:2px solid #0ff;padding:10px;margin-bottom:20px;background:#001;}";
  response += ".kbd-help{color:#888;font-size:0.8em;margin-bottom:10px;}";
  response += "</style></head><body><h1>> MOTION_LAMP_OS v1.5</h1>";

  response += "<div class='dash'><h3>[SYSTEM_DASHBOARD]</h3>";
  response += "BATTERY: " + String(v) + "V<br>UPTIME: " + String((unsigned long)(millis()/1000)) + "s</div>";

  response += "<div class='kbd-help'>[KEYBOARD_EDIT_MODE]: Click a Brightness Profile. Use ARROWS to navigate & change brightness.</div>";

  response += "<form method='post' action='/config' id='cfgForm'>";
  response += "<table><tr><th>Env</th><th>Min V</th><th>Max V</th><th>Loop Pt</th><th>Brightness Profile</th><th>Test</th></tr>";
  for (int i = 0; i < NUM_ENVELOPES; i++) {
    response += "<tr><td>" + String(i) + "</td>";
    response += "<td><input type='text' name='vmin" + String(i) + "' value='" + String(cfg.v_ranges[i][0]) + "'></td>";
    response += "<td><input type='text' name='vmax" + String(i) + "' value='" + String(cfg.v_ranges[i][1]) + "'></td>";
    response += "<td><input type='text' name='lp" + String(i) + "' value='" + String(cfg.loop_points[i]) + "'></td>";
    response += "<td><div class='visual' onclick='focusEnv(" + String(i) + ")' id='visCont" + String(i) + "'>" + getVisual(i) + "</div></td>";
    response += "<td><button type='button' onclick=\"location.href='/test?env=" + String(i) + "'\">RUN</button></td></tr>";
  }
  response += "</table>";
  for (int i = 0; i < NUM_ENVELOPES; i++) {
    response += "<textarea name='steps" + String(i) + "' id='steps" + String(i) + "' style='display:none'>";
    for (int j = 0; j < NUM_STEPS; j++) {
      response += String(cfg.envelopes[i][j].brightness) + "," + String(cfg.envelopes[i][j].duration);
      if (j < NUM_STEPS - 1) response += ";";
    }
    response += "</textarea>";
  }
  response += "<br><input type='submit' value='COMMIT_CHANGES_TO_ROM'></form>";

  response += "<script>";
  response += "let curEnv=-1, curStep=0;";
  response += "function focusEnv(e){ if(curEnv!=-1) clearSel(); curEnv=e; curStep=0; updateSel(); }";
  response += "function clearSel(){ let el=document.getElementById('v'+curEnv+'_'+curStep); if(el) el.classList.remove('sel'); }";
  response += "function updateSel(){ let el=document.getElementById('v'+curEnv+'_'+curStep); if(el) el.classList.add('sel'); }";
  response += "document.addEventListener('keydown', (e)=>{ if(curEnv==-1) return; if(['ArrowLeft','ArrowRight','ArrowUp','ArrowDown'].includes(e.key)) e.preventDefault();";
  response += "clearSel(); if(e.key=='ArrowLeft') curStep=Math.max(0,curStep-1); else if(e.key=='ArrowRight') curStep=Math.min(59,curStep+1);";
  response += "let ta=document.getElementById('steps'+curEnv); let steps=ta.value.split(';'); let parts=steps[curStep].split(','); let b=parseInt(parts[0]);";
  response += "if(e.key=='ArrowUp') b=Math.min(255,b+15); else if(e.key=='ArrowDown') b=Math.max(0,b-15);";
  response += "parts[0]=b; steps[curStep]=parts.join(','); ta.value=steps.join(';');";
  response += "let ch='.'; if(b>0){ if(b<64) ch='░'; else if(b<128) ch='▒'; else if(b<192) ch='▓'; else ch='█'; }";
  response += "document.getElementById('v'+curEnv+'_'+curStep).innerText=ch; updateSel(); });";
  response += "</script></body></html>";
  server.send(200, "text/html", response);
}

void handleConfig() {
  if (server.hasArg("ssid")) strncpy(cfg.ssid, server.arg("ssid").c_str(), 31);
  if (server.hasArg("pass")) strncpy(cfg.pass, server.arg("pass").c_str(), 31);
  if (server.hasArg("vmult")) cfg.v_multiplier = server.arg("vmult").toFloat();
  for (int i = 0; i < NUM_ENVELOPES; i++) {
    if (server.hasArg("vmin" + String(i))) cfg.v_ranges[i][0] = server.arg("vmin" + String(i)).toFloat();
    if (server.hasArg("vmax" + String(i))) cfg.v_ranges[i][1] = server.arg("vmax" + String(i)).toFloat();
    if (server.hasArg("lp" + String(i))) cfg.loop_points[i] = (byte)server.arg("lp" + String(i)).toInt();
    if (server.hasArg("steps" + String(i))) {
      String stepsStr = server.arg("steps" + String(i));
      int start = 0;
      memset(cfg.envelopes[i], 0, sizeof(cfg.envelopes[i]));
      for (int j = 0; j < NUM_STEPS; j++) {
        int sep = stepsStr.indexOf(';', start);
        String step = (sep == -1) ? stepsStr.substring(start) : stepsStr.substring(start, sep);
        step.trim();
        if (step.length() == 0) continue;
        int comma = step.indexOf(',');
        if (comma != -1) {
          cfg.envelopes[i][j].brightness = (byte)step.substring(0, comma).toInt();
          cfg.envelopes[i][j].duration = (unsigned int)step.substring(comma + 1).toInt();
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
    analogWrite(LED_PIN, cfg.envelopes[envIdx][j].brightness);
    unsigned long stepStart = millis();
    while (millis() - stepStart < cfg.envelopes[envIdx][j].duration) {
       bool motionNow = (digitalRead(MOTION_PIN) == HIGH);
       if (motionNow && !motionWasHigh) {
         if (cfg.loop_points[envIdx] != 0xFF && cfg.loop_points[envIdx] < NUM_STEPS) {
           j = cfg.loop_points[envIdx] - 1;
           motionWasHigh = true;
           break;
         }
       }
       if (!motionNow) motionWasHigh = false;
       #ifndef MOCK_ARDUINO
       server.handleClient();
       #endif
       yield();
    }
    if (cfg.envelopes[envIdx][j].brightness == 0 && j > 0) break;
  }
  analogWrite(LED_PIN, 0);
}

void setup() {
  Serial.begin(9600);
  EEPROM.begin(sizeof(DeviceConfig) + 10);
  if (EEPROM.read(1) != INIT_DONE_FLAG) loadDefaults();
  else loadFromEEPROM();
  pinMode(MOTION_PIN, INPUT);
  pinMode(CONFIG_PIN, INPUT_PULLUP);
  bool forceConfig = true;
  unsigned long bootCheckStart = millis();
  while (millis() - bootCheckStart < 5000) {
    if (digitalRead(CONFIG_PIN) == HIGH) {
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
    digitalWrite(LED_PIN, HIGH);
    WiFi.softAP(cfg.ssid, cfg.pass);
    Serial.print("AP IP: ");
    Serial.println(WiFi.softAPIP());
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
        configStart = millis();
      }
      yield();
    }
    server.stop();
    ESP.deepSleep(0);
  } else {
    Serial.println("Normal mode");
    pinMode(LED_PIN, OUTPUT);
    if (digitalRead(MOTION_PIN) == HIGH) {
      float battery_v = (float)analogRead(ANALOG_PIN) * (3.3 / 1024.0) * cfg.v_multiplier;
      int envIdx = -1;
      for (int i = 0; i < NUM_ENVELOPES; i++) {
        if (battery_v >= cfg.v_ranges[i][0] && battery_v <= cfg.v_ranges[i][1]) {
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
