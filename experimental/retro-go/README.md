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

The dual-boot installation uses the two application binaries in dedicated partitions named `launcher` and `retro-core`. Retro-Go locates these partitions by label.

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
- The connected device has not been flashed by this branch yet.
- Before installation, read and verify a full 16 MB flash backup.
- Verify the ESP32-S3 MAC address is `14:c1:9f:4b:b5:88` before every write.
- Preserve the NeoDeck v0.10.0 recovery binary and full-flash backup.
