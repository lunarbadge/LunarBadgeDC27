/*****************************************************************************
 * Made in cloudy Seattle
 *
 * (C) Copyright 2019 @LunarBadge (https://lunarbadge.github.io/)
 *
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * If you use this code or extend it, please let us know!
 *
 * Contributors:
 *  @nofullname
 * 
 *****************************************************************************/

#include "lander.h"
#include "lunarlanderGame.h"
#include "CommanderGame.h"
#include "FriendsApp.h"
#include "SettingsApp.h"
#include "hardware.h"
#include "BleLander.h"
#include "idler.h"

// game play
#define RESTART_DELAY 1000
// some things on the screen blink; this defines how frequently.
#define GAME_ICONS_FLASH_INTERVAL  500
#define IDLE_TIMEOUT 60000


unsigned long now;
unsigned long lastBlink = 0;
bool blinkOn = false;
unsigned short brightness = 128;

unsigned long pauseUntilTime;
unsigned long lastUpdate = 0;
unsigned long nextBatterySample = 0;
unsigned long nextFlicker = 0;
unsigned long nextOn = 0;
unsigned long lastInteraction = 0;

float batteryLevel;

bool welcomeIssued = false;

typedef enum 
{
    menuSelectionPlay = 0,
    menuSelectionCommander,
    menusSelectionSettings,
    menuSelectionFriends,
    menuSelectionCredits,
    menuSelectionLast
} menuItems;

const char *menuChoices[menuSelectionLast] = {"Lander", "Commander", "Settings", "Friends", "Credits"};
menuItems menuSelection = menuSelectionPlay;
typedef enum {
    gameStateStartingUp,
    gameStatePaused,
    gameStatePlaying,
    gameStateCredits,
    gameStateMainMenu,
    gameStateReadyToReturnToMenu, 
    gameStateIdle } GameState;

ColorMap defaultColorMap { ILI9341_BLACK, ILI9341_CYAN, ILI9341_YELLOW, ILI9341_ORANGE, ILI9341_RED, ILI9341_WHITE, ILI9341_YELLOW, ILI9341_GREEN };


GameState currentGameState = gameStateStartingUp;

LunarLanderGame theOriginal(defaultColorMap);
CommanderGame theCommandModuleGame(defaultColorMap);
SettingsApp theSettings(defaultColorMap);
FriendsApp  myFriends(defaultColorMap);
BleLander   btLander;

App *currentGame = nullptr;

bool displayedStats = false;        // don't redisplay when paused.

// AnalogWrite is not supported on the ESP32 so we need these values for the low-level commands
// to adjust the screen brightness or do anything else with analog output.
const int freq = 5000;
int screenBrightnessChannel = 0;
int resolution = 8;
int cabinLightChannel = 1;

ColorMap currentColorMap;
 
const std::vector<const char *> credits =
{
    "@RowanPhipps  - hardware design",
    "@NoFullName17 - art & software",
    "@GPhipps      - lander trace",
};

const std::vector<const char *> backers =
{
    "Ciph3r",
    "Martin Lebel",
    "Shagghie Highsage",
    "Tikidood",
};

