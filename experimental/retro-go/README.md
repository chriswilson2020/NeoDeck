# Retro-Go dual-boot experiment

This branch keeps the NeoDeck production firmware unchanged while testing a limited Retro-Go setup on the T-Deck Plus.

## Scope

- Nintendo Game Boy
- Nintendo Game Boy Color
- Nintendo Entertainment System
- No SNES, Sega, Doom, MSX, or other launcher categories
- A `Return to system Launcher` action that boots the bmorcelli Launcher partition (`app0`)

## Firmware

| File | SHA-256 |
| --- | --- |
| `firmware/retro-go-launcher.bin` | `881071e55eb3ffb3ba8a8f29ee40657d6035d3a19bd9a293f4ccc91460cbec15` |
| `firmware/retro-go-core.bin` | `82b5521611705a75628194db4915b9bf6970ce6d9d7dc8fa5fa708fba72ac979` |
| `firmware/retro-go-standalone.img` | `eb8a7d3aa15d5a0b82a75b81a8908a0f8e55e21d6ac814b4e3b8855c0f483168` |

The standalone image is for recovery and isolated emulator testing. It replaces the device partition table when flashed at `0x0` and must not be used for dual boot.

`dual-boot-partitions.csv` is retained as development material only. The first hardware trial produced a black screen with both the OTA slot and system Launcher, so the combined image and installable dual-boot components were removed. Do not flash this layout until the Launcher boot/display path has been reproduced and fixed offline.

## Installed layout

| Offset | Size | Label | Purpose |
| --- | --- | --- | --- |
| `0x9000` | `0x5000` | `nvs` | Preserved settings |
| `0x10000` | `0x180000` | `app0` | bmorcelli system Launcher |
| `0x1A0000` | `0x180000` | `neodeck` | Experimental NeoDeck |
| `0x320000` | `0x110000` | `launcher` | Retro-Go frontend |
| `0x430000` | `0x100000` | `retro-core` | GB/GBC/NES core |

## Source

`patches/retro-go-limited-gb-nes.patch` applies the experiment to Retro-Go 1.46. It was built from commit `57e1121` with ESP-IDF 5.3.4 for target `t-deck-plus`.

## ROM storage

ROMs are not included. Place legally obtained or homebrew ROMs on the FAT32 SD card in:

- `/roms/gb`
- `/roms/gbc`
- `/roms/nes`

Save files, save states, configuration, and artwork are also stored on the SD card. Normal gameplay causes little SD-card wear.

## Safety status

- Production `main` is not modified.
- The first experimental image was flashed to the verified T-Deck Plus on 2026-10-30 and produced a black screen.
- The device was restored from the exact pre-experiment 16 MB backup and NeoDeck v0.10.0 was confirmed working again.
- The failed combined image has been removed from this branch and must not be distributed or flashed.
- Verify the ESP32-S3 MAC address is `14:c1:9f:4b:b5:88` before every write.
- Full 16 MB pre-flash backup: `outputs/TDeckPlus-v0.10.0-full-flash-backup.bin`
- Backup SHA-256: `b72ab6abe4de3c5067da3fb46c9a9d61f2335beae73ffc3fee9eaa70ed49733c`
- NeoDeck v0.10.0 application recovery image: `outputs/NeoDeck-v0.10.0.bin`

To restore the exact previous device state, enter the ESP32-S3 bootloader and write the full backup at offset `0x0`. Always verify the MAC address first.
