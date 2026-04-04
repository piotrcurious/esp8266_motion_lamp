#include <Arduino.h>
#include <LowPower.h>

// Define the pins for the motion sensor and the LED
const int motionPin = 2; // The pin that can wake up Arduino from deep sleep
const int ledPin = 9; // The pin that controls the LED brightness
const int batteryPin = A0; // The pin that measures the battery voltage

// Define the number of envelopes and steps per envelope
const int numEnvelopes = 5;
const int numSteps = 60;

// Define the structure for each step
struct Step {
  byte brightness; // 0-255
  unsigned int duration; // ms
};

// Define the structure for each envelope
struct Envelope {
  Step steps[numSteps];
  int loopPoint; // step index to go back to if another motion event is detected
};

// Define the envelope data in PROGMEM
const Envelope envelopes[numEnvelopes] PROGMEM = {
  // Envelope 0: High battery (fade in and out)
  {
    {
      {0, 100}, {25, 100}, {50, 100}, {75, 100}, {100, 100},
      {125, 100}, {150, 100}, {175, 100}, {200, 100}, {225, 100},
      {255, 2000}, // stay on
      {225, 100}, {200, 100}, {175, 100}, {150, 100},
      {125, 100}, {100, 100}, {75, 100}, {50, 100}, {25, 100}, {0, 0}
    },
    10 // loop point (the step with 255 brightness)
  },
  // Envelope 1
  {{{0,0}}, -1},
  // Envelope 2
  {{{0,0}}, -1},
  // Envelope 3
  {{{0,0}}, -1},
  // Envelope 4: Low battery
  {{{0,0}}, -1}
};

// Define global variables
volatile bool motionDetected = false;
int currentEnvelope = -1;
int currentStep = -1;
unsigned long stepStartTime = 0;

void motionISR() {
  motionDetected = true;
}

int getEnvelopeIndex() {
  int analogValue = analogRead(batteryPin);
  float voltage = analogValue * (5.0 / 1023.0);
  
  if (voltage > 4.2) return 0;
  if (voltage > 3.9) return 1;
  if (voltage > 3.6) return 2;
  if (voltage > 3.3) return 3;
  return 4;
}

void executeStep(int envIdx, int stepIdx) {
  if (envIdx < 0 || envIdx >= numEnvelopes || stepIdx < 0 || stepIdx >= numSteps) return;

  Step s;
  memcpy_P(&s, &envelopes[envIdx].steps[stepIdx], sizeof(Step));

  analogWrite(ledPin, s.brightness);
  stepStartTime = millis();
  currentEnvelope = envIdx;
  currentStep = stepIdx;
}

void setup() {
  pinMode(motionPin, INPUT_PULLUP);
  pinMode(ledPin, OUTPUT);
  attachInterrupt(digitalPinToInterrupt(motionPin), motionISR, FALLING);
}

void loop() {
  if (motionDetected) {
    motionDetected = false;
    if (currentEnvelope == -1) {
      executeStep(getEnvelopeIndex(), 0);
    } else {
      int loopPoint = (int)pgm_read_word(&envelopes[currentEnvelope].loopPoint);
      if (loopPoint != -1) {
        executeStep(currentEnvelope, loopPoint);
      }
    }
  }

  if (currentEnvelope != -1) {
    Step s;
    memcpy_P(&s, &envelopes[currentEnvelope].steps[currentStep], sizeof(Step));
    
    if (s.duration == 0 || (millis() - stepStartTime >= s.duration)) {
      int nextStep = currentStep + 1;
      Step nextS;
      if (nextStep < numSteps) {
        memcpy_P(&nextS, &envelopes[currentEnvelope].steps[nextStep], sizeof(Step));
      }

      if (nextStep >= numSteps || (nextS.duration == 0 && nextS.brightness == 0)) {
        analogWrite(ledPin, 0);
        currentEnvelope = -1;
        currentStep = -1;
      } else {
        executeStep(currentEnvelope, nextStep);
      }
    }
  }

  if (currentEnvelope == -1) {
    LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);
  }
}
