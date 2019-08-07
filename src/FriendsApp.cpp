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

#include "FriendsApp.h"

//const String FriendsApp::testFriends = String("53;alice;2;428:50;bob;1;589:45;cate;3;581:53;dale;2;428:50;erin;1;589:45;fred;3;581:53;gina;2;428:50;harry;1;589:45;isolde;3;581:53;jack;2;428:50;karen;1;589:45;louis;3;581:53;mary;2;428:50;nathan;1;589:45;ollie;3;581:53;pete;2;428:50;q;1;589:45;roger;3;581:53;sara;2;428:50;tom;1;589:45;una;3;581:53;victor;2;428:50;wendy;1;589:45;xavier;3;581:53;yolander;2;428:50;zed;1;589:");

void FriendsApp::init(BleLander *bleScanner)
{
	scanner = bleScanner;

	// read known friends from preferences.
	// score / name (int / string).  If int == -1 we are done.
	Preferences preferences;
	DebugPrintln("friends preferences");
    preferences.begin("myFriends", true /* readonly */);
    int count = preferences.getInt("friendCount", 0);
    count = 3;
    DebugPrintln(count);
    if (count > 0)
    {
	    String allFriends = preferences.getString("allFriends", "");
	    // allFriends = FriendsApp::testFriends;
	    // score;name;connections;lastconnected:score:name;connections;lastconnected...
	    // DebugPrintf("allfriends %s\n", allFriends.c_str());
	    int startIndex = 0;
	    int nextIndex = allFriends.indexOf(";");
	    while (nextIndex != -1)
	    {
			AFriend newFriend;
			newFriend.friendsScore = allFriends.substring(startIndex, nextIndex).toInt();
			startIndex = nextIndex + 1;
			nextIndex = allFriends.indexOf(";", startIndex);
			newFriend.friendsName = allFriends.substring(startIndex, nextIndex);
			//DebugPrintf("name is %s\n", allFriends.substring(startIndex, nextIndex).c_str());
			startIndex = nextIndex + 1;
			nextIndex = allFriends.indexOf(";", startIndex);
			newFriend.connectionCount = allFriends.substring(startIndex, nextIndex).toInt();
			startIndex = nextIndex + 1;
			nextIndex = allFriends.indexOf(":", startIndex);
			newFriend.lastConnected = allFriends.substring(startIndex, nextIndex).toInt();

			friendsList[newFriend.friendsName] = newFriend;

			startIndex = nextIndex + 1;
			nextIndex = allFriends.indexOf(";", startIndex);    
	    }
	    // DebugPrintf("allfriends (%d) %s\n", friendsList.size(), allFriends.c_str());
	}
	preferences.end();
}

void FriendsApp::addFriend(String name, int score)
{
	AFriend newFriend(name, score);
	friendsList[name] = newFriend;
}

void FriendsApp::removeFriend(String name)
{
	friendsList.erase(name);
}

void FriendsApp::persistFriends()
{
	Preferences preferences;
    preferences.begin("myFriends", false /* notreadonly */);
    preferences.putInt("friendCount", friendsList.size());

    if (friendsList.size() > 0)
    {
    	String data;
    	for (auto f : friendsList)
    	{
    		data.concat(f.second.friendsScore);
    		data.concat(";");
    		data.concat(f.second.friendsName);
    		data.concat(";");
    		data.concat(f.second.connectionCount);
    		data.concat(";");
    		data.concat(f.second.lastConnected);
    		data.concat(":");
    	}
	    preferences.putString("allFriends", data);
	}
	preferences.end();
}

// update the state of the game (screen, etc) and return true when the game is over.
bool FriendsApp::updateIsComplete(unsigned long now)
{
	return finished;
}

// is the game paused?
bool FriendsApp::isStopped(unsigned long now)
{
	return false;
}

