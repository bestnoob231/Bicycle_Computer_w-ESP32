// /==================================================================================================\
// ||                                                                                                ||
// ||         ____  _                 _         ____                            _                    ||
// ||        | __ )(_) ___ _   _  ___| | ___   / ___|___  _ __ ___  _ __  _   _| |_ ___ _ __         ||
// ||        |  _ \| |/ __| | | |/ __| |/ _ \ | |   / _ \| '_ ` _ \| '_ \| | | | __/ _ \ '__|        ||
// ||        | |_) | | (__| |_| | (__| |  __/ | |__| (_) | | | | | | |_) | |_| | ||  __/ |           ||
// ||        |____/|_|\___|\__, |\___|_|\___|  \____\___/|_| |_| |_| .__/ \__,_|\__\___|_|           ||
// ||                      |___/                                   |_|                               ||
// ||                                     _______ ____  ____ _________                               ||
// ||                          __      __/ / ____/ ___||  _ \___ /___ \                              ||
// ||                          \ \ /\ / / /|  _| \___ \| |_) ||_ \ __) |                             ||
// ||                           \ V  V / / | |___ ___) |  __/___) / __/                              ||
// ||                            \_/\_/_/  |_____|____/|_|  |____/_____|                             ||
// ||                                                                                                ||
// ||         _ _   _           _       ___               _                     _    ____  _____ _   ||
// ||    __ _(_) |_| |__  _   _| |__   / / |__   ___  ___| |_ _ __   ___   ___ | |__|___ \|___ // |  ||
// ||   / _` | | __| '_ \| | | | '_ \ / /| '_ \ / _ \/ __| __| '_ \ / _ \ / _ \| '_ \ __) | |_ \| |  ||
// ||  | (_| | | |_| | | | |_| | |_) / / | |_) |  __/\__ \ |_| | | | (_) | (_) | |_) / __/ ___) | |  ||
// ||   \__, |_|\__|_| |_|\__,_|_.__/_/  |_.__/ \___||___/\__|_| |_|\___/ \___/|_.__/_____|____/|_|  ||
// ||   |___/                                                                                        ||
// ||                                                                                                ||
// \==================================================================================================/

//#################   INCLUDES    #################

#include <EEPROM.h>
#include <TFT_eSPI.h>
#include <images.h>

//#################################################


//#################  DEFINITIONS  #################

bool debug = false;   //  Default "false". If it's true, there will be a lot of Serial.print

// TFT screen object and variables
//. . . . . . . . . . . . . . . . . . . . . . . . . . .
TFT_eSPI tft = TFT_eSPI();
unsigned short txtFont = 2;             // Selecting font. For more information search for TFT_eSPI Library
unsigned long lastShowGearChange = 0;   // For tracking of when was last change of state 
bool draw_gear_icon = false;            // default "false". For tracking whether the gear icon is drawn or not
bool darkTheme = true;                  // dark = true, white = false. For rendering menu's theme
uint16_t backgroundColor = 0x0000;      // Default "0x0000" (black). Background color hex value
uint16_t textColor = 0xFFFF;            // Default "0xFFFF" (white). Text rendering color hex value
//. . . . . . . . . . . . . . . . . . . . . . . . . . .

// Other pin definitions
//. . . . . . . . . . . . . . . . . . . . . . . . . . .
#define HallEffectSensor 15   // ESP32 pin 21 aka. GPIO15 
#define buttonLeft 12         // ESP32 pin 18 aka. GPIO12
#define buttonMid 13          // ESP32 pin 20 aka. GPIO13
#define buttonRight 14        // ESP32 pin 17 aka. GPIO14
//. . . . . . . . . . . . . . . . . . . . . . . . . . .

// Needs for menu operations
//. . . . . . . . . . . . . . . . . . . . . . . . . . .
enum MenuState {
  TIME,             // Main menu
  AVG,              // 
  SETTINGS,         // - - - - - - - - - - - - - - - -
  THEME,            // Settings menu
  THEME_DARK,       // For change theme. Not a menu 
  THEME_WHITE,      // -------------------------------
  CIRC,             // Settings menu
  CIRC_26,          // For change diameter. Not a menu
  CIRC_27,          // --------------------------------
  CIRC_28,          // 
  CIRC_29,          // --------------------------------
  RESET,            // Settings menu
  BACK,             // For quiting from settings
  MENU_ENUM_COUNT   // Using for knowing end of the enum
};
MenuState currentMenu = TIME;   // Default "TIME". For keeping the current menu state
bool enter_settings = false;    // in settings = true, not in settings = false. To select menu cycle; main menu or settings
//. . . . . . . . . . . . . . . . . . . . . . . . . . .

