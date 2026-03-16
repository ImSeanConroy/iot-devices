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
static const unsigned long PUBLISH_INTERVAL_MS = 60000;
static const unsigned long LOOP_DELAY_MS = 10;

unsigned long lastPublishMs = 0;
unsigned long lastMqttRetryMs = 0;
unsigned long sequence = 0;

void connectWiFi();
void connectMQTT();
void publishMessage();

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

void publishMessage() {
  StaticJsonDocument<256> doc;
  doc["event"] = "produce";
  doc["thing"] = THINGNAME;
  doc["sequence"] = sequence++;
  doc["uptime_ms"] = millis();

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

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
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

  unsigned long now = millis();
  if (client.connected() && (now - lastPublishMs >= PUBLISH_INTERVAL_MS)) {
    lastPublishMs = now;
    publishMessage();
  }

  delay(LOOP_DELAY_MS);
}