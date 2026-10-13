#!/usr/bin/env python3
"""Verify that every RI_* icon code point exists in the deployed RemixIcon TTF."""

from __future__ import annotations

import hashlib
import re
import struct
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
ICON_HEADER = ROOT / "ElenixOS/src/ui/symbol/eos_icon.h"
SOURCE_TTF = ROOT / "ElenixOS/third_party/RemixIcon/fonts/remixicon.ttf"
ICON_TTF = ROOT / "fs/.sys/res/font/remixicon.ttf"
NON_GLYPH_CONTROLS = {"RI__NULL", "RI_NONMARKINGRETURN"}


def _u16(data: bytes, offset: int) -> int:
    return struct.unpack_from(">H", data, offset)[0]


def _u32(data: bytes, offset: int) -> int:
    return struct.unpack_from(">I", data, offset)[0]


def _cmap_subtables(data: bytes) -> list[tuple[int, int, int]]:
    if len(data) < 12 or data[:4] not in (b"\x00\x01\x00\x00", b"OTTO", b"true", b"typ1"):
        raise ValueError("not a recognized TrueType/OpenType font")

    table_count = _u16(data, 4)
    tables: dict[bytes, tuple[int, int]] = {}
    for index in range(table_count):
        entry = 12 + index * 16
        if entry + 16 > len(data):
            raise ValueError("truncated font table directory")
        tag = data[entry : entry + 4]
        offset = _u32(data, entry + 8)
        length = _u32(data, entry + 12)
        if offset + length > len(data):
            raise ValueError(f"table {tag!r} exceeds the font file")
        tables[tag] = (offset, length)

    if b"cmap" not in tables:
        raise ValueError("font has no cmap table")
    cmap_offset, cmap_length = tables[b"cmap"]
    if cmap_length < 4:
        raise ValueError("truncated cmap table")
    record_count = _u16(data, cmap_offset + 2)
    subtables = []
    for index in range(record_count):
        record = cmap_offset + 4 + index * 8
        platform = _u16(data, record)
        encoding = _u16(data, record + 2)
        relative = _u32(data, record + 4)
        subtable = cmap_offset + relative
        if subtable + 2 > cmap_offset + cmap_length:
            raise ValueError("cmap subtable offset is out of bounds")
        fmt = _u16(data, subtable)
        if fmt in (4, 12):
            subtables.append((platform, encoding, subtable))
    return subtables


def _format4_lookup(data: bytes, offset: int):
    seg_count = _u16(data, offset + 6) // 2
    end_offset = offset + 14
    end_codes = struct.unpack_from(f">{seg_count}H", data, end_offset)
    start_offset = end_offset + 2 * seg_count + 2
    start_codes = struct.unpack_from(f">{seg_count}H", data, start_offset)
    delta_offset = start_offset + 2 * seg_count
    deltas = struct.unpack_from(f">{seg_count}h", data, delta_offset)
    range_offset = delta_offset + 2 * seg_count
    range_offsets = struct.unpack_from(f">{seg_count}H", data, range_offset)

    def has_glyph(codepoint: int) -> bool:
        if codepoint > 0xFFFF:
            return False
        for index, end_code in enumerate(end_codes):
            if codepoint > end_code:
                continue
            if codepoint < start_codes[index]:
                return False
            if range_offsets[index] == 0:
                return ((codepoint + deltas[index]) & 0xFFFF) != 0
            glyph_pos = range_offset + index * 2 + range_offsets[index] + (codepoint - start_codes[index]) * 2
            glyph_index = _u16(data, glyph_pos)
            if glyph_index == 0:
                return False
            return ((glyph_index + deltas[index]) & 0xFFFF) != 0
        return False

    return has_glyph


def _format12_lookup(data: bytes, offset: int):
    group_count = _u32(data, offset + 12)
    groups = [struct.unpack_from(">III", data, offset + 16 + 12 * i) for i in range(group_count)]

    def has_glyph(codepoint: int) -> bool:
        for first, last, first_glyph in groups:
            if first <= codepoint <= last:
                return first_glyph + codepoint - first != 0
        return False

    return has_glyph


def main() -> int:
    if not ICON_HEADER.is_file():
        raise SystemExit(f"missing icon definitions: {ICON_HEADER}")
    if not SOURCE_TTF.is_file():
        raise SystemExit(f"missing RemixIcon source font: {SOURCE_TTF}")
    if not ICON_TTF.is_file():
        raise SystemExit(f"missing deployed icon font: {ICON_TTF}; run CMake configure first")

    macros = {
        name: int(codepoint, 16)
        for name, codepoint in re.findall(
            r'^#define (RI_[A-Z0-9_]+) "\\u([0-9A-Fa-f]{4,6})"$',
            ICON_HEADER.read_text(encoding="utf-8"),
            re.MULTILINE,
        )
    }
    if not macros:
        raise SystemExit("no RI_* Unicode macros found")

    source_data = SOURCE_TTF.read_bytes()
    data = ICON_TTF.read_bytes()
    if hashlib.sha256(source_data).digest() != hashlib.sha256(data).digest():
        raise SystemExit("deployed RemixIcon TTF differs from the checked-out source font")
    subtables = _cmap_subtables(data)
    candidates = [entry for entry in subtables if entry[0] == 3 and entry[1] in (1, 10)]
    if not candidates:
        candidates = [entry for entry in subtables if entry[0] == 0]
    if not candidates:
        raise SystemExit("font has no Unicode cmap in format 4 or 12")

    # Prefer format 12 when available, otherwise BMP format 4 covers RI PUA.
    selected = max(candidates, key=lambda item: _u16(data, item[2]))
    fmt = _u16(data, selected[2])
    has_glyph = _format12_lookup(data, selected[2]) if fmt == 12 else _format4_lookup(data, selected[2])
    missing = {name for name, codepoint in macros.items() if not has_glyph(codepoint)}
    if missing != NON_GLYPH_CONTROLS:
        absent = ", ".join(f"{name}=U+{macros[name]:04X}" for name in sorted(missing)) or "none"
        raise SystemExit(f"cmap mismatch: unexpected missing RI_* glyphs: {absent}")

    sha256 = hashlib.sha256(data).hexdigest()
    print(
        f"PASS macros={len(macros)} glyphs={len(macros) - len(missing)} "
        f"non_glyph_controls={len(missing)} cmap_format={fmt} source_matches_deployed=yes sha256={sha256}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
