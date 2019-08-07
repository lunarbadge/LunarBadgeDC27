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
#include "BleLander.h"

#pragma once

class AFriend 
{
	public:
		String friendsName;
		int friendsScore;
		int connectionCount;
		unsigned long lastConnected;	//in seconds, not millis

		AFriend(String name, int score) : friendsName(name), friendsScore(score)
		{
			connectionCount = 1;
			lastConnected = millis();
		};

		AFriend() {};
};

class FriendsApp : public App, public BleFriendObserver
{

	public:
		FriendsApp(ColorMap & c) : colors(c)
		{
			initialized = false;
		};

		const char * Name()
		{
			return "Friends";
		};

		void init(BleLander *scanner);

		void onScanResult(String result)
		{
			DebugPrintf("Scanning found %s\r\n", result.c_str());
			// parse the result, add to friends.  This will be score / name; we calculate # connections and last connected time.
			if (result.length() == 0 || result.indexOf(":") == -1)
				return;
			String name = result.substring(0, result.indexOf(":"));
			int score = result.substring(result.indexOf(":") + 1).toInt();
			if (friendsList.find(name) != friendsList.end()) {
				friendsList[name].connectionCount++;
				friendsList[name].lastConnected = millis();
				friendsList[name].friendsScore = score;
			}
			else
			{
				addFriend(name, score);
			}
			persistFriends();
			drawFriendsList();
		};	// from BleFriendObserver

		bool isBluetoothEnabled() const
		{
			return btEnabled;
		};

		void setBluetoothEnabled(bool enabled)
		{
			btEnabled = enabled;
		};

		// update the state of the game (screen, etc) and return true when the game is over.
		bool updateIsComplete(unsigned long now) override;

		// is the game paused?
		bool isStopped(unsigned long now) override;

		// start the game
		void start(unsigned long now, const ColorMap & c) override;

		void buttonAction(unsigned long now, ButtonKeys action) override;

		short gamesWon() override
		{
			return 0;
		};
		short gamesPlayed() override
		{
			return 0;
		};
		short gameScore() override
		{
			return 0;
		};

		void resetStats() override {};

	private:
		void drawFriendsList();
		void clearMenuSelection(int selection);
		void setMenuSelection(int selection);
		void changeMenuSelectionColor(int selection, uint16_t foreground, uint16_t background);
		void addFriend(String name, int score);
		void removeFriend(String name);
		void persistFriends();
		void resetAction(unsigned long now);

		std::map<String, AFriend> friendsList;
		static const String testFriends;
		size_t selectedFriend;
		bool finished;
		char *myName;
		bool btEnabled;
		bool initialized;
		BleLander *scanner;
		int firstOnScreen = 0;
		const int friendCountOnScreen = 10;
		ColorMap & colors;
};
