/*
  CosmicWatch Desktop Muon Detector Arduino Code

  This code is used to record data to the built in microSD card reader/writer.
  
  Questions?
  Spencer N. Axani
  saxani@mit.edu

  Requirements: Sketch->Include->Manage Libraries:
  SPI, EEPROM, SD, and Wire are probably already installed.
  1. Adafruit SSD1306     -- by Adafruit Version 1.0.1
  2. Adafruit GFX Library -- by Adafruit Version 1.0.2
  3. TimerOne             -- by Jesse Tane et al. Version 1.1.0
*/

#include <SPI.h>
#include <SD.h>
#include <EEPROM.h>
#include <Servo.h>

#define SDPIN 10
SdFile root;
Sd2Card card;
SdVolume volume;

File myFile;

const int SIGNAL_THRESHOLD    = 50; // 1000; //50;        // Min threshold to trigger on
const int RESET_THRESHOLD     = 15; // 100; // 15; 

const int StartAngle = 45;
const int EndAngle = 90;  
const int AngleStep = 5;
const unsigned long MeasurementPeriod = 48 * 3600LL;

const int CalibrationAngle = 1;
const int CalibrationPeriod = 60; // 12 * 3600LL;

//initialize variables
char detector_name[40];

unsigned long time_stamp                    = 0L;
unsigned long start_time                    = 0L;      // Start time reference variable
long int      total_deadtime                = 0L;      // total time between signals

unsigned long measurement_t1;
unsigned long measurement_t2;

float temperatureC;


long int      count                         = 0L;         // A tally of the number of muon counts observed
long int      countAll                      = 0L;         // all observed muons (no matter if master or slave)
long int      coinc                         = 0L;         // count of coincidences

char          filename[]                    = "File_000.txt";
byte role = 0;  // 0 single, 1 primary, 2 secondary
byte keep_pulse;

// GPS
volatile unsigned long ppsoffset;
volatile unsigned long ppsseconds;
volatile unsigned long ppsus;
volatile unsigned long endPeriod;

volatile int slowPrint;                   // flag to print the slow count in the loop
long int slowCountStart;                  // count at the beginning of the minute
long int slowCoincStart;                  // coincidences at the beginning of the minute
float slowTemp;                           // temperature to print (aggregate of all measurements)
unsigned long slowDeadtimeStart;          // deadtime at the beginning of the minute

enum {
  NEWCMD = 1,
  REMOVECMD = 2,
  READCMD = 3,
};

Servo myservo;  // create servo object to control a servo
// twelve servo objects can be created on most boards

int pos = 0;    // variable to store the servo position

void setPos(int p) {
  int n;

  n = 1;
  if (p < pos) {
    n = -1;
  }

  while (p != pos) {
//    Serial.println((String) ">> " + pos);
    myservo.write(pos);
    delay(5);
    pos += n;
  }

  myservo.write(pos);  // just in case
}

void println(String s) {
  Serial.println(s);
  if (myFile) {
    myFile.println(s);
    myFile.flush();
  }
}

