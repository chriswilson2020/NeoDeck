#pragma once
#include <stdint.h>
#include <string.h>
#include <limits.h>

// Shared by firmware and host tests. Backend transfers exactly one 512-byte
// sector; USB requests may cover multiple sectors or only part of one.
namespace MscBlocks {
template <typename Backend>
int32_t transfer(Backend& disk, uint32_t sectors, uint32_t lba,
                 uint32_t offset, uint8_t* data, uint32_t length, bool write) {
  const uint64_t position = uint64_t(lba) * 512 + offset;
  const uint64_t capacity = uint64_t(sectors) * 512;
  if (length > INT32_MAX || position > capacity || length > capacity - position ||
      (length && !data)) return -1;
  uint32_t sector = position / 512;
  uint32_t within = position % 512;
  uint32_t done = 0;
  uint8_t scratch[512];
  while (done < length) {
    const uint32_t count = (length - done < 512 - within)
                            ? length - done : 512 - within;
    if (within == 0 && count == 512) {
      if (!(write ? disk.writeRAW(data + done, sector)
                  : disk.readRAW(data + done, sector))) return -1;
    } else {
      if (!disk.readRAW(scratch, sector)) return -1;
      if (write) {
        memcpy(scratch + within, data + done, count);
        if (!disk.writeRAW(scratch, sector)) return -1;
      } else memcpy(data + done, scratch + within, count);
    }
    done += count;
    ++sector;
    within = 0;
  }
  return done;
}
}
