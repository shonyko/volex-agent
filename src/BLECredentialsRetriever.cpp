#include <BLECredentialsRetriever.h>

CredentialsRetriever::MyServerCallbacks::MyServerCallbacks(bool *connected) {
  this->connected = connected;
}

CredentialsRetriever::MyCharacteristicCallbacks::MyCharacteristicCallbacks(
    BLECallback callback) {
  this->callback = callback;
}

CredentialsRetriever::CredentialsRetriever(const String &name) {
  BLEDevice::init((DEVICE_PREFIX + name).c_str());
  server = BLEDevice::createServer();
  server->setCallbacks(new MyServerCallbacks(&connected));

  service = server->createService(SERVICE_UUID);

  characteristic = service->createCharacteristic(
      CHARACTERISTIC_UUID,
      BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY);

  characteristic->addDescriptor(new BLE2902());

  auto advertising = BLEDevice::getAdvertising();
  advertising->addServiceUUID(SERVICE_UUID);
}

void CredentialsRetriever::retrieve(BLECallback callback) {
  service->start();
  server->getAdvertising()->start();
}