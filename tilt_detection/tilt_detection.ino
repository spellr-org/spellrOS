#include <Adafruit_MSA301.h>
#include <Adafruit_Sensor.h>
#include <ArrayList.h>
#include <float.h> 

Adafruit_MSA311 msa;

const int numReadings = 10;
double rawAXs[numReadings]; // the readings from the analog input
double rawAYs[numReadings];
double rawAZs[numReadings];

const double magnitude_thres = 10;
const double spike_thres = 20;
bool isSwiping = false;

long startSwipeTime = 0;
long millis_thres = 100;

long potentialStopTime = 0;
long potentialStopThres = 70;
long totalSwipeThres = 100;

double swipeDataPY[20];
double swipeDataMA[20];
int swipeIndex = 0;

const int swipeHistory = 200;

double AYs[swipeHistory];
double VYs[swipeHistory-1];
double PYs[2];

bool swipeUp = false;


void pushReading(double arr[], double val) {
  // Shift all the readings one position down
  for (int i = sizeof(arr) - 1; i > 0; i--) {
    arr[i] = arr[i - 1];
  }
  arr[0] = val;
}

double average(double arr[]) {
  double total = 0; // Initialize total sum of the array elements
  for(int i = 0; i < sizeof(arr); i++) {
    total += arr[i]; // Sum up all the elements
  }
  double avg = total / sizeof(arr); // Calculate the average
  return avg; // Return the average value
}

double sum(double arr[]) {
  double total = 0; // Initialize total sum of the array elements
  for(int i = 0; i < sizeof(arr); i++) {
    total += arr[i]; // Sum up all the elements
  }
  return total;
}

double magnitude(double x, double y, double z){
  return sqrt(x*x + y*y + z*z);
}

int maxIndex(double arr[]) {
  double max = DBL_MIN;
  double index = -1;
  for(int i = 0; i < sizeof(arr); i++) {
    if (arr[i] > max) {
      index = i;
      max = arr[i];
    }
  }
  return index;
}

int maxValue(double arr[]) {
  double max = DBL_MIN;
  double index = -1;
  for(int i = 0; i < sizeof(arr); i++) {
    if (arr[i] > max) {
      index = i;
      max = arr[i];
    }
  }
  return max;
}

void clear(double arr[]) {
  for (int i = 0; i < sizeof(arr); i++) {
    arr[i] = 0;
  }
}


void setup() {
  Serial.begin(9600);

  if (!msa.begin(MSA311_I2CADDR_DEFAULT, &Wire1)) {
    Serial.println("Failed to find MSA301 chip");
    while (1) { delay(10); }
  }
  Serial.println("MSA301 Found!");

  clear(rawAXs); clear(rawAYs); clear(rawAZs);
  clear(AYs); clear(VYs); clear(PYs);
}

void loop() {
  sensors_event_t event; 
  msa.getEvent(&event);

  double rawAX = event.acceleration.x;
  double rawAY = event.acceleration.y;
  double rawAZ = event.acceleration.z;

  pushReading(rawAXs, rawAX);
  pushReading(rawAYs, rawAY);
  pushReading(rawAZs, rawAZ);

  double avgAX = average(rawAXs);
  double avgAY = average(rawAYs);
  double avgAZ = average(rawAZs);

  double AX = event.acceleration.x - avgAX;
  double AY = event.acceleration.y - avgAY;
  double AZ = event.acceleration.z - avgAZ;

  pushReading(AYs, AY);
  double VY = sum(AYs);
  pushReading(VYs, VY);
  double PY = sum(VYs);
  pushReading(PYs, PY);

  double magnitudeA = magnitude(AX, AY, AZ);

  // Serial.print("AY:"); Serial.print(AY); Serial.print(",");
  // Serial.print("VY:"); Serial.print(VY); Serial.print(",");
  // Serial.print("PY:"); Serial.print(PY); Serial.print(",");
  // Serial.print("MA:"); Serial.println(magnitudeA);

  if (magnitudeA >= magnitude_thres && !isSwiping) {
    startSwipeTime = millis();
    isSwiping = true;
    swipeUp = (PY <= 0);
    swipeIndex = 0;
  }

  if (isSwiping && magnitudeA >= magnitude_thres) {
    potentialStopTime = millis();
  }

  if (isSwiping && (magnitudeA < magnitude_thres) && (millis() >= potentialStopTime + potentialStopThres)) {
    isSwiping = false;

    if (swipeUp && (maxValue(swipeDataMA) >= spike_thres) && (millis() >= startSwipeTime + totalSwipeThres)) {
      Serial.println("Swiped Up");
    } else {
      Serial.println("Swiped Down");
    }

    // int index = maxIndex(swipeDataMA);
    // double swipeValue = swipeDataPY[index];

    // if (swipeValue >= 0) {
    //   // Serial.println("Swiped Down");
    // } else {
    //   // Serial.println("Swiped Up");
    // }

    clear(swipeDataPY);
    clear(swipeDataMA);

    // Serial.println(millis() - startReadingTime);
    // Serial.println(swipeValue);
  }

  if (isSwiping) {
    swipeDataPY[swipeIndex] = PY;
    swipeDataMA[swipeIndex] = magnitudeA;
    swipeIndex = swipeIndex + 1;
  }
 
  delay(30); 
}