void setup()
{
    Serial.begin(9600);
    delay(500);
    DebugPrintln("setup"); 

    tft.begin();

    // initialize pins / buttons
    
    pinMode(LITE_PIN, OUTPUT);
    ledcSetup(screenBrightnessChannel, freq, resolution);
    ledcAttachPin(LITE_PIN, screenBrightnessChannel);


    for (auto bde : buttonDebouncer) 
    {
        auto bd = bde.second;
        pinMode(bd.buttonName, INPUT);
        bd.lastDebounceTime = 0;
        bd.lastClickedTime = 0;
        bd.lastRepeatTime = 0;
        bd.lastState = 1;
        bd.state = 1;
    }

    nextBatterySample = 0;

    if (CABIN_LIGHTS   != 0)
    {
        pinMode(CABIN_LIGHTS, OUTPUT);
        ledcSetup(cabinLightChannel, freq, resolution);
        ledcAttachPin(CABIN_LIGHTS, cabinLightChannel);
    }

    if (ENGINE_LED_1   != 0)
    {
        pinMode(ENGINE_LED_1, OUTPUT);
    }

    if (ENGINE_LED_2   != 0)
    {
        pinMode(ENGINE_LED_2, OUTPUT);
    }

    if (ENGINE_LED_3   != 0)
    {
        pinMode(ENGINE_LED_3, OUTPUT);
    }
    
     // read diagnostics (optional but can help debug problems)
#ifndef PRODUCTION
    uint8_t x = tft.readcommand8(ILI9341_RDMODE);
    Serial.print("Display Power Mode: 0x"); Serial.println(x, HEX);
    x = tft.readcommand8(ILI9341_RDMADCTL);
    Serial.print("MADCTL Mode: 0x"); Serial.println(x, HEX);
    x = tft.readcommand8(ILI9341_RDPIXFMT);
    Serial.print("Pixel Format: 0x"); Serial.println(x, HEX);
    x = tft.readcommand8(ILI9341_RDIMGFMT);
    Serial.print("Image Format: 0x"); Serial.println(x, HEX);
    x = tft.readcommand8(ILI9341_RDSELFDIAG);
    Serial.print("Self Diagnostic: 0x"); Serial.println(x, HEX); 
#endif
    
    // set to landscape mode.
    tft.setRotation(1);
    
    // should get this from settings.
    

    // must init these after the esp has finished starting so reading Preferences works.
    theOriginal.init();
    theCommandModuleGame.init();
    theSettings.init();
    DebugPrintln("initialized");
    DebugPrintln("initializing bluetooth");
    btLander.init(theSettings.playerName(), theOriginal.gameScore());
    btLander.setEnabled(theSettings.isBluetoothEnabled());
    DebugPrintln("initializing friends");
    myFriends.init(&btLander);
    DebugPrintln("Done");
    Idler::getInstance()->init();

    brightness = theSettings.getBrightness();   // max 255
    ledcWrite(screenBrightnessChannel, brightness);
    lastInteraction = millis();
    delay(100);

    currentColorMap = theSettings.getColorMap();
    drawHelp();
    Idler::getInstance()->setIdle(true);
}

void drawHelp()
{
    // 'csm_buttons', 272x109px
    tft.fillScreen(currentColorMap.ColorBackground);
    tft.drawBitmap(24, 65, badgeBitmap, 275, 112, currentColorMap.ColorLanderTrace);
   
    tft.setFont();
    tft.setTextSize(2);
    tft.setTextColor(currentColorMap.ColorTitle, currentColorMap.ColorBackground); 
    tft.setCursor(15, 5);
    tft.print("Welcome to Lunar Lander!");
    tft.fillRect(241, 65, 80, TEXT_SPACING+3, currentColorMap.ColorBackground);
    tft.setCursor(245, 68);
    tft.print("R");
    
    // use triangles instead, way simpler?  -- redo ---
    tft.drawBitmap(51, 122, upArrowBitmap, 10, 13, currentColorMap.ColorTitle);
    tft.drawBitmap(61, 138, downArrowBitmap, 10, 13, currentColorMap.ColorTitle);

    tft.setTextColor(currentColorMap.ColorWin, currentColorMap.ColorBackground);
    tft.setCursor(40, 220);
    tft.print("Any button to start");
}

void changeMenuSelection(menuItems selection, uint16_t foreground, uint16_t background)
{
    tft.setTextSize(2);
    tft.setCursor(2, (int(selection) + 1) * TEXT_SPACING * 2);
    tft.setTextColor(foreground, background);
    tft.print(menuChoices[selection]);
    batteryWarning();
}


void clearMenuSelection(menuItems prevItem)
{
    changeMenuSelection(prevItem, currentColorMap.ColorForeground, currentColorMap.ColorBackground);
}

void setMenuSelection(menuItems newSelection)
{
    changeMenuSelection(newSelection, currentColorMap.ColorBackground, currentColorMap.ColorForeground);
    drawGameStats();
    batteryWarning();
}