// Variables to keep track of button presses
//. . . . . . . . . . . . . . . . . . . . . . . . . . .
#define DEBOUNCE_TIME 200   // Default 200 (ms). If button ghosting occure, increase value. If you want click faster in a row, decrease value.  
unsigned long lastButtonPressTime = 0;      // For tracking of when was last press
volatile bool buttonLeftPressed = false;    // Default false. Left button flag
volatile bool buttonRightPressed = false;   // Default false. Right button flag
volatile bool buttonMidPressed = false;     // Default false. Middle button flag
//. . . . . . . . . . . . . . . . . . . . . . . . . . .

//#################################################


//################# INITIALIZIONS #################

// Variables for calculating and tracking time
//. . . . . . . . . . . . . . . . . . . . . . . . . . .
unsigned long seconds = 0;
unsigned long minutes = 0;
unsigned long hours = 0;
//. . . . . . . . . . . . . . . . . . . . . . . . . . .

// All string variables
//. . . . . . . . . . . . . . . . . . . . . . . . . . .
//Strings variables for printing to screen
String speedStr;
String maxSpeedStr;
String avgSpeedStr;
String totalDistanceStr;
//. . . . . . . . . . . . . . . . . . . . . . . . . . .

// EEPROM variables
//. . . . . . . . . . . . . . . . . . . . . . . . . . .
#define EEPROM_SIZE 64    // EEPROM size that will used
const int eeMaxSpeedAddress = 0;           // 0-3
const int eeAvgSpeedAddress = 4;           // 4-7
const int eeDistAddress = 8;               // 8-11
const int eeDistHundredAddress = 12;       // 12-15
const int eeElapsedTotalTimeAddress = 16;  // 16-19
const int eeDarkThemeAddress = 20;         // 19 (bool için 1 byte yeter)
const int eeDiameterAddress = 24;          // 24-27
//. . . . . . . . . . . . . . . . . . . . . . . . . . .

// Misc
//. . . . . . . . . . . . . . . . . . . . . . . . . . .
unsigned int waitUntil = 1600;          // "calculate()" function will be triggered if designated time has passed
volatile unsigned short counter = 0;    // Counter for storing how many passes did magnet on wheel

float fStartTime = 0.0;
float fElapsedTime = 0.0;

float fSpeed = 0.0;
float fMaxSpeed = 0.0;
float fAvgSpeed = 0.0;

float fTakenDistanceCm = 0.0;
float fTotalDistanceKm = 0.0;
int distanceHundredKm = 0;
float fElapsedTotalTime = 0.0;

//  "W_DIAMETER" is bicycle's wheel diameter (in cm) that magnet attached. It should be;

float currentDiameter = 66.04;
const float diameters[] = { 66.04, 69.00, 71.00, 73.00 };
const uint8_t menuCirc[] = { CIRC_26, CIRC_27, CIRC_28, CIRC_29 };

// float wheelDiameters[4] = {66.04, 69.85, 71.12, 73.66};
//. . . . . . . . . . . . . . . . . . . . . . . . . . .

void menuDynamic(int spd_x, int spd_y, int max_x, int max_y, int time_x, int time_y, int dist_x, int dist_y, int avg_x, int avg_y);
void menuStatic(int spd_x, int spd_y, int max_x, int max_y, int time_x, int time_y, int dist_x, int dist_y, int avg_x, int avg_y);
void settingsStatic(String text_1, String text_2, String text_3, String text_4, byte textSize);
void settingsDynamic(int selected);
void resetAll();

//#################################################


void IRAM_ATTR hallInterrupt() {
  counter++;
}

void IRAM_ATTR buttonInterrupt() {
  unsigned long currentTime = millis();
  if ((currentTime - lastButtonPressTime) > DEBOUNCE_TIME) {
    if (digitalRead(buttonLeft) == LOW) {
      buttonLeftPressed = true;
    } else if (digitalRead(buttonRight) == LOW) {
      buttonRightPressed = true;
    } else if (digitalRead(buttonMid) == LOW) {
      buttonMidPressed = true;
    }
    lastButtonPressTime = currentTime;
  }
}

