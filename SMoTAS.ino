// Smart Motion-Triggered Alarm System's Code
// Author: Luca Cabiddu - University of Florida

#include <LiquidCrystal.h>      // Include LCD library [1]

// Pins
const int tiltPin    = 2;     // Tilt switch input pin using INPUT_PULLUP [2]
const int buttonPin  = 3;     // Push button input pin using INPUT_PULLUP [3]
const int buzzerPin  = 8;     // Passive buzzer output pin [4]

const int ledRedPin   = 9;      // RGB LED red channel (PWM pin) [5]
const int ledGreenPin = 10;     // RGB LED green channel (PWM pin) [5]
const int ledBluePin  = 11;     // RGB LED blue channel (PWM pin) [5]

// LCD wiring (rs, en, d4, d5, d6, d7)
LiquidCrystal lcd(7, 6, 5, 4, 13, 12);      // LCD 4-bit wiring setup [1]

// States
enum SystemState { DISARMED, ARMED, ALARM };      // Define symbolic names for integer values (0,1,2) so code reads like English instead of numbers
SystemState currentState = DISARMED;      // Start in DISARMED so the system is safe and inactive on power-up
SystemState lastState    = DISARMED;      // Remember the previous state so the display/LED/buzzer update only when something changes

// Button edge detection
int lastButtonReading = HIGH;     // Store the previous loop's button value (HIGH = not pressed) to detect new presses [3]

// Prevents instant alarm right after arming
bool justArmed = false;     // Flag used to skip tilt detection immediately after arming, avoiding false alarm from button movement

// Helpers
void setRGB(uint8_t r, uint8_t g, uint8_t b) {      // Helper to change the RGB LED color, takes 3 bytes; compiler passes them via registers for speed [5]
  analogWrite(ledRedPin,   r);      // Writes PWM duty cycle to hardware timer controlling pin 9 (0 = off, 255 = full on)
  analogWrite(ledGreenPin, g);      // Same mechanism for green channel
  analogWrite(ledBluePin,  b);      // Same mechanism for blue channel
}

// Update LCD, LED, and buzzer according to state
void showState(SystemState s) {
  lcd.clear();      // Sends command byte to LCD to clear DDRAM memory, resetting cursor position [1]

  if (s == DISARMED) {
    lcd.setCursor(0, 0); lcd.print("Status: DISARMED");     // Print in the first line of the LCD
    lcd.setCursor(0, 1); lcd.print("Press to arm");     // Print in the second line of the LCD
    setRGB(0,0,255);      // PWM outputs set LED to pure blue, indicating the system is disarmed [5]
    noTone(buzzerPin);      // Calls tone library to stop generating waveforms on pin 8 [4]
  }
  else if (s == ARMED) {
    lcd.setCursor(0, 0); lcd.print("Status: ARMED");      // Same mechanism as before
    lcd.setCursor(0, 1); lcd.print("Tilt = ALARM");
    setRGB(0,255,0);      // Green indicates the system is armed and monitoring tilts [5]
    noTone(buzzerPin); 
  }
  else if (s == ALARM) {
    lcd.setCursor(0, 0); lcd.print("!!! ALARM !!!");      // Same mechanism here
    lcd.setCursor(0, 1); lcd.print("Press to disarm");
    setRGB(255,0,0);      // Red indicates the system has detected a tilt and started the alarm
    tone(buzzerPin, 1000);      // Hardware timer generates a 1kHz square wave on pin 8 to sound the alarm [4]
  }
}

// Setup
void setup() {
  pinMode(tiltPin,   INPUT_PULLUP);     // Configures internal pull-up transistor; pin reads HIGH until the tilt switch closes toward ground [2]
  pinMode(buttonPin, INPUT_PULLUP);     // Same idea: stable HIGH when unpressed, LOW when button completes circuit to ground [3]

  pinMode(buzzerPin, OUTPUT);     // Allows Arduino to actively drive the buzzer line HIGH/LOW or modulate with tone() [4]
  pinMode(ledRedPin, OUTPUT);     // Enable RGB LED control pins as outputs so PWM can drive them [5]
  pinMode(ledGreenPin, OUTPUT);
  pinMode(ledBluePin, OUTPUT);

  lcd.begin(16, 2);     // Send LCD initialization sequence: sets mode, clears memory, configures display geometry [1]

  showState(currentState);      // Immediately updates LCD and LED to visually confirm startup state
}

// Main loop
void loop() {

  // Button edge detection
  int reading = digitalRead(buttonPin);     // Reads logic level from pin register (HIGH = idle, LOW = pressed) [3]

  if (lastButtonReading == HIGH && reading == LOW) {      // Detects a HIGH to LOW transition (new press event rather than holding)
    if (currentState == DISARMED) {     // If user pressed button while system was off...
      currentState = ARMED;     // Then activate monitoring mode
      justArmed = true;     // Avoid triggering alarm during same loop's motion from pressing button
    } else {      // If system was already armed or alarming...
      currentState = DISARMED;      // Then reset to safe mode
      justArmed = false;      // No need to suppress tilt logic here
    }
  }

  lastButtonReading = reading;      // Saves value so next loop iteration can detect transitions

  // Tilt detection
  int tiltState = digitalRead(tiltPin);     // Reads mechanical tilt switch state; HIGH/LOW changes when ball shifts inside sensor [2]

  if (currentState == ARMED && !justArmed) {      // Only check sensor when system is armed and not in the arming loop
    if (tiltState == HIGH) {      // HIGH indicates switch moved, meaning device was tilted or disturbed
      currentState = ALARM;     // Trigger alarm response in state machine
    }
  }

  justArmed = false;      // Resets suppression so tilt detection is enabled next loop

  // Update outputs if state changed
  if (currentState != lastState) {      // Avoid unnecessary hardware writes unless state has changed
    showState(currentState);      // Pushes new state info to LCD, LED, buzzer
    lastState = currentState;     // Records new state
  }

  delay(50);      // Allows mechanical bounce to settle; stabilizes readings without complex debouncing [2]
}
