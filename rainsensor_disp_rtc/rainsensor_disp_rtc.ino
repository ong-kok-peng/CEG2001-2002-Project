#include <AltSoftSerial.h>
#include <Wire.h>
#include "SSD1306Ascii.h"
#include "SSD1306AsciiAvrI2c.h" 
#include "virtuabotixRTC.h" 
#include <AM2302-Sensor.h>
#include <stdio.h>

//RTC
#define DS1302_CLK_PIN 2
#define DS1302_DAT_PIN 3
#define DS1302_RST_PIN 4
virtuabotixRTC RTC(DS1302_CLK_PIN, DS1302_DAT_PIN, DS1302_RST_PIN); 
unsigned long prevRTCMillis = 0;
unsigned int day, month, year, hours, minutes, seconds;

//OLED Display
#define I2C_ADDRESS 0x3C
SSD1306AsciiAvrI2c oled;
const long oledOnDuration = 10000;
unsigned long prevOledOnTime = 0;
bool oledIsOn = true;
const int oledWakeUpBtn = A3;

//DHT22
constexpr unsigned int SENSOR_PIN {A0};
AM2302::AM2302_Sensor dht22{SENSOR_PIN};
unsigned long prevDHT22Millis = 0;
const long DHT22ReadInterval = 3000;
float temperature; float humidity;

//Software serial
AltSoftSerial softSerial;
static const uint16_t LINE_MAX = 64;   // adjust to your longest line
char line[LINE_MAX];
unsigned long prevSoftSerialRxTime = 0;
const long softSerialRxTimeout = 5000;

void setDS1302WithCompileTime() {
  int Year, Month, Day, Hour, Minute, Second;
  char MonthStr[4]; // 3 chars for month name + null terminator

  // Parse __DATE__ (format e.g., "Nov 23 2025")
  sscanf(__DATE__, "%s %d %d", MonthStr, &Day, &Year);

  //Convert MonthStr to Month
  const char month_strs[] = "JanFebMarAprMayJunJulAugSepOctNovDec";
  for (Month = 0; Month < 12; Month++) {
    if (strncmp(MonthStr, month_strs + (Month * 3), 3) == 0) { break; }
  }
  Month += 1;

  // Parse __TIME__ (format e.g., "03:05:00")
  sscanf(__TIME__, "%d:%d:%d", &Hour, &Minute, &Second);

  RTC.setDS1302Time(Second, Minute, Hour, 0, Day, Month, Year); 
}


void getRTCDateTime() {
  unsigned long nowRTCMillis = millis();

  if (nowRTCMillis - prevRTCMillis >= 1000) {
    RTC.updateTime(); 
    year = RTC.year; month = RTC.month; day = RTC.dayofmonth;
    hours = RTC.hours; minutes = RTC.minutes; seconds = RTC.seconds;
    prevRTCMillis = nowRTCMillis;
  }
}

bool softSerialRx() {
  //receive serial line from external MCUs on soft serial port
  bool softSerialRxTimedOut = false;
  
  if (millis() - prevSoftSerialRxTime >= softSerialRxTimeout) { softSerialRxTimedOut = true; return softSerialRxTimedOut; } 
  
  if (softSerial.available()) {
    int n = softSerial.readBytesUntil('\n', line, LINE_MAX - 1);
    if (n > 0) {
      prevSoftSerialRxTime = millis();
      if (n && line[n - 1] == '\r') n--; // Trim optional '\r'
      line[n] = '\0';
    }
  }

  return softSerialRxTimedOut;
}

void readDHT22() {
  unsigned long nowDHT22Millis = millis();

  if (nowDHT22Millis - prevDHT22Millis >= DHT22ReadInterval) {
    auto dht22Status = dht22.read();
  
    if (dht22Status == 0) {
      temperature = dht22.get_Temperature();
      humidity = dht22.get_Humidity();
    }
    else { temperature = -1; humidity = -1; }

    prevDHT22Millis = nowDHT22Millis;
  }
}

void dispOled() {
  if (millis() - prevOledOnTime >= oledOnDuration && oledIsOn) { oled.ssd1306WriteCmd(SSD1306_DISPLAYOFF); oledIsOn = false; }

  if (digitalRead(A3) == LOW && !oledIsOn) {
    oledIsOn = true; oled.ssd1306WriteCmd(SSD1306_DISPLAYON); prevOledOnTime = millis();
  }
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  softSerial.begin(9600);
  softSerial.setTimeout(50);
  
  // Set sketch compiling time to RTC if RTC not set (ONE OFF OPERATION! COMMENT THIS LINE after setting the rtc time!)
  //setDS1302WithCompileTime();

  if (dht22.begin()) { delay(3000); } // this delay is needed to receive valid data
   else {
      while (true) {
        Serial.println(F("Error init dht22. => Please check sensor connection!"));
        delay(1000);
      }
   }
   
  Wire.begin();
  oled.begin(&Adafruit128x64, I2C_ADDRESS);
  oled.clear();
  oled.setFont(Adafruit5x7); // A common fixed-width font
  pinMode(oledWakeUpBtn, INPUT_PULLUP); // digital pin for waking up the oled
}

void loop() {
  // put your main code here, to run repeatedly:
  bool softSerialRxTimedOut = softSerialRx();
  getRTCDateTime();
  readDHT22();
  dispOled();

  oled.setCursor(0, 0);
  if (hours < 10) oled.print("0"); oled.print(hours); oled.print(":");
  if (minutes < 10) oled.print("0"); oled.print(minutes); oled.print(":");
  if (seconds < 10) oled.print("0"); oled.print(seconds);
  oled.print("  ");
  oled.print(day); oled.print("/"); oled.print(month); oled.print("/"); oled.print(year);

  oled.setCursor(0,1); oled.print("---------------------");

  oled.setCursor(0,2); 
  if (!softSerialRxTimedOut) {
     oled.println(line);
  }
  else {
    oled.println("No RX received.");
  }

  oled.setCursor(0,4); 
  oled.print(temperature); oled.print(" C; "); oled.print(humidity); oled.print(" %RH");
  
}
