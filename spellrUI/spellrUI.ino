// IN OMAR WE TRUST
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <U8g2lib.h>
#include <SoftwareSerial.h>

#include <Adafruit_MSA301.h>
#include <Adafruit_Sensor.h>
#include <float.h> 
#include "Bitmaps.h"
//#include "Menu.h"
//#include "AccelerometerHandler.h"

// DEFINING THE PINS
#define BUTTON_UP_PIN 12 // pin for UP button 
#define BUTTON_SELECT_PIN 5 // pin for SELECT button
#define BUTTON_DOWN_PIN 4 // pin for DOWN button

// Display configuration
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

Adafruit_MSA311 msa;

// U8G2 Library shit: need to adjust the stuff in the parentheses after u8g2() in order to use multiple serial ports or something like that
// U8GLIB_SSD1306_128X64 u8g(U8G_I2C_OPT_DEV_0 | U8G_I2C_OPT_NO_ACK | U8G_I2C_OPT_FAST); // Fast I2C / TWI
U8G2_SSD1306_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0);
// U8GLIB_SSD1306_128X64 u8g(13, 11, 8, 9, 10); // SPI connection
// for SPI connection, use this wiring:
// GND > GND
// VCC > 5V
// SCL > 13
// SDA > 11
// RES > 10
// DC > 9
// CS > 8

SoftwareSerial BTSerial(7, 8);








// LETTER ANIMATION BITMAPS
String SSWORD = "HHH";
int currentLetter = 0;
int frameCounter = 0;
int wifi_counter = 0;




const int NUM_ITEMS = 8; // number of items in the list and also the number of screenshots and screenshots with QR codes (other screens)
const int MAX_ITEM_LENGTH = 20; // maximum characters for the item name

char menu_items [NUM_ITEMS] [MAX_ITEM_LENGTH] = {  // array with item names
  { "TRANSCRIBE" }, 
  { "Battery" }, 
  { "Dashboard" },
  { "WordQuest" },
  { "Bluetooth" }, 
  { "Calibrate" }
 };

// String menuItems[] = {"TRANSCRIBE", "Battery", "Dashboard", "WordQuest", "Bluetooth", "Calibrate"};
// Menu menu(u8g2, menuItems, sizeof(menuItems) / sizeof(menuItems[0]));



int button_up_clicked = 0; // only perform action when button is clicked, and wait until another press
int button_select_clicked = 0; // same as above
int button_down_clicked = 0; // same as above

int item_selected = 0; // which item in the menu is selected

int item_sel_previous; // previous item - used in the menu screen to draw the item before the selected one
int item_sel_next; // next item - used in the menu screen to draw next item after the selected one

int current_screen = 0;   // 0 = menu, 1 = screenshot, 2 = qr

// SPELLING: 0 just shows the word, 1 shows the letters one by one, 2 shows each individual letter being written out
int spelling_mode = 0;



/***************

STATE TRANSCRIBE - variables needed for transcription functionality

***************/
const int micPin = A0;
bool inTranscribeMode = false;

String inputString = "";      // a String to hold incoming data
bool stringComplete = false;  // whether the string is complete

String translated_word = "Try Again!";


/***************

STATE SWR - Variables needed for SWR game (wordquest)

***************/
char* words[] = {"apple", "banana", "orange", "qwerty", "zxcvbn"};
bool wordIsReal[] = {true, true, true, false, false};
int numWords = sizeof(words) / sizeof(words[0]);

int wordIndex = 0;
bool swr_waiting = true;
int correct = 0, incorrect = 0;

float baselinePitch = -30.0;

/********************

HELPER FN FOR FUNDING TILT DIRECTION (pitch and roll based on accelerometer)

********************/

String getTiltDirection(float pitch, float roll, float baselinePitch) {
  String direction = "";
  
  // Define the baseline as 30 degrees tilted up
  // float baselinePitch = -30.0; // Adjusting for 30 degrees up as the new baseline
  
  // Define the buffer region (deadzone) to avoid sensitivity to minor tilts
  float bufferRegion = 30.0; // Degrees
  
  // Adjust pitch relative to the new baseline
  pitch -= baselinePitch;
  
  if (pitch > bufferRegion) {
    direction += "Down ";
  } else if (pitch < -bufferRegion) {
    direction += "Up ";
  }
  
  if (roll > bufferRegion) {
    direction += "Right";
  } else if (roll < -bufferRegion) {
    direction += "Left";
  }
  
  if (direction == "") {
    direction = "Stable";
  }
  
  return direction;
}


