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

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);

#define CLK 4
#define DT 3
#define SW 2
#define LED_PIN 7

int lastStateCLK;
int currentStateCLK;
int lastStateDT;
int currentStateDT;
int lastButtonState;
unsigned long buttonPressStartTime = 0;
unsigned long lastButtonPressTime = 0;
bool longPressHandled = false;
const unsigned long doubleClickThreshold = 300;
const unsigned long longPressThreshold = 2000;

String songName = "";
int songPixelLength = 0;
int currentPosition = 0;
int volume = 50;
bool isPlaying = true;
bool isBTConnected = true;
bool inMenu = false;
int menuIndex = 0;
int subMenuIndex = 0;
int brightnessLevel=50;
int contrastLevel=50;

enum MenuState {MAIN_MENU,TUNER_MENU,DISPLAY_MENU,DISPLAY_BRIGHTNESS,DISPLAY_CONTRAST};
MenuState currentMenuState=MAIN_MENU;

const char* mainMenuItems[]={"Tuner","Display"};
const int mainMenuCount=2;
const char* tunerItems[]={"Bass","Treble","Loudness"};
const int tunerCount=3;
const char* displayItems[]={"Brightness"};
const int displayCount=1;

int encoderAccum=0;
bool displayOn = true;
int rapidClickCount = 0;
unsigned long lastRapidClickTime = 0;
const unsigned long rapidClickInterval = 500; // 500ms to accumulate up to 5 clicks

void applyDisplaySettings() {
  // With the Adafruit_SSD1306 library, you can send commands directly:
  // Contrast command for SSD1309/SSD1306 is 0x81 followed by contrast value (0-255).
  // brightnessLevel is 0-100, map it to 0-255 for noticeable changes.
  uint8_t contrastValue = map(brightnessLevel, 0, 100, 0, 255);
  
  display.ssd1306_command(0x81);
  display.ssd1306_command(contrastValue);
}

void calculateSongPixelLength(){ songPixelLength = songName.length()*6; }

void bootupAnimation() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  for(int x=0;x<SCREEN_WIDTH;x+=4){
    display.clearDisplay();
    display.drawLine(x,0,x,SCREEN_HEIGHT-1,SSD1306_WHITE);
    display.display();
    delay(10);
  }
  for(int y=0;y<SCREEN_HEIGHT;y+=4){
    display.clearDisplay();
    display.drawLine(0,y,SCREEN_WIDTH-1,y,SSD1306_WHITE);
    display.display();
    delay(10);
  }
}

void boldText(String text) {
  int x=display.getCursorX();
  int y=display.getCursorY();
  display.setCursor(x,y);display.print(text);
  display.setCursor(x+1,y);display.print(text);
  display.setCursor(x,y);
}

void softwareReset() {
  wdt_enable(WDTO_15MS);
  while(1){}
}

void handleSerialInput(String input) {
  input.trim();
  if(input.startsWith("SONG:")){
    songName=input.substring(5);
    calculateSongPixelLength();
  } else if(input.startsWith("POS:")) currentPosition=input.substring(4).toInt();
  else if(input.startsWith("VOL:")) volume=input.substring(4).toInt();
  else if(input.startsWith("STATE:")) isPlaying=(input.substring(6)=="PLAY");
  else if(input.startsWith("BT:")) isBTConnected=(input.substring(3)=="ON");
  else if(input.equalsIgnoreCase("RESET")){
    Serial.println("System is resetting...");
    softwareReset();
  }
}

void navigateMenu(int direction) {
  if(!inMenu)return;
  if(currentMenuState==MAIN_MENU){
    menuIndex+=direction;
    if(menuIndex<0)menuIndex=mainMenuCount-1;
    if(menuIndex>=mainMenuCount)menuIndex=0;
  } else if(currentMenuState==TUNER_MENU){
    subMenuIndex+=direction;
    if(subMenuIndex<0)subMenuIndex=tunerCount-1;
    if(subMenuIndex>=tunerCount)subMenuIndex=0;
  } else if(currentMenuState==DISPLAY_MENU){
    subMenuIndex+=direction;
    if(subMenuIndex<0)subMenuIndex=displayCount-1;
    if(subMenuIndex>=displayCount)subMenuIndex=0;
  } else if(currentMenuState==DISPLAY_BRIGHTNESS){
    brightnessLevel+=direction;
    if(brightnessLevel<0)brightnessLevel=0;
    if(brightnessLevel>100)brightnessLevel=100;
    applyDisplaySettings();
  } 
}

