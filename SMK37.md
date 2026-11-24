# LOOM-8 on M-Vave SMK-37

**Development port: packages build, but the complete control mapping and SMK simulator/device validation
are still in progress. These builds are for simulator development, not a hardware installation handoff.**

LOOM-8 keeps its name, logo, boot screen, engines and project/preset formats. The FM-1 application, package
and update loader are checked byte-for-byte against the unmodified baseline after each porting area.

## Models and builds

The development source is hosted in a fork, with the platform and BSP changes pinned through submodules:

```sh
git clone --branch smk37 --recursive https://github.com/tonybyrne/fm1-loom.git
```

The Pro, Elite and MKE-P37 share the board target, with a separate package identity for each model.
Starrykey 37 Play uses the Pro build. Use the prerequisites in [BUILDING.md](BUILDING.md), then:

```sh
BOARD=smk37 MODEL=pro   sh build.sh
BOARD=smk37 MODEL=elite sh build.sh
BOARD=smk37 MODEL=mke   sh build.sh
```

| Model | Package in `app/build/` |
| --- | --- |
| SMK-37 Pro / Starrykey 37 Play | `loom-smk37.fwsc` |
| SMK-37 Elite | `loom-smk37-elite.fwsc` |
| MKE-P37 | `loom-smk37-mke.fwsc` |

Each package has a matching `-res.bin` file for its unchanged 909 cymbal data. Large fonts are embedded in
the SMK application. The resource file is uploaded through the running application; the update loader's
write allow-list is never extended for resources. Keep the resource file beside its package.

Build the FM-1 last before running its tests: both targets share intermediates.

```sh
sh build.sh
sh tests/run_tests.sh
```

For byte-identity verification, compare the app, package and loader against an unmodified build of the
baseline revisions listed in [SMK37-STORAGE.md](SMK37-STORAGE.md). Use the same toolchain and build both on
the same day because the compiler embeds dates. The development workspace's external reference checksum
file is not required to build a fresh checkout.

## Controls

The existing logical Loom controls map to the nearest printed SMK control. K1 is the virtual master pot;
the other encoders preserve Loom's SELECT, ALGORITHM, PRESETS and four parameter-knob order.
The SMK screen uses **K1–K8** for physical knob references, including parameter strips, hints and help.
Button labels keep Loom's existing names (HOME, SEL, EDIT, and so on). The MIDI engine's eight CC controls
span two pages: each page uses K5–K8, identified as ENGINE or ENGINE 2 in its CC settings.

| Printed SMK control | Loom function |
| --- | --- |
| K1 | MASTER |
| K2 / K3 / K4 | SELECT / ALGORITHM / PRESETS |
| K5–K8 | Loom parameter knobs 1–4 |
| PATCH | HOME |
| SCALE | SEL |
| CHORD | ENV |
| NOTE REPEAT | LFO |
| PARA | EDIT |
| GLOBE | GLO |
| BT | SAVE |
| ARP / SEQ | ARP / SEQ |
| SEQ PLAY / SEQ REC | PLAY / REC |
| OCTAVE− / OCTAVE+ | OCT− / OCT+ |
| Middle 27 keys | Existing FM-1 key functions |
| Pads, top row 1–8 / bottom row 9–16 | White-key equivalents, with pad LEDs in Loom's colours |

Pending mapping work: dedicated pad-page navigation on `<` / `>`, outer-key note routing, fader banks and
their overlays, pitch/modulation routing and the velocity setting. KNOB BANK and FADER BANK are reserved for the approved fader track-group and parameter-bank
selection. Wheel scope and pedal behaviour remain owner decisions in `smk37-port.yaml`.

## Storage and samples

- Three project slots and the existing 20-sector shared project pool are retained. Physical sectors are
  relocated; the logical object IDs, project/preset payloads and `LM8B` backup format stay the same.
- The SMK owner tag is `LOOM8`; untagged data or a different family's store is cleared before loading it.
  Factory calibration is always read-only. See [SMK37-STORAGE.md](SMK37-STORAGE.md) for the full map.
- The sample bank is **100 KiB**, down from 216 KiB on FM-1. The original 12-sample factory set fits without
  changing its encoded audio. It leaves about 11.6 KiB for additional data; larger collections require a
  subset of samples. There is no silent truncation or automatic reduction of audio quality.
- The four optional breaks are not preinstalled, including in SMK emulation. Their empty sample IDs stay
  reserved, so existing project sample numbers do not shift. Explicit sample loading remains possible.
- Project/preset backups are intended to transfer across boards. Raw sample-bank images need rebuilding
  for the destination board's windows and size. Python and web upload paths reject a capacity mismatch
  before sending an erase/begin command.

For SMK sample-tool operations, select the SMK layout explicitly:

```sh
BOARD=smk37 python3 tools/loom_samples.py factory
```

This is a hardware command for the owner, after installation and backup; the agent does not run it.

## Simulator checks

The native simulator can compile the SMK application path:

```sh
BOARD=smk37 sh build.sh --host
```

Its executable is `app/build/host/loom-smk37_host`. Smoke and backup/restore scenarios exercise the SMK
application code. Separate host models test matrix contacts, debounce, encoder detents, read-only factory
trim, pin directions, protected flash, the split project pool and interrupted writes. This is not yet a
complete SMK physical-panel simulator, nor execution of the `.fwsc` on hardware.
Raw physical-flash image import/export is disabled in this native SMK build until its split-pool image
support is implemented; use project/preset backups for the current cross-board tests.

## Installation and return to stock

Hardware installation waits for the completed mapping and simulator checks, the owner's explicit OK and
a verified full flash backup. The owner performs every hardware action. Once those conditions are met,
the intended installer is:

```sh
python3 base/tools/fm1_install.py app/build/loom-smk37.fwsc
```

Use the package for the exact model. Install and resume both check model identity, including after the
loader handoff; missing identity writes nothing. To return to stock, use M-Vave's M-UPGRADE with that
model's factory firmware. Stock firmware and flash dumps are not included in this repository.

Never erase or program `0x00000–0x03FFF`, `0xC0000–0xC1FFF`, `0xF4000–0xFDFFF` or `0xFF000–0xFFFFF`.
The SMK has no TRS MIDI input; PH8 is a matrix row, so TRS MIDI clock input cannot carry over. PB1 is never
driven, the key-matrix rows remain inputs, and PC9 stays analog.
