#include <AltSoftSerial.h>

AltSoftSerial softSerial;
static const uint16_t LINE_MAX = 64;   // adjust to your longest line
char line[LINE_MAX];

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  softSerial.begin(9600);
  softSerial.setTimeout(50);
}

void loop() {
  // put your main code here, to run repeatedly:
  if (softSerial.available()) {
    int n = softSerial.readBytesUntil('\n', line, LINE_MAX - 1);
    if (n > 0) {
      // Trim optional '\r'
      if (n && line[n - 1] == '\r') n--;
      line[n] = '\0';
      Serial.println(line);  // safe: prints from static buffer
    }
  }
  
}