void handleMenuClick() {
  if(!inMenu)return;
  if(currentMenuState==MAIN_MENU){
    if(menuIndex==0){currentMenuState=TUNER_MENU;subMenuIndex=0;}
    else if(menuIndex==1){currentMenuState=DISPLAY_MENU;subMenuIndex=0;}
  } else if(currentMenuState==DISPLAY_MENU){
    if(subMenuIndex==0)currentMenuState=DISPLAY_BRIGHTNESS;
  }
}

void handleDoubleClick() {
  if(!inMenu){
    inMenu=true;
    currentMenuState=MAIN_MENU;
    menuIndex=0;
    subMenuIndex=0;
    return;
  }
  if(inMenu){
    if(currentMenuState==MAIN_MENU){
      inMenu=false;
      return;
    } else {
      currentMenuState=MAIN_MENU;
      menuIndex=0;
      subMenuIndex=0;
    }
  }
}

const unsigned char playIconLeft[] PROGMEM={
  0b01110000,
  0b01111000,
  0b01111100,
  0b01111100,
  0b01111000,
  0b01110000
};

void drawMenu(const char* items[],int count,int selected) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  for(int i=0;i<count;i++){
    display.setCursor(0,i*10);
    if(i==selected){
      display.fillRect(0,i*10,128,10,SSD1306_WHITE);
      display.setTextColor(SSD1306_BLACK);
    }else display.setTextColor(SSD1306_WHITE);
    display.print(items[i]);
  }
  display.display();
}

void drawValueMenu(String title,int value) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor((128-(title.length()*6))/2,0);display.print(title);
  display.setCursor((128-(String(value).length()*6))/2,12);display.print(value);
  display.display();
}

void displayMenu() {
  if(currentMenuState==MAIN_MENU) drawMenu(mainMenuItems,mainMenuCount,menuIndex);
  else if(currentMenuState==TUNER_MENU) drawMenu(tunerItems,tunerCount,subMenuIndex);
  else if(currentMenuState==DISPLAY_MENU) drawMenu(displayItems,displayCount,subMenuIndex);
  else if(currentMenuState==DISPLAY_BRIGHTNESS) drawValueMenu("Brightness",brightnessLevel);
  else if(currentMenuState==DISPLAY_CONTRAST) drawValueMenu("Contrast",contrastLevel);
}


void displayUI() {
  if (!displayOn) {
    // If display is off, do nothing
    return;
  }

  if (inMenu) { displayMenu(); return; }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  int sep = songName.indexOf(" - ");
  String artist = "";
  String song = "";
  if (sep != -1) {
    artist = songName.substring(0, sep);
    song = songName.substring(sep + 3);
  } else song = songName;
  int songW = song.length() * 6;
  int songX = (SCREEN_WIDTH - songW) / 2;
  display.setCursor(songX, 0);
  if (song.length() > 21) boldText(song.substring(0, 21));
  else boldText(song);
  if (sep != -1) {
    int artistW = artist.length() * 6;
    int artistX = (SCREEN_WIDTH - artistW) / 2;
    display.setCursor(artistX, 10);
    if (artist.length() > 21) display.print(artist.substring(0, 21));
    else display.print(artist);
  }
  display.drawRect(0, 20, 128, 4, SSD1306_WHITE);
  int seekBarWidth = map(currentPosition, 0, 100, 0, 128);
  display.fillRect(0, 21, seekBarWidth, 2, SSD1306_WHITE);
  display.setCursor(0, 24);
  display.print("V: "); display.print(volume); display.print("%");
  display.setCursor(92, 24);
  display.print(isBTConnected ? "BT:ON" : "BT:OFF");
  int playPauseX = (SCREEN_WIDTH - 6) / 2;
  int playPauseY = 25;
  if (isPlaying) display.drawBitmap(playPauseX, playPauseY, playIconLeft, 6, 6, SSD1306_WHITE);
  else { display.setCursor(playPauseX, playPauseY); boldText("="); }
  display.display();
}
void controlLED(unsigned long currentTime) {
  static bool isFlashing=false;
  static bool ledOn=true;
  static unsigned long lastToggle=0;
  int buttonState=digitalRead(SW);
  if(buttonState==LOW){
    if(!isFlashing){
      isFlashing=true;
      ledOn=false;
      digitalWrite(LED_PIN,LOW);
      lastToggle=currentTime;
    }else{
      if(currentTime-lastToggle>=500){
        ledOn=!ledOn;
        digitalWrite(LED_PIN,ledOn?HIGH:LOW);
        lastToggle=currentTime;
      }
    }
  }else{
    if(isFlashing){
      isFlashing=false;
      digitalWrite(LED_PIN,HIGH);
    }
  }
}

