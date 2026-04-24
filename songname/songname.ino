#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

String currentSong = "";
bool newData = false;

void setup() {
  Serial.begin(9600);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C); // Use the address that worked (0x3C or 0x3D)
  
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 20);
  display.println("READY & WAITING...");
  display.println("Start Python now!");
  display.display();
}

void loop() {
  receiveSerial();
  if (newData) {
    updateDisplay();
    newData = false;
  }
}

void receiveSerial() {
  static bool recvInProgress = false;
  char rc;
  while (Serial.available() > 0 && newData == false) {
    rc = Serial.read();
    if (recvInProgress) {
      if (rc != '>') {
        currentSong += rc;
      } else {
        recvInProgress = false;
        newData = true;
      }
    } else if (rc == '<') {
      currentSong = "";
      recvInProgress = true;
    }
  }
}

void updateDisplay() {
  display.stopscroll();
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("NOW PLAYING:");
  display.drawLine(0, 10, 128, 10, WHITE);

  display.setTextSize(1);
  display.setCursor(0, 25);
  display.println(currentSong);
  display.display();
  display.startscrollleft(0x03, 0x07); 
}