// start the game
void FriendsApp::start(unsigned long now, const ColorMap &c)
{
	colors = c;
	selectedFriend = 0;
	firstOnScreen = 0;
	drawFriendsList();
	finished = false;
}

// always up
void FriendsApp::buttonAction(unsigned long now, ButtonKeys button)
{
	switch (button)
	{
		case buttonA:
		{
			if (selectedFriend == friendsList.size())
			{
				clearMenuSelection(selectedFriend);
				selectedFriend = firstOnScreen + friendCountOnScreen - 1;
				setMenuSelection(selectedFriend);
			} 
			else if (selectedFriend > firstOnScreen) // not 0!
		    {
		    	clearMenuSelection(selectedFriend);
		        selectedFriend = selectedFriend - 1;
		        setMenuSelection(selectedFriend);
		    }
		    break;
		}

		case buttonB:
		{
			if (selectedFriend == firstOnScreen + friendCountOnScreen - 1)
			{
				clearMenuSelection(selectedFriend);
				selectedFriend = friendsList.size();
				setMenuSelection(selectedFriend);
			}
			else if (selectedFriend < friendsList.size() + 1)
			{
				clearMenuSelection(selectedFriend);
				selectedFriend = selectedFriend + 1;
				setMenuSelection(selectedFriend);
			}
			break;
		}

		case buttonC:	
		{
			if (firstOnScreen + friendCountOnScreen < friendsList.size())
			{
				firstOnScreen += friendCountOnScreen;
				drawFriendsList();
			}
			break;
		}
		case buttonD: 
		{
			// DebugPrintln("scroll up?");
			if (firstOnScreen > 0) {
				firstOnScreen = max(0, firstOnScreen - friendCountOnScreen);
				drawFriendsList();
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


void FriendsApp::resetAction(unsigned long now)
{
	int16_t  x1, y1;
    uint16_t w, h;

	if (selectedFriend == friendsList.size())
	{	// this is the Done button
		finished = true;
	} 
	else if (selectedFriend == friendsList.size() + 1)
	{	// this is the scan button
		if (!btEnabled)
		{
			// clear screen, draw a message saying "enable bluetooth", delay for a bit, redraw screen.
			tft.fillScreen(colors.ColorBackground);
			tft.setFont();  // standard font
    		tft.setTextSize(2);
    		tft.setTextColor(colors.ColorTitle, colors.ColorBackground);
    		tft.setCursor(60, 90);
    		tft.print(" Enable bluetooth  \n\n          in Settings    \n\n     to scan for friends ");
    		delay(3000);
    		selectedFriend = friendsList.size();
    		drawFriendsList();
    		return;
		}

		// scan!
		DebugPrintln("scan");
		scanner->setObserver(this);

		tft.setTextSize(2);
		tft.getTextBounds("Scanning ...", 100, 110, &x1, &y1, &w, &h);
		tft.fillRect(x1-2, y1-2, w+4, h+4, colors.ColorBackground);
		tft.setTextColor(colors.ColorTitle, colors.ColorBackground);
		tft.setCursor(100, 110);
		tft.print("Scanning ...");

		(void)scanner->scanForFriends();
		// scan for m seconds, add each friend found via the callback provided.
		// this is a blocking call.

		// afterwards, set the button to "Done"
		selectedFriend = friendsList.size();
		firstOnScreen = 0;
		drawFriendsList();
	}
	else
	{
		// if you want to clear your friend's list, do that in Reset, but not here.
		// maybe reset should be part of settings?
		// go to Done
		selectedFriend = friendsList.size();
		drawFriendsList();
	}
}

void FriendsApp::changeMenuSelectionColor(int selection, uint16_t foreground, uint16_t background)
{
	tft.setTextColor(foreground, background);
	if (selection == friendsList.size())
	{
		tft.setTextSize(2);
		tft.setCursor(2, SCREEN_HEIGHT - TEXT_SPACING * 2);
	    tft.print("Done");
    
	}
	else if (selection == friendsList.size() + 1)
	{

	    tft.setTextSize(2);
	    tft.setCursor(SCREEN_WIDTH/2, SCREEN_HEIGHT - TEXT_SPACING * 2);
	    tft.print("Scan");
	}
	else
	{
		tft.setTextSize(1);
		int verticalPlacement = TEXT_SPACING * 3 + (selection - firstOnScreen) * TEXT_SPACING;

		tft.setCursor(2,  verticalPlacement);

		int i = 0;
	    for (auto f : friendsList)
	    {
	    	if (i == selection)
	    	{
        		tft.printf("%5d  %-34s %4d", f.second.friendsScore, f.first.c_str(), f.second.connectionCount);
        		break;
	    	}
	    	i++;
	    }
	}
}

void FriendsApp::clearMenuSelection(int selection)
{
	changeMenuSelectionColor(selection, colors.ColorForeground, colors.ColorBackground);
}

void FriendsApp::setMenuSelection(int selection)
{
	changeMenuSelectionColor(selection, colors.ColorBackground, colors.ColorForeground);
}

void FriendsApp::drawFriendsList()
{
	tft.fillScreen(colors.ColorBackground);

	unsigned long now = millis();
    tft.setCursor(8, 0);
    tft.setFont();  // standard font
    tft.setTextSize(2);  
    tft.setTextColor(colors.ColorTitle, colors.ColorBackground); 
    tft.print("Friends");

    if (friendsList.size() == 0)
    {
    	tft.setCursor(50,  110);
    	tft.setTextSize(2);  
    	tft.print("Scan for friends");
    }
    else
    {
	    int verticalPlacement = TEXT_SPACING * 2;
	    tft.setTextSize(1);
	    tft.setTextColor(colors.ColorForeground, colors.ColorBackground);
	    tft.setCursor(2,  verticalPlacement);
	    tft.printf("score  %-30ssightings", "name");

	    verticalPlacement += TEXT_SPACING;

	    if (selectedFriend < friendsList.size() && 
	    		(selectedFriend < firstOnScreen || selectedFriend >= firstOnScreen + friendCountOnScreen))
	    {
	    	selectedFriend = firstOnScreen;
		}
	   
	    int i = 0;
	    for (auto f : friendsList)
	    {
	    	if (i < firstOnScreen)
	    	{
	    		// DebugPrintf("%d is not on screen; skipping\n", i);
	    		i++;
	    		continue;
	    	}

	    	if (i >= firstOnScreen + friendCountOnScreen)
	    	{
	    		// DebugPrintf("%d is not on screen, so we are done here\n", i);
	    		break;
	    	}

			if (i == selectedFriend)
			{
	            tft.setTextColor(colors.ColorBackground, colors.ColorForeground);
	        }
	        else
	        {
	            tft.setTextColor(colors.ColorForeground, colors.ColorBackground); 
	        }
	        tft.setCursor(2,  verticalPlacement);
	        tft.printf("%5d  %-34s %4d", f.second.friendsScore, f.first.c_str(), f.second.connectionCount);
	    	verticalPlacement += TEXT_SPACING;

	    	i++;
	    }
	}
    if (selectedFriend == friendsList.size())
    {
	    tft.setTextColor(colors.ColorBackground, colors.ColorForeground);
    }
    else
    {
        tft.setTextColor(colors.ColorForeground, colors.ColorBackground); 
    }
    tft.setCursor(2, SCREEN_HEIGHT - TEXT_SPACING * 2);
    tft.setTextSize(2);
    tft.print("Done");
    if (selectedFriend == friendsList.size() + 1)
    {
    	tft.setTextColor(colors.ColorBackground, colors.ColorForeground);
    }
    else
    {
        tft.setTextColor(colors.ColorForeground, colors.ColorBackground); 
    }
    tft.setCursor(SCREEN_WIDTH/2, SCREEN_HEIGHT - TEXT_SPACING * 2);
    tft.print("Scan");
}

