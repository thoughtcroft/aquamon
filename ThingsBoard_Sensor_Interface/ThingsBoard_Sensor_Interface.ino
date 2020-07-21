/*------------------------------------------------------------------------------
  21/07/2020
  Author: bainsworld
  Platforms: ESP8266
  Language: C++/Arduino
  File: ThingsBoard_Sensor_Interface.ino
  ------------------------------------------------------------------------------
  Description:
  Controller for connecting to various sensors using the Arduino Uno and then
  sending the telemetry data to ThingsBoard via an ESP8266 D1 Mini via Serial
  ------------------------------------------------------------------------------*/
#include <ArduinoJson.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "CommonConfig.h"      // common values such as timing defaults

// sensor configurations

// DS18B20 temperature sensor
#define ONE_WIRE_BUS 2
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

bool debug = BW_DEBUG;
String message = "";
bool messageReady = false;
DynamicJsonDocument doc(1024); // ArduinoJson version 6+

void setup() {
  sensors.begin();
  sensors.setResolution(9);
  Serial.begin(BW_SERIAL_BAUD);
  delay(BW_RECONNECT_DELAY);
  Serial.println();
  Serial.println(F("Starting aquamon sender ..."));
}

void loop() {
  // Monitor serial communication
  while (Serial.available()) {
    message = Serial.readString();
    messageReady = true;
  }
  // Only process message if there's one
  if (messageReady) {
    // The only messages we'll parse will be formatted in JSON
    if (checkMessage()) {
      doc["type"] = "response";
      doc["temperature"] = getTemperature();
      serializeJson(doc, Serial);
    }
    messageReady = false;
  }
}

bool checkMessage() {
  // Attempt to deserialize the JSON-formatted message
  DeserializationError error = deserializeJson(doc, message);
  if (error) {
    if (debug) {
      Serial.print(F("\nDeserializeJson() failed: "));
      Serial.println(error.c_str());
    }
    return false;
  }

  // Check message is valid
  const char* type = doc["type"];
  if ((type) && (strcmp(type, "request") == 0))  {
    return true;
  } else {
    if (debug) {
      Serial.print(F("\nInvalid type received: "));
      Serial.println(type);
    }
    return false;
  }
}


float getTemperature() {
  sensors.requestTemperatures(); 
  return sensors.getTempCByIndex(0);
}
