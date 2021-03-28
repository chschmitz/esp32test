/**
 * BasicHTTPClient.ino
 *
 *  Created on: 24.05.2015
 *
 */

#include <Arduino.h>

#include <WiFi.h>
#include <WiFiMulti.h>

#include <HTTPClient.h>

#include "credentials.h"

String CMD_AUTH = "/rr_connect?password=";
String CMD_ON = "/rr_gcode?gcode=M106%20P2%20S255%20M106%20P3%20S255"; 
String CMD_OFF = "/rr_gcode?gcode=M106%20P2%20S0%20M106%20P3%20S0";

#define USE_SERIAL Serial

WiFiMulti wifiMulti;

const int TOGGLE_BUTTON = 14;
const int SIGNAL_LED = 2;

String URL_AUTH = String(DUET_URL);
String URL_ON = String(DUET_URL);
String URL_OFF = String(DUET_URL);

void setup() {
  pinMode(TOGGLE_BUTTON, INPUT_PULLUP);
  pinMode(SIGNAL_LED, OUTPUT);
  attachInterrupt(TOGGLE_BUTTON, isr, FALLING);
  USE_SERIAL.begin(115200);

  USE_SERIAL.println();
  USE_SERIAL.println();
  USE_SERIAL.println();

  for(uint8_t t = 4; t > 0; t--) {
    USE_SERIAL.printf("[SETUP] WAIT %d...\n", t);
    USE_SERIAL.flush();
    delay(1000);
  }
  wifiMulti.addAP(WIFI_SSID, WIFI_PASSWD);

  URL_AUTH.concat(CMD_AUTH);
  URL_AUTH.concat(DUET_PASSWD);
  URL_ON.concat(CMD_ON);
  URL_OFF.concat(CMD_OFF);

  USE_SERIAL.printf("Auth URL: %s\n", URL_AUTH.c_str());
  USE_SERIAL.printf("On URL: %s\n", URL_ON.c_str());
  USE_SERIAL.printf("Off URL: %s\n", URL_OFF.c_str());  
}

void makeRequest(HTTPClient& http, String url) {
  http.begin(url);  
  USE_SERIAL.print("[HTTP] GET...\n");
  int httpCode = http.GET();

  if(httpCode > 0) {
    USE_SERIAL.printf("[HTTP] GET... code: %d\n", httpCode);
    if(httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      USE_SERIAL.println(payload);
    }
  } else {
    USE_SERIAL.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
  }
  http.end();
}

int onOff = 1;
void makeRequest() {
  if((wifiMulti.run() == WL_CONNECTED)) {

    HTTPClient http;
    USE_SERIAL.print("[HTTP] begin...\n");
        
    makeRequest(http, URL_AUTH);
    if (onOff) {
      USE_SERIAL.print("[HTTP] on...\n");      
      makeRequest(http, URL_ON);
      digitalWrite(SIGNAL_LED, HIGH);
    } else {
      USE_SERIAL.print("[HTTP] off...\n");      
      makeRequest(http, URL_OFF);
      digitalWrite(SIGNAL_LED, LOW);
    }
    http.end();
  }

  onOff = !onOff;
}

int lastInterrupt = 0;
int wait = 1;
void isr() {
  if (millis() - lastInterrupt > 50) {
    wait = 0;
  }
  lastInterrupt = millis();  
}

void loop() {
  while (wait) {
    // FIXME: there must be a better way to synchronize this
    delay(100);
  }
  USE_SERIAL.print("Button pressed");
  makeRequest();
  wait = 1;
}