void setup() {
  analogReference (EXTERNAL);
  ADCSRA &= ~(bit (ADPS0) | bit (ADPS1) | bit (ADPS2));    // clear prescaler bits
  //ADCSRA |= bit (ADPS1);                                   // Set prescaler to 4  
  ADCSRA |= bit (ADPS0) | bit (ADPS1); // Set prescaler to 8
  
  get_detector_name(detector_name);
  
  pinMode(3, OUTPUT);
  Serial.begin(115200/*9600*/);
  Serial.setTimeout(3000);

  if (SD.begin(SDPIN)) {
    role = 0;
    pinMode(6, INPUT);
    delay(1000);
//    if (digitalRead(6) == HIGH) {
      // we are the primary, there is secondary
      role = 1;
//    }
  } else {
    // there is no card, assume that we are secondary
    pinMode(6, OUTPUT);
    digitalWrite(6, HIGH);
//    delay(1000);
//    digitalWrite(6, LOW);
    role = 2;
  }
    
  analogRead(A0);
  
//  println("##########################################################################################");
//  println("### CosmicWatch: The Desktop Muon Detector");
//  println("### Questions? saxani@mit.edu");
//  println("### Comp_date Comp_time Event Ardn_time[ms] ADC[0-1023] SiPM[mV] Deadtime[ms] Temp[C] Name");
//  println("##########################################################################################");
//  println("### Device ID: " + (String) detector_name);
  println("# Role " + (String) role);

  if (role != 2) {  
    myservo.attach(A5, 790, 2790);  // 0: attaches the servo on pin 9 to the servo object
//    myservo.attach(A5, 500, 2450);  // 1: attaches the servo on pin 9 to the servo object

    // calibration period
    setPos(CalibrationAngle);
    delay(1000);
    newFile(CalibrationPeriod);
    
    pinMode(2, INPUT);
    digitalWrite(2, HIGH);
    attachInterrupt(digitalPinToInterrupt(2), ppsIsr, RISING);
  }
}

void loop() {
  int adc, pulse, c;

  pulse = 0;
  measurement_t1 = micros();
  if (slowPrint) {
    if (ppsseconds != 0) {
      println((String) "## " + ppsseconds + " " + (countAll - slowCountStart) + " " + (coinc - slowCoincStart) + " " + (total_deadtime - slowDeadtimeStart) + " " + (slowTemp / (countAll - slowCountStart)));
    }

    if (ppsseconds >= endPeriod) {
      int p;
      
      if (pos == CalibrationAngle)
        p = StartAngle;
      else
        p = pos + AngleStep;

      if (p > EndAngle)
        p = StartAngle;

      setPos(p);
      delay(1000);
      newFile(MeasurementPeriod);
      return;
    }
    
    slowCountStart = countAll;
    slowCoincStart = coinc;
    slowTemp = 0.0;
    slowDeadtimeStart = total_deadtime;
    slowPrint = 0;
  }

  if (Serial.available() >= 1) {
    runCommand();
    return;
  }
  
  if (analogRead(A0) > SIGNAL_THRESHOLD) {
    if (role != 1) {
      digitalWrite(6, LOW);
    } else {
      c = digitalRead(6) == LOW;
    }

    adc = analogRead(A0);    
//    analogRead(A3);

    time_stamp = millis() - start_time;  
    temperatureC = (((analogRead(A3)+analogRead(A3)+analogRead(A3))/3. * (3300./1024)) - 500)/10. ;
    slowTemp += temperatureC;

   countAll++; 
    if (role != 1) {
//      digitalWrite(6, HIGH);
      analogRead(A3);
      digitalWrite(6, HIGH);
      count++;
      pulse = 1;
    } else {
//      analogRead(A3);      
      c = c || (digitalRead(6) == LOW);
      if (c == HIGH) {
          pulse = 1;
          count++;
          coinc++;
      }
    }

    unsigned long ppstime;
    if (ppsoffset != 0) {
      ppstime = ppsseconds * (unsigned long) 1000 + ppsoffset + (measurement_t1 - ppsus) / (unsigned long) 1000;
    } else {
      ppstime = time_stamp;
    }

    if (pulse) {
//      analogWrite(3, LED_BRIGHTNESS);
      println((String)count + " " + time_stamp + " " + adc+ " " + total_deadtime+ " " + temperatureC + " " + (long) (ppstime - time_stamp));
//      digitalWrite(3, LOW);
    }
    
    while(analogRead(A0) > RESET_THRESHOLD){
      continue;
    }
  }
    
  total_deadtime += (micros() - measurement_t1) / 1000.;
}

void readFromSD(){
  for (uint8_t i = 1; i < 211; i++) {
      
    int hundreds = (i-i/1000*1000)/100;
    int tens = (i-i/100*100)/10;
    int ones = i%10;
    filename[5] = hundreds + '0';
    filename[6] = tens + '0';
    filename[7] = ones + '0';
    if (SD.exists(filename)) {
        delay(10);  
        File dataFile = SD.open(filename);
        Serial.println("Read " + (String)filename);
        while (dataFile.available()) {
            Serial.write(dataFile.read());
        }
        dataFile.close();
        Serial.println("EOF");
    }
  }
    
  Serial.println("Done");
}

