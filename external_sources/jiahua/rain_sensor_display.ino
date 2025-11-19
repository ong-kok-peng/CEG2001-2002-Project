// OLED will now auto-cycle through 5 screens every 3 seconds:
// 1.Main Screen
// 2.Moisture Log Graph
// 3.Rain Prediction
// 4.Rain Duration Tracker
// 5.Weather Icon

// Arduino wire connections
// GND -> GND
// VCC -> 5V
// SCL -> A5
// SDA -> A4

// To run the program, make sure to copy and paste the unix timestamp from https://www.unixtimestamp.com/ in the serial output
// to get the current time

// The OLED might not displaying accurate information right now
// you guys can help me modify the code so that it can link to our physical prototype

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// OLED Display
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Moisture sensor
const int sensorPin = A0;

// Time variables (PC synced)
unsigned long syncedUnixTime = 0;
unsigned long syncMillis = 0;
int hours, minutes, seconds;

// Auto-calibration
int dryBaseline = 0;

// Moisture logs (20 samples)
int logs[20];
int logIndex = 0;

// Screen cycling
int screenID = 0;
unsigned long lastScreenSwitch = 0;

// Rain duration
bool wasRaining = false;
unsigned long rainStart = 0;
unsigned long rainDuration = 0;

void setup() {
  Serial.begin(9600);
  Wire.begin();
  delay(200);

  // Initialize OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED ALLOCATION FAILED");
    while (1);
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(0,0);
  display.println("Waiting for PC time...");
  display.display();

  Serial.println("Send Unix timestamp:");

  // Initialize logs
  for (int i = 0; i < 20; i++) logs[i] = 0;
}

void loop() {

  // ===== 1. RECEIVE PC TIME =====
  if (Serial.available()) {
    unsigned long ts = Serial.parseInt();
    if (ts > 100000) {
      syncedUnixTime = ts + (8 * 3600);  // GMT+8
      syncMillis = millis();
      Serial.println("Time synced!");
    }
  }

  // ===== 2. UPDATE CLOCK =====
  if (syncedUnixTime > 0) {
    unsigned long elapsed = (millis() - syncMillis) / 1000;
    unsigned long now = syncedUnixTime + elapsed;

    hours = (now % 86400L) / 3600;
    minutes = (now % 3600) / 60;
    seconds = now % 60;
  }

  // ===== 3. READ MOISTURE SENSOR =====
  int rawValue = analogRead(sensorPin);

  // Dynamic baseline calibration
  if (rawValue > dryBaseline) dryBaseline = rawValue;

  int moisturePercent = map(rawValue, dryBaseline, 0, 0, 100);
  moisturePercent = constrain(moisturePercent, 0, 100);

  // Store in logs
  logs[logIndex] = moisturePercent;
  logIndex = (logIndex + 1) % 20;

  // Rain status
  String rainStatus;
  if (moisturePercent < 30) rainStatus = "No rain";
  else if (moisturePercent < 70) rainStatus = "Light rain";
  else rainStatus = "Heavy rain";

  // Rain duration timer
  if (rainStatus != "No rain") {
    if (!wasRaining) {
      wasRaining = true;
      rainStart = millis();
    }
    rainDuration = millis() - rainStart;
  } else {
    wasRaining = false;
  }

  // Rain prediction based on trend
  int trend = logs[logIndex] - logs[(logIndex + 19) % 20];
  String prediction;

  if (trend > 8) prediction = "Likely Rain Soon";
  else if (trend < -8) prediction = "Drying Up";
  else prediction = "Stable";

  // ===== 4. AUTO SCREEN CYCLING =====
  if (millis() - lastScreenSwitch > 3000) {
    screenID = (screenID + 1) % 5;
    lastScreenSwitch = millis();
  }

  // ===== 5. RENDER SCREENS =====
  display.clearDisplay();
  display.setCursor(0, 0);

  // ------- SCREEN 0: MAIN SCREEN -------
  if (screenID == 0) {
    display.print("Time: ");
    if (hours < 10) display.print("0");
    display.print(hours); display.print(":");
    if (minutes < 10) display.print("0");
    display.print(minutes); display.print(":");
    if (seconds < 10) display.print("0");
    display.print(seconds);

    display.setCursor(0, 15);
    display.drawLine(0, 14, 127, 14, SSD1306_WHITE);

    display.setCursor(0, 25);
    display.print("Moisture: ");
    display.print(moisturePercent);
    display.println("%");

    display.setCursor(0, 45);
    display.print("Status: ");
    display.print(rainStatus);
  }

  // ------- SCREEN 1: MOISTURE GRAPH -------
  else if (screenID == 1) {
    display.println("Moisture Log:");
    display.drawLine(0, 14, 127, 14, SSD1306_WHITE);

    for (int i = 0; i < 20; i++) {
      int x = i * 6;
      int h = map(logs[i], 0, 100, 0, 40);
      display.drawRect(x, 63 - h, 5, h, SSD1306_WHITE);
    }
  }

  // ------- SCREEN 2: RAIN PREDICTION -------
  else if (screenID == 2) {
    display.println("Rain Prediction:");
    display.drawLine(0, 14, 127, 14, SSD1306_WHITE);

    display.setCursor(0, 25);
    if (prediction.length() == 0) display.print("Calculating...");
    else display.print(prediction);
  }

  // ------- SCREEN 3: RAIN DURATION -------
  else if (screenID == 3) {
    display.println("Rain Duration:");
    display.drawLine(0, 14, 127, 14, SSD1306_WHITE);

    display.setCursor(0, 25);
    if (rainDuration < 1000) {
      display.print("Not raining now");
    } else {
      unsigned long sec = rainDuration / 1000;
      display.print(sec);
      display.print(" sec raining");
    }
  }

  // ------- SCREEN 4: WEATHER ICON -------
  else if (screenID == 4) {

    bool blink = (millis() / 500) % 2;

    display.println("Weather:");

    // No rain → Sun
    if (rainStatus == "No rain") {
      if (blink) display.drawCircle(64, 32, 15, SSD1306_WHITE);
    }
    // Light rain → Cloud + 1 drop
    else if (rainStatus == "Light rain") {
      display.drawCircle(64, 20, 15, SSD1306_WHITE);
      if (blink) display.drawLine(60, 40, 64, 60, SSD1306_WHITE);
    }
    // Heavy rain → Cloud + 3 drops
    else {
      display.drawCircle(64, 20, 15, SSD1306_WHITE);
      if (blink) {
        display.drawLine(50, 40, 55, 60, SSD1306_WHITE);
        display.drawLine(64, 40, 69, 60, SSD1306_WHITE);
        display.drawLine(78, 40, 83, 60, SSD1306_WHITE);
      }
    }
  }

  display.display();
}
