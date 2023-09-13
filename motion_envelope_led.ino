// Define the pins for the motion sensor and the LED
const int motionPin = 2; // The pin that can wake up Arduino from deep sleep
const int ledPin = 9; // The pin that controls the LED brightness
const int batteryPin = A0; // The pin that measures the battery voltage

// Define the number of envelopes and steps per envelope
const int numEnvelopes = 5;
const int numSteps = 60;

// Define the envelope data as a 3D array
// Each envelope is a 2D array of [brightness, duration, loop] values for each step
// The loop value indicates the step to go back to if another motion event is detected
// The last step of each envelope should have a duration of 0 to indicate the end
int envelopeData[numEnvelopes][numSteps][3] = {
  // Envelope 1: A simple fade in and out
  {{0, 100, 0}, {10, 100, 0}, {20, 100, 0}, {30, 100, 0}, {40, 100, 0}, {50, 100, 0}, {60, 100, 0}, {70, 100, 0}, {80, 100, 0}, {90, 100, 0}, {100, 1000, 10}, {90, 100, 10}, {80, 100, 10}, {70, 100, 10}, {60, 100, 10}, {50, 100, 10}, {40, 100, 10}, {30, 100, 10}, {20, 100, 10}, {10, 100, 10}, {0, 0, -1}},
  
  // Envelope 2: A pulsing effect
  {{0, 50, -1}, {50, 50, -1}, {100, 50,-1}, {50 ,50 ,-1}},
  
   // Envelope ... (add more envelopes as desired)
};

// Define a variable to store the current envelope index
int currentEnvelope = -1;

// Define a variable to store the current step index
int currentStep = -1;

// Define a variable to store the start time of the current step
unsigned long startTime = -1;

// Define a variable to store the battery voltage level
float batteryLevel = -1;

// Define a function to read the battery voltage and map it to an envelope index
int getEnvelopeIndex() {
  
   // Read the analog value from the battery pin and convert it to voltage
   int analogValue = analogRead(batteryPin);
   float voltage = analogValue * (5.0 /1023.0);
   
   // Map the voltage to an envelope index based on some criteria
   // For example: if voltage is less than or equal to X volts then use envelope Y
   // Adjust the values as needed
   
   if (voltage <=3.2) {
     return numEnvelopes -1; // Use the last envelope if voltage is too low
   }
   else if (voltage <=3.6) {
     return numEnvelopes -2; // Use the second last envelope if voltage is low
   }
   else if (voltage <=4.0) {
     return numEnvelopes -3; // Use the third last envelope if voltage is medium
   }
   else if (voltage <=4.4) {
     return numEnvelopes -4; // Use the fourth last envelope if voltage is high
   }
   else {
     return numEnvelopes -5; // Use the first envelope if voltage is very high
   }
}

// Define a function to execute a step of an envelope
void executeStep(int envelopeIndex , int stepIndex) {
  
    // Get the brightness and duration values from the envelope data array
    int brightness = envelopeData[envelopeIndex][stepIndex][0];
    int duration = envelopeData[envelopeIndex][stepIndex][1];
    
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
  
    // Get the duration value from the envelope data array
    int duration = envelopeData[currentEnvelope][currentStep][1];
    
    // Check if the duration is zero or not
    if (duration ==0) {
      return true; // The step is finished if the duration is zero
    }
    
    // Check if the elapsed time since the start of the step is greater than or equal to the duration
    unsigned long elapsedTime = millis() - startTime;
    if (elapsedTime >= duration) {
      return true; // The step is finished if the elapsed time is greater than or equal to the duration
    }
    
    // Otherwise, the step is not finished
    return false;
}

// Define a function to get the next step index of an envelope
int getNextStepIndex(int envelopeIndex , int stepIndex) {
  
    // Get the loop value from the envelope data array
    int loop = envelopeData[envelopeIndex][stepIndex][2];
    
    // Check if the loop value is negative or not
    if (loop <0) {
      return -1; // There is no next step if the loop value is negative
    }
    
    // Check if the loop value is zero or not
    if (loop ==0) {
      return stepIndex +1; // The next step is the next index if the loop value is zero
    }
    
    // Otherwise, the next step is the loop value
    return loop;
}

// Define a function to handle a motion event
void handleMotionEvent() {
  
   // Read the battery voltage and get the corresponding envelope index
   int envelopeIndex = getEnvelopeIndex();
   
   // Execute the first step of the envelope
   executeStep(envelopeIndex , 0);
}

// Define a function to handle a timer event
void handleTimerEvent() {
  
   // Check if there is a current envelope and step
   if (currentEnvelope >=0 && currentStep >=0) {
     
     // Check if the current step is finished or not
     if (isStepFinished()) {
       
       // Get the next step index of the current envelope
       int nextStepIndex = getNextStepIndex(currentEnvelope , currentStep);
       
       // Check if there is a next step or not
       if (nextStepIndex >=0) {
         
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
  
  // Handle a timer event every 10 milliseconds
  handleTimerEvent();
  
  // Put Arduino into deep sleep mode to save power
  LowPower.powerDown(SLEEP_FOREVER , ADC_OFF , BOD_OFF);
}
