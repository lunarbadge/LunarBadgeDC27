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
#include "app.h"
#include "hardware.h"

#pragma once

#define NAME_LEN 20
#define COLOR_SHIFT_DURATION 2000

class SettingsApp : public App
{
	// interface to implement

	public:
		SettingsApp(ColorMap & c) : colors(c)
		{
			initialized = false;
		};

		void init();

		unsigned short getBrightness() {
			return brightness;
		}

		const char * Name() override
		{
			return "Settings";
		}

		bool isBluetoothEnabled() const {
			return btEnabled;
		};

		const char *playerName() {
			if (!initialized)
				init();
			
			Serial.printf("player name is: %s\r\n", myName);
			return myName;
		}

		void setBluetoothEnabled(bool enabled);
		const ColorMap & getColorMap() const;

		void buttonAction(unsigned long now, ButtonKeys button) override;

		bool updateIsComplete(unsigned long now) override;

		bool isStopped(unsigned long now) override;

		void start(unsigned long now, const ColorMap & c) override;

		short gamesWon() override { return 0; };
		short gamesPlayed() override { return 0; };
		short gameScore() override { return 0; };

		void resetStats() override {};

	private:
		void drawSettingsMenu();
		void updateMenuSelection( uint16_t foreground, uint16_t background);
		void updateName(const char * screenName);
		void updateBadgePreferences();
		void resetAction(unsigned long now);
		void unlock(unsigned long now);

		unsigned short brightness;
		bool colorsUnlocked = false;
		int16_t cyclingColors = -1;
		unsigned long colorShift;
		bool settingColors;
		bool settingName;
		bool finished;
		char myName[NAME_LEN];
		char newName[NAME_LEN];
		bool btEnabled;
		String salutation;
		bool initialized;
		bool settingBrightness;

		typedef enum {
			gameSettingsName = 0,
			gameSettingsBrightness,
			gameSettingsBluetooth,
			gameSettingsColors,
			gameSettingsDone,
			gameSettingsLast
		} settingsItems;

		const std::vector<const char *> settingsNames = {"Name", "Brightness", "Bluetooth", "Change Colors", "Done"};
		static const std::vector<ColorMap> colorSchemes;
		uint16_t colorSchemeIndex;
		settingsItems selectedSetting;
		ColorMap & colors;	// current colors in use.

		double h = 0;
};
