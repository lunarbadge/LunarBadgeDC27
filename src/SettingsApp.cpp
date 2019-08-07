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
 * 	@nofullname
 * 
 *****************************************************************************/

#include "lander.h"
#include "hardware.h"
#include "SettingsApp.h"

#pragma once

#define DEFCON27_PINK	0xcb77
#define DEFCON27_BLUE	0x5cbc
#define DEFCON27_YELLOW	0xff6f
#define DEFCON27_MAUVE	0x9c39

#define LCARS_ORANGE  0xf630
#define LCARS_BROWN   0xbb0b
#define LCARS_MAUVE   0x9cff
#define LCARS_PINK    0xc4f9

#if 0
typedef struct ColorMap {
	uint16_t ColorBackground;
	uint16_t ColorOrbiter; //cyan
	uint16_t ColorLander; // yellow
	uint16_t ColorExplosion; // orange
	uint16_t ColorBurn; // red
	uint16_t ColorForeground; // white
	uint16_t ColorTitle; // yellow
	uint16_t ColorWin; // green
	uint16_t ColorLanderTrace; // 
} ColorMap;
#endif

const std::vector<ColorMap> SettingsApp::colorSchemes = {
	{ ILI9341_BLACK, ILI9341_CYAN, ILI9341_YELLOW, ILI9341_ORANGE, ILI9341_RED, ILI9341_WHITE, ILI9341_YELLOW, ILI9341_GREEN, ILI9341_WHITE },	// classic
	{ ILI9341_BLACK, DEFCON27_BLUE, DEFCON27_YELLOW, ILI9341_ORANGE, ILI9341_RED, ILI9341_WHITE, DEFCON27_YELLOW, DEFCON27_MAUVE,  DEFCON27_BLUE},
	{ ILI9341_BLACK, DEFCON27_PINK, DEFCON27_MAUVE, ILI9341_YELLOW, ILI9341_BLUE, DEFCON27_PINK, DEFCON27_YELLOW, DEFCON27_MAUVE,  DEFCON27_YELLOW},
	{ ILI9341_BLACK, LCARS_PINK, LCARS_MAUVE, LCARS_PINK, LCARS_ORANGE, LCARS_PINK, LCARS_ORANGE, LCARS_ORANGE,  LCARS_ORANGE},
	{ DEFCON27_PINK, DEFCON27_BLUE, DEFCON27_YELLOW, ILI9341_ORANGE, ILI9341_RED, ILI9341_BLACK, DEFCON27_YELLOW, ILI9341_WHITE, ILI9341_WHITE },
	{ DEFCON27_YELLOW, DEFCON27_BLUE, DEFCON27_PINK, ILI9341_ORANGE, ILI9341_RED, ILI9341_BLACK, DEFCON27_PINK, ILI9341_WHITE,  DEFCON27_BLUE,},
};

// update the state of the app (screen, etc) and return true when the game is over.
bool SettingsApp::updateIsComplete(unsigned long now)
{
	if (selectedSetting == gameSettingsName)
	{
		bool endofline = false;
		while (Serial.available() && !endofline  && strlen(newName) < NAME_LEN)
		{
			char byteRead = Serial.read();
			switch (byteRead)
			{
				case '\b':
					// backspace; overwrite last character.  
					if (strlen(newName) > 0)
					{
						newName[strlen(newName) - 1] = ' ';
						Serial.printf("\r%s%s", salutation.c_str(), newName);
						newName[strlen(newName) - 1] = '\0';
						Serial.printf("\r%s%s", salutation.c_str(), newName);
					}
					break;
				case '\n':
				case '\r':
				{
					// new line means we are done
					endofline = true;
					break;
				}
				default:
				{
					Serial.print((char)byteRead);
					newName[strlen(newName)] = (char)byteRead;
					if (strlen(newName) == NAME_LEN)
					{
						endofline = true;
					}
					Serial.flush();
					break;
				}
			}
			updateName(newName);	// actually want to print newName not myName
		}
		if (endofline)
		{
			strcpy(myName, newName);
			updateMenuSelection(colors.ColorBackground, colors.ColorForeground);
			Serial.printf("\r\nSetting name to %s\r\n", myName);
			updateMenuSelection(colors.ColorForeground, colors.ColorBackground);
			selectedSetting = settingsItems((int)selectedSetting + 1);
			updateMenuSelection(colors.ColorBackground, colors.ColorForeground);
		}
	}
	else if (cyclingColors != -1 && now > colorShift)
	{
		colorShift = now + COLOR_SHIFT_DURATION;
        if (cyclingColors == colorSchemes.size())
        {
        	cyclingColors = 0;
        }
        else
        {
        	cyclingColors++;
        }
        colors = colorSchemes[cyclingColors];
        drawSettingsMenu();
	}
	return finished;
}