float calibratePitch () {
  // Calibration logic to establish baseline pitch

  // Variables to store the sum of pitch readings and the final baseline pitch
  float pitchSum = 0.0;
  const int readings = 30; // Number of readings to take for averaging
  
  // Take multiple readings of the pitch over a short period
  for (int i = 0; i < readings; i++) {
      sensors_event_t event; 
      msa.getEvent(&event);
      

      double rawAX = event.acceleration.x;
      double rawAY = event.acceleration.y;
      double rawAZ = event.acceleration.z;

      float pitch = atan(event.acceleration.x / sqrt(event.acceleration.y * event.acceleration.y + event.acceleration.z * event.acceleration.z)) * (180.0 / M_PI);
      pitchSum += pitch; // Assuming readPitch() is your function to read the current pitch
      delay(100); // Short delay between readings to spread them out over time
}

  float new_pitch_base = pitchSum / readings;
  return new_pitch_base;
}

/***************

STATE BLUETOOTH - Variables needed for bluetooth connection and maintenance

***************/
bool is_bluetooth_connected = false;


void setup() {
  u8g2.begin();
  u8g2.setColorIndex(1);  // set the color to white

  // define pins for buttons
  // INPUT_PULLUP means the button is HIGH when not pressed, and LOW when pressed
  // since it´s connected between some pin and GND
  pinMode(BUTTON_UP_PIN, INPUT_PULLUP); // up button
  pinMode(BUTTON_SELECT_PIN, INPUT_PULLUP); // select button
  pinMode(BUTTON_DOWN_PIN, INPUT_PULLUP); // down button

  BTSerial.begin(9600);
  Serial.begin(9600);
  pinMode(micPin, INPUT);


  if (!msa.begin(MSA311_I2CADDR_DEFAULT, &Wire1)) {
      Serial.println("Failed to find MSA301 chip");
      while (1) { delay(10); }
 }
  Serial.println("MSA301 Found!");

  // Menu menu(u8g2, menu_items, NUM_ITEMS);
}


