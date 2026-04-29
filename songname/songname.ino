#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// --- Variables for Media Info ---
String songInfo = "Waiting...";
String artistName = "Spotify";
String albumInfo = "";
int progress = 0;
bool newData = false;

// --- Variables for Controls ---
const int buttonPin = 3;
const int potPin = A0;
bool lastButtonState = HIGH;
int lastVol = -1;
const int volThreshold = 2; // Prevents "jitter"

void setup() {
  Serial.begin(115200);
  pinMode(buttonPin, INPUT_PULLUP); 
  
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    for(;;); // Don't proceed if OLED fails
  }
  
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(20, 25);
  display.println("Bridge Connecting...");
  display.display();
}

void loop() {
  // 1. Handle Volume Knob (Potentiometer)
  readVolume();

  // 2. Handle Play/Pause (Button)
  readButton();

  // 3. Listen for Song Info from PC
  receiveSerial();

  // 4. Update Screen if new data arrived
  if (newData) {
    updateDisplay();
    newData = false; 
  }
}

// --- HELPER FUNCTIONS ---

void readVolume() {
  long sum = 0;
  for(int i = 0; i < 10; i++) { sum += analogRead(potPin); delay(1); }
  int avgPot = sum / 10;
  int currentVol = map(avgPot, 0, 1023, 100, 0);

  if (abs(currentVol - lastVol) >= volThreshold) {
    Serial.println(currentVol); // Send volume to Python
    lastVol = currentVol;
  }
}

void readButton() {
  bool currentButtonState = digitalRead(buttonPin);
  // Detect "Falling Edge" (Button just got pressed)
  if (currentButtonState == LOW && lastButtonState == HIGH) {
    Serial.println("<TOGGLE>"); // Send command to Python
    delay(200); // Simple debounce
  }
  lastButtonState = currentButtonState;
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
        // Parsing logic: <Song - Artist|Album|Progress>
        int hyphen = rawData.indexOf('-');
        int firstPipe = rawData.indexOf('|');
        int secondPipe = rawData.lastIndexOf('|');

        if (firstPipe != -1 && secondPipe != -1) {
          songInfo = rawData.substring(0, hyphen);
          artistName = rawData.substring(hyphen + 2, firstPipe);
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
  
  // Header
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("Now Playing");
  display.drawLine(0, 10, 128, 10, WHITE);

  // Song Title
  display.setCursor(0, 18);
  display.setTextSize(1);
  display.println(songInfo);

  // Artist
  display.setCursor(0, 32);
  display.print("By: ");
  display.println(artistName);

  // Progress Bar
  int barWidth = 110;
  int barHeight = 6;
  int barX = (128 - barWidth) / 2;
  int barY = 54;
  
  display.drawRect(barX, barY, barWidth, barHeight, WHITE);
  int fillWidth = map(progress, 0, 100, 0, barWidth - 4);
  display.fillRect(barX + 2, barY + 2, fillWidth, barHeight - 4, WHITE);

  display.display();
}