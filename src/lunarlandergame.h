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

// mask used for angles, 0 .. 15.
#define ANGLES 0x000F

// The world is 512 pixels wide and wraps.  When not zoomed, 320 pixels are visible at a time.
#define WORLD_X_MASK 0x01FF

// images
// BORDER is distance from left side,this must be SCREEN_WIDTH / 2, because we keep the lander
// in the middle of the screen (horizontally)
#define BORDER 160
#define TOP_BORDER TEXT_SPACING * 4
#define LOWER_BORDER 10
#define WORLD_WIDTH 512
#define START_LANDER_ALTITUDE SCREEN_HEIGHT - TOP_BORDER  // measured in m, actually this is the screen.
#define ENGINE_LEVELS 5
#define GRAVITY  0.00162
#define ZOOM_HEIGHT 90

// don't change the random error too frequently because it is confusing.
#define ERROR_CHANGE_DELAY  4000

// The landing zones.  Only half of these will be active at any time.
// The rest still give some kind of bonus though.  Coordinates are world coordinates.
typedef struct {
  short x, y, width, bonus;
  const char * bonusText;
} LandingZone;

typedef struct {
	short vx, vy;
} Velocity;

// a GameCamera is a view into the world (x, y and zoom).  Could be useful for other games.

// convert world coordinate to screen coordinate, including zoom.  p is a GeoPoint, c is a GameCamera
#define worldCoordToScreenX(p, c) ((((p.x) - (c.x)) * (c).zoom + WORLD_WIDTH) & WORLD_X_MASK)
#define worldCoordToScreenY(p, c) (SCREEN_HEIGHT - (p.y) * (c.zoom) + (c.y) - LOWER_BORDER)

class GameCamera {
	public:
		GameCamera(short x = 0, short y = 0, short zoom = 1) : x(x), y(y), zoom(zoom), mx(mx), my(my) {
			if (zoom == 2) {
				mx = (x + SCREEN_WIDTH / 2 + WORLD_WIDTH) & WORLD_X_MASK;
				my = (y + SCREEN_HEIGHT / 2);
			}
			else {
				mx = (x + SCREEN_WIDTH + WORLD_WIDTH) & WORLD_X_MASK;
				my = y + SCREEN_HEIGHT;
			}
		};

		void setZoom(const short z) {
			zoom = z;
			if (zoom == 2) {
				mx = (x + (SCREEN_WIDTH / 2) + WORLD_WIDTH) & WORLD_X_MASK;
				my = y + SCREEN_HEIGHT / 2;
			}
			else {
				mx = (x + SCREEN_WIDTH + WORLD_WIDTH) & WORLD_X_MASK;
				my = y + SCREEN_HEIGHT;
			}
			DebugPrintf("visible x in world coords is %d to %d, zoom %d\r\n", x, mx, zoom);
		}

		bool onScreenX(const short wx) const {
			if (zoom == 1) {
				return x < mx ? (wx >= x && wx < mx) : (wx >= x || wx < mx);
			} else {
				return x < mx ? (wx >= x && wx < mx) : (wx >= x || wx < mx);
			}
		}

		bool onScreenY(const short wy) const {
			return wy >= y && wy < my;
		}

		void CopyFrom(const GameCamera & cam) {
			x = cam.x;
			y = cam.y;
			zoom = cam.zoom;
			mx = cam.mx;
			my = cam.my;
		}
	
	    short x, y, zoom, mx, my;
};


// offsets for points used to draw a lander at a specific angle
typedef struct  {
	short height;
	short line1x1, line1y1, line1x2, line1y2;
	short line2x1, line2y1, line2x2, line2y2;
	short leg1x1, leg1y1, leg1x2, leg1y2;
	short leg2x1, leg2y1, leg2x2, leg2y2;
	short burnx1, burny2, burnx2, burny1;
	short burnx3, burny3; // multiply by burn rate
} LanderPoints;


// the lander
class TheEagle {
	public:

		void init(int x, int y)
		{
			angle = 3;
			enginesLevel = 0;
			fuel = startfuel;
			landerMass = landerMassDry + (fuel * fuelUnitMass);
			velocityX = startvelocityX;
			velocityY = startvelocityY;
			fuel = startfuel;
			landerX = x * internalZoom;
			altitude = y * internalZoom; 
		};

		int angle;  // 0 .. 15
		float velocityX;
		float velocityY;
		int   enginesLevel;
		float fuel;
		float landerMass;

		void updateLanderBurn(unsigned long timePassed)
		{
			 DebugPrintf("time passed %ld\r\n", timePassed);
			// Burn fuel
		    if (fuel <= 0) {
		        enginesLevel = 0;
		        fuel = 0;
		    }
		    if (enginesLevel > 0)
		        fuel -= (fuelBurnRate * timePassed * enginesLevel);
		 
		 	#define fudge 0.032 

		    // Update lander mass
		    landerMass = landerMassDry + (fuel * fuelUnitMass);

		    // Change lander velocity
		    velocityX += timePassed * (fudge * deltav[angle].vx * enginesLevel) / landerMass;  // m/s
		    float landerAccelerationUpFromThrust = (fudge * deltav[angle].vy * enginesLevel) / landerMass;
		    velocityY += (GRAVITY -  landerAccelerationUpFromThrust) * timePassed;
		  	// Move lander
		    altitude -= round(velocityY * timePassed * internalZoom/ 1000.0);
		    landerX  += round (velocityX * timePassed * internalZoom / 1000.0);
		}

