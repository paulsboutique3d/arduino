#include <Arduino.h>
#include <LiquidCrystal.h>
#include <AccelStepper.h> // Include the AccelStepper library

// Initialize the LCD display
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

// Specify the pins for the indicator LED and flow rate adjustment buttons
byte statusLed = 13;
byte incrementButton = 3;
byte decrementButton = 4; // Add a new button for decrement

byte sensorInterrupt = 0; // 0 = pin 2; 1 = pin 3
byte sensorPin = 2;

// The hall-effect flow sensor outputs approximately 4.5 pulses per second per
// litre/minute of flow.
float calibrationFactor = 4.5;

volatile byte pulseCount;

float flowRate;
unsigned int flowMilliLitres;
unsigned long totalMilliLitresA;
unsigned long totalMilliLitresB;

unsigned long oldTime;

// Initialize the A4988 stepper motor driver
AccelStepper stepper(1, 10, 11); // (driver mode, step pin, direction pin)
int stepsPerRevolution = 200;  // Number of steps per revolution for your stepper motor


void pulseCounter() {
  pulseCount++;
}

void setup() {
  lcd.begin(16, 2);

  // Initialize the A4988 stepper motor driver
  stepper.setMaxSpeed(1000);    // Set the maximum speed in steps per second
  stepper.setAcceleration(500); // Set the acceleration in steps per second per second
  stepper.setSpeed(500);        // Set the initial speed

  // Set up the flow rate adjustment buttons
  pinMode(incrementButton, INPUT_PULLUP);
  pinMode(decrementButton, INPUT_PULLUP);

  // Initialize a serial connection for reporting values to the host
  Serial.begin(38400);

  // Set up the status LED line as an output
  pinMode(statusLed, OUTPUT);
  digitalWrite(statusLed, HIGH);  // We have an active-low LED attached

  pinMode(sensorPin, INPUT);
  digitalWrite(sensorPin, HIGH);

  pulseCount        = 0;
  flowRate          = 0.0;
  flowMilliLitres   = 0;
  totalMilliLitresA = 0;
  totalMilliLitresB = 0;
  oldTime           = 0;

  // The Hall-effect sensor is connected to pin 2 which uses interrupt 0.
  // Configured to trigger on a FALLING state change (transition from HIGH
  // state to LOW state)
  attachInterrupt(sensorInterrupt, pulseCounter, FALLING);
}

void loop() {
  if (digitalRead(incrementButton) == LOW) {
    calibrationFactor += 0.1; // Adjust the increment as needed
  }

  if (digitalRead(decrementButton) == LOW) {
    calibrationFactor -= 0.1; // Adjust the decrement as needed
  }

  if ((millis() - oldTime) > 1000) {
    detachInterrupt(sensorInterrupt);

    flowRate = ((1000.0 / (millis() - oldTime)) * pulseCount) / calibrationFactor;
    oldTime = millis();

    flowMilliLitres = (flowRate / 60) * 1000;
    totalMilliLitresA += flowMilliLitres;
    totalMilliLitresB += flowMilliLitres;

    unsigned int frac;
    Serial.print(int(flowRate));
    Serial.print(".");
    frac = (flowRate - int(flowRate)) * 10;
    Serial.print(frac, DEC);
    Serial.print(" ");
    Serial.print(flowMilliLitres);
    Serial.print(" ");
    Serial.print(totalMilliLitresA);
    Serial.print(" ");
    Serial.println(totalMilliLitresB);

    lcd.setCursor(0, 0);
    lcd.print("                ");
    lcd.setCursor(0, 0);
    lcd.print("Flow: ");
    if (int(flowRate) < 10) {
      lcd.print(" ");
    }
    lcd.print(int(flowRate));
    lcd.print('.');
    lcd.print(frac, DEC);
    lcd.print(" L");
    lcd.print("/min");

    lcd.setCursor(0, 1);
    lcd.print(int(totalMilliLitresA / 1000));
    lcd.print("L");
    lcd.setCursor(8, 1);
    lcd.print(int(totalMilliLitresB / 1000));
    lcd.print("L");

    pulseCount = 0;

    attachInterrupt(sensorInterrupt, pulseCounter, FALLING);
  }

  if (flowRate > 5.0) {
    // Rotate the stepper motor a specific number of steps
    stepper.move(stepsPerRevolution / 2); // Half a revolution in this example
    stepper.runToPosition(); // Move the motor
  }
}


