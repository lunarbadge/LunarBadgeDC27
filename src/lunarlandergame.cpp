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

#include "lunarlandergame.h"

const std::vector<const char *> LunarLanderGame::ErrorCodes = {"", "1201", "1203"};

void LunarLanderGame::start(unsigned long now, const ColorMap & c)
{
    colors = c;
    lander.init(BORDER-1, START_LANDER_ALTITUDE);
    // and all the other stuff
    lastLanderLocation.x = -1;
    lastLanderLocation.y = -1;
    lastLanderAngle -1;
    lastLanderBurn = -1;
    
    unsigned short mask = 1;
    int j = 0;
    unsigned short lzs = lzCollection[random(landingZones.size())];

    for (auto lz : landingZones )
    {
        if (lzs & mask)
        {
            landingZoneBonus[j] = lz.bonus;
        }
        else
        {
            landingZoneBonus[j] = 1;            
        }
        mask = mask << 1;
        j++;
    }
    
    timeDilationFactor = 2.0;
    currentCamera.x = 0;
    currentCamera.y = 0;
    currentCamera.setZoom(1);
    
    // // Reset physics clock
    lastPhysicsTimestamp = 0;
    
    won = false;
    lost = false;
    quit = false;
    paused = false;
    
    // Clear everything
    redrawScreen(false);
    flicker = 0;

    starsOnScreen[0].x = -1;
    starsOnScreen[0].y = -1;
    starsDrawn = 0;
}

bool LunarLanderGame::isStopped(unsigned long now) 
{
    return (won || lost);
}

void LunarLanderGame::resetStats()
{
    stats.wins = stats.played = stats.score = 0;
    Preferences preferences;
    preferences.clear();
    preferences.begin("Lander", false);
    preferences.putUInt("score", stats.score);
    preferences.putUInt("wins", stats.wins);
    preferences.putUInt("played", stats.played);
    preferences.end();
}

bool LunarLanderGame::updateIsComplete(const unsigned long now)
{
    if (!paused)
    {
        physics(now);
        redrawScreen(true);
        updateText(now);
    }
    if (won || lost || quit)
        DebugPrintln("quitting for real");
    return (won || lost || quit);
}

