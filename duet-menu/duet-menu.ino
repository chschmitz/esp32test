#include <ArduinoJson.h>
#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "credentials.h"
#include "AiEsp32RotaryEncoder.h"
#include "Arduino.h"
#include <Adafruit_SSD1306.h>
#include <Adafruit_GrayOLED.h>
#include <Adafruit_SPITFT.h>
#include <gfxfont.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SPITFT_Macros.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)

#define ROTARY_ENCODER_A_PIN 32
#define ROTARY_ENCODER_B_PIN 33
#define ROTARY_ENCODER_BUTTON_PIN 25
#define ROTARY_ENCODER_VCC_PIN -1 
#define ROTARY_ENCODER_STEPS 4

#define min(a, b) ((a) < (b) ? (a) : (b))

String CMD_AUTH = "/rr_connect?password=";
String CMD_LIST = "/rr_filelist?dir=macros/test"; 

String URL_AUTH = String(DUET_URL);
String URL_LIST = String(DUET_URL);

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
AiEsp32RotaryEncoder rotaryEncoder = AiEsp32RotaryEncoder(ROTARY_ENCODER_A_PIN, ROTARY_ENCODER_B_PIN, ROTARY_ENCODER_BUTTON_PIN, ROTARY_ENCODER_VCC_PIN, ROTARY_ENCODER_STEPS);

HTTPClient http;
int firstDisplayed = 0;
int screenHeight = 8;

char **macros;
int nmacros;

portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

#define NO_MACRO -1
volatile int executeMacro = NO_MACRO;

void setup() {
  Serial.begin(115200);

  connectWifi();

  URL_AUTH.concat(CMD_AUTH);
  URL_AUTH.concat(DUET_PASSWD);
  URL_LIST.concat(CMD_LIST);

  Serial.printf("Auth URL: %s\n", URL_AUTH.c_str());
  Serial.printf("List URL: %s\n", URL_LIST.c_str());
 
  authenticate(http);
  readMacros(http);

  setupDisplay();
  setupRotaryEncoder();

  displayMenu();
}

void connectWifi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.print("\nWiFi connected:");
  Serial.println(WiFi.localIP());
}

String makeRequest(HTTPClient& http, String url) {
  http.begin(url);  
  Serial.print("[HTTP] GET...\n");
  int httpCode = http.GET();
  String payload;
  
  if (httpCode > 0) {
    Serial.printf("[HTTP] GET... code: %d\n", httpCode);
    if(httpCode == HTTP_CODE_OK) {
      payload = http.getString();
    }
  } else {
    Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
    Serial.printf("[HTTP] GET... failed, error: %d\n", httpCode);
  }
  http.end();
  return payload;
}

void authenticate(HTTPClient& http) {
  makeRequest(http, URL_AUTH);
  Serial.println("Authenticated.");
}

void readMacros(HTTPClient& http) {
  Serial.println("readMacros");
  
  String json = makeRequest(http, URL_LIST);
  Serial.println(json); 
  StaticJsonDocument<64> filter;
  filter["files"][0]["name"] = true;
  DynamicJsonDocument doc(3000);
  DeserializationError error = deserializeJson(doc, json, DeserializationOption::Filter(filter));
  JsonArray jmacros = doc["files"].as<JsonArray>();
  nmacros = jmacros.size();

  macros = new char*[nmacros];
  for (int i = 0; i < nmacros; i++) {
    const char *ithmacro = jmacros[i]["name"].as<char *>();
    macros[i] = new char[strlen(ithmacro)];
    strcpy(macros[i], ithmacro);
    Serial.print("*");
    Serial.println(macros[i]);
  }  
}

void setupDisplay() {
  Serial.println("setupDisplay()");
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  } 
}

volatile unsigned long lastTimePressed = 0;
void rotaryClick() {
  Serial.println("rotaryClick()");
  //ignore multiple press in that time milliseconds
  if (millis() - lastTimePressed < 500)
  {
    return;
  }
  portENTER_CRITICAL_ISR(&mux);
  lastTimePressed = millis();
  Serial.print("Selected: ");
  executeMacro = rotaryEncoder.readEncoder();
  Serial.println(macros[executeMacro]);
  portEXIT_CRITICAL_ISR(&mux);
}

void rotaryLoop() {
  if (rotaryEncoder.encoderChanged()) {
    displayMenu();
  }
}

void setupRotaryEncoder() {
  Serial.println("setupRotaryEncoder()");
  rotaryEncoder.begin();

  rotaryEncoder.setup(
    [] { rotaryEncoder.readEncoder_ISR(); },
    [] { rotaryClick(); });

  Serial.print(nmacros);
  Serial.println(" macros.");
  rotaryEncoder.setBoundaries(0, nmacros - 1, false);
  rotaryEncoder.setAcceleration(25); 
}

void displayMenu() {
  Serial.println("displayMenu()");
  
  int selectedLine = rotaryEncoder.readEncoder();
  if (selectedLine < firstDisplayed) {
    firstDisplayed = selectedLine;
  }
  if (selectedLine > firstDisplayed + screenHeight - 1) {
    firstDisplayed = selectedLine - screenHeight + 1;
  }
    
  display.clearDisplay();
  display.setTextSize(8 / screenHeight);
  display.setCursor(0, 0);
  for (int i = 0; i < min(screenHeight, nmacros); i++) {
    int item = i + firstDisplayed;
 
    if (item == selectedLine) {
      display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
      Serial.println(macros[i]);
    } else {
      display.setTextColor(SSD1306_WHITE);
    }
    display.println(macros[i]);
  }
  display.display();
}

void loop() { 
  rotaryLoop(); 
  int executeMacroLocal = NO_MACRO;
  portENTER_CRITICAL(&mux);
  executeMacroLocal = executeMacro;
  executeMacro = NO_MACRO;
  portEXIT_CRITICAL(&mux);

  if (executeMacroLocal != NO_MACRO) {
    Serial.printf("I would be executing %s right now.\n", macros[executeMacroLocal]); 
    delay(1000);
  }
  
  delay(100);
}
