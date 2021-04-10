#include <Arduino.h>

#include <WiFi.h>
#include <WiFiMulti.h>

#include <HTTPClient.h>

#include "credentials.h"

#include <splash.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GrayOLED.h>
#include <Adafruit_SPITFT.h>
#include <gfxfont.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SPITFT_Macros.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

String CMD_AUTH = "/rr_connect?password=";
String CMD_ON = "/rr_gcode?gcode=M106%20P2%20S255%20M106%20P3%20S255"; 
String CMD_OFF = "/rr_gcode?gcode=M106%20P2%20S0%20M106%20P3%20S0";
String CMD_STATUS = "/rr_status?type=1"; 

#define USE_SERIAL Serial

WiFiMulti wifiMulti;

const int TOGGLE_BUTTON = 14;
const int SIGNAL_LED = 2;

String URL_AUTH = String(DUET_URL);
String URL_ON = String(DUET_URL);
String URL_OFF = String(DUET_URL);
String URL_STATUS = String(DUET_URL);

void setup() {
  pinMode(TOGGLE_BUTTON, INPUT_PULLUP);
  pinMode(SIGNAL_LED, OUTPUT);
  attachInterrupt(TOGGLE_BUTTON, isr, FALLING);
  USE_SERIAL.begin(115200);

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
  URL_STATUS.concat(CMD_STATUS);

  USE_SERIAL.printf("Auth URL: %s\n", URL_AUTH.c_str());
  USE_SERIAL.printf("On URL: %s\n", URL_ON.c_str());
  USE_SERIAL.printf("Off URL: %s\n", URL_OFF.c_str());  

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  } 
  display.clearDisplay();
}
 
String sub(String what, String fromMark, String toMark) {
    int fromPos = what.indexOf(fromMark);
    if (fromPos < 0) {
      return "";
    }
    int toPos = what.indexOf(toMark, fromPos + fromMark.length());
    if (toPos < 0) {
      return "";
    }
    return what.substring(fromPos + fromMark.length(), toPos);
}

int onOff = 1;

void printStatus() {
  display.clearDisplay();
  display.setTextSize(1); // Draw 2X-scale text
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print("Licht: ");
  display.println(!onOff); 

  if((wifiMulti.run() == WL_CONNECTED)) {
    HTTPClient http;
    http.begin(URL_AUTH);
    http.GET();
    http.end();

    http.begin(URL_STATUS);  
    int httpCode = http.GET();

    if(httpCode > 0) {
      if(httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        USE_SERIAL.println(payload);
        display.print("Status: ");
        display.println(sub(payload, "{\"status\":\"", "\""));
        display.print("Temp: "); 
        display.println(sub(payload, ",\"current\":[", "]"));
      }  
    } else {
      USE_SERIAL.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
    }
    http.end();
  }

  display.display();
}

void makeRequest(HTTPClient& http, String url) {
  http.begin(url);  
  USE_SERIAL.print("[HTTP] GET...\n");
  int httpCode = http.GET();

  if(httpCode > 0) {
    USE_SERIAL.printf("[HTTP] GET... code: %d\n", httpCode);
    if(httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
    }
  } else {
    USE_SERIAL.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
  }
  http.end();
}

int buttonPressed = 0;
void isr() {
  buttonPressed = 1;
}

void toggleLight() {
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

void loop() {
  if (buttonPressed) {
    toggleLight();
    buttonPressed = 0;    
  }
  delay(5000);
  printStatus();
}