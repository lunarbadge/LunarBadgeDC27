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

#include "CommanderGame.h"

// update the state of the game (screen, etc) and return true when the game is over.
bool CommanderGame::updateIsComplete(unsigned long now) 
{
	// erase last point
	if (radialPoint >= landerTrajectory.size())
    {
        Serial.println("The game ends");
		return true;
    }

    switch (radialPoint)
    {
    case 0:
        Serial.println("The command module is orbiting the moon.");
        break;

    case 5:
        Serial.println("The lander separates from the command module");
        break;

    case 30:
        Serial.println("The lander lands on the lunar surface");
        break;

    case 388:
        Serial.println("The lander lifts off from the lunar surface");
        break;

    case 414:
        Serial.println("The lander docks with the command module");
        break;

    default:
        break;
    }

	tft.fillCircle(orbitLocatons[radialPoint].x, orbitLocatons[radialPoint].y, 2, colors.ColorBackground);
	
	tft.fillCircle(landerTrajectory[radialPoint].x, landerTrajectory[radialPoint].y, 2, colors.ColorBackground);

	radialPoint++;
	if (radialPoint == landerTrajectory.size())
    {
		Preferences preferences;
        if (!preferences.begin("Commander", false))
        {
            Serial.println("failed to open preferences for update");
        }
        //if (won)
        stats.wins++;
        stats.played++;

        preferences.clear();

        if (preferences.putUInt("score", stats.score) != 4)
        {
            DebugPrintln("could not save score");
        }
        if (preferences.putUInt("wins", stats.wins) != 4)
        {
            DebugPrintln("could not save wins");
        }
        if (preferences.putUInt("played", stats.played) != 4)
        {
            DebugPrintln("could not save wins");
        }
        // Close the Preferences
        preferences.end();
		return true;
	}

	tft.fillCircle(landerTrajectory[radialPoint].x, landerTrajectory[radialPoint].y, 2, colors.ColorLander);
	tft.fillCircle(orbitLocatons[radialPoint].x, orbitLocatons[radialPoint].y, 2, colors.ColorOrbiter);

	return false;

}

void CommanderGame::printRandomMessage()
{
	tft.setFont();
	tft.setCursor(1, 231);
    tft.setTextSize(1);
    tft.setTextColor(colors.ColorForeground, colors.ColorBackground); 
    tft.fillRect(1, 231, 318, 8, colors.ColorBackground);
    int index = random(messages.size());
    tft.print(messages[index]);
    Serial.println(messages[index]);
}

// is the game paused?
bool CommanderGame::isStopped(unsigned long now)
{
    return true;
}

// start the game
void CommanderGame::start(unsigned long now, const ColorMap & c)
{
    Serial.println("The command module is critical for supporting the lander");
    tft.drawRGBBitmap(66, 26, greyMoon, moonMask, moon2_width, moon2_height);
    Serial.println("an image of the moon appears");
	radialPoint = 0;
    colors = c;
}

// GameStatistics gameStats();

void CommanderGame::resetStats()
{
    stats.wins = stats.played = stats.score = 0;
    Preferences preferences;
    preferences.begin("Commander", false);
    preferences.clear();
    preferences.putUInt("score", stats.score);
    preferences.putUInt("wins", stats.wins);
    preferences.putUInt("played", stats.played);
    preferences.end();
}


