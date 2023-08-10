#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <ESPmDNS.h>
#include <WiFi.h>

class WifiManager {
private:
  String ssid;
  String pass;

public:
  WifiManager();
};

#endif