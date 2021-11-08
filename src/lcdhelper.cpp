#include <Wire.h>
#include <LiquidCrystal.h>

void setFirstLine(String line, LiquidCrystal lcd){
  lcd.setCursor(0, 0);
  lcd.print(line);
}

void setSecondLine(String line, LiquidCrystal lcd) {
  lcd.setCursor(0, 1);
  lcd.print(line);
}

void setThirdLine(String line, LiquidCrystal lcd){
  lcd.setCursor(0, 2);
  lcd.print(line);
}

void setFourthLine(String line, LiquidCrystal lcd) {
  lcd.setCursor(0, 3);
  lcd.print(line);
}
