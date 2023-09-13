// Define the pins for the motion sensor, the LED, and the analog input
#define MOTION_PIN D0 // The pin that can wake up esp8266 from deep sleep
#define LED_PIN D1 // The pin that controls the LED brightness
#define ANALOG_PIN A0 // The pin that measures the battery voltage

// Define the number of envelopes, steps, and set points
#define NUM_ENVELOPES 5 // The number of battery voltage levels
#define NUM_STEPS 60 // The number of steps in each envelope
#define NUM_SET_POINTS 5 // The number of set points for each envelope

// Define the configuration mode flag and timeout
#define CONFIG_MODE_FLAG 0x55 // The flag value to indicate configuration mode
#define CONFIG_MODE_TIMEOUT 30000 // The timeout value for configuration mode in milliseconds

// Define the web server port and the HTML page content
#define WEB_SERVER_PORT 80 // The port for the web server
#define HTML_PAGE "<html><head><title>ESP8266 Motion Sensor Lamp Configuration</title></head><body><h1>ESP8266 Motion Sensor Lamp Configuration</h1><form method=\"post\" action=\"/config\"><table border=\"1\"><tr><th>Envelope</th><th>Set Point</th><th>Battery Voltage (V)</th><th>LED Brightness (0-1023)</th></tr>" // The HTML page content before the table data

// Declare the global variables for the envelopes, set points, and configuration mode flag
int envelopes[NUM_ENVELOPES][NUM_STEPS]; // The array to store the envelopes data
float set_points[NUM_ENVELOPES][NUM_SET_POINTS]; // The array to store the set points data
byte config_mode_flag; // The variable to store the configuration mode flag

// Declare the EEPROM library and the web server object
#include <EEPROM.h>
#include <ESP8266WebServer.h>
ESP8266WebServer server(WEB_SERVER_PORT);

// The setup function runs once when the device starts or resets
void setup() {
  // Initialize the serial communication for debugging
  Serial.begin(9600);
  
  // Initialize the EEPROM library and read the configuration mode flag
  EEPROM.begin(512);
  config_mode_flag = EEPROM.read(0);

  // Check if the configuration mode flag is set or not
  if (config_mode_flag == CONFIG_MODE_FLAG) {
    // Enter the configuration mode and clear the flag
    Serial.println("Entering configuration mode");
    EEPROM.write(0, 0);
    EEPROM.commit();

    // Initialize the pins for the motion sensor and the LED
    pinMode(MOTION_PIN, INPUT);
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

    // Initialize the web server and handle requests
    server.on("/", handleRoot); // Handle GET requests for the root path
    server.on("/config", handleConfig); // Handle POST requests for the config path
    server.begin();
    Serial.println("Web server started");

    // Wait for configuration mode timeout or motion detection
    unsigned long start_time = millis();
    while (millis() - start_time < CONFIG_MODE_TIMEOUT) {
      server.handleClient(); // Handle any incoming requests from clients
      if (digitalRead(MOTION_PIN) == HIGH) {
        break; // Exit the loop if motion is detected
      }
    }

    // Stop the web server and exit the configuration mode
    server.stop();
    Serial.println("Web server stopped");
    Serial.println("Exiting configuration mode");
  }
  else {
    // Enter the normal mode and read the envelopes and set points data from EEPROM
    Serial.println("Entering normal mode");
    for (int i = 0; i < NUM_ENVELOPES; i++) {
      for (int j = 0; j < NUM_STEPS; j++) {
        envelopes[i][j] = EEPROM.read(1 + i * NUM_STEPS + j); // Read one byte per step
      }
      for (int k = 0; k < NUM_SET_POINTS; k++) {
        byte high_byte = EEPROM.read(1 + NUM_ENVELOPES * NUM_STEPS + i * NUM_SET_POINTS * 2 + k * 2); // Read one byte per high byte of set point
        byte low_byte = EEPROM.read(1 + NUM_ENVELOPES * NUM_STEPS + i * NUM_SET_POINTS * 2 + k * 2 + 1); // Read one byte per low byte of set point
        set_points[i][k] = (high_byte << 8 | low_byte) / 100.0; // Combine two bytes and divide by 100 to get float value of set point
      }
    }

    // Initialize the pins for the motion sensor and the LED
    pinMode(MOTION_PIN, INPUT);
    pinMode(LED_PIN, OUTPUT);

    // Check if motion is detected or not
    if (digitalRead(MOTION_PIN) == HIGH) {
      // Execute the envelope based on the battery voltage level
      Serial.println("Motion detected");
      float battery_voltage = analogRead(ANALOG_PIN) * 3.3 / 1024.0 * 2.0; // Calculate the battery voltage from the analog input value
      Serial.print("Battery voltage: ");
      Serial.println(battery_voltage);
      int envelope_index = -1; // The index of the envelope to execute
      for (int i = 0; i < NUM_ENVELOPES; i++) {
        if (battery_voltage >= set_points[i][0] && battery_voltage <= set_points[i][1]) {
          // The battery voltage is within the range of this envelope
          envelope_index = i;
          break;
        }
      }
      if (envelope_index != -1) {
        // Execute the envelope with the given index
        Serial.print("Executing envelope ");
        Serial.println(envelope_index);
        for (int j = 0; j < NUM_STEPS; j++) {
          // Execute each step of the envelope
          int led_brightness = envelopes[envelope_index][j]; // Get the LED brightness value for this step
          analogWrite(LED_PIN, led_brightness); // Set the LED brightness
          delay(1000); // Wait for one second
          if (digitalRead(MOTION_PIN) == HIGH) {
            // Motion is detected again, loop back to the looping point of the envelope
            Serial.println("Motion detected again");
            j = set_points[envelope_index][2] - 1; // Set the step index to the looping point minus one
          }
        }
      }
      else {
        // No envelope matches the battery voltage level, do nothing
        Serial.println("No envelope matches the battery voltage level");
      }
    }
    else {
      // No motion is detected, do nothing
      Serial.println("No motion detected");
    }

    // Turn off the LED and enter deep sleep mode until motion is detected again
    digitalWrite(LED_PIN, LOW);
    ESP.deepSleep(0);
    Serial.println("Entering deep sleep mode");
  }
}

