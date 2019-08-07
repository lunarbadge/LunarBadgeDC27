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

class BleLander;
#pragma once

#define IDLE_LIGHT_INTERVAL 1000

/* A singleton class to do nothing other than
 * flash some LEDs to keep the badge looking pretty.
 *
 * It is pinned to the second CPU so it doesn't interfere with tasks running
 * on the first CPU (Arduino uses CPU1 by default).
 */

class Idler 
{
	public:
		void init();

		void nextIdleStep(unsigned long now);

		void setIdle(const bool idle);

		void setScanning(const bool scan);

		void setBle(BleLander *bleL) {
			bleLanderL = bleL;
		}

		BleLander *bleLander() { return bleLanderL; };

		static Idler *getInstance()
	    {
	        if (!s_instance)
	          s_instance = new Idler();
	        return s_instance;
	    }

	    unsigned long stopTime() const { return scanStopTime; };

	    bool isIdling() const { return idling; };

	    bool isScanning() const { return scanning; };

	private:
		bool idling;
		bool scanning;
		int idleLightStep = 0;
		unsigned long lightChange;
		unsigned long scanStopTime;
		BleLander *bleLanderL;
		static const std::vector<unsigned short> idleSteps;
		static Idler *s_instance;
};