bool shouldBypassMenu(MenuState menu) {
  switch (menu) {
    case THEME_DARK:
    case THEME_WHITE:
    case CIRC_29:
    case CIRC_28:
    case CIRC_27:
    case CIRC_26:
      return true;
    default:
      return false;
  }
}

void nextMenu() {
  do {
    currentMenu = static_cast<MenuState>((currentMenu + 1) % MENU_ENUM_COUNT);
  } while (
    (enter_settings && (currentMenu <= SETTINGS || shouldBypassMenu(currentMenu))) || (!enter_settings && currentMenu > SETTINGS));
}

void previousMenu() {
  do {
    currentMenu = static_cast<MenuState>((currentMenu - 1 + MENU_ENUM_COUNT) % MENU_ENUM_COUNT);
  } while (
    (enter_settings && (currentMenu <= SETTINGS || shouldBypassMenu(currentMenu))) || (!enter_settings && currentMenu > SETTINGS));
}

void changeTheme() {
  if (darkTheme) {
    textColor = 0XFFFF;
    backgroundColor = 0X0000;
    darkTheme = true;
  } else {
    textColor = 0X0000;
    backgroundColor = 0XFFFF;
    darkTheme = false;
  }
}

void changeDiameter(MenuState state) {
  switch (state) {
    case CIRC_26:
      currentDiameter = 66.04;
      break;
    case CIRC_27:
      currentDiameter = 69.85;
      break;
    case CIRC_28:
      currentDiameter = 71.12;
      break;
    case CIRC_29:
      currentDiameter = 73.66;
      break;
  }
}

void drawImage(const uint8_t img[], unsigned &img_w, unsigned &img_h, bool &is_drew, unsigned img_size = 4) {
  unsigned long now = millis();

  if (now - lastShowGearChange > 500) {

    tft.setTextSize(img_size);
    tft.setTextDatum(BL_DATUM);

    switch (is_drew) {
      case 0:
        tft.drawBitmap(
          tft.width() - (img_w + 5), tft.height() - (img_h + 5),
          img, gearW, gearH,
          textColor);

        is_drew = true;
        break;
      case 1:
        tft.drawBitmap(
          tft.width() - (img_w + 5), tft.height() - (img_h + 5),
          img, gearW, gearH,
          backgroundColor);

        is_drew = false;
        break;
    }

    lastShowGearChange = millis();
  }
}

void displayMenuStatic(MenuState state) {
  tft.fillScreen(backgroundColor);  // Clear the screen

  switch (state) {
    case TIME:
      menuStatic(5, 0, 70, 0, 50, 50, 5, 115, 70, 115);
      break;
    case AVG:
      menuStatic(5, 0, 70, 0, 70, 115, 5, 115, 53, 50);
      break;
    case SETTINGS:
      settingsStatic("ENTER", "SETTINGS", "", "", 2);
      break;
    case THEME:
      settingsStatic("CHANGE", "THEME", "", "", 2);
      break;
    case THEME_DARK:
      settingsStatic("DARK", "WHITE", "", "", 2);
      break;
    case THEME_WHITE:
      settingsStatic("DARK", "WHITE", "", "", 2);
      break;
    case CIRC:
      settingsStatic("CHANGE", "WHEEL", "DIAMETER", "", 2);
      break;
    case CIRC_26:
      settingsStatic("26", "27", "28", "29", 2);
      break;
    case CIRC_27:
      settingsStatic("26", "27", "28", "29", 2);
      break;
    case CIRC_28:
      settingsStatic("26", "27", "28", "29", 2);
      break;
    case CIRC_29:
      settingsStatic("26", "27", "28", "29", 2);
      break;
    case RESET:
      settingsStatic("RESET", "ALL", "", "", 2);
      break;
    case BACK:
      settingsStatic("BACK", "", "", "", 2);
      break;
  }
}

void displayMenuDynamic(MenuState state) {
  switch (state) {
    case TIME:
      menuDynamic(5, 13, 126, 13, 10, 70, 5, 130, 125, 130);
      break;
    case AVG:
      menuDynamic(5, 13, 126, 13, 70, 140, 5, 130, 96, 70);
      break;
    case THEME:
    case CIRC:
    case RESET:
    case BACK:
      drawImage(gear_icon, gearW, gearH, draw_gear_icon, 4);
      break;
    case THEME_DARK:
    case THEME_WHITE:
      drawImage(gear_icon, gearW, gearH, draw_gear_icon, 4);
      settingsDynamic(state == THEME_DARK ? 1 : 2);
      break;
    case CIRC_26:
    case CIRC_27:
    case CIRC_28:
    case CIRC_29:
      drawImage(gear_icon, gearW, gearH, draw_gear_icon, 4);
      settingsDynamic((state - CIRC_26) + 1);
      break;
  }
}

