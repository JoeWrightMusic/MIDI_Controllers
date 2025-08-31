/*
OPERATION OF MoCap_Claw 

claw will spit out continuous gyro xyz data on cc 10,11,12 
index, mid, ring and small finger when pressed on 13->16

thumb will find min/max values while thumb is held down, use this
to set a gesture range which will be mapped to 0->127 on xyz axes.
No midi will be sent from gyro during mapping.

while thumb held down, index, mid and ring fingers will send dummy
x/y/z values for mapping in ableton or similar: 
index->x, mid->y, ring->z


N.B - LOTS of the code here could be made more efficient! 

*/


// Libraries for the MPU6050, MPR121, Weighted Average Filter, & MIDIUSB
// Install these from the libraries tab in the Arduino IDE
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
#include "MIDIUSB.h"
#include <Ewma.h>
#include "Adafruit_MPR121.h"

// Variables etc. for MPR121...
#ifndef _BV
#define _BV(bit) (1 << (bit)) 
#endif

// debugging on/off...
// #define DEBUG_TOUCHES;
// #define DEBUG_GYRO;
// #define DEBUG_RANGE;

uint16_t lasttouched = 0;//last cap sensor touched
uint16_t currtouched = 0;//current capsenseor touched
int presses[] = { 0, 0, 0, 0, 0 };//array to store on/off state of capsensors
int pressThreshold = 5;
//re-number these to re-order touches if needed.
int thumb = 2; 
int index = 0;
int middle = 4;
int ring = 3;
int small = 1;
int fingPreState[] = {0,0,0,0}; 


// Library for MPR121
Adafruit_MPR121 cap = Adafruit_MPR121();

// Library for MPU6050
Adafruit_MPU6050 mpu;

// Weighted average filters for our gyro data
Ewma xFilter(0.1); 
Ewma yFilter(0.1); 
Ewma zFilter(0.1); 

// Variables for xyz values
// for current value
float x = 0;
float y = 0;
float z = 0;
// for minimums
float xMin = 0;
float yMin = 0;
float zMin = 0;
// for maximums 
float xMax = 0;
float yMax = 0;
float zMax = 0;
// this variable will be 1 the first time the code runs
int setMinMax = 1; 
// previous xyz values 
float xPrev = 0;
float yPrev = 0;
float zPrev = 0;

// flush will be used to trigger MIDIusb.flush when 
// cc messages are ready
int flush = 0;

// map mode
int mapMode = 0;
int mapModePre = 0;
int cooldown = 0;

// Function for sending a MIDI CC message
void sendCC(byte control, byte value) {
  midiEventPacket_t event = {0x0B, 0xB0 | 0, control, value};
  MidiUSB.sendMIDI(event);
}

//reset min/max values for x y z 
void resetGyroMinMax(){
    xMin = x;
    xMax = xMin;

    yMin = y;
    yMax = yMin;

    zMin = z;
    zMax = zMin;
    //set first to 0 so that this section doesn't run again
    setMinMax = 0;
}

//get min max in map mode
void getRange(){
  // if new min or maximum, store the value
  if(x<=xMin){xMin = x;};
  if(x>=xMax){xMax = x;};
  if(y<=yMin){yMin = y;};
  if(y>=yMax){yMax = y;};
  if(z<=zMin){zMin = z;};
  if(z>=zMax){zMax = z;};
}

