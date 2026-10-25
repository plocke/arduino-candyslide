
#include <Arduino.h>
#include <LiquidCrystal.h>
#include <Servo.h>
#include <Wire.h>
#include <eepromhelper.h>
#include <lcdhelper.h>
#include <light_sound_helper.h>

// Servo myTopservo; // create servo object to control a servo

bool ALLOW_SERIAL = false;
int resetButton = A5;
int slideLightPin = 7;

// to add more candy chutes, only need to change this section
Servo servos_array[] = { Servo(), Servo() };
int servos_pins_array[] = { 9, 13 };
int candy_button_pins_array[] = { A4, 2 };
int candy_light_pins_array[] = { A3, 8 };
int candy_given_count_array[] = { 0, 0 };
const int NUMBER_CANDYCHUTES = 2;
String candy_names_array[] = { "Smarties", "Coffee Crisp" };
// end chutes

int randomButtonPin = A0;
int randomButtonLightPin = A1;
int randomButtonPressCount = 0;
int randomWinnerCount = 0;

const int speakerPin = A2;

LiquidCrystal lcd(12, 11, 10, 6, 5, 4, 3);
eeprom_config_struct eepromsave;

int pressed_button = -1;
int currentLightState = HIGH;
int currentCandyButtonState = LOW;
int currentRandomButtonState = HIGH;
unsigned long lastPulseTime = 0;
unsigned long lastRandomButtonStateChange = 0;
unsigned long lastCandyButtonStateChange = 0;
int bonusChancePercentage = 20; // change this for higher or lower bonus chance on random

const int BONUS_RESULT = -1;
const int SERVO_CANDY_CYCLE_DELAY = 500; // how long a chute stays open
// Servo travel. Full 0/180 stalls most hobby servos against their end stops (buzzing, heat, gear wear).
// Calibrate these to the smallest sweep that still clears the candy without buzzing at either end.
const int SERVO_CLOSED_ANGLE = 10;
const int SERVO_OPEN_ANGLE = 170;
const int SERVO_RETURN_SETTLE_MS = 2500; // time given for a servo to get fully home before its signal is removed
const long candyLightPulseLengthMs = 1000;
const long activateLightsPulseLengthMs = 75;
const int DEBOUNCE_TIME_MILLIS = 100;
const int flashCountOnCandy = 4;
const boolean RESET_EEPROM = false; // set to true and next boot will clear eeprom

void turnAllCandyLightsOnOrOff(bool turnOn)
{
    for (int i = 0; i < NUMBER_CANDYCHUTES; i++) {
        digitalWrite(candy_light_pins_array[i], turnOn ? HIGH : LOW);
        digitalWrite(randomButtonLightPin, turnOn ? HIGH : LOW);
    }
}

// Set the target before attaching so the first pulse is never the library's 90 degree default.
void openServo(int chute)
{
    servos_array[chute].write(SERVO_OPEN_ANGLE);
    servos_array[chute].attach(servos_pins_array[chute]);
}

void closeServo(int chute)
{
    servos_array[chute].write(SERVO_CLOSED_ANGLE);
}

void detachServos()
{
    for (int i = 0; i < NUMBER_CANDYCHUTES; i++) {
        servos_array[i].detach(); // stops jitter while idle
        // detach() only clears a flag; if it lands mid-pulse the ISR never ends the pulse and the
        // signal line stays HIGH, which sends the servo somewhere random. Force it low.
        digitalWrite(servos_pins_array[i], LOW);
    }
}

void setLCDstartingText()
{
    lcd.clear();
    setFirstLine(candy_names_array[0] + ": " + String(candy_given_count_array[0]), lcd); // change text to whatever you like. keep it clean!
    setSecondLine(candy_names_array[1] + ": " + String(candy_given_count_array[1]), lcd);
    setThirdLine("Bonus Wins: " + String(randomWinnerCount), lcd);
    setFourthLine("Bonus Tries: " + String(randomButtonPressCount), lcd);
}

