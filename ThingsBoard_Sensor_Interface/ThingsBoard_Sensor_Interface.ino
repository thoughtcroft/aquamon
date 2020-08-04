/*------------------------------------------------------------------------------
  21/07/2020
  Author: bainsworld
  Platform: Arduino Uno
  Language: C++/Arduino
  File: ThingsBoard_Sensor_Interface.ino
  ------------------------------------------------------------------------------
  Description:
  Controller for connecting to various sensors using the Arduino Uno and then
  sending the telemetry data to ThingsBoard via an ESP8266 D1 Mini by Serial
  communications. Sensors are:
   - DS18B20 water temperature
   - DS18B20 air temperature
   - YF-S201 water fow rate
  ------------------------------------------------------------------------------*/

#include "BainsworldConfig.h"  // sensitive config values from here - goes first
#include "CommonConfig.h"      // common values such as timing defaults

#include <ArduinoJson.h>
#include <ArduinoLog.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// temperature sensors - specific hardware address for each sensor
#define ONE_WIRE_BUS 2
#define TEMP_RESOLUTION 10
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
DeviceAddress waterThermometer = { 0x28, 0x3F, 0x80, 0x90, 0x3A, 0x19, 0x01, 0x49 };
DeviceAddress airThermometer =   { 0x28, 0x1A, 0x52, 0x94, 0x97, 0x08, 0x03, 0xB5 };
// calibration data { low, high }
float waterTempRange[] = { 0.5, 100.0 };
float airTempRange[] =   { 1.4, 99.0  };
float refTempRange[] =   { 0.0, 99.6  };

// Flow rate sensor
#define PULSES_PER_LITRE 450   // From the datasheet (7.5 Hz at 1L/min)
int flowPin = 3;               // This is the input pin on the Arduino
volatile int flowCounter;      // This integer needs to be set as volatile to ensure it updates correctly during the interrupt process

// Messages are communicated via UART
String message = "";
bool messageReady = false;
DynamicJsonDocument doc(1024); // ArduinoJson version 6+


void setup() {
  Serial.begin(BW_SERIAL_BAUD);
  delay(BW_RETRY_DELAY);       // wait a bit
  while (!Serial && !Serial.available()) {
    // wait for Serial port to be available
  }
  Log.begin(LOG_LEVEL_VERBOSE, &Serial, false);
  Log.notice(F("" CR));
  Log.notice(F("**********************************" CR));
  Log.notice(F("*** Starting aquamon collector ***" CR));
  Log.notice(F("**********************************" CR));
  Log.notice(F("" CR));

  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_BUILTIN, OUTPUT);

  // setup various sensors
  setupThermometer(waterThermometer, "water temperature sensor");
  setupThermometer(airThermometer, "air temperature sensor");
  pinMode(flowPin, INPUT);
}


void loop() {
  // Monitor serial communication
  while (Serial.available()) {
    message = Serial.readString();
    messageReady = true;
  }
  // Only process message if one is requested
  if (messageReady) {
    // The only messages we'll parse will be formatted in JSON
    if (checkMessage()) {
      doc["type"] = "response";
      doc["water_temperature"] = getTemperature(waterThermometer, waterTempRange);
      doc["air_temperature"] = getTemperature(airThermometer, airTempRange);
      doc["flow_rate"] = calculateFlowRate();
      Log.notice(F("Sending response: "));
      serializeJson(doc, Serial);
      Log.notice(CR);
      flashLED();  // so a human observer knows we just sent data
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
    Log.notice(F("Found %s at address %d" CR), deviceName, deviceAddress);
    sensors.setResolution(deviceAddress, TEMP_RESOLUTION);
  }
}

float getTemperature(DeviceAddress deviceAddress, float rawTempRange[]) {
  // adjust the reading based on measured point calibration
  static float refRange = refTempRange[HIGH] - refTempRange[LOW];
  float rawRange = rawTempRange[HIGH] - rawTempRange[LOW];
  sensors.requestTemperaturesByAddress(deviceAddress);
  float rawTemp = sensors.getTempC(deviceAddress);
  return ((((rawTemp - rawTempRange[LOW]) * refRange) / rawRange) + refTempRange[LOW]);
}

void flashLED() {
  // turn the LED on and off so we know it is working
  digitalWrite(LED_BUILTIN, HIGH);
  delay(BW_RETRY_DELAY);
  digitalWrite(LED_BUILTIN, LOW);
}

float calculateFlowRate() {
  // pulses in one second converted to litres per minute
  float flowCounter = getFlowCounter(1000);
  return ((flowCounter / PULSES_PER_LITRE) * 60);
}

float getFlowCounter(int ticks) {
  // enable interrupts to count pulses during a defined period
  static int interrupt = digitalPinToInterrupt(flowPin);
  attachInterrupt(interrupt, updateFlowCounter, RISING);
  flowCounter = 0;                  // reset the counter
  delay(ticks);                     // wait for configured period
  detachInterrupt(interrupt);       // stop updating counter
  return flowCounter;
}

void updateFlowCounter() {
  // Interrupt Service Routine to count pulses
  flowCounter++;
}
