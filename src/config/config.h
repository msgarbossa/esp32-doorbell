#ifndef CONFIG
#define CONFIG

#define DEVICE_NAME "doorbell"

// WiFi
#define WIFI_NETWORK "SSID"
#define WIFI_PASSWORD "PW"
#define WIFI_TIMEOUT 20000 // 20 seconds
#define WIFI_RECOVER_TIME_MS 1000 // (1-20 seconds)

// MQTT
#define MQTT_HOST IPAddress(X, X, X, X)
#define MQTT_PORT 1883
#define MQTT_USER "user"
#define MQTT_PASSWORD "password"
#define MQTT_CONNECT_DELAY 200
#define MQTT_CONNECT_TIMEOUT 10000 // 10 seconds
#define MQTT_PUBLISH_SEC 300 // 5 minutes

#define LED_PIN 2 // for activity blink
#define SOUND_PIN 32 // for alerts

// NTP
#define NTP_SERVER "10.10.1.1"
#define NTP_GMT_OFFSET_SECONDS -25200
#define NTP_DAYLIGHT_OFFSET_SECONDS 0
#define NTP_UPDATE_INTERVAL_MS 3600000

// Doorbell
#define DOORBELL_BUTTON_PIN 26
#define DOORBELL_DND_ENABLED true  // Do Not Disturb, requires NTP_ENABLED if true
#define DOORBELL_DND_START_HR 20  // silences hr >= this number
#define DOORBELL_DND_END_HR 7  // silences hr <= this number
#define DOORBELL_RESET_SEC 3  // Minimum time between ring messages (10)

// Motion sensor configurations
#define MOTION_ENABLED true
#define MOTION_PIN 27
#define MOTION_OFF_SEC 10  // Should be long enough for Prometheus to scrape (65s)

#endif