void loop() {

  u8g2.firstPage(); // required for page drawing mode for u8g library

  // if (spellrmenu.current_screen == "MENU") {
  //   sensors_event_t event; 
  //   msa.getEvent(&event);
  //   float pitch = atan(event.acceleration.x / sqrt(event.acceleration.y * event.acceleration.y + event.acceleration.z * event.acceleration.z)) * (180.0 / M_PI);
  //   float roll = atan(event.acceleration.y / sqrt(event.acceleration.x * event.acceleration.x + event.acceleration.z * event.acceleration.z)) * (180.0 / M_PI);
  //   String tiltDirection = getTiltDirection(pitch, roll, baselinePitch);

  //   // up and down buttons only work for the menu screen
  //   if (tiltDirection.startsWith("Down")) { // up button clicked - jump to previous menu item
  //     spellrmenu.item_selected = spellrmenu.item_selected - 1;
  //     button_up_clicked = 1; // set button to clicked to only perform the action once
  //     if (item_selected < 0) { // if first item was selected, jump to last item
  //       item_selected = NUM_ITEMS-1;
  //     }
  //     delay(500);
  //   }
  //     else if (tiltDirection.startsWith("Up")) { // down button clicked - jump to next menu item
  //       item_selected = item_selected + 1; // select next item
  //       button_down_clicked = 1; // set button to clicked to only perform the action once
  //       if (item_selected >= NUM_ITEMS) { // last item was selected, jump to first menu item
  //         item_selected = 0;
  //       }
  //       delay(500);
  //     }

  //     if ((digitalRead(BUTTON_UP_PIN) == HIGH) && (button_up_clicked == 1)) { // unclick 
  //       button_up_clicked = 0;
  //     }
  //     if ((digitalRead(BUTTON_DOWN_PIN) == HIGH) && (button_down_clicked == 1)) { // unclick
  //       button_down_clicked = 0;
  //     }


  // }

  if (current_screen == 0) { // MENU SCREEN
    sensors_event_t event; 
    msa.getEvent(&event);
    float pitch = atan(event.acceleration.x / sqrt(event.acceleration.y * event.acceleration.y + event.acceleration.z * event.acceleration.z)) * (180.0 / M_PI);
    float roll = atan(event.acceleration.y / sqrt(event.acceleration.x * event.acceleration.x + event.acceleration.z * event.acceleration.z)) * (180.0 / M_PI);

    String tiltDirection = getTiltDirection(pitch, roll, baselinePitch);



      // up and down buttons only work for the menu screen
      if (tiltDirection.startsWith("Down")) { // up button clicked - jump to previous menu item
        item_selected = item_selected - 1; // select previous item
        button_up_clicked = 1; // set button to clicked to only perform the action once
        if (item_selected < 0) { // if first item was selected, jump to last item
          item_selected = NUM_ITEMS-1;
        }
        delay(500);
      }
      else if (tiltDirection.startsWith("Up")) { // down button clicked - jump to next menu item
        item_selected = item_selected + 1; // select next item
        button_down_clicked = 1; // set button to clicked to only perform the action once
        if (item_selected >= NUM_ITEMS) { // last item was selected, jump to first menu item
          item_selected = 0;
        }
        delay(500);
      }

      if ((digitalRead(BUTTON_UP_PIN) == HIGH) && (button_up_clicked == 1)) { // unclick 
        button_up_clicked = 0;
      }
      if ((digitalRead(BUTTON_DOWN_PIN) == HIGH) && (button_down_clicked == 1)) { // unclick
        button_down_clicked = 0;
      }

  }

  if ((digitalRead(BUTTON_SELECT_PIN) == LOW) && (button_select_clicked == 0)) { // select button clicked, jump between screens
     button_select_clicked = 1; // set button to clicked to only perform the action once
     if (current_screen == 0) {current_screen = 1;} // menu items screen --> screenshots screen
     else {current_screen = 0;} // back to screen --> menu items screen
  }
  if ((digitalRead(BUTTON_SELECT_PIN) == HIGH) && (button_select_clicked == 1)) { // unclick 
    button_select_clicked = 0;
  }

  // set correct values for the previous and next items
  item_sel_previous = item_selected - 1;
  if (item_sel_previous < 0) {item_sel_previous = NUM_ITEMS - 1;} // previous item would be below first = make it the last
  item_sel_next = item_selected + 1;  
  if (item_sel_next >= NUM_ITEMS) {item_sel_next = 0;} // next item would be after last = make it the first


  do {
    ///////////////////RENDER/////////////////////////// RENDERING SCREEN STUFF BELOW HERE


    /********************
    current_screen 0
    RENDER MENU stuff here

    ********************/
    if (current_screen == 0) { // MENU SCREEN

      // selected item background
      u8g2.drawBitmap(0, 22, 128/8, 21, bitmap_item_sel_outline);

      // draw previous item as icon + label
      u8g2.setFont(u8g_font_7x14);
      u8g2.drawStr(25, 15, menu_items[item_sel_previous]); 
      u8g2.drawBitmap( 4, 2, 16/8, 16, bitmap_icons[item_sel_previous]);          

      // draw selected item as icon + label in bold font
      u8g2.setFont(u8g_font_7x14B);    
      u8g2.drawStr(25, 15+20+2, menu_items[item_selected]);   
      u8g2.drawBitmap( 4, 24, 16/8, 16, bitmap_icons[item_selected]);     

      // draw next item as icon + label
      u8g2.setFont(u8g_font_7x14);     
      u8g2.drawStr(25, 15+20+20+2+2, menu_items[item_sel_next]);   
      u8g2.drawBitmap( 4, 46, 16/8, 16, bitmap_icons[item_sel_next]);  

      // draw scrollbar background
      u8g2.drawBitmap(128-8, 0, 8/8, 64, bitmap_scrollbar_background);

      // draw scrollbar handle
      u8g2.drawBox(125, 64/NUM_ITEMS * item_selected, 3, 64/NUM_ITEMS); 


      if (is_bluetooth_connected) {
        // draw bluetooth connected
        u8g2.drawBitmap(128-16-4, 1, 16/8, 16, bluetooth_logo);    
      }
    }

    /********************
    current_screen 1, item_selected 3
    RENDER SWR in-game graphics, the plus sign + thing and the 

    ********************/
    else if (current_screen == 1 && item_selected == 3) {
      // SWR GAME GRAPHICS HERE
      if (swr_waiting) {
          u8g2.setFont(u8g_font_7x14B);
          u8g2.setFontMode(1); // Enable transparent mode
          int textWidth = u8g2.getStrWidth("+");
          int x = (128 - textWidth) / 2; // Calculate x-coordinate to center the text
          int y = 20; // Adjust this value to vertically position the text
          u8g2.drawStr(x, y, "+");
      } else {
        u8g2.setFont(u8g_font_7x14B);
        u8g2.setFontMode(1); // Enable transparent mode
        int textWidth = u8g2.getStrWidth(words[wordIndex]);
        int x = (128 - textWidth) / 2; // Calculate x-coordinate to center the text
        int y = 35; // Adjust this value to vertically position the text
        u8g2.drawStr(x, y, words[wordIndex]);

        u8g2.setFont(u8g2_font_5x7_tr); // Set a smaller font
        u8g2.drawStr(x + 5, 10, "REAL"); // Draw "LEFT" in the lower-left corner
        u8g2.drawStr(x + 5, 62, "FAKE"); // Draw "RIGHT" in the lower-right corner
      }
    }

    /********************
    current_screen 1, item_selected 4
    RENDER BLUETOOTH thing

    ********************/
    else if (current_screen == 1 && item_selected == 4) {
      if (!is_bluetooth_connected) {
        u8g2.setFont(u8g_font_7x14B);
        u8g2.setFontMode(1); // Enable transparent mode
        int textWidth = u8g2.getStrWidth("Connecting...");
        int x = (128 - textWidth) / 2; // Calculate x-coordinate to center the text
        int y = 62; // Adjust this value to vertically position the text
        
        // WIFI LOADING ANIMATION
        u8g2.clearBuffer();
        u8g2.drawXBMP(39, 7, 50, 50, wifi_epd_bitmap_allArray[wifi_counter]);
        u8g2.drawStr(x, y, "Connecting...");
        u8g2.sendBuffer();
        // delay(100);
        wifi_counter = (wifi_counter + 1) % wifi_epd_bitmap_allArray_LEN;
      } else {
        u8g2.setFont(u8g_font_7x14B);
        u8g2.setFontMode(1); // Enable transparent mode
        int textWidth = u8g2.getStrWidth("Connected!");
        int x = (128 - textWidth) / 2; // Calculate x-coordinate to center the text
        int y = 35; // Adjust this value to vertically position the text
        u8g2.drawStr(x, y, "Connected!");
      }
    }

    /********************
    current_screen 1 item_selected 0
    RENDER Listening..... graphic

    ********************/
    else if (current_screen == 1 && item_selected == 0) {
        u8g2.setFont(u8g_font_7x14B);
        u8g2.setFontMode(1); // Enable transparent mode
        int textWidth = u8g2.getStrWidth("Listening...");
        int x = (128 - textWidth) / 2; // Calculate x-coordinate to center the text
        int y = 35; // Adjust this value to vertically position the text
        u8g2.drawStr(x, y, "Listening...");
    }

    /********************
    current_screen 1 item_selected 5
    RENDER Calibration

    ********************/
    else if (current_screen == 1 && item_selected == 5) {
        u8g2.setFont(u8g_font_7x14B);
        u8g2.setFontMode(1); // Enable transparent mode
        int textWidth = u8g2.getStrWidth("Hold Still...");
        int x = (128 - textWidth) / 2; // Calculate x-coordinate to center the text
        int y = 35; // Adjust this value to vertically position the text
        u8g2.drawStr(25, 25, "Hold Still...");
    }
    /********************
    current_screen 1    DEFAULT SCREEN
    RENDER whatever the bitmap is supposed to be

    ********************/
    else if (current_screen == 1) {
      u8g2.drawBitmap( 0, 0, 128/8, 64, bitmap_screenshots[item_selected]); // draw screenshot
    }

    /********************
    current_screen 3
    RENDER output of transcribe word, different spelling modes

    ********************/
    else if (current_screen == 3) {   // DISPLAY WORDS SCREEN
      if (spelling_mode == 0) {
        // ANIMATED LETTERS HERE
        SSWORD = translated_word.c_str();

        const unsigned char** frames = getBitmapArray(SSWORD[currentLetter]);
        int frameCount = getBitmapArrayLength(SSWORD[currentLetter]);

        u8g2.clearBuffer(); // Clear the display buffer
        u8g2.drawXBMP((128 - 1 - 64) / 2, 3, 64, 64, frames[frameCounter]); // Draw the current frame
        u8g2.sendBuffer();
        delay(50);

        frameCounter++;
        if (frameCounter >= frameCount) {
            frameCounter = 0;
            currentLetter = (currentLetter + 1) % SSWORD.length();
        }

        // if currentLetter is LAST, then exit!
      } else if (spelling_mode == 1) {
        // REGULAR WORD SPELLING MODE: JUST DISPLAY THE TEXT ON THE SCREEN
        u8g2.setFont(u8g_font_7x14B);
        u8g2.setFontMode(1); // Enable transparent mode

        // ANIMATED LETTERS HERE
        int textWidth = u8g2.getStrWidth(translated_word.c_str());
        int x = (128 - textWidth) / 2; // Calculate x-coordinate to center the text
        int y = 35; // Adjust this value to vertically position the text
        u8g2.drawStr(x, y, translated_word.c_str());
      }

      //TODO TODO TODO MODE 3: cycle through each letter, and then for the letter that's being spelled, you make it BIGGER and show how it's air-written
    }
    
    /********************
    current_screen 4
    RENDER SWR GAME (wordquest)

    ********************/
    else if (current_screen == 4) {   
      u8g2.drawStr(25, 15+10+2, ("Correct: " + String(correct)).c_str());
      u8g2.drawStr(25, 15+30+2, ("Incorrect: " + String(incorrect)).c_str());
      // correct = 0, incorrect = 0;
      wordIndex = 0;
    }
  } while ( u8g2.nextPage() ); // required for page drawing mode with u8g library






    ///////////////////LOGIC/////////////////////////// Internal logic and state stuff for all screens here



/*********************************

Transcribe LOGIC: listening to the microphone and handling serial shit

********************************/
  if (current_screen == 1 && item_selected == 0) {
    Serial.println("GO");
    BTSerial.println("GO");

    inTranscribeMode = true;

    // two button presses:
    // while (inTranscribeMode) {
    //   int micValue = analogRead(micPin);  // Read the microphone level
    //   Serial.println(micValue);           // Print the microphone level to the Serial Monitor
    //   BTSerial.println(micValue);           // Print the microphone level to the Serial Monitor
    //   delay(10);                          // Adjust based on your desired sampling rate
    //   if ((digitalRead(BUTTON_SELECT_PIN) == LOW) && (button_select_clicked == 0)) { // select button clicked, jump between screens
    //     button_select_clicked = 1; // set button to clicked to only perform the action once
    //     inTranscribeMode = false;
    //   }
    //   if ((digitalRead(BUTTON_SELECT_PIN) == HIGH) && (button_select_clicked == 1)) { // unclick 
    //       button_select_clicked = 0;
    //   }
    // }

    // one holding press:
    // Check if the select button is pressed
    if (digitalRead(BUTTON_SELECT_PIN) == LOW) {
      inTranscribeMode = true; // Enter transcribe mode
    }

    while (inTranscribeMode) {
      int micValue = analogRead(micPin); // Read the microphone level

      Serial.println(micValue); // Print the microphone level to the Serial Monitor
      BTSerial.println(micValue); // Print the microphone level to the Serial Monitor

      // Check if the select button is released
      if (digitalRead(BUTTON_SELECT_PIN) == HIGH) {
        inTranscribeMode = false; // Exit transcribe mode
      }

      delay(10); // Adjust based on your desired sampling rate
    }

    

    Serial.println("STOP");
    BTSerial.println("STOP");
    // 2 SECOND DELAY BETWEEN
    delay(2000); 

    // Check if something is available in Serial
    while (BTSerial.available()) {
      char inChar = (char)BTSerial.read();
      inputString += inChar;
      if (inChar == '\n') {
        stringComplete = true;
      }
    }

    if (stringComplete) {
      // Check if the string starts with "RES: "
      if (inputString.startsWith("RES: ")) {
        // Extract the word after "RES: "
        String word = inputString.substring(5);  // Skip "RES: " (4 chars) and one space
        translated_word = word;
      }
      inputString = "";        // Clear the input string for the next read
      stringComplete = false;  // Reset the flag
    }
    
    current_screen = 3;
    u8g2.clearBuffer();
    u8g2.sendBuffer();
  }


/*********************************

SWR GAME WORD LOGIC (wordquest)

********************************/
  if (current_screen == 1 && item_selected == 3) {
    // ENDING THE GAME
    if (wordIndex == numWords - 1) {
        current_screen = 4;
    }

    else {
      // STARTING NEW GAME
      if (wordIndex == 0) {
          correct = 0, incorrect = 0;
      }

      if (swr_waiting) {
          // Calibration logic to establish baseline pitch

          // Calculate the average pitch to establish the baseline
          baselinePitch = calibratePitch();

          // Now baselinePitch holds the average pitch during calibration
          // You can use this as your reference pitch for future tilt detection

          swr_waiting = false; // Exit calibration mode

      } else {
        Serial.println(baselinePitch);
        String fake_or_real = "";

        // Wait for serial input
        while (fake_or_real.length() == 0) {

          /////////////// INSERT IMU LOGIC HERE
          // IMU: if it's left tilt, println "left\n", otherwise print "right\n"
          sensors_event_t event; 
          msa.getEvent(&event);

          float pitch = atan(event.acceleration.x / sqrt(event.acceleration.y * event.acceleration.y + event.acceleration.z * event.acceleration.z)) * (180.0 / M_PI);
          float roll = atan(event.acceleration.y / sqrt(event.acceleration.x * event.acceleration.x + event.acceleration.z * event.acceleration.z)) * (180.0 / M_PI);

          String tiltDirection = getTiltDirection(pitch, roll, baselinePitch);


          if (tiltDirection.startsWith("Up")) {
            fake_or_real = "fake"; // left = UP
          } else if (tiltDirection.startsWith("Down")) {
            fake_or_real = "real"; // right = DOWN
          }
        }

        // Check if the response is correct
        if (fake_or_real == "real") {
          if (wordIsReal[wordIndex]) {
            correct = correct + 1;
          } else {
            incorrect = incorrect + 1;
          }
        } else if (fake_or_real == "fake") {
          if (!wordIsReal[wordIndex]) {
            correct = correct + 1;
          } else {
            incorrect = incorrect + 1;
          }
        }
        // Serial.println(fake_or_real);
        // Serial.println(correct);
        // Serial.println(incorrect);

        fake_or_real = ""; // Reset the fake_or_real string
        wordIndex = (wordIndex + 1) % numWords;
        swr_waiting = true;
      }
    }
  }


  /*********************************

  BLUETOOTH LOGIC

  ********************************/
  if (current_screen == 1 && item_selected == 4) { // Bluetooth screen
    is_bluetooth_connected = BTSerial.available(); // Update connection status
  }

  /*********************************

  CALIBRATING LOGIC

  ********************************/
  else if (current_screen == 1 && item_selected == 5) { // Bluetooth screen
    baselinePitch = calibratePitch();
    current_screen = 0;
  }

}



