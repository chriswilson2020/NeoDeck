#include "../src/msc_blocks.h"
#include <array>
#include <cassert>
#include <cstdio>

struct Disk {
  std::array<uint8_t, 16 * 512> bytes;
  int reads = 0, writes = 0;
  int failRead = -1, failWrite = -1;
  Disk() {
    for (size_t i = 0; i < bytes.size(); ++i) bytes[i] = (i * 31 + i / 512) % 251;
  }
  bool readRAW(uint8_t* data, uint32_t sector) {
    assert(sector < 16);
    ++reads;
    if (int(sector) == failRead) return false;
    memcpy(data, bytes.data() + sector * 512, 512);
    return true;
  }
  bool writeRAW(uint8_t* data, uint32_t sector) {
    assert(sector < 16);
    ++writes;
    if (int(sector) == failWrite) return false;
    memcpy(bytes.data() + sector * 512, data, 512);
    return true;
  }
};

int main() {
  // Exercise TinyUSB's 4096-byte buffer, offset normalization, unaligned
  // buffers, partition sector zero, and requests ending exactly at capacity.
  const uint32_t starts[] = {0, 1, 511, 512, 513, 1023, 2048, 7680, 8191};
  const uint32_t lengths[] = {0, 1, 2, 511, 512, 513, 1024, 4096};
  for (uint32_t start : starts) for (uint32_t length : lengths) {
    Disk disk;
    const auto original = disk.bytes;
    std::array<uint8_t, 4098> buffer;
    buffer.fill(0xA5);
    const bool valid = uint64_t(start) + length <= disk.bytes.size();
    // Deliberately express positions >512 as offsets, as TinyUSB can do.
    int32_t result = MscBlocks::transfer(disk, 16, 0, start,
                                         buffer.data() + 1, length, false);
    assert(result == (valid ? int32_t(length) : -1));
    if (valid) {
      assert(!memcmp(buffer.data() + 1, original.data() + start, length));
      assert(buffer[0] == 0xA5 && buffer[length + 1] == 0xA5);
    } else assert(disk.reads == 0);
    buffer.fill(0x5A);
    result = MscBlocks::transfer(disk, 16, start / 512, start % 512,
                                  buffer.data() + 1, length, true);
    assert(result == (valid ? int32_t(length) : -1));
    for (size_t i = 0; i < disk.bytes.size(); ++i)
      assert(disk.bytes[i] == (valid && i >= start && i < start + length
                               ? 0x5A : original[i]));
    if (!valid) assert(disk.writes == 0);
  }
  Disk disk;
  uint8_t buffer[1024] = {};
  disk.failRead = 1;
  assert(MscBlocks::transfer(disk, 16, 1, 0, buffer, 512, false) == -1);
  assert(MscBlocks::transfer(disk, 16, 1, 1, buffer, 10, true) == -1);
  assert(disk.writes == 0); // Failed read-modify-write must not write garbage.
  disk.failRead = -1;
  disk.failWrite = 1;
  assert(MscBlocks::transfer(disk, 16, 1, 0, buffer, 512, true) == -1);
  assert(MscBlocks::transfer(disk, 16, 1, 1, buffer, 10, true) == -1);
  assert(MscBlocks::transfer(disk, 16, UINT32_MAX, UINT32_MAX, buffer, 1, false) == -1);
  assert(MscBlocks::transfer(disk, 16, 0, 0, nullptr, 1, false) == -1);
  assert(MscBlocks::transfer(disk, 16, 0, 0, buffer, UINT32_MAX, true) == -1);
  puts("MSC block tests passed (144 transfer cases plus failure/bounds checks)");
}
