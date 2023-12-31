#ifndef AGENTS_H
#define AGENTS_H

#include <Arduino.h>
#include <Constants.h>
#include <CustomTasks.h>
#include <Utils.h>
#include <functional>
#include <map>
#include <memory>
#include <mqtt.h>
#include <vector>

namespace Agent {
struct param {
  int id;
  String value;
};

struct input {
  int id;
  int src;
  String value;
  std::function<void(const String &s)> handler;
};

void convertFromJson(JsonVariantConst src, Agent::param &dst) {
  auto json = src.as<JsonObjectConst>();
  dst.id = json["id"].as<int>();
  dst.value = json["value"].as<String>();
}

void convertFromJson(JsonVariantConst src, Agent::input &dst) {
  auto json = src.as<JsonObjectConst>();
  dst.id = json["id"].as<int>();
  dst.src = json["src"].as<int>();
  dst.value = json["value"].as<String>();
}

struct Config {
  int id;
  std::map<int, std::function<void(const String &s)>> params;
  std::map<int, input> inputs;
  std::vector<int> outputs;

  Config(int id, std::map<int, std::function<void(const String &s)>> params,
         std::map<int, input> inputs, std::vector<int> outputs)
      : id(id), params(params), inputs(inputs), outputs(outputs) {}
};

namespace {
std::unique_ptr<Config> config = nullptr;
}; // namespace

std::vector<std::function<void(const String &s)>> getParamHandlers();
std::vector<std::function<void(const String &s)>> getInputHandlers();
void _setup();
void _reset();
void _setupListeners();

bool hasConfig() { return config != nullptr; }

void setup() { _setup(); }

void reset() {
  config = nullptr;
  _reset();
}

void setupListeners() { _setupListeners(); }

void applyConfig(const String &s) {
  Serial.println("Got config: " + s);
  StaticJsonDocument<256> doc;
  auto err = deserializeJson(doc, s);
  if (err) {
    Serial.println("Failed to parse config");
    return;
  }

  int id = doc["id"].as<int>();
  auto idStr = String(id);

  int idx = 0;
  auto param_handlers = getParamHandlers();
  std::map<int, std::function<void(const String &s)>> paramsMap;
  auto param_arr = doc["params"].as<JsonArrayConst>();
  for (auto obj : param_arr) {
    auto param = obj.as<Agent::param>();
    param_handlers[idx](param.value);
    paramsMap[param.id] = param_handlers[idx];
    auto paramId = param.id;
    subscribe(
        ("param/" + String(paramId)).c_str(),
        [paramId](const String &value) { config->params[paramId](value); });
    idx++;
  }

  idx = 0;
  auto input_handlers = getInputHandlers();
  std::map<int, input> inputsMap;
  auto input_arr = doc["inputs"].as<JsonArrayConst>();
  for (auto obj : input_arr) {
    auto input = obj.as<Agent::input>();
    input_handlers[idx](input.value);
    input.handler = input_handlers[idx];
    inputsMap[input.id] = input;
    auto inputId = input.id;
    subscribe(("pin/" + String(inputId)).c_str(),
              [inputId](const String &value) {
                config->inputs[inputId].handler(value);
              });
    subscribe(("pin/" + String(input.src)).c_str(),
              [inputId](const String &value) {
                config->inputs[inputId].handler(value);
              });
    subscribe(("pin/" + String(inputId) + "/src").c_str(),
              [inputId](const String &json) {
                StaticJsonDocument<256> doc;
                auto err = deserializeJson(doc, json);
                if (err) {
                  Serial.println("Failed to parse config");
                  return;
                }

                // Serial.print("Received new source for pin ");
                // Serial.print(inputId);
                // Serial.print(" : ");
                // Serial.println(json);

                auto src = doc.as<Agent::param>();
                auto &input = config->inputs[inputId];
                // Serial.print("Unsubscribing from: ");
                // Serial.println(input.src);
                unsubscribe(("pin/" + String(input.src)).c_str());
                if (src.id) {
                  input.handler(src.value);
                  // Serial.print("Subscribing to: ");
                  // Serial.println(src.id);
                  subscribe(("pin/" + String(src.id)).c_str(), input.handler);
                }
                input.src = src.id;
              });
    idx++;
  }

  std::vector<int> outputs;
  auto outputs_arr = doc["outputs"].as<JsonArrayConst>();
  for (auto out : outputs_arr) {
    outputs.push_back(out.as<int>());
  }

  config = std::make_unique<Config>(id, std::move(paramsMap),
                                    std::move(inputsMap), std::move(outputs));
}

}; // namespace Agent

#ifdef VLX_LED

#define AGENT_BLUEPRINT "vlx_led"

#define LED_PIN 5

