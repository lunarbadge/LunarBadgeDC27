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

class CommanderGame : public App
{
	// interface to implement

	public:
		CommanderGame(ColorMap c) : colors(c) {};

		void init()
		{
			Preferences preferences;
		    if (!preferences.begin("Commander", true))
		    {
 		    	DebugPrintln("failed to open preferences for read");
		    }
		    stats.score = preferences.getUInt("score", (uint32_t)0);
		    stats.wins  = preferences.getUInt("wins", (uint32_t)0);
		    stats.played = preferences.getUInt("played", (uint32_t)0);
		    DebugPrintf("commander stats: record is score %u, won %u, played %u\r\n", stats.score, stats.wins, stats.played);
		    preferences.end();
		};

		// the game has a name
		const char * Name() override
		{
			return "Command Game";
		}

		// update the state of the game (screen, etc) and return true when the game is over.
		bool updateIsComplete(unsigned long now) override;

		// is the game paused?
		bool isStopped(unsigned long now) override;

		// start the game
		void start(unsigned long now, const ColorMap &c) override;

		void resetStats() override;

		void buttonAction(unsigned long now, ButtonKeys button) override 
		{
			printRandomMessage();
		};

		short gamesWon() override
		{
			return stats.wins;
		};

		short gamesPlayed() override
		{
			return stats.played;
		};

		short gameScore() override
		{
			return stats.score;
		};

	private:
		void printRandomMessage();
		
		GameStatistics stats;
		uint16_t radialPoint;
		GeoPoint landerLoc;
		ColorMap &colors;

		static const std::vector<GeoPoint> orbitLocatons;
		static const std::vector<GeoPoint> landerTrajectory;

		static const std::vector<const char *> messages;

};
