#define ONBOARD_LED 2

void setup() {
    Serial.begin(115200);
    pinMode(ONBOARD_LED, OUTPUT);
    Serial.println("setup");
}

void loop() {
    Serial.println("Loop");
    delay(500);
    digitalWrite(ONBOARD_LED, HIGH);
    delay(500);
    digitalWrite(ONBOARD_LED, LOW);
}
