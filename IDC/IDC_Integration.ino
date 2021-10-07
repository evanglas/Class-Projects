// Evan Glas
// IDC Code

#include <Wire.h> // Library for I2C communication
#include <LiquidCrystal_I2C.h> // Library for LCD
#include "Adafruit_TCS34725.h"
#include <Servo.h>
#include <Adafruit_MPU6050.h>
#include "pitches.h"

// Defines sensor/piezo pins
#define servoPin 13
#define qtPin A2
#define echoPin 7
#define triggerPin 6
#define buzzerPin 12

// Defines LED pins
#define greenPin1 2
#define greenPin2 8
#define redPin 3
#define yellowPin 5


/* Initialise with default values (int time = 2.4ms, gain = 1x) */
// Adafruit_TCS34725 tcs = Adafruit_TCS34725();

/* Initialise with specific int time and gain values */
Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_700MS, TCS34725_GAIN_1X);

Servo s;
LiquidCrystal_I2C lcd = LiquidCrystal_I2C(0x27, 16, 2);
Adafruit_MPU6050 mpu;

// Initialize task variables
int air = 0;
int fire = 0;
int earth = 0;
int water = 0;
int total = 0;
int airCompleted = 0;
int waterCompleted = 0;
int earthCompleted = 0;
int fireCompleted = 0;
int completed = 0;

// Returns the sound wave travel time from the ping sensor in microseconds
long readPing(int tPin, int ePin)
{
  pinMode(triggerPin, OUTPUT);  // Clear the trigger
  digitalWrite(triggerPin, LOW);
  delayMicroseconds(2);
  // Sets the trigger pin to HIGH state for 10 microseconds
  digitalWrite(triggerPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(triggerPin, LOW);
  pinMode(echoPin, INPUT);
  // Reads the echo pin, and returns the sound wave travel time in microseconds
  return pulseIn(echoPin, HIGH);
}

// Completes the air task
// Determines number of horns by rotating standard servo 180 degrees in 3 degree intervals.
// QTI sensor records number of intervals in which a horn blocks the sensor in whiteTime
// Based on whiteTime, returns correct horn value
int airTask() {
  //Serial.println(Serial.available());
  //String so = Serial.readString();
  //Serial.println(so);
  int whiteTime = 0;
  s.write(0);
  delay(500);
  // Simulates a half-circle rotation
  boolean onWhite = false;
  for (int i = 0; i <= 210; i += 3) {
    s.write(i);
    delay(100);
    //Calls function 'RCTime' Request reading from QTI sensor at pin 'linesensor1' saves value in variable 'qti'
    int qti1 = analogRead(A2);
    Serial.println(qti1);
    if(qti1 < 600){
      if (i <= 6) {
        onWhite = true;
      }
      whiteTime++;
    }
    delay(200);
    //Serial.println(qti1);
  }
  
  Serial.print("White time:");
  Serial.println(whiteTime);
  
  if(whiteTime < 2){
    lcd.clear();
    lcd.print("No attachment");
    return 3;
  } else if(onWhite){
    lcd.clear();
    lcd.print("Straight Line");
    return 2;
  } else{
    lcd.clear();
    lcd.print("Single Arm");
    return 1;
  }
  Serial.print("White time: ");
  Serial.println(whiteTime);
  delay(1000);
}

// Returns appropriate fire task value using color sensor
// Considers ratio of red light to all other colors to differentiate
// red, orange, and blue circles.
int fireTask() {
  uint16_t r, g, b, c, colorTemp, lux;

  tcs.getRawData(&r, &g, &b, &c);
  // colorTemp = tcs.calculateColorTemperature(r, g, b);
  colorTemp = tcs.calculateColorTemperature_dn40(r, g, b, c);
  lux = tcs.calculateLux(r, g, b);

  delay(300); 
  
  Serial.print("Color Temp: "); Serial.print(colorTemp, DEC); Serial.print(" K - ");
  Serial.print("Lux: "); Serial.print(lux, DEC); Serial.print(" - ");
  Serial.print("R: "); Serial.print(r, DEC); Serial.print(" ");
  Serial.print("G: "); Serial.print(g, DEC); Serial.print(" ");
  Serial.print("B: "); Serial.print(b, DEC); Serial.print(" ");
  Serial.print("C: "); Serial.print(c, DEC); Serial.print(" ");
  Serial.println(" ");
  
  lcd.clear();
  int colorSum = r + g + b;
  float pred = (float)r/colorSum;
  Serial.println(pred);
  if (pred < .3) {
    lcd.print("BLUE");
    return 3;
  } else if (pred < .53) {
    lcd.print("ORANGE");
    return 2;
  } else {
    lcd.print("RED");
    return 1;
  }
}

// Completes earth task by converting PING reading to inches,
// then finding the distance to the paper rock.
// Returns correct rock distance
int earthTask() {
  lcd.clear();
  double cm = 0.01723 * readPing(4, 5);
  double inches = (cm / 2.54);
  Serial.println(inches);
  if (inches < 3) {
    lcd.print("SHORT");
    return 1;
  } else if (inches < 6.5) {
    lcd.print("MEDIUM");
    return 2;
  } else {
    lcd.print("LONG");
    return 3;
  }
}

// Completes water task by finding angle of each wave using accelerometer
// Returns appropriate wave number
int waterTask() {
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);
  double ax = a.acceleration.x;
  double ay = a.acceleration.y;
  double az = a.acceleration.z;
  
  lcd.clear();
  Serial.println(ax);
  Serial.print("AccY= ");
  Serial.println(ay);
  Serial.print("AccZ= ");
  Serial.println(az);
  Serial.println("-----------------------------------------");
    if(az < -9){
      lcd.print("Wave 1");
      return 1;
      }
    else if(az < -5 && az >= -9){
      lcd.print("Wave 2");
      return 2;
    }
    else if(az >= -5){
      lcd.print("Wave 3");
      return 3;
    }
}

