/*
 */

#include "Idler.h"
#include "BleLander.h"

// run the idler as a separate task, pinned to a different CPU (0)
// this should not conflict with BLE.
// if it does, try pinning it to CPU 1.

TaskHandle_t IdleTask;
Idler * Idler::s_instance = nullptr;
const std::vector<unsigned short> Idler::idleSteps =  {0, 1, 3, 7, 6, 4, 0, 1, 2, 4, 2, 1};

/* The idle task runs continuously.  Since it's the only task running on CPU0,
 * it can block.  It also unblocks BLE if it locks up for some reason.
 */
void TaskIdle( void * pvParameters ) {
    unsigned long now;

    for (;;)
    {
        now = millis();
        Idler *idleInstance = Idler::getInstance();
        if (idleInstance->isIdling())
        {
            idleInstance->nextIdleStep(now);
        }
        if (idleInstance->isScanning() && now > idleInstance->stopTime()
            && idleInstance->bleLander() != nullptr)
        {
            // Serial.println("Force STOP scanning");
            // idleInstance->bleLander()->setEnabled(false);
            // delay(50);
            // Serial.println("RESTART bt");
            // idleInstance->bleLander()->setEnabled(true);
        }
        delay(IDLE_LIGHT_INTERVAL);
    } 
}

/*
 * Create the task, pinned to core 0.  Probably doesn't need this much stack.
 */
void Idler::init()
{
    bleLanderL = nullptr;
    idling = false;
    scanning = false;
    idleLightStep = 0;
    lightChange = 0;
    scanStopTime = 0;

	xTaskCreatePinnedToCore(
                    TaskIdle,    /* Task function. */
                    "IdleTask",  /* name of task. */
                    10000,       /* Stack size of task */
                    NULL,        /* parameter of the task */
                    1,           /* priority of the task */
                    &IdleTask,   /* Task handle to keep track of created task */
                    0);          /* pin task to core 0 */   
}

/* Next step.
 */
void Idler::nextIdleStep(unsigned long now)
{
	// idle program
    // if (now > lightChange )
    // {
    //     lightChange = now + IDLE_LIGHT_INTERVAL;

        if (idleLightStep >= idleSteps.size())
        {
            idleLightStep = 0;
        }
        digitalWrite(ENGINE_LED_1, (idleSteps[idleLightStep] & 0x1));
        digitalWrite(ENGINE_LED_2, (idleSteps[idleLightStep] & 0x2));
        digitalWrite(ENGINE_LED_3, (idleSteps[idleLightStep] & 0x4));

        idleLightStep++;
    // }
}

/* Effectively turn the idle process on and off.
 */
void Idler::setIdle(bool idleNow)
{
	if (!idling  && idleNow)
	{
		// lightChange = millis();
		idleLightStep = 0;
	}
    idling = idleNow;    
}

void Idler::setScanning(bool scan)
{
    if (!scanning && scan)
    {
        scanStopTime = millis() + 10000;
    }
    scanning = scan;
}
