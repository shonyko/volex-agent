#ifndef CREDENTIALS_STORE_H
#define CREDENTIALS_STORE_H

#include <Preferences.h>

#define NAMESPACE "volex"
#define SSID_KEY "ssid"
#define PASS_KEY "pass"

class CredentialsStore {
protected:
  Preferences prefs;

  String read(const char *key);
  void write(const char *key, const String &val);

public:
  CredentialsStore();

  bool init();

  void writeSSID(const String &ssid);
  void writePass(const String &pass);

  String getSSID();
  String getPass();
};

#endif