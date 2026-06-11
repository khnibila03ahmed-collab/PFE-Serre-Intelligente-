#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "DHT.h"
#include <DHT_U.h>
#include <Servo.h>

#define Type DHT11

// ======== LCD ========
LiquidCrystal_I2C lcd1(0x27, 16, 2);
LiquidCrystal_I2C lcd2(0x3F, 16, 2);

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
#define CO2_THRESHOLD 700

const int ldrPin  = A1;
const int lampPin = 5;
int ldrValue      = 0;
int seuilLDR      = 300;

int soilPin  = A2;
int pompePin = 4;

// ======== Servo ========
Servo myServo;

bool pompeActive    = false;
bool windowOpen     = false;
bool co2AlarmActive = false;
char Incoming_value = 0;
bool modeManuel = false;

// ======== timing dyal CO2 beep intermittent ========
unsigned long lastBeepTime = 0;
bool beepState = false;

#define BEEP_ON  300
#define BEEP_OFF 300

// ======== [FIX] timing dyal beepOpen non-bloquant ========
unsigned long beepOpenStart = 0;
bool beepOpenActive = false;

// ======== startBeepOpen: bdl delay(2000) ========
void startBeepOpen() {
  digitalWrite(buzzerPin, LOW);
  beepOpenStart = millis();
  beepOpenActive = true;
}

// ======== openWindow ========
void openWindow() {
  if (!windowOpen) {
    myServo.write(135);
    delay(2000);
    windowOpen = true;

    if (!co2AlarmActive) {
      startBeepOpen(); // [FIX] bdlna beepOpen() b startBeepOpen()
    }
  }

  // Relay inverse: LOW = ON
  digitalWrite(fanPin, LOW);
}

// ======== closeWindow ========
void closeWindow() {
  if (windowOpen) {
    myServo.write(0);
    delay(500);
    windowOpen = false;
  }

  // Relay inverse: HIGH = OFF
  digitalWrite(fanPin, HIGH);
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

  digitalWrite(ledPin1, LOW);
  digitalWrite(ledPin2, LOW);
  digitalWrite(ledPin3, LOW);

  // ===== Relay inverse =====
  digitalWrite(fanPin, HIGH);
  digitalWrite(lampPin, HIGH);
  digitalWrite(pompePin, HIGH);

  digitalWrite(buzzerPin, HIGH);

  myServo.attach(servoPin);
  myServo.write(0);

  Serial.begin(9600);

  HT.begin();
  Wire.begin();

  // ===== LCD1 =====
  lcd1.init();
  lcd1.backlight();

  lcd1.setCursor(3, 0);
  lcd1.print("Hello!");

  lcd1.setCursor(0, 1);
  lcd1.print(" Salam Alikom");

  // ===== LCD2 =====
  lcd2.init();
  lcd2.backlight();

  lcd2.setCursor(2, 0);
  lcd2.print(":(");

  lcd2.setCursor(0, 1);
  lcd2.print("  Initializing..");

  delay(1500);

  lcd1.clear();
  lcd2.clear();
}

