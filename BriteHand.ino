#include <Servo.h>

// Pin assignments for the two sensors

const int S1OUT = 4;
const int S1EN = 5;

const int S2OUT = 8;
const int S2EN = 9;

// Pin assignments for the servo
const int Squirt180 = 10;  // this servo output is always set to 180 degrees and is used to get the horn on properly.
const int SquirtPin = 11; // this is the one actually used to move the arm. So use the 180 one to get the horn adjusted, then switch to this one.

// Buzzer outputs tones to help figure out good or bad hits
const int Buzzer = 12;

Servo Squirt;
Servo SquirtSetup;  // always 180 degrees, used to put horn on properly

void cycleSensors() {
  digitalWrite(S1EN, LOW);
  digitalWrite(S2EN, LOW);
  delay(100);
  digitalWrite(S1EN, HIGH);
  digitalWrite(S2EN, HIGH);
  delay(100);
}

int handDetected(int sensorEN, int sensorOUT) {
    byte handDetected = 0;

    digitalWrite(sensorEN, HIGH);     // Enable the internal 38kHz signal.
    delayMicroseconds(210);           // Wait 210µs (8 pulses of 38kHz).
    if(digitalRead(sensorOUT)) {      // If detector Output is HIGH,
        handDetected = 0;
    } else {                          // but if the Output is LOW,
      delayMicroseconds(395);         // wait for another 15 pulses.
      if(digitalRead(sensorOUT)) {    // If the Output is now HIGH,                                // then first Read was noise
          handDetected = 0;          // and no object was detected;
      } else {                        // but if the Output is still LOW,
          handDetected = 1;           // then an object was truly detected.
      }
  }
  delayMicroseconds(200);             // wait a bit
  //digitalWrite(sensorEN, LOW);        // Disable the internal 38kHz signal.
  
  //Serial.print(sensorOUT); Serial.print("="); Serial.println(handDetected);
  return handDetected;
}

void setup ()
{
 Serial.begin(9600); // Initialization serial output
 SquirtSetup.attach(Squirt180);
 SquirtSetup.write(180);   // this servo pin always sits at 180 degrees to help install the servo horn in the right orientation.

 // Provide power to sensors through the arduino pins. The KY-032 sensor
 // requires 20 mA which can be provided by the nano. We're not enabling both
 // at the same time either so it should be fine.
 pinMode(S1OUT, INPUT);
 pinMode(S1EN, OUTPUT);
 digitalWrite(S1EN, LOW); // start with it off
 
 pinMode(S2OUT, INPUT);
 pinMode(S2EN, OUTPUT);
 digitalWrite(S2EN, LOW);

 pinMode(Buzzer, OUTPUT);

 // play a series of tones on startup to confirm the buzzer works

 tone(Buzzer,400,500);
 delay(500);
 tone(Buzzer,800,500);
 delay(500);
 tone(Buzzer, 600, 250);
 delay(500);
 tone(Buzzer, 600, 250);
 delay(250);
 pinMode(Buzzer, INPUT); // stops little squeaks
}

void toneOut(int freq, int t) {
  pinMode(Buzzer, OUTPUT);
  tone(Buzzer, freq, t);
  delay(t);
  pinMode(Buzzer, INPUT); // so we don't get any weird noises
}

unsigned long LastDetectTime = 0;
unsigned long LastNondetectTime = 0;

#define TOOLONGDETECTING 10000

void loop ()
{

   // to avoid false positives, we'll require several hits on both sensors in a row
   // before dispensing. You could adjust the number of hits required to tune sensitivity
   // just don't go below 1.
#define HITSNEEDED 3

   int hits = 0;
   for (int i = 0; i < HITSNEEDED; i++) {
     int detected = 0;
     detected = handDetected(S1EN, S1OUT) + handDetected(S2EN, S2OUT);
     if (detected == 2) {
      LastDetectTime = millis();
      hits++;
      toneOut(250*hits,25); 
     } else {
      // no detect
      LastNondetectTime = millis();
     }
     delay(25);
   }
   //Serial.println(hits); // for debugging


#define SERVODELAY 300 // For faster servo like MG958 make this 200

   if (hits == HITSNEEDED) {
     Squirt.attach(SquirtPin);
     Squirt.write(140);
     delay(SERVODELAY);
     Squirt.write(180);
     delay(SERVODELAY);
     Squirt.detach();   
     delay(500); // add an extra delay after sequence ends so it doesn't output too fast
   }

   cycleSensors();
   delay(50);

   // This code is for safety
   if (millis() > LastNondetectTime + TOOLONGDETECTING) {
    // we've been continuously detecting so something is probably wrong
    // Wait to have a nondetect event.
    while (handDetected(S1EN, S1OUT)+handDetected(S2EN,S2OUT) == 2) {
      toneOut(50,10);
      LastDetectTime = millis();
      delay(1000);
    }
    LastNondetectTime = millis();
    // now let's require nondetects on both sensors for several seconds
    while (handDetected(S1EN, S1OUT)+handDetected(S2EN,S2OUT) == 0 && millis() < LastNondetectTime+5000) {
      toneOut(50,10);
      delay(1000);
    }
    LastNondetectTime == millis();
   }
}
