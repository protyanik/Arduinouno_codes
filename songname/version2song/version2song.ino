#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// --- OLED Settings ---
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// --- Pin Definitions ---         // pin alocations add or remove pins here
const int potPin = A0;
const int playPausePin = 3;
const int prevPin = 2;
const int nextPin = 4;
const int customPin = 5;


// --- Logic Variables ---
String songTitle = "Waiting...";
String artistName = "Connect Bridge";
String albumName = "";
int progress = 0;
bool newData = false;

int lastVol = -1;
const int volThreshold = 2;  // Ignore tiny electrical noise

bool lastPlayState = HIGH;
bool lastPrevState = HIGH;
bool lastNextState = HIGH;

void setup() {
  Serial.begin(9600);

  pinMode(playPausePin, INPUT_PULLUP);
  pinMode(prevPin, INPUT_PULLUP);
  pinMode(nextPin, INPUT_PULLUP);
  pinMode(customPin, INPUT_PULLUP);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD0136 allocation failed"));
    for (;;);  // Halt if OLED not found
  }

  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(10, 20);
  display.println("READY...");
  display.display();
}

void loop() {
  handleVolume();
  handleButtons();
  receiveSerial();

  if (newData) {
    updateDisplay();
    newData = false;
  }
}

// --- FEATURE: Volume with Smoothing & Inverted Logic ---
void handleVolume() {
  long sum = 0;
  for (int i = 0; i < 15; i++) {  // Increased averaging for extra stability
    sum += analogRead(potPin);
    delay(1);
  }
  int avgPot = sum / 15;

  // FIXED: 100 to 0 handles the "opposite" rotation issue
  int currentVol = map(avgPot, 0, 1023, 100, 0);

  if (abs(currentVol - lastVol) >= volThreshold) {
    Serial.println(currentVol);
    lastVol = currentVol;
  }
}

// --- FEATURE: Navigation Buttons ---
void handleButtons() {
  checkButton(playPausePin, lastPlayState, "<TOGGLE>");
  checkButton(prevPin, lastPrevState, "<PREV>");
  checkButton(nextPin, lastNextState, "<NEXT>");
  checkButton(customPin, lastNextState, "<CSTOM1>");
}

void checkButton(int pin, bool &lastState, String command) {
  bool currentState = digitalRead(pin);
  if (currentState == LOW && lastState == HIGH) {
    Serial.println(command);
    delay(200);  // Simple debounce
  }
  lastState = currentState;
}

// --- FEATURE: Serial Parser (<Title - Artist|Album|Progress>) ---
void receiveSerial() {
  static bool recvInProgress = false;
  static String rawData = "";

  while (Serial.available() > 0 && newData == false) {
    char rc = Serial.read();

    if (recvInProgress) {
      if (rc != '>') {
        rawData += rc;
      } else {
        int hyphen = rawData.indexOf('-');
        int firstPipe = rawData.indexOf('|');
        int secondPipe = rawData.lastIndexOf('|');

        if (firstPipe != -1 && secondPipe != -1) {
          songTitle = rawData.substring(0, hyphen);
          artistName = rawData.substring(hyphen + 1, firstPipe);
          albumName = rawData.substring(firstPipe + 1, secondPipe);
          progress = rawData.substring(secondPipe + 1).toInt();
          newData = true;
        }
        rawData = "";
        recvInProgress = false;
      }
    } else if (rc == '<') {
      rawData = "";
      recvInProgress = true;
    }
  }
}

// --- FEATURE: OLED UI Design ---
void updateDisplay() {
  display.clearDisplay();
  display.setTextColor(WHITE);

  // Song album name                        //album name update
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println(albumName);

  display.drawLine(0, 12, 128, 12, WHITE);

  // song name                              // Song name added
  display.setCursor(0, 20);
  display.println(songTitle);
// artist name                            // artist name added
  display.setCursor(0, 32);
  display.print("By:");
  display.println(artistName);

  // Progress Bar
  int barWidth = 110;
  int barHeight = 8;
  int barX = 9;
  int barY = 50;

  display.drawRect(barX, barY, barWidth, barHeight, WHITE);
  int fillWidth = map(progress, 0, 100, 0, barWidth - 4);
  display.fillRect(barX + 2, barY + 2, fillWidth, barHeight - 4, WHITE);

  display.display();
}

