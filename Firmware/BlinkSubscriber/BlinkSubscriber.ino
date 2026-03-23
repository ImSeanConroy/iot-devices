/*
 * ============================================================
 * Project: IoT Device (Blink Subscriber)
 * Developer: Sean Conroy
 * Board: Seeed Studio Xiao ESP32C3
 * License: MIT
 * Description:
 *   - Connects to AWS IoT Core using MQTT over a secure connection.
 *   - Subscribes to a predefined topic to receive real-time messages.
 *   - Parses color command payloads and drives an external WS2812 LED.
 *   - Supports named color states and an off state.
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

struct RgbColor {
  uint8_t red;
  uint8_t green;
  uint8_t blue;
};

#define WS2812_PIN 10
#define WS2812_LED_COUNT 1
#define WS2812_BRIGHTNESS 64

static constexpr RgbColor kWs2812Red = {255, 0, 0};
static constexpr RgbColor kWs2812Green = {0, 255, 0};
static constexpr RgbColor kWs2812Blue = {0, 0, 255};
static constexpr RgbColor kWs2812Purple = {128, 0, 255};
static constexpr RgbColor kWs2812Pink = {255, 20, 147};
static constexpr RgbColor kWs2812Orange = {255, 140, 0};
static constexpr RgbColor kWs2812Yellow = {255, 255, 0};

unsigned long lastMqttRetryMs = 0;

Adafruit_NeoPixel pixels(WS2812_LED_COUNT, WS2812_PIN, NEO_RGB + NEO_KHZ800);

void connectWiFi();
void connectMQTT();
void messageHandler(String &topic, String &payload);
void setColor(const RgbColor &color);
void setColor(uint8_t red, uint8_t green, uint8_t blue);

void setColor(const RgbColor &color) {
  setColor(color.red, color.green, color.blue);
}

void setColor(uint8_t red, uint8_t green, uint8_t blue) {
  uint32_t color = pixels.Color(red, green, blue);
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

  if (strcmp(action, "set_color") == 0) {
    if (strcmp(state, "red") == 0) {
      setColor(kWs2812Red);
    } else if (strcmp(state, "green") == 0) {
      setColor(kWs2812Green);
    } else if (strcmp(state, "blue") == 0) {
      setColor(kWs2812Blue);
    } else if (strcmp(state, "purple") == 0) {
      setColor(kWs2812Purple);
    } else if (strcmp(state, "pink") == 0) {
      setColor(kWs2812Pink);
    } else if (strcmp(state, "orange") == 0) {
      setColor(kWs2812Orange);
    } else if (strcmp(state, "yellow") == 0) {
      setColor(kWs2812Yellow);
    } else if (strcmp(state, "off") == 0) {
      setColor(0, 0, 0);
    } else {
      Serial.println("Ignoring message: unknown color state.");
      return;
    }

    Serial.print("LED color set: ");
    Serial.println(state);
  } else {
    Serial.println("Ignoring message: expected action=set_color.");
  }
}

void setup() {
  Serial.begin(115200);
  pixels.begin();
  pixels.setBrightness(WS2812_BRIGHTNESS);
  setColor(0, 0, 0);
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