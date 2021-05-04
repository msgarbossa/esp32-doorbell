#include <Arduino.h>
#include <WiFi.h>
extern "C" {
  #include "freertos/FreeRTOS.h"
  #include "freertos/timers.h"
}
#include <AsyncMqttClient.h>
#include <time.h>

#include "config/config.h"
#include "functions/wifi-connection.h"
#include "functions/ota.h"
#include "functions/doorbell.h"
#include "functions/mqtt.h"
#include "functions/motion.h"

// #include "tasks/motion-detect.h"

#ifdef __arm__
// should use uinstd.h to define sbrk but Due causes a conflict
extern "C" char* sbrk(int incr);
#else  // __ARM__
extern char *__brkval;
#endif  // __arm__

// Variables to hold metric/sensor values
int doorbellStatus = 0;
bool doorbellChanged = false;
int motionState = 0;
bool motionChanged = false;
unsigned long prevMillisMotion = 0; // Stores last motionState = 1
unsigned long prevMillisDoorbell = 0;  // Stores last doorbellStatus = 1
unsigned long prevMillisMQTT = 0;   // Stores last time sensor was published
unsigned long prevNtpMillis = 0;   // Stores last time NTP was updated

// Variables to hold sensor info
int8_t wifi_strength;
IPAddress ip;
time_t now;
struct tm * timeinfo;
char TimeString[9]; // 8 digits plus the null char (00:00:00)


// Temperature MQTT Topics
#define MQTT_PUB_TOPIC "home/" DEVICE_NAME "/metrics"
#define MQTT_SUB_TOPIC "home/" DEVICE_NAME "/cmd"

AsyncMqttClient mqttClient;
TimerHandle_t mqttReconnectTimer;
TimerHandle_t wifiReconnectTimer;
// TimerHandle_t checkDoorTimer;
// TimerHandle_t checkLeakTimer;

const char* wl_status_to_string(int status) {
  switch (status) {
    case WL_NO_SHIELD: return "WL_NO_SHIELD";
    case WL_IDLE_STATUS: return "WL_IDLE_STATUS";
    case WL_NO_SSID_AVAIL: return "WL_NO_SSID_AVAIL";
    case WL_SCAN_COMPLETED: return "WL_SCAN_COMPLETED";
    case WL_CONNECTED: return "WL_CONNECTED";
    case WL_CONNECT_FAILED: return "WL_CONNECT_FAILED";
    case WL_CONNECTION_LOST: return "WL_CONNECTION_LOST";
    case WL_DISCONNECTED: return "WL_DISCONNECTED";
    default: return "UNKNOWN";
  }
}

void connectToWifi() {
  btStop(); // turn off bluetooth
  Serial.println("Connecting to Wi-Fi...");
  WiFi.begin(WIFI_NETWORK, WIFI_PASSWORD);
}

void connectToMqtt() {
  Serial.println("Connecting to MQTT...");
  mqttClient.connect();
}

void WiFiEvent(WiFiEvent_t event) {
  Serial.printf("[WiFi-event] event: %d\n", event);
  switch(event) {
    case SYSTEM_EVENT_STA_GOT_IP:
      Serial.println("WiFi connected");
      Serial.println("IP address: ");
      ip = WiFi.localIP();
      Serial.println(WiFi.localIP());
      connectToMqtt();
      break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
      Serial.println("WiFi lost connection");
      xTimerStop(mqttReconnectTimer, 0); // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
      xTimerStart(wifiReconnectTimer, 0);
      break;
    default:
      break;
  }
}

void updateWiFiSignalStrength() {
  if(WiFi.isConnected()) {
    //serial_println(F("[WIFI] Updating signal strength..."));
    wifi_strength = WiFi.RSSI();
    // Serial.print("[WIFI] signal strength: ");
    // Serial.println(wifi_strength);
  }
}

void onMqttConnect(bool sessionPresent) {
  Serial.println("Connected to MQTT.");
  Serial.print("Session present: ");
  Serial.println(sessionPresent);
  uint16_t packetIdSub = mqttClient.subscribe(MQTT_SUB_TOPIC, 2);
  Serial.print("Subscribing at QoS 2, packetId: ");
  Serial.println(packetIdSub);
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
  Serial.println("Disconnected from MQTT.");
  if (WiFi.isConnected()) {
    xTimerStart(mqttReconnectTimer, 0);
  }
}

