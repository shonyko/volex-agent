#ifndef MQTT_H
#define MQTT_H

#include <AsyncMqttClient.h>
#include <map>
#include <queue>

#define MQTT_HOST "volex.local"
#define MQTT_PORT 1883

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
  Serial.println(payload);
  // eventQueue.push({.topic = String(topic), .payload = String(payload)});
}

// void onMqttPublish(uint16_t packetId) {
//   Serial.println("Publish acknowledged.");
//   Serial.print("  packetId: ");
//   Serial.println(packetId);
// }
} // namespace

// void subscribe(const char *topic, TopicHandler handler = prettyPrintHandler)
// {
//   Serial.print("Subscribing to: ");
//   Serial.println(topic);
//   // topicHandlers.emplace(String(topic), handler);
//   mqttClient.subscribe(topic, 0);
// }

void unsubscribe(const char *topic) {
  Serial.print("Unsubscribing from: ");
  Serial.println(topic);
  mqttClient.unsubscribe(topic);
  // topicHandlers.erase(String(topic));
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