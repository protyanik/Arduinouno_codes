#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

String songInfo = "";
String albumInfo = "";
int progress = 0;
bool newData = false;

void setup() {
  Serial.begin(9600);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
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
  static String rawData = "";
  char rc;

  while (Serial.available() > 0 && newData == false) {
    rc = Serial.read();
    if (recvInProgress) {
      if (rc != '>') {
        rawData += rc;
      } else {
        // Find the two pipes
        int firstPipe = rawData.indexOf('|');
        int secondPipe = rawData.lastIndexOf('|');

        if (firstPipe != -1 && secondPipe != -1) {
          songInfo = rawData.substring(0, firstPipe);
          albumInfo = rawData.substring(firstPipe + 1, secondPipe);
          progress = rawData.substring(secondPipe + 1).toInt();
        }
        rawData = "";
        recvInProgress = false;
        newData = true;
      }
    } else if (rc == '<') {
      rawData = "";
      recvInProgress = true;
    }
  }
}

void updateDisplay() {
  display.clearDisplay();
  
  // 1. TOP: Album Name (Small, Dimmer look)
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  // Optional: Center the album name
  display.println(albumInfo);
  display.drawLine(0, 10, 128, 10, WHITE); // Decorative line

  // 2. MIDDLE: Song & Artist
  display.setCursor(0, 18);
  display.setTextSize(1); // Keep at 1 if names are long, or 2 for short names
  display.println(songInfo);

  // 3. BOTTOM: Progress Bar
  int barWidth = 110;
  int barHeight = 6;
  int barX = (128 - barWidth) / 2;
  int barY = 54;
  
  display.drawRect(barX, barY, barWidth, barHeight, WHITE);
  int fillWidth = map(progress, 0, 100, 0, barWidth - 4);
  display.fillRect(barX + 2, barY + 2, fillWidth, barHeight - 4, WHITE);

  display.display();
}