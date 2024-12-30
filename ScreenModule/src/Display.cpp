#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <avr/wdt.h>
#include <avr/pgmspace.h>

// --------------------------------------------------
// 1) Display Definitions
// --------------------------------------------------
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32

#define OLED_MOSI 11
#define OLED_CLK  13
#define OLED_DC   6
#define OLED_CS   8
#define OLED_RESET 9

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, 
                         OLED_MOSI, OLED_CLK, OLED_DC, 
                         OLED_RESET, OLED_CS);

// --------------------------------------------------
// 2) Pins, Arrays, and Global Variables
// --------------------------------------------------
#define CLK 4
#define DT 3
#define SW 2
#define LED_PIN 7

const int maxDevices = 10;
String deviceNames[maxDevices];
String deviceMacs[maxDevices];
int deviceCount = 0;

// Encoder and button states
int lastStateCLK, currentStateCLK, lastStateDT, currentStateDT, lastButtonState;

// Timing variables
unsigned long buttonPressStartTime = 0;
unsigned long lastButtonPressTime  = 0;
unsigned long lastSongUpdateTime   = 0;
unsigned long lastRapidClickTime   = 0;

// Flags and states
bool longPressHandled    = false;
bool isPlaying           = true;
bool isBTConnected       = true;
bool inMenu              = false;
bool displayOn           = true;
bool callIncoming        = false;

// Menu and system variables
String songName = "";
int songPixelLength   = 0;
int currentPosition   = 0;
int volume            = 50;
int menuIndex         = 0;
int subMenuIndex      = 0;
int brightnessLevel   = 50;
int contrastLevel     = 50;
int encoderAccum      = 0;
int rapidClickCount   = 0;
String deviceName     = "NONE";

// Animation flags and timing
bool fastForwardAnimationActive = false;
bool rewindAnimationActive      = false;
unsigned long animationStartTime=0;
const unsigned long animationDuration = 500; // Duration in ms

// --------------------------------------------------
// 3) Menu State Enumeration
// --------------------------------------------------
enum MenuState {
  MAIN_MENU,
  TUNER_MENU,
  DISPLAY_MENU,
  DISPLAY_BRIGHTNESS,
  DISPLAY_CONTRAST,
  SYSTEM_MENU,
  DEVICE_NAME_MENU,
  CALL_MENU,
  PAIRING_MENU
};
MenuState currentMenuState = MAIN_MENU;

// --------------------------------------------------
// 4) Store Menu Items in PROGMEM
// --------------------------------------------------
// Main Menu
const char mainMenu0[] PROGMEM = "Tuner";
const char mainMenu1[] PROGMEM = "Display";
const char mainMenu2[] PROGMEM = "System";
const char* const mainMenuItems[] PROGMEM = { mainMenu0, mainMenu1, mainMenu2 };
const int mainMenuCount = 3;

// Tuner
const char tuner0[] PROGMEM = "Bass";
const char tuner1[] PROGMEM = "Treble";
const char tuner2[] PROGMEM = "Loudness";
const char* const tunerItems[] PROGMEM = { tuner0, tuner1, tuner2 };
const int tunerCount = 3;

// Display
const char display0[] PROGMEM = "Brightness";
const char* const displayItems[] PROGMEM = { display0 };
const int displayCount = 1;

// System
const char system0[] PROGMEM = "Restart Service";
const char system1[] PROGMEM = "Device Name";
const char system2[] PROGMEM = "Pairing";
const char* const systemItems[] PROGMEM = { system0, system1, system2 };
const int systemCount = 3;

// Call
const char call0[] PROGMEM = "Accept";
const char call1[] PROGMEM = "Reject";
const char* const callMenuItems[] PROGMEM = { call0, call1 };
const int callMenuCount = 2;

// Pairing
const char pair0[] PROGMEM = "Device 1";
const char pair1[] PROGMEM = "Device 2";
const char pair2[] PROGMEM = "Device 3";
const char pair3[] PROGMEM = "Device 4";
const char pair4[] PROGMEM = "Device 5";
const char* const pairingMenuItems[] PROGMEM = {
  pair0, pair1, pair2, pair3, pair4
};
const int pairingMenuCount = 5;

// Variables for delayed single-click handling
bool singleClickPending   = false;
unsigned long singleClickTime = 0;

// Constants for thresholds
const unsigned long doubleClickThreshold = 300;
const unsigned long longPressThreshold   = 2000;
const unsigned long rapidClickInterval   = 500;