void LunarLanderGame::buttonAction(unsigned long now, ButtonKeys button) 
{
    switch (button)
    {
        case buttonA:
        {
            if (paused)
            {
                paused = false;
                tft.fillScreen(colors.ColorBackground);
                return;
            }

            lander.angle = (lander.angle + 1) & ANGLES;
            setRandomError(now); 
            break;
        }

        case buttonB:
        {
            if (paused)
            {
                paused = false;
                tft.fillScreen(colors.ColorBackground);
                return;
            }
            
            lander.angle = (lander.angle + 15 ) & ANGLES;
            setRandomError(now);
            break;
        }

        case buttonC:
        {
            if (paused)
            {
                paused = false;
                tft.fillScreen(colors.ColorBackground);
                return;
            }
            if (lander.enginesLevel > 0)
            {
                lander.enginesLevel--;
                setRandomError(now);
            }
            break;
        }

        case buttonD:
        {
            if (paused)
            {
                paused = false;
                tft.fillScreen(colors.ColorBackground);
                return;
            }
            if (lander.enginesLevel < ENGINE_LEVELS && lander.fuel > 0)
            {
                lander.enginesLevel++;
                setRandomError(now);
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

void LunarLanderGame::resetAction(unsigned long now)
{
    if (paused)
    {
        quit = true;
        DebugPrintln("quitting");
    }
    else
    {
        DebugPrintln("pausing");
        
        paused = true;
        tft.setTextColor(colors.ColorTitle, colors.ColorBackground);
        tft.setTextSize(2);
        tft.fillRect(20, 82, 280, 40, colors.ColorBackground);
        tft.setCursor(58, 84);
        tft.print("Paused; R to exit");
        tft.setCursor(22, 102);
        tft.print("Any other key to resume");

    }
}

// what is the score assuming landing at this place?
unsigned int LunarLanderGame::ScoreAtWorldX(int x, int vx, int vy)
{
    int baseScore;
    if (vy < 4.6)
    {
        baseScore = 75;
    }
    else if (vy < 8.0)
    {
        baseScore = 50;
    }
    else if (vy < 16.0)
    {
        baseScore = 30;
    }
    else
    {
        baseScore = 15;
    }

    for (auto lz :  landingZones )
    {
        if (x >= lz.x - 2 && x <= lz.x + lz.width + 2)
        {
            return baseScore * lz.bonus;
        }
    }

    for (auto p : flatBits )
    {
        if (x >= p.first - 1 && x < p.second + p.first + 1)
        {
            return baseScore;
        }
    }
    return 0;
}


// Updates positions of lander and stars based on physics
// Uses world coordinates; scrolling and translation to screen coordinates happens in the redraw methods
void LunarLanderGame::physics(unsigned long timestamp)
{

    // Determine how long has passed since we last calculated the physics, in seconds
    if (lastPhysicsTimestamp == 0)
    {
        lastPhysicsTimestamp = timestamp;
    }
    
    unsigned long timePassed = (timestamp - lastPhysicsTimestamp) / timeDilationFactor;
    if (timePassed == 0)
    {
        return;
    }
      
    lastPhysicsTimestamp = timestamp;

    lander.updateLanderBurn(timePassed);
   
    landerLocation.x = lander.locX();
    landerLocation.y = lander.locY();

    // If lander landed, work out if it was a success, and pause
    unsigned short index = (landerLocation.x + WORLD_WIDTH) & WORLD_X_MASK;
    unsigned short landerHeight = landerOffsets[lander.angle + TheEagle::deltav.size()].height;
    currentGroundHeight = altitudes[index];

    if (landerLocation.y - landerHeight <= currentGroundHeight)
    {
        DebugPrintf("lander velocity (%f, %f); at (%d, %d) - screen (%d, %d); ground height %d (index %d), camera (%d, %d), zoom %d, landerHeight %d\r\n",
            lander.velocityX, lander.velocityY, landerLocation.x, landerLocation.y, 
            worldCoordToScreenX(landerLocation, currentCamera), worldCoordToScreenY(landerLocation, currentCamera),
            currentGroundHeight, index, currentCamera.x, currentCamera.y, currentCamera.zoom, landerHeight);

        int gameScore = ScoreAtWorldX(index, lander.velocityX, lander.velocityY);
        if (gameScore > 0 && lander.angle == 0)
        {
            stats.score += gameScore;
            won = true;
        }
        else
        {
            stats.score += 5;
            lost = true;
        }

        Preferences preferences;
        preferences.begin("Lander", false);
        preferences.clear();
        if (won)
        {
            stats.wins++;
        }
        stats.played++;

        preferences.putUInt("score", stats.score);
        preferences.putUInt("wins", stats.wins);
        preferences.putUInt("played", stats.played);
        preferences.end();
        
        lander.enginesLevel = 0;
    }
}

// update the display.  scroll if needed.  Zoom if needed.
void LunarLanderGame::redrawScreen(bool eraseLastFrame)
{
    if (paused)
    {
        return;
    }

    if (eraseLastFrame)
    {
        previousCamera.CopyFrom(currentCamera);  

        bool scrolled = false;
        bool zoomChanged = false;

        if (currentCamera.zoom == 1 && landerLocation.y < ZOOM_HEIGHT)
        {
            zoomChanged = true;
            currentCamera.zoom = 2;
            timeDilationFactor = 4.0;
        }
        else if (currentCamera.zoom == 2 && landerLocation.y >= ZOOM_HEIGHT)
        {
            zoomChanged = true;
            currentCamera.zoom = 1;
            timeDilationFactor = 2.0;
        }
        
        if (worldCoordToScreenX(landerLocation, previousCamera) != BORDER)
        {
            currentCamera.x = (landerLocation.x + WORLD_WIDTH - BORDER / previousCamera.zoom) & WORLD_X_MASK;  // good
            scrolled = true;
        }

        if (SCREEN_HEIGHT - landerLocation.y < TOP_BORDER + previousCamera.y)
        {
            currentCamera.y = landerLocation.y + TOP_BORDER - SCREEN_HEIGHT;
            scrolled = true;
            DebugPrintf("vertical scroll: current camara (%d, %d)\r\n", currentCamera.x, currentCamera.y);
            // scroll up
        } 
        else if (SCREEN_HEIGHT - landerLocation.y > TOP_BORDER + previousCamera.y && previousCamera.y > 0)
        {
            // scroll down
            currentCamera.y = std::min(0, landerLocation.y + TOP_BORDER - SCREEN_HEIGHT);
            DebugPrintf("not used: scrolling down: current camara (%d, %d)\r\n", currentCamera.x, currentCamera.y);
            scrolled = true;
        }

        if (zoomChanged || scrolled)
        {
            currentCamera.setZoom(currentCamera.zoom);
        }
        
        // Erase stars.  They can vary in magnitude when we redraw, but not move relative to mountains etc.
        for (auto i = 0; i < starsDrawn; i++)
        {
            tft.fillCircle(starsOnScreen[i].x, starsOnScreen[i].y, 1, colors.ColorBackground);
        }
        starsDrawn = 0;
    }

    // bool out = false;

    for (int i = 0; i < stars.size(); i++)
    {
        if (currentCamera.onScreenX(stars[i].x) && currentCamera.onScreenY(stars[i].y))
        {
            starsOnScreen[starsDrawn].x = worldCoordToScreenX(stars[i], currentCamera);
            starsOnScreen[starsDrawn].y = worldCoordToScreenY(stars[i], currentCamera);
            tft.fillCircle(starsOnScreen[starsDrawn].x, starsOnScreen[starsDrawn].y, (random(7) % 3 == 0 ? 1 : 0), colors.ColorForeground); 
            starsDrawn++;
        }
    }
    
    if (currentCamera.y > std::max(TOP_BORDER, BORDER) && (currentCamera.y != previousCamera.y || currentCamera.x != previousCamera.x))
    {
        if (currentCamera.y > 0)
        {
            for (int i = 0; i < highStars.size(); i++)
            {
                if (highStars[i].y > SCREEN_HEIGHT - currentCamera.y && currentCamera.onScreenX(highStars[i].x))
                {
                    starsOnScreen[starsDrawn].x = worldCoordToScreenX(highStars[i], currentCamera);
                    starsOnScreen[starsDrawn].y = (highStars[i].y + currentCamera.y) % SCREEN_HEIGHT;
                    tft.fillCircle(starsOnScreen[starsDrawn].x, starsOnScreen[starsDrawn].y, (random(7) % 3 == 0 ? 1 : 0), colors.ColorForeground); 
                    starsDrawn++; 
                }
            }
        }
    }
        
    // Redraw lunar surface.  When we are scrolling, erase the old line by drawing it black as we go.   
    if (eraseLastFrame)
    {
        if (currentCamera.x != previousCamera.x || currentCamera.y != previousCamera.y || currentCamera.zoom != previousCamera.zoom)
        {
            drawMountains(colors.ColorBackground, previousCamera); 
            eraseLandingZones(previousCamera);  
        }
    }
    drawMountains(colors.ColorForeground, currentCamera);

    if (won || lost)
    {
        drawLandingZones(currentCamera);
    }
          
    // Draw lander where it actually is.   The first coordinate should be approximately the middle of the screen.
    // the y coord is the issue.
    
    DebugPrintf("zoomed lander is (%d, %d); lander location is (%d, %d); y: (%d), altitude %d\r\n",
        worldCoordToScreenX(landerLocation, currentCamera), 
        SCREEN_HEIGHT - lander.locY() * 2 + currentCamera.y, 
        landerLocation.x,
        landerLocation.y, 
        worldCoordToScreenY(landerLocation, currentCamera),
        altitudes[landerLocation.x]);


    drawLander(worldCoordToScreenX(landerLocation, currentCamera),
               worldCoordToScreenY(landerLocation, currentCamera),
               lander.enginesLevel,
               lander.angle,
               currentCamera.zoom);

    switch (lander.enginesLevel)
    {
        case 0:
            digitalWrite(ENGINE_LED_1, 0);
            digitalWrite(ENGINE_LED_2, 0);
            digitalWrite(ENGINE_LED_3, 0);
            break;
        case 1:
            digitalWrite(ENGINE_LED_1, 1);
            digitalWrite(ENGINE_LED_2, 0);
            digitalWrite(ENGINE_LED_3, 0);
            break;
        case 2:
        case 3:
            digitalWrite(ENGINE_LED_1, 1);
            digitalWrite(ENGINE_LED_2, 1);
            digitalWrite(ENGINE_LED_3, 0);
            break;
        default:
            digitalWrite(ENGINE_LED_1, 1);
            digitalWrite(ENGINE_LED_2, 1);
            digitalWrite(ENGINE_LED_3, 1);
            break;
    }
}

// draw mountains given the currentCamera
void LunarLanderGame::drawMountains(uint16_t color, const GameCamera & cam )
{
    if (cam.zoom == 1)
    {
        for (int i = 0; i < SCREEN_WIDTH; i++)
        {
            int index = (cam.x + WORLD_WIDTH + i + 1) & WORLD_X_MASK;
            tft.drawLine(i, (SCREEN_HEIGHT - altitudes[index] + cam.y - LOWER_BORDER), 
                    ((i + 1) & WORLD_X_MASK), (SCREEN_HEIGHT - altitudes[(index + 1) &WORLD_X_MASK] + cam.y - LOWER_BORDER), color);
        }
    }
    else
    {
        for (int i = 0; i < SCREEN_WIDTH / 2; i++)
        {
            int index = (cam.x + WORLD_WIDTH + i + 2) & WORLD_X_MASK; 
            tft.drawLine(i << 1, (SCREEN_HEIGHT - (altitudes[index] << 1) + cam.y - LOWER_BORDER), 
                    (i << 1) + 2, (SCREEN_HEIGHT - (altitudes[(index + 1) & WORLD_X_MASK] << 1) - LOWER_BORDER), color);
        }
    }
}

// angle is index into array of offsets for line1 (x1, y1, x2, y2), line2, leg1, leg2 and burn triangle with unit 1
// draw the lander centered at (x,y).  These are screen coordinates.  zoom tells the size.
void LunarLanderGame::drawLanderHelper(int x, int y, int burn, int angle, uint16_t landerColor, uint16_t burnColor, short zoom)
{
    if (burn < 0)
    {
        return;
    }

    if (zoom == 1)
    {
        tft.drawCircle(x, y, 3, landerColor);
    }
    else
    {
        tft.drawCircle(x, y, 6, landerColor);
    }

    unsigned int angleCount = TheEagle::deltav.size();

    const LanderPoints *offsets;

    if (zoom == 2)
    {
        offsets = &(landerOffsets[angle]);
    }
    else
    {
        offsets = &(landerOffsets[angle + angleCount]);
    }
   
    tft.drawLine(x + offsets->line1x1, y + offsets->line1y1, x + offsets->line1x2, y + offsets->line1y2, landerColor);
    tft.drawLine(x + offsets->line2x1, y + offsets->line2y1, x + offsets->line2x2, y + offsets->line2y2, landerColor);
    tft.drawLine(x + offsets->leg1x1, y + offsets->leg1y1, x + offsets->leg1x2, y + offsets->leg1y2, landerColor);
    tft.drawLine(x + offsets->leg2x1, y + offsets->leg2y1, x + offsets->leg2x2, y + offsets->leg2y2, landerColor);

    if (burn > 0)
    {
        tft.fillTriangle(x + offsets->leg1x1, y + offsets->leg1y1, x + offsets->leg2x1, y + offsets->leg2y1, 
          x + offsets->burnx3 * burn, y + offsets->burny3 * burn, burnColor);
    }       
}

// screen coordinates, use the given zoom factor (1 or 2)
void LunarLanderGame::drawLander(int x, int y, int burn, int angle, short zoom)
{
    if (burn < 0)
    {
          return;
    }
          
    if (lastLanderAngle != -1)
    {
        drawLanderHelper(lastLanderLocation.x, lastLanderLocation.y, lastLanderBurn, lastLanderAngle, colors.ColorBackground, colors.ColorBackground, lastLanderZoom);
    }

    if (burn < 0)
    {
        return;
    }
    
    if (angle > 16 || angle < 0)
    {
        return;
    }

    if (won)
    {
        DebugPrintf("won; lander at (%d, %d)\r\n", x, y);
    }

    if (!lost)
    {
        drawLanderHelper(x, y, burn, angle, colors.ColorForeground, colors.ColorBurn, zoom);
    }
    else
    {
        DebugPrintf("lost; explosion at (%d, %d)\r\n", x, y);
        drawExplosion(x, y);
    }
    
    lastLanderLocation.x = x;
    lastLanderLocation.y = y;
    lastLanderAngle = angle;
    lastLanderBurn = burn;
    lastLanderZoom = zoom;
} 

// updates the text on the screen and sets "paused" when the game is over.  No zooming or anything fancy.
void LunarLanderGame::updateText(unsigned long timestamp)
{
// Draw the current frame
    int16_t  x1, y1;
    uint16_t w, h;
    
    tft.setFont();  // standard font
    tft.setCursor(0, 0);
    tft.setTextColor(colors.ColorForeground, colors.ColorBackground);  tft.setTextSize(1);
    tft.println("SCORE:");
    tft.setCursor(0, TEXT_SPACING);
    tft.println("TIME:");
    tft.setCursor(0, TEXT_SPACING*2);
    tft.println("FUEL:");
    tft.setCursor(RIGHT_TEXT_COL, 0);
    tft.println("ALTITUDE:");
    tft.setCursor(RIGHT_TEXT_COL, TEXT_SPACING);
    if (lander.velocityX > 0)
    {
        tft.println("Vx:    ->");
    }
    else
    {
        tft.println("Vx:    <-");
    }
    tft.setCursor(RIGHT_TEXT_COL, TEXT_SPACING*2);
    if (lander.velocityY < 0)
    {
        tft.println("Vy:    ^ ");
    }
    else
    {
        tft.println("Vy:    v ");
    }
    // now print values
    tft.setCursor(LABEL_WIDTH, 0);
    tft.printf("%4d", stats.score);
    tft.setCursor(LABEL_WIDTH, TEXT_SPACING);
    tft.printf("%d:%02d", timestamp / (1000 * 60), (timestamp /1000) % 60);
    //tft.println("0:00");  // not used
    tft.setCursor(LABEL_WIDTH, TEXT_SPACING * 2);

    if (lander.fuelLow())
    {
        tft.setTextColor(colors.ColorBurn, colors.ColorBackground);
    }
    else
    {
        tft.setTextColor(colors.ColorForeground, colors.ColorBackground);
    }

    tft.fillRect(LABEL_WIDTH, TEXT_SPACING*2, 24, 8, colors.ColorBackground);
    tft.printf("%d%%", lander.fuelRemaining());
    tft.setTextColor(colors.ColorForeground, colors.ColorBackground);
    tft.setCursor(RIGHT_TEXT_COL + LABEL_WIDTH, 0);
    tft.fillRect(RIGHT_TEXT_COL + LABEL_WIDTH, 0, 54, 8, colors.ColorBackground);
    tft.printf("%d m", landerLocation.y - currentGroundHeight);
    tft.setCursor(RIGHT_TEXT_COL + LABEL_WIDTH, TEXT_SPACING); 
    tft.fillRect(RIGHT_TEXT_COL + LABEL_WIDTH, TEXT_SPACING, 54, 8, colors.ColorBackground);
    tft.printf("%4.1f m/s", fabs(lander.velocityX / 4)); 
    tft.setCursor(RIGHT_TEXT_COL + LABEL_WIDTH, TEXT_SPACING*2);
    tft.fillRect(RIGHT_TEXT_COL + LABEL_WIDTH, TEXT_SPACING*2, 54, 8, colors.ColorBackground);
    tft.printf("%4.1f m/s", fabs(lander.velocityY / 4));
    DebugPrintf("lander velocity (%f, %f) - abs is (%f, %f)\r\n", lander.velocityX, lander.velocityY, fabs(lander.velocityX), fabs(lander.velocityY));

    // Won/lost status display 
    if (won)
    {
        tft.setCursor(8, SCREEN_HEIGHT/2 - 40);
        tft.setTextColor(colors.ColorWin, colors.ColorBackground);  tft.setTextSize(2);
        tft.println("You win!");
    }
    else if (lost)
    {
        tft.setCursor(8, SCREEN_HEIGHT/2 - 40);
        tft.setTextColor(colors.ColorBurn, colors.ColorBackground);  tft.setTextSize(2);
        tft.println("You lose");
    }
}

// clear the mysterious error code that flashes on the screen
void LunarLanderGame::clearErrorCode()
{   
    int16_t  x1, y1;
    uint16_t w, h;
    tft.setFont(&FreeMonoBoldOblique12pt7b); 

    tft.getTextBounds(ErrorCodes[errorCode], TEXT_SPACING, SCREEN_HEIGHT-TEXT_SPACING, &x1, &y1, &w, &h);
    tft.fillRect(x1, y1, w, h, colors.ColorBackground);
    tft.setFont();
    errorCode = 0;
}

// show an error code during operation
void LunarLanderGame::showErrorCode()
{
    tft.setFont(&FreeMonoBoldOblique12pt7b); 

    tft.setCursor(TEXT_SPACING, SCREEN_HEIGHT-TEXT_SPACING);
    tft.setTextColor(colors.ColorBurn); 
    tft.println(ErrorCodes[errorCode]);
    tft.setFont();
}

// choose an error code
void LunarLanderGame::setRandomError(unsigned long now)
{
    if (errorCode == 0 || now - lastErrorSet > ERROR_CHANGE_DELAY)
    {
        lastErrorSet = now;
        if (errorCode != 0)
        {
            clearErrorCode();
        }
        errorCode = random(ErrorCodes.size());
        if (errorCode != 0)
        {
            showErrorCode();
        }
    }
}

// erase any visible landing zones
void LunarLanderGame::eraseLandingZones(const GameCamera &cam)
{
    tft.setFont();
    int16_t x1, y1;
    uint16_t w, h;
    
    int j = 0;
    for (auto lz : landingZones)
    {
        if (!cam.onScreenX(lz.x))
        {
            continue;
        }

        if (landingZoneBonus[j] > 1)
        {
            tft.getTextBounds(lz.bonusText, 
                          worldCoordToScreenX(lz, cam),
                          worldCoordToScreenY(lz, cam) + 2, 
                          &x1, &y1, &w, &h);
            tft.fillRect(x1, y1, w, h, colors.ColorBackground);
        }
        tft.fillRect(worldCoordToScreenX(lz, cam), worldCoordToScreenY(lz, cam), lz.width * cam.zoom, 2 * cam.zoom, colors.ColorBackground);
        j++;
    }
}

// draw the landing zones
void LunarLanderGame::drawLandingZones(const GameCamera & cam)
{
    tft.setFont();
    int16_t x1, y1;
    uint16_t w, h;
    int j = 0;
    for (auto lz : landingZones)
    {
        if (!cam.onScreenX(lz.x))
        {
            continue;
        }

        tft.fillRect(worldCoordToScreenX(lz, cam), worldCoordToScreenY(lz, cam), lz.width * cam.zoom, 2 * cam.zoom, colors.ColorLander);
        if (landingZoneBonus[j] > 1)
        { 
            if ((((lz.x + WORLD_WIDTH - cam.x) - 12) & WORLD_X_MASK) * cam.zoom < SCREEN_WIDTH) 
            {
                tft.setCursor(worldCoordToScreenX(lz, cam), worldCoordToScreenY(lz, cam) + 2);
                tft.setTextColor(colors.ColorLander, colors.ColorBackground);
                tft.setTextSize(1);
                tft.println(lz.bonusText);
            }     
        }
        j++;
    }
}

void LunarLanderGame::drawExplosion(uint16_t x, uint16_t y)
{
    DebugPrintf("drawing explosion at (%d, %d)\r\n", x, y);
    for (auto triangle : explosionTriangles)
    {
        tft.fillTriangle(triangle.p1.x + x, triangle.p1.y + y,
            triangle.p2.x + x, triangle.p2.y + y, triangle.p3.x + x, triangle.p3.y + y, colors.ColorExplosion);
    }
}

