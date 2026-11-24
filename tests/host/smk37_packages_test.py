#!/usr/bin/env python3
# SPDX-License-Identifier: MPL-2.0
# Copyright (C) 2026 Tony Byrne
"""Read-only checks of the real per-model development packages; no MIDI backend is opened."""
import struct
import sys
import zlib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "base/tools"))
import fm1_install as I
import fm1pkg_make as P
import lz4blk

models = {"loom-smk37": "SMK-37 Pro", "loom-smk37-elite": "SMK-37 Elite", "loom-smk37-mke": "MKE-P37"}
if not all((ROOT / "app/build" / (stem + ".fwsc")).exists() for stem in models):
    print("skip SMK packages (build all three MODEL variants first)")
    sys.exit(0)
for stem, model in models.items():
    raw = (ROOT / "app/build" / (stem + ".fwsc")).read_bytes()
    checked_product, checked_image, _ = I.load_package(ROOT / "app/build" / (stem + ".fwsc"), False)
    product_model, version = I.product_of(raw).rsplit("_", 1)
    assert product_model == model and version.isdecimal() and version.startswith("9")
    image = I.logical_image(raw)
    assert checked_product == I.product_of(raw) and checked_image == image
    h = bytearray(image[:0x40]); P.enc(h, 0, len(h))
    assert P.crc16(h[2:]) == struct.unpack_from("<H", h)[0]
    count = struct.unpack_from("<H", h, 8)[0]
    assert count == 2 and P.crc16(image[0x40:0x40+count*0x50]) == struct.unpack_from("<H", h, 2)[0]
    parts = {}
    for i in range(count):
        e = bytearray(image[0x40+i*0x50:0x90+i*0x50]); P.enc(e, 0, len(e))
        kind, _, crc, _, off, size = struct.unpack_from("<HHHHII", e)
        parts[kind] = image[off:off+size]
        assert len(parts[kind]) == size and P.crc16(parts[kind]) == crc
    assert set(parts) == {0,100} and len(parts[0]) == 0x9D000
    area = bytearray(parts[0][0x4000:]); P.sfc(area, 0, len(area), 0, P.KEY)
    assert area[0x120:0x124] == bytes.fromhex("04818000")
    assert struct.unpack_from("<I", area, 32+8)[0] == 0x97400
    ota, loader, at = parts[100], bytearray(), 64
    while at < len(ota):
        packed, unpacked = struct.unpack_from("<II", ota, at); at += 8
        block = lz4blk.decompress(ota[at:at+packed]); at += packed
        assert len(block) == unpacked
        loader += block
    assert ("ota-" + model + "_900").encode() in loader
    res = (ROOT / "app/build" / (stem + "-res.bin")).read_bytes()
    magic, size, crc = struct.unpack_from("<III", res)
    assert magic == 0x31534552 and len(res) == size+32 <= 18*4096
    assert zlib.crc32(res[32:]) == crc
    print(model + ": app entry, package checksums, model loader and external resource bounds passed")
