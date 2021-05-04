
#include <Arduino.h>
#include "../config/config.h"
#include "../functions/attention.h"
#include <time.h>

extern unsigned long prevMillisDoorbell;
extern int doorbellStatus;
extern bool doorbellChanged;
extern time_t now;
extern struct tm * timeinfo;

int doorbellStatusPrev;

void doorbellSound() {
    #if DOORBELL_DND_ENABLED == true
        // Check time from local clock
        time(&now);
        timeinfo = localtime(&now);
        // Serial.print("[DOORBELL] hour=");
        // Serial.println(timeinfo->tm_hour);
        // Compare scheduling settings against time
        if(timeinfo->tm_hour >= DOORBELL_DND_START_HR) {
            Serial.println("[DOORBELL] silencing sound due to DND");
            return;
        } else if(timeinfo->tm_hour <= DOORBELL_DND_END_HR) {
            Serial.println("[DOORBELL] silencing sound due to DND");
            return;
        }
    #endif
    play_sound();
}

portMUX_TYPE mox = portMUX_INITIALIZER_UNLOCKED;

void setupDoorbell() {
    pinMode(DOORBELL_BUTTON_PIN, INPUT);
    Serial.println("[DOORBELL] setup complete");
}

int buttonStateLocal = 0;
void doorbellMonitor(void * parameter) {

    // pinMode(DOORBELL_BUTTON_PIN, INPUT);

    // Wait 1 seconds to give time for WiFi and MQTT to start
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    for(;;) {
        buttonStateLocal = digitalRead(DOORBELL_BUTTON_PIN);
        if(buttonStateLocal == HIGH ) {
            vTaskEnterCritical(&mox);
            if(doorbellStatus == 0) {
                // Serial.println("[DOORBELL] button pressed");                                                             
                doorbellChanged = true;
            }
            doorbellStatus = 1;  
            prevMillisDoorbell = millis();
            vTaskExitCritical(&mox);
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }

        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}