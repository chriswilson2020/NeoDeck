#include "../src/credential_import.h"
#include <cassert>
#include <cstdio>

int main() {
  JsonDocument source, target;
  assert(!deserializeJson(source, R"({"wifiSsid":"home","wifiPass":"fake-password","anthropicKey":"fake-key","wifiNetworks":[{"ssid":"home","pass":"fake-password"},{"ssid":"guest","pass":""}],"envs":[{"name":"do not copy"}]})"));
  assert(CredentialImport::merge(target, source));
  assert(target["wifiSsid"] == "home");
  assert(target["wifiPass"] == "fake-password");
  assert(target["anthropicKey"] == "fake-key");
  assert(target["wifiNetworks"].size() == 2);
  assert(target["envs"].isNull());
  assert(!CredentialImport::merge(target, source));

  target.clear();
  assert(!deserializeJson(target, R"({"wifiSsid":"home","wifiPass":"","anthropicKey":"sd-key","envs":[{"name":"keep"}],"aiModel":"keep-model"})"));
  assert(CredentialImport::merge(target, source));
  assert(target["wifiPass"] == ""); // An open network stays open.
  assert(target["wifiNetworks"][0]["pass"] == "");
  assert(target["anthropicKey"] == "sd-key");
  assert(target["envs"][0]["name"] == "keep");
  assert(target["aiModel"] == "keep-model");

  target.clear(); source.clear();
  assert(!deserializeJson(source, R"({"wifiSsid":"legacy","wifiPass":"legacy-pass","anthropicKey":"legacy-key"})"));
  assert(CredentialImport::merge(target, source));
  assert(target["wifiNetworks"][0]["ssid"] == "legacy");
  assert(target["wifiNetworks"][0]["pass"] == "legacy-pass");
  assert(target["wifiSsid"] == "legacy");
  assert(target["wifiPass"] == "legacy-pass");
  assert(!CredentialImport::merge(target, source));

  target.clear();
  JsonArray networks = target["wifiNetworks"].to<JsonArray>();
  for (int i = 0; i < 10; ++i) {
    JsonObject p = networks.add<JsonObject>();
    p["ssid"] = std::string("network-") + std::to_string(i);
    p["pass"] = "sd-password";
  }
  target["wifiSsid"] = "network-0";
  target["wifiPass"] = "sd-password";
  target["anthropicKey"] = "sd-key";
  assert(!CredentialImport::merge(target, source));
  assert(networks.size() == 10);
  assert(target["wifiSsid"] == "network-0");
  puts("Credential import tests passed: missing values, legacy config, preservation, deduplication, capacity, repeated import");
}
