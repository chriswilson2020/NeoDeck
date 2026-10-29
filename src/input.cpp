#include "input.h"
#include <Wire.h>
#include "config.h"

Input gInput;

void Input::begin() {
  Wire.begin(BOARD_I2C_SDA, BOARD_I2C_SCL);

  pinMode(TB_UP, INPUT_PULLUP);
  pinMode(TB_DOWN, INPUT_PULLUP);
  pinMode(TB_LEFT, INPUT_PULLUP);
  pinMode(TB_RIGHT, INPUT_PULLUP);
  pinMode(TB_CLICK, INPUT_PULLUP);
}

InputEvent Input::poll() {
  // --- Keyboard: read one byte from the T-Deck keyboard MCU ---
  // The keyboard returns 0x00 when nothing is pressed. Throttle slightly so a
  // held key doesn't flood the UI.
  if (millis() - lastKbMs_ > 8) {
    lastKbMs_ = millis();
    if (Wire.requestFrom((int)KEYBOARD_ADDR, 1) == 1) {
      int c = Wire.read();
      if (c > 0) return {EvType::CHAR, (char)c};
    }
  }

  // --- Trackball: detect falling edges as discrete steps ---
  int up = digitalRead(TB_UP), dn = digitalRead(TB_DOWN);
  int lt = digitalRead(TB_LEFT), rt = digitalRead(TB_RIGHT);

  if (lastUp_ == HIGH && up == LOW) { lastUp_ = up; return {EvType::UP, 0}; }
  if (lastDn_ == HIGH && dn == LOW) { lastDn_ = dn; return {EvType::DOWN, 0}; }
  if (lastLt_ == HIGH && lt == LOW) { lastLt_ = lt; return {EvType::LEFT, 0}; }
  if (lastRt_ == HIGH && rt == LOW) { lastRt_ = rt; return {EvType::RIGHT, 0}; }
  lastUp_ = up; lastDn_ = dn; lastLt_ = lt; lastRt_ = rt;

  // --- Click (debounced) ---
  if (digitalRead(TB_CLICK) == LOW && millis() - lastClickMs_ > 250) {
    lastClickMs_ = millis();
    return {EvType::SELECT, 0};
  }

  return {EvType::NONE, 0};
}

// Explicitly invoked from USB serial console with 'K'. This diagnostic is
// deliberately NOT an automatic boot probe: an older C3 keyboard firmware
// may ignore raw mode and must not be classified as supporting it.
// LilyGO commands: 0x03 raw, 0x04 normal; raw reply = five matrix bytes.
void runKeyboardRawDiagnostic() {
  Serial.println("[kbd] Diagnostic: press Mic, Shift, and Shift+Mic in turn.");
  Serial.println("[kbd] Raw protocol probe (6 seconds); input pauses during test.");
  Wire.beginTransmission(KEYBOARD_ADDR);
  Wire.write((uint8_t)0x03);
  int status = Wire.endTransmission();
  Serial.printf("[kbd] raw command I2C status=%d (ACK does NOT prove support)\n", status);
  if (status == 0) {
    delay(50);
    uint8_t previous[5] = {0};
    bool havePrevious = false;
    int fiveByteReads = 0;
    int changed = 0;
    uint32_t started = millis();
    while (millis() - started < 6000) {
      uint8_t keys[5] = {0};
      int count = Wire.requestFrom((int)KEYBOARD_ADDR, 5);
      int actual = 0;
      while (Wire.available() && actual < 5) keys[actual++] = Wire.read();
      if (count == 5 && actual == 5) {
        ++fiveByteReads;
        bool different = !havePrevious;
        for (int i = 0; i < 5; ++i)
          if (havePrevious && keys[i] != previous[i]) different = true;
        if (different) {
          ++changed;
          Serial.printf("[kbd] raw: %02X %02X %02X %02X %02X | mic=%d leftShift=%d rightShift=%d\n",
                        keys[0], keys[1], keys[2], keys[3], keys[4],
                        !!(keys[0] & (1u << 6)), !!(keys[1] & (1u << 6)),
                        !!(keys[2] & (1u << 3)));
          memcpy(previous, keys, sizeof(previous));
          havePrevious = true;
        }
      } else {
        Serial.printf("[kbd] short read: requested=5 returned=%d actual=%d\n", count, actual);
      }
      delay(60);
    }
    Serial.printf("[kbd] five-byte reads=%d state changes=%d\n", fiveByteReads, changed);
    Serial.println("[kbd] Confirm raw support ONLY if Mic/Shift bits follow actual presses.");
  }
  // Restore character mode even if the command was unrecognized.
  Wire.beginTransmission(KEYBOARD_ADDR);
  Wire.write((uint8_t)0x04);
  int restore = Wire.endTransmission();
  Serial.printf("[kbd] restored normal mode, I2C status=%d\n", restore);
}
