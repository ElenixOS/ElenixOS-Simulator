#!/usr/bin/env python3
"""Generate checked-in EPKG v1 golden packages.

The generated binaries are the stable cross-language fixtures. This script is
only a fixture generator; it is not part of the future packager.
"""

from __future__ import annotations

import shutil
import struct
import sys
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parent
sys.path.insert(0, str(ROOT.parents[1] / "ElenixOS" / "scripts"))
import eos_pkg_builder  # noqa: E402


def _write_app(root: Path, with_icon: bool = True) -> None:
    root.mkdir(parents=True)
    (root / "manifest.json").write_text(
        '{\n'
        '  "id": "com.example.epkgv1",\n'
        '  "name": "EPKG v1 Fixture",\n'
        '  "version": "1.2.3",\n'
        '  "author": "ElenixOS",\n'
        '  "description": "Deterministic EPKG v1 fixture",\n'
        '  "minApiLevel": 0,\n'
        '  "targetApiLevel": 0,\n'
        '  "permissions": ["storage"]\n'
        '}\n',
        encoding="utf-8",
    )
    if with_icon:
        (root / "icon.bin").write_bytes(bytes(range(64)))
    (root / "main.js").write_bytes((b"const epkgFixture = 'lz4';\\n" * 2400) + b"\\n")
    nested = root / "assets" / "nested"
    nested.mkdir(parents=True)
    (nested / "data.txt").write_bytes((b"nested-data-0123456789\\n" * 900) + b"tail")
    (root / "empty.bin").write_bytes(b"")
    (root / "random.bin").write_bytes(bytes((index * 73 + 19) & 0xFF for index in range(4097)))


def _build_input(output: Path, package_type: str, compression: str, with_icon: bool = True) -> bytes:
    with tempfile.TemporaryDirectory(prefix="epkg-v1-input-") as temp_dir:
        source = Path(temp_dir) / "fixture"
        _write_app(source, with_icon=with_icon)
        eos_pkg_builder.pack_directory(source, output, package_type, compression)
    return output.read_bytes()


def _write_invalid(name: str, data: bytes) -> None:
    (ROOT / "invalid" / name).write_bytes(data)


def _u32(data: bytearray, offset: int) -> int:
    return struct.unpack_from("<I", data, offset)[0]


def _set_u32(data: bytearray, offset: int, value: int) -> None:
    struct.pack_into("<I", data, offset, value)


def main() -> None:
    shutil.rmtree(ROOT / "valid", ignore_errors=True)
    shutil.rmtree(ROOT / "invalid", ignore_errors=True)
    (ROOT / "valid").mkdir(parents=True)
    (ROOT / "invalid").mkdir(parents=True)

    none_data = _build_input(ROOT / "valid" / "none.epk", "app", "none")
    lz4_data = _build_input(ROOT / "valid" / "lz4_multiblock.epk", "app", "lz4")
    _build_input(ROOT / "valid" / "watchface.epk", "watchface", "auto", with_icon=False)

    _write_invalid("bad_magic.epk", b"NOPE" + none_data[4:])

    bad_crc = bytearray(none_data)
    bad_crc[-1] ^= 0x01
    _write_invalid("bad_crc.epk", bytes(bad_crc))

    _write_invalid("truncated.epk", none_data[:-7])

    bad_fpm = bytearray(none_data)
    bad_fpm[16 + 60 : 20 + 60] = b"\xFF\xFF\xFF\x7F"
    _write_invalid("fpm_size_mismatch.epk", bytes(bad_fpm))

    bad_offset = bytearray(none_data)
    table_offset = _u32(bad_offset, 32)
    _set_u32(bad_offset, table_offset + 12, 0xFFFFFF00)
    _write_invalid("offset_out_of_bounds.epk", bytes(bad_offset))

    bad_path = bytearray(none_data)
    table_offset = _u32(bad_path, 32)
    path_len = _u32(bad_path, table_offset + 4)
    path_start = table_offset + 28
    replacement = b"../manifest.json"
    if len(replacement) != path_len:
        replacement = b"../" + b"x" * (path_len - 3)
    bad_path[path_start : path_start + path_len] = replacement
    _write_invalid("path_traversal.epk", bytes(bad_path))

    bad_lz4 = bytearray(lz4_data)
    data_offset = _u32(bad_lz4, 36)
    manifest_size = _u32(bad_lz4, 60 + 16)
    icon_size = _u32(bad_lz4, 60 + 20)
    bad_lz4[data_offset + manifest_size + icon_size + 8] ^= 0xFF
    _write_invalid("damaged_lz4.epk", bytes(bad_lz4))

    bad_record = bytearray(none_data)
    table_offset = _u32(bad_record, 32)
    _set_u32(bad_record, table_offset, 27)
    _write_invalid("short_record.epk", bytes(bad_record))

    print(f"generated fixtures in {ROOT}")


if __name__ == "__main__":
    main()