// --------------------------------------------------
// Forward Declarations
// --------------------------------------------------
void applyDisplaySettings();
void calculateSongPixelLength();
void bootupAnimation();
void boldText(String text);
void softwareReset();
void handleSerialInput(String input);
void navigateMenu(int d);
void handleMenuClick();
void handleDoubleClick();
void displayMenu();
void displayUI();
void controlLED(unsigned long currentTime);
void toggleDisplay();
void handleInputs(unsigned long currentTime);
void populateDeviceList(const String& mac, const String& name);
void drawValueMenu(String title, int value);
void drawValueMenu_S(String title, String textValue);



// --------------------------------------------------
// 5) Setup
// --------------------------------------------------
void setup() {
  delay(100);
  Serial.begin(9600);

  // Initialize encoder/button pins
  pinMode(CLK, INPUT);
  pinMode(DT,  INPUT);
  pinMode(SW,  INPUT_PULLUP);

  lastStateCLK    = digitalRead(CLK);
  lastStateDT     = digitalRead(DT);
  lastButtonState = digitalRead(SW);

  // Initialize LED
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);

  // Initialize display
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)){
    Serial.println(F("SSD1306 allocation failed"));
    while(true);
  }
  calculateSongPixelLength();
  bootupAnimation();
  applyDisplaySettings();
}

// --------------------------------------------------
// 6) Main Loop
// --------------------------------------------------
void loop() {
  unsigned long currentTime = millis();
  handleInputs(currentTime);

  // Check Serial
  if(Serial.available()) {
    String incoming = Serial.readStringUntil('\n');
    handleSerialInput(incoming);
  }

  // Handle single-click if pending
  if (singleClickPending && (currentTime - singleClickTime > doubleClickThreshold)) {
    singleClickPending = false;
    // Process single click in certain states
    if (inMenu && currentMenuState == MAIN_MENU) {
      handleMenuClick();
    }
  }

  // Draw UI
  displayUI();

  // Blink LED if SW is held
  controlLED(currentTime);
}

// --------------------------------------------------
// 7) Implementation of Functions
// --------------------------------------------------
void populateDeviceList(const String& mac, const String& name) {
  if (deviceCount < maxDevices) {
    deviceMacs[deviceCount] = mac;
    deviceNames[deviceCount] = name;
    deviceCount++;
  }
}

// --------------------------------------------------
void applyDisplaySettings() {
  uint8_t contrastValue = map(brightnessLevel, 0, 100, 0, 255);
  display.ssd1306_command(0x81);
  display.ssd1306_command(contrastValue);
}

// --------------------------------------------------
void calculateSongPixelLength() {
  songPixelLength = songName.length() * 6;
}

// --------------------------------------------------
void bootupAnimation() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  for(int x=0; x<SCREEN_WIDTH; x+=4) {
    display.clearDisplay();
    display.drawLine(x, 0, x, SCREEN_HEIGHT-1, SSD1306_WHITE);
    display.display();
    delay(10);
  }
  for(int y=0; y<SCREEN_HEIGHT; y+=4) {
    display.clearDisplay();
    display.drawLine(0, y, SCREEN_WIDTH-1, y, SSD1306_WHITE);
    display.display();
    delay(10);
  }
}

// --------------------------------------------------
void boldText(String text) {
  int x = display.getCursorX();
  int y = display.getCursorY();
  display.setCursor(x, y);
  display.print(text);
  display.setCursor(x+1, y);
  display.print(text);
  display.setCursor(x, y);
}

// --------------------------------------------------
void softwareReset() {
  wdt_enable(WDTO_15MS);
  while(1) {}
}

// --------------------------------------------------
void removeDevice(int deviceIndex) {
  if (deviceIndex < 0 || deviceIndex >= deviceCount) return;
  // Shift devices down
  for (int i = deviceIndex; i < deviceCount - 1; i++) {
    deviceNames[i] = deviceNames[i + 1];
    deviceMacs[i]  = deviceMacs[i + 1];
  }
  deviceCount--;
  Serial.println(F("Device removed."));
}

