#!/usr/bin/env python3
"""Generate the packed U+2500..U+257F arm table in terminal_box_data.h."""
import unicodedata

DIRECTIONS = {"UP": 1, "RIGHT": 2, "DOWN": 4, "LEFT": 8,
              "VERTICAL": 5, "HORIZONTAL": 10}
WEIGHTS = {"LIGHT": 1, "HEAVY": 2, "DOUBLE": 3, "SINGLE": 1}

values = []
for codepoint in range(0x2500, 0x2580):
    name = unicodedata.name(chr(codepoint)).removeprefix("BOX DRAWINGS ")
    if "DIAGONAL" in name or "ARC" in name:
        values.append(0)
        continue
    arms = [0, 0, 0, 0]  # up, right, down, left
    default_weight = 0
    for component in name.split(" AND "):
        words = component.split()
        weight = next((value for word, value in WEIGHTS.items() if word in words), 0)
        if weight and not default_weight:
            default_weight = weight
        weight = weight or default_weight or 1
        for direction, mask in DIRECTIONS.items():
            if direction in words:
                for index, bit in enumerate((1, 2, 4, 8)):
                    if mask & bit:
                        arms[index] = weight
    values.append(arms[0] | arms[1] << 2 | arms[2] << 4 | arms[3] << 6)

print("""#pragma once
#include <stdint.h>

// U+2500..U+257F. Two bits per arm: up, right, down, left.
// 0=none, 1=light, 2=heavy, 3=double. Arc and diagonal characters are
// special-cased by the renderer and therefore have zero descriptors here.
static const uint8_t kBoxArms[128] = {""")
for offset in range(0, len(values), 16):
    print("  " + ", ".join(f"0x{value:02X}" for value in values[offset:offset + 16]) + ",")
print("};")
