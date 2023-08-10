#include <Arduino.h>
#include <CredentialsRetriever.h>
#include <CustomTasks.h>
#include <MyWiFi.h>
#include <Tasks.h>
#include <esp_now.h>
#include <forward_list>
#include <mqtt.h>
#include <stack>
#include <vector>

#define VOLEX_PREFIX "[volex-conn]"
#define AGENT_BLUEPRINT "vlx_led"

// std::vector<Task> tasks;
// std::stack<int> finishedTasks;

// ESP-NOW
uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
esp_now_peer_info_t peerInfo;

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success"
                                                : "Delivery Fail");
}

void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) {
  String dataStr((char *)incomingData);
  // Serial.println("Got: " + dataStr);

  int idx;
  if ((idx = dataStr.indexOf('|')) == -1) {
    Serial.println("Wrong credentials format: " + dataStr);
    return;
  }

  auto ssid = dataStr.substring(0, idx);
  auto pass = dataStr.substring(idx + 1);
  CredentialsRetriever::setCredentials({.ssid = ssid, .pass = pass});
}

void esp_now_setup() {
  WiFi.mode(WIFI_STA);

  if (esp_now_init() != 0) {
    Serial.println("Error initializing ESP-NOW.");
    ESP.restart();
  }

  esp_now_register_send_cb(OnDataSent);
  esp_now_register_recv_cb(OnDataRecv);

  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    ESP.restart();
  }
}

void request_credentials() {
  String s(VOLEX_PREFIX);
  s += AGENT_BLUEPRINT;

  esp_err_t result =
      esp_now_send(broadcastAddress, (uint8_t *)s.c_str(), s.length() + 1);
  if (result != ESP_OK) {
    Serial.println("Error sending the data");
  }
}
// ESP-NOW END

// WiFi
void wifi_try_connect() {
  auto c = CredentialsRetriever::getCredentials();
  Serial.println("Got credentials: " + c.ssid + " | " + c.pass);
  MyWiFi::connectToWifi(c);
}

void onWifiConnect() {
  // tasks.push_back(std::move(DependentTask(
  //     std::vector{MyWiFi::dependency},
  //     [](ITask *task) -> void { setMillis(task, 0); },
  //     [](ITask *task) -> boolean {
  //       auto prevMillis = getMillis(task);
  //       auto currMillis = millis();
  //       if (currMillis - prevMillis >= 2000) {
  //         setMillis(task, currMillis);

  //         Serial.print("Resolving IP address for ");
  //         Serial.print(MQTT_HOST);
  //         Serial.println("...");

  //         auto ip = MyWiFi::resolveBrokerIp(MQTT_HOST);
  //         if (ip == INADDR_NONE) {
  //           Serial.println("Could not find MQTT host. Retrying in 2
  //           seconds"); return false;
  //         }

  //         Serial.print("MQTT host IP address: ");
  //         Serial.println(ip);
  //         setMqttAddr(ip);
  //         return true;
  //       }

  //       return false;
  //     },
  //     [](ITask *task) -> void { connectToMqtt(); })));

  Tasks::queueTask(new DependentTask(
      {MyWiFi::dependency},
      new TimedTask(
          []() {
            Serial.print("Resolving IP address for ");
            Serial.print(MQTT_HOST);
            Serial.println("...");

            auto ip = MyWiFi::resolveBrokerIp(MQTT_HOST);
            if (ip == INADDR_NONE) {
              Serial.println("Could not find MQTT host. Retrying in 2 seconds");
              return false;
            }

            Serial.print("MQTT host IP address: ");
            Serial.println(ip);
            setMqttAddr(ip);
            return true;
          },
          connectToMqtt, 2000)));
}

void onWifiDisconnect() {
  Serial.println("Trying to reconnect in 2 seconds");
  // tasks.emplace_back(
  //     [](ITask *task) -> void { setMillis(task, millis()); },
  //     [](ITask *task) -> boolean { return millis() - getMillis(task) >=
  //     2000;
  //     },s
  //     [](ITask *task) -> void {
  //       MyWiFi::connectToWifi(CredentialsRetriever::getCredentials());
  //     });
  Tasks::setTimeout(wifi_try_connect, 2000);
}
// WiFi END