// Setup function, runs before the main loop starts
void setup(void) {
  // Setup serial communication
  Serial.begin(115200);
  // Wait a bit...
  delay(5000);
 
  // // Try to initialize MPU6050!
  if (!mpu.begin()) {
    //will wait forever until mpu connects
    while (1) {
      delay(10);
    }
  }
  // // Try to initialize MPR121
  if (!cap.begin(0x5A)) {
    //will wait forever until mpr121 connects
    while (1);
    delay(10);
  }

  cap.setAutoconfig(true);
  // MPU SETTINGS
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  // Serial.print("Accelerometer range set to: ");
  // switch (mpu.getAccelerometerRange()) {
  // case MPU6050_RANGE_2_G:
  //   // Serial.println("+-2G");
  //   break;
  // case MPU6050_RANGE_4_G:
  //   // Serial.println("+-4G");
  //   break;
  // case MPU6050_RANGE_8_G:
  //   // Serial.println("+-8G");
  //   break;
  // case MPU6050_RANGE_16_G:
  //   // Serial.println("+-16G");
  //   break;
  // }
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  // Serial.print("Gyro range set to: ");
  // switch (mpu.getGyroRange()) {
  // case MPU6050_RANGE_250_DEG:
  //   // Serial.println("+- 250 deg/s");
  //   break;
  // case MPU6050_RANGE_500_DEG:
  //   // Serial.println("+- 500 deg/s");
  //   break;
  // case MPU6050_RANGE_1000_DEG:
  //   // Serial.println("+- 1000 deg/s");
  //   break;
  // case MPU6050_RANGE_2000_DEG:
  //   // Serial.println("+- 2000 deg/s");
  //   break;
  // }

  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
  // // Serial.print("Filter bandwidth set to: ");
  // switch (mpu.getFilterBandwidth()) {
  // case MPU6050_BAND_260_HZ:
  //   // Serial.println("260 Hz");
  //   break;
  // case MPU6050_BAND_184_HZ:
  //   // Serial.println("184 Hz");
  //   break;
  // case MPU6050_BAND_94_HZ:
  //   // Serial.println("94 Hz");
  //   break;
  // case MPU6050_BAND_44_HZ:
  //   // Serial.println("44 Hz");
  //   break;
  // case MPU6050_BAND_21_HZ:
  //   // Serial.println("21 Hz");
  //   break;
  // case MPU6050_BAND_10_HZ:
  //   // Serial.println("10 Hz");
  //   break;
  // case MPU6050_BAND_5_HZ:
  //   // Serial.println("5 Hz");
  //   break;
  // }
}

