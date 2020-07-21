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
const char pass[] = BW_WIFI_PASSWORD;
int status = WL_IDLE_STATUS;
unsigned long lastSend;
WiFiClient espClient;

// ThingsBoard
const char thingsboardServer[] = BW_THINGSBOARD_SERVER;
const char token[] = BW_THINGSBOARD_TOKEN;
ThingsBoard tb(espClient);


void setup() {
  Serial.begin(BW_SERIAL_BAUD);
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(F("."));
    delay(BW_RETRY_DELAY);
  }
  Serial.print(F("\nIP Address: "));
  Serial.println(WiFi.localIP());
}

void loop() {
  status = WiFi.status();
  if ( status != WL_CONNECTED) {
    while ( status != WL_CONNECTED) {
      Serial.print(F("Attempting to connect to SSID: "));
      Serial.println(ssid);
      status = WiFi.begin(ssid, pass);
      delay(BW_RETRY_DELAY);
    }
    Serial.println(F("Connected to SSID"));
  }

  if ( !tb.connected() ) {
    reconnectThingsBoard();
  }

  if ( millis() - lastSend > BW_SEND_FREQUENCY ) {
    getAndSendData();
    lastSend = millis();
  }

  tb.loop();
}

void reconnectThingsBoard() {
  // Loop until we're reconnected
  while (!tb.connected()) {
    Serial.print(F("Connecting to ThingsBoard node ..."));
    // Attempt to connect (clientId, username, password)
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
  while (messageReady == false) { // blocking but that's ok
    if (Serial.available()) {
      message = Serial.readString();
      messageReady = true;
    }
  }

  // Attempt to deserialize the JSON-formatted message
  DeserializationError error = deserializeJson(doc, message);
  if (error) {
    Serial.print(F("\ndeserializeJson() failed: "));
    Serial.println(error.c_str());
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
    Serial.print(F("\nSending JSON: "));
    Serial.println(jsonChar);
    tb.sendTelemetryJson(jsonChar);
  } else {
    Serial.print(F("\ninvalid type received: "));
    Serial.println(type);
  }
}