		int locX() const {
			return (landerX / internalZoom);
		}

		int locY() const {
			return altitude / internalZoom;
		}

		int fuelRemaining() const
		{
			return round((fuel / startfuel) * 100);
		}

		bool fuelLow() const 
		{
			return fuel / startfuel <= 0.2;
		}

		static const std::vector<Velocity> deltav;

	private:
		const float startvelocityY = 1.0;   // measured in m/s
		const float startvelocityX = 20.0;   // measured in m/s
		const int startfuel = 100000 ;  // measured in ml, was l
		const float fuelBurnRate = 1.0; // ml / ms == litres/sec
		const float fuelUnitMass = 2.550 / startfuel;   // mass / volume, kg / litre
		const int landerMassDry = 2150; // kg
		const int internalZoom = 16;

		int altitude;	// landerY
		int landerX;

};


class LunarLanderGame : public App
{
	public:

		LunarLanderGame(ColorMap & c) : colors(c) {};

		void init()
		{
		    Preferences preferences;
		    preferences.begin("Lander", false);
		    stats.score = preferences.getUInt("score", 0);
		    stats.wins = preferences.getUInt("wins", 0);
		    stats.played = preferences.getUInt("played", 0);
		    DebugPrintf("record is score %d, won %d, played %d\r\n", stats.score, stats.wins, stats.played);
		    preferences.end();
		};

		const char *Name() override 
		{
			return name;
		}

		void start(unsigned long now, const ColorMap & c) override;

		void blinkAction(bool blinkOn) override
		{
			flicker++;
			if (CABIN_LIGHTS != 0)
			{
				if (flicker == flickerCount)
				{
					ledcWrite(cabinLightChannel, 0);
					flicker = 0;
				}
				else
				{
					ledcWrite(cabinLightChannel, 127);
				}	
			}

			if (blinkOn)
			{
				if (errorCode != 0)
				{
					clearErrorCode();
				}
				eraseLandingZones(currentCamera);
				
			}
			else
			{
				if (errorCode != 0)
				{
					showErrorCode();
				}
				drawLandingZones(currentCamera);
			}		
		}

		void buttonAction(unsigned long now, ButtonKeys button) override;

		bool updateIsComplete(unsigned long now) override ;
		bool isStopped(unsigned long now) override ;

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

		void resetStats() override;

	private:
		unsigned int ScoreAtWorldX(int x, int vx, int vy);
		void physics(unsigned long timestamp);
		void redrawScreen(bool eraseLastFrame);
		void drawMountains(uint16_t color, const GameCamera & cam );
		void drawLanderHelper(int x, int y, int burn, int angle, uint16_t landerColor, uint16_t burnColor, short zoom);
		void drawLander(int x, int y, int burn, int angle, short zoom);
		void updateText(const unsigned long timestamp);
		void drawExplosion(uint16_t x, uint16_t y);
		void resetAction(unsigned long now);

		GeoPoint starsOnScreen[64];	// max number of stars
		int starsDrawn;

		static const std::vector<short> altitudes;

		static const std::vector<LandingZone> landingZones;
		uint16_t landingZoneBonus[8];

		// Random collections of landing zones.  Each is a bitmap with 4 of the 8 landing zones selected.
		static const std::vector<short> lzCollection;

		// star locations (generated by a spreadsheet).  These are world coordinates.
		static const std::vector<GeoPoint> stars;

		// when we scroll up, these are the visible stars (there is a 256 pixel vertical wrap, not really noticeable.
		static const std::vector<GeoPoint> highStars;

		// there are 16 possible angles and this defines the points we could draw the lander at.  Generated by spreadsheet.
		static const std::vector<LanderPoints> landerOffsets;

		static const std::vector<Triangle> explosionTriangles;

		static const std::vector<std::pair<short, short>> flatBits;

		int flicker;
		const int flickerCount = 6;
		const char * name = "LunarLander";
		GameCamera currentCamera;
		GameCamera previousCamera;
		int errorCode = 0;
		TheEagle lander;
		static const std::vector<const char *> ErrorCodes;
		unsigned long lastErrorSet;

		// Lander locations in world coordinates (0 .. 511)
		GeoPoint landerLocation;
		GeoPoint lastLanderLocation;

		int lastLanderAngle;
		int lastLanderBurn;
		int lastLanderZoom = 1;

		short currentGroundHeight;

		float timeDilationFactor = 2.0;

		unsigned long pauseUntilTime = millis() + 500;
		unsigned long lastPhysicsTimestamp = 0;

		GameStatistics stats;

		bool won, lost, quit;
		bool paused;

		ColorMap & colors;

		// clear the mysterious error code that flashes on the screen
		void clearErrorCode();

		// show an error code during operation
		void showErrorCode();

		// choose an error code
		void setRandomError(unsigned long now);

		// erase any visible landing zones
		void eraseLandingZones(const GameCamera &cam);

		// draw the landing zones
		void drawLandingZones(const GameCamera &cam);

};




