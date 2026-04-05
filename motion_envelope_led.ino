#include <Arduino.h>
#include <LowPower.h>

const int motionPin = 2;
const int ledPin = 9;
const int batteryPin = A0;

const int numEnvelopes = 5;
const int numSteps = 60;

struct Step {
  byte brightness;
  unsigned int duration;
};

struct Envelope {
  Step steps[numSteps];
  int loopPoint;
};

const Envelope envelopes[numEnvelopes] PROGMEM = {
  {
    {
      {25, 100}, {51, 100}, {76, 100}, {102, 100}, {127, 100},
      {153, 100}, {178, 100}, {204, 100}, {229, 100}, {255, 100},
      {255, 5000}, {229, 100}, {204, 100}, {178, 100}, {153, 100},
      {127, 100}, {102, 100}, {76, 100}, {51, 100}, {25, 100},
      {0, 100}, {0, 0}
    },
    10
  },
  {
    {
      {51, 100}, {102, 100}, {153, 100}, {204, 100}, {255, 100},
      {204, 100}, {153, 100}, {102, 100}, {51, 100}, {0, 100},
      {51, 100}, {102, 100}, {153, 100}, {204, 100}, {255, 100},
      {204, 100}, {153, 100}, {102, 100}, {51, 100}, {0, 100},
      {51, 100}, {102, 100}, {153, 100}, {204, 100}, {255, 100},
      {204, 100}, {153, 100}, {102, 100}, {51, 100}, {0, 100},
      {0, 0}
    },
    0
  },
  {
    {
      {255, 100}, {0, 100}, {255, 100}, {0, 700}, {255, 100},
      {0, 100}, {255, 100}, {0, 700}, {255, 100}, {0, 100},
      {255, 100}, {0, 700}, {255, 100}, {0, 100}, {255, 100},
      {0, 700}, {255, 100}, {0, 100}, {255, 100}, {0, 700},
      {255, 100}, {0, 100}, {255, 100}, {0, 700}, {255, 100},
      {0, 100}, {255, 100}, {0, 700}, {255, 100}, {0, 100},
      {255, 100}, {0, 700}, {255, 100}, {0, 100}, {255, 100},
      {0, 700}, {255, 100}, {0, 100}, {255, 100}, {0, 700},
      {0, 0}
    },
    0
  },
  {
    {
      {32, 30000}, {0, 0}
    },
    0
  },
  {
    {
      {255, 200}, {0, 200}, {255, 200}, {0, 200}, {255, 200},
      {0, 200}, {255, 200}, {0, 200}, {255, 200}, {0, 200},
      {255, 200}, {0, 200}, {255, 200}, {0, 200}, {255, 200},
      {0, 200}, {255, 200}, {0, 200}, {255, 200}, {0, 200},
      {255, 200}, {0, 200}, {255, 200}, {0, 200}, {255, 200},
      {0, 200}, {255, 200}, {0, 200}, {255, 200}, {0, 200},
      {255, 200}, {0, 200}, {255, 200}, {0, 200}, {255, 200},
      {0, 200}, {255, 200}, {0, 200}, {255, 200}, {0, 200},
      {255, 200}, {0, 200}, {255, 200}, {0, 200}, {255, 200},
      {0, 200}, {255, 200}, {0, 200}, {255, 200}, {0, 200},
      {255, 200}, {0, 200}, {255, 200}, {0, 200}, {255, 200},
      {0, 200}, {255, 200}, {0, 200}, {255, 200}, {0, 200}
    },
    0
  }
};

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
      int lp = (int)pgm_read_word(&envelopes[currentEnvelope].loopPoint);
      if (lp != -1 && lp < numSteps) executeStep(currentEnvelope, lp);
    }
  }

  if (currentEnvelope != -1) {
    Step s;
    memcpy_P(&s, &envelopes[currentEnvelope].steps[currentStep], sizeof(Step));
    if (millis() - stepStartTime >= s.duration) {
      int nextStep = currentStep + 1;
      Step nextS;
      bool hasNext = false;
      if (nextStep < numSteps) {
        memcpy_P(&nextS, &envelopes[currentEnvelope].steps[nextStep], sizeof(Step));
        if (nextS.duration > 0) hasNext = true;
      }

      if (!hasNext) {
        analogWrite(ledPin, 0);
        currentEnvelope = -1;
        currentStep = -1;
      } else {
        executeStep(currentEnvelope, nextStep);
      }
    }
  }
  if (currentEnvelope == -1) LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);
}
