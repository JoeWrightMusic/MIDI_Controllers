//_______________________LIBS
#include <Wire.h>
#include <Adafruit_LIS3MDL.h>
#include <Adafruit_Sensor.h>
#include "MIDIUSB.h"
#include <Ewma.h>


Adafruit_LIS3MDL mag;

// https://github.com/LennartHennigs/Button2
#include "Button2.h"
Button2 button;


//_______________________CONSTANTS
#define BUTTON_PIN 5
#define LED_PIN 17
#define LED_LOOP_LEN 100
// #define DEBUG_GYRO
// #define DEBUG_BTN
// #define DEBUG_MODE 
// #define DEBUG_SETRANGE
// #define DEBUG_MAP
// #define DEBUG_MIDI


//_______________________VARIABLES
float deadZonePortion = 0.25;
float deadZoneSize[] = {0,0,0};
float span[] = {0,0,0};
int endMap=1;

float x = 0;
float y = 0;
float z = 0;

float xRange[] = {-100,100};
float yRange[] = {-100,100};
float zRange[] = {-100,100};
int btn = 0;
int ledTick = 0;
float prevMIDI[] = {-1,-1,-1};
int doSend=0;

int mode = 0;
/* MODES 
-1: start of gesture map mode 
0: (back to) performance mode 
1: map X
2: map Y
3: map Z
*/

//_______________________FUNCTIONS 
void click(){
  if(mode>=0){mode=-1;}
  else {mode=mode+1;}

  #ifdef DEBUG_BTN
  Serial.println("click");
  #endif
}

void longClick(){
  if(mode<=0){mode=1;
  } else if(mode>0){mode=mode+1;}

  if(mode==4){mode=0;};

  #ifdef DEBUG_BTN
  Serial.println("long click");
  #endif
}

void ledOn(){
  digitalWrite(LED_PIN, LOW);
}
void ledOff(){
  digitalWrite(LED_PIN, HIGH);
}

void ledBlink1(){
  if(ledTick<(LED_LOOP_LEN/8)){
    ledOn();
  } else {
    ledOff();
  }
  ledTick = (ledTick+1) % LED_LOOP_LEN;
}

void ledBlink2(){
  if(ledTick<(LED_LOOP_LEN/8)){
    ledOn();
  } else if ((ledTick>(LED_LOOP_LEN/8)*2) && ledTick<((LED_LOOP_LEN/8)*3)){
    ledOn();
  } else {
    ledOff();
  }
  ledTick = (ledTick+1) % LED_LOOP_LEN;
}

void ledBlink3(){
  if(ledTick<(LED_LOOP_LEN/8)){
    ledOn();
  } else if ((ledTick>(LED_LOOP_LEN/8)*2) && ledTick<((LED_LOOP_LEN/8)*3)){
    ledOn();
  } else if ((ledTick>(LED_LOOP_LEN/8)*4) && ledTick<((LED_LOOP_LEN/8)*5)){
    ledOn();
  } else {
    ledOff();
  }
  ledTick = (ledTick+1) % LED_LOOP_LEN;
}

void sendCC(byte control, byte value) {
  midiEventPacket_t event = {0x0B, 0xB0 | 0, control, value};
  MidiUSB.sendMIDI(event);
}

void getGestureRange(){
  int getSpan=0;
  if(x<xRange[0] || endMap==1){xRange[0]=x; getSpan=1;}
  if(x>xRange[1] || endMap==1){xRange[1]=x; getSpan=1;}

  if(y<yRange[0] || endMap==1){yRange[0]=y; getSpan=1;}
  if(y>yRange[1] || endMap==1){yRange[1]=y; getSpan=1;}

  if(z<zRange[0] || endMap==1){zRange[0]=z; getSpan=1;}
  if(z>zRange[1] || endMap==1){zRange[1]=z; getSpan=1;}
  
  if(getSpan==1){
    span[0] = xRange[1]-xRange[0];
    span[1] = yRange[1]-yRange[0];
    span[2] = zRange[1]-zRange[0];

    deadZoneSize[0] = span[0]*deadZonePortion;
    deadZoneSize[1] = span[1]*deadZonePortion;
    deadZoneSize[2] = span[2]*deadZonePortion;

    getSpan=0;
  }

  #ifdef DEBUG_SETRANGE 
    Serial.print("z: ");
    Serial.print(z);
    Serial.print("\t zR: ");
    Serial.print(zRange[0]);
    Serial.print("  ");
    Serial.print(zRange[1]);
    Serial.print("\t span:");
    Serial.println(span[2]);
  #endif

  endMap=0;
}

void mapGestures(){
  x = constrain(x, xRange[0]+deadZoneSize[0], xRange[1]-deadZoneSize[0]);
  y = constrain(y, yRange[0]+deadZoneSize[1], yRange[1]-deadZoneSize[1]);
  z = constrain(z, zRange[0]+deadZoneSize[2], zRange[1]-deadZoneSize[2]);

  x = map(x,xRange[0]+deadZoneSize[0], xRange[1]-deadZoneSize[0],0.0, 127.0);
  y = map(y,yRange[0]+deadZoneSize[1], yRange[1]-deadZoneSize[1],0.0, 127.0);
  z = map(z,zRange[0]+deadZoneSize[2], zRange[1]-deadZoneSize[2],0.0, 127.0);

  #ifdef DEBUG_MAP //only shows X
    Serial.print("xMap: ");
    Serial.print(x);
    Serial.print("\t yMap: ");
    Serial.print(y);
    Serial.print("\t zMap: ");
    Serial.println(z);
  #endif
}

void sendMIDI(){
  if(prevMIDI[0]!=x){
    sendCC(1,x);
    doSend=1;
  }
  if(prevMIDI[1]!=y){
    sendCC(2,y);
    doSend=1;
  }
  if(prevMIDI[2]!=z){
    sendCC(3,z);
    doSend=1;
  }

  if(doSend==1){
    MidiUSB.flush();
    #ifdef DEBUG_MIDI
      Serial.print("x: ");
      Serial.print(x);
      Serial.print("  y:  ");
      Serial.print(y);
      Serial.print("  z:  ");
      Serial.println(z);
    #endif
    doSend=0;
  }

  prevMIDI[0]=x;
  prevMIDI[1]=y;
  prevMIDI[2]=z;
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
  button.setLongClickTime(2000);
  // button.setDoubleClickTime(400);
}


void loop() {
  mag.read();
  x = mag.x;
  y = mag.y;
  z = mag.z; 

  button.loop();

  // mode logic
  switch (mode) {
    case -1:
      getGestureRange();
      ledOn();
      break;

    case 0:
      ledOff();
      mapGestures();
      sendMIDI();
      endMap=1;
      break;
      
    case 1:
      sendCC(1, 64);
      MidiUSB.flush();
      ledBlink1();
      delay(10);
      endMap=1;
      break;

    case 2:
      sendCC(2, 64);
      MidiUSB.flush();
      ledBlink2();
      delay(10);
      endMap=1;
      break;

    case 3:
      sendCC(3, 64);
      MidiUSB.flush();
      ledBlink3();
      delay(10);
      endMap=1;
      break;

    default:
      ledOff();
      endMap=1;
      break;
  }
  


  #ifdef DEBUG_GYRO
    Serial.print("\nX:  "); Serial.print(x); 
    Serial.print("  \tY:  "); Serial.print(y); 
    Serial.print("  \tZ:  "); Serial.println(z); 
  #endif
  
  #ifdef DEBUG_MODE
  Serial.println(mode);
  #endif

  delay(10); 
  // Serial.println();
}
