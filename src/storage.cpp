#include "storage.h"
#include <ArduinoJson.h>
#include "fsx.h"
#include "credential_import.h"
#include <LittleFS.h>

AppConfig gConfig;

namespace Storage {

bool begin() { return true; }   // mounting is handled by fsx::begin()

bool load() {
  File f = fsx::fs().open(fsx::configPath(), "r");
  if (!f) return false;

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, f);
  f.close();
  if (err) {
    Serial.printf("[cfg] parse error: %s\n", err.c_str());
    return false;
  }

  gConfig.wifiSsid     = doc["wifiSsid"]     | "";
  gConfig.wifiPass     = doc["wifiPass"]     | "";
  gConfig.wifiNetworks.clear();
  for (JsonObject o : doc["wifiNetworks"].as<JsonArray>()) {
    WifiProfile p;
    p.ssid = o["ssid"] | "";
    p.pass = o["pass"] | "";
    if (!p.ssid.isEmpty() && gConfig.wifiNetworks.size() < 10)
      gConfig.wifiNetworks.push_back(p);
  }
  // Backwards compatibility with the original one-network config.json.
  if (gConfig.wifiNetworks.empty() && !gConfig.wifiSsid.isEmpty())
    gConfig.wifiNetworks.push_back({gConfig.wifiSsid, gConfig.wifiPass});
  gConfig.anthropicKey = doc["anthropicKey"] | "";
  gConfig.aiModel      = doc["aiModel"]      | "claude-opus-4-8";
  gConfig.lockSalt     = doc["lockSalt"]     | "";
  gConfig.lockHash     = doc["lockHash"]     | "";

  gConfig.envs.clear();
  for (JsonObject o : doc["envs"].as<JsonArray>()) {
    SshEnv e;
    e.name    = o["name"]    | "";
    e.host    = o["host"]    | "";
    e.user    = o["user"]    | "";
    e.pass    = o["pass"]    | "";
    e.port    = o["port"]    | 22;
    e.auth    = o["auth"]    | 0;
    e.keyName = o["keyName"] | "";
    e.keyPass = o["keyPass"] | "";
    gConfig.envs.push_back(e);
  }
  Serial.printf("[cfg] loaded %u env(s)\n", (unsigned)gConfig.envs.size());
  return true;
}

bool save() {
  JsonDocument doc;
  doc["wifiSsid"]     = gConfig.wifiSsid;
  doc["wifiPass"]     = gConfig.wifiPass;
  JsonArray networks = doc["wifiNetworks"].to<JsonArray>();
  for (const WifiProfile& p : gConfig.wifiNetworks) {
    JsonObject o = networks.add<JsonObject>();
    o["ssid"] = p.ssid;
    o["pass"] = p.pass;
  }
  doc["anthropicKey"] = gConfig.anthropicKey;
  doc["aiModel"]      = gConfig.aiModel;
  doc["lockSalt"]     = gConfig.lockSalt;
  doc["lockHash"]     = gConfig.lockHash;

  JsonArray arr = doc["envs"].to<JsonArray>();
  for (auto& e : gConfig.envs) {
    JsonObject o = arr.add<JsonObject>();
    o["name"]    = e.name;
    o["host"]    = e.host;
    o["user"]    = e.user;
    o["pass"]    = e.pass;
    o["port"]    = e.port;
    o["auth"]    = e.auth;
    o["keyName"] = e.keyName;
    o["keyPass"] = e.keyPass;
  }

  File f = fsx::fs().open(fsx::configPath(), "w");
  if (!f) return false;
  serializeJsonPretty(doc, f);
  f.close();
  return true;
}

const char* importFlashCredentials() {
  if (!fsx::usingSD()) return "Insert an SD card and restart first";
  if (!LittleFS.begin(false)) return "Internal storage unavailable";
  JsonDocument source, target;
  File input = LittleFS.open("/config.json", "r");
  if (!input) { LittleFS.end(); return "No internal settings found"; }
  const auto sourceError = deserializeJson(source, input);
  input.close();
  LittleFS.end(); // Internal copy stays untouched, even on read errors.
  if (sourceError || !source.is<JsonObject>()) return "Internal settings could not be read";

  fs::FS& disk = fsx::fs();
  const String path = fsx::configPath();
  const bool hadConfig = disk.exists(path);
  if (hadConfig) {
    File current = disk.open(path, "r");
    if (!current) return "SD settings could not be opened";
    const auto error = deserializeJson(target, current);
    current.close();
    if (error || !target.is<JsonObject>()) return "SD settings invalid; left unchanged";
  }
  if (!CredentialImport::merge(target, source)) return "No missing credentials to import";
  if (target.overflowed()) return "Not enough memory to import";

  // Reserve a new backup name; never overwrite an earlier import backup.
  String backup;
  if (hadConfig) {
    for (int i = 1; i <= 99; ++i) {
      String candidate = path + ".before-import-" + String(i);
      if (!disk.exists(candidate)) { backup = candidate; break; }
    }
    if (backup.isEmpty()) return "Too many import backups on SD";
  }
  const String temp = path + ".import-tmp";
  if (disk.exists(temp)) return "Previous import temp file exists";
  File output = disk.open(temp, "w");
  if (!output) return "Cannot write SD import file";
  const size_t expected = measureJsonPretty(target);
  const size_t written = serializeJsonPretty(target, output);
  output.flush();
  output.close();
  JsonDocument verified;
  File check = disk.open(temp, "r");
  const bool valid = check && written == expected &&
      !deserializeJson(verified, check) &&
      verified.as<JsonVariantConst>() == target.as<JsonVariantConst>();
  check.close();
  if (!valid) { disk.remove(temp); return "SD write verification failed"; }
  if (hadConfig && !disk.rename(path, backup)) {
    disk.remove(temp);
    return "Could not back up SD settings";
  }
  if (!disk.rename(temp, path)) {
    const bool restored = !hadConfig || disk.rename(backup, path);
    disk.remove(temp);
    return restored ? "Import failed; settings unchanged" : "Import failed; restore SD backup";
  }
  if (!load()) return "Saved; restart to reload settings";
  return "Imported! Use Save & reconnect WiFi";
}

void addEnv(const SshEnv& e)              { gConfig.envs.push_back(e); save(); }
void updateEnv(size_t i, const SshEnv& e) { if (i < gConfig.envs.size()) { gConfig.envs[i] = e; save(); } }
void removeEnv(size_t i)                  { if (i < gConfig.envs.size()) { gConfig.envs.erase(gConfig.envs.begin() + i); save(); } }

}  // namespace Storage
