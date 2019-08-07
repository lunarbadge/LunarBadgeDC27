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

#include "hardware.h"

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

// Has a button been pressed (with debounce).  Repeating enabled for
// all buttons except Reset because that's annoying.
bool debounceStateChanged(Debouncer &button)
{
    int reading = digitalRead(button.buttonName);
    unsigned long now = millis();

    if (reading != button.lastState)
    {
        button.lastDebounceTime = now;
    }

    if ((now - button.lastDebounceTime) > DEBOUNCE)
    {
        if (reading != button.state)
        {
            button.lastState = button.state;
            button.state = reading;
            
            if (reading == LOW)
            {
                button.lastClickedTime = now;
                button.lastRepeatTime = now;
                return true;
            }
        }
        else if (button.buttonName != RESTART_PIN && reading == LOW && 
                (now - button.lastClickedTime) > REPEAT_DELAY && (now - button.lastRepeatTime) > REPEAT_RATE)
        {
            button.lastRepeatTime = now;
            return true;
        }
    }
    button.lastState = reading;
    return false;
}


bool resetPressed(Debouncer &resetButton)
{
    return debounceStateChanged(resetButton);
} 

bool anyButtonPressed()
{
    for (ButtonKeys i = restartButton; i <= buttonD; i = ButtonKeys((int)i+1))
    {
        if (debounceStateChanged(buttonDebouncer[i]))
        {
            return true;
        }
    }
    return false;
}

double readdch()
 {
    long h = 0;
    for (int i = 0; i < 1000; i++)
    {
        h += hallRead();
    }
    // put the button keys back to their expected behavior.
    pinMode(36, INPUT);
    pinMode(39, INPUT);
    return (double)h / 1000.0;
}