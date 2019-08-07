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

#pragma once

// most games have statistics.  You might want this too.
class GameStatistics
{
	public:
		uint32_t wins;
		uint32_t played;
		uint32_t score;
};

// Most of the menu items are "Apps", and this describes how to interact with them.
class App
{
	// interface to implement

	public:
		// the app has a name
		virtual const char * Name() = 0;

		// blinkAction is called periodically for the app to update anything that should
		// blink, or similar.
		virtual void blinkAction(bool blinkOn) {};

		virtual void buttonAction(unsigned long now, ButtonKeys key);

		// update the state of the app (screen, etc) and return true when the app is finished.
		// this will get called no more than every 60ms.
		virtual bool updateIsComplete(unsigned long now) = 0;

		// is the app paused?
		virtual bool isStopped(unsigned long now);

		// game statistics
		virtual short gamesWon() = 0;
		virtual short gamesPlayed() = 0;
		virtual short gameScore() = 0;

		// start the isStopped
		virtual void start(unsigned long now, const ColorMap & colors) = 0;

		virtual void resetStats();

};
