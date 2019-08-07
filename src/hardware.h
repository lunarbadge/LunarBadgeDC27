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
#pragma once

// Compiling / building:
// Use Adafruit ESP32 Feather, with large variant.
// Partition Scheme: Huge App.
//

// TFT connections (also MISO / MOSI)

#define TFT_DC 25
#define TFT_CS 26

#define LITE_PIN  21

// LEDs
#define CABIN_LIGHTS 17
#define ENGINE_LED_1 14
#define ENGINE_LED_2 12
#define ENGINE_LED_3 13	

// buttons
#define RESTART_PIN 4

#define BUTTON_ONE_PIN   36
#define BUTTON_TWO_PIN   39
#define BUTTON_THREE_PIN 15
#define BUTTON_FOUR_PIN  34

// SAO.  Not using these with this version of the badge software.
#define SAO_IO_1 32
#define SAO_IO_2 33
#define SAO_SCL  22
#define SAO_SDA  23

// management
#define BATTERY_LEVEL_PIN 35
#define BATTERY_SAMPLE_DELAY 5000

// buttons and debounce

#define DEBOUNCE 50
#define REPEAT_DELAY 100
#define REPEAT_RATE 50

// parameters used for debouncing buttons
typedef struct {
	unsigned long lastDebounceTime;
	unsigned long lastClickedTime;
	unsigned long lastRepeatTime;
	int state;
	int lastState;
	int buttonName;
} Debouncer;

extern bool debounceStateChanged(Debouncer &button);

typedef enum ButtonKeys { restartButton = 0, buttonA, buttonB, buttonC, buttonD, };

extern std::map<ButtonKeys, Debouncer> buttonDebouncer;

extern bool anyButtonPressed();

// game parameters
#define MILLIS_PER_FRAME 60

typedef struct 
{
	short x;
	short y;
} GeoPoint;

typedef struct
{
	GeoPoint p1, p2, p3;
} Triangle;

extern Adafruit_ILI9341 tft;

// screen size
#define SCREEN_HEIGHT 240
#define SCREEN_WIDTH  320 
#define RIGHT_TEXT_COL SCREEN_WIDTH/2
#define TEXT_SPACING 16
#define LABEL_WIDTH 64


extern double readdch();

