#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <avr/wdt.h>

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

#define CLK 4
#define DT 3
#define SW 2
#define LED_PIN 7

int lastStateCLK, currentStateCLK, lastStateDT, currentStateDT, lastButtonState;
unsigned long buttonPressStartTime=0, lastButtonPressTime=0, lastSongUpdateTime=0, lastRapidClickTime=0;
bool longPressHandled=false, isPlaying=true, isBTConnected=true, inMenu=false, displayOn=true;
const unsigned long doubleClickThreshold=300, longPressThreshold=2000, rapidClickInterval=500;

// Call-incoming flag
bool callIncoming = false;

String songName="";
int songPixelLength=0, currentPosition=0, volume=50;
int menuIndex=0, subMenuIndex=0, brightnessLevel=50, contrastLevel=50;
int encoderAccum=0, rapidClickCount=0;
String deviceName = "NONE";

// When we do FF or RW, we show a short “animation” 
bool fastForwardAnimationActive = false;
bool rewindAnimationActive      = false;
unsigned long animationStartTime=0;
const unsigned long animationDuration = 500; // ms to display >> or <<

// --------------------------------------------------
// CALL_MENU state
// --------------------------------------------------
enum MenuState {
  MAIN_MENU,
  TUNER_MENU,
  DISPLAY_MENU,
  DISPLAY_BRIGHTNESS,
  DISPLAY_CONTRAST,
  SYSTEM_MENU,
  DEVICE_NAME_MENU,
  CALL_MENU
};
MenuState currentMenuState = MAIN_MENU;

const char* mainMenuItems[]    = { "Tuner", "Display", "System" };
const int   mainMenuCount      = 3;
const char* tunerItems[]       = { "Bass", "Treble", "Loudness" };
const int   tunerCount         = 3;
const char* displayItems[]     = { "Brightness" };
const int   displayCount       = 1;
const char* systemItems[]      = { "Restart Service", "Device Name" };
const int   systemCount        = 2;
const char* callMenuItems[]    = { "Accept", "Reject" };
const int   callMenuCount      = 2;

void applyDisplaySettings() {
  uint8_t contrastValue = map(brightnessLevel, 0, 100, 0, 255);
  display.ssd1306_command(0x81);
  display.ssd1306_command(contrastValue);
}

void calculateSongPixelLength() {
  songPixelLength = songName.length() * 6;
}

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

void boldText(String text) {
  int x=display.getCursorX();
  int y=display.getCursorY();
  display.setCursor(x,y);   display.print(text);
  display.setCursor(x+1,y); display.print(text);
  display.setCursor(x,y);
}

void softwareReset() {
  wdt_enable(WDTO_15MS);
  while(1) {}
}

// --------------------------------------------------
// handleSerialInput
// --------------------------------------------------
void handleSerialInput(String input) {
  input.trim();
  if(input.startsWith("SONG:")){
    // Check if "[INCOMING]" substring => callIncoming = true
    if (input.indexOf("[INCOMING]") != -1) {
      callIncoming = true;
    }
    songName = input.substring(5);
    calculateSongPixelLength();
  } 
  else if(input.startsWith("POS:")) {
    currentPosition = input.substring(4).toInt();
  }
  else if(input.startsWith("VOL:")) {
    volume = input.substring(4).toInt();
  }
  else if(input.startsWith("STATE:")) {
    isPlaying = (input.substring(6)=="PLAY");
  }
  else if(input.startsWith("BT:")) {
    isBTConnected = (input.substring(3)=="ON");
  }
  else if(input.equalsIgnoreCase("RESET")){
    Serial.println("System is resetting...");
    softwareReset();
  }
}