void setLCDdispensingText(int buttonPressed)
{
    lcd.clear();
    setFirstLine("********************", lcd);
    setSecondLine(candy_names_array[buttonPressed] + " !!", lcd);
    setThirdLine(" # Trick-or-Treat # ", lcd);
    setFourthLine("********************", lcd);
}
void setBonusWinnerText()
{
    lcd.clear();
    setFirstLine("********************", lcd);
    setSecondLine(String(randomWinnerCount) + " Winner!!", lcd);
    setThirdLine(" # Trick-or-Treat # ", lcd);
    setFourthLine("********************", lcd);
}

void resetEEPROM()
{
    for (int i = 0; i < NUMBER_CANDYCHUTES; i++) {
        eepromsave.candy_counts[i] = 0;
        candy_given_count_array[i] = 0;
    }
    eepromsave.randomButtonPressCount = 0;
    eepromsave.randomWinnerCount = 0;
    randomWinnerCount = 0;
    randomButtonPressCount = 0;

    EEPROM_writeAnything(EEPROM_SAVELOCATION, eepromsave);
}

void setup()
{
    if (ALLOW_SERIAL) {
        Serial.begin(9600);
    }

    if (RESET_EEPROM) {
        resetEEPROM();
    }

    EEPROM_readAnything(EEPROM_SAVELOCATION, eepromsave);
    randomButtonPressCount = eepromsave.randomButtonPressCount;
    randomWinnerCount = eepromsave.randomWinnerCount;

    randomSeed(millis() + analogRead(A6));

    for (int i = 0; i < NUMBER_CANDYCHUTES; i++) {
        candy_given_count_array[i] = eepromsave.candy_counts[i];
        pinMode(candy_button_pins_array[i], INPUT_PULLUP);
        pinMode(candy_light_pins_array[i], OUTPUT);
        servos_array[i].write(SERVO_CLOSED_ANGLE);
        servos_array[i].attach(servos_pins_array[i]);
    }

    pinMode(randomButtonPin, INPUT_PULLUP);
    pinMode(randomButtonLightPin, OUTPUT);
    pinMode(resetButton, INPUT_PULLUP);
    pinMode(slideLightPin, OUTPUT);
    pinMode(speakerPin, OUTPUT);
    digitalWrite(slideLightPin, HIGH);

    lcd.begin(20, 4); // columns, rows.  use 16,2 for a 16x2 LCD, etc.
    lcd.clear(); // start with a blank screen
    setSecondLine("Happy Halloween 2026", lcd); // greeting shown while the startup tune plays
    playBonusTune(speakerPin); // the tune plus the delay below give the servos time to reach home before release
    delay(SERVO_CANDY_CYCLE_DELAY);
    detachServos();
    setLCDstartingText();
}

