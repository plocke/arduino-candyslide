
#include <Arduino.h>
#include <Wire.h>
#include <Servo.h>
#include <LiquidCrystal.h>
#include <eepromhelper.h>
#include <lcdhelper.h>


Servo myservo; // create servo object to control a servo
//Servo myTopservo; // create servo object to control a servo

bool ALLOW_SERIAL = false;
int candyButton = 2;
int resetButton = A5;
int slideLightPin = 7;
int candyLightPin = 8;
int servoPin = 9;
//int topServoPin = 10;
LiquidCrystal lcd(12, 11, 10, 6, 5, 4, 3);
eeprom_config_struct eepromsave;

int currentLightState = HIGH;
int currentCandyButtonState = LOW;
unsigned long lastPulseTime = 0;
unsigned long lastCandyButtonStateChange = 0;
int candyGiven = 0;

const long candyLightPulseLengthMs = 1000;
const long activateLightsPulseLengthMs = 75;
const int debounceTimeMillis = 100;
const int flashCountOnCandy=4;
const boolean RESET_EEPROM = false; //set to true and next boot will clear eeprom

void setup()
{
  if (ALLOW_SERIAL) {
    Serial.begin(9600);
  }

  if (RESET_EEPROM) {
    eepromsave.candylifetime = 0;
    candyGiven = 0;
    EEPROM_writeAnything(EEPROM_SAVELOCATION, eepromsave);
  }


  EEPROM_readAnything(EEPROM_SAVELOCATION, eepromsave);
  candyGiven = eepromsave.candylifetime;


  myservo.attach(servoPin); // attaches the servo on pin 9 to the servo object
  // myTopservo.attach(topServoPin);
  // myTopservo.write(180);
  pinMode(candyButton, INPUT_PULLUP);
  pinMode(resetButton, INPUT_PULLUP);
  pinMode(candyLightPin, OUTPUT);
  pinMode(slideLightPin, OUTPUT);
  digitalWrite(slideLightPin, HIGH);
  myservo.write(0);  // starting position;
  //myservo.detach();
  lcd.begin(20,4);              // columns, rows.  use 16,2 for a 16x2 LCD, etc.
  lcd.clear();                  // start with a blank screen
  setFirstLine("CANDY GIVEN: "+String(candyGiven), lcd);    // change text to whatever you like. keep it clean!
  setSecondLine("---", lcd);
  setThirdLine("Press Foot Button", lcd);
  setFourthLine("For Candy!", lcd);


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
      lcd.setCursor(19, 1);
      lcd.write("|");
      lcd.setCursor(19, 2);
      lcd.write("|");
      lcd.setCursor(19, 3);
      lcd.write("V");

    }
    else if (currentLightState == LOW)
    {
      digitalWrite(candyLightPin, HIGH);
      currentLightState = HIGH;
      lcd.setCursor(19, 1);
      lcd.write(" ");
      lcd.setCursor(19, 2);
      lcd.write(" ");
      lcd.setCursor(19, 3);
      lcd.write(" ");
    }
    lastPulseTime = millis();
  }

  if (digitalRead(candyButton) == LOW && (currentCandyButtonState == HIGH) 
    && ((millis()-lastCandyButtonStateChange) > debounceTimeMillis))
  {
    currentCandyButtonState = LOW;
    lastCandyButtonStateChange = millis();
    candyGiven++;
    lcd.clear();
    setFirstLine("********************", lcd); 
    setSecondLine("! DISPENSING CANDY !", lcd);
    setThirdLine("Trick-or-Treat # "+String(candyGiven), lcd);
    setFourthLine("********************", lcd);
    myservo.write(0);
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
    delay(3000);
    digitalWrite(candyLightPin, LOW);
    lastPulseTime = millis();
    eepromsave.candylifetime = candyGiven;
    EEPROM_writeAnything(EEPROM_SAVELOCATION, eepromsave);
    lcd.clear();
    setFirstLine("CANDY GIVEN: "+String(candyGiven), lcd);    // change text to whatever you like. keep it clean!
    setSecondLine("---", lcd);
    setThirdLine("Press Foot Button", lcd);
    setFourthLine("For Candy!", lcd);
  } 
  
  if (digitalRead(candyButton) == HIGH && (millis()-lastCandyButtonStateChange > debounceTimeMillis)) {
      currentCandyButtonState = HIGH;
      lastCandyButtonStateChange = millis();
  }

  if (digitalRead(resetButton) == LOW) {
    eepromsave.candylifetime = 0;
    candyGiven = 0;
    EEPROM_writeAnything(EEPROM_SAVELOCATION, eepromsave);
    lcd.clear();
    setFirstLine("Reset candy count", lcd);    // change text to whatever you like. keep it clean!

  }
  
  //myservo.writeMicroseconds(0);                // sets the servo position according to the scaled value
  //delay(250);                           // waits for the servo to get there
}