// Completes the communication task by taking a letter, t, then calling
// enterVal() to get a numerical value from the user
void enterTask(char t) {
  if (t == 'a') {
    air = enterVal();
    airCompleted = 1;
  } else if (t == 'e') {
    earth = enterVal();
    earthCompleted = 1;
  } else if (t == 'f') {
    fire = enterVal();
    fireCompleted = 1;
  } else if (t == 'w') {
    water = enterVal();
    waterCompleted = 1;
  }
}

// Returns integer entered by the user in the serial monitor
int enterVal() {
  while (!Serial.available()) {
    delay(20);
  }
  char num = Serial.read();
  return num - '0';
}

// Executes water-themed light show using LED's
void waterShow() {
    for (int x = 0; x < 100; x++) {
      digitalWrite(3, LOW);
      digitalWrite(greenPin1, HIGH);
      delay(1000);
      digitalWrite(greenPin1, LOW);
      digitalWrite(greenPin2, HIGH);
      delay(1000);
      digitalWrite(greenPin2, LOW);
  }
}

// Plays Fur Elise using Piezospeaker (song in minor key)
void minorSong() {
    int tempo = 80; 
  int melody[] = {
                      
    // Fur Elise (A Minor)
  
    NOTE_E5, 16, NOTE_DS5, 16, //1
    NOTE_E5, 16, NOTE_DS5, 16, NOTE_E5, 16, NOTE_B4, 16, NOTE_D5, 16, NOTE_C5, 16,
    NOTE_A4, -8, NOTE_C4, 16, NOTE_E4, 16, NOTE_A4, 16,
    NOTE_B4, -8, NOTE_E4, 16, NOTE_GS4, 16, NOTE_B4, 16,
    NOTE_C5, 8,  REST, 16, NOTE_E4, 16, NOTE_E5, 16,  NOTE_DS5, 16,
    
    NOTE_E5, 16, NOTE_DS5, 16, NOTE_E5, 16, NOTE_B4, 16, NOTE_D5, 16, NOTE_C5, 16,//6
    NOTE_A4, -8, NOTE_C4, 16, NOTE_E4, 16, NOTE_A4, 16, 
    NOTE_B4, -8, NOTE_E4, 16, NOTE_C5, 16, NOTE_B4, 16, 
    NOTE_A4 , 4, REST, 8, //9 - 1st ending
  
    //repaets from 1 ending on 10
    NOTE_E5, 16, NOTE_DS5, 16, //1
    NOTE_E5, 16, NOTE_DS5, 16, NOTE_E5, 16, NOTE_B4, 16, NOTE_D5, 16, NOTE_C5, 16,
    NOTE_A4, -8, NOTE_C4, 16, NOTE_E4, 16, NOTE_A4, 16,
    NOTE_B4, -8, NOTE_E4, 16, NOTE_GS4, 16, NOTE_B4, 16,
    NOTE_C5, 8,  REST, 16, NOTE_E4, 16, NOTE_E5, 16,  NOTE_DS5, 16,
    
    NOTE_E5, 16, NOTE_DS5, 16, NOTE_E5, 16, NOTE_B4, 16, NOTE_D5, 16, NOTE_C5, 16,//6
    NOTE_A4, -8, NOTE_C4, 16, NOTE_E4, 16, NOTE_A4, 16, 
    NOTE_B4, -8, NOTE_E4, 16, NOTE_C5, 16, NOTE_B4, 16, 
    NOTE_A4, 8, REST, 16, NOTE_B4, 16, NOTE_C5, 16, NOTE_D5, 16, //10 - 2nd ending
  };

  // sizeof gives the number of bytes, each int value is composed of two bytes (16 bits)
  // there are two values per note (pitch and duration), so for each note there are four bytes
  int notes = sizeof(melody) / sizeof(melody[0]) / 2;
  
  // this calculates the duration of a whole note in ms
  int wholenote = (60000 * 4) / tempo;
  
  int divider = 0, noteDuration = 0;
  for (int thisNote = 0; thisNote < notes * 2; thisNote = thisNote + 2) {
  
      // calculates the duration of each note
      divider = melody[thisNote + 1];
      if (divider > 0) {
        // regular note, just proceed
        noteDuration = (wholenote) / divider;
      } else if (divider < 0) {
        // dotted notes are represented with negative durations!!
        noteDuration = (wholenote) / abs(divider);
        noteDuration *= 1.5; // increases the duration in half for dotted notes
      }
  
      // we only play the note for 90% of the duration, leaving 10% as a pause
      tone(buzzerPin, melody[thisNote], noteDuration * 0.9);
  
      // Wait for the specief duration before playing the next note.
      delay(noteDuration);
  
      // stop the waveform generation before the next note.
      noTone(buzzerPin);
  }
}