void toggleDisplay() {
  displayOn = !displayOn;
  if (displayOn) {
    display.ssd1306_command(0xAF); // Display ON
  } else {
    display.ssd1306_command(0xAE); // Display OFF
  }
}



void handleInputs(unsigned long currentTime) {
  currentStateCLK = digitalRead(CLK);
  currentStateDT = digitalRead(DT);
  bool clkChanged = (currentStateCLK != lastStateCLK);
  int currentButtonState = digitalRead(SW);
  bool buttonReleased = (lastButtonState == LOW && currentButtonState == HIGH);

  if (clkChanged) {
    if (inMenu) {
      if (currentStateDT != currentStateCLK) encoderAccum++;
      else encoderAccum--;
      if (abs(encoderAccum) >= 2) {
        navigateMenu((encoderAccum > 0) ? 1 : -1);
        encoderAccum = 0;
      }
    } else {
      if (currentStateDT != currentStateCLK) {
        volume = min(100, volume + 1);
        Serial.println("VOL+" + String(volume));
      } else {
        volume = max(0, volume - 1);
        Serial.println("VOL-" + String(volume));
      }
    }
  }

  if (currentButtonState == LOW) {
    if (lastButtonState == HIGH) {
      buttonPressStartTime = currentTime;
      longPressHandled = false;
    } else {
      if ((currentTime - buttonPressStartTime) > longPressThreshold && !longPressHandled) {
        isPlaying = !isPlaying;
        Serial.println(isPlaying ? "PLAY" : "PAUSE");
        longPressHandled = true;
      }
    }
  } else {
    if (buttonReleased) {
      if (!longPressHandled && ((currentTime - buttonPressStartTime) <= doubleClickThreshold)) {
        // Check for double click
        if ((currentTime - lastButtonPressTime) < doubleClickThreshold && lastButtonPressTime != 0) {
          Serial.println("DOUBLE_CLICK");
          handleDoubleClick();
          lastButtonPressTime = 0;
        } else {
          // This is a potential single click
          // Track rapid clicks for 5-press toggle
          if (currentTime - lastRapidClickTime > rapidClickInterval) {
            rapidClickCount = 0; 
          }
          rapidClickCount++;
          lastRapidClickTime = currentTime;
          if (rapidClickCount >= 5) {
            toggleDisplay();
            rapidClickCount = 0;
          }

          lastButtonPressTime = currentTime;
        }
      } else if (!longPressHandled) {
        // Single click confirmed (not double click)
        if (inMenu) handleMenuClick();
      }
    }
  }

  lastStateCLK = currentStateCLK;
  lastStateDT = currentStateDT;
  lastButtonState = currentButtonState;
}

void setup() {
  Serial.begin(9600);
  pinMode(CLK,INPUT);
  pinMode(DT,INPUT);
  pinMode(SW,INPUT_PULLUP);
  lastStateCLK=digitalRead(CLK);
  lastStateDT=digitalRead(DT);
  lastButtonState=digitalRead(SW);
  pinMode(LED_PIN,OUTPUT);
  digitalWrite(LED_PIN,HIGH);
  if(!display.begin(SSD1306_SWITCHCAPVCC,0x3C)){
    Serial.println("SSD1309 allocation failed");
    while(true);
  }
  calculateSongPixelLength();
  bootupAnimation();
  applyDisplaySettings();
}

void loop() {
  unsigned long currentTime=millis();
  handleInputs(currentTime);
  if(Serial.available()){
    String incoming=Serial.readStringUntil('\n');
    handleSerialInput(incoming);
  }
  displayUI();
  controlLED(currentTime);
}