// is the app paused?
bool SettingsApp::isStopped(unsigned long now)
{
	return false;
}

void SettingsApp::setBluetoothEnabled(bool enabled)
{
	btEnabled = enabled;
}	

void SettingsApp::init()
{
	Preferences preferences;
    preferences.begin("badgeSettings", true /* readonly */);
    int result = preferences.getString("name", myName, NAME_LEN);
    if (result != strlen(myName) + 1)
    {  
	    uint8_t    macID[8];
	    
	   	memset(macID, '\0', 8);
	    esp_efuse_mac_get_default(macID);

	    uint64_t badgeNumber = macID[1];
	    badgeNumber = badgeNumber << 8 | macID[5];
	    badgeNumber = badgeNumber << 8 | macID[3];
	    badgeNumber = badgeNumber << 8 | macID[0];
	    badgeNumber = badgeNumber << 8 | macID[2];
	    badgeNumber = badgeNumber << 8 | macID[4];
	   
	    sprintf(myName, "badge #%llx", badgeNumber);
    	Serial.printf("Oops; I don't know you, will call you %s\r\n", myName);
    }
    btEnabled = preferences.getBool("bluetoothOn", false);
    colorsUnlocked = preferences.getBool("cul", false);
    colorSchemeIndex = preferences.getUShort("color", 0);
    brightness = preferences.getUShort("brightness", MAX_BRIGHT);
    preferences.end();
    initialized = true;
}

void SettingsApp::updateBadgePreferences()
{
	Preferences preferences;
    preferences.begin("badgeSettings", false);
    
    size_t result = preferences.putString("name", myName);
    if (result != strlen(myName))
    {
    	DebugPrintln("could not write string");
    }
    preferences.putBool("bluetoothOn", btEnabled);
    preferences.putBool("cul", colorsUnlocked);
    preferences.putUShort("color", colorSchemeIndex);
    preferences.putUShort("brightness", brightness);
    preferences.end();
}

// start the app
void SettingsApp::start(unsigned long now, const ColorMap & c)
{
	colors = c;
	finished = false;
	selectedSetting = gameSettingsBrightness;
	drawSettingsMenu();
	settingBrightness = false;
	settingName = false;
	cyclingColors = -1;
	if (!colorsUnlocked)
	{
		h = readdch();
	}
}

