#include <Wire.h>
#include <Adafruit_MSA301.h>
#include <Adafruit_Sensor.h>

Adafruit_MSA301 msa;

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10); // Wait for the serial console to open

  Serial.println("Adafruit MSA301 test!");

  if (!msa.begin(MSA311_I2CADDR_DEFAULT, &Wire1)) { // Default I2C address & Wire used
    Serial.println("Failed to find MSA301 chip");
    while (1) { delay(10); }
  }
  Serial.println("MSA301 Found!");
}

// Determine the direction of tilt based on pitch and roll
String getTiltDirection(float pitch, float roll) {
  String direction = "";
  
  // Define the baseline as 30 degrees tilted up
  float baselinePitch = -30.0; // Adjusting for 30 degrees up as the new baseline
  
  // Define the buffer region (deadzone) to avoid sensitivity to minor tilts
  float bufferRegion = 18.0; // Degrees
  
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

void loop() {
  sensors_event_t event; 
  msa.getEvent(&event);

  float pitch = atan(event.acceleration.x / sqrt(event.acceleration.y * event.acceleration.y + event.acceleration.z * event.acceleration.z)) * (180.0 / M_PI);
  float roll = atan(event.acceleration.y / sqrt(event.acceleration.x * event.acceleration.x + event.acceleration.z * event.acceleration.z)) * (180.0 / M_PI);

  String tiltDirection = getTiltDirection(pitch, roll);

  Serial.println(tiltDirection);

  delay(500); // Delay a bit to make the output more readable
}