// --------------------------------------------------
// navigateMenu
// --------------------------------------------------
void navigateMenu(int d) {
  if(!inMenu) return;

  if (currentMenuState == CALL_MENU) {
    // Move subMenuIndex up/down
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
  else if(currentMenuState == SYSTEM_MENU) {
    if(subMenuIndex == 0) {
      Serial.println("RESTART_SERVICE");
    } else if(subMenuIndex == 1) {
      Serial.println("DEVICE_NAME");
    }
  }
  else if(currentMenuState == DISPLAY_BRIGHTNESS) {
    brightnessLevel += d;
    brightnessLevel = max(0, min(100, brightnessLevel));
    applyDisplaySettings();
  }
}

// --------------------------------------------------
// handleMenuClick
// --------------------------------------------------
void handleMenuClick() {
  if(!inMenu) return;

  if(currentMenuState == CALL_MENU) {
    if(subMenuIndex == 0) {
      // Accept
      Serial.println("ACCEPT_CALL");
    } else {
      // Reject
      Serial.println("REJECT_CALL");
    }
    callIncoming      = false;
    inMenu            = false;
    currentMenuState  = MAIN_MENU;
    return;
  }

  // Normal menus
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
  else if(currentMenuState == DISPLAY_MENU) {
    if(subMenuIndex == 0) {
      currentMenuState = DISPLAY_BRIGHTNESS;
    }
  }
  else if(currentMenuState == SYSTEM_MENU) {
    if(subMenuIndex == 0) {
      Serial.println("RESTART_SERVICE");
    } else if(subMenuIndex == 1) {
      currentMenuState = DEVICE_NAME_MENU;
    }
  }
}

// --------------------------------------------------
// handleDoubleClick
// --------------------------------------------------
void handleDoubleClick() {
  // If there's an incoming call, open CALL_MENU
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

void drawMenu(const char* items[], int count, int selected) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  for(int i=0; i<count; i++){
    display.setCursor(0, i*10);
    if(i==selected){
      display.fillRect(0, i*10, 128, 10, SSD1306_WHITE);
      display.setTextColor(SSD1306_BLACK);
    } else {
      display.setTextColor(SSD1306_WHITE);
    }
    display.print(items[i]);
  }
  display.display();
}

void drawValueMenu(String title,int value) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor((128 - (title.length()*6))/2, 0);
  display.print(title);
  display.setCursor((128 - (String(value).length()*6))/2, 12);
  display.print(value);
  display.display();
}

void drawValueMenu_S(String title, String textValue) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor((128 - (title.length()*6)) / 2, 0);
  display.print(title);

  display.setCursor((128 - (textValue.length()*6)) / 2, 12);
  display.print(textValue);

  display.display();
}

// --------------------------------------------------
// displayMenu
// --------------------------------------------------
void displayMenu() {
  switch(currentMenuState) {
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
      drawValueMenu("Brightness", brightnessLevel);
      break;
    case DISPLAY_CONTRAST:
      drawValueMenu("Contrast", contrastLevel);
      break;
    case SYSTEM_MENU:
      drawMenu(systemItems, systemCount, subMenuIndex);
      break;
    case DEVICE_NAME_MENU:
      drawValueMenu_S("Device Name", deviceName);
      break;
    case CALL_MENU:
      drawMenu(callMenuItems, callMenuCount, subMenuIndex);
      break;
  }
}

// --------------------------------------------------
// The main UI when not in a menu
// --------------------------------------------------
int songScrollOffset   = 0;
int artistScrollOffset = 0;
unsigned long lastScrollUpdate = 0;
const unsigned long scrollInterval = 150;

