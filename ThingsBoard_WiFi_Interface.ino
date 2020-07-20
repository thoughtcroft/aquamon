/*------------------------------------------------------------------------------
  20/07/2020
  Author: bainsworld
  Platforms: ESP8266
  Language: C++/Arduino
  File: ThingsBoard_WiFi_Interface.ino
  ------------------------------------------------------------------------------
  Description:
  Controller for connecting to my ThingsBoard instance and posting data that has
  been sent from the Arduino Uno via the Serial connection
  ------------------------------------------------------------------------------*/
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include <ThingsBoard.h>
#include "BainsworldConfig.h"

// Timing values
const int SEND_FREQUENCY = 1000;
const int RETRY_DELAY = 500;
const int RECONNECT_DELAY = 5000;
const int SERIAL_BAUD = 115200;

// Wi-Fi
const char ssid[] = WIFI_SSID;
const char pass[] = WIFI_PASSWORD;
int status = WL_IDLE_STATUS;
unsigned long lastSend;
WiFiClient espClient;

// ThingsBoard
const char thingsboardServer[] = THINGSBOARD_SERVER;
const char token[] = THINGSBOARD_TOKEN;
ThingsBoard tb(espClient);


void setup() {
  Serial.begin(SERIAL_BAUD);
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(RETRY_DELAY);
  }
  Serial.println("");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  status = WiFi.status();
  if ( status != WL_CONNECTED) {
    while ( status != WL_CONNECTED) {
      Serial.print("Attempting to connect to SSID: ");
      Serial.println(ssid);
      status = WiFi.begin(ssid, pass);
      delay(RETRY_DELAY);
    }
    Serial.println("Connected to SSID");
  }

  if ( !tb.connected() ) {
    reconnectThingsBoard();
  }

  if ( millis() - lastSend > SEND_FREQUENCY ) {
    getAndSendData();
    lastSend = millis();
  }

  tb.loop();
}

void reconnectThingsBoard() {
  // Loop until we're reconnected
  while (!tb.connected()) {
    Serial.print("Connecting to ThingsBoard node ...");
    // Attempt to connect (clientId, username, password)
    if ( tb.connect(thingsboardServer, token) ) {
      Serial.println( "[DONE]" );
    } else {
      Serial.print( "[FAILED]" );
      Serial.println( " : retrying..." );
      // Wait before retrying
      delay( RECONNECT_DELAY );
    }
  }
}

void getAndSendData() {
  // Send a JSON-formatted request with key "type" and value "request"
  // then obtain the response and submit keys to ThingsBoard
  DynamicJsonDocument doc(1024);
  long temperature = 0;
  
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
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.c_str());
    return;
  }

  // Just send temperature for now
  temperature = doc["temperature"];
  Serial.print("Sending temperature: ");
  Serial.println(temperature);
  tb.sendTelemetryFloat("temperature", temperature);
}
