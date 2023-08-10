#include <CredentialsStore.h>

CredentialsStore::CredentialsStore() {}

String CredentialsStore::read(const char *key) { return prefs.getString(key); }

void CredentialsStore::write(const char *key, const String &val) {
  prefs.putString(key, val);
}

bool CredentialsStore::init() {
  if (!prefs.begin(NAMESPACE)) {
    Serial.println("Could not open the credentials store");
    return false;
  }
  return true;
}

void CredentialsStore::writeSSID(const String &ssid) { write(SSID_KEY, ssid); }

void CredentialsStore::writePass(const String &pass) { write(PASS_KEY, pass); }

String CredentialsStore::getSSID() { return read(SSID_KEY); }

String CredentialsStore::getPass() { return read(PASS_KEY); }