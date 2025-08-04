#define BLYNK_TEMPLATE_ID "TMPL3oqQsVU7o"
#define BLYNK_TEMPLATE_NAME "smart home 2"
#define BLYNK_AUTH_TOKEN "KAvMYi00g71WBGseVLZAX0XcFcqPT_ub"
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <SPI.h>
#include <MFRC522.h>

// WiFi credentials
char ssid[] = "NARZO";
char pass[] = "123456780";

// RFID Pins
#define RST_PIN 22
#define SS_PIN  21
MFRC522 rfid(SS_PIN, RST_PIN);

// Relay Pins
int relay1 = 13;
int relay2 = 12;
int relay3 = 14;
int relay4 = 27; // Water Pump

// Ultrasonic Sensor
#define trigPin 22
#define echoPin 23
const int tankHeight = 40; // Tank height in cm

BlynkTimer timer;
int tankLevel = 0;
bool manualControl = false;
int pumpState = LOW;

// Relay control from app
BLYNK_WRITE(V1) {
  int val = param.asInt();
  digitalWrite(relay1, val);
  Serial.println("Relay 1 (Appliance 1): " + String(val ? "ON" : "OFF"));
}

BLYNK_WRITE(V2) {
  int val = param.asInt();
  digitalWrite(relay2, val);
  Serial.println("Relay 2 (Appliance 2): " + String(val ? "ON" : "OFF"));
}

BLYNK_WRITE(V3) {
  int val = param.asInt();
  digitalWrite(relay3, val);
  Serial.println("Relay 3 (Appliance 3): " + String(val ? "ON" : "OFF"));
}

BLYNK_WRITE(V4) {
  int val = param.asInt();
  manualControl = true;
  pumpState = val;
  digitalWrite(relay4, pumpState);
  Serial.println("Relay 4 (Pump - Manual): " + String(pumpState ? "ON" : "OFF"));
}

void setup() {
  Serial.begin(115200);

  pinMode(relay1, OUTPUT);
  pinMode(relay2, OUTPUT);
  pinMode(relay3, OUTPUT);
  pinMode(relay4, OUTPUT);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  digitalWrite(relay1, LOW);
  digitalWrite(relay2, LOW);
  digitalWrite(relay3, LOW);
  digitalWrite(relay4, LOW);

  SPI.begin();  // SCK=18, MOSI=23, MISO=19
  rfid.PCD_Init();

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  timer.setInterval(2000L, sendTankLevel);
  timer.setInterval(1000L, checkRFID);
}

void loop() {
  Blynk.run();
  timer.run();
}

// Water Level Monitoring
void sendTankLevel() {
  long duration;
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  duration = pulseIn(echoPin, HIGH, 30000); // timeout of 30ms
  if (duration == 0) {
    Serial.println("Ultrasonic sensor error: No reading");
    Blynk.virtualWrite(V5, 0);
    return;
  }

  int distance = duration * 0.034 / 2;
  int level = tankHeight - distance;
  level = constrain(level, 0, tankHeight);
  tankLevel = map(level, 0, tankHeight, 0, 100);
  Blynk.virtualWrite(V5, tankLevel);

  Serial.println("Ultrasonic Reading: " + String(distance) + " cm");
  Serial.println("Calculated Tank Level: " + String(tankLevel) + " %");

  // Auto Pump Logic
  if (!manualControl) {
    if (tankLevel <= 20 && pumpState == LOW) {
      pumpState = HIGH;
      digitalWrite(relay4, pumpState);
      Serial.println("Pump Auto: ON (Tank Low)");
    } else if (tankLevel >= 95 && pumpState == HIGH) {
      pumpState = LOW;
      digitalWrite(relay4, pumpState);
      Serial.println("Pump Auto: OFF (Tank Full)");
    }
  } else {
    Serial.println("Pump is under Manual Control");
  }
}

// RFID Reading
void checkRFID() {
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) return;

  String uidStr = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    uidStr += String(rfid.uid.uidByte[i], HEX);
  }

  uidStr.toUpperCase();

  String member = "";
  if (uidStr == "3C74BA03") member = "Member 1";
  else if (uidStr == "D394C12C") member = "Member 2";
  else if (uidStr == "250E0204") member = "Member 3";
  else if (uidStr == "03E17731") member = "Member 4";
  else member = "Unknown Card";

  String log = member + " Accessed at: " + getTime();
  Serial.println("RFID Detected → " + log);
  Blynk.virtualWrite(V6, log);

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
}

// Get time string
String getTime() {
  time_t now = time(nullptr);
  struct tm* p_tm = localtime(&now);
  char timeStr[25];
  sprintf(timeStr, "%02d:%02d:%02d", p_tm->tm_hour, p_tm->tm_min, p_tm->tm_sec);
  return String(timeStr);
}