void loop()
{
    if ((millis() - lastPulseTime) > candyLightPulseLengthMs) {

        if (currentLightState == HIGH) {
            turnAllCandyLightsOnOrOff(false);
            currentLightState = LOW;
            // blinkCursorOnScreen(true);
        } else if (currentLightState == LOW) {
            turnAllCandyLightsOnOrOff(true);
            currentLightState = HIGH;
            // blinkCursorOnScreen(false);
        }
        lastPulseTime = millis();
    }

    pressed_button = -1;
    for (int i = 0; i < NUMBER_CANDYCHUTES; i++) {
        if (digitalRead(candy_button_pins_array[i]) == LOW) {
            pressed_button = i;
            break;
        }
    }

    if (currentRandomButtonState == HIGH && (millis() - lastRandomButtonStateChange) > DEBOUNCE_TIME_MILLIS) {
        if (digitalRead(randomButtonPin) == LOW) {
            currentRandomButtonState = LOW;
            lastRandomButtonStateChange = millis();
            int randNum = random(100);
            int candyChuteIndexOrBonus;
            if (randNum < bonusChancePercentage) {
                candyChuteIndexOrBonus = BONUS_RESULT;
            } else {
                candyChuteIndexOrBonus = random(NUMBER_CANDYCHUTES);
            }
            randomButtonPressCount++;
            lcd.clear();
            setBonusWinnerText();
            setFourthLine("Bonus!: " + String(randNum) + " " + String(candyChuteIndexOrBonus) + " " + String(bonusChancePercentage), lcd);
            if (candyChuteIndexOrBonus == BONUS_RESULT) {
                randomWinnerCount++;
               
                // Open every chute
                for (int i = 0; i < NUMBER_CANDYCHUTES; i++) {
                    openServo(i);
                }

                 for (int i = 0; i < 3; i++) {
                    digitalWrite(randomButtonLightPin, HIGH);
                    delay(100);
                    digitalWrite(randomButtonLightPin, LOW);
                    delay(100);
                }

                playBonusTune(speakerPin);
                delay(SERVO_CANDY_CYCLE_DELAY);


                // Close every chute and give them the same time to get home as a normal candy press
                for (int i = 0; i < NUMBER_CANDYCHUTES; i++) {
                    closeServo(i);
                }
                delay(SERVO_RETURN_SETTLE_MS);
                detachServos();
            } else {
                pressed_button = candyChuteIndexOrBonus;
            }
            eepromsave.randomWinnerCount = randomWinnerCount;
            eepromsave.randomButtonPressCount = randomButtonPressCount;
            EEPROM_writeAnything(EEPROM_SAVELOCATION, eepromsave);
            setLCDstartingText();

        }
    }
    if (currentRandomButtonState == LOW && digitalRead(randomButtonPin) == HIGH) {

        currentRandomButtonState = HIGH;
    }

    if (pressed_button >= 0 && (currentCandyButtonState == HIGH) && ((millis() - lastCandyButtonStateChange) > DEBOUNCE_TIME_MILLIS)) {
        currentCandyButtonState = LOW;
        lastCandyButtonStateChange = millis();
        candy_given_count_array[pressed_button]++;
        setLCDdispensingText(pressed_button);

        openServo(pressed_button);

        turnAllCandyLightsOnOrOff(false);
        for (int i = 0; i < flashCountOnCandy; i++) {
            digitalWrite(candy_light_pins_array[pressed_button], HIGH);
            digitalWrite(slideLightPin, HIGH);
            delay(activateLightsPulseLengthMs);
            digitalWrite(candy_light_pins_array[pressed_button], LOW);
            digitalWrite(slideLightPin, LOW);
            delay(activateLightsPulseLengthMs);
        }

        digitalWrite(candy_light_pins_array[pressed_button], HIGH);
        digitalWrite(slideLightPin, HIGH);

        delay(SERVO_CANDY_CYCLE_DELAY);

        closeServo(pressed_button);

        for (int i = 0; i < flashCountOnCandy; i++) {
            digitalWrite(slideLightPin, HIGH);
            delay(75);
            digitalWrite(slideLightPin, LOW);
            delay(75);
        }
        digitalWrite(slideLightPin, HIGH);
        delay(SERVO_RETURN_SETTLE_MS);
        detachServos();
        turnAllCandyLightsOnOrOff(false);
        lastPulseTime = millis();
        eepromsave.candy_counts[pressed_button] = candy_given_count_array[pressed_button];
        EEPROM_writeAnything(EEPROM_SAVELOCATION, eepromsave);
        lcd.clear();
        setLCDstartingText();
    }

    if (currentCandyButtonState == LOW && (pressed_button < 0 || digitalRead(candy_button_pins_array[pressed_button]) == HIGH) && (millis() - lastCandyButtonStateChange > DEBOUNCE_TIME_MILLIS)) {
        currentCandyButtonState = HIGH;
        lastCandyButtonStateChange = millis();
        for (int i = 0; i < flashCountOnCandy; i++) {
            turnAllCandyLightsOnOrOff(true);
            delay(50);
            turnAllCandyLightsOnOrOff(false);
            delay(50);
        }
    }

    if (digitalRead(resetButton) == LOW) {
        resetEEPROM();
        lcd.clear();
        setFirstLine("Reset candy count", lcd);
        delay(2000);
        setLCDstartingText();
    }
}