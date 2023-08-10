#ifndef BLE_CREDENTIALS_RETRIEVER_H
#define BLE_CREDENTIALS_RETRIEVER_H

#include <BLE2902.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <Data.h>

#define DEVICE_PREFIX "volex-agent="

#define SERVICE_UUID "946c1c80-4015-42dd-b9b8-f8c606f4dcde"
#define CHARACTERISTIC_UUID "5f065c2e-d8f3-40ac-8bbd-057e4d864e98"

typedef std::function<void(const Credentials &)> BLECallback;

class CredentialsRetriever {
private:
  BLEServer *server = NULL;
  BLEService *service = NULL;
  BLECharacteristic *characteristic = NULL;
  bool connected = false;

  class MyServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer *pServer) { *connected = true; };

    void onDisconnect(BLEServer *pServer) { *connected = false; }

  private:
    bool *connected;

  public:
    MyServerCallbacks(bool *connected);
  };

  class MyCharacteristicCallbacks : public BLECharacteristicCallbacks {

    void send(BLECharacteristic *pCharacteristic, int val) {
      pCharacteristic->setValue(val);
      pCharacteristic->notify();
    }

    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string value = pCharacteristic->getValue();
      if (value.length() <= 0) {
        Serial.println("[CredetialsRetriever] Got empty data");
        send(pCharacteristic, 0);
        return;
      }

      char str[value.length()];
      strcpy(str, value.c_str());
      Serial.println("[CredetialsRetriever] Got data: " + String(str));
      send(pCharacteristic, 1);

      Credentials c;
      char *token = strtok(str, ":");
      c.ssid = String(token);
      token = strtok(NULL, ":");
      c.pass = String(token);
      callback(c);
    }

  private:
    BLECallback callback;

  public:
    MyCharacteristicCallbacks(BLECallback callback);
  };

public:
  CredentialsRetriever(const String &name);
  void retrieve(BLECallback callback);
};

#endif