void drawGameStats()
{
    // print statistics for selected game
    tft.setCursor(2, 231);
    tft.setTextSize(1);
    tft.setTextColor(currentColorMap.ColorForeground, currentColorMap.ColorBackground); 

    switch (menuSelection) 
    {
        case menuSelectionPlay:
        {
            short wins, played, score;

            wins = theOriginal.gamesWon();
            played = theOriginal.gamesPlayed(); 
            score = theOriginal.gameScore();
        
            tft.printf("Completed %u games; won %u (%u%%), score %u",
                played, wins,
                played == 0 ? 0 : played == 0 ? 0 : wins * 100 / played, score);
            break;
        }
        case menuSelectionCommander:
        {
            short wins, played;
            
            wins = theCommandModuleGame.gamesWon();
            played = theCommandModuleGame.gamesPlayed(); 
        
            tft.printf("Completed %u games; won %u (%u%%)",
                played, wins,
                played == 0 ? 0 : played == 0 ? 0 : wins * 100 / played);
            break;
        }
        default:
        {
            tft.fillRect(0, 231, 319, 239, currentColorMap.ColorBackground);
            break;
        }
    }   
}

void drawMenu(bool displayWelcome)
{
    if (displayWelcome)
    {
        Serial.println("Welcome to lunar lander");
    }
    tft.fillScreen(currentColorMap.ColorBackground);

    tft.setCursor(8, 2);
    tft.setFont();  // standard font
    tft.setTextSize(2);
    tft.setTextColor(currentColorMap.ColorTitle, currentColorMap.ColorBackground); 
    const char * playerName = theSettings.playerName();
    if (playerName[0] == '\0')
    {
        tft.print("Pick a name");
    }
    else
    {
        tft.printf("Hi %s", playerName);
    }

    for (auto i = menuSelectionPlay; i < menuSelectionLast; i = menuItems((int)i+1))
    {
        if (i == menuSelection)
        {
            tft.setTextColor(currentColorMap.ColorBackground, currentColorMap.ColorForeground);
        }
        else
        {
            tft.setTextColor(currentColorMap.ColorForeground, currentColorMap.ColorBackground); 
        }
        tft.setCursor(2,  (int(i) + 1) * TEXT_SPACING * 2);
        tft.print(menuChoices[i]);
    }
    tft.drawBitmap(122, 70, landerBitmap, 196, 158, currentColorMap.ColorLanderTrace);

    drawGameStats();
    batteryWarning();
}

void batteryWarning()
{
    tft.setCursor(272, 231);
    tft.setTextSize(1);
    tft.setTextColor(currentColorMap.ColorBurn, currentColorMap.ColorBackground); 
    tft.printf("bat: %.1f", batteryLevel);
}

void batteryError()
{
    tft.setTextSize(2);
    tft.fillScreen(currentColorMap.ColorTitle);
    tft.setTextColor(currentColorMap.ColorBackground, currentColorMap.ColorTitle); 
    tft.setCursor(30, 110);
    tft.printf("Low battery: stopping");
    tft.setCursor(100, 150);
    tft.printf("Vbat: %.1f", batteryLevel);
    delay(5000);
    // power stuff down; will wake on buttons 1 / 2.
    esp_sleep_enable_ext1_wakeup(0xA000,ESP_EXT1_WAKEUP_ALL_LOW);
    esp_deep_sleep_start();
}

void showCredits()
{
    int16_t  x1, y1;
    uint16_t w, h;
    tft.fillScreen(currentColorMap.ColorBackground);
    tft.setFont(&FreeSans9pt7b); 
    tft.setTextColor(currentColorMap.ColorTitle, currentColorMap.ColorBackground); 
    tft.getTextBounds("Credits", 0, 56, &x1, &y1, &w, &h);
    Serial.printf("credits at (%d, %d), size %u x %u\r\n", x1, y1, w, h);
    // credits at (1, 44), size 56 x 13
    tft.setCursor(2, 15);
    tft.printf("Credits");
    tft.setTextColor(currentColorMap.ColorForeground, currentColorMap.ColorBackground); 
    tft.setFont(&FreeSans9pt7b); 
    tft.setTextSize(1);
    int i = 0;
    int vertical = 38;
    for (auto c : credits)
    {
        tft.setCursor(2, vertical);
        vertical += 24;
        tft.printf("%s\r\n", c);
        Serial.println(c);
    }
    vertical += 10;
    tft.setCursor(2,  vertical);
    tft.setTextColor(currentColorMap.ColorTitle, currentColorMap.ColorBackground);
    tft.printf("Odyssey backers:");
    Serial.println("Odyssey backers:");
    tft.setTextColor(currentColorMap.ColorForeground, currentColorMap.ColorBackground); 
    vertical += 30;
    for (auto c : backers)
    {
        tft.setCursor(10, vertical);
        vertical += 24;
        tft.printf("%s\r\n", c);
        Serial.println(c);
    }
    tft.setFont();
}