void displayUI() {
  if (!displayOn) {
    return;
  }

  // ------------------------------------------------
  // Check if we’re in FF/RW “animation”
  // ------------------------------------------------
  if (fastForwardAnimationActive || rewindAnimationActive) {
    unsigned long animTime = millis() - animationStartTime;
    if (animTime < animationDuration) {
      // STILL within the animation window
      display.clearDisplay();
      display.setTextSize(2);
      display.setTextColor(SSD1306_WHITE);

      // Just place the arrows in the center for demo
      int xCenter = (SCREEN_WIDTH / 2) - 6;
      int yCenter = (SCREEN_HEIGHT / 2) - 8;
      display.setCursor(xCenter, yCenter);

      if (fastForwardAnimationActive) {
        // Example: display ">>"
        display.print(">>");
      } 
      else if (rewindAnimationActive) {
        // Example: display "<<"
        display.print("<<");
      }
      display.display();
      return; // skip normal UI rendering
    }
    else {
      // Animation has expired
      fastForwardAnimationActive = false;
      rewindAnimationActive      = false;
    }
  }

  // If we’re in a menu, show that
  if (inMenu) {
    displayMenu();
    return;
  }

  // Otherwise, do normal UI
  display.setTextWrap(false);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  int seekLineWidth = map(currentPosition, 2, 100, 2, 128);
  if (seekLineWidth > 2) {
    display.drawLine(0, 0, seekLineWidth, 0, SSD1306_WHITE);
  }

  // Split "Artist - Song"
  int sep = songName.indexOf(" - ");
  String artist = "";
  String song   = "";
  if (sep != -1) {
    artist = songName.substring(0, sep);
    song   = songName.substring(sep + 3);
  } else {
    song   = songName;
  }

  unsigned long currentTime = millis();
  if (currentTime - lastScrollUpdate > scrollInterval) {
    lastScrollUpdate = currentTime;
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
  for (int i = 0; i < song.length(); i++) {
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
    for (int i = 0; i < artist.length(); i++) {
      int charX = artistX + (i * 6) - artistScrollOffset;
      if ((charX + 6) <= 0) continue;
      if (charX >= SCREEN_WIDTH) break;

      display.setCursor(charX, 14);
      display.write(artist[i]);
    }
  }

  // Clear bottom line
  for (int i = 0; i < 128; i++) {
    display.drawLine(i, SCREEN_HEIGHT, i, SCREEN_HEIGHT, SSD1306_BLACK);
  }

  // Volume bar
  int volumeBarWidth = map(volume, 0, 100, 0, 90);
  display.fillRect(18, 28, volumeBarWidth, 2, SSD1306_WHITE);

  // Volume ticks
  for (int v = 0; v <= 100; v += 5) {
    int tickPosition = map(v, 0, 100, 18, 18 + 90);
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
  display.setCursor(SCREEN_WIDTH - 12, 24);
  display.print("BT");

  display.display();
}

// --------------------------------------------------
// controlLED
// --------------------------------------------------
void controlLED(unsigned long currentTime) {
  static bool isFlashing=false;
  static bool ledOn=true;
  static unsigned long lastToggle=0;
  int buttonState = digitalRead(SW);
  if(buttonState==LOW){
    if(!isFlashing){
      isFlashing=true;
      ledOn=false;
      digitalWrite(LED_PIN,LOW);
      lastToggle=currentTime;
    } else {
      if(currentTime - lastToggle >= 500){
        ledOn = !ledOn;
        digitalWrite(LED_PIN, ledOn?HIGH:LOW);
        lastToggle=currentTime;
      }
    }
  } else {
    if(isFlashing){
      isFlashing=false;
      digitalWrite(LED_PIN,HIGH);
    }
  }
}

// Toggle the entire display ON/OFF
void toggleDisplay() {
  displayOn = !displayOn;
  if (displayOn) {
    display.ssd1306_command(0xAF); // Display ON
  } else {
    display.ssd1306_command(0xAE); // Display OFF
  }
}

// --------------------------------------------------
// handleInputs
// --------------------------------------------------
void handleInputs(unsigned long currentTime) {
  currentStateCLK = digitalRead(CLK);
  currentStateDT  = digitalRead(DT);
  bool clkChanged = (currentStateCLK != lastStateCLK);
  int currentButtonState = digitalRead(SW);

  bool buttonPressed  = (currentButtonState == LOW);
  bool buttonReleased = (lastButtonState == LOW && currentButtonState == HIGH);
  unsigned long heldTime = (buttonPressed) ? (currentTime - buttonPressStartTime) : 0;

  // We'll keep a static accumulator for FF/RW spins
  // and a static boolean “inFFRWMode” to track if
  // user is actively spinning for FF/RW (rather than
  // going for a play/pause long-press).
  static int  ffRwAccum   = 0;
  static bool inFFRWMode  = false;

  // Handle encoder rotation
  if (clkChanged) {
    if (inMenu) {
      // In a menu => normal menu navigation
      if (currentStateDT != currentStateCLK) encoderAccum++;
      else encoderAccum--;
      if (abs(encoderAccum) >= 2) {
        navigateMenu((encoderAccum > 0) ? 1 : -1);
        encoderAccum = 0;
      }
    } 
    else {
      // Not in a menu => Either volume or FF/RW
      if (buttonPressed) {
        // If the knob is turned while pressed => FFRW
        inFFRWMode = true;
        if (currentStateDT != currentStateCLK) ffRwAccum++;
        else ffRwAccum--;

        // If we cross threshold, send once
        if (abs(ffRwAccum) >= 2) {
          if (ffRwAccum > 0) {
            Serial.println("FF");
            // Start FF animation
            fastForwardAnimationActive = true;
            rewindAnimationActive      = false;
            animationStartTime         = currentTime;
          } else {
            Serial.println("RW");
            // Start RW animation
            rewindAnimationActive      = true;
            fastForwardAnimationActive = false;
            animationStartTime         = currentTime;
          }
          ffRwAccum = 0;
        }
      }
      else {
        // Button not pressed => normal volume
        ffRwAccum  = 0;
        inFFRWMode = false;

        if (currentStateDT != currentStateCLK) {
          volume = min(100, volume + 1);
          Serial.println("VOL+" + String(volume));
        } else {
          volume = max(0, volume - 1);
          Serial.println("VOL-" + String(volume));
        }
      }
    }
  }

  // Handle button press/release times
  if (buttonPressed) {
    if (lastButtonState == HIGH) {
      buttonPressStartTime = currentTime;
      longPressHandled     = false;
      inFFRWMode           = false;  // We haven't turned yet
    } else {
      // If we have been holding
      if (!longPressHandled && !inFFRWMode && (heldTime > longPressThreshold)) {
        // Only do PLAY/PAUSE if we are NOT in FFRW mode
        isPlaying = !isPlaying;
        Serial.println(isPlaying ? "PLAY" : "PAUSE");
        longPressHandled = true;
      }
    }
  } 
  else {
    // Button is not pressed
    if (buttonReleased) {
      bool shortPress = ((currentTime - buttonPressStartTime) <= doubleClickThreshold);
      if (!longPressHandled && shortPress) {
        // Check for double-click
        if ((currentTime - lastButtonPressTime) < doubleClickThreshold && lastButtonPressTime != 0) {
          Serial.println("DOUBLE_CLICK");
          handleDoubleClick();
          lastButtonPressTime = 0;
        } else {
          // Potential single click
          if ((currentTime - lastRapidClickTime) > rapidClickInterval) {
            rapidClickCount = 0;
          }
          rapidClickCount++;
          lastRapidClickTime = currentTime;
          if (rapidClickCount >= 5) {
            toggleDisplay();
            rapidClickCount = 0;
          }
          lastButtonPressTime = currentTime;

          // Single click for menu?
          if (inMenu) handleMenuClick();
        }
      }
    }
  }

  lastStateCLK = currentStateCLK;
  lastStateDT  = currentStateDT;
  lastButtonState = currentButtonState;
}

void setup() {
  delay(100);
  Serial.begin(9600);

  pinMode(CLK,INPUT);
  pinMode(DT,INPUT);
  pinMode(SW,INPUT_PULLUP);

  lastStateCLK  = digitalRead(CLK);
  lastStateDT   = digitalRead(DT);
  lastButtonState = digitalRead(SW);

  pinMode(LED_PIN,OUTPUT);
  digitalWrite(LED_PIN,HIGH);

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)){
    Serial.println("SSD1309 allocation failed");
    while(true);
  }
  calculateSongPixelLength();
  bootupAnimation();
  applyDisplaySettings();
}

void loop() {
  unsigned long currentTime = millis();
  handleInputs(currentTime);

  // Read any serial commands
  if(Serial.available()){
    String incoming = Serial.readStringUntil('\n');
    handleSerialInput(incoming);
  }

  // Draw UI
  displayUI();

  // Blink LED if SW is held
  controlLED(currentTime);
}
