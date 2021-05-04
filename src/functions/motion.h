
#include <Arduino.h>
#include "../config/config.h"

extern int motionState;
extern bool motionChanged;
extern unsigned long prevMillisMotion;

void motionSetup() {
    // PIR Motion Sensor mode INPUT_PULLUP
    pinMode(MOTION_PIN, INPUT_PULLUP);

    Serial.println("[MOTION] setup complete");
}

portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
int motionStateLocal = 0;
void motionMonitor(void * parameter) {

    // Wait 1 seconds to give time for WiFi and MQTT to start
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    for(;;) {
        motionStateLocal = digitalRead(MOTION_PIN);
        if(motionStateLocal == HIGH ) {
            vTaskEnterCritical(&mux);
            if(motionState == 0) {
                motionState = 1;
                // Serial.println("[MOTION] detected");    
                motionChanged = true;
            }
            vTaskExitCritical(&mux);
            prevMillisMotion = millis();
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }

        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
}
