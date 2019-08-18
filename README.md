# Lunar Lander Badge

![BadgeSAO](/images/withSAO.png "Lunar Lander Badge with SAO attached")

The software was written using Arduino, with the help of some esp32 native libraries where needed. The esp32 libraries are based on FreeRTOS.

## Hardware

__Warning__ The Lunar Lander badge uses a LIPO battery. Please be careful not to puncture or incinerate it, due to the risk of fire / explosions.  Rockets are launched using explosives, but it is better for you and your badge if you avoid it.

The hardware schematic is [here](/schematic.pdf "here").

It is similar to an Adafruit Feather, used in early prototyping. The additions are a TFT LCD screen, five buttons, seven LEDs and the SAO port for expansion. The esp32 communicates with the screen using SPI. The board incorporates a charging circuit for a LIPO battery (provided with the badge).

The code uses BLE (Bluetooth Low Energy) from the esp32, but not WiFi (though it's not blocked).

The hardware pinouts are defined in hardware.h. Four of the defined pins are not currently in use but are connected to the SAO port. The I2C connections (SDA / SCL) used for the SAO do not have additional pull-up resistors on the board, so may work best with SAOs that include their own resistors. Since the current software doesn't use I2C it is available for use with SAOs that support serial. There are two additional GPIO pins connected to the SAO.

### First look

#### SAO connector
If you are soldering on a SAO connector, the tab must be at the front, as shown in the silkscreen.  This is a keyed connector.

![SAO1](/images/SAO1.png "SAO silkscreen")

It should look like this when assembled:

![SAO2](/images/SAO2.png "SAO attached to badge")

Some tarnish on the silver is normal.  It is due to the manufacturing process.

#### Attaching the battery

Batteries ship separately, and are not fully charged.  The badge has a built-in LIPO charger, so you can connect the battery to charge it.  The badge starts working when the charge is 3.2V (nominal LIPO battery voltage is 3.7V; it is usually in the range 3.2 - 4.2V).

The battery is attached using double-sided tape (included).

Connect the battery (make sure the connector is the correct way up; the red wire is closer to the edge of the board).

![BadgeTape](/images/tape.png "tape and battery connections")

Cut the tape into two strips as shown. 

Remove one side of the paper covering the tape, and attach the tape to the battery.

![BadgeTape2](/images/BatteryTape.png "Battery ready to attach to badge")

Remove the other side of the tape, then carefully place the battery on the badge.  The tape should not be on the screen connector.  

Don’t press too hard because you could damage the screen.

There will be space between the battery and the badge; this is by design.

![BadgeSandwich](/images/sandwich.png "space between badge and battery")

If the battery is low there is a warning:

![BatteryWarning](/images/batteryWarning.png "Battery Warning")

#### Screen Cover
The screen is shipped with a thin film covering it.  You’ll find the display looks better without it.

## Software
The software is mostly written in C++. We pre-compute values where possible to avoid complex calculations.

### Building and Loading the code

Download and install Arduino. You will also need the esp32 packages. The BLE libraries on the esp32 use a considerable amount of memory, so the code _will not_ load with the standard Adafruit Feather board definition. You need to edit boards.txt, and repeat it when you upgrade Arduino or the esp32 libraries. The changes below add a new partition scheme to a standard Adafruit Feather. The partition disables OTA (over the air update) which is not used by this code.

    ~/Library/Arduino15/packages/esp32/hardware/esp32/1.0.2/boards.txt

    $ diff boards.txt orig_boards.txt
    1406,1412d1405
    < featheresp32.menu.PartitionScheme.default=Default 4MB with spiffs (1.2MB APP/1.5MB SPIFFS)
    < featheresp32.menu.PartitionScheme.default.build.partitions=default
    < featheresp32.menu.PartitionScheme.huge_app=Huge APP (3MB No OTA/1MB SPIFFS)
    < featheresp32.menu.PartitionScheme.huge_app.build.partitions=huge_app
    < featheresp32.menu.PartitionScheme.huge_app.upload.maximum_size=3145728

When you build using the Arduino IDE, select the "Adafruit ESP32 Feather" as the board, and "Huge APP (3MB No OTA/1MB SPIFFS)" as the Partition Scheme (if you build using other tools, hopefully you know what you are doing).

### Software Components

#### lunarLander.ino
The main Arduino code, consisting of setup() and loop() functions. Setup creates all the apps and initializes everything. The main event loop doesn't use a delay(); instead it first checks for button presses and processes them, then if it's been a while since the last action, it will call the currently-active app. There is too much app-specific code here, but that's the general idea. It uses the bitmap stored in landerImage. It also displays the credits.

#### App
Defines the interface that should be implemented by any apps or games running on the badge. SettingsApp, LunarLanderGame, FriendsApp and CommanderGame all implement App.

#### BLELander
The code that interacts with bluetooth. If you want to listen for results from scanning for friends, implement BleFriendObserver.

#### Idler
The Idler keeps the lights on (literally) while the badge is doing other things. It runs as a separate task, pinned to CPU0. Arduino code usually runs on CPU1.

#### LunarLanderGame
The reason we are here. This is a clean-room re-implementation of the Lunar Lander game. Enjoy! It uses LunarLanderConstants.h for the moonscape and the pre-calculated Lunar Lander.

#### Settings
The settings screen allows you to change the name on the badge, turn bluetooth on or off, and adjust the brightness (and colors, if unlocked).

#### CommanderGame
Where would the Lunar Lander be without the command module?  Don't overlook this one.

It uses a greyscale image of the moon, greyMoon.cpp.

#### FriendsApp
The friends app is where you can connect with other badges and share your score.

