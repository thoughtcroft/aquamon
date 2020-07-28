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

#include "BainsworldConfig.h"  // sensitive config values from here - goes first
#include "CommonConfig.h"      // common values such as timing defaults

#include <ArduinoJson.h>
#include <ArduinoLog.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// Temperature sensors
// specific hardware address for each sensor
#define ONE_WIRE_BUS 2
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
DeviceAddress waterThermometer = { 0x28, 0x3F, 0x80, 0x90, 0x3A, 0x19, 0x01, 0x49 };

String message = "";
bool messageReady = false;
DynamicJsonDocument doc(1024); // ArduinoJson version 6+


void setup() {
  Serial.begin(BW_SERIAL_BAUD);
  delay(BW_RETRY_DELAY);       // wait a bit
  Log.begin(LOG_LEVEL_VERBOSE, &Serial, false);
  while (!Serial && !Serial.available()) {
    // wait for Serial port to be available
  }
  Log.notice(F("" CR));
  Log.notice(F("**********************************" CR));
  Log.notice(F("*** Starting aquamon collector ***" CR));
  Log.notice(F("**********************************" CR));
  Log.notice(F("" CR));

  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_BUILTIN, OUTPUT);

  // setup various sensors
  setupThermometer(waterThermometer, "water temperature sensor");
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
      doc["water_temperature"] = getTemperature(waterThermometer);
      Log.notice(F("Sending response: "));
      serializeJson(doc, Serial);
      flashLED();
    }
    messageReady = false;
  }
}

bool checkMessage() {
  // Attempt to deserialize the JSON-formatted message
  Log.verbose(F("Received message: %s"), message.c_str());
  DeserializationError error = deserializeJson(doc, message);
  if (error) {
    Log.error(F("DeserializeJson() failed: %s" CR), error.c_str());
    return false;
  }
  // Check message is valid
  const char* type = doc["type"];
  if ((type) && (strcmp(type, "request") == 0))  {
    return true;
  } else {
    Log.error(F("Invalid type received: %s" CR), type);
    return false;
  }
}

void setupThermometer(DeviceAddress deviceAddress, const char* deviceName) {
  sensors.begin();
  if (!sensors.validAddress(deviceAddress) || !sensors.isConnected(deviceAddress)) {
    Log.error(F("The %s has an invalid or missing address!" CR), deviceName);
  } else {
    Log.notice(F("Found %s at address %i" CR), deviceName, deviceAddress);
    sensors.setResolution(deviceAddress, 9);
  }
}

float getTemperature(DeviceAddress deviceAddress) {
  sensors.requestTemperaturesByAddress(deviceAddress);
  return sensors.getTempC(deviceAddress);
}

void flashLED() {
  digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on
  delay(BW_RETRY_DELAY);             // wait a bit
  digitalWrite(LED_BUILTIN, LOW);    // turn the LED off
}
