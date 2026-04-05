#include <Arduino.h>

#define PIR_PIN 2
#define LED_PIN 9
#define BATT_PIN A0

#define NUM_ENVELOPES 5
#define NUM_STEPS 60

#define MIN_BATT_VAL 600
#define MAX_BATT_VAL 900

struct Step {
  byte brightness;
  unsigned int duration;
};

struct Envelope {
  Step steps[NUM_STEPS];
  int loopPoint;
};

const Envelope envelopes[NUM_ENVELOPES] PROGMEM = {
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
int currentStepIdx = -1;
unsigned long stepStartTime = 0;

void motionISR() {
   motionDetected = true;
}

void executeStep(int envIdx, int stepIdx) {
   if (envIdx < 0 || envIdx >= NUM_ENVELOPES || stepIdx < 0 || stepIdx >= NUM_STEPS) return;
   Step s;
   memcpy_P(&s, &envelopes[envIdx].steps[stepIdx], sizeof(Step));
   analogWrite(LED_PIN, s.brightness);
   stepStartTime = millis();
   currentEnvelope = envIdx;
   currentStepIdx = stepIdx;
}

void goToDeepSleep() {
   ESP.deepSleep(0);
}

void setup() {
   analogWriteRange(255);
   pinMode(PIR_PIN , INPUT_PULLUP);
   pinMode(LED_PIN , OUTPUT);
   pinMode(BATT_PIN , INPUT);

   // Handle wake-up if PIR is already HIGH
   if (digitalRead(PIR_PIN) == HIGH) {
      motionDetected = true;
   }

   attachInterrupt(digitalPinToInterrupt(PIR_PIN), motionISR , RISING);
}

void loop() {
   if (motionDetected) {
      motionDetected = false;
      int batteryValue = analogRead(BATT_PIN);
      int envIdx = map(batteryValue , MAX_BATT_VAL , MIN_BATT_VAL , 0 , NUM_ENVELOPES-1);
      envIdx = constrain(envIdx, 0, NUM_ENVELOPES - 1);
      if (currentEnvelope == -1) {
         executeStep(envIdx, 0);
      } else {
         int lp = (int)pgm_read_word(&envelopes[currentEnvelope].loopPoint);
         if (lp != -1 && lp < NUM_STEPS) executeStep(currentEnvelope, lp);
      }
   }

   if (currentEnvelope != -1) {
      Step s;
      memcpy_P(&s, &envelopes[currentEnvelope].steps[currentStepIdx], sizeof(Step));
      if (millis() - stepStartTime >= s.duration) {
         int nextStep = currentStepIdx + 1;
         Step nextS;
         bool hasNext = false;
         if (nextStep < NUM_STEPS) {
            memcpy_P(&nextS, &envelopes[currentEnvelope].steps[nextStep], sizeof(Step));
            if (nextS.duration > 0) hasNext = true;
         }
         if (!hasNext) {
            analogWrite(LED_PIN, 0);
            currentEnvelope = -1;
            currentStepIdx = -1;
            goToDeepSleep();
         } else {
            executeStep(currentEnvelope, nextStep);
         }
      }
   } else {
      goToDeepSleep();
   }
}
