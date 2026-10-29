#include "../include/terminal_unicode.h"
#include "../include/terminal_box_data.h"
#include <cassert>
#include <cstdio>
#include <initializer_list>

int main() {
  using namespace TerminalUnicode;
  for (uint32_t cp = 0x2500; cp <= 0x259F; ++cp) {
    assert(hasFallback(cp));
    assert(cellCodepoint(cp) == cp);
  }
  for (uint32_t cp = 0x2800; cp <= 0x28FF; ++cp) {
    assert(hasFallback(cp));
    assert(cellCodepoint(cp) == cp);
  }
  for (uint32_t cp : {0x2190U, 0x2191U, 0x2192U, 0x2193U, 0x00B0U})
    assert(cellCodepoint(cp) == cp);
  for (uint32_t cp : {0xE0A0U, 0xE0A1U, 0xE0A2U, 0xE0B0U, 0xE0B1U, 0xE0B2U, 0xE0B3U})
    assert(cellCodepoint(cp) == cp);

  // Canonical continuous box arms: horizontal, vertical and four corners.
  assert(kBoxArms[0x2500 - 0x2500] == 0x44);
  assert(kBoxArms[0x2502 - 0x2500] == 0x11);
  assert(kBoxArms[0x250C - 0x2500] == 0x14);
  assert(kBoxArms[0x2510 - 0x2500] == 0x50);
  assert(kBoxArms[0x2514 - 0x2500] == 0x05);
  assert(kBoxArms[0x2518 - 0x2500] == 0x41);
  for (uint32_t cp = 0x2500; cp <= 0x257F; ++cp) {
    const bool special = (cp >= 0x256D && cp <= 0x2573);
    assert(special || kBoxArms[cp - 0x2500] != 0);
  }

  assert(cellCodepoint('A') == 'A');
  assert(cellCodepoint(0x2018) == '\'');
  assert(cellCodepoint(0x2019) == '\'');
  assert(cellCodepoint(0x201C) == '"');
  assert(cellCodepoint(0x201D) == '"');
  assert(cellCodepoint(0x2013) == '-');
  assert(cellCodepoint(0x2014) == '-');
  assert(cellCodepoint(0x2603) == Replacement); // unsupported snowman is visible
  assert(cellCodepoint(0x1F600) == Replacement); // unsupported non-BMP glyph
  assert(cellCodepoint(0) == Replacement);

  assert(decSpecialGraphic('x') == 0x2502);
  assert(decSpecialGraphic('q') == 0x2500);
  assert(decSpecialGraphic('l') == 0x250C);
  assert(decSpecialGraphic('k') == 0x2510);
  assert(decSpecialGraphic('m') == 0x2514);
  assert(decSpecialGraphic('j') == 0x2518);
  assert(decSpecialGraphic('A') == 'A');
  puts("Terminal Unicode coverage tests passed");
}