void removeAllSD() {
  for (uint8_t i = 1; i < 211; i++) {
      
    int hundreds = (i-i/1000*1000)/100;
    int tens = (i-i/100*100)/10;
    int ones = i%10;
    filename[5] = hundreds + '0';
    filename[6] = tens + '0';
    filename[7] = ones + '0';
      
    if (SD.exists(filename)) {
        delay(10);  
        Serial.println("Delete " + (String)filename);
        SD.remove(filename);   
      }        
  }
  Serial.println("Done");
}

void newFile(unsigned long period) {
  myFile.close();
  for (uint8_t i = 1; i < 201; i++) {
      int hundreds = (i-i/1000*1000)/100;
      int tens = (i-i/100*100)/10;
      int ones = i%10;
      filename[5] = hundreds + '0';
      filename[6] = tens + '0';
      filename[7] = ones + '0';
      if (! SD.exists(filename)) {
          Serial.println("New " + (String)filename);
          myFile = SD.open(filename, FILE_WRITE); 
          break; 
      }
   }

  println("# Device ID: " + (String) detector_name);
  println("# Angle " + (String) pos);
  count = 0;
  countAll = 0;
  coinc = 0;
  total_deadtime = 0;
  start_time = millis();
  ppsoffset = 0;
  ppsus = micros();
  ppsseconds = 0;
  endPeriod = period;
  slowCountStart = 0;
  slowCoincStart = 0;
  slowTemp = 0;
  slowPrint = 0;
  slowDeadtimeStart = 0;
  delay(10);
}

int getCommand() {
//    Serial.print("CosmicWatchDetector " + (String) detector_name + "> ");
    String message = "";
    message = Serial.readString();
    Serial.println((String) "*** " + message);
    if (message == "read\n") {
      return READCMD;
    } else if (message == "remove\n"){
      return REMOVECMD;
    } else if (message == "new\n"){
      return NEWCMD;
    } else {
      int n = message.toInt();
      setPos(n);
      delay(1000);
    }

    return NEWCMD;
}

void runCommand() {
  int c = getCommand();
  switch (c) {
    case NEWCMD:
      newFile(MeasurementPeriod);
      break;

    case REMOVECMD:
      myFile.close();
      removeAllSD();
      newFile(MeasurementPeriod);
      break;

    case READCMD:
      readFromSD();
      break;
  }
}

boolean get_detector_name(char* det_name) 
{
    byte ch;                              // byte read from eeprom
    int bytesRead = 0;                    // number of bytes read so far
    ch = EEPROM.read(bytesRead);          // read next byte from eeprom
    det_name[bytesRead] = ch;               // store it into the user buffer
    bytesRead++;                          // increment byte counter

    while ( (ch != 0x00) && (bytesRead < 40) && ((bytesRead) <= 511) ) 
    {
        ch = EEPROM.read(bytesRead);
        det_name[bytesRead] = ch;           // store it into the user buffer
        bytesRead++;                      // increment byte counter
    }
    if ((ch != 0x00) && (bytesRead >= 1)) {det_name[bytesRead - 1] = 0;}
    return true;
}

void ppsIsr()
{
  unsigned long us, dt;
  
  us = micros();
  dt = us - ppsus;
  
  ppsseconds += dt / 1000000;
  if (dt % 1000000 >= 500000) {
    ppsseconds++;
  }

  ppsus = us;  
  if (ppsoffset == 0) {
    ppsoffset = dt / 1000;
    ppsseconds = 0;
  }

  if (ppsseconds % 60 == 0) {
    slowPrint = 1;
  }
  
//  interrupts();
 
//  Serial.println("# ping"); //  + (String) ppsoffset);
}
