
#include <Arduino.h>
#include <Wire.h>
#include <Servo.h>
#include <LiquidCrystal.h>
#include <eepromhelper.h>

Servo myservo; // create servo object to control a servo
//Servo myTopservo; // create servo object to control a servo

bool ALLOW_SERIAL = false;
int candyButton = 2;
int slideLightPin = 7;
int candyLightPin = 8;
int servoPin = 9;
//int topServoPin = 10;
LiquidCrystal lcd(12, 11, 10, 6, 5, 4, 3);
eeprom_config_struct eepromsave;

int currentLightState = HIGH;
int currentCandyButtonState = LOW;
long candyLightPulseLengthMs = 1000;
long activateLightsPulseLengthMs = 75;
unsigned long lastPulseTime = 0;
unsigned long lastCandyButtonStateChange = 0;
int debounceTimeMillis = 100;
int flashCountOnCandy=4;
void setup()
{
  if (ALLOW_SERIAL) {
    Serial.begin(9600);
  }

  if (RESET_EEPROM) {
    //eepromsave.fastestTimeInEeprom = -1;
    //eepromsave.longestChainInEeprom = -1;
    EEPROM_writeAnything(EEPROM_SAVELOCATION, eepromsave);
  }


  EEPROM_readAnything(EEPROM_SAVELOCATION, eepromsave);


  myservo.attach(servoPin); // attaches the servo on pin 9 to the servo object
  // myTopservo.attach(topServoPin);
  // myTopservo.write(180);
  pinMode(candyLightPin, OUTPUT);
  pinMode(slideLightPin, OUTPUT);
  digitalWrite(slideLightPin, HIGH);
  myservo.write(0);  // starting position;
  //myservo.detach();
  lcd.begin(20,4);              // columns, rows.  use 16,2 for a 16x2 LCD, etc.
  lcd.clear();                  // start with a blank screen
  lcd.setCursor(0,0);           // set cursor to column 0, row 0 (the first row)
  lcd.print("i am bob");    // change text to whatever you like. keep it clean!
  lcd.setCursor(0,1);           // set cursor to column 0, row 1
  lcd.print("i am your brother");
 
  // if you have a 4 row LCD, uncomment these lines to write to the bottom rows
  // and change the lcd.begin() statement above.
  lcd.setCursor(0,2);         // set cursor to column 0, row 2
  lcd.print("how did i do this?");
  lcd.setCursor(0,3);         // set cursor to column 0, row 3
  lcd.print("unknown!");


}


void loop()
{
  if ((millis() - lastPulseTime) > candyLightPulseLengthMs)
  {
      //myservo.write(0);  // starting position;
      // myTopservo.write(180);
    if (currentLightState == HIGH)
    {
      digitalWrite(candyLightPin, LOW);
      currentLightState = LOW;
    }
    else if (currentLightState == LOW)
    {
      digitalWrite(candyLightPin, HIGH);
      currentLightState = HIGH;
    }
    lastPulseTime = millis();
  }

  if (digitalRead(candyButton) == LOW && (currentCandyButtonState == HIGH) 
    && ((millis()-lastCandyButtonStateChange) > debounceTimeMillis))
  {
    currentCandyButtonState = LOW;
    lastCandyButtonStateChange = millis();
    //myservo.attach(servoPin);
    myservo.write(0);
    delay(100);
    myservo.write(180);
    // myTopservo.write(0);
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
    // myTopservo.write(180);
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
    //myservo.detach();
  } 
  
  if (digitalRead(candyButton) == HIGH && (millis()-lastCandyButtonStateChange > debounceTimeMillis)) {
      currentCandyButtonState = HIGH;
      lastCandyButtonStateChange = millis();
  }
  
  //myservo.writeMicroseconds(0);                // sets the servo position according to the scaled value
  //delay(250);                           // waits for the servo to get there
}