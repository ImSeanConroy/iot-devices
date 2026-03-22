/*
 * ============================================================
 * Project: IoT Device (Producer)
 * Developer: Sean Conroy
 * Board: Seeed Studio Xiao ESP32C3
 * License: MIT
 * Description:
 *   - Connects to AWS IoT Core using MQTT over a secure connection.
 *   - Publishes periodic JSON messages to a predefined topic.
 *   - Includes reconnect logic for Wi-Fi and MQTT to stay online.
 *   - Designed as a reusable producer/test publisher for IoT flows.
 *   - Uses WiFi credentials and certificates defined in "secrets.h".
 * ============================================================
 */

#include "secrets.h"
#include <WiFiClientSecure.h>
#include <MQTTClient.h>
#include <ArduinoJson.h>
#include "WiFi.h"

WiFiClientSecure net = WiFiClientSecure();
MQTTClient client = MQTTClient(256);

static const unsigned long WIFI_RETRY_MS = 500;
static const unsigned long MQTT_RETRY_MS = 2000;
static const unsigned long LOOP_DELAY_MS = 10;
static const unsigned long BUTTON_DEBOUNCE_MS = 35;
static const char *COLOR_STATES[] = {"red", "green", "blue", "purple", "pink", "orange", "yellow", "off"};
static const size_t COLOR_STATE_COUNT = sizeof(COLOR_STATES) / sizeof(COLOR_STATES[0]);

#define BUTTON_PIN 9
#define BUTTON_ACTIVE_STATE LOW

unsigned long lastMqttRetryMs = 0;
unsigned long sequence = 0;
int lastPublishedColorIndex = -1;

int stableButtonState = HIGH;
int lastButtonReading = HIGH;
unsigned long lastButtonTransitionMs = 0;

void connectWiFi();
void connectMQTT();
void publishMessage(const char *state);
void handleButton();

void connectWiFi() {
  if (WiFi.status() == WL_CONNECTED) {
    return;
  }

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(WIFI_RETRY_MS);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("Wi-Fi connected.");
}

void connectMQTT() {
  if (client.connected()) {
    return;
  }

  Serial.println("Connecting to AWS IoT...");
  while (!client.connect(THINGNAME)) {
    Serial.print(".");
    delay(MQTT_RETRY_MS);
  }
  Serial.println();
  Serial.println("AWS IoT connected.");
}

void connectAWS() {
  // Configure WiFiClientSecure to use the AWS IoT device credentials
  net.setCACert(AWS_CERT_CA);
  net.setCertificate(AWS_CERT_CRT);
  net.setPrivateKey(AWS_CERT_PRIVATE);

  // Connect to the MQTT broker on the AWS endpoint we defined earlier
  client.begin(AWS_IOT_ENDPOINT, 8883, net);

  connectWiFi();
  connectMQTT();
}

void publishMessage(const char *state) {
  StaticJsonDocument<256> doc;
  doc["action"] = "set_color";
  doc["state"] = state;
  doc["sequence"] = sequence++;

  String payload;
  serializeJson(doc, payload);

  bool ok = client.publish(AWS_IOT_PUBLISH_TOPIC, payload);
  if (ok) {
    Serial.print("Published to ");
    Serial.print(AWS_IOT_PUBLISH_TOPIC);
    Serial.print(": ");
    Serial.println(payload);
  } else {
    Serial.println("Publish failed.");
  }
}

void handleButton() {
  int reading = digitalRead(BUTTON_PIN);

  if (reading != lastButtonReading) {
    lastButtonReading = reading;
    lastButtonTransitionMs = millis();
  }

  if ((millis() - lastButtonTransitionMs) >= BUTTON_DEBOUNCE_MS && reading != stableButtonState) {
    stableButtonState = reading;

    if (stableButtonState == BUTTON_ACTIVE_STATE) {
      Serial.println("Button pressed. Publishing random color event.");
      if (client.connected()) {
        int nextColorIndex = random(COLOR_STATE_COUNT);
        if (COLOR_STATE_COUNT > 1 && nextColorIndex == lastPublishedColorIndex) {
          nextColorIndex = (nextColorIndex + 1 + random(COLOR_STATE_COUNT - 1)) % COLOR_STATE_COUNT;
        }

        const char *nextColor = COLOR_STATES[nextColorIndex];
        publishMessage(nextColor);
        lastPublishedColorIndex = nextColorIndex;
      } else {
        Serial.println("MQTT not connected. Skipping publish.");
      }
    }
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  stableButtonState = digitalRead(BUTTON_PIN);
  lastButtonReading = stableButtonState;

  randomSeed(micros());

  connectAWS();
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }

  if (!client.connected()) {
    unsigned long now = millis();
    if (now - lastMqttRetryMs >= MQTT_RETRY_MS) {
      lastMqttRetryMs = now;
      connectMQTT();
    }
  }

  client.loop();
  handleButton();

  delay(LOOP_DELAY_MS);
}