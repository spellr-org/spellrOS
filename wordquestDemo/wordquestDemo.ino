#include <Wire.h>
#include <Adafruit_MSA301.h>
#include <Adafruit_Sensor.h>
#include <U8g2lib.h>

Adafruit_MSA311 msa;
U8G2_SSD1306_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0);

const char* words[] = {"apple", "banana", "orange", "qwerty", "zxcvbn"};
const bool wordIsReal[] = {true, true, true, false, false};
const int numWords = sizeof(words) / sizeof(words[0]);

int currentWordIndex = 0;
float neutralY = 0.0;
const unsigned long calibrationTime = 3000; // 3 seconds


// TUNE UNTIL IT FEELS RIGHT
const float neutralRange = 200; // Adjust this value to change the neutral range
const float speedThreshold = 400.0; // Adjust this value to change the speed threshold





float prevY = 0.0;

void setup(void) {
  Serial.begin(115200);
  while (!Serial) delay(10);  // will pause Zero, Leonardo, etc until serial console opens

  Serial.println("Adafruit MSA301 test!");
  Serial.println("Number of words: " + String(numWords));

  pinMode(16, INPUT_PULLUP);  // SCL1 (Teensy Pin 16)
  pinMode(17, INPUT_PULLUP);  // SDA1 (Teensy Pin 17)
  Wire1.setSCL(16);  // Replace SCL1 with the actual pin number for SCL1
  Wire1.setSDA(17);  // Replace SDA1 with the actual pin number for SDA1
  Wire1.begin();

  // Try to initialize!
  if (!msa.begin(MSA311_I2CADDR_DEFAULT, &Wire1)) {
    Serial.println("Failed to find MSA301 chip");
    while (1) { delay(10); }
  }
  Serial.println("MSA301 Found!");

  // Initialize the display
  u8g2.begin();
  u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.clearBuffer();
  u8g2.drawStr(0, 10, "Calibrating...");
  u8g2.sendBuffer();

  unsigned long startTime = millis();
  while (millis() - startTime < calibrationTime) {
    msa.read();
    neutralY = (neutralY + msa.y) / 2.0;  // Average y values for calibration
  }

  u8g2.clearBuffer();
  u8g2.drawStr(0, 10, words[currentWordIndex]);
  u8g2.sendBuffer();
}

void loop() {
  Serial.println("+");
  delay(3000);  // Delay to establish neutral position

  // Calibrate neutral position
  unsigned long startTime = millis();
  while (millis() - startTime < calibrationTime) {
    msa.read();
    neutralY = (neutralY + msa.y) / 2.0;  // Average y values for calibration
  }


  Serial.println(currentWordIndex);
  Serial.println(words[currentWordIndex]);

  // u8g2.clearBuffer();
  // u8g2.drawStr(0, 10, words[currentWordIndex]);
  // u8g2.sendBuffer();

  // Detect left or right tilt relative to neutral position
  bool tiltDetected = false;
  while (!tiltDetected) {
    msa.read();

    if (abs(msa.y - prevY) > speedThreshold) {
      if (msa.y < neutralY - neutralRange) {
        Serial.println("Left");
        tiltDetected = true;
      } else if (msa.y > neutralY + neutralRange) {
        Serial.println("Right");
        tiltDetected = true;
      }
    }

    prevY = msa.y; // Update the previous Y value
    delay(100);
  }

  // Update word index based on tilt direction and correctness
  // if (wordIsReal[currentWordIndex]) {
  //   if (Serial.readString() == "Left\n") {
  //     currentWordIndex = (currentWordIndex + 1) % numWords;
  //   } else {
  //     u8g2.clearBuffer();
  //     u8g2.drawStr(0, 10, "Wrong!");
  //     u8g2.sendBuffer();
  //     delay(1000);
  //     u8g2.clearBuffer();
  //     u8g2.drawStr(0, 10, words[currentWordIndex]);
  //     u8g2.sendBuffer();
  //   }
  // } else {
  //   if (Serial.readString() == "Right\n") {
  //     currentWordIndex = (currentWordIndex + 1) % numWords;
  //   } else {
  //     u8g2.clearBuffer();
  //     u8g2.drawStr(0, 10, "Wrong!");
  //     u8g2.sendBuffer();
  //     delay(1000);
  //     u8g2.clearBuffer();
  //     u8g2.drawStr(0, 10, words[currentWordIndex]);
  //     u8g2.sendBuffer();
  //   }
  // }
  currentWordIndex = (currentWordIndex + 1) % numWords;
}