// --------------------------------------------------
void displayDeviceDetails(int deviceIndex) {
  if (deviceIndex < 0 || deviceIndex >= deviceCount) return;

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(0, 0);
  display.print(F("Name: "));
  display.print(deviceNames[deviceIndex]);

  display.setCursor(0, 10);
  display.print(F("MAC: "));
  display.print(deviceMacs[deviceIndex]);

  display.setCursor(0, 20);
  display.print(F("[Remove]"));

  display.display();

  while (true) {
    int buttonState = digitalRead(SW);
    if (buttonState == LOW && lastButtonState == HIGH) {
      removeDevice(deviceIndex);
      inMenu = false;
      currentMenuState = MAIN_MENU;
      break;
    }
    lastButtonState = buttonState;
  }
}

// --------------------------------------------------
void displayPairingMenu(String passkey) {
  inMenu = true;
  currentMenuState = CALL_MENU;
  subMenuIndex = 0;

  while (inMenu) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);

    display.setCursor(0, 0);
    display.print(F("Passcode: "));
    display.print(passkey);

    for (int i = 0; i < callMenuCount; i++) {
      display.setCursor(0, 10 + (i * 10));
      if (i == subMenuIndex) {
        display.fillRect(0, 10 + (i * 10), 128, 10, SSD1306_WHITE);
        display.setTextColor(SSD1306_BLACK);
      } else {
        display.setTextColor(SSD1306_WHITE);
      }

      // Read string from PROGMEM
      char buffer[16];
      strcpy_P(buffer, (char*)pgm_read_word(&(callMenuItems[i])));
      display.print(buffer);
    }
    display.display();

    int currentStateCLK = digitalRead(CLK);
    if (currentStateCLK != lastStateCLK) {
      if (digitalRead(DT) != currentStateCLK) {
        subMenuIndex = (subMenuIndex + 1) % callMenuCount;
      } else {
        subMenuIndex = (subMenuIndex - 1 + callMenuCount) % callMenuCount;
      }
      lastStateCLK = currentStateCLK;
    }

    int buttonState = digitalRead(SW);
    if (buttonState == LOW && lastButtonState == HIGH) {
      if (subMenuIndex == 0) {
        Serial.println(F("PAIRING:ACCEPT"));
      } else {
        Serial.println(F("PAIRING:DECLINE"));
      }
      inMenu = false;
    }
    lastButtonState = buttonState;
  }
}

// --------------------------------------------------
void populateDeviceListFromSerial(const String& data) {
  deviceCount = 0;
  int startIdx = 0;
  while (startIdx < (int)data.length()) {
    int separatorIdx  = data.indexOf('|', startIdx);
    int nextDeviceIdx = data.indexOf(',', startIdx);

    if (separatorIdx == -1 || 
        (nextDeviceIdx != -1 && separatorIdx > nextDeviceIdx)) break;

    String mac  = data.substring(startIdx, separatorIdx);
    String name = (nextDeviceIdx == -1)
                  ? data.substring(separatorIdx + 1)
                  : data.substring(separatorIdx + 1, nextDeviceIdx);

    populateDeviceList(mac, name);

    startIdx = (nextDeviceIdx == -1) ? data.length() : nextDeviceIdx + 1;
  }
}

// --------------------------------------------------
void handleSerialInput(String input) {
  input.trim();

  if (input.startsWith(F("DEVICES:"))) {
    String deviceData = input.substring(8);
    populateDeviceListFromSerial(deviceData);
  }
  else if (input.startsWith(F("SONG:"))) {
    if (input.indexOf(F("[INCOMING]")) != -1) {
      callIncoming = true;
    }
    // Serial.print(F("Free memory: "));
    // Serial.println(freeMemory());

    songName = input.substring(5);
    Serial.println(songName);
    calculateSongPixelLength();
  }
  else if (input.startsWith(F("POS:"))) {
    currentPosition = input.substring(4).toInt();
  }
  // else if (input.startsWith(F("VOL:"))) {
  //   volume = input.substring(4).toInt();
  // }
  else if (input.startsWith(F("STATE:"))) {
    isPlaying = (input.substring(6) == F("PLAY"));
  }
  else if (input.startsWith(F("BT:"))) {
    isBTConnected = (input.substring(3) == F("ON"));
  }
  else if (input.equalsIgnoreCase(F("RESET"))) {
    Serial.println(F("System is resetting..."));
    softwareReset();
  }
}

