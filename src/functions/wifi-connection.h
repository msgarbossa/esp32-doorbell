#ifndef TASK_WIFI_CONNECTION
#define TASK_WIFI_CONNECTION

#include <Arduino.h>
#include "WiFi.h"
#include "wifi-connection.h"
#include "../config/config.h"

// This allows lower-level WiFi settings (wifi_set_phy_mode, system_phy_set_max_tpw)
// extern "C" {
//     #include "user_interface.h"
// }


void connectWiFi(){

    unsigned long startAttemptTime = millis();
    unsigned short elapsedAttemptTime = 0;
    // WiFi.setPhyMode(WIFI_PHY_MODE_11G);
    WiFi.printDiag(Serial);
    // WiFi.setTxPower(powervalue max 82);
    // WiFi.setPhyMode(WIFI_PHY_MODE_11G);
    // wifi_set_phy_mode(PHY_MODE_11G);
    // system_phy_set_max_tpw(powervalue max 82); // divide by 4 for dBm value
    // Name            Speed    Indoor Range
    // Wireless AC      1 Gbps      115 Feet
    // Wireless N     300 Mbps      230 Feet
    // Wireless G      54 Mbps      125 Feet
    // Wireless B      11 Mbps      115 Feet

    WiFi.mode(WIFI_STA);
    WiFi.setHostname(DEVICE_NAME);

    unsigned short retryMs = 400;
    while (WiFi.status() != WL_CONNECTED && elapsedAttemptTime < WIFI_TIMEOUT) {
        elapsedAttemptTime = millis() - startAttemptTime;
        Serial.println("[WIFI] connecting (" + String(elapsedAttemptTime) + "ms)");
        WiFi.begin(WIFI_NETWORK, WIFI_PASSWORD);
        delay(retryMs);
        retryMs = retryMs * 2;
    }

    elapsedAttemptTime = millis() - startAttemptTime;
    // Make sure that we're actually connected
    if(WiFi.status() != WL_CONNECTED){
        Serial.println("[WIFI] FAILED (" + String(elapsedAttemptTime) + "ms): ");
    } else {
        Serial.println("[WIFI] connected (" + String(elapsedAttemptTime) + "ms): ");
        Serial.print(WiFi.localIP());
        Serial.print(F(" "));
        Serial.println(WiFi.macAddress());
    }
}

void disconnectWiFi() {
    if (WiFi.status() == WL_CONNECTED) {
        WiFi.disconnect();
        WiFi.mode(WIFI_OFF);
        Serial.println("[WIFI] disconnect/off complete");
    }
}


#endif