// buttonA navigates up, buttonB navigates down in the menu.
// buttonC and buttonD navigate options with the selected setting.
void SettingsApp::buttonAction(unsigned long now, ButtonKeys button)
{
	if (cyclingColors != -1) {
		cyclingColors = -1;
		colors = colorSchemes[colorSchemeIndex];
	}
	switch (button)
	{
		case buttonA:
		{
			if (selectedSetting > 0) 
		    {
		    	updateMenuSelection(colors.ColorForeground, colors.ColorBackground);
		        selectedSetting = settingsItems((int)selectedSetting - 1);
		        updateMenuSelection(colors.ColorBackground, colors.ColorForeground);
		    }
		    break;
		}
		case buttonB:
		{
			if (selectedSetting < gameSettingsLast - 1)
			{
				settingName = false;
				updateMenuSelection(colors.ColorForeground, colors.ColorBackground);
				selectedSetting = settingsItems((int)selectedSetting + 1);
				updateMenuSelection(colors.ColorBackground, colors.ColorForeground);
			}
			break;
		}
		case buttonC:
		{
			if (gameSettingsBrightness == selectedSetting)
			{
				brightness = brightness > 0 ? brightness - 1 : 0;
				ledcWrite(screenBrightnessChannel, brightness);
				updateMenuSelection(colors.ColorBackground, colors.ColorForeground);			
			}
			else if (gameSettingsColors == selectedSetting)
			{
				if (!colorsUnlocked)
				{
					unlock(now);
				}
				else
				{
					if (colorSchemeIndex > 0)
					{
						colors = colorSchemes[--colorSchemeIndex];
					} 
					else
					{
						colorSchemeIndex = colorSchemes.size() - 1;
						colors = colorSchemes[colorSchemeIndex];
					}
					drawSettingsMenu();
				}
			}
			else if (gameSettingsBluetooth == selectedSetting)
			{
				// toggle bluetooth
				btEnabled = !btEnabled;
				setBluetoothEnabled(btEnabled);
				updateMenuSelection(colors.ColorBackground, colors.ColorForeground);
			}
			else if (gameSettingsDone == selectedSetting)
			{
				resetAction(now);
			}
			break;
		}
		case buttonD:
		{
			if (gameSettingsBrightness == selectedSetting)
			{
				brightness = brightness < 251 ? brightness + 1 : 255;
				ledcWrite(screenBrightnessChannel, brightness);
				updateMenuSelection(colors.ColorBackground, colors.ColorForeground);
			}
			else if (gameSettingsColors == selectedSetting)
			{
				if (!colorsUnlocked)
				{
					unlock(now);
				}
				else
				{
					if (colorSchemeIndex < colorSchemes.size() - 1)
					{
						colors = colorSchemes[++colorSchemeIndex];	
					}
					else
					{
						colors = colorSchemes[0];
						colorSchemeIndex = 0;
					}
					drawSettingsMenu();
				}				
			}
			else if (gameSettingsBluetooth == selectedSetting)
			{
				// toggle bluetooth
				btEnabled = !btEnabled;
				setBluetoothEnabled(btEnabled);
				updateMenuSelection(colors.ColorBackground, colors.ColorForeground);
			}
			else if (gameSettingsDone == selectedSetting)
			{
				resetAction(now);
			}
			break;
		}
		case restartButton:
		{
			resetAction(now);
			break;
		}
		default:
			break;
	}
}

void SettingsApp::resetAction(unsigned long now)
{
	DebugPrintf("reset; selected %d\r\n", selectedSetting);

	switch(selectedSetting)
	{
		case gameSettingsName:
		{
			if (settingName)
			{
				settingName = false;
				Serial.printf("Your name is %s \r\n", myName);
			}
			else
			{
				settingName = true;
				if (strlen(myName) == 0) {
					salutation = String("Choose a name: ");
					memset(newName, '\0', NAME_LEN);
				}
	        	else
	        	{
	        		salutation = String("Change your name (currently " + String(myName) + "): ");
	        		memset(newName, '\0', NAME_LEN);
	        	}
	        	Serial.printf("\r\n%s", salutation.c_str());
	        }
			break;
		}

		case gameSettingsBrightness:
		{
			// go into brightness settings mode; up/down, reset when done.
			// settingBrightness = !settingBrightness;
			updateMenuSelection(colors.ColorForeground, colors.ColorBackground);
			selectedSetting = gameSettingsDone;
			updateMenuSelection(colors.ColorBackground, colors.ColorForeground);
			break;
		}

		case gameSettingsDone:
		{
			finished = true;
			updateBadgePreferences();
			break;
		}
			
		case gameSettingsBluetooth:
		{
			// toggle bluetooth
			btEnabled = !btEnabled;
			setBluetoothEnabled(btEnabled);
			updateMenuSelection(colors.ColorBackground, colors.ColorForeground);
			break;
		}

		case gameSettingsColors:
		{
			// settingColors = ! settingColors;
			updateMenuSelection(colors.ColorForeground, colors.ColorBackground);
			selectedSetting = gameSettingsDone;
			updateMenuSelection(colors.ColorBackground, colors.ColorForeground);
			break;
		}    

		default:
			break;
	}
}

