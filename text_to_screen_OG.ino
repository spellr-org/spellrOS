#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SoftwareSerial.h>

#define SCREEN_WIDTH 128  // OLED display width, in pixels
#define SCREEN_HEIGHT 64  // OLED display height, in pixels
#define OLED_RESET -1     // Reset pin # (or -1 if sharing Arduino reset pin)

const int buttonPin = 2;  // Button connected to pin 2
const int micPin = A0;    // Microphone output connected to A0
const int rxPin = 7;
const int txPin = 8;

bool isRecording = false;            // Recording state
bool lastButtonState = HIGH;         // Last state of the button (assuming pull-up)
unsigned long lastDebounceTime = 0;  // Last time the button state changed
unsigned long debounceDelay = 50;    // Debounce delay in milliseconds

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

SoftwareSerial BTSerial(rxPin, txPin); // RX | TX

void setup() {
 pinMode(buttonPin, INPUT_PULLUP);  // Set the button as an input with an internal pull-up resistor
 pinMode(micPin, INPUT);            // Set the microphone pin as an input

 pinMode(rxPin, INPUT);
 pinMode(txPin, OUTPUT);

 Serial.begin(9600);
 // Wait for serial to initialize.
 while (!Serial) {
   ;  // wait for serial port to connect. Needed for native USB port only
 }
 Serial.println("initialized");

 BTSerial.begin(9600);
 while(!BTSerial) {
   ;
 }
 Serial.println("BT initialized");
 BTSerial.println("BT initialized");

 if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {  // Check your display's I2C address
   Serial.println(F("SSD1306 allocation failed"));
   BTSerial.println(F("SSD1306 allocation failed"));
   for (;;)
     ;  // Don't proceed, loop forever
 }

 display.display();
 delay(2000);  // Pause for 2 seconds
 display.clearDisplay();
 display.setTextSize(1);               // Normal 1:1 pixel scale
 display.setTextColor(SSD1306_WHITE);  // Draw white text
}

String inputString = "";      // a String to hold incoming data
bool stringComplete = false;  // whether the string is complete


void loop() {
 handleVoltageDetection();
 if (isRecording) {
   int micValue = analogRead(micPin);  // Read the microphone level
   Serial.println(micValue);           // Print the microphone level to the Serial Monitor
   BTSerial.println(micValue);
   delay(10);                          // Adjust based on your desired sampling rate
 }

 displaySerialInput();
}

void handleVoltageDetection() {
 bool currentVoltageState = digitalRead(buttonPin);  // Read the current state of pin 2

 // If pin 2 is HIGH (3V detected), start recording. Otherwise, stop.
 if (currentVoltageState == LOW && !isRecording) {
   isRecording = true;
   Serial.println("GO");
   BTSerial.println("GO");
 } else if (currentVoltageState == HIGH && isRecording) {
   isRecording = false;
   Serial.println("STOP");
   BTSerial.println("STOP");
 }
}

void displaySerialInput() {
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
     displayText(word.trim());                // Trim and display the word
   }
   inputString = "";        // Clear the input string for the next read
   stringComplete = false;  // Reset the flag
 }
}

void displayText(String text) {
 // This function now expects just the word to display,
 // so the checking and extraction part is removed from here.
 display.clearDisplay();               // Clear the display for fresh text
 display.setTextSize(1);               // Normal 1:1 pixel scale
 display.setTextColor(SSD1306_WHITE);  // Draw white text
 display.setCursor(0, 0);              // Start at top-left corner
 display.println(text);
 display.display();  // Actually draw the text on the screen
}