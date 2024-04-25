#include "RTClib.h"
#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include <Ezo_i2c.h>
#include "TSYS01.h"
#include "MS5837.h"
#include "Arduino.h"
#include <ArduinoJson.h>

RTC_PCF8523 rtc;
Sd2Card card;
SdVolume volume;
SdFile root;
MS5837 msensor;
TSYS01 tsensor;
Ezo_board dox = Ezo_board(97, "DO");
Ezo_board orp = Ezo_board(98, "ORP");
Ezo_board ph = Ezo_board(99, "PH");
Ezo_board ec = Ezo_board(100, "EC");

const int chipSelect = 10;
int ledPin = 13;
int leakPin = 2;
int leak = 0;

void receive_reading(Ezo_board &Sensor) {
  Sensor.receive_read_cmd();
}

void setup() {

  Serial.begin(9600);
  while (!Serial) {};  // wait for serial port to connect. Needed for native USB port only

  pinMode(ledPin, OUTPUT);
  pinMode(leakPin, INPUT);

  Wire.begin();

  while (!tsensor.init()) {
        Serial.println("TSYS01 device failed to initialize!");
        delay(2000);
    };

  while (!msensor.init()) {
        Serial.println("MS5837 device failed to initialize!");
        delay(2000);
  };

  msensor.setModel(MS5837::MS5837_30BA);
  msensor.setFluidDensity(997);  // kg/m^3 (997 for freshwater, 1029 for seawater)

  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC.");
    Serial.flush();
    while (1) delay(10);
  }
  rtc.start();
}

void loop() {

  msensor.read();

  tsensor.read();

  dox.send_read_cmd();
  orp.send_read_cmd();
  ph.send_read_cmd();
  ec.send_read_cmd();

  delay(1000);

  DateTime now = rtc.now();

  receive_reading(dox);
  receive_reading(orp);
  receive_reading(ph);
  receive_reading(ec);

  digitalWrite(ledPin, leak);

  JsonDocument doc;
  doc["year"] = now.year();
  doc["month"] = now.month();
  doc["day"] = now.day();
  doc["hour"] = now.hour();
  doc["minute"] = now.minute();
  doc["second"] = now.second();
  doc["depth"] = msensor.depth();
  doc["temp"] = tsensor.temperature();
  doc["doxy"] = dox.get_last_received_reading();
  doc["oxrp"] = orp.get_last_received_reading();
  doc["phl"] = ph.get_last_received_reading();
  doc["ecn"] = ec.get_last_received_reading();
  doc["leak"] = digitalRead(leakPin);

  serializeJson(doc, Serial);
  Serial.println();

}
