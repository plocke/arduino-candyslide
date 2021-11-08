#ifndef lcdhelper_h
#define lcdhelper_h
#include <Wire.h>
#include <LiquidCrystal.h>

void setFirstLine(String line, LiquidCrystal lcd);
void setSecondLine(String line, LiquidCrystal lcd);
void setThirdLine(String line, LiquidCrystal lcd);
void setFourthLine(String line, LiquidCrystal lcd);
#endif
