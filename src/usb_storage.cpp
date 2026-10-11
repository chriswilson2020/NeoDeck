#include "usb_storage.h"
#include "config.h"
#include "ui.h"
#include "msc_blocks.h"
#include <Preferences.h>
#include <SD.h>
#include <sd_diskio.h>
#include <SPI.h>
#include <USB.h>
#include <USBMSC.h>
#include <atomic>
#include "freertos/semphr.h"
extern "C" {
#include "ff.h"
#include "diskio.h"
}

#if ARDUINO_USB_MODE
#error USB storage requires USB OTG mode (ARDUINO_USB_MODE=0)
#endif

namespace UsbStorage {
// Core 2.0.14 starts USB before setup(). Register the interface during static
// initialization, but advertise no medium throughout a normal NeoDeck boot.
static USBMSC msc;
static SemaphoreHandle_t ioMutex = nullptr;
static uint32_t sectors = 0;
static std::atomic<bool> ejected{false};
static bool accepting = false; // protected by ioMutex after begin()
// Use the version-pinned Arduino SD block driver without registering a VFS
// or mounting FAT. A USB host owns the filesystem, even on an exFAT/raw card.
struct RawCard {
  uint8_t drive = 0xFF;
  bool readRAW(uint8_t* data, uint32_t sector) {
    return sd_read_raw(drive, data, sector);
  }
  bool writeRAW(uint8_t* data, uint32_t sector) {
    return sd_write_raw(drive, data, sector);
  }
};
static RawCard card;

bool consumeBootRequest() {
  Preferences prefs;
  if (!prefs.begin("neodeck-usb", false)) return false;
  const bool requested = prefs.getBool("once", false);
  const bool cleared = !requested || prefs.putBool("once", false) == 1;
  prefs.end();
  return requested && cleared;
}

bool requestBoot() {
  Preferences prefs;
  if (!prefs.begin("neodeck-usb", false)) return false;
  const bool saved = prefs.putBool("once", true) == 1;
  prefs.end();
  if (!saved) return false;
  // Trackball select is GPIO0 / BOOT. Do not reboot while it is held low.
  while (digitalRead(TB_CLICK) == LOW) delay(10);
  delay(50);
  ESP.restart();
  return true;
}

static int32_t transfer(uint32_t lba, uint32_t offset, uint8_t* data,
                        uint32_t length, bool write) {
  if (!ioMutex) return -1;
  xSemaphoreTake(ioMutex, portMAX_DELAY);
  const int32_t result = accepting
      ? MscBlocks::transfer(card, sectors, lba, offset, data, length, write) : -1;
  xSemaphoreGive(ioMutex);
  return result;
}

static bool startStop(uint8_t, bool start, bool loadEject) {
  if (loadEject && !start && ioMutex) {
    xSemaphoreTake(ioMutex, portMAX_DELAY);
    accepting = false;
    msc.mediaPresent(false);
    ejected.store(true);
    xSemaphoreGive(ioMutex);
  }
  return true;
}

void begin() {
  ioMutex = xSemaphoreCreateMutex();
  card.drive = sdcard_init(BOARD_SD_CS, &SPI, 8000000);
  if (!ioMutex || card.drive == 0xFF ||
      (disk_initialize(card.drive) & STA_NOINIT) ||
      sdcard_sector_size(card.drive) != 512 ||
      !(sectors = sdcard_num_sectors(card.drive))) {
    UI::usbStorageStatus("SD card unavailable", "Check the card is fully inserted.",
                         "Click or Enter to return.");
    return;
  }
  msc.vendorID("NeoDeck");
  msc.productID("SD Card");
  msc.productRevision("1.0");
  msc.onRead([](uint32_t lba, uint32_t offset, void* data, uint32_t length) {
    return transfer(lba, offset, static_cast<uint8_t*>(data), length, false);
  });
  msc.onWrite([](uint32_t lba, uint32_t offset, uint8_t* data, uint32_t length) {
    return transfer(lba, offset, data, length, true);
  });
  msc.onStartStop(startStop);
  if (!msc.begin(sectors, 512)) {
    UI::usbStorageStatus("USB storage failed", "Restart and retry.",
                         "Click or Enter to return.");
    return;
  }
  // This is the last TFT/SPI access until the medium has been withdrawn and
  // all callbacks have finished. USB callbacks perform synchronous SD writes.
  UI::usbStorageStatus("USB SD card active", "Eject the drive on your computer.",
                       "Then click or Enter to return.");
  accepting = true;
  msc.mediaPresent(true);
}

void tick(const InputEvent& ev) {
  // Keep CDC quiet while MSC owns the card. Hardware testing on macOS with
  // this pinned stack succeeded with the serial monitor disconnected.
  // The menu click/Enter that caused the reboot may still be held. Require
  // a release before accepting an exit action in this new boot.
  static bool exitArmed = false;
  static uint32_t releaseSince = millis();
  if (!exitArmed) {
    if (digitalRead(TB_CLICK) == LOW || ev.type != EvType::NONE) releaseSince = millis();
    else if (millis() - releaseSince >= 750) exitArmed = true;
    return;
  }
  static bool displayedEject = false;
  if (ejected.load() && !displayedEject) {
    // Acquiring the same mutex ensures any in-flight SPI callback has left.
    xSemaphoreTake(ioMutex, portMAX_DELAY);
    UI::usbStorageStatus("SD card ejected", "Safe to return to NeoDeck.",
                         "Click or Enter to restart.");
    xSemaphoreGive(ioMutex);
    displayedEject = true;
  }
  if (ev.type != EvType::SELECT && ev.ch != '\r') return;
  // Some OSes unmount without sending START STOP UNIT. The user's click is
  // their explicit acknowledgement of the on-screen eject instruction.
  if (ioMutex) {
    xSemaphoreTake(ioMutex, portMAX_DELAY);
    accepting = false;
    msc.mediaPresent(false);
    xSemaphoreGive(ioMutex);
  }
  // Let the USB task finish its command response before reset.
  delay(250);
  ESP.restart();
}
}
