//_______________________LIBS
#include <Wire.h>
#include <Adafruit_LIS3MDL.h>
#include <Adafruit_Sensor.h>
Adafruit_LIS3MDL mag;

// https://github.com/LennartHennigs/Button2
#include "Button2.h"
Button2 button;


//_______________________CONSTANTS
#define BUTTON_PIN 5
// #define DEBUG_GYRO
#define DEBUG_BTN

//_______________________VARIABLES
int x = 0;
int y = 0;
int z = 0;
int btn = 0;


//_______________________FUNCTIONS 
void click(){
  // #ifdef DEBUG_BTN
  Serial.println("click");
  // #endif
}

void longClick(){
  // #ifdef DEBUG_BTN
  Serial.println("long click");
  // #endif
}



//_______________________SETUP
void setup() {
  Serial.begin(115200);   

  mag.begin_I2C();
  delay(100);
  mag.setPerformanceMode(LIS3MDL_ULTRAHIGHMODE);
  mag.setOperationMode(LIS3MDL_CONTINUOUSMODE);
  mag.setDataRate(LIS3MDL_DATARATE_560_HZ);
  mag.setRange(LIS3MDL_RANGE_4_GAUSS);
  delay(100);

  button.begin(BUTTON_PIN);
  delay(100);
  button.setClickHandler(click);
  button.setLongClickHandler(longClick);
  // button.setLongClickTime(1000);
  // button.setDoubleClickTime(400);
}

void loop() {
  mag.read();
  x = mag.x;
  y = mag.y;
  z = mag.z; 

  button.loop();

  #ifdef DEBUG_GYRO
    Serial.print("\nX:  "); Serial.print(x); 
    Serial.print("  \tY:  "); Serial.print(y); 
    Serial.print("  \tZ:  "); Serial.println(z); 
  #endif

  delay(10); 
  // Serial.println();
}
