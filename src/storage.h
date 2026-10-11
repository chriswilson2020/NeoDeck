#pragma once
#include "config.h"

// Persistence of AppConfig (WiFi creds, API key, AI model, SSH environments)
// to the active filesystem (/deckssh/config.json on SD, /config.json in flash).
namespace Storage {
  bool begin();          // mounting is handled by fsx::begin()
  bool load();           // reads CFG_PATH into gConfig
  bool save();           // writes gConfig to CFG_PATH
  // Copy missing WiFi/API credentials from flash to SD; never format flash.
  // Returns a short status for the Settings screen, with no secret values.
  const char* importFlashCredentials();

  // Convenience env CRUD (all persist immediately)
  void addEnv(const SshEnv& e);
  void updateEnv(size_t i, const SshEnv& e);
  void removeEnv(size_t i);
}
