#include <Arduino.h>

#define MOTION_PIN 2
#define LED_PIN 9
#define BATTERY_PIN A0

#define NUM_ENVELOPES 5
#define NUM_STEPS 60

#define MIN_BATTERY_VALUE 600
#define MAX_BATTERY_VALUE 900

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
      {0, 100}, {0, 0}, {0, 0}, {0, 0}, {0, 0},
      {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0},
      {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0},
      {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0},
      {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0},
      {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0},
      {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0},
      {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}
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
      {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0},
      {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0},
      {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0},
      {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0},
      {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0},
      {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}
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
      {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0},
      {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0},
      {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0},
      {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}
    },
    0
  },
  {
    {
      {32, 30000}, {0, 0}, {0, 0}, {0, 0}, {0, 0},
      {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0},
      {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0},
      {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0},
      {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0},
      {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0},
      {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0},
      {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0},
      {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0},
      {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0},
      {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0},
      {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}
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

void goToDeepSleep() {
   cli();
   set_sleep_mode(SLEEP_MODE_PWR_DOWN);
   sleep_enable();
   sleep_bod_disable();
   sei();
   sleep_cpu();
   sleep_disable();
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

void setup() {
   pinMode(MOTION_PIN , INPUT_PULLUP);
   pinMode(LED_PIN , OUTPUT);
   pinMode(BATTERY_PIN , INPUT);
   attachInterrupt(digitalPinToInterrupt(MOTION_PIN), motionISR , RISING);
}

void loop() {
   if (motionDetected) {
      motionDetected = false;
      int batteryValue = analogRead(BATTERY_PIN);
      int envIdx = map(batteryValue , MIN_BATTERY_VALUE , MAX_BATTERY_VALUE , NUM_ENVELOPES-1 , 0);
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
      if (s.duration == 0 || (millis() - stepStartTime >= s.duration)) {
         int nextStep = currentStepIdx + 1;
         Step nextS;
         if (nextStep < NUM_STEPS) memcpy_P(&nextS, &envelopes[currentEnvelope].steps[nextStep], sizeof(Step));
         if (nextStep >= NUM_STEPS || (nextS.duration == 0 && nextS.brightness == 0)) {
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
