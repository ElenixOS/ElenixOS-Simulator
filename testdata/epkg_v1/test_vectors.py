#!/usr/bin/env python3
"""Protocol-level tests for checked-in EPKG v1 golden vectors."""

from __future__ import annotations

import struct
import zlib
from pathlib import Path


ROOT = Path(__file__).resolve().parent
HEADER = struct.Struct("<4s14I")
FPM = struct.Struct("<4s7I")
ENTRY = struct.Struct("<IIBBHIIII")


def lz4_decode(block: bytes, expected_size: int) -> bytes:
    output = bytearray()
    position = 0
    while position < len(block):
        token = block[position]
        position += 1
        literal_size = token >> 4
        if literal_size == 15:
            while True:
                value = block[position]
                position += 1
                literal_size += value
                if value != 255:
                    break
        output.extend(block[position : position + literal_size])
        position += literal_size
        if position == len(block):
            break
        offset = struct.unpack_from("<H", block, position)[0]
        position += 2
        match_size = token & 0x0F
        if match_size == 15:
            while True:
                value = block[position]
                position += 1
                match_size += value
                if value != 255:
                    break
        match_size += 4
        if offset == 0 or offset > len(output):
            raise ValueError("invalid LZ4 offset")
        for _ in range(match_size):
            output.append(output[-offset])
    if len(output) != expected_size:
        raise ValueError("invalid LZ4 output size")
    return bytes(output)


def parse_package(path: Path) -> tuple[dict, list[dict], bytes]:
    data = path.read_bytes()
    if len(data) < HEADER.size:
        raise ValueError("truncated header")
    values = HEADER.unpack_from(data, 0)
    if values[0] != b"EPKG" or values[1] != 1 or values[2] != 60:
        raise ValueError("invalid header")
    if zlib.crc32(data[:56] + b"\0\0\0\0") & 0xFFFFFFFF != values[14]:
        raise ValueError("header CRC")
    fpm_values = FPM.unpack_from(data, 60)
    if fpm_values[0] != b"FPMA" or fpm_values[1] != 1 or fpm_values[2] != 32:
        raise ValueError("invalid FPMA")

    entries = []
    position = values[8]
    for _ in range(values[5]):
        record_size, path_len, entry_type, codec, flags, offset, stored, original, crc = ENTRY.unpack_from(data, position)
        if record_size < 28 + path_len:
            raise ValueError("short record")
        path_start = position + 28
        name = data[path_start : path_start + path_len].decode("utf-8")
        entries.append(
            {
                "name": name,
                "type": entry_type,
                "codec": codec,
                "offset": offset,
                "stored": stored,
                "original": original,
                "crc": crc,
            }
        )
        position += record_size
    if position != values[9]:
        raise ValueError("table boundary")

    data_cursor = values[9]
    decoded = {}
    codecs = set()
    for entry in entries:
        if entry["type"] != 0:
            continue
        if entry["offset"] != data_cursor:
            raise ValueError("non-contiguous data")
        stored = data[entry["offset"] : entry["offset"] + entry["stored"]]
        if len(stored) != entry["stored"]:
            raise ValueError("data boundary")
        if entry["codec"] == 0:
            raw = stored
        elif entry["codec"] == 1:
            raw_parts = []
            block_position = 0
            while block_position < len(stored):
                raw_size, compressed_size = struct.unpack_from("<II", stored, block_position)
                block_position += 8
                compressed = stored[block_position : block_position + compressed_size]
                block_position += compressed_size
                raw_parts.append(lz4_decode(compressed, raw_size))
            if block_position != len(stored):
                raise ValueError("LZ4 framing")
            raw = b"".join(raw_parts)
        else:
            raise ValueError("codec")
        if len(raw) != entry["original"] or zlib.crc32(raw) & 0xFFFFFFFF != entry["crc"]:
            raise ValueError("file CRC")
        codecs.add(entry["codec"])
        decoded[entry["name"]] = raw
        data_cursor += entry["stored"]
    if data_cursor != len(data) or values[12] != data_cursor - values[9]:
        raise ValueError("data total")
    if "manifest.json" not in decoded:
        raise ValueError("manifest missing")
    if decoded["manifest.json"] != data[values[9] : values[9] + fpm_values[4]]:
        raise ValueError("manifest FPMA")
    if fpm_values[3] & 1:
        icon_start = values[9] + fpm_values[4]
        if decoded["icon.bin"] != data[icon_start : icon_start + fpm_values[5]]:
            raise ValueError("icon FPMA")
    return {"header": values, "fpm": fpm_values, "codecs": codecs}, entries, decoded["manifest.json"]


def main() -> None:
    valid = sorted((ROOT / "valid").glob("*.epk"))
    assert len(valid) == 3, valid
    parsed = {path.name: parse_package(path)[0] for path in valid}
    assert parsed["none.epk"]["codecs"] == {0}
    assert 1 in parsed["lz4_multiblock.epk"]["codecs"]
    assert parsed["watchface.epk"]["fpm"][3] == 0

    invalid = sorted((ROOT / "invalid").glob("*.epk"))
    assert len(invalid) >= 7, invalid
    for path in invalid:
        try:
            parse_package(path)
        except (AssertionError, KeyError, UnicodeDecodeError, struct.error, ValueError):
            continue
        raise AssertionError(f"invalid fixture accepted: {path.name}")
    print(f"EPKG v1 vectors: {len(valid)} valid and {len(invalid)} invalid cases passed")


if __name__ == "__main__":
    main()