void handleLeftButton() {
  switch (currentMenu) {
    case THEME_WHITE:
      darkTheme = true;
      changeTheme();
      currentMenu = THEME_DARK;
      break;

    case CIRC_27:
      currentMenu = CIRC_26;
      changeDiameter(currentMenu);
      break;

    case CIRC_28:
      currentMenu = CIRC_27;
      changeDiameter(currentMenu);
      break;

    case CIRC_29:
      currentMenu = CIRC_28;
      changeDiameter(currentMenu);
      break;

    case THEME_DARK:
    case CIRC_26:
      break;

    default:
      previousMenu();
      break;
  }

  displayMenuStatic(currentMenu);
}

void handleRightButton() {
  switch (currentMenu) {
    case THEME_DARK:
      darkTheme = false;
      changeTheme();
      currentMenu = THEME_WHITE;
      break;

    case CIRC_26:
      currentMenu = CIRC_27;
      changeDiameter(currentMenu);
      break;

    case CIRC_27:
      currentMenu = CIRC_28;
      changeDiameter(currentMenu);
      break;

    case CIRC_28:
      currentMenu = CIRC_29;
      changeDiameter(currentMenu);
      break;

    case THEME_WHITE:
    case CIRC_29:
      // Do nothing
      break;

    default:
      nextMenu();
      break;
  }

  displayMenuStatic(currentMenu);
}

void handleMiddleButton() {
  if (currentMenu == SETTINGS) {
    enter_settings = true;
    nextMenu();
  } else if (currentMenu == THEME) {
    currentMenu = darkTheme ? THEME_DARK : THEME_WHITE;
  } else if (currentMenu == THEME_DARK || currentMenu == THEME_WHITE) {
    currentMenu = THEME;
  } else if (currentMenu == CIRC) {
    switch (int(currentDiameter)) {
      case 66: currentMenu = CIRC_26; break;
      case 69: currentMenu = CIRC_27; break;
      case 71: currentMenu = CIRC_28; break;
      case 73: currentMenu = CIRC_29; break;
    }
  } else if (currentMenu == CIRC_26 || currentMenu == CIRC_27 || currentMenu == CIRC_28 || currentMenu == CIRC_29) {
    currentMenu = CIRC;
  } else if (currentMenu == RESET) {
    resetAll();
    enter_settings = false;
    currentMenu = TIME;
  } else if (currentMenu == BACK) {
    enter_settings = false;
    currentMenu = TIME;
  }

  displayMenuStatic(currentMenu);
}

void changeMenu() {
  if (buttonLeftPressed) {
    buttonLeftPressed = false;
    handleLeftButton();
  }else if (buttonRightPressed) {
    buttonRightPressed = false;
    handleRightButton();
  }else if (buttonMidPressed) {
    buttonMidPressed = false;
    handleMiddleButton();
  }
}

void resetAll() {
  fMaxSpeed = 0;
  fElapsedTotalTime = 0;
  fAvgSpeed = 0;
  fTotalDistanceKm = 0;
  distanceHundredKm = 0;
  fElapsedTotalTime = 0;
  darkTheme = true;
}

void calculate() {
  if (counter > 0 && counter <= 20) {

    fElapsedTime = millis() - fStartTime;
    fElapsedTotalTime += fElapsedTime;
    fTakenDistanceCm = 2.0 * PI * (currentDiameter / 2.0) * float(counter);
    fTotalDistanceKm += (fTakenDistanceCm / 100000.0);
    fSpeed = (fTakenDistanceCm / float(fElapsedTime)) * 36;
    fMaxSpeed = max(fMaxSpeed, fSpeed);
    fElapsedTotalTime += fElapsedTime;
    fAvgSpeed = fTotalDistanceKm / (fElapsedTotalTime / 3600000.0);

  } else {
    fSpeed = 0.0;  // Set speed value zero if there is no sensor input
  }

  counter = 0;
  fStartTime = millis();
}

