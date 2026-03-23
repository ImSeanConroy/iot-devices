/*
 * ============================================================
 * Project: IoT Device (Door Sensor Publisher)
 * Developer: Sean Conroy
 * Board: Seeed Studio Xiao ESP32C3
 * License: MIT
 * Description:
 *   - Connects to AWS IoT Core using MQTT over a secure connection.
 *   - Debounces a reed switch input and publishes state changes.
 *   - Includes reconnect logic for Wi-Fi and MQTT to stay online.
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
static const unsigned long DEBOUNCE_MS = 35;

#define REED_PIN 9

unsigned long lastMqttRetryMs = 0;
unsigned long sequence = 0;
unsigned long lastDebounceTimeMs = 0;

int stableDoorState = HIGH;
int lastDoorReading = HIGH;

void connectWiFi();
void connectMQTT();
void connectAWS();
void handleDoorSensor();
bool publishDoorState(const char* eventType, int state);

const char* doorStateText(int state) {
  // INPUT_PULLUP wiring: HIGH=open (idle), LOW=closed (magnet present)
  return state == HIGH ? "open" : "closed";
}

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
  net.setCACert(AWS_CERT_CA);
  net.setCertificate(AWS_CERT_CRT);
  net.setPrivateKey(AWS_CERT_PRIVATE);

  client.begin(AWS_IOT_ENDPOINT, 8883, net);

  connectWiFi();
  connectMQTT();
}

bool publishDoorState(const int state) {
  StaticJsonDocument<256> doc;
  doc["message_type"] = "telemetry";
  doc["event"] = "door_changed";
  doc["thing"] = THINGNAME;
  doc["state"] = doorStateText(state);
  doc["value"] = state;
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

  return ok;
}

void handleDoorSensor() {
  int reading = digitalRead(REED_PIN);

  if (reading != lastDoorReading) {
    lastDoorReading = reading;
    lastDebounceTimeMs = millis();
  }

  if ((millis() - lastDebounceTimeMs) >= DEBOUNCE_MS && reading != stableDoorState) {
    stableDoorState = reading;
    Serial.print("Door changed: ");
    Serial.println(doorStateText(stableDoorState));

    if (client.connected()) {
      publishDoorState(stableDoorState);
    } else {
      Serial.println("MQTT not connected. Skipping publish.");
    }
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(REED_PIN, INPUT_PULLUP);

  stableDoorState = digitalRead(REED_PIN);
  lastDoorReading = stableDoorState;

  connectAWS();
  Serial.print("Initial door state: ");
  Serial.println(doorStateText(stableDoorState));

  if (client.connected()) {
    publishDoorState("door_startup", stableDoorState);
  }
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
  handleDoorSensor();

  delay(LOOP_DELAY_MS);
}