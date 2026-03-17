/*
 * ============================================================
 * Project: IoT Device (Subscriber)
 * Developer: Sean Conroy
 * Board: Seeed Studio Xiao ESP32C3
 * License: MIT
 * Description:
 *   - Connects to AWS IoT Core using MQTT over a secure connection.
 *   - Subscribes to a predefined topic to receive real-time messages.
 *   - On receiving a message, an external LED toggles to signal incoming data.
 *   - Designed to enable quiet, non-intrusive notifications (e.g., for calls).
 *   - Uses WiFi credentials and certificates defined in "secrets.h".
 * ============================================================
 */

#include "secrets.h"
#include <WiFiClientSecure.h>
#include <MQTTClient.h>
#include <ArduinoJson.h>
#include <Adafruit_NeoPixel.h>
#include "WiFi.h"

WiFiClientSecure net = WiFiClientSecure();
MQTTClient client = MQTTClient(256);

static const unsigned long WIFI_RETRY_MS = 500;
static const unsigned long MQTT_RETRY_MS = 2000;
static const unsigned long LOOP_DELAY_MS = 10;

#ifndef WS2812_PIN
#ifdef LED_PIN
#define WS2812_PIN LED_PIN
#else
#define WS2812_PIN 10
#endif
#endif

#ifndef WS2812_LED_COUNT
#define WS2812_LED_COUNT 1
#endif

#ifndef WS2812_ON_R
#define WS2812_ON_R 0
#endif

#ifndef WS2812_ON_G
#define WS2812_ON_G 32
#endif

#ifndef WS2812_ON_B
#define WS2812_ON_B 0
#endif

#ifndef WS2812_BRIGHTNESS
#define WS2812_BRIGHTNESS 64
#endif

unsigned long lastMqttRetryMs = 0;

bool ledState = false;
Adafruit_NeoPixel pixels(WS2812_LED_COUNT, WS2812_PIN, NEO_GRB + NEO_KHZ800);

void connectWiFi();
void connectMQTT();
void messageHandler(String &topic, String &payload);
void applyLedState();

void applyLedState() {
  uint32_t color = ledState ? pixels.Color(WS2812_ON_R, WS2812_ON_G, WS2812_ON_B) : pixels.Color(0, 0, 0);
  for (int i = 0; i < WS2812_LED_COUNT; i++) {
    pixels.setPixelColor(i, color);
  }
  pixels.show();
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

  client.subscribe(AWS_IOT_SUBSCRIBE_TOPIC);
  Serial.println();
  Serial.println("AWS IoT connected and subscribed.");
}

void connectAWS() {
  // Configure WiFiClientSecure to use the AWS IoT device credentials
  net.setCACert(AWS_CERT_CA);
  net.setCertificate(AWS_CERT_CRT);
  net.setPrivateKey(AWS_CERT_PRIVATE);

  // Connect to the MQTT broker on the AWS endpoint we defined earlier
  client.begin(AWS_IOT_ENDPOINT, 8883, net);

  // Create a message handler
  client.onMessage(messageHandler);

  connectWiFi();
  connectMQTT();
}

void messageHandler(String &topic, String &payload) {
  Serial.println();
  Serial.println("Incoming: " + topic + " - " + payload);

  StaticJsonDocument<256> doc;
  DeserializationError error = deserializeJson(doc, payload);
  if (error) {
    Serial.print("Payload is not JSON: ");
    Serial.println(error.c_str());
    return;
  }

  const char *action = doc["action"] | "";
  const char *state = doc["state"] | "";

  if (strcmp(action, "set_led") == 0 && (strcmp(state, "on") == 0 || strcmp(state, "off") == 0)) {
    ledState = (strcmp(state, "on") == 0);
    applyLedState();
    Serial.print("LED set: ");
    Serial.println(ledState ? "ON" : "OFF");
  } else {
    Serial.println("Ignoring message: expected action=set_led and state=on|off.");
  }
}

void setup() {
  Serial.begin(115200);
  pixels.begin();
  pixels.setBrightness(WS2812_BRIGHTNESS);
  applyLedState();
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

  delay(LOOP_DELAY_MS);
}