//Main loop - runs over and over
void loop() {
  flush=0; 

  //get capsense values
  currtouched = cap.touched();
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  //if first loop...
  if(setMinMax==1){
    resetGyroMinMax();
  }
   
  for (uint8_t i=0; i<5; i++) {
    // it if *is* touched and *wasnt* touched before, alert!
    if ((currtouched & _BV(i)) && !(lasttouched & _BV(i)) ) {
      presses[i] = 1;
      // Serial.print(i); Serial.println(" touched");
    }
    // if it *was* touched and still is, iterate!
    if ((currtouched & _BV(i)) && (lasttouched & _BV(i)) ) {
      presses[i]++;
      presses[i] = constrain( presses[i], 0, pressThreshold);
    }
    // if it *was* touched and now *isnt*, alert!
    if (!(currtouched & _BV(i)) && (lasttouched & _BV(i)) ) {
      presses[i] = 0;
      // Serial.print(i); Serial.println(" released");
    }
  }

  // reset our state
  lasttouched = currtouched;

  // (it says its reading acceleration but I think these are switched on cheap boards?)
  x = xFilter.filter(a.acceleration.x); 
  y = yFilter.filter(a.acceleration.y);
  z = zFilter.filter(a.acceleration.z);

  //handle thumb...
  if(presses[thumb]==pressThreshold){
    if(mapModePre==0){
      resetGyroMinMax();
      mapModePre=1;
    } else {
      cooldown = 400;
      getRange();

      //in map mode, spit out dummy x/y/z for mapping
      if(presses[index]==pressThreshold){
        sendCC(10, 64);
        flush=1;
      } else if (presses[middle]==pressThreshold){
        sendCC(11, 64);
        flush=1;
      } else if (presses[ring]==pressThreshold){
        sendCC(12, 64);
        flush=1;
      }
    }
  } else {
    //if thumb not pressed, normal messaging... 
    mapModePre=0;
    //check for finger presses (this code could be more efficient)...
    //index
    if(presses[index]==pressThreshold){
      if(fingPreState[0]==0){//transition from off to on
        sendCC(13, 127);//send cc 'on'
        flush=1;
        fingPreState[0]=1;
      } else {fingPreState[0]=1;}//stays on
    } else if (presses[index]==0) {
      if(fingPreState[0]==1){//transition from on to off
        sendCC(13, 0);//send cc 'off'
        flush=1;
        fingPreState[0]=0;
      } else {fingPreState[0]=0;}//stays off
    }

    //middle
    if(presses[middle]==pressThreshold){
      if(fingPreState[1]==0){//transition from off to on
        sendCC(14, 127);//send cc 'on'
        flush=1;
        fingPreState[1]=1;
      } else {fingPreState[1]=1;}//stays on
    } else if (presses[middle]==0) {
      if(fingPreState[1]==1){//transition from on to off
        sendCC(14, 0);//send cc 'off'
        flush=1;
        fingPreState[1]=0;
      } else {fingPreState[1]=0;}//stays off
    }

    //ring
    if(presses[ring]==pressThreshold){
      if(fingPreState[2]==0){//transition from off to on
        sendCC(15, 127);//send cc 'on'
        flush=1;
        fingPreState[2]=1;
      } else {fingPreState[2]=1;}//stays on
    } else if (presses[ring]==0) {
      if(fingPreState[2]==1){//transition from on to off
        sendCC(15, 0);//send cc 'off'
        flush=1;
        fingPreState[2]=0;
      } else {fingPreState[2]=0;}//stays off
    }

    //small
    if(presses[small]==pressThreshold){
      if(fingPreState[3]==0){//transition from off to on
        sendCC(16, 127);//send cc 'on'
        flush=1;
        fingPreState[3]=1;
      } else {fingPreState[3]=1;}//stays on
    } else if (presses[small]==0) {
      if(fingPreState[3]==1){//transition from on to off
        sendCC(16, 0);//send cc 'off'
        flush=1;
        fingPreState[3]=0;
      } else {fingPreState[3]=0;}//stays off
    }

    //send xyz vals
    // scale xyz to midi range using min / max 
    x = constrain(x, xMin, xMax);
    y = constrain(y, yMin, yMax);
    z = constrain(z, zMin, zMax);

    x = (x-xMin) / (xMax-xMin) * 127;
    y = (y-yMin) / (yMax-yMin) * 127;
    z = (z-zMin) / (zMax-zMin) * 127;

    //cooldown adds a break before sending xyz data gain from mapping mode
    //this gives a window to map the fingers as well
    cooldown = constrain(cooldown-1, 0, 1000);
    if(cooldown==0){
      if(x!=xPrev){//send midicc if x has changed value
          sendCC(10,x);
          flush = 1;
      }
      if(y!=yPrev){//send midicc if y has changed value
          sendCC(11,y);
          flush = 1;
      }
      if(z!=zPrev){//send midicc if z has changed value... etc.
          sendCC(12,z);
          flush = 1;
      }
    }

    //Record previous xyz values
    xPrev=x;
    yPrev=y;
    zPrev=z; 

     //If we have midi cc to send, trigger the flush function
    if(flush){MidiUSB.flush();}

  }
  #ifdef DEBUG_TOUCHES
    Serial.print("thumb");
    Serial.print(" ");
    Serial.print(presses[thumb]);
    Serial.print("   ");
    Serial.print("index");
    Serial.print(" ");
    Serial.print("   ");
    Serial.print(presses[index]);
    Serial.print("   ");
    Serial.print("middle");
    Serial.print(" ");
    Serial.print(presses[middle]);
    Serial.print("   ");
    Serial.print("ring");
    Serial.print(" ");
    Serial.print(presses[ring]);
    Serial.print("   ");
    Serial.print("small");
    Serial.print(" ");
    Serial.println(presses[small]);
  #endif

  #ifdef DEBUG_GYRO
    Serial.print(x);
    Serial.print("   ");
    Serial.print(y);
    Serial.print("   ");
    Serial.println(z);
  #endif

  #ifdef DEBUG_RANGE
    Serial.print(xMin);
    Serial.print("   ");
    Serial.print(xMax);
    Serial.print("   ");
    Serial.print(yMin);
    Serial.print("   ");
    Serial.print(yMax);
    Serial.print("   ");
    Serial.print(zMin);
    Serial.print("   ");
    Serial.println(zMax);
  #endif

  // Serial.println(cooldown);
}



  

  




  