// --------------------------------------------------
void navigateMenu(int d) {
  if(!inMenu) return;

  if (currentMenuState == CALL_MENU) {
    subMenuIndex += d;
    if(subMenuIndex < 0) subMenuIndex = callMenuCount - 1;
    if(subMenuIndex >= callMenuCount) subMenuIndex = 0;
    return;
  }

  if(currentMenuState == MAIN_MENU) {
    menuIndex += d;
    if(menuIndex < 0) menuIndex = mainMenuCount - 1;
    if(menuIndex >= mainMenuCount) menuIndex = 0;
  }
  else if(currentMenuState == TUNER_MENU) {
    subMenuIndex += d;
    if(subMenuIndex < 0) subMenuIndex = tunerCount - 1;
    if(subMenuIndex >= tunerCount) subMenuIndex = 0;
  }
  else if(currentMenuState == DISPLAY_MENU) {
    subMenuIndex += d;
    if(subMenuIndex < 0) subMenuIndex = displayCount-1;
    if(subMenuIndex >= displayCount) subMenuIndex = 0;
  }
  else if (currentMenuState == PAIRING_MENU) {
    subMenuIndex += d;
    if (subMenuIndex < 0) subMenuIndex = pairingMenuCount - 1;
    if (subMenuIndex >= pairingMenuCount) subMenuIndex = 0;
    return;
  }
  else if (currentMenuState == SYSTEM_MENU) {
    subMenuIndex += d;
    if (subMenuIndex < 0) subMenuIndex = systemCount - 1;
    if (subMenuIndex >= systemCount) subMenuIndex = 0;
  }
  else if(currentMenuState == DISPLAY_BRIGHTNESS) {
    brightnessLevel += d;
    brightnessLevel = max(0, min(100, brightnessLevel));
    applyDisplaySettings();
  }
}

// --------------------------------------------------
void handleMenuClick() {
  if(!inMenu) return;

  if(currentMenuState == CALL_MENU) {
    if(subMenuIndex == 0) {
      Serial.println(F("ACCEPT_CALL"));
    } else {
      Serial.println(F("REJECT_CALL"));
    }
    callIncoming = false;
    inMenu       = false;
    currentMenuState = MAIN_MENU;
    return;
  }

  if(currentMenuState == MAIN_MENU) {
    if(menuIndex == 0) {
      currentMenuState = TUNER_MENU;
      subMenuIndex = 0;
    } else if(menuIndex == 1) {
      currentMenuState = DISPLAY_MENU;
      subMenuIndex = 0;
    } else if(menuIndex == 2) {
      currentMenuState = SYSTEM_MENU;
      subMenuIndex = 0;
    }
  }
  else if(currentMenuState == TUNER_MENU) {
    // Tuner submenu logic
  }
  else if(currentMenuState == DISPLAY_MENU) {
    if(subMenuIndex == 0) {
      currentMenuState = DISPLAY_BRIGHTNESS;
    }
  }

  if (currentMenuState == SYSTEM_MENU) {
    if (subMenuIndex == 0) {
      Serial.println(F("RESTART_SERVICE"));
    } else if (subMenuIndex == 1) {
      currentMenuState = DEVICE_NAME_MENU;
      subMenuIndex = 0;
    } else if (subMenuIndex == 2) {
      Serial.println(F("GET_PAIRED_DEVICES"));
      delay(10);
      currentMenuState = PAIRING_MENU;
      subMenuIndex = 0;
    }
  }
  else if (currentMenuState == DISPLAY_MENU) {
    if (subMenuIndex == 0) {
      currentMenuState = DISPLAY_BRIGHTNESS;
      Serial.println(F("DISPLAY_BRIGHTNESS_MENU_SELECTED"));
    }
  }
  else if (currentMenuState == DISPLAY_BRIGHTNESS) {
    Serial.println(F("ADJUSTING_BRIGHTNESS"));
  }
  else if (currentMenuState == PAIRING_MENU) {
    Serial.print(F("PAIRING_SELECT:"));
    // Print pairingMenuItems[subMenuIndex]
    char buffer[16];
    strcpy_P(buffer, (char*)pgm_read_word(&(pairingMenuItems[subMenuIndex])));
    Serial.println(buffer);
  }
}

// --------------------------------------------------
void handleDoubleClick() {
  if(callIncoming && !inMenu) {
    inMenu = true;
    currentMenuState = CALL_MENU;
    subMenuIndex = 0;
    return;
  }

  if(!inMenu){
    inMenu = true;
    currentMenuState = MAIN_MENU;
    menuIndex = 0;
    subMenuIndex = 0;
    return;
  }
  if(inMenu){
    if(currentMenuState == MAIN_MENU){
      inMenu = false;
      return;
    } else {
      currentMenuState = MAIN_MENU;
      menuIndex = 0;
      subMenuIndex = 0;
    }
  }
}

