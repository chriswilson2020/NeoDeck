#pragma once
#include <stdint.h>

namespace TerminalUnicode {
constexpr uint16_t Replacement = 0xFFFD;
constexpr uint16_t WideLead    = 0xFFFC;
constexpr uint16_t WideTrail   = 0xFFFF;

inline bool isPowerline(uint32_t cp) {
  return (cp >= 0xE0A0 && cp <= 0xE0A2) || (cp >= 0xE0B0 && cp <= 0xE0B3);
}

// VT100 DEC Special Graphics characters used by ncurses for borders.
inline uint16_t decSpecialGraphic(uint8_t ch) {
  switch (ch) {
    case 'f': return 0x00B0; // degree
    case 'j': return 0x2518; // lower-right corner
    case 'k': return 0x2510; // upper-right corner
    case 'l': return 0x250C; // upper-left corner
    case 'm': return 0x2514; // lower-left corner
    case 'n': return 0x253C; // crossing lines
    case 'q': return 0x2500; // horizontal line
    case 't': return 0x251C; // left tee
    case 'u': return 0x2524; // right tee
    case 'v': return 0x2534; // bottom tee
    case 'w': return 0x252C; // top tee
    case 'x': return 0x2502; // vertical line
    default:  return ch;
  }
}

inline bool hasFallback(uint32_t cp) {
  return (cp >= 0x2500 && cp <= 0x259F) ||
         cp == 0x2190 || cp == 0x2191 || cp == 0x2192 || cp == 0x2193 ||
         cp == 0x00B0 || (cp >= 0x2800 && cp <= 0x28FF) || isPowerline(cp);
}

// NeoDeck's built-in terminal font is ASCII. Every other accepted code point
// is drawn by the compact fallback renderer; unsupported input becomes one
// visible, single-cell replacement glyph.
inline uint16_t cellCodepoint(uint32_t cp) {
  if (cp >= 0x20 && cp <= 0x7E) return static_cast<uint16_t>(cp);
  if (hasFallback(cp)) return static_cast<uint16_t>(cp);
  return Replacement;
}
}