// Mock function to get bitmap array for a character
const unsigned char** getBitmapArray(char letter) {
    switch (letter) {
        case 'B': return B_epd_bitmap_allArray;
        case 'C': return C_epd_bitmap_allArray;
        case 'D': return D_epd_bitmap_allArray;
        case 'E': return E_epd_bitmap_allArray;
        case 'F': return F_epd_bitmap_allArray;
        case 'G': return G_epd_bitmap_allArray;
        case 'H': return H_epd_bitmap_allArray;
        case 'I': return I_epd_bitmap_allArray;
        case 'J': return J_epd_bitmap_allArray;
        case 'K': return K_epd_bitmap_allArray;
        case 'L': return L_epd_bitmap_allArray;
        default: return L_epd_bitmap_allArray;
    }
}

// Mock function to get bitmap array length for a character
int getBitmapArrayLength(char letter) {
    switch (letter) {
        case 'B':
            return B_epd_bitmap_allArray_LEN;
        case 'C':
            return C_epd_bitmap_allArray_LEN;
        case 'D':
            return D_epd_bitmap_allArray_LEN;
        case 'E':
            return E_epd_bitmap_allArray_LEN;
        case 'F':
            return F_epd_bitmap_allArray_LEN;
        case 'G':
            return G_epd_bitmap_allArray_LEN;
        case 'H':
            return H_epd_bitmap_allArray_LEN;
        case 'I':
            return I_epd_bitmap_allArray_LEN;
        case 'J':
            return J_epd_bitmap_allArray_LEN;
        case 'K':
            return K_epd_bitmap_allArray_LEN;
        case 'L':
            return L_epd_bitmap_allArray_LEN;
        default: return 1;  // Length of default bitmap
    }
}
