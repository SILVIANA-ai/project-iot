/*********
  Rui Santos & Sara Santos - Random Nerd Tutorials
  Complete instructions at https://RandomNerdTutorials.com/esp32-neo-6m-gps-module-arduino/
  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files.
  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
*********/

#include <TinyGPS++.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

// Define the RX and TX pins for Serial 2
#define RXD2 16
#define TXD2 17

#define GPS_BAUD 9600

// The TinyGPS++ object
TinyGPSPlus gps;

// Create an instance of the HardwareSerial class for Serial 2
HardwareSerial gpsSerial(2);
Adafruit_MPU6050 mpu;

void setup() {
  // Serial Monitor
  Serial.begin(115200);
  
  // Start Serial 2 with the defined RX and TX pins and a baud rate of 9600
  gpsSerial.begin(GPS_BAUD, SERIAL_8N1, RXD2, TXD2);
  Serial.println("Serial 2 started at 9600 baud rate");

  Wire.begin();
  if (!mpu.begin()) {
    Serial.println("MPU6050 connection failed");
    while (1) {
      delay(10);
    }
  } else {
    Serial.println("MPU6050 connection successful");
  }
}

void loop() {
  // This sketch displays information every time a new sentence is correctly encoded.
  unsigned long start = millis();

  while (millis() - start < 1000) {
    while (gpsSerial.available() > 0) {
      gps.encode(gpsSerial.read());
    }
    if (gps.location.isUpdated()) {
      Serial.print("LAT: ");
      Serial.println(gps.location.lat(), 6);
      Serial.print("LONG: "); 
      Serial.println(gps.location.lng(), 6);
      Serial.print("SPEED (km/h) = "); 
      Serial.println(gps.speed.kmph()); 
      Serial.print("ALT (min)= "); 
      Serial.println(gps.altitude.meters());
      Serial.print("HDOP = "); 
      Serial.println(gps.hdop.value() / 100.0); 
      Serial.print("Satellites = "); 
      Serial.println(gps.satellites.value()); 
      Serial.print("Time in UTC: ");
      Serial.println(String(gps.date.year()) + "/" + String(gps.date.month()) + "/" + String(gps.date.day()) + "," + String(gps.time.hour()) + ":" + String(gps.time.minute()) + ":" + String(gps.time.second()));
      
      // MPU6050 readings using Adafruit library
      sensors_event_t a, g, temp;
      mpu.getEvent(&a, &g, &temp);

      Serial.println("MPU6050 Data:");
      Serial.print("Accel X: "); Serial.print(a.acceleration.x);
      Serial.print(" | Accel Y: "); Serial.print(a.acceleration.y);
      Serial.print(" | Accel Z: "); Serial.println(a.acceleration.z);
      Serial.print("Gyro X: "); Serial.print(g.gyro.x);
      Serial.print(" | Gyro Y: "); Serial.print(g.gyro.y);
      Serial.print(" | Gyro Z: "); Serial.println(g.gyro.z);
      Serial.print("Temperature: "); Serial.print(temp.temperature); Serial.println(" degC");
      Serial.println("");
    }
  }
}