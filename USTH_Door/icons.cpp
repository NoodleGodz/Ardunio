/**
   Arduino Electronic Safe

   Copyright (C) 2020, Uri Shaked.
   Released under the MIT License.
*/

#include <Arduino.h>
#include "icons.h"

const byte iconLocked[8] PROGMEM = {
  0b01110,
  0b10001,
  0b10001,
  0b11111,
  0b11011,
  0b11011,
  0b11111,
};

const byte iconUnlocked[8] PROGMEM = {
  0b01110,
  0b10000,
  0b10000,
  0b11111,
  0b11011,
  0b11011,
  0b11111,
};

uint8_t bell[8] = {0x4,0xe,0xe,0xe,0x1f,0x0,0x4};

void init_icons(LiquidCrystal_I2C &lcd) {
  byte icon[8];
  memcpy_P(icon, iconLocked, sizeof(icon));
  lcd.createChar(ICON_LOCKED_CHAR, icon);
  memcpy_P(icon, iconUnlocked, sizeof(icon));
  lcd.createChar(ICON_UNLOCKED_CHAR, icon);
  lcd.createChar(ICON_BELL, bell);
}
