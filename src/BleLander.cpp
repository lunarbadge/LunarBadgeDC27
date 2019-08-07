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

#include "BleLander.h"
#include "Idler.h"

BLEUUID BleLander::bleServiceUUID (LANDER_SERVICE_UUID);
BLEUUID BleLander::bleCharacteristicUUID(BLE_CHARACTERISTIC_UUID);

BLERemoteCharacteristic* BleLander::pRemoteCharacteristic;
BLEAdvertisedDevice* BleLander::myDevice;

// init: called from setup()
void BleLander::init(const char *myName, int myScore)
{
    name = std::string(myName);
    score = myScore;
    if (initialized)
    {
        return;
    }
    BLEDevice::init("Lunar Lander Badge");
    setupServer();
    setupClient();
    initialized = true;
}

// the implementation of the characteristics callbacks.
// onRead is called when the client is reading the characteristic value.
// this gives me the opportunity to update information before it is read,
// for example if the score has updated.
void BleLander::onRead(BLECharacteristic* pCharacteristic)
{
    pCharacteristic->setValue(getAdvertisement().c_str());
}

// onWrite is called when a client has written a new value.
// actually not going to use this at all.
void BleLander::onWrite(BLECharacteristic* pCharacteristic)
{

}

// set up advertising
void BleLander::setupServer()
{
    BLEServer  *pServer  = BLEDevice::createServer();
    BLEService *pService = pServer->createService(bleServiceUUID);
    BLECharacteristic *pCharacteristic = pService->createCharacteristic(
                                         bleCharacteristicUUID,
                                         BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_WRITE
                                       );

    pCharacteristic->setValue(getAdvertisement().c_str());
    pCharacteristic->setCallbacks(this);
    pService->start();
    BLEAdvertising *pAdvertising = pServer->getAdvertising();
    pAdvertising->addServiceUUID(bleServiceUUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
    pAdvertising->setMinPreferred(0x12);
    //pAdvertising->start();    // start advertising when enabled.
}

void BleLander::processScanResults(BLEScanResults results)
{
    Serial.println("processing scan results");
    for (uint32_t i = 0; i < results.getCount(); i++)
    {
        myDevice = new BLEAdvertisedDevice(results.getDevice(i));
        //Serial.println("Checking device");
        // Serial.println(results.getDevice(i).toString().c_str());
        if (advertisedDeviceIsMine(*myDevice))
        {
            connectToServer();
            if (i < results.getCount() - 1)
                delay(20);  // need to delay to let the connection reset
        }
    }
}

// assumes the scanner is already set up
void BleLander::scanForFriends()
{
    if (!btOn)
    {
        return;
    }

    scanning = true;
    Idler::getInstance()->setBle(this);
    Idler::getInstance()->setScanning(true);
    BLEScan* pBLEScan = BLEDevice::getScan();
    Serial.println("commence scan");
    // this will block if it has less than three args.  Clears the previous results
    processScanResults(pBLEScan->start(10, false));
    pBLEScan->stop();
    Idler::getInstance()->setScanning(false);
    scanning = false;
}

// used to turn bluetooth on and off.
// starts advertising but does not start scanning.
// start scanning from friends.
// will stop scanning.
void BleLander::setEnabled(const bool enabled)
{
    if (enabled == btOn)
    {
        return;
    }

    if (enabled)
    {
        BLEDevice::startAdvertising();
    }
    else
    {
        BLEScan* pBLEScan = BLEDevice::getScan();
        BLEDevice::getAdvertising()->stop();
        Idler::getInstance()->setScanning(false);
        pBLEScan->stop();
        scanning = false;
    } 
    btOn = enabled;
}


// onConnect() and onDisconnect() are called when connections are made (or broken)
// probably nothing to do here.
// I don't need to implement these at all.

void BleLander::onConnect(BLEClient* pclient)
{
    DebugPrintln("on connect");
}

void BleLander::onDisconnect(BLEClient* pclient)
{
    DebugPrintln("onDisconnect");
}

// connect to a server, when found.
bool BleLander::connectToServer()
{
    BLEClient*  pClient = BLEDevice::createClient();
    
    pClient->setClientCallbacks(this);

    // Connect to the remote BLE Server.
    pClient->connect(myDevice); 
    
    // Obtain a reference to the service we are after in the remote BLE server.
    BLERemoteService* pRemoteService = pClient->getService(bleServiceUUID);
    if (pRemoteService == nullptr)
    {
        DebugPrintf("Failed to find our service UUID: %s\r\n", bleServiceUUID.toString().c_str());
        pClient->disconnect();
        return false;
    }
    
    DebugPrintln("Looking for remote characteristic");
    // Obtain a reference to the characteristic in the service of the remote BLE server.
    pRemoteCharacteristic = pRemoteService->getCharacteristic(bleCharacteristicUUID);
    if (pRemoteCharacteristic == nullptr)
    {
        Serial.println(bleCharacteristicUUID.toString().c_str());
        pClient->disconnect();
        return false;
    }
    
    DebugPrintln("Reading remote characteristic");
    // Read the value of the characteristic.
    if(pRemoteCharacteristic->canRead())
    {
        std::string value = pRemoteCharacteristic->readValue();
        Serial.printf("Found: %s\r\n", value.c_str());
        
        // this is where I call back the observer with the string.
        if (observer != nullptr)
        {
            observer->onScanResult(String(value.c_str()));
        }
    }
    else
    {
        pClient->disconnect();
        return false;
    }
    pClient->disconnect();

    return true;
}

bool BleLander::advertisedDeviceIsMine(BLEAdvertisedDevice &advertisedDevice)
{
    DebugPrintf("Found: %s; ", advertisedDevice.toString().c_str());

    // We have found a device, let us now see if it contains the service we are looking for.
    if (!advertisedDevice.haveServiceUUID())
    {
        DebugPrintf("%s (%s) has no service UUID\r\n", advertisedDevice.getName().c_str(), advertisedDevice.getManufacturerData().c_str());
        return false;
    }
    BLEUUID service = advertisedDevice.getServiceUUID();

    if (service.equals(bleServiceUUID))
    {
        return true;
    }
    return false;
}

 /**
   * Called for each result when scanning.  We could stop the scan here if we only want one
   * result.
   */
void BleLander::onResult(BLEAdvertisedDevice advertisedDevice)
{
    if (advertisedDeviceIsMine(advertisedDevice))
    {
        DebugPrintln("; scanning device...");
        BLEDevice::getScan()->stop();   // we find one then we stop scanning. But this is not useful.
        myDevice = new BLEAdvertisedDevice(advertisedDevice);

      // can I look for new ones only?  actually that is the default.  We want old ones too.
        // actually should have a list of devices to connect to, if we can't connect to them in the
        // callback due to latency.
    }
    else
    {
        DebugPrintf(" %s; not this one\r\n", advertisedDevice.getServiceDataUUID().toString().c_str());
    }

}  // onResult

// set up scanning for servers
void BleLander::setupClient()
{  
    // Retrieve a Scanner and set the callback we want to use to be informed when we
    // have detected a new device.  Specify that we don't want active scanning, but don't
    // start the scanner yet.
    DebugPrintln("setting up client");
    BLEScan* pBLEScan = BLEDevice::getScan();
    //pBLEScan->setAdvertisedDeviceCallbacks(this, true);   // we are implementing onResult, and we want duplicates
    pBLEScan->setInterval(1349);
    pBLEScan->setWindow(449);
    pBLEScan->setActiveScan(true); // active scan uses more power; set it to false.
    DebugPrintln("set up");
} // End of setup.


// This is the main loop function.
// we should advertise for 1s, every 5s?
// we advertise whenever BT is on, at the moment.
void BleLander::doLoop(unsigned long now)
{
    
} // End of loop
