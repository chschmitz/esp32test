#include "ArduinoJson.h"
#include "credentials.h"

void setup() {
  Serial.begin(115200);
}

void loop() {
  char jsonMessage[] = "{ \"foo\": \"bar\", \"age\": \"112\" }";
  StaticJsonDocument<500> doc;
  DeserializationError err = deserializeJson(doc, jsonMessage);
  if (err) {
    Serial.print(F("deserializeJson() failed with code "));
    Serial.println(err.c_str());
  }
  const char* value = doc["foo"];
  Serial.println(value);

  int age = doc["age"];
  Serial.println(age);
  delay(1000);
}
