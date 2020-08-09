/*------------------------------------------------------------------------------
  20/07/2020
  Author: bainsworld
  Platforms: ESP8266
  Language: C++/Arduino
  File: ThingsBoard_WiFi_Interface.ino
  ------------------------------------------------------------------------------
  Description:
  Controller for connecting to my ThingsBoard instance via Wi-Fi and posting data
  that has been sent from the Arduino Uno via the Serial connection
  ------------------------------------------------------------------------------*/

#include "BainsworldConfig.h"  // sensitive config values from here - goes first
#include "CommonConfig.h"      // common values such as timing defaults

#include <ArduinoJson.h>
#include <ArduinoLog.h>
#include <ESP8266WiFi.h>
#include <ThingsBoard.h>

// Wi-Fi
const char ssid[] = BW_WIFI_SSID;
const char password[] = BW_WIFI_PASSWORD;
int status = WL_IDLE_STATUS;
unsigned long lastSend;
WiFiClient espClient;

// ThingsBoard
const char thingsboardServer[] = BW_THINGSBOARD_SERVER;
const char token[] = BW_THINGSBOARD_TOKEN;
ThingsBoard tb(espClient);


void setup() {
  Serial.begin(BW_SERIAL_BAUD);
  delay(500);
  while (!Serial && !Serial.available()) {
    // wait for Serial port to be available
  }
  Log.begin(LOG_LEVEL_VERBOSE, &Serial, false);
  Log.notice(F("" CR));
  Log.notice(F("*******************************" CR));
  Log.notice(F("*** Starting aquamon sender ***" CR));
  Log.notice(F("*******************************" CR));
  Log.notice(F("" CR));

  // initialize digital pin LED_BUILTIN as an output
  // and turn it off (that is HIGH on D1 mini)
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
}


void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }

  if (!tb.connected()) {
    connectThingsBoard();
  }

  if (millis() - lastSend > BW_SEND_FREQUENCY) {
    getAndSendData();
    lastSend = millis();
  }

  tb.loop();
  flashLED();  // so a human observer knows we are doing something
}

void connectWiFi() {
  status = WiFi.begin(ssid, password);
  if (status != WL_CONNECTED) {
    Log.verbose(F("Connecting to SSID: %s..."), ssid);
    while (status != WL_CONNECTED) {
      // Wait before retrying
      delay(BW_RETRY_DELAY);
      status = WiFi.status();
    }
    Log.verbose(F("[DONE]" CR));
    Log.verbose(F("IP Address: %s" CR), WiFi.localIP().toString().c_str());
  }
}

void connectThingsBoard() {
  // Loop until we're connected
  while (!tb.connected()) {
    Log.verbose(F("Connecting to ThingsBoard node..."));
    if ( tb.connect(thingsboardServer, token) ) {
      Log.verbose(F("[DONE]" CR));
    } else {
      Log.verbose(F("[FAILED]"));
      Log.verbose(F(" : retrying..."));
      // Wait before retrying
      delay( BW_RECONNECT_DELAY );
    }
  }
}

void getAndSendData() {
  // Send a JSON-formatted request with key "type" and value "request"
  // then obtain the response and submit keys to ThingsBoard
  DynamicJsonDocument doc(1024);

  // Sending the request
  doc["type"] = "request";
  serializeJson(doc, Serial);

  // Reading the response
  boolean messageReady = false;
  String message = "";
  while ( messageReady == false ) { // blocking but that's ok
    if (Serial.available()) {
      message = Serial.readString();
      messageReady = true;
    }
  }

  // Attempt to deserialize the JSON-formatted message
  Log.verbose(F(CR "Received message: %s"), message.c_str());
  DeserializationError error = deserializeJson(doc, message);
  if (error) {
    Log.error(F("DeserializeJson() failed: %s" CR), error.c_str());
    return;
  }

  // Check message is valid
  const char* type = doc["type"];
  if ((type) && (strcmp(type, "response") == 0))  {
    // Send JSON without the type keyword
    // everything else is telemetry data
    char jsonChar[100];
    doc.remove("type");
    serializeJson(doc, jsonChar);
    Log.verbose(F("Sending JSON: %s" CR), jsonChar);
    tb.sendTelemetryJson(jsonChar);
  } else {
    Log.error(F("Invalid message type received: %s" CR), type);
  }
}

void flashLED() {
  // turn the LED on and off so we know it is working
  // note that it is inverted on D1 mini so LOW is on
  digitalWrite(LED_BUILTIN, LOW);
  delay(BW_RETRY_DELAY);
  digitalWrite(LED_BUILTIN, HIGH);
}
