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

#include "lander.h"
#include <BLEDevice.h>

class Idler;

#pragma once

// the interface used when you want results from the BLE scanner.
// implement this class and pass to it to BleLander's setObserver
// to get called back when it finds friends.
class BleFriendObserver
{
    public:
        virtual void onScanResult(String result) = 0;
};

// BLE scanner and advertise.
class BleLander : public BLEClientCallbacks, public BLEAdvertisedDeviceCallbacks, public BLECharacteristicCallbacks
{
    public:

        BleLander() : btOn(false), lastRun(0), initialized(false), observer(nullptr), scanning(false) {};

        // init: called from setup.
        void init(const char *myName, int myScore);

        void doLoop(unsigned long now);

        bool connectToServer();

        void setName(const char *myName)
        {
            name = std::string(myName); 
            DebugPrintf("set name to %s\r\n", name.c_str());
        };

        //myScore is the total score (from lunarlandergame.cpp)
        void setScore(int myScore)
        {
            score = myScore;
            advertisement = "";
        };

        void setEnabled(const bool enable);

        bool isEnabled() const
        {
            return btOn;
        };

        const bool isScanning() const { return scanning; } ;

        void setObserver(BleFriendObserver *obs)
        {
            observer = obs;
        }

        void scanForFriends() ;

        const String &getAdvertisement()
        {
            if (advertisement.length() == 0)
            {
                if (name.length() == 0)
                {
                    advertisement = "anonymous:" + score;
                }
                else
                {
                    advertisement = String(name.c_str()) + ":";
                    advertisement = advertisement + score;
                }
            }
            DebugPrintf("setting advertisement to %s\r\n", advertisement.c_str());
            return advertisement;
        }

        // BLEClientCallbacks.  Not really needed.
        void onConnect(BLEClient* pclient) override;
        void onDisconnect(BLEClient* pclient) override;

        // BLEAdvertisedDeviceCallbacks.  We get this callback when a result is found from scanning, because
        // we call pBLEScan->setAdvertisedDeviceCallbacks(this).
        void onResult(BLEAdvertisedDevice advertisedDevice) override;

        // BLECharacteristicCallbacks.  This was useful for debugging but not needed now.
        virtual void onRead(BLECharacteristic* pCharacteristic) override;
        virtual void onWrite(BLECharacteristic* pCharacteristic) override;

    private:
        bool btOn;
        bool initialized;
        bool scanning;
        std::string name;
        int score = 0;
        String advertisement;
        unsigned long lastRun;

        static BLEUUID bleServiceUUID;
        static BLEUUID bleCharacteristicUUID;

        static boolean doScan;
        static BLERemoteCharacteristic* pRemoteCharacteristic;
        static BLEAdvertisedDevice* myDevice;

        BleFriendObserver *observer;

        bool advertisedDeviceIsMine(BLEAdvertisedDevice &advertisedDevice);
        void processScanResults(BLEScanResults results);

        void setupServer();
        void setupClient();

        
};
