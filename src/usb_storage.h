#pragma once
#include "input.h"

namespace UsbStorage {
  bool consumeBootRequest();
  bool requestBoot();             // sets one-shot NVS flag and restarts
  void begin();                   // exclusive boot path, before fsx/SSH/WiFi
  void tick(const InputEvent& ev);
}
