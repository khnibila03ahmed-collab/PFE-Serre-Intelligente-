#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "DHT.h"
#include <DHT_U.h>
#include <Servo.h>
#define Type DHT11

// ======== LCD ========
LiquidCrystal_I2C lcd(0x20, 16, 2);

// ======== DHT ========
int sensePin = 8;
DHT HT(sensePin, Type);
float humidity;
float temperature;

// ======== Pins ========
int ledPin1   = 9;
int ledPin2   = 10;
int ledPin3   = 11;
int fanPin    = 12;
int buzzerPin = 6;
int servoPin  = 7;

#define CO2_PIN       A0
#define CO2_THRESHOLD 800

const int ldrPin  = A1;
const int lampPin = 5;
int ldrValue      = 0;
int seuilLDR      = 300;

int soilPin  = A2;
int pompePin = 4;
bool pompeActive = false;

// ======== LCD screen toggle ========
bool showScreen1 = true;
unsigned long lastSwitch = 0;
#define SCREEN_INTERVAL 1000

// ======== Servo state ========
Servo myServo;
bool windowOpen     = false;
bool buzzer_sounded = false;

// ======== openWindow ========
void openWindow() {
  if (!windowOpen) {
    myServo.write(135);
    delay(500);
    windowOpen     = true;
    buzzer_sounded = false;
  }
  digitalWrite(fanPin, HIGH);
  if (!buzzer_sounded) {
    digitalWrite(buzzerPin, HIGH);
    delay(500);
    digitalWrite(buzzerPin, LOW);
    buzzer_sounded = true;
  }
}

// ======== closeWindow ========
void closeWindow() {
  if (windowOpen) {
    myServo.write(0);
    delay(500);
    windowOpen     = false;
    buzzer_sounded = false;
  }
  digitalWrite(fanPin,    LOW);
  digitalWrite(buzzerPin, LOW);
}

// ======== Setup ========
void setup() {
  pinMode(ledPin1,   OUTPUT);
  pinMode(ledPin2,   OUTPUT);
  pinMode(ledPin3,   OUTPUT);
  pinMode(fanPin,    OUTPUT);
  pinMode(buzzerPin, OUTPUT);
  pinMode(lampPin,   OUTPUT);
  pinMode(pompePin,  OUTPUT);

  digitalWrite(ledPin1,   LOW);
  digitalWrite(ledPin2,   LOW);
  digitalWrite(ledPin3,   LOW);
  digitalWrite(fanPin,    LOW);
  digitalWrite(buzzerPin, LOW);
  digitalWrite(lampPin,   LOW);
  digitalWrite(pompePin,  LOW);

  myServo.attach(servoPin);
  myServo.write(0);

  Serial.begin(9600);
  HT.begin();

  Wire.begin();
  lcd.init();
  lcd.backlight();
  lcd.setCursor(4, 0);
  lcd.print("Hello!");
  lcd.setCursor(0, 1);
  lcd.print(" Salam Alikom");
  delay(1000);
  lcd.clear();
}

// ======== Loop ========
void loop() {
  unsigned long now = millis();

  // ---- Lecture capteurs ----
  humidity    = HT.readHumidity();
  temperature = HT.readTemperature();
  int co2     = analogRead(CO2_PIN);
  ldrValue    = analogRead(ldrPin);

  int soilRaw      = analogRead(soilPin);
  int soilHumidity = map(soilRaw, 1023, 0, 0, 100);
  soilHumidity     = constrain(soilHumidity, 0, 100);

  // ---- LDR / Lamp ----
  digitalWrite(lampPin, ldrValue < seuilLDR ? HIGH : LOW);

  // ---- Pompe ----
  if (soilHumidity < 30)       pompeActive = true;
  else if (soilHumidity >= 90) pompeActive = false;
  digitalWrite(pompePin, pompeActive ? HIGH : LOW);

  // ---- LEDs humidité air ----
  if (humidity < 40) {
    digitalWrite(ledPin1, HIGH);
    digitalWrite(ledPin2, LOW);
    digitalWrite(ledPin3, LOW);
  } else if (humidity <= 80) {
    digitalWrite(ledPin1, LOW);
    digitalWrite(ledPin2, HIGH);
    digitalWrite(ledPin3, LOW);
  } else {
    digitalWrite(ledPin1, LOW);
    digitalWrite(ledPin2, LOW);
    digitalWrite(ledPin3, HIGH);
  }

  // ---- Cherjem / Fan / Buzzer ----
  bool co2High = (co2 > CO2_THRESHOLD);
  if (co2High || temperature > 25 || humidity < 40 || humidity > 80) {
    openWindow();
  } else {
    closeWindow();
  }

  // ---- Serial ----
  Serial.print("Temp: ");      Serial.print(temperature);
  Serial.print(" | Hum: ");    Serial.print(humidity);
  Serial.print(" | CO2: ");    Serial.print(co2);
  Serial.print(" | LDR: ");    Serial.print(ldrValue);
  Serial.print(" | Soil: ");   Serial.print(soilHumidity);
  Serial.print("% | Pompe: "); Serial.println(pompeActive ? "ON" : "OFF");

  // ---- LCD switch ----
  if (now - lastSwitch >= SCREEN_INTERVAL) {
    lastSwitch  = now;
    showScreen1 = !showScreen1;
    lcd.clear();
  }

  if (showScreen1) {
    // Screen 1: Temp + Hum + CO2
    lcd.setCursor(0, 0);
    lcd.print("T:");
    lcd.print(temperature, 1);
    lcd.print((char)223);
    lcd.print("C H:");
    lcd.print(humidity, 0);
    lcd.print("%  ");

    lcd.setCursor(0, 1);
    if (co2High) {
      lcd.print("CO2 HIGH:");
      lcd.print(co2);
      lcd.print("  ");
    } else {
      lcd.print("CO2:");
      lcd.print(co2);
      lcd.print(" PPM  ");
    }
  } else {
    // Screen 2: LDR + Soil
    lcd.setCursor(0, 0);
    lcd.print("LDR:");
    lcd.print(ldrValue);
    lcd.print(ldrValue < seuilLDR ? " ON " : " OFF");

    lcd.setCursor(0, 1);
    lcd.print("Soil:");
    lcd.print(soilHumidity);
    lcd.print("%");
    lcd.print(pompeActive ? " P:ON " : " P:OFF");
  }

  delay(500);
}