void loop()
{
    now = millis();

    if (BATTERY_LEVEL_PIN != 0 && now > nextBatterySample)
    {
        nextBatterySample = now + BATTERY_SAMPLE_DELAY;
        batteryLevel = (analogRead(BATTERY_LEVEL_PIN)/4095.0)*2.0*3.3*1.05;
        
        if (gameStateMainMenu == currentGameState || gameStateStartingUp == currentGameState)
        {
            Serial.print("VBat: " );
            Serial.println(batteryLevel);
        }
        // Deep sleep to protect battery.
        if (batteryLevel <= 3.2)
        {
            batteryError();
            return; // not reached
        }
    }

    if (theSettings.isBluetoothEnabled())
    {
        btLander.doLoop(now);
    }
    
    switch (currentGameState)
    {
        case gameStateStartingUp:
        {
            // pause until we are ready
            if (anyButtonPressed()) {
                currentGameState = gameStateMainMenu;
                drawMenu(true);
                Idler::getInstance()->setIdle(true);
            }
            break;
        }
        case gameStateMainMenu:
        {
            // shell commands
            if (debounceStateChanged(buttonDebouncer[restartButton])
                || debounceStateChanged(buttonDebouncer[buttonD])
                || debounceStateChanged(buttonDebouncer[buttonC]))
            {
                lastInteraction = now;
                switch (menuSelection)
                {
                case menuSelectionPlay:
                    tft.fillScreen(currentColorMap.ColorBackground);
                    DebugPrintln("playing");
                    // Redraw all

                    currentGameState = gameStatePlaying;
                    currentGame = &theOriginal;
                    currentGame->start(now, currentColorMap);
                    pauseUntilTime = now + RESTART_DELAY;
                    lastUpdate = now;
                    Idler::getInstance()->setIdle(false);
                    break;

                case menuSelectionCommander:
                    tft.fillScreen(currentColorMap.ColorBackground);
                    // Redraw all
                    
                    currentGameState = gameStatePlaying;
                    currentGame = &theCommandModuleGame;
                    currentGame->start(now, currentColorMap);
                    pauseUntilTime = now + RESTART_DELAY;
                    lastUpdate = now;
                    break;

                case menuSelectionFriends:
                    tft.fillScreen(currentColorMap.ColorBackground);
                    currentGameState = gameStatePlaying;
                    myFriends.setBluetoothEnabled(theSettings.isBluetoothEnabled());
                    currentGame = &myFriends;
                    currentGame->start(now, currentColorMap);
                    pauseUntilTime = now + RESTART_DELAY;
                    lastUpdate = now;
                    break;

                case menusSelectionSettings:
                    tft.fillScreen(currentColorMap.ColorBackground);
                    currentGameState = gameStatePlaying;
                    currentGame = &theSettings;
                    currentGame->start(now, currentColorMap);
                    pauseUntilTime = now + RESTART_DELAY;
                    lastUpdate = now;
                    break;

                case menuSelectionCredits:
                    DebugPrintln("rolling credits");
                    currentGameState = gameStateCredits;
                    pauseUntilTime = now + RESTART_DELAY;
                    showCredits();
                    lastUpdate = now;
                    break;
                    
                default:
                    DebugPrintln("no idea why I am here!");
                    break;
                }
            }
            
            if (debounceStateChanged(buttonDebouncer[buttonA]))
            {
                lastInteraction = now;
                if (menuSelection > 0) 
                {
                    clearMenuSelection(menuSelection);
                    menuSelection = menuItems((int)menuSelection - 1);
                    setMenuSelection(menuSelection);
                }
            }
        
            if (debounceStateChanged(buttonDebouncer[buttonB]))
            {
                lastInteraction = now;
                if (menuSelection < menuSelectionLast - 1)
                {
                    clearMenuSelection(menuSelection);
                    menuSelection = menuItems((int)menuSelection + 1);
                    setMenuSelection(menuSelection);
                }
            }
          
            break;
        }

        case gameStateIdle:
        {
            if (anyButtonPressed())
            {
                lastInteraction = now;
                currentGameState = gameStateMainMenu;
                ledcWrite(screenBrightnessChannel, brightness);
                drawMenu(true);
                Idler::getInstance()->setIdle(false);
            }
            break;
        }

        case gameStateReadyToReturnToMenu:
        {
            if (pauseUntilTime < now && anyButtonPressed())
            {
                lastInteraction = now;
                currentGameState = gameStateMainMenu;
                drawMenu(true);
            }
            break;
        }
    
        case gameStatePaused:
        {
            if (pauseUntilTime < now)
            {
                short wins, played;
                wins = currentGame->gamesWon();
                played = currentGame->gamesPlayed(); 

                DebugPrintln("paused; displaying stats");
                tft.setCursor(8, SCREEN_HEIGHT/2 - TEXT_SPACING);
                tft.setTextColor(currentColorMap.ColorForeground, currentColorMap.ColorBackground);  tft.setTextSize(2);
                tft.println("Press Select to go back");
                tft.setCursor(8, SCREEN_HEIGHT/2 + TEXT_SPACING);
                tft.printf("Played %u, won %u (%u%%)", 
                    played, wins,
                    played == 0 ? 0 : wins * 100 / played);

                currentGameState = gameStateReadyToReturnToMenu;
                pauseUntilTime = now  + RESTART_DELAY * 2;
                Idler::getInstance()->setIdle(true);
            }
            break;
        }

        case gameStatePlaying:
        {
            // blink the screen artifacts that need to blink.
            if (now - lastBlink > GAME_ICONS_FLASH_INTERVAL)
            {
                currentGame->blinkAction(blinkOn);
                lastBlink = now;
                blinkOn = !blinkOn;
            }

            for (ButtonKeys i = restartButton; i <= buttonD; i = ButtonKeys((int)i+1))
            {
                if (debounceStateChanged(buttonDebouncer[i]))
                {
                    lastInteraction = now;
                    currentGame->buttonAction(now, i);
                }
            }

            // game step
            if (now - lastUpdate > MILLIS_PER_FRAME)
            {
                if (currentGame->updateIsComplete(now)) 
                {
                    // check to see if settings changed
                    if (currentGame == &theSettings)
                    {
                        if (theSettings.isBluetoothEnabled() != btLander.isEnabled())
                        {
                            DebugPrintln("we are enabling bluetooth now");
                            btLander.setEnabled(theSettings.isBluetoothEnabled());
                        }
                        currentColorMap = theSettings.getColorMap();
                    }
                    else if (currentGame == &theOriginal)
                    {
                        btLander.setScore(theOriginal.gameScore());
                    }

                    if (currentGame->isStopped(now))
                    {
                        currentGameState = gameStatePaused;
                        pauseUntilTime = now + RESTART_DELAY;
                    }
                    else
                    {
                        currentGameState = gameStateMainMenu;
                        drawMenu(true);
                    }
                    Idler::getInstance()->setIdle(true);  
                }
                lastUpdate = now;
            }
            break;
        }

        case gameStateCredits:
        {
            if (pauseUntilTime < now)
            {
                currentGameState = gameStateReadyToReturnToMenu;
            }
            break;
        }

        default:
            DebugPrintln("undefined state!");
            break;
    }

    if (now > nextFlicker)
    {
        ledcWrite(cabinLightChannel, 50);
        nextFlicker = now + 1400 + random(800);
    }
    else if (now > nextOn)
    {
        ledcWrite(cabinLightChannel, 127);
        nextOn = now + random(100) + 50;
    }

    if (lastInteraction + IDLE_TIMEOUT < now)
    {
        ledcWrite(screenBrightnessChannel, 0);
        currentGameState = gameStateIdle;
    }
}