// Executes fire-themed show using LED's
void fireShow() {
  for (int x = 0; x < 100; x++) {
    int redTime = random(150, 250);
    digitalWrite(redPin, HIGH);
    delay(redTime);
    digitalWrite(redPin, LOW);
    int yellowTime = random(50, 100);
    digitalWrite(yellowPin, HIGH);
    delay(yellowTime);
    digitalWrite(yellowPin, LOW);
  }
}

// Plays Mii Channel Theme on piezospeaker (whimsical song)
// Code adapted from Rahul, song is from github
void whimsicalSong() {
  int tempo = 115; 
  int melody[] = {
                      
    // Mii Channel Theme
  
    NOTE_FS4,8, REST,8, NOTE_A4,8, NOTE_CS5,8, REST,8,NOTE_A4,8, REST,8, NOTE_FS4,8, //1
    NOTE_D4,8, NOTE_D4,8, NOTE_D4,8, REST,8, REST,4, REST,8, NOTE_CS4,8,
    NOTE_D4,8, NOTE_FS4,8, NOTE_A4,8, NOTE_CS5,8, REST,8, NOTE_A4,8, REST,8, NOTE_F4,8,
    NOTE_E5,-4, NOTE_DS5,8, NOTE_D5,8, REST,8, REST,4,
    
    NOTE_GS4,8, REST,8, NOTE_CS5,8, NOTE_FS4,8, REST,8,NOTE_CS5,8, REST,8, NOTE_GS4,8, //5
    REST,8, NOTE_CS5,8, NOTE_G4,8, NOTE_FS4,8, REST,8, NOTE_E4,8, REST,8,
    NOTE_E4,8, NOTE_E4,8, NOTE_E4,8, REST,8, REST,4, NOTE_E4,8, NOTE_E4,8,
    NOTE_E4,8, REST,8, REST,4, NOTE_DS4,8, NOTE_D4,8, 
  
    NOTE_CS4,8, REST,8, NOTE_A4,8, NOTE_CS5,8, REST,8,NOTE_A4,8, REST,8, NOTE_FS4,8, //9
    NOTE_D4,8, NOTE_D4,8, NOTE_D4,8, REST,8, NOTE_E5,8, NOTE_E5,8, NOTE_E5,8, REST,8,
    REST,8, NOTE_FS4,8, NOTE_A4,8, NOTE_CS5,8, REST,8, NOTE_A4,8, REST,8, NOTE_F4,8,
    NOTE_E5,2, NOTE_D5,8, REST,8, REST,4,
  
    NOTE_B4,8, NOTE_G4,8, NOTE_D4,8, NOTE_CS4,4, NOTE_B4,8, NOTE_G4,8, NOTE_CS4,8, //13
    NOTE_A4,8, NOTE_FS4,8, NOTE_C4,8, NOTE_B3,4, NOTE_F4,8, NOTE_D4,8, NOTE_B3,8,
    NOTE_E4,8, NOTE_E4,8, NOTE_E4,8, REST,4, REST,4, NOTE_AS4,4,
    NOTE_CS5,8, NOTE_D5,8, NOTE_FS5,8, NOTE_A5,8, REST,8, REST,4, 
  };

  // sizeof gives the number of bytes, each int value is composed of two bytes (16 bits)
  // there are two values per note (pitch and duration), so for each note there are four bytes
  int notes = sizeof(melody) / sizeof(melody[0]) / 2;
  
  // this calculates the duration of a whole note in ms
  int wholenote = (60000 * 4) / tempo;
  
  int divider = 0, noteDuration = 0;
  for (int thisNote = 0; thisNote < notes * 2; thisNote = thisNote + 2) {
  
      // calculates the duration of each note
      divider = melody[thisNote + 1];
      if (divider > 0) {
        // regular note, just proceed
        noteDuration = (wholenote) / divider;
      } else if (divider < 0) {
        // dotted notes are represented with negative durations!!
        noteDuration = (wholenote) / abs(divider);
        noteDuration *= 1.5; // increases the duration in half for dotted notes
      }
  
      // we only play the note for 90% of the duration, leaving 10% as a pause
      tone(buzzerPin, melody[thisNote], noteDuration * 0.9);
  
      // Wait for the specief duration before playing the next note.
      delay(noteDuration);
  
      // stop the waveform generation before the next note.
      noTone(buzzerPin);
  }
}

