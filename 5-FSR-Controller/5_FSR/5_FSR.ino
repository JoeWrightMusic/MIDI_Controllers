/*
5_FSR 

Arduino controller for 5 FSRs using an arduino pro micro. 
The Fusion files are designed to go with a breakout board 
using 10mm diameter FSRs, but the design will work fine with
any round fsrs of 10mm or less in each slot. 

*/


#include "MIDIUSB.h"
#include <Ewma.h>

// #define DEBUG_MODE 

Ewma filt0(0.1); 
Ewma filt1(0.1); 
Ewma filt2(0.1); 
Ewma filt3(0.1); 
Ewma filt4(0.1); 

int fsrPins[] = {A0,A1,A2,A3,A6};
int fsr[] = {0,0,0,0,0};
int fsrPre[] = {0,0,0,0,0};
int doFlush = 0;

void sendCC(byte control, byte value) {
  midiEventPacket_t event = {0x0B, 0xB0 | 0, control, value};
  MidiUSB.sendMIDI(event);
}

void setup() {
  Serial.begin(115200);
}

void loop() {
  doFlush=0;

  for(int i=0; i<5; i++){
    int temp=0;
    temp = analogRead(fsrPins[i]);
    temp = constrain(temp, 300, 950);
    temp = map(temp, 300, 950, 0, 127);
    
    if(i==0){fsr[i] = filt0.filter(temp);}
    else if(i==1){fsr[i] = filt1.filter(temp);}
    else if(i==2){fsr[i] = filt2.filter(temp);}
    else if(i==3){fsr[i] = filt3.filter(temp);}
    else if(i==4){fsr[i] = filt4.filter(temp);}
      

    if(fsrPre[i]!=fsr[i]){
      sendCC(1+i, fsr[i]);
      doFlush=1;
    };

    fsrPre[i]=fsr[i];

    #ifdef DEBUG_MODE
      Serial.print(fsr[i]);
      Serial.print("\t");
    #endif
  }
  #ifdef DEBUG_MODE
    Serial.println("");
  #endif

  if(doFlush==1){
    MidiUSB.flush();
  }
}
