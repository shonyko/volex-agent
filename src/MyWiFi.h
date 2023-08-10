#ifndef MY_WIFI_H
#define MY_WIFI_H

#ifndef ESP8266
#include <WiFi.h>
#else
#include <ESP8266WiFi.h>
#endif

#include <Data.h>
#include <mDNSResolver.h>

namespace MyWiFi {

namespace {
WiFiUDP udp;
mDNSResolver::Resolver resolver(udp);
boolean lastState = false;
} // namespace

IPAddress resolveBrokerIp(const char *host) {
  resolver.setLocalIP(WiFi.localIP());
  return resolver.search(host);
}

typedef struct {
  std::function<void()> onConnect;
  std::function<void()> onDisconnect;
} WiFiConfig;

WiFiConfig conf;
std::shared_ptr<boolean> dependency = nullptr;

void onWifiConnect(WiFiEvent_t event, WiFiEventInfo_t info) {
  lastState = true;
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  dependency = std::make_shared<boolean>(true);
  if (conf.onConnect != nullptr) {
    conf.onConnect();
  }
}

void onWifiDisconnect(WiFiEvent_t event, WiFiEventInfo_t info) {
  if (!lastState) {
    return;
  }
  lastState = false;
  Serial.println("Disconnected from WiFi");
  if (dependency != nullptr) {
    *dependency = false;
  }
  if (conf.onDisconnect != nullptr) {
    conf.onDisconnect();
  }
}

void wifi_setup(WiFiConfig config) {
  conf = config;
  WiFi.onEvent(onWifiConnect, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_GOT_IP);
  WiFi.onEvent(onWifiDisconnect,
               WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
}

void connectToWifi(const Credentials &c) { WiFi.begin(c.ssid, c.pass); }

void wifiLoop() {
  if (!WiFi.isConnected()) {
    return;
  }
  // clear the udp buffer
  resolver.loop();
}

} // namespace MyWiFi

#endif