// Define the pins for the motion sensor and the LED
#define MOTION_PIN 2 // This pin can wake up ESP8266 from deep sleep
#define LED_PIN 9 // This pin can control the LED brightness with PWM

// Define the analog pin for measuring the battery voltage
#define BATTERY_PIN A0 // This pin is connected to a voltage divider

// Define the number of envelopes and steps
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
  // Envelope 0: A simple fade in and out
  {
    {{0, 100}, {10, 100}, {20, 100}, {30, 100}, {40, 100}, {50, 100}, {60, 100}, {70, 100}, {80, 100}, {90, 100},
     {100, 100}, {110, 100}, {120, 100}, {130, 100}, {140, 100}, {150, 100}, {160, 100}, {170, 100}, {180, 100}, {190, 100},
     {200, 100}, {210, 100}, {220, 100}, {230, 100}, {240, 100}, {250, 100}, {255, 5000}, // Fade in and stay on for 5 seconds
     {250, 100}, {240, 100}, {230, 100}, {220, 100}, {210, 100}, {200, 100}, {190, 100}, {180, 100}, {170, 100}, {160, 100},
     {150, 100}, {140, 100}, {130, 100}, {120, 100}, {110, 100}, {100, 100}, {90, 100}, {80, 100}, {70, 100}, {60, 100},
     {50, 100}, {40, 100}, {30, 100}, {20, 100}, {10, 100}, {0, 0}}, // Fade out and end the envelope
    -1 // No loop point for this envelope
   },
   // Fill others
   {{{0,0}}, -1}, {{{0,0}}, -1}, {{{0,0}}, -1}, {{{0,0}}, -1}
};

// Declare some global variables
volatile bool motionDetected = false; // A flag to indicate if motion is detected
int currentEnvelope = -1; // The index of the current envelope being executed
int currentStepIdx = -1; // Renamed to avoid shadow

// The interrupt service routine for the motion pin
void motionISR() {
   // Set the flag to indicate motion is detected
   motionDetected = true;
}

// The setup function runs once when you press reset or power the board
void setup() {
  
   // Initialize the serial monitor for debugging (optional)
   Serial.begin(9600);

   // Set the motion pin as input with pull-up resistor
   pinMode(MOTION_PIN , INPUT_PULLUP);

   // Set the LED pin as output
   pinMode(LED_PIN , OUTPUT);

   // Set the battery pin as input
   pinMode(BATTERY_PIN , INPUT);

   // Attach an interrupt to the motion pin to detect rising edge (motion detected)
   attachInterrupt(digitalPinToInterrupt(MOTION_PIN), motionISR , RISING);
}

// The function to go to deep sleep mode
void goToDeepSleep() {
   // Use ESP.deepSleep function to put ESP8266 in deep sleep mode
   ESP.deepSleep(0); // Sleep indefinitely until RST pin is pulled low
}

// The loop function runs over and over again forever
void loop() {
  
   // Check if motion is detected
   if (motionDetected) {
      // Reset the flag
      motionDetected = false;

      // Read the battery voltage from the analog pin
      int batteryValue = analogRead(BATTERY_PIN);

      // Map the battery value to an envelope index from [0..NUM_ENVELOPES-1]
      int envelopeIndex = map(batteryValue , MAX_BATTERY_VALUE , MIN_BATTERY_VALUE , 0 , NUM_ENVELOPES-1);

      // Constrain the envelope index to [0..NUM_ENVELOPES-1]
      envelopeIndex = constrain(envelopeIndex , 0 , NUM_ENVELOPES-1);
     
      // If the envelope index is different from the current one,
      if (envelopeIndex != currentEnvelope) {
         // Switch to the new envelope and start from the first step
         currentEnvelope = envelopeIndex;
         currentStepIdx = 0;
      }
      else {
         // If the current envelope has a loop point,
         if (envelopes[currentEnvelope].loopPoint != -1) {
            // Go back to the loop point
            currentStepIdx = envelopes[currentEnvelope].loopPoint;
         }
      }
   }

   // If there is a current envelope and step,
   if (currentEnvelope != -1 && currentStepIdx != -1) {
      // Get the current step from PROGMEM
      Step s = envelopes[currentEnvelope].steps[currentStepIdx];

      // Set the LED brightness according to the current step
      analogWrite(LED_PIN , s.brightness);

      // Wait for the duration of the current step
      if (s.duration > 0) {
          delay(s.duration);
      }

      // Increment the current step
      currentStepIdx++;

      // If the current step is beyond the last step, or duration is 0
      if (currentStepIdx >= NUM_STEPS || (currentStepIdx > 0 && s.duration == 0)) {
         // Reset the current envelope and step
         currentEnvelope = -1;
         currentStepIdx = -1;

         // Turn off the LED
         analogWrite(LED_PIN , 0);

         // Go to deep sleep mode to save power
         goToDeepSleep();
      }
   } else {
       goToDeepSleep();
   }
}