namespace Agent {

bool state = false;
int brightness = 100;

void update_led() { ledcWrite(0, state * (brightness / 100.0) * 255); }

void update_power(const String &s) {
  state = trueStr.equalsIgnoreCase(s);
  update_led();
}

void update_brightness(const String &s) {
  brightness = clamp(s.toInt(), 0, 100);
  update_led();
}

std::vector<std::function<void(const String &s)>> getParamHandlers() {
  return {};
};

std::vector<std::function<void(const String &s)>> getInputHandlers() {
  return {update_power, update_brightness};
}

void _setup() {
  pinMode(LED_PIN, OUTPUT);
  ledcSetup(0, 5000, 8); // 12000 pt rgb
  ledcAttachPin(LED_PIN, 0);
  update_led();
}

void _setupListeners() {}

void _reset() {}

} // namespace Agent
#endif

#ifdef VLX_SWITCH

#define AGENT_BLUEPRINT "vlx_switch"

#define SWITCH_PIN 5

namespace Agent {

std::shared_ptr<bool> dependency = std::make_shared<bool>(true);
bool last_state = false;
unsigned long last_millis;

bool publish_on_change = true;
unsigned long publish_period = 5000;

void update_publish_mode(const String &s) {
  publish_on_change = trueStr.equalsIgnoreCase(s);
}
void update_publish_period(const String &s) {
  publish_period = std::strtoul(s.c_str(), NULL, 0);
}

std::vector<std::function<void(const String &s)>> getParamHandlers() {
  return {update_publish_mode, update_publish_period};
};

std::vector<std::function<void(const String &s)>> getInputHandlers() {
  return {};
}

void send_val() {
  auto out_id = config->outputs.at(0);
  auto topic = "pin/" + String(out_id);
  auto payload = last_state ? trueStr.c_str() : falseStr.c_str();
  // Serial.print("out_id: ");
  // Serial.println(out_id);
  // Serial.print("topic: ");
  // Serial.println(topic);
  // Serial.print("payload: ");
  // Serial.println(payload);
  mqttClient.publish(topic.c_str(), 0, false, payload);
  last_millis = millis();
}

void _setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(SWITCH_PIN, INPUT_PULLUP);
}

void _setupListeners() {
  Tasks::queueTask(new DependentTask(
      {dependency}, new Tasks::Task(
                        []() {
                          last_state = digitalRead(SWITCH_PIN);
                          digitalWrite(LED_BUILTIN, last_state);
                          send_val();
                        },
                        []() {
                          auto state = digitalRead(SWITCH_PIN);
                          digitalWrite(LED_BUILTIN, state);
                          if (publish_on_change) {
                            if (state != last_state) {
                              last_state = state;
                              send_val();
                            }
                          } else if (millis() - last_millis >= publish_period) {
                            last_state = state;
                            send_val();
                          }

                          return false;
                        },
                        Tasks::NoOp)));
}

void _reset() {
  *dependency = false;
  dependency = std::make_shared<bool>(true);
}

} // namespace Agent
#endif

#ifdef VLX_SLIDER

#define AGENT_BLUEPRINT "vlx_slider"

#define SLIDER_PIN 34

namespace Agent {

std::shared_ptr<bool> dependency = std::make_shared<bool>(true);
int last_value = 0;
unsigned long last_millis;

bool publish_on_change = true;
unsigned long publish_period = 5000;

void update_publish_mode(const String &s) {
  publish_on_change = trueStr.equalsIgnoreCase(s);
}
void update_publish_period(const String &s) {
  publish_period = std::strtoul(s.c_str(), NULL, 0);
}

std::vector<std::function<void(const String &s)>> getParamHandlers() {
  return {update_publish_mode, update_publish_period};
};

std::vector<std::function<void(const String &s)>> getInputHandlers() {
  return {};
}

void send_val() {
  auto out_id = config->outputs.at(0);
  auto topic = "pin/" + String(out_id);
  auto payload = String((int)((last_value / 40.0) * 100));
  // Serial.print("out_id: ");
  // Serial.println(out_id);
  // Serial.print("topic: ");
  // Serial.println(topic);
  // Serial.print("payload: ");
  // Serial.println(payload);
  mqttClient.publish(topic.c_str(), 0, false, payload.c_str());
  last_millis = millis();
}

void _setup() { pinMode(SLIDER_PIN, INPUT); }

int read_avg(int read_count = 10) {
  unsigned int sum = 0;
  for (int i = 0; i < read_count; i++) {
    sum += analogRead(SLIDER_PIN);
  }

  return std::round(1.0 * sum / read_count) / 100;
}

void _setupListeners() {
  Tasks::queueTask(new DependentTask(
      {dependency}, new Tasks::Task(
                        []() {
                          last_value = read_avg();
                          send_val();
                        },
                        []() {
                          auto value = read_avg();
                          if (publish_on_change) {
                            if (std::abs(value - last_value) > 1) {
                              last_value = value;
                              send_val();
                            }
                          } else if (millis() - last_millis >= publish_period) {
                            last_value = value;
                            send_val();
                          }

                          return false;
                        },
                        Tasks::NoOp)));
}

void _reset() {
  *dependency = false;
  dependency = std::make_shared<bool>(true);
}

} // namespace Agent
#endif

#endif