void eepromRead() {
  float temp;
  uint8_t themeTemp;

  EEPROM.get(eeMaxSpeedAddress, temp);
  if (temp != 0 && !isnan(temp)) fMaxSpeed = temp;

  EEPROM.get(eeAvgSpeedAddress, temp);
  if (temp != 0 && !isnan(temp)) fAvgSpeed = temp;

  EEPROM.get(eeDistAddress, temp);
  if (temp != 0 && !isnan(temp)) fTotalDistanceKm = temp;

  EEPROM.get(eeDistHundredAddress, temp);
  if (temp != 0 && !isnan(temp)) distanceHundredKm = temp;

  EEPROM.get(eeElapsedTotalTimeAddress, temp);
  if (temp != 0 && !isnan(temp)) fElapsedTotalTime = temp;

  EEPROM.get(eeDarkThemeAddress, themeTemp);
  if (themeTemp != 0xFF) darkTheme = themeTemp;

  EEPROM.get(eeDiameterAddress, temp);
  if (temp != 0 && !isnan(temp)) currentDiameter = temp;
}

void eepromWrite() {
  float data;
  uint8_t themeTemp;

  EEPROM.get(eeMaxSpeedAddress, data);
  if (data != fMaxSpeed) EEPROM.put(eeMaxSpeedAddress, fMaxSpeed);

  EEPROM.get(eeAvgSpeedAddress, data);
  if (data != fAvgSpeed) EEPROM.put(eeAvgSpeedAddress, fAvgSpeed);

  EEPROM.get(eeDistAddress, data);
  if (data != fTotalDistanceKm) EEPROM.put(eeDistAddress, fTotalDistanceKm);

  EEPROM.get(eeDistHundredAddress, data);
  if (data != distanceHundredKm) EEPROM.put(eeDistHundredAddress, distanceHundredKm);

  EEPROM.get(eeElapsedTotalTimeAddress, data);
  if (data != fElapsedTotalTime) EEPROM.put(eeElapsedTotalTimeAddress, fElapsedTotalTime);

  EEPROM.get(eeDarkThemeAddress, themeTemp);
  if (themeTemp != darkTheme) EEPROM.put(eeDarkThemeAddress, darkTheme);

  EEPROM.get(eeDiameterAddress, data);
  if (data != currentDiameter) EEPROM.put(eeDiameterAddress, currentDiameter);

  EEPROM.commit();
}

void drawFloat(float value, int digits, int x, int y, int datum = TL_DATUM) {
  int padding = tft.textWidth("99.9", txtFont);
  tft.setTextPadding(padding);
  tft.setTextDatum(datum);
  tft.drawFloat(value, digits, x, y, txtFont);
}

void drawInt(int value, int x, int y, int datum = TR_DATUM, const char* paddingText = "999") {
  int padding = tft.textWidth(paddingText, txtFont);
  tft.setTextPadding(padding);
  tft.setTextDatum(datum);
  tft.drawNumber(value, x, y, txtFont);
}

void drawTime(int x, int y) {
  unsigned long totalSeconds = fElapsedTotalTime / 1000;
  unsigned long hrs = totalSeconds / 3600;
  unsigned long mins = (totalSeconds % 3600) / 60;
  unsigned long secs = totalSeconds % 60;

  char timeStr[9];
  sprintf(timeStr, "%02lu:%02lu:%02lu", hrs, mins, secs);

  int padding = tft.textWidth("99.99.99", txtFont);
  tft.setTextPadding(padding);
  tft.setTextDatum(TL_DATUM);
  tft.drawString(timeStr, x, y, txtFont);
}

void menuDynamic(int spd_x, int spd_y, int max_x, int max_y, int time_x, int time_y, int dist_x, int dist_y, int avg_x, int avg_y) {
  tft.setTextColor(textColor, backgroundColor);
  tft.setTextSize(2);
  tft.setTextDatum(TL_DATUM);

  drawFloat(fSpeed, 1, spd_x, spd_y);

  if (fTotalDistanceKm >= 100.0) {
    distanceHundredKm += static_cast<int>(fTotalDistanceKm / 100.0);
    fTotalDistanceKm = fmod(fTotalDistanceKm, 100.0);
  }

  if (distanceHundredKm > 0) {
    tft.setTextSize(1);
    drawInt(distanceHundredKm, (tft.width() / 2) - 2, dist_y - 14);
    tft.setTextSize(2);
  }

  drawFloat(fTotalDistanceKm, 1, dist_x, dist_y);
  drawFloat(fMaxSpeed, 1, max_x, max_y, TR_DATUM);
  drawFloat(fAvgSpeed, 1, avg_x, avg_y, TR_DATUM);

  // Menüye göre saat yazı boyutu
  tft.setTextSize((currentMenu == TIME) ? 2 : 1);

  drawTime(time_x, time_y);
}

