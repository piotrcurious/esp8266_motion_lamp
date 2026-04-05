# Motion Activated LED Lamp (ESP8266 & Arduino)

A sophisticated, energy-efficient motion sensor lamp firmware featuring configurable light envelopes, battery monitoring, and a retro-tech web interface.

![Web UI Preview](web_ui_screenshot.png)

## Features
- **5 Distinct Light Envelopes**: Fully configurable sequences with 60 steps each (brightness & duration).
- **Adaptive Battery Logic**: Automatically selects light profiles based on lithium-ion battery voltage.
- **Responsive Re-triggering**: Non-blocking state machine with edge-detection allows envelopes to restart at a defined `loopPoint` if motion is detected during execution.
- **Deep Sleep Support**: Maximizes battery life by utilizing low-power modes between events.
- **Motion Lamp OS Web UI**: Technical dashboard for real-time configuration, live visualization of light profiles, and integrated testing.
- **Keyboard Editor**: Interactive keyboard-based envelope editing for rapid adjustments.
- **Force-Config Mode**: Dedicated boot-pin trigger for reliable access to configuration settings.

---

## Hardware Setup

### Wiring Diagram (Typical ESP8266)
| Component | Pin | Notes |
| :--- | :--- | :--- |
| PIR Sensor | **D0 (GPIO16)** | Must be connected to **RST** via diode/resistor for wake-up. |
| LED (PWM) | **D1 (GPIO5)** | Use a MOSFET/Transistor for high-power LEDs. |
| Config Button | **D3 (GPIO0)** | Pulls to GND during boot to force config mode. |
| Battery Sense| **A0** | Connected to Li-Ion via voltage divider. |

### Calibration
Measure your battery voltage with a multimeter and compare it to the `BATTERY` on the Dashboard. Adjust the `V_MULT` (Voltage Multiplier) in the web interface until they match.

---

## Web UI & Keyboard Editor

The Technical Dashboard allows for advanced control over the lamp's behavior.

### Keyboard Shortcuts
To use the keyboard editor, click on any **Brightness Profile** in the table:
- **Left / Right Arrow**: Choose the step to edit (indicated by a red highlight).
- **Up / Down Arrow**: Increase or decrease brightness at the selected step (15 units per press).

---

## Usage Instructions

### 1. Initial Configuration
On the first boot, the device will start a WiFi Access Point:
- **SSID**: `MotionLampAP`
- **Password**: `12345678`
- **IP**: `192.168.4.1`

Navigate to `http://192.168.4.1` to access the technical dashboard.

### 2. Entering Configuration Mode
If the device is already configured and in normal operation:
1. Hold the **Config Button** (D3/GPIO0) down.
2. Power on or Reset the device.
3. Keep holding for **5 seconds**.
4. The LED will signal entry, and the Access Point will start.

### 3. Testing Envelopes
Click the **RUN** button in any row to immediately execute that light envelope for testing purposes.

---

## Code Outline

### State Machine Logic
All versions utilize a non-blocking state machine to handle timing without using `delay()`. This ensures the system remains responsive to the PIR sensor and web requests.

```cpp
// Logic Flow:
if (motionDetected) {
    if (currentEnvelope == -1) {
        startNewEnvelope(batteryMapping());
    } else {
        jumpToLoopPoint(); // Edge detected
    }
}

if (stepTimeExpired()) {
    executeNextStep();
}
```

---

## Testing
The firmware includes a complete **C++ Mock Environment** in the `tests/` directory. You can verify the logic on a PC using `g++`:
```bash
g++ -Itests tests/Arduino.cpp web_pid_led.ino tests/test_web_pid.cpp -o test_runner
./test_runner
```
