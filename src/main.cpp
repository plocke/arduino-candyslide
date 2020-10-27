
#include <Arduino.h>
#include <Wire.h>
#include <Servo.h>

Servo myservo; // create servo object to control a servo

int candyButton = 2;
int candyLightPin = 8;
int currentLightState = HIGH;
long candyLightPulseLengthMs = 1000;
long lastPulseTime = 0;
int flashCountOnCandy=4;
void setup()
{
  Serial.begin(9600);
  myservo.attach(9); // attaches the servo on pin 9 to the servo object
  myservo.write(0);  // starting position;
  pinMode(candyLightPin, OUTPUT);
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

  if (digitalRead(candyButton) == HIGH)
  {
    for (int i = 0; i < flashCountOnCandy; i++)
    {
      digitalWrite(candyLightPin, HIGH);
      delay(75);
      digitalWrite(candyLightPin, LOW); 
      delay(75);    
    }
    
          digitalWrite(candyLightPin, HIGH);

    Serial.println("Activating servo 1");
    myservo.write(180);
    delay(1000);
    myservo.write(0);
    delay(1500);
    digitalWrite(candyLightPin, LOW);
    lastPulseTime = millis();
  }
  //myservo.writeMicroseconds(0);                // sets the servo position according to the scaled value
  //delay(250);                           // waits for the servo to get there
}