// The loop function runs repeatedly after the setup function finishes
void loop() {
  // Do nothing in the loop function
}

// The function to handle GET requests for the root path
void handleRoot() {
  // Send the HTML page content as the response
  String response = HTML_PAGE; // Start with the HTML page content before the table data
  for (int i = 0; i < NUM_ENVELOPES; i++) {
    for (int j = 0; j < NUM_SET_POINTS; j++) {
      response += "<tr>"; // Start a new table row
      response += "<td>" + String(i) + "</td>"; // Add the envelope number as the first column
      response += "<td>" + String(j) + "</td>"; // Add the set point number as the second column
      response += "<td><input type=\"text\" name=\"v" + String(i) + String(j) + "\" value=\"" + String(set_points[i][j]) + "\"></td>"; // Add an input field for the battery voltage as the third column
      response += "<td><input type=\"text\" name=\"b" + String(i) + String(j) + "\" value=\"" + String(envelopes[i][j]) + "\"></td>"; // Add an input field for the LED brightness as the fourth column
      response += "</tr>"; // End the table row
    }
  }
  response += "</table><input type=\"submit\" value=\"Save\"></form></body></html>"; // End the table and add a submit button to save the configuration
  server.send(200, "text/html", response); // Send the response with status code 200 and content type text/html
}

// The function to handle POST requests for the config path
void handleConfig() {
  // Parse and save the configuration data from the request parameters
  for (int i = 0; i < NUM_ENVELOPES; i++) {
    for (int j = 0; j < NUM_SET_POINTS; j++) {
      String v_name = "v" + String(i) + String(j); // The name of the parameter for the battery voltage
      String b_name = "b" + String(i) + String(j); // The name of the parameter for the LED brightness
      if (server.hasArg(v_name) && server.hasArg(b_name)) {
        // The request has both parameters for this set point, parse and save them
        float v_value = server.arg(v_name).toFloat(); // Parse the battery voltage value as a float
        int b_value = server.arg(b_name).toInt(); // Parse the LED brightness value as an int
        set_points[i][j] = v_value; // Save the battery voltage value to the set points array
        envelopes[i][j] = b_value; // Save the LED brightness value to the envelopes array
        byte high_byte = (int)(v_value * 100) >> 8; // Get the high byte of the battery voltage value multiplied by 100
        byte low_byte = (int)(v_value * 100) & 0xFF; // Get the low byte of the battery voltage value multiplied by 100
        EEPROM.write(1 + i * NUM_STEPS + j, b_value); // Write one byte per step to EEPROM
        EEPROM.write(1 + NUM_ENVELOPES * NUM_STEPS + i * NUM_SET_POINTS * 2 + j * 2, high_byte); // Write one byte per high byte of set point to EEPROM
        EEPROM.write(1 + NUM_ENVELOPES * NUM_STEPS + i * NUM_SET_POINTS * 2 + j * 2 + 1, low_byte); // Write one byte per low byte of set point to EEPROM
      }
    }
  }
  EEPROM.commit(); // Commit the changes to EEPROM
  Serial.println("Configuration saved"); // Print a message to the serial monitor

  // Send a response with a link to restart the device
  server.send(200, "text/html", "<html><head><title>ESP8266 Motion Sensor Lamp Configuration</title></head><body><h1>Configuration saved</h1><p>Please <a href=\"/restart\">restart</a> the device to apply the changes.</p></body></html>"); // Send the response with status code 200 and content type text/html
}

// The function to handle GET requests for the restart path
void handleRestart() {
  // Send a response with a message and restart the device
  server.send(200, "text/html", "<html><head><title>ESP8266 Motion Sensor Lamp Configuration</title></head><body><h1>Restarting...</h1></body></html>"); // Send the response with status code 200 and content type text/html
  Serial.println("Restarting..."); // Print a message to the serial monitor
  delay(1000); // Wait for one second
  ESP.restart(); // Restart the device
}
