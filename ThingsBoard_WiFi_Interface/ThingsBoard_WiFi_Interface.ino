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
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include <ThingsBoard.h>
#include "BainsworldConfig.h"  // sensitive config values from here
#include "CommonConfig.h"      // common values such as timing defaults

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

bool debug = BW_DEBUG;

void setup() {
  Serial.begin(BW_SERIAL_BAUD);
  delay(BW_RECONNECT_DELAY);
  Serial.println();
  Serial.println(F("Starting aquamon receiver ..."));
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
}

void connectWiFi() {
  status = WiFi.begin(ssid, password);
  if (status != WL_CONNECTED) {
    Serial.print(F("Connecting to SSID: "));
    Serial.print(ssid);
    while (status != WL_CONNECTED) {
      Serial.print(F("."));
      delay(BW_RETRY_DELAY);
      status = WiFi.status();
    }
    Serial.println(F("[DONE]"));
    Serial.print(F("IP Address: "));
    Serial.println(WiFi.localIP());
  }
}

void connectThingsBoard() {
  // Loop until we're connected
  while (!tb.connected()) {
    Serial.print(F("Connecting to ThingsBoard node......"));
    if ( tb.connect(thingsboardServer, token) ) {
      Serial.println(F("[DONE]"));
    } else {
      Serial.print(F("[FAILED]"));
      Serial.println(F(" : retrying..."));
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
  DeserializationError error = deserializeJson(doc, message);
  if (error) {
    if (debug) {
      Serial.print(F("\nDeserializeJson() failed: "));
      Serial.println(error.c_str());
    }
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
    if (debug) {
      Serial.print(F("\nSending JSON: "));
      Serial.println(jsonChar);
    }
    tb.sendTelemetryJson(jsonChar);
  } else {
    if (debug) {
      Serial.print(F("\nInvalid type received: "));
      Serial.println(type);
    }
  }
}
