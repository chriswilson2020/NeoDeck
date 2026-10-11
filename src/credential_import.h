#pragma once
#include <ArduinoJson.h>
#include <cstring>

// Merge only WiFi and API credentials. Existing SD values always win,
// including an empty password for an existing open WiFi network.
namespace CredentialImport {
inline bool merge(JsonDocument& target, const JsonDocument& source) {
  bool changed = false;
  auto addNetwork = [&](const char* ssid, const char* pass) {
    if (!ssid || !*ssid) return;
    JsonArray networks = target["wifiNetworks"].as<JsonArray>();
    for (JsonObjectConst network : networks)
      if (!std::strcmp(network["ssid"] | "", ssid)) return;
    // Also preserve the legacy active network's password.
    if (!std::strcmp(target["wifiSsid"] | "", ssid))
      pass = target["wifiPass"] | "";
    if (networks.size() >= 10) return;
    if (networks.isNull()) networks = target["wifiNetworks"].to<JsonArray>();
    JsonObject entry = networks.add<JsonObject>();
    entry["ssid"] = ssid;
    entry["pass"] = pass;
    changed = true;
  };
  // Keep the SD's selected legacy network first when upgrading old configs.
  addNetwork(target["wifiSsid"] | "", target["wifiPass"] | "");
  for (JsonObjectConst network : source["wifiNetworks"].as<JsonArrayConst>())
    addNetwork(network["ssid"] | "", network["pass"] | "");
  addNetwork(source["wifiSsid"] | "", source["wifiPass"] | "");
  if (!*(target["wifiSsid"] | "")) {
    JsonObjectConst first = target["wifiNetworks"][0];
    if (const char* ssid = first["ssid"].as<const char*>()) {
      target["wifiSsid"] = ssid;
      target["wifiPass"] = first["pass"] | "";
      changed = true;
    }
  }
  if (!*(target["anthropicKey"] | "") && *(source["anthropicKey"] | "")) {
    target["anthropicKey"] = source["anthropicKey"];
    changed = true;
  }
  return changed;
}
}