void menuStatic(int spd_x, int spd_y, int max_x, int max_y, int time_x, int time_y, int dist_x, int dist_y, int avg_x, int avg_y) {

  tft.setTextDatum(TL_DATUM);

  // Write static text to screen
  //-----------------------------------------------------
  tft.setTextSize(1);

  tft.drawFastHLine(0, 45, tft.width(), TFT_ORANGE);
  tft.drawFastVLine(64, 0, 45, TFT_ORANGE);
  tft.drawFastHLine(0, 115, tft.width(), TFT_ORANGE);
  tft.drawFastVLine(64, 115, 45, TFT_ORANGE);

  tft.setTextColor(TFT_MAGENTA);

  tft.drawString("TIME", time_x, time_y, txtFont);
  tft.drawString("SPD", spd_x, spd_y, txtFont);
  tft.drawString("DIST", dist_x, dist_y, txtFont);
  tft.drawString("MAX", max_x, max_y, txtFont);
  tft.drawString("AVG", avg_x, avg_y, txtFont);

  //-----------------------------------------------------
  //-----------------------------------------------------
}

void settingsDynamic(int selected) {
  tft.setTextSize(2);
  tft.setTextDatum(MC_DATUM);

  int step = -10;

  for (int i = 0; i < selected; i++) {
    step += 30;
  }

  tft.drawRect(3, step, tft.width() - 6, 30, TFT_ORANGE);
}

void settingsStatic(String text_1, String text_2, String text_3, String text_4, byte textSize) {

  tft.setTextSize(textSize);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(textColor, backgroundColor);

  if (!text_1.isEmpty()) {
    tft.drawString(text_1, tft.width() / 2, 35, txtFont);
  }

  if (!text_2.isEmpty()) {
    tft.drawString(text_2, tft.width() / 2, 65, txtFont);
  }

  if (!text_3.isEmpty()) {
    tft.drawString(text_3, tft.width() / 2, 95, txtFont);
  }

  if (!text_4.isEmpty()) {
    tft.drawString(text_4, tft.width() / 2, 125, txtFont);
  }

  draw_gear_icon = false;
  drawImage(gear_icon, gearW, gearH, draw_gear_icon, 4);

  lastShowGearChange = millis();
}

void fakeTrigger() {
  if (counter <= 10) {
    int randomValue = random(0, 4);  // 0 dahil, 6 hariç => 0 ile 5 arasında
    counter += randomValue;
  }
}

void setup() {

  Serial.begin(115200);  // Initialize Serial connection
  EEPROM.begin(EEPROM_SIZE);

  pinMode(HallEffectSensor, INPUT_PULLUP);  // Set pin mode for hall effect sensor
  pinMode(buttonRight, INPUT_PULLUP);       // Set pin mode for right button
  pinMode(buttonLeft, INPUT_PULLUP);        // Set pin mode for left button
  pinMode(buttonMid, INPUT_PULLUP);         // Set pin mode for middle button
  delay(100);

  eepromRead();

  changeTheme();

  tft.init();
  tft.fillScreen(backgroundColor);            // Clear the screen
  tft.setRotation(2);                         // Set screen rotation. (normal: 0, upside down: 2)
  Serial.println("TFT Screen Initialized!");  // Write "TFT Screen Initialized!"
  delay(50);

  attachInterrupt(HallEffectSensor, hallInterrupt, FALLING);
  attachInterrupt(buttonLeft, buttonInterrupt, FALLING);
  attachInterrupt(buttonMid, buttonInterrupt, FALLING);
  attachInterrupt(buttonRight, buttonInterrupt, FALLING);

  displayMenuStatic(currentMenu);
  displayMenuDynamic(currentMenu);
}

void loop() {
  if (debug) {
    fakeTrigger();
  }

  if (millis() - fStartTime >= waitUntil) {
    calculate();
  }

  eepromWrite();

  changeMenu();

  displayMenuDynamic(currentMenu);

  delay(150);
}
