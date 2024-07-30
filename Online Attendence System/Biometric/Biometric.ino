#include <Adafruit_Fingerprint.h>
#include <EEPROM.h>
#include <LiquidCrystal.h>
#include <SoftwareSerial.h>
#include <Wire.h>
#include "RTClib.h"

// Initialize LCD
LiquidCrystal lcd(8, 9, 10, 11, 12, 13);

// Initialize Fingerprint Sensor
SoftwareSerial fingerPrint(2, 3);
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&fingerPrint);

// Initialize RTC
RTC_DS3231 rtc;

// Pin Definitions
#define register_back 14
#define delete_ok 15
#define forward 16
#define reverse 17
#define match 18
#define buzzer 5

// Number of Records
#define records 10

// Attendance Data Storage
int user_count[records] = {0};

DateTime now;

void setup() {
  // Initialize LCD
  lcd.begin(16, 2);

  // Initialize Serial
  Serial.begin(9600);

  // Initialize Pins
  pinMode(register_back, INPUT_PULLUP);
  pinMode(forward, INPUT_PULLUP);
  pinMode(reverse, INPUT_PULLUP);
  pinMode(delete_ok, INPUT_PULLUP);
  pinMode(match, INPUT_PULLUP);
  pinMode(buzzer, OUTPUT);
  digitalWrite(buzzer, LOW);

  // Initialize Fingerprint Sensor
  finger.begin(57600);
  lcd.print("Finding Module..");
  if (finger.verifyPassword()) {
    lcd.clear();
    lcd.print("Module Found");
    delay(2000);
  } else {
    lcd.clear();
    lcd.print("Module Not Found");
    while (1);
  }

  // Initialize RTC
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  if (rtc.lostPower()) {
    //rtc.adjust(DateTime(2024, 7, 9, 10, 0, 0));
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  // Initial LCD Message
  lcd.clear();
  lcd.print("Fingerprint");
  lcd.setCursor(0, 1);
  lcd.print("Attendance System");
  delay(2000);
}

void loop() {
  now = rtc.now();
  lcd.setCursor(0, 0);
  lcd.print("Time: ");
  lcd.print(now.hour(), DEC);
  lcd.print(':');
  lcd.print(now.minute(), DEC);
  lcd.print(':');
  lcd.print(now.second(), DEC);
  lcd.setCursor(0, 1);
  lcd.print("Date: ");
  lcd.print(now.day(), DEC);
  lcd.print('/');
  lcd.print(now.month(), DEC);
  lcd.print('/');
  lcd.print(now.year(), DEC);

  int result = getFingerprintIDez();
  if (result > 0) {
    digitalWrite(buzzer, HIGH);
    delay(100);
    digitalWrite(buzzer, LOW);
    lcd.clear();
    lcd.print("ID:");
    lcd.print(result);
    lcd.setCursor(0, 1);
    lcd.print("Please Wait....");
    delay(1000);
    recordAttendance(result);
    lcd.clear();
    lcd.print("Attendance");
    lcd.setCursor(0, 1);
    lcd.print("Registered");
    delay(2000);
  }

  if (digitalRead(register_back) == 0) {
    lcd.clear();
    lcd.print("Please Wait");
    delay(1000);
    while (digitalRead(register_back) == 0);
    enrollFingerprint();
  }

  delay(300);
}

void recordAttendance(int id) {
  int eepIndex = (user_count[id - 1] * 7) + ((id - 1) * 210);
  EEPROM.write(eepIndex++, now.hour());
  EEPROM.write(eepIndex++, now.minute());
  EEPROM.write(eepIndex++, now.second());
  EEPROM.write(eepIndex++, now.day());
  EEPROM.write(eepIndex++, now.month());
  EEPROM.write(eepIndex++, now.year() >> 8);
  EEPROM.write(eepIndex++, now.year());
  user_count[id - 1]++;
}

void enrollFingerprint() {
  int id = 1;
  lcd.clear();
  lcd.print("Enter Finger ID:");

  while (1) {
    lcd.setCursor(0, 1);
    lcd.print(id);

    if (digitalRead(forward) == 0) {
      id++;
      if (id > records) id = 1;
      delay(500);
    } else if (digitalRead(reverse) == 0) {
      id--;
      if (id < 1) id = records;
      delay(500);
    } else if (digitalRead(delete_ok) == 0) {
      getFingerprintEnroll(id);
      return;
    } else if (digitalRead(register_back) == 0) {
      return;
    }
  }
}

uint8_t getFingerprintEnroll(int id) {
  int p = -1;
  lcd.clear();
  lcd.print("finger ID:");
  lcd.print(id);
  lcd.setCursor(0, 1);
  lcd.print("Place Finger");
  delay(2000);

  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
      case FINGERPRINT_OK:
        lcd.clear();
        lcd.print("Image taken");
        break;
      case FINGERPRINT_NOFINGER:
        lcd.clear();
        lcd.print("No Finger Found");
        break;
      case FINGERPRINT_PACKETRECIEVEERR:
        lcd.clear();
        lcd.print("Comm Error");
        break;
      case FINGERPRINT_IMAGEFAIL:
        lcd.clear();
        lcd.print("Imaging Error");
        break;
      default:
        lcd.clear();
        lcd.print("Unknown Error");
        break;
    }
  }

  p = finger.image2Tz(1);
  switch (p) {
    case FINGERPRINT_OK:
      lcd.clear();
      lcd.print("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      lcd.clear();
      lcd.print("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      lcd.clear();
      lcd.print("Comm Error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
    case FINGERPRINT_INVALIDIMAGE:
      lcd.clear();
      lcd.print("Feature Not Found");
      return p;
    default:
      lcd.clear();
      lcd.print("Unknown Error");
      return p;
  }

  lcd.clear();
  lcd.print("Remove Finger");
  delay(2000);
  p = 0;
  while (p != FINGERPRINT_NOFINGER) {
    p = finger.getImage();
  }

  lcd.clear();
  lcd.print("Place Finger Again");
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
      case FINGERPRINT_OK:
        break;
      case FINGERPRINT_NOFINGER:
        break;
      case FINGERPRINT_PACKETRECIEVEERR:
        break;
      case FINGERPRINT_IMAGEFAIL:
        break;
      default:
        return p;
    }
  }

  p = finger.image2Tz(2);
  switch (p) {
    case FINGERPRINT_OK:
      break;
    case FINGERPRINT_IMAGEMESS:
    case FINGERPRINT_PACKETRECIEVEERR:
    case FINGERPRINT_FEATUREFAIL:
    case FINGERPRINT_INVALIDIMAGE:
      return p;
    default:
      return p;
  }

  p = finger.createModel();
  if (p != FINGERPRINT_OK) return p;

  p = finger.storeModel(id);
  if (p == FINGERPRINT_OK) {
    lcd.clear();
    lcd.print("Finger Stored!");
    delay(2000);
  }
  return p;
}

int getFingerprintIDez() {
  uint8_t p = finger.getImage();
  if (p != FINGERPRINT_OK) return -1;
  p = finger.image2Tz();
  if (p != FINGERPRINT_OK) return -1;
  p = finger.fingerFastSearch();
  if (p != FINGERPRINT_OK) {
    lcd.clear();
    lcd.print("Finger Not Found");
    lcd.setCursor(0, 1);
    lcd.print("Try Later");
    delay(2000);
    return -1;
  }
  return finger.fingerID;
}