void SettingsApp::updateName(const char * screenName)
{
	//tft.setTextColor()
	tft.setCursor(2,  (int(selectedSetting) + 1) * TEXT_SPACING * 2);
	tft.printf("%s: %-20s", settingsNames[selectedSetting], screenName);
}

void SettingsApp::updateMenuSelection( uint16_t foreground, uint16_t background)
{
	tft.setTextColor(foreground, background);
	if (selectedSetting == gameSettingsDone)
    {
    	tft.setCursor(2, SCREEN_HEIGHT - TEXT_SPACING * 2);
    }
    else
    {
		tft.setCursor(2,  (int(selectedSetting) + 1) * TEXT_SPACING * 2);
    }

	switch (selectedSetting)
	{
		case gameSettingsBrightness:
		{
			tft.printf("%s: %3d", settingsNames[selectedSetting], (int)brightness);
			break;
		}
		case gameSettingsName:
		{
			if (strlen(myName) == 0) {
	        	tft.printf("%s:", settingsNames[selectedSetting]);
        	} else {
	        	tft.printf("%s: %-20s", settingsNames[selectedSetting], myName);
	        }
	        break;
		}
		case gameSettingsBluetooth:
		{
			tft.printf("%s: %-3s", settingsNames[selectedSetting], btEnabled ? "on" : "off");
			break;
		}
		default:
		{
			tft.print(settingsNames[selectedSetting]);
			break;
		}
	}
}

void SettingsApp::drawSettingsMenu()
{
    tft.fillScreen(colors.ColorBackground);

    tft.setCursor(8, 0);
    tft.setFont();  // standard font
    tft.setTextSize(2);  
    tft.setTextColor(colors.ColorTitle, colors.ColorBackground); 
    tft.print("Settings");

    for (auto i = gameSettingsName; i < gameSettingsLast; i = settingsItems((int)i+1))
    {
        if (i == selectedSetting)
        {
            tft.setTextColor(colors.ColorBackground, colors.ColorForeground);
        }
        else
        {
            tft.setTextColor(colors.ColorForeground, colors.ColorBackground); 
        }

        if (i == gameSettingsDone)
        {
        	tft.setCursor(2, SCREEN_HEIGHT - TEXT_SPACING * 2);
        }
        else
        {
        	tft.setCursor(2, (int(i) + 1) * TEXT_SPACING * 2);
        }
        
        if (i == gameSettingsBrightness)
        {
        	tft.printf("%s: %3d", settingsNames[i], (int)brightness);
        }
        else 
        {
        	if (i == gameSettingsName)
        	{
	        	if (strlen(myName) == 0)
	        	{
		        	tft.printf("%s:", settingsNames[i]);
	        	}
	        	else
	        	{
		        	tft.printf("%s: %-20s", settingsNames[i], myName);
		        }
        	}
	        else 
	        {
	        	if (i == gameSettingsBluetooth)
		        {
		        
		       		tft.printf("%s: %s", settingsNames[i], btEnabled ? "on" : "off");
		        }
		        else
		        {
		        	tft.print(settingsNames[i]);
		        }
		    }
		}
    }
    tft.drawBitmap(122, 70, landerBitmap, 196, 158, colors.ColorLanderTrace);
}

const ColorMap & SettingsApp::getColorMap() const {
	return colorSchemes[colorSchemeIndex];
}

void SettingsApp::unlock(unsigned long now)
{
	if (!colorsUnlocked)
	{
		double d = readdch();
		colorsUnlocked = abs(d - h) > 18.0;
		if (colorsUnlocked)
		{
			Serial.println("Colors unlocked!");
			tft.fillScreen(DEFCON27_MAUVE);
			tft.drawBitmap(122, 70, landerBitmap, 196, 158, DEFCON27_YELLOW);
			tft.setCursor(8, SCREEN_HEIGHT - TEXT_SPACING * 2);
            tft.setTextColor(ILI9341_BLACK, DEFCON27_MAUVE);  
            tft.setTextSize(2);
            tft.println("Colors unlocked!");
            delay(2000);
            updateBadgePreferences();
            colorShift = now + COLOR_SHIFT_DURATION;
            cyclingColors = 1;
            colors = colorSchemes[cyclingColors];
            drawSettingsMenu();
		}
	}
}