// --------------------------------------------------
// drawMenu: Now reads strings from PROGMEM
// --------------------------------------------------
void drawMenuItemFromPROGMEM(const char* const *menuItems, int index, int y) {
  char buffer[16];
  strcpy_P(buffer, (char*)pgm_read_word(&(menuItems[index])));
  display.setCursor(0, y);
  display.print(buffer);
}

void drawMenu(const char* const menuItems[], int count, int selected) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  int startIdx = max(0, min(selected - 1, count - 3));
  for (int i = startIdx; i < min(startIdx + 3, count); i++) {
    int y = (i - startIdx) * 10;
    if (i == selected) {
      display.fillRect(0, y, SCREEN_WIDTH, 10, SSD1306_WHITE);
      display.setTextColor(SSD1306_BLACK);
    } else {
      display.setTextColor(SSD1306_WHITE);
    }
    // Copy string from PROGMEM for display
    drawMenuItemFromPROGMEM(menuItems, i, y);
  }
  display.display();
}


// --------------------------------------------------
void displayMenu() {
  switch (currentMenuState) {
    case MAIN_MENU:
      drawMenu(mainMenuItems, mainMenuCount, menuIndex);
      break;
    case TUNER_MENU:
      drawMenu(tunerItems, tunerCount, subMenuIndex);
      break;
    case DISPLAY_MENU:
      drawMenu(displayItems, displayCount, subMenuIndex);
      break;
    case DISPLAY_BRIGHTNESS:
      drawValueMenu(F("Brightness"), brightnessLevel);
      break;
    case DISPLAY_CONTRAST:
      drawValueMenu(F("Contrast"), contrastLevel);
      break;
    case SYSTEM_MENU:
      drawMenu(systemItems, systemCount, subMenuIndex);
      break;
    case DEVICE_NAME_MENU:
      drawValueMenu_S(F("Device Name"), deviceName);
      break;
    case CALL_MENU:
      drawMenu(callMenuItems, callMenuCount, subMenuIndex);
      break;
    case PAIRING_MENU:
      drawMenu(pairingMenuItems, pairingMenuCount, subMenuIndex);
      break;
  }
}

// --------------------------------------------------
void drawValueMenu(String title, int value) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // Center the title
  int16_t xPos = (SCREEN_WIDTH - (title.length() * 6)) / 2;
  display.setCursor(xPos, 0);
  display.print(title);

  // Center the int value
  String valStr = String(value);
  xPos = (SCREEN_WIDTH - (valStr.length() * 6)) / 2;
  display.setCursor(xPos, 12);
  display.print(valStr);
  
  display.display();
}

// --------------------------------------------------
void drawValueMenu_S(String title, String textValue) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  
  int16_t xPos = (SCREEN_WIDTH - (title.length() * 6)) / 2;
  display.setCursor(xPos, 0);
  display.print(title);

  xPos = (SCREEN_WIDTH - (textValue.length() * 6)) / 2;
  display.setCursor(xPos, 12);
  display.print(textValue);

  display.display();
}

// --------------------------------------------------
// The main UI when not in a menu
// --------------------------------------------------
int songScrollOffset   = 0;
int artistScrollOffset = 0;
unsigned long lastScrollUpdate = 0;
const unsigned long scrollInterval = 150;