void onMqttPublish(uint16_t packetId) {
  Serial.print("Publish acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
}

void sendMqttUpdate() {
  char msg[58];  // variable for MQTT payload
  sprintf(msg, "{\"s\":\"%d\",\"d\":\"%d\",\"m\":\"%d\"}", wifi_strength, doorbellStatus, motionState);
  uint16_t packetIdPub1 = mqttClient.publish(MQTT_PUB_TOPIC, 0, false, msg);
  Serial.printf("[MQTT] Publishing on topic %s at QoS 1, packetId: %i\n", MQTT_PUB_TOPIC, packetIdPub1);
  Serial.println(msg);
}

void onMqttSubscribe(uint16_t packetId, uint8_t qos) {
  Serial.println("Subscribe acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
  Serial.print("  qos: ");
  Serial.println(qos);
}
void onMqttUnsubscribe(uint16_t packetId) {
  Serial.println("Unsubscribe acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
}

void onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {
  Serial.println("Publish received.");
  Serial.print("  topic: ");
  Serial.println(topic);
  Serial.print("  qos: ");
  Serial.println(properties.qos);
  Serial.print("  dup: ");
  Serial.println(properties.dup);
  Serial.print("  retain: ");
  Serial.println(properties.retain);
  Serial.print("  len: ");
  Serial.println(len);
  Serial.print("  index: ");
  Serial.println(index);
  Serial.print("  total: ");
  Serial.println(total);

  char s_payload[len + 1];
  String payload2 = "";
  if (len < 20) {
    Serial.print("  payload: ");
    s_payload[len] = '\0';
    strncpy(s_payload, payload, len);
    Serial.println(s_payload);
    payload2 = s_payload;
  } else {
    Serial.println(" payload len>20");
  }
  if (payload2 == "blink") {
    blink_now();
    prevMillisMQTT = 0;
  }
  if (payload2 == "toggle") {
    play_sound();
  }

}


void fetchTimeFromNTP() {
  if(!WiFi.isConnected()) {
      return;
  }
  Serial.println("[NTP] Updating...");
  // init and get the time
  configTime(NTP_GMT_OFFSET_SECONDS, NTP_DAYLIGHT_OFFSET_SECONDS, NTP_SERVER);
  Serial.println("[NTP] Done");
}

void setup() {
  Serial.begin(115200);
  Serial.println();

  pinMode(LED_PIN, OUTPUT);  // Initialize the LED pin as an output

  // sound setup
  setup_sound();

  mqttReconnectTimer = xTimerCreate("mqttTimer", pdMS_TO_TICKS(2000), pdFALSE, (void*)0, reinterpret_cast<TimerCallbackFunction_t>(connectToMqtt));
  wifiReconnectTimer = xTimerCreate("wifiTimer", pdMS_TO_TICKS(2000), pdFALSE, (void*)0, reinterpret_cast<TimerCallbackFunction_t>(connectToWifi));

  WiFi.onEvent(WiFiEvent);

  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  mqttClient.onSubscribe(onMqttSubscribe);
  mqttClient.onUnsubscribe(onMqttUnsubscribe);
  mqttClient.onMessage(onMqttMessage);
  mqttClient.onPublish(onMqttPublish);
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  mqttClient.setCredentials(MQTT_USER, MQTT_PASSWORD);
  connectToWifi();
  setupOTA();
  fetchTimeFromNTP();

  setupDoorbell();
  xTaskCreate(
      doorbellMonitor,    // Function
      "Doorbell Status",  // Task name
      5000,                  // Stack size (bytes)
      NULL,                   // Parameter
      3,                      // Task priority
      NULL                    // Task handle
    );

  #if MOTION_ENABLED == true
      motionSetup();
      xTaskCreate(
        motionMonitor,    // Function
        "Motion Status",  // Task name
        5000,                  // Stack size (bytes)
        NULL,                   // Parameter
        4,                      // Task priority
        NULL                    // Task handle
    );
  #endif
  
  vTaskDelay(1000 / portTICK_PERIOD_MS);
}

void printTime() {
    time(&now);
    timeinfo = localtime(&now);
    sprintf(TimeString, "%02d:%02d:%02d", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
    Serial.print("[TIME] ");
    Serial.println(TimeString);
}

void loop() {
  ArduinoOTA.handle();

  unsigned long currentMillis = millis();
  bool forceMqttUpdate = false;

  // update NTP every NTP_UPDATE_INTERVAL_MS millis
  if (currentMillis - prevNtpMillis >= NTP_UPDATE_INTERVAL_MS) {
    // Save the last time NTP was updated
    prevNtpMillis = currentMillis;
    fetchTimeFromNTP();
  }

  // motion reset
  if(motionState == 1 && (currentMillis - prevMillisMotion > (MOTION_OFF_SEC*1000))) {
    Serial.println("[MOTION] reset");
    motionState = 0;
    printTime();
  }

  // motion alert
  if(motionState == 1 && motionChanged && (currentMillis - prevMillisMotion < (MOTION_OFF_SEC*1000))) {
    Serial.println("[MOTION] ACTIVE");
    printTime();
    forceMqttUpdate = true;
  }

  // doorbell reset
  if(doorbellStatus == 1 && (currentMillis - prevMillisDoorbell > (DOORBELL_RESET_SEC*1000))) {
    Serial.println("[DOORBELL] reset");
    doorbellStatus = 0;
    printTime();
  }

  // doorbell alert
  if(doorbellStatus == 1 && doorbellChanged && (currentMillis - prevMillisDoorbell < (DOORBELL_RESET_SEC*1000))) {
    Serial.println("[DOORBELL] ring!");
    doorbellSound(); // takes into account Do Not Disturb
    blink_now();
    printTime();
    forceMqttUpdate = true;
  }

  // publish a new MQTT message MQTT_PUBLISH_SEC seconds
  if ((currentMillis - prevMillisMQTT >= (MQTT_PUBLISH_SEC * 1000)) || (forceMqttUpdate == true)) {
    // Save the last time a new reading was published
    prevMillisMQTT = currentMillis;
    doorbellChanged = false;
    motionChanged = false;
    blink_now();
    updateWiFiSignalStrength();
    sendMqttUpdate();
  }
}
