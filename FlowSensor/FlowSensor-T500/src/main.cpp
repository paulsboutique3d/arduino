#include <LiquidCrystal.h>
#include <AccelStepper.h>

// Initialize the LCD display
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

// Specify the pins for the indicator LED and flow rate adjustment buttons
byte statusLed = 13;
byte incrementButton = 3;
byte decrementButton = 4;

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
int stepsPerRevolution = 200;

// Set the threshold for turning on the stepper (adjust as needed)
const float flowThreshold = 10.0;


void pulseCounter() {
  pulseCount++;
}


void setup() {
  lcd.begin(16, 2);
  stepper.setMaxSpeed(1000);
  stepper.setAcceleration(500);
  stepper.setSpeed(500);

  pinMode(incrementButton, INPUT_PULLUP);
  pinMode(decrementButton, INPUT_PULLUP);

  Serial.begin(38400);
  pinMode(statusLed, OUTPUT);
  digitalWrite(statusLed, HIGH);
  pinMode(sensorPin, INPUT);
  digitalWrite(sensorPin, HIGH);

  pulseCount = 0;
  flowRate = 0.0;
  flowMilliLitres = 0;
  totalMilliLitresA = 0;
  totalMilliLitresB = 0;
  oldTime = 0;

  attachInterrupt(sensorInterrupt, pulseCounter, FALLING);
}

void loop() {
  if (digitalRead(incrementButton) == LOW) {
    calibrationFactor += 0.1;
  }

  if (digitalRead(decrementButton) == LOW) {
    calibrationFactor -= 0.1;
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

    if (flowRate > flowThreshold) {
      // Flow rate exceeds the threshold, decrease the flow by turning the stepper motor
      stepper.move(-100); // Adjust the number of steps as needed
      stepper.runToPosition();
    } else if (flowRate < -flowThreshold) {
      // Flow rate is below the negative threshold, increase the flow by turning the stepper motor
      stepper.move(100); // Adjust the number of steps as needed
      stepper.runToPosition();
    }
  }
}

