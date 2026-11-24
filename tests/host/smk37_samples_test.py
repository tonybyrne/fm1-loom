#!/usr/bin/env python3
# SPDX-License-Identifier: MPL-2.0
# Copyright (C) 2026 Tony Byrne
"""Check unbundled SMK breaks with the actual generator and sample reader; no devices."""
import os
import subprocess
import struct
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
with tempfile.TemporaryDirectory(prefix="loom-smk-samples-") as tmp:
    banks = []
    for board in ("fm1", "smk37"):
        out = Path(tmp) / board
        out.mkdir()
        env = {**os.environ, "BOARD": board}
        subprocess.run([sys.executable, str(ROOT / "tools/gen_samples.py"), str(out / "samples.h")],
                       env=env, check=True)
        subprocess.run([sys.executable, str(ROOT / "tools/gen_samples.py"), "--factory-bank", str(out / "factory.bin")],
                       env=env, check=True)
        banks.append((out / "factory.bin").read_bytes())
        exe = out / "samples_test"
        subprocess.run([os.environ.get("CC", "cc"), "-std=c99", "-O1", "-Wall", "-Wextra", "-Werror",
                        "-DPLAT_HOST", "-DPLAT_WEB", f"-DBOARD_SMK37={int(board == 'smk37')}",
                        "-I" + str(out), str(ROOT / "tests/host/smk37_samples_test.c"), "-o", str(exe)], check=True)
        subprocess.run([str(exe)], check=True)
    def audio(bank):
        result = []
        for i in range(bank[6]):
            e = 64 + i * 40
            adp, ck, n = struct.unpack_from("<III", bank, e + 12)
            result.append((bank[e:e + 12], bank[e + 20:e + 40], bank[adp:adp + (n + 1) // 2],
                           bank[ck:ck + ((n + 127) // 128) * 4]))
        return result
    assert audio(banks[0]) == audio(banks[1]), "board layout changes must preserve factory sample audio and IDs"
    assert len(banks[1]) <= 100 * 1024, "the original factory set must fit the SMK bank"
    sys.path.insert(0, str(ROOT / "tools"))
    import loom_samples
    class WrongBoard:
        calls = []
        def req(self, cmd, args):
            self.calls.append(cmd)
            assert cmd == 15, "mismatched uploads must not start or erase a bank"
            return [2, 100, 0, 0, 0, 0, 0, 0]
    link = WrongBoard()
    try:
        loom_samples.upload(link, banks[0])
    except ValueError:
        pass
    else:
        raise AssertionError("FM-1 sample bank accepted by an SMK device")
    assert link.calls == [15]
print("SMK optional breaks omitted; FM-1 defaults and sample IDs preserved")