// ======== Loop ========
void loop() {

  unsigned long now = millis();
  // ===== Bluetooth =====
if (Serial.available() > 0)
{
  Incoming_value = Serial.read();

  switch(Incoming_value)
  {
    case 'A':
      modeManuel = false;
      break;

    case 'M':
      modeManuel = true;
      break;

    case '1':
      if(modeManuel)
        digitalWrite(lampPin, LOW);
      break;

    case '0':
      if(modeManuel)
        digitalWrite(lampPin, HIGH);
      break;

    case '2':
      if(modeManuel)
      {
        myServo.write(135);
        windowOpen = true;
        digitalWrite(fanPin, LOW);
      }
      break;

    case '3':
      if(modeManuel)
      {
        myServo.write(0);
        windowOpen = false;
        digitalWrite(fanPin, HIGH);
      }
      break;

    case '4':
      if(modeManuel)
      {
        pompeActive = true;
        digitalWrite(pompePin, LOW);
      }
      break;

    case '5':
      if(modeManuel)
      {
        pompeActive = false;
        digitalWrite(pompePin, HIGH);
      }
      break;
  }
}   // ← زيد هاد القوس هنا

  // ===== [FIX] Gestion beepOpen non-bloquant =====
  if (beepOpenActive && (now - beepOpenStart >= 2000)) {
    beepOpenActive = false;
    if (!co2AlarmActive) {
      digitalWrite(buzzerPin, HIGH);
    }
  }

  humidity    = HT.readHumidity();
  temperature = HT.readTemperature();

  int co2 = analogRead(CO2_PIN);

  ldrValue = analogRead(ldrPin);

  int soilRaw = analogRead(soilPin);

  int soilHumidity = map(soilRaw, 1023, 0, 0, 100);
  soilHumidity = constrain(soilHumidity, 0, 100);

  bool lampState = (ldrValue < seuilLDR);

  bool co2High = (co2 > CO2_THRESHOLD);

  // ===== LDR / Lamp =====
  // Relay inverse: LOW = ON
  if(!modeManuel)
{
  digitalWrite(lampPin, lampState ? LOW : HIGH);
}

  // ===== Pompe =====
  if(!modeManuel)
{
  if (soilHumidity < 30) {
    pompeActive = true;
  }
  else if (soilHumidity >= 60) {
    pompeActive = false;
  }
}
digitalWrite(pompePin, pompeActive ? LOW : HIGH);
  // ===== LEDs humidité =====
  if (humidity < 40) {

    digitalWrite(ledPin1, HIGH);
    digitalWrite(ledPin2, LOW);
    digitalWrite(ledPin3, LOW);

  }
  else if (humidity <= 80) {

    digitalWrite(ledPin1, LOW);
    digitalWrite(ledPin2, HIGH);
    digitalWrite(ledPin3, LOW);

  }
  else {

    digitalWrite(ledPin1, LOW);
    digitalWrite(ledPin2, LOW);
    digitalWrite(ledPin3, HIGH);

  }

  // ===== CO2 Alarm =====
  if (co2High) {

    co2AlarmActive = true;

    if (now - lastBeepTime >= (beepState ? BEEP_ON : BEEP_OFF)) {

      lastBeepTime = now;

      beepState = !beepState;

      digitalWrite(buzzerPin, beepState ? LOW : HIGH);
    }

  }
  else {

    if (co2AlarmActive) {

      co2AlarmActive = false;

      beepState = false;

      digitalWrite(buzzerPin, HIGH);
    }
  }

  // ===== Fan + Window =====
  if(!modeManuel)
{
  if (co2High || temperature > 32 || humidity < 40 || humidity > 80)
  {
    openWindow();
  }
  else
  {
    closeWindow();
  }
}

  // ===== Serial Monitor =====
  Serial.print("Temp: ");
  Serial.print(temperature);

  Serial.print(" | Hum: ");
  Serial.print(humidity);

  Serial.print(" | CO2: ");
  Serial.print(co2);

  Serial.print(" | LDR: ");
  Serial.print(ldrValue);

  Serial.print(" | Soil: ");
  Serial.print(soilHumidity);

  Serial.print("% | Pompe: ");
  Serial.println(pompeActive ? "ON" : "OFF");

  // ===== LCD 1 =====
  lcd1.setCursor(0, 0);

  lcd1.print("T:");
  lcd1.print(temperature, 1);

  lcd1.print((char)223);
  lcd1.print("C H:");

  lcd1.print(humidity, 0);
  lcd1.print("%  ");

  lcd1.setCursor(0, 1);

  lcd1.print("Lamp:");
  lcd1.print(lampState ? "ON " : "OFF");

  lcd1.print(" L:");
  lcd1.print(ldrValue);
  lcd1.print("  ");

  // ===== LCD 2 =====
  lcd2.setCursor(0, 0);

  if (co2High) {

    lcd2.print("CO2!:");
    lcd2.print(co2);
    lcd2.print("     ");

  }
  else {

    lcd2.print("CO2:");
    lcd2.print(co2);
    lcd2.print(" PPM  ");

  }

  lcd2.setCursor(0, 1);

  lcd2.print("Sol:");
  lcd2.print(soilHumidity);
  lcd2.print("%");

  lcd2.print(pompeActive ? " P:ON " : " P:OFF");
}