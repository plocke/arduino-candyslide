
#include <Arduino.h>
#include <Wire.h>
#include <Servo.h>
#include <LiquidCrystal.h>
#include <eepromhelper.h>
#include <lcdhelper.h>


//Servo myTopservo; // create servo object to control a servo

bool ALLOW_SERIAL = false;
int candyButton = 2;
int resetButton = A5;
int slideLightPin = 7;
int candyLightPin = 8;
int servoPin = 9;
int topServoPin = 13;




Servo servos_array[] = {Servo(), Servo()};
int servos_pins_array[] = {9, 13};
int candy_button_pins_array[] = {2, A4};
int pressed_button = -1;

LiquidCrystal lcd(12, 11, 10, 6, 5, 4, 3);
eeprom_config_struct eepromsave;

int currentLightState = HIGH;
int currentCandyButtonState = LOW;
unsigned long lastPulseTime = 0;
unsigned long lastCandyButtonStateChange = 0;
int candyGiven = 0;

const int NUMBER_CANDYCHUTES = 2;
const int SERVO_CANDY_CYCLE_DELAY = 500;
const long candyLightPulseLengthMs = 1000;
const long activateLightsPulseLengthMs = 75;
const int debounceTimeMillis = 100;
const int flashCountOnCandy=4;
const boolean RESET_EEPROM = false; //set to true and next boot will clear eeprom

void detachServos() {
  for (int i = 0; i < NUMBER_CANDYCHUTES; i++)
    {
      servos_array[i].detach(); //stops jitter. @todo move to after longer delay to give friction time
    }
}

void setLCDstartingText()
{
  lcd.clear();
  setFirstLine("CANDY GIVEN: "+String(candyGiven), lcd);    // change text to whatever you like. keep it clean!
  setSecondLine("-SLIDE v3--", lcd);
  setThirdLine("Press Foot Button", lcd);
  setFourthLine("For Candy!", lcd);
}

void setLCDdispensingText()
{
    lcd.clear();
    setFirstLine("********************", lcd); 
    setSecondLine("! DISPENSING CANDY !", lcd);
    setThirdLine("Trick-or-Treat # "+String(candyGiven), lcd);
    setFourthLine("********************", lcd);
}

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
  for (int i = 0; i < NUMBER_CANDYCHUTES; i++)
   {
    pinMode(candy_button_pins_array[i], INPUT_PULLUP);
    servos_array[i].attach(servos_pins_array[i]);
    servos_array[i].write(0);
  }
  
  //myservo.attach(servoPin); // attaches the servo on pin 9 to the servo object
  //myTopservo.attach(topServoPin);
  //pinMode(candyButton, INPUT_PULLUP);
  pinMode(resetButton, INPUT_PULLUP);
  pinMode(candyLightPin, OUTPUT);
  pinMode(slideLightPin, OUTPUT);
  digitalWrite(slideLightPin, HIGH);

  //myservo.write(0);  // starting position;
  //myTopservo.write(0);
  lcd.begin(20,4);              // columns, rows.  use 16,2 for a 16x2 LCD, etc.
  lcd.clear();                  // start with a blank screen
  setLCDstartingText();
  delay(SERVO_CANDY_CYCLE_DELAY);
  detachServos();
}

void blinkCursorOnScreen(bool blinkOn)
{
      if (blinkOn)
      {
        lcd.setCursor(19, 1);
        lcd.write("|");
        lcd.setCursor(19, 2);
        lcd.write("|");
        lcd.setCursor(19, 3);
        lcd.write("V");
      } else {
        lcd.setCursor(19, 1);
        lcd.write(" ");
        lcd.setCursor(19, 2);
        lcd.write(" ");
        lcd.setCursor(19, 3);
        lcd.write(" ");
      }
      
}

void loop()
{
  if ((millis() - lastPulseTime) > candyLightPulseLengthMs)
  {
    
    if (currentLightState == HIGH)
    {
      digitalWrite(candyLightPin, LOW);
      currentLightState = LOW;
      blinkCursorOnScreen(true);

    }
    else if (currentLightState == LOW)
    {
      digitalWrite(candyLightPin, HIGH);
      currentLightState = HIGH;
      blinkCursorOnScreen(false);
    }
    lastPulseTime = millis();
  }

  pressed_button = -1;
  for (int i = 0; i < NUMBER_CANDYCHUTES; i++)
  {
    if (digitalRead(candy_button_pins_array[i]) == LOW)
    {
      pressed_button = i;
      break;
    }
  }
  

  if (pressed_button >= 0 && (currentCandyButtonState == HIGH) 
    && ((millis()-lastCandyButtonStateChange) > debounceTimeMillis))
  {
    currentCandyButtonState = LOW;
    lastCandyButtonStateChange = millis();
    candyGiven++;
    setLCDdispensingText();

    
      servos_array[pressed_button].attach(servos_pins_array[pressed_button]);
      servos_array[pressed_button].write(180);
   

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


    delay(SERVO_CANDY_CYCLE_DELAY);
  
    servos_array[pressed_button].write(0);

    for (int i = 0; i < flashCountOnCandy; i++)
    {
      digitalWrite(slideLightPin, HIGH);
      delay(75);
      digitalWrite(slideLightPin, LOW);
      delay(75);    
    }
    digitalWrite(slideLightPin, HIGH);
    delay(2500);
    detachServos();
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
  
  if (currentCandyButtonState == LOW && 
  (pressed_button < 0 || digitalRead(candy_button_pins_array[pressed_button]) == HIGH) && 
   (millis()-lastCandyButtonStateChange > debounceTimeMillis)) {
      currentCandyButtonState = HIGH;
      lastCandyButtonStateChange = millis();
      for (int i = 0; i < flashCountOnCandy; i++)
    {
      digitalWrite(candyLightPin, HIGH);
      delay(50);
      digitalWrite(candyLightPin, LOW);
      delay(50);    
    }
  }

  if (digitalRead(resetButton) == LOW) {
    eepromsave.candylifetime = 0;
    candyGiven = 0;
    EEPROM_writeAnything(EEPROM_SAVELOCATION, eepromsave);
    lcd.clear();
    setFirstLine("Reset candy count", lcd);    
  }
}