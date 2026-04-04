#include <Arduino.h>

#define MOTION_PIN 2 // This pin can wake up ESP8266 from deep sleep
#define LED_PIN 9 // This pin can control the LED brightness with PWM
#define BATTERY_PIN A0 // This pin is connected to a voltage divider

#define NUM_ENVELOPES 5 // There are five envelopes
#define NUM_STEPS 60 // Each envelope has 60 steps

// Battery voltage range (analog readings)
#define MIN_BATTERY_VALUE 600 // ~3.0V
#define MAX_BATTERY_VALUE 900 // ~4.5V

// Define the structure for each step
struct Step {
  byte brightness; // The LED brightness from 0 to 255
  unsigned int duration; // The duration of the step in milliseconds
};

// Define the structure for each envelope
struct Envelope {
  Step steps[NUM_STEPS]; // The array of steps
  int loopPoint; // The index of the step to loop back to if motion is detected again, -1 if none
};

// Declare the envelopes as constants in PROGMEM
const Envelope envelopes[NUM_ENVELOPES] PROGMEM = {
  // Envelope 0: High battery (fade in and out)
  {
    {
      {0, 100}, {50, 100}, {100, 100}, {150, 100}, {200, 100}, {255, 1000},
      {200, 100}, {150, 100}, {100, 100}, {50, 100}, {0, 0}
    },
    5 // Loop point at 255 brightness step
  },
  // Fill others with empty
  {{{0,0}}, -1}, {{{0,0}}, -1}, {{{0,0}}, -1}, {{{0,0}}, -1}
};

volatile bool motionDetected = false;
int currentEnvelope = -1;
int currentStepIdx = -1;
unsigned long stepStartTime = 0;

void motionISR() {
   motionDetected = true;
}

void goToDeepSleep() {
   ESP.deepSleep(0); // Indefinite sleep
}

void setup() {
   Serial.begin(9600);
   pinMode(MOTION_PIN , INPUT_PULLUP);
   pinMode(LED_PIN , OUTPUT);
   pinMode(BATTERY_PIN , INPUT);
   attachInterrupt(digitalPinToInterrupt(MOTION_PIN), motionISR , RISING);
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

void loop() {
   if (motionDetected) {
      motionDetected = false;
      int batteryValue = analogRead(BATTERY_PIN);
      int envIdx = map(batteryValue , MAX_BATTERY_VALUE , MIN_BATTERY_VALUE , 0 , NUM_ENVELOPES-1);
      envIdx = constrain(envIdx, 0, NUM_ENVELOPES - 1);

      if (currentEnvelope == -1) {
         executeStep(envIdx, 0);
      } else {
         int loopPoint = (int)pgm_read_word(&envelopes[currentEnvelope].loopPoint);
         if (loopPoint != -1) {
            executeStep(currentEnvelope, loopPoint);
         }
      }
   }

   if (currentEnvelope != -1) {
      Step s;
      memcpy_P(&s, &envelopes[currentEnvelope].steps[currentStepIdx], sizeof(Step));

      if (s.duration == 0 || (millis() - stepStartTime >= s.duration)) {
         int nextStep = currentStepIdx + 1;
         Step nextS;
         if (nextStep < NUM_STEPS) {
            memcpy_P(&nextS, &envelopes[currentEnvelope].steps[nextStep], sizeof(Step));
         }

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
