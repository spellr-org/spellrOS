#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h> // Include the Wire library for I2C

// Use the correct constructor for your OLED display, this is just an example
U8G2_SSD1306_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0);

void setup(void) {
  u8g2.begin();
}

void loop(void) {
  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_ncenB14_tr);
    u8g2.drawStr(0,24,"Hello World!");
  } while ( u8g2.nextPage() );
}