// MQTT
void onMqttConnect() {
  // Listen for config settings
  // subscribe(WiFi.macAddress().c_str());

  // Request config
  // TODO: make this retry if no answer
  // mqttClient.publish(REQUEST_CONFIG, 0, false, WiFi.macAddress().c_str());

  // TODO:
  // task pentru handle events
  // task-uri separate pentru ciclu citire senzor / transmitere date
}

void onMqttDisconnect() {
  Serial.println("Trying to reconnect in 2 seconds");
  // tasks.push_back(std::move(DependentTask(
  //     std::vector{MyWiFi::dependency},
  //     [](ITask *task) -> void { setMillis(task, millis()); },
  //     [](ITask *task) -> boolean { return millis() - getMillis(task) >=
  //     2000;
  //     },
  //     [](ITask *task) -> void { connectToMqtt(); })));
}
// MQTT END

void setup() {
  Serial.begin(115200);
  Serial.println();

  esp_now_setup();
  MyWiFi::wifi_setup(
      {.onConnect = onWifiConnect, .onDisconnect = onWifiDisconnect});
  mqtt_setup({.onConnect = onMqttConnect, .onDisconnect = onMqttDisconnect});

  auto interval = Tasks::setInterval(request_credentials, 2000);
  Tasks::queueTask(new Tasks::Task(
      Tasks::NoOp, CredentialsRetriever::hasCredentials, [interval]() {
        Tasks::clearInterval(interval);
        wifi_try_connect();
      }));

  // Tasks::setTimeout([interval]() { Tasks::clearInterval(interval); },
  // 6000);

  // Tasks::queueTask(Tasks::Task([]() {}, []() { return true; }, []() {}));
  // Tasks::queueTask(DependentTask(
  //     {MyWiFi::dependency}, []() {}, []() { return true; }, []() {}));
  // DependentTask(
  //     {MyWiFi::dependency}, []() {}, []() { return true; }, []() {});

  // Tasks::queueTask(
  //     DependentTask({MyWiFi::dependency}, DependentTask(
  //                                             {MyWiFi::dependency}, []()
  //                                             {},
  //                                             []() { return true; }, []()
  //                                             {})));

  // Tasks::queueTask(Tasks::Task([]() {}, []() { return true; }, []() {}));

  // DependentTask t(
  //     {MyWiFi::dependency}, []() {}, []() { return true; }, []() {});

  // tasks.emplace_back(
  //     [](ITask *task) -> void {
  //       request_credentials();
  //       setMillis(task, millis());
  //     },
  //     [](ITask *task) -> boolean {
  //       if (CredentialsRetriever::hasCredentials()) {
  //         return true;
  //       }

  //       auto prevMillis = getMillis(task);
  //       auto currMillis = millis();
  //       if (currMillis - prevMillis >= 2000) {
  //         Serial.println("Ask more");
  //         request_credentials();
  //         setMillis(task, currMillis);
  //       }

  //       return false;
  //     },
  //     [](ITask *task) -> void {
  //       auto c = CredentialsRetriever::getCredentials();
  //       Serial.println("Got credentials: " + c.ssid + " | " + c.pass);
  //       MyWiFi::connectToWifi(c);
  //     });
}

// unsigned long previousMillis;

void loop() {
  // wifiLoop();
  Tasks::loop();

  // int idx = 0;
  // for (auto &task : tasks) {
  //   if (!task.isStarted()) {
  //     task.setup();
  //   }

  //   if (task.isFinished()) {
  //     task.finish();
  //     finishedTasks.push(idx);
  //   }

  //   idx++;
  // }

  // while (!finishedTasks.empty()) {
  //   auto idxToRemove = finishedTasks.top();
  //   tasks.erase(tasks.begin() + idxToRemove);
  //   finishedTasks.pop();
  // }

  // auto currentMillis = millis();
  // if (currentMillis - previousMillis >= 2000) {
  //   Serial.println("heartbeat");
  //   String s = VOLEX_PREFIX;
  //   s += AGENT_BLUEPRINT;
  //   esp_err_t result =
  //       esp_now_send(broadcastAddress, (uint8_t *)s.c_str(), s.length() +
  //       1);

  //   if (result == ESP_OK) {
  //     Serial.println("Sent with success");
  //   } else {
  //     Serial.println("Error sending the data");
  //   }
  //   previousMillis = currentMillis;
  // }
}