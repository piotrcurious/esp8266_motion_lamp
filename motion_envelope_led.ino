// Define the pins for the motion sensor and the LED
const int motionPin = 2; // The pin that can wake up Arduino from deep sleep
const int ledPin = 9; // The pin that controls the LED brightness
const int batteryPin = A0; // The pin that measures the battery voltage

// Define the number of envelopes and steps per envelope
const int numEnvelopes = 5;
const int numSteps = 60;

// Define the structure for each step
struct Step {
  int brightness; // 0-255
  int duration;   // ms
  int loop;       // next step index if motion detected, -1 if no loop
};

// Define the envelope data as a 3D array or array of structs
// To stay close to original intent, I'll use a struct for clarity but keep it in PROGMEM
Step envelopeData[numEnvelopes][numSteps] = {
  // Envelope 1: A simple fade in and out
  {
    {0, 100, 0}, {25, 100, 0}, {50, 100, 0}, {75, 100, 0}, {100, 100, 0},
    {125, 100, 0}, {150, 100, 0}, {175, 100, 0}, {200, 100, 0}, {225, 100, 0},
    {255, 1000, 10}, {225, 100, 10}, {200, 100, 10}, {175, 100, 10}, {150, 100, 10},
    {125, 100, 10}, {100, 100, 10}, {75, 100, 10}, {50, 100, 10}, {25, 100, 10}, {0, 0, -1}
  },
  
  // Envelope 2: A pulsing effect
  {
    {0, 50, -1}, {50, 50, -1}, {100, 50,-1}, {50 ,50 ,-1}, {0, 0, -1}
  },
  
  // Fill others with default
  {{0,0,-1}}, {{0,0,-1}}, {{0,0,-1}}
};

// Define a variable to store the current envelope index
int currentEnvelope = -1;

// Define a variable to store the current step index
int currentStep = -1;

// Define a variable to store the start time of the current step
unsigned long startTime = 0;

// Define a function to read the battery voltage and map it to an envelope index
int getEnvelopeIndex() {
  
   // Read the analog value from the battery pin and convert it to voltage
   int analogValue = analogRead(batteryPin);
   float voltage = analogValue * (5.0 / 1023.0);
   
   if (voltage <= 3.2) return 4;
   if (voltage <= 3.6) return 3;
   if (voltage <= 4.0) return 2;
   if (voltage <= 4.4) return 1;
   return 0;
}

// Define a function to execute a step of an envelope
void executeStep(int envelopeIndex , int stepIndex) {
    if (envelopeIndex < 0 || envelopeIndex >= numEnvelopes || stepIndex < 0 || stepIndex >= numSteps) return;

    // Get the brightness and duration values from the envelope data array
    int brightness = envelopeData[envelopeIndex][stepIndex].brightness;
    
    // Set the LED brightness using analogWrite
    analogWrite(ledPin , brightness);
    
    // Set the start time of the step using millis()
    startTime = millis();
    
    // Set the current envelope and step index variables
    currentEnvelope = envelopeIndex;
    currentStep = stepIndex;
}

// Define a function to check if a step is finished or not
bool isStepFinished() {
    if (currentEnvelope < 0) return true;

    // Get the duration value from the envelope data array
    int duration = envelopeData[currentEnvelope][currentStep].duration;
    
    // Check if the duration is zero or not
    if (duration == 0) {
      return true; // The step is finished if the duration is zero
    }
    
    // Check if the elapsed time since the start of the step is greater than or equal to the duration
    unsigned long elapsedTime = millis() - startTime;
    if (elapsedTime >= (unsigned long)duration) {
      return true; // The step is finished if the elapsed time is greater than or equal to the duration
    }
    
    // Otherwise, the step is not finished
    return false;
}

// Define a function to get the next step index of an envelope
int getNextStepIndex(int envelopeIndex , int stepIndex) {
    // Check if the next step is within bounds
    if (stepIndex + 1 >= numSteps) return -1;
    
    // If duration of next step is 0 and it's not the first step, it's likely an end marker
    if (envelopeData[envelopeIndex][stepIndex + 1].duration == 0 && envelopeData[envelopeIndex][stepIndex + 1].brightness == 0) {
        return -1;
    }

    return stepIndex + 1;
}

// Define a function to handle a motion event
void handleMotionEvent() {
   if (currentEnvelope == -1) {
       // Read the battery voltage and get the corresponding envelope index
       int envelopeIndex = getEnvelopeIndex();
       // Execute the first step of the envelope
       executeStep(envelopeIndex , 0);
   } else {
       // If already running, check for loop point
       int loopStep = envelopeData[currentEnvelope][currentStep].loop;
       if (loopStep != -1) {
           executeStep(currentEnvelope, loopStep);
       }
   }
}

// Define a function to handle a timer event
void handleTimerEvent() {
   // Check if there is a current envelope and step
   if (currentEnvelope >= 0 && currentStep >= 0) {
     
     // Check if the current step is finished or not
     if (isStepFinished()) {
       
       // Get the next step index of the current envelope
       int nextStepIndex = getNextStepIndex(currentEnvelope , currentStep);
       
       // Check if there is a next step or not
       if (nextStepIndex >= 0) {
         // Execute the next step of the current envelope
         executeStep(currentEnvelope , nextStepIndex);
       }
       else {
         // Turn off the LED and reset the current envelope and step variables
         analogWrite(ledPin , 0);
         currentEnvelope = -1;
         currentStep = -1;
       }
     }
   }
}

// Define a function to set up the Arduino
void setup() {
  // Set the motion pin as an input with an internal pull-up resistor
  pinMode(motionPin , INPUT_PULLUP);
  
  // Set the LED pin as an output
  pinMode(ledPin , OUTPUT);
  
  // Set up an interrupt to wake up Arduino from deep sleep when motion is detected
  attachInterrupt(digitalPinToInterrupt(motionPin) , handleMotionEvent , FALLING);
}

// Define a function to loop the Arduino
void loop() {
  
  // Handle a timer event
  handleTimerEvent();
  
  if (currentEnvelope == -1) {
      // Put Arduino into deep sleep mode to save power only when no envelope is running
      // Note: In real Arduino, handleTimerEvent wouldn't be called if we are in deep sleep.
      // The logic should be: if no envelope is running, go to sleep.
      // Wake up is handled by interrupt.
      LowPower.powerDown(SLEEP_FOREVER , ADC_OFF , BOD_OFF);
  } else {
      // Small delay to prevent busy looping in simulation
      delay(10);
  }
}
