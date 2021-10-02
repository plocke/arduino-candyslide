
#include <Arduino.h>
#include <Wire.h>
#include <Servo.h>

Servo myservo; // create servo object to control a servo

int candyButton = 2;
int slideLightPin = 7;
int candyLightPin = 8;
int servoPin = 9;

int currentLightState = HIGH;
int currentCandyButtonState = LOW;
unsigned long candyLightPulseLengthMs = 1000;
unsigned long activateLightsPulseLengthMs = 75;
unsigned long lastPulseTime = 0;
unsigned long lastCandyButtonStateChange = 0;
int debounceTimeMillis = 100;
int flashCountOnCandy=4;
void setup()
{
  Serial.begin(9600);
  myservo.attach(servoPin); // attaches the servo on pin 9 to the servo object
  myservo.write(0);  // starting position;
  pinMode(candyLightPin, OUTPUT);
  pinMode(slideLightPin, OUTPUT);
  digitalWrite(slideLightPin, HIGH);
}


void loop()
{

  if ((millis() - lastPulseTime) > candyLightPulseLengthMs)
  {
    if (currentLightState == HIGH)
    {
      digitalWrite(candyLightPin, LOW);
      currentLightState = LOW;
      Serial.println("Setting light to Low");
    }
    else if (currentLightState == LOW)
    {
      digitalWrite(candyLightPin, HIGH);
      Serial.println("Setting light to High");
      currentLightState = HIGH;
    }
    lastPulseTime = millis();
  }

  if (digitalRead(candyButton) == LOW && (currentCandyButtonState == HIGH) 
    && (millis()-lastCandyButtonStateChange > debounceTimeMillis))
  {
    currentCandyButtonState = LOW;
    lastCandyButtonStateChange = millis();
    myservo.write(180);
    for (int i = 0; i < flashCountOnCandy; i++)
    {
      digitalWrite(candyLightPin, HIGH);
      digitalWrite(slideLightPin, HIGH);
      delay(activateLightsPulseLengthMs);
      digitalWrite(candyLightPin, LOW); 
      digitalWrite(slideLightPin, LOW);
      delay(activateLightsPulseLengthMs);    
    }
    
      digitalWrite(candyLightPin, HIGH);
      digitalWrite(slideLightPin, HIGH);


    delay(1000- (activateLightsPulseLengthMs*2*flashCountOnCandy));
    myservo.write(0);
    for (int i = 0; i < flashCountOnCandy; i++)
    {
      digitalWrite(slideLightPin, HIGH);
      delay(75);
      digitalWrite(slideLightPin, LOW);
      delay(75);    
    }
    digitalWrite(slideLightPin, HIGH);
    delay(1000);
    digitalWrite(candyLightPin, LOW);
    lastPulseTime = millis();
  } 
  
  if (digitalRead(candyButton) == HIGH && (millis()-lastCandyButtonStateChange > debounceTimeMillis)) {
      currentCandyButtonState = HIGH;
      lastCandyButtonStateChange = millis();
  }
  
  //myservo.writeMicroseconds(0);                // sets the servo position according to the scaled value
  //delay(250);                           // waits for the servo to get there
}