// Initializes variables, sets pinModes
void setup() {
 Serial.begin(9600);
 
 pinMode(qtPin, INPUT);
 pinMode(buzzerPin, INPUT);
 pinMode(greenPin1, OUTPUT);
 pinMode(greenPin2, OUTPUT);
 pinMode(redPin, OUTPUT);
 pinMode(yellowPin, OUTPUT);
 
 s.attach(servoPin);
 s.write(0); // initialize position of servo
 
 lcd.init();
 lcd.backlight();

 if (tcs.begin()) {
  Serial.println("Found sensor");
 } else {
  Serial.println("No TCS34725 found ... check your connections");
  while (1);
 }
  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip");
    while (1) {
      delay(10);
    }
  }
  Serial.println("MPU6050 Found!");
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
 
}

// Main loop: gets user input to completed a task, completes it
// Once all four tasks are completed, displays final output and executes show/music
void loop() {
  // Reads in user choice of task to do (a = air, w = water, c = communication,
  // f = fire, e = earth, n/s for debugging)
  if (Serial.available()) {
    char task = Serial.read();
    Serial.print("Executing task ");
    Serial.println(task);
    if (task == 'a') {
      air = airTask();
      airCompleted = 1;
      Serial.println("Completed");
    } else if (task == 'f') {
      fire = fireTask();
      fireCompleted = 1;
      Serial.println("Completed");
    } else if (task == 'e') {
      earth = earthTask();
      earthCompleted = 1;
    } else if (task == 'w') {
      water = waterTask();
      waterCompleted = 1;
    } else if (task == 'c') {
      Serial.read();
      while(!Serial.available()) {
        delay(20);
      }
      Serial.println("Enter Desired Value");
      enterTask(Serial.read());
    } else if (task == 'n') {
      whimsicalSong();
    } else if (task == 's') {
      //fireShow();
      waterShow();
    }
    // Sums up whether each task has been completed (1 if completed, 0 if not)
    completed = airCompleted + fireCompleted + earthCompleted + waterCompleted;
    // Sums up current total of task outputs
    total = air + fire + earth + water;
    Serial.print("Current Total: ");
    Serial.println(total);
    // If all four tasks have been completed, executes final show/music
    if (completed == 4) {
      lcd.clear();
      lcd.print(total);
      Serial.println("Executing Final Task");
      if (total % 4 == 0) {
        waterShow();
      } else if (total % 4 == 1) {
        minorSong();
      } else if (total % 4 == 2) {
        fireShow();
      } else if (total % 4 == 3) {
        whimsicalSong();
      }
      Serial.println("IDC Completed. Grade = 1000%");
    }
    Serial.read();
  }

 }
