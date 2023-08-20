#ifndef MQTT_H
#define MQTT_H

#include <ArduinoJson.h>
#include <AsyncMqttClient.h>
#include <map>
#include <queue>

#define MQTT_HOST "volex.local"
#define MQTT_PORT 1883

namespace Mqtt {
std::shared_ptr<boolean> dependency = nullptr;
};

AsyncMqttClient mqttClient;

typedef std::function<void(const String &)> TopicHandler;

// Map to associate topics with their corresponding handlers
std::map<String, TopicHandler> mqttTopicHandlers;

typedef struct {
  String topic;
  String payload;
} MqttEvent;

typedef struct {
  std::function<void()> onConnect;
  std::function<void()> onDisconnect;
} MqttConfig;

std::queue<MqttEvent> mqttEventQueue;

namespace {
MqttConfig config;

void _onMqttConnect(bool sessionPresent) {
  Serial.println("Connected to MQTT broker!");
  Mqtt::dependency = std::make_shared<boolean>(true);
  if (config.onConnect != nullptr) {
    config.onConnect();
  }
}

void _onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
  Serial.print("Disconnected from MQTT: ");
  switch (reason) {
  case AsyncMqttClientDisconnectReason::TCP_DISCONNECTED:
    Serial.println("TCP connection lost");
    break;
  case AsyncMqttClientDisconnectReason::MQTT_UNACCEPTABLE_PROTOCOL_VERSION:
    Serial.println("Unacceptable protocol version");
    break;
  case AsyncMqttClientDisconnectReason::MQTT_IDENTIFIER_REJECTED:
    Serial.println("Identifier rejected");
    break;
  case AsyncMqttClientDisconnectReason::MQTT_SERVER_UNAVAILABLE:
    Serial.println("Server unavailable");
    break;
  case AsyncMqttClientDisconnectReason::MQTT_MALFORMED_CREDENTIALS:
    Serial.println("Malformed credentials");
    break;
  case AsyncMqttClientDisconnectReason::MQTT_NOT_AUTHORIZED:
    Serial.println("Not authorized");
    break;
  case AsyncMqttClientDisconnectReason::ESP8266_NOT_ENOUGH_SPACE:
    Serial.println("Not enough space on ESP8266");
    break;
  case AsyncMqttClientDisconnectReason::TLS_BAD_FINGERPRINT:
    Serial.println("TLS bad fingerprint");
    break;
  default:
    Serial.println("unknown cause");
    break;
  }
  if (Mqtt::dependency != nullptr) {
    *Mqtt::dependency = false;
  }

  std::queue<MqttEvent>().swap(mqttEventQueue);

  if (config.onDisconnect != nullptr) {
    config.onDisconnect();
  }
}

// void onMqttSubscribe(uint16_t packetId, uint8_t qos) {
//   Serial.println("Subscribe acknowledged.");
//   Serial.print("  packetId: ");
//   Serial.println(packetId);
//   Serial.print("  qos: ");
//   Serial.println(qos);
// }

// void onMqttUnsubscribe(uint16_t packetId) {
//   Serial.println("Unsubscribe acknowledged.");
//   Serial.print("  packetId: ");
//   Serial.println(packetId);
// }

void onMqttMessage(char *topic, char *payload,
                   AsyncMqttClientMessageProperties properties, size_t len,
                   size_t index, size_t total) {
  Serial.println("Received message from MQTT broker:");
  Serial.print("  topic: ");
  Serial.println(topic);
  Serial.print("  payload: ");
  Serial.println(String(payload, len));
  Serial.print("  len: ");
  Serial.println(len);
  Serial.print("  index: ");
  Serial.println(index);
  Serial.print("  total: ");
  Serial.println(total);
  mqttEventQueue.push(
      {.topic = String(topic), .payload = String(payload, len)});
}

// void onMqttPublish(uint16_t packetId) {
//   Serial.println("Publish acknowledged.");
//   Serial.print("  packetId: ");
//   Serial.println(packetId);
// }
} // namespace

void prettyPrintHandler(const String &payload) {
  StaticJsonDocument<256> doc;
  auto err = deserializeJson(doc, payload);
  if (err) {
    Serial.println("Failed to parse JSON");
    return;
  }
  String json;
  serializeJsonPretty(doc, json);
  Serial.println("Received:");
  Serial.println(json);
  Serial.println("====");
  doc.clear();
}

void handle(const MqttEvent &e) {
  Serial.print("Starting to handle: ");
  Serial.println(e.topic);
  auto handlerIter = mqttTopicHandlers.find(e.topic);
  if (handlerIter == mqttTopicHandlers.end()) {
    Serial.println("No handler found");
    return;
  }
  handlerIter->second(e.payload);
}

void subscribe(const char *topic, TopicHandler handler = prettyPrintHandler) {
  Serial.print("Subscribing to: ");
  Serial.println(topic);
  mqttTopicHandlers.emplace(String(topic), handler);
  mqttClient.subscribe(topic, 0);
  Serial.print("Subscribed to: ");
  Serial.println(topic);
}

void unsubscribe(const char *topic) {
  Serial.print("Unsubscribing from: ");
  Serial.println(topic);
  mqttClient.unsubscribe(topic);
  mqttTopicHandlers.erase(String(topic));
}

void mqtt_setup(MqttConfig user_config) {
  config = user_config;
  mqttClient.onConnect(_onMqttConnect);
  mqttClient.onDisconnect(_onMqttDisconnect);
  // mqttClient.onSubscribe(onMqttSubscribe);
  // mqttClient.onUnsubscribe(onMqttUnsubscribe);
  mqttClient.onMessage(onMqttMessage);
  // mqttClient.onPublish(onMqttPublish);
}

void setMqttAddr(IPAddress ip) { mqttClient.setServer(ip, MQTT_PORT); }

void connectToMqtt() {
  Serial.println("Connecting to MQTT...");
  mqttClient.connect();
}

#endif