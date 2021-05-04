#include <Arduino.h>

#include "../config/config.h"

const int freq1 = 2000;
const int freq2 = 2500;
const int channel = 0;
const int resolution = 8;

void setup_sound() {
  pinMode(SOUND_PIN, OUTPUT);
  ledcSetup(channel, freq1, resolution);
  ledcAttachPin(SOUND_PIN, channel);
}

void play_sound() {
  Serial.println("[SOUND] Triggered");
  ledcWriteTone(0,freq1);
  vTaskDelay(250 / portTICK_PERIOD_MS);
  ledcWriteTone(0,freq2);
  vTaskDelay(250 / portTICK_PERIOD_MS);
  ledcWrite(channel,0);
}

void blink_now() {
    digitalWrite(LED_PIN, HIGH);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    digitalWrite(LED_PIN, LOW);
}
