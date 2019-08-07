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

#include <Preferences.h>
#include <gfxfont.h>
#include <Adafruit_GFX.h>
#include <Fonts/FreeMonoBoldOblique12pt7b.h>
#include <Fonts/FreeSans9pt7b.h>
#include "Adafruit_ILI9341.h"
#include <vector>
#include <string>
#include <map>

#include <Wire.h>
#include "SPI.h"

#pragma once

#define MAX_BRIGHT 10

#define LANDER_SERVICE_UUID     "25602AD1-32EF-482C-92A8-CCDE215CD1C1"
#define BLE_CHARACTERISTIC_UUID "981ACF50-D2D1-402C-A4A6-4BE2AF58AA3E"

#define ADVERTISEMENT_SZ 80


// 'landerbm', 196x158px
extern const uint8_t landerBitmap [] PROGMEM ;
// 'csm_buttons', 272x109px
extern const uint8_t badgeBitmap [] PROGMEM ;

// 'downarrow', 10x13px
extern const uint8_t upArrowBitmap [] PROGMEM ;
extern const uint8_t downArrowBitmap [] PROGMEM ;

// 'leftarrow', 14x18px
extern const uint8_t rightRotateBitmap [] PROGMEM;
extern const uint8_t leftRotateBitmap [] PROGMEM;

#define moon2_width 188
#define moon2_height 188

extern const uint16_t greyMoon[] PROGMEM;
extern const uint8_t moonMask[] PROGMEM;
extern int screenBrightnessChannel;
extern int cabinLightChannel;

#define fabs(x) ( ((x) > 0) ? (x) : -(x) )

#define DEBUG 0

#define DebugPrintf(fmt, ...) \
            do { if (DEBUG) Serial.printf(fmt, __VA_ARGS__); } while (0)

#define DebugPrintln(s) \
            do { if (DEBUG) Serial.println(s); } while (0)

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



