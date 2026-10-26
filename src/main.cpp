
#include <Arduino.h>
#include <LiquidCrystal.h>
#include <Servo.h>
#include <Wire.h>
#include <eepromhelper.h>
#include <lcdhelper.h>
#include <light_sound_helper.h>

// Servo myTopservo; // create servo object to control a servo

const String SOFTWARE_VERSION = "5"; // shown on the boot screen
bool ALLOW_SERIAL = false;
bool SOUND_ENABLED = false; // default until saved in EEPROM; false mutes the speaker, tune timing is kept so servo dwell is unchanged
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
const unsigned long RESET_HOLD_MS = 3000; // hold the reset button this long to clear counts; shorter press toggles sound
const int flashCountOnCandy = 4;
const boolean RESET_EEPROM = false; // set to true and next boot will clear eeprom

void turnAllCandyLightsOnOrOff(bool turnOn)
{
    for (int i = 0; i < NUMBER_CANDYCHUTES; i++) {
        digitalWrite(candy_light_pins_array[i], turnOn ? HIGH : LOW);
        digitalWrite(randomButtonLightPin, turnOn ? HIGH : LOW);
    }
}

bool isAnyCandyButtonDown()
{
    for (int i = 0; i < NUMBER_CANDYCHUTES; i++) {
        if (digitalRead(candy_button_pins_array[i]) == LOW) {
            return true;
        }
    }
    return false;
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

// bonusRoll < 0 means the dispense came from a candy button, so there is no roll to show
void setLCDdispensingText(int buttonPressed, int bonusRoll)
{
    lcd.clear();
    setFirstLine("********************", lcd);
    setSecondLine(candy_names_array[buttonPressed] + " !!", lcd);
    setThirdLine(" # Trick-or-Treat # ", lcd);
    setFourthLine("********************", lcd);
    if (bonusRoll >= 0) {
        setFourthLine(String(bonusRoll), lcd); // overwrites the bottom-left stars
    }
}
void setBonusWinnerText(int bonusRoll)
{
    lcd.clear();
    setFirstLine("********************", lcd);
    setSecondLine(String(randomWinnerCount) + " Winner!!", lcd);
    setThirdLine(" # Trick-or-Treat # ", lcd);
    setFourthLine("********************", lcd);
    setFourthLine(String(bonusRoll), lcd); // overwrites the bottom-left stars
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
    eepromsave.soundSetting = SOUND_ENABLED ? 1 : 0; // keep the current sound choice across a count reset

    EEPROM_writeAnything(EEPROM_SAVELOCATION, eepromsave);
}

// Short press toggles sound; holding for RESET_HOLD_MS resets the counts.
void handleResetButton()
{
    unsigned long pressStart = millis();
    bool didReset = false;
    while (digitalRead(resetButton) == LOW) {
        if (!didReset && (millis() - pressStart) >= RESET_HOLD_MS) {
            resetEEPROM();
            lcd.clear();
            setFirstLine("Reset candy count", lcd);
            didReset = true;
        }
    }
    unsigned long heldMs = millis() - pressStart;
    delay(DEBOUNCE_TIME_MILLIS); // let the contacts settle after release

    if (didReset) {
        delay(2000);
    } else if (heldMs > DEBOUNCE_TIME_MILLIS) {
        SOUND_ENABLED = !SOUND_ENABLED;
        eepromsave.soundSetting = SOUND_ENABLED ? 1 : 0;
        EEPROM_writeAnything(EEPROM_SAVELOCATION, eepromsave);
        lcd.clear();
        setFirstLine(SOUND_ENABLED ? "Sound: ON" : "Sound: OFF", lcd);
        if (SOUND_ENABLED) {
            tone(speakerPin, 880, 150);
        }
        delay(1000);
    } else {
        return; // too short to be a real press
    }
    setLCDstartingText();
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
    if (eepromsave.soundSetting <= 1) {
        SOUND_ENABLED = eepromsave.soundSetting == 1;
    }

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
    setFirstLine("Bonus chance: " + String(bonusChancePercentage) + "%", lcd);
    setSecondLine("Happy Halloween 2026", lcd); // greeting shown while the startup tune plays
    setThirdLine("Version " + SOFTWARE_VERSION, lcd);
    setFourthLine(SOUND_ENABLED ? "Sound: ON" : "Sound: OFF", lcd);
    playBonusTune(speakerPin, SOUND_ENABLED); // the tune plus the delay below give the servos time to reach home before release
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
    int bonusRoll = -1; // set when this dispense comes from the random button
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
            if (candyChuteIndexOrBonus == BONUS_RESULT) {
                setBonusWinnerText(randNum);
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

                playBonusTune(speakerPin, SOUND_ENABLED);
                delay(SERVO_CANDY_CYCLE_DELAY);


                // Close every chute and give them the same time to get home as a normal candy press
                for (int i = 0; i < NUMBER_CANDYCHUTES; i++) {
                    closeServo(i);
                }
                delay(SERVO_RETURN_SETTLE_MS);
                detachServos();
            } else {
                pressed_button = candyChuteIndexOrBonus;
                bonusRoll = randNum;
            }
            eepromsave.randomWinnerCount = randomWinnerCount;
            eepromsave.randomButtonPressCount = randomButtonPressCount;
            EEPROM_writeAnything(EEPROM_SAVELOCATION, eepromsave);
            setLCDstartingText();

        }
    }
    // Only count it as released once it has stayed up for the debounce time, so a long hold followed
    // by contact bounce on release doesn't look like a fresh press
    if (currentRandomButtonState == LOW) {
        if (digitalRead(randomButtonPin) == LOW) {
            lastRandomButtonStateChange = millis();
        } else if ((millis() - lastRandomButtonStateChange) > DEBOUNCE_TIME_MILLIS) {
            currentRandomButtonState = HIGH;
            lastRandomButtonStateChange = millis();
        }
    }

    if (pressed_button >= 0 && (currentCandyButtonState == HIGH) && ((millis() - lastCandyButtonStateChange) > DEBOUNCE_TIME_MILLIS)) {
        currentCandyButtonState = LOW;
        lastCandyButtonStateChange = millis();
        candy_given_count_array[pressed_button]++;
        setLCDdispensingText(pressed_button, bonusRoll);

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

    // Same release rule as the random button: every candy button must stay up for the debounce time
    if (currentCandyButtonState == LOW && isAnyCandyButtonDown()) {
        lastCandyButtonStateChange = millis();
    } else if (currentCandyButtonState == LOW && (millis() - lastCandyButtonStateChange > DEBOUNCE_TIME_MILLIS)) {
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
        handleResetButton();
    }
}