void displayUI() {
  if (!displayOn) return;

  // Check if in FF/RW animation
  if (fastForwardAnimationActive || rewindAnimationActive) {
    unsigned long animTime = millis() - animationStartTime;
    if (animTime < animationDuration) {
      display.clearDisplay();
      display.setTextSize(2);
      display.setTextColor(SSD1306_WHITE);

      int xCenter = (SCREEN_WIDTH / 2) - 6;
      int yCenter = (SCREEN_HEIGHT / 2) - 8;
      display.setCursor(xCenter, yCenter);

      if (fastForwardAnimationActive) {
        display.print(F(">>"));
      }
      else if (rewindAnimationActive) {
        display.print(F("<<"));
      }
      display.display();
      return;
    }
    else {
      fastForwardAnimationActive = false;
      rewindAnimationActive      = false;
    }
  }

  if (inMenu) {
    displayMenu();
    return;
  }

  // Normal UI
  display.setTextWrap(false);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  int seekLineWidth = map(currentPosition, 2, 100, 2, 128);
  if (seekLineWidth > 2) {
    display.drawLine(0, 0, seekLineWidth, 0, SSD1306_WHITE);
  }

  int sep = songName.indexOf(" - ");
  String artist = "";
  String song   = "";
  if (sep != -1) {
    artist = songName.substring(0, sep);
    song   = songName.substring(sep + 3);
  } else {
    song   = songName;
  }

  if (millis() - lastScrollUpdate > scrollInterval) {
    lastScrollUpdate = millis();
    if (song.length() > 21) {
      songScrollOffset = (songScrollOffset + 1) 
                         % ((song.length() * 6) - SCREEN_WIDTH + 6);
    } else {
      songScrollOffset = 0;
    }
    if (artist.length() > 21) {
      artistScrollOffset = (artistScrollOffset + 1) 
                           % ((artist.length() * 6) - SCREEN_WIDTH + 6);
    } else {
      artistScrollOffset = 0;
    }
  }

  // Draw scrolling song
  int songW = song.length() * 6;
  int songX = (SCREEN_WIDTH - min(SCREEN_WIDTH, songW)) / 2;
  display.fillRect(0, 2, SCREEN_WIDTH, 12, SSD1306_BLACK);

  for (int i = 0; i < (int)song.length(); i++) {
    int charX = songX + (i * 6) - songScrollOffset;
    if ((charX + 7) <= 0) continue;
    if (charX >= SCREEN_WIDTH) break;

    display.setCursor(charX, 2);
    display.write(song[i]);
    display.setCursor(charX + 1, 2);
    display.write(song[i]);
  }

  // Draw scrolling artist
  if (sep != -1) {
    int artistW = artist.length() * 6;
    int artistX = (SCREEN_WIDTH - min(SCREEN_WIDTH, artistW)) / 2;
    display.fillRect(0, 14, SCREEN_WIDTH, 12, SSD1306_BLACK);

    for (int i = 0; i < (int)artist.length(); i++) {
      int charX = artistX + (i * 6) - artistScrollOffset;
      if ((charX + 6) <= 0) continue;
      if (charX >= SCREEN_WIDTH) break;

      display.setCursor(charX, 14);
      display.write(artist[i]);
    }
  }

  // Clear bottom line
  for (int i = 0; i < SCREEN_WIDTH; i++) {
    display.drawLine(i, SCREEN_HEIGHT, i, SCREEN_HEIGHT, SSD1306_BLACK);
  }

  // Volume bar
  int volumeBarWidth = map(volume, 0, 100, 0, 90);
  display.fillRect(18, 28, volumeBarWidth, 2, SSD1306_WHITE);

  // Volume ticks
  for (int v = 0; v <= 100; v += 5) {
    int tickPosition = map(v, 0, 100, 18, 108);
    display.drawLine(tickPosition, 28, tickPosition, 29, SSD1306_WHITE);
  }

  // Volume number
  display.setCursor(0, 24);
  display.print(volume);

  // Play/pause indicator
  int playPauseX = (SCREEN_WIDTH - 6) / 2;
  int playPauseY = 21;
  if (isPlaying) {
    static const unsigned char playIconLeft[] PROGMEM = {
      0b01110000,
      0b01111000,
      0b01111100,
      0b01111100,
      0b01111000,
      0b01110000
    };
    display.drawBitmap(playPauseX, playPauseY, playIconLeft, 6, 6, SSD1306_WHITE);
  } else {
    display.setCursor(playPauseX, playPauseY);
    boldText("=");
  }

  // Bluetooth indicator
  if(isBTConnected) {
    display.setCursor(SCREEN_WIDTH - 12, 24);
    display.print(F("BT"));
  }


  display.display();
}

// --------------------------------------------------
void controlLED(unsigned long currentTime) {
  static bool isFlashing = false;
  static bool ledOn      = true;
  static unsigned long lastToggle = 0;

  int buttonState = digitalRead(SW);
  if(buttonState == LOW){
    if(!isFlashing){
      isFlashing = true;
      ledOn      = false;
      digitalWrite(LED_PIN, LOW);
      lastToggle = currentTime;
    } else {
      if(currentTime - lastToggle >= 500){
        ledOn = !ledOn;
        digitalWrite(LED_PIN, ledOn ? HIGH : LOW);
        lastToggle = currentTime;
      }
    }
  } else {
    if(isFlashing){
      isFlashing = false;
      digitalWrite(LED_PIN, HIGH);
    }
  }
}

// --------------------------------------------------
void toggleDisplay() {
  displayOn = !displayOn;
  if (displayOn) {
    display.ssd1306_command(0xAF); // ON
  } else {
    display.ssd1306_command(0xAE); // OFF
  }
}

// --------------------------------------------------
void handleInputs(unsigned long currentTime) {
  currentStateCLK = digitalRead(CLK);
  currentStateDT  = digitalRead(DT);
  bool clkChanged = (currentStateCLK != lastStateCLK);

  int currentButtonState = digitalRead(SW);
  bool buttonPressed  = (currentButtonState == LOW);
  bool buttonReleased = (lastButtonState == LOW && currentButtonState == HIGH);

  unsigned long heldTime = (buttonPressed) ? (currentTime - buttonPressStartTime) : 0;

  static int  ffRwAccum   = 0;
  static bool inFFRWMode  = false;

  // Handle encoder rotation
  if (clkChanged) {
    if (inMenu) {
      if (currentStateDT != currentStateCLK) encoderAccum++;
      else encoderAccum--;

      if (abs(encoderAccum) >= 2) {
        navigateMenu((encoderAccum > 0) ? 1 : -1);
        encoderAccum = 0;
      }
    } else {
      if (buttonPressed) {
        inFFRWMode = true;
        if (currentStateDT != currentStateCLK) ffRwAccum++;
        else ffRwAccum--;
        if (abs(ffRwAccum) >= 2) {
          if (ffRwAccum > 0) {
            Serial.println(F("FF"));
            fastForwardAnimationActive = true;
            rewindAnimationActive      = false;
            animationStartTime         = currentTime;
          } else {
            Serial.println(F("RW"));
            rewindAnimationActive      = true;
            fastForwardAnimationActive = false;
            animationStartTime         = currentTime;
          }
          ffRwAccum = 0;
        }
      } else {
        ffRwAccum  = 0;
        inFFRWMode = false;
        // Adjust volume
        if (currentStateDT != currentStateCLK) {
          volume = min(100, volume + 1);
          Serial.print(F("VOL+"));
          Serial.println(volume);
        } else {
          volume = max(0, volume - 1);
          Serial.print(F("VOL-"));
          Serial.println(volume);
        }
      }
    }
  }

  // Handle button press/release
  if (buttonPressed) {
    if (lastButtonState == HIGH) {
      buttonPressStartTime = currentTime;
      longPressHandled     = false;
      inFFRWMode           = false;
    }
    else {
      if (!longPressHandled && !inFFRWMode && (heldTime > longPressThreshold)) {
        if (inMenu && currentMenuState != MAIN_MENU) {
          inMenu           = false;
          currentMenuState = MAIN_MENU;
          Serial.println(F("Exited to Main Menu via Long Press."));
        }
        else {
          isPlaying = !isPlaying;
          Serial.println(isPlaying ? F("PLAY") : F("PAUSE"));
        }
        longPressHandled = true;
      }
    }
  } 
  else {
    if (buttonReleased) {
      bool shortPress = ((currentTime - buttonPressStartTime) <= doubleClickThreshold);
      if (!longPressHandled && shortPress) {
        if (singleClickPending && (currentTime - singleClickTime < doubleClickThreshold)) {
          singleClickPending = false;
          Serial.println(F("DOUBLE_CLICK"));
          handleDoubleClick();
        } else {
          singleClickPending = true;
          singleClickTime    = currentTime;
        }

        if ((currentTime - lastRapidClickTime) > rapidClickInterval) {
          rapidClickCount = 0;
        }
        rapidClickCount++;
        lastRapidClickTime  = currentTime;
        lastButtonPressTime = currentTime;

        if (rapidClickCount >= 5) {
          toggleDisplay();
          rapidClickCount = 0;
        }
      }
    }
  }

  // Check single click
  if (singleClickPending && (currentTime - singleClickTime > doubleClickThreshold)) {
    singleClickPending = false;
    if (inMenu) {
      handleMenuClick();
    }
  }

  // Update last states
  lastStateCLK    = currentStateCLK;
  lastStateDT     = currentStateDT;
  lastButtonState = currentButtonState;
}