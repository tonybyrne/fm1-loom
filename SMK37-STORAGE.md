# LOOM-8 SMK-37 storage map

**Status: implemented board-stage layout, under simulator validation.** The owner approved a smaller bank
and omission of optional breaks. Large fonts are now immutable application data, which makes room for the
unchanged factory samples in a 100 KiB bank. FM-1 addresses, formats and build flags remain unchanged.

## Implemented SMK layout

All ranges below are inclusive and sector-aligned. Keep the reference board's flash allow-list and the
second guard in the RAM flash driver. Ordinary project/sample/resource writes stay out of the app slot
and update staging; only update, restore-journal and diagnostic routines use staging.

| Range | Size | Use |
| --- | --- | --- |
| `0x04000–0x9CFFF` | 612 KiB | Application/package area; update-loader writes only |
| `0x9D000–0xAEFFF` | 72 KiB | Project pool sectors 0–17 |
| `0xAF000–0xAFFFF` | 4 KiB | Board-layer data-family owner tag, at its reference address |
| `0xB0000–0xB1FFF` | 8 KiB | Project pool sectors 18–19 |
| `0xB2000–0xB3FFF` | 8 KiB | `OBJ_PANEL`, A/B copies of the port's panel calibration, never factory trim |
| `0xB4000–0xB5FFF` | 8 KiB | `OBJ_PRESETS`, A/B copies |
| `0xB6000–0xB7FFF` | 8 KiB | `OBJ_BLE`, A/B copies of Loom's radio settings; not stock BTIF |
| `0xB8000–0xBAFFF` | 12 KiB | Sample-bank header and first data segment; wheels use auto calibration |
| `0xBB000–0xBFFFF` | 20 KiB | Update staging; restore journal at BC000, optional crash history at BD000/BE000, optional stats and update record at BF000 |
| `0xC2000–0xD3FFF` | 72 KiB | Resource region: unchanged 909 cymbals |
| `0xD4000–0xE9FFF` | 88 KiB | Sample-bank second data segment |

The resource payload is **72,632 bytes**, plus its 32-byte commit header, fitting 18 sectors. The unchanged
15,800-byte large-font blob is embedded in the app instead. The sample bank spans two windows totalling
102,400 bytes (100 KiB), including its 2,048-byte header. The original factory bank uses 90,505 bytes and
leaves 11,895 bytes. The factory audio and its sample IDs are unchanged.

Do not write any other range. In particular, never erase/program `0x00000–0x03FFF`, `0xC0000–0xC1FFF`,
`0xF4000–0xFDFFF`, or `0xFF000–0xFFFFF`. Factory velocity curves at `0xC1000` remain read-only.

## Every existing stored object

The FM-1 retains its current `app/app.h` layout. The following are the FM-1 legacy A/B addresses; project
objects now use the existing 20-sector pool at `0x97000–0xAAFFF` after migration.

| Enum / object IDs | FM-1 legacy A/B sectors | SMK location |
| --- | --- | --- |
| `OBJ_PANEL` (0) | `0xDC000`, `0xDD000` | `0xB2000`, `0xB3000` |
| `OBJ_PRESETS` (1) | `0xDE000`, `0xDF000` | `0xB4000`, `0xB5000` |
| `OBJ_PROJ(0, 0..6)` (2–8) | Pairs beginning `0x97000`, `0x99000`, `0x9B000`, `0x9D000`, `0x9F000`, `0xA1000`, `0xA3000` | Shared SMK project pool |
| `OBJ_PROJ(1, 0..6)` (9–15) | Pairs beginning `0xA5000`, `0xA7000`, `0xA9000`, `0xAB000`, `0xAD000`, `0xAF000`, `0xB1000` | Shared SMK project pool |
| `OBJ_PROJ(2, 0..6)` (16–22) | Pairs beginning `0xB3000`, `0xB5000`, `0xB7000`, `0xB9000`, `0xBB000`, `0xBD000`, `0xBF000` | Shared SMK project pool |
| `OBJ_BLE` (23) | `0xFC000`, `0xFD000` | `0xB6000`, `0xB7000` |

Each project's objects retain their IDs and payloads: object 0 is settings, objects 1–6 are the compressed
stream chunks. All 21 logical project objects share the same 20 physical pool sectors, as on FM-1. The pool
is split around the owner tag; SMK-only sector lookup must handle the split everywhere, including reads,
saves, previous-copy fallback, pool scans and boot. Do not assign fictitious contiguous A/B pairs to these
pooled objects. On SMK there is no legacy FM-1 map migration: never scan or erase those old addresses.

## Compatibility and capacity

- Preserve project/preset payload formats, object IDs, CRCs and backup magic `LM8B`, subject to cross-board
  restore tests. Moving flash addresses does not require changing project payload formats.
- FM-1 and SMK are separate devices: compatibility means backup/restore interchange, not copying an entire
  flash dump or adopting FM-1 physical addresses.
- Do not share Felucca's owner tag merely because the board layer comes from Felucca. Determine the tag from
  Loom's own compatible layouts; existing Loom uses its own payload magics and has no SMK owner tag yet.
- Keep three project slots and 20 pool sectors, the existing presets, and unchanged resource audio/font data.
- **Sample-bank capacity falls from 216 KiB to 100 KiB.** Keep 32 logical user slots and the bank format.
  CLI generation uses `BOARD=smk37`; raw bank images from the other board need repacking. Python and web
  send paths compare the bank header's capacity with the destination's INFO reply before sending BEGIN or
  erasing anything. The factory set fits, with about 11.6 KiB remaining for additional data.

### Original capacity investigation

The unchanged hardware build is now available:

| Item | Bytes |
| --- | ---: |
| FM-1 application | 577,892 |
| Reference SMK application slot (`0x97400`) | 619,520 |
| Maximum room before adding any SMK code | 41,628 |
| Existing resource payload | 88,444 |
| Shortfall for putting every resource into the app, before SMK code | 46,816 |
| Factory sample bank, including header | 90,505 |
| Break pack, including its own bank header | 90,548 |
| Factory samples + all four breaks in one bank (one header) | 179,005 |

The owner chose to omit the optional four-break pack from SMK defaults, including emulation.
The hardware firmware and default 90,505-byte factory bank already exclude these breaks, so this choice
did **not** reclaim additional hardware flash or resolve the original 72 KiB layout's shortfall by itself.
The SMK sample generator omits the browser's automatic break payload; empty factory slots 12–15 remain
for project/sample-number compatibility. Explicit loading of samples remains available. FM-1 defaults
and browser behaviour stay unchanged.

Therefore **putting every resource into the app is not currently feasible**. Even a hypothetical full
160 KiB sample window would not hold the unchanged factory samples and all four breaks together.

The implemented split moves only large fonts into the application and uses the remaining three VM sectors
for the bank header/data. It does not change an audio encoding or widen the board allow-list. Each compiled application
is checked against the reference SMK app slot and the existing SRAM spare requirement.

## Required checks before enabling writes

1. Establish and retain FM-1 app/package/loader hashes, with no new FM-1 flags.
2. Assert every proposed object/window is inside its permitted region, disjoint from other uses and outside
   protected flash. Cover all 24 existing object IDs, including every pooled project object.
3. Host-test the split pool, previous-copy recovery, full-store errors and interrupted saves/restores.
4. Test app, update loader, resources, samples, installers and resume paths against forbidden writes and
   model mismatches; no install/resume write without package identity.
5. Preserve the reference guards, and compare FM-1 hashes after each porting area.
6. Simulator success does not replace the playbook's owner-confirmed hardware checklist and read-only
   `trimdat` check. Hardware installation requires explicit owner approval and a verified full backup.

## Initial unchanged-source checks

Source baseline: LOOM-8 `22d96133d512137f8fafed274de0ca10ce31a76a`, fm1-base
`7337375c15e9db26f23b0c68a1db61429add2f48`, felucca-bsp `2a5af3dfc8bccb92e37d48a3e7e0bf9a5827e51f`.

- Host simulator build passes. Existing warning: host `MRQ` redefinition.
- Loom's host suite passes with the pinned Node 24.22.3, including update entry and loader tests against
  the built package. The earlier `samples-web` failure under Node 20.18.1 is resolved by using the pinned
  Node version; no source change was needed.
- The platform suite has one baseline failure: `installer-fast-py`, in the fast reinstall case that should
  retain an existing resource region and read its record once. Its other executed checks pass. The platform
  template's update tests are skipped because no template hardware package was built.
- Browser emulator/export tests are skipped because Emscripten is unavailable. A DSP fingerprint reference
  of 1,268 cases was saved before firmware changes.
- Address arithmetic checks pass for sector alignment, pairwise disjointness, the 20-sector split pool and
  exclusion of all four protected ranges. These check the proposal, not an implemented flash driver.
- Python 3.14 with Pillow 13.0.0, fonttools 4.68.0 and raqm can generate the fonts and resources.
- The pinned JieLi toolchain archive matches SHA-256
  `a1545cddbc451a06bac43eb5cd96d831470c42a97c5c0b1f98879a20fc7647bd`; its clang reports 4.0.1.
  The SDK is at pinned commit `d179b4484759423312073f5fbb232501aa491047`; the build verifies its three
  package files. Prerequisites are outside the firmware repositories; builds run in Linux/amd64 Docker.
- Two unchanged FM-1 hardware builds pass and reproduce all three hashes below. SRAM is 472,920 of
  475,136 bytes, with 2,216 bytes free (1,984 reserved spare). New SMK buffers need an explicit RAM budget.
- The build identity hashes source-file text, including inactive preprocessor branches. Merely wrapping
  changes in `BOARD_SMK37` would still change the FM-1 binary through that identity. The port must account
  for this without adding an FM-1 compiler flag or hiding future active FM-1 source changes.

## System-layer checkpoint (earlier)

- `base/tools/smk37.py` selects the board/model and projects explicitly `BOARD_SMK37`-gated sources onto
  their original FM-1 text for build-identity hashing. Other active FM-1 edits still change the identity.
  No FM-1 compiler flag was added; the app/package/loader hashes remain the baseline hashes.
- `base/bsp/boards/smk37/flash_map.h` retains the reference allow-list and compile-time asserts. The actual
  RAM erase, split-erase and program entry points independently reject protected ranges, even when a caller
  overrides its range policy. The app API's whole-request preflight uses the protected guard too.
- Pro, Elite and MKE update loaders cross-build with their model identities and VM-only staging at
  `0xBB000–0xBFFFF`. Their record cleanup stays below `0xC0000`. The loader's display transport uses SMK
  pins; host pin checks preserve PB1 and PC9.
- Package layout tests use a synthetic, in-memory app: region descriptors match the SMK reference and
  model/layout mismatches are refused. These are not installable Loom SMK application packages.
- The real loader package parser rejects resource entries before any write, including forged, correctly
  checksummed entries targeting the VM store. Builder-side rejection alone is not the safety boundary.
- The loader allow-list is not widened for resources. SMK resources will need transfer by the running app
  after the app target and resource layout are implemented; SMK packages cannot carry loader-written resources.
- Python and web installers require package identity before writing, match the loader model on resume and
  after normal-install handoff, and parse identities to the first NUL. Exact model matching is used for the
  three SMK variants. Web boot-area installation is disabled for SMK models.
- At that checkpoint the SMK app target was disabled; the board-stage integration below now enables it.
  Standalone loader test outputs remain under `app/build/smk37-system/<model>/loader/`.
- At the system-layer checkpoint, the firmware and submodules were local worktree changes on `smk37`
  branches. The publication checkout now pins the platform and BSP source commits in personal forks.
- Final checkpoint verification: Loom's suite passes, including all 1,268 saved DSP fingerprints; the
  platform's suite passes all executed checks, including the three real SMK loader builds and installer
  model tests. The initial Python fast-installer failure did not recur. Repeated macOS host tests needed a
  fresh copied executable inode to avoid a code-signature-cache kill; the runner now removes that old copy.
  The existing BSP flash tests pass with address/undefined-behaviour sanitizers. FM-1 was rebuilt last and
  all three baseline hashes match. Emscripten/browser tests and the platform template's hardware-package
  tests remain skipped; Loom's own hardware-package update tests ran and passed.

| Artifact | SHA-256 |
| --- | --- |
| `app/build/loom.bin` | `1f922dd0edda7093e9768ec9d2ac70dc163454a073ec0c0e19261008a3f76f43` |
| `app/build/loom.fwsc` | `04f0f881fd86d27942b57eea2e5b16cc3e9bff07b3490fec20c7981e9253505d` |
| `app/build/loader/ota.bin` | `4314438cd9cbdeed3f2565a5baaedada6060728f03f1b653acb62ba855eacfb5` |

The workspace's `fm1-reference.sha256` checks these from the firmware repository. Rebuild the unmodified
baseline on a later day if date-dependent output changes; do not replace the baseline with ported output.

## Board-layer checkpoint

- Pro, Elite and MKE development application packages build with their matching identities. The apps are
  593,672 / 593,688 / 593,672 bytes respectively, under the 619,520-byte SMK slot. SRAM use is 472,792 of
  475,136 bytes, leaving 2,344 bytes with the existing 1,984-byte spare requirement retained.
- Matrix contacts, 18 buttons, eight encoders, analog pads/faders/wheels and battery acquisition use the
  SMK board driver. Factory trim is validated and read in place; invalid records use RAM defaults without
  writing calibration. IRQ-taking helpers restore the previous interrupt state.
- Input pin directions, read-only trim, contact timing, debounce, detents, pad order and matrix-table
  uniqueness pass host tests. LCD/pad-LED transport stays in the BSP; GRB encoding/chain order is tested.
  LCD and LED DMA transfers finish or are aborted before changing SPI1 pin groups.
- All 24 stored object IDs are covered by the physical-map tests. Split-pool tests cover every sector,
  full-store refusal, interrupted saves and interrupted owner-tag initialization. Incompatible FELUCCA
  stores are cleared; a matching LOOM8 tag preserves data. Protected regions remain unchanged in the model.
- Restore journal, optional crash history and optional debug statistics use BC000, BD000/BE000 and BF000,
  respectively, inside staging. Their former FM-1 addresses would overlap SMK sample data and are not used.
- Native SMK scenarios pass smoke, sequencing, backup/restore, factory-bank SysEx upload/readback and
  large-font rendering. Project/preset backups restore in both directions between FM-1 and SMK native
  builds, including after reboot. These tests do not execute the hardware `.fwsc` or emulate all SMK pins.
- Raw physical-flash import/export is explicitly unavailable in the native SMK simulator until it models
  the split pool. Project/preset backup and the sample-bank paths remain available.
- All executed Loom and platform suite checks pass. The 1,268 FM-1 DSP fingerprints match; the FM-1 app,
  package and loader hashes still match the original baseline. Browser/Emscripten and real hardware checks
  remain outstanding. No device port has been opened to install and nothing has been flashed.
- The current development artifact hashes are in `app/build/smk37-development.sha256` (generated/local,
  not release material). Full extra-control mapping remains the next area; the knob-label refresh is below.

## Knob-label refresh

The owner reports successful firmware operation in the SMK emulator recorded in `smk37-port.yaml`.
SMK physical knob references now use K1–K8, including parameter-strip identifiers, footer hints, help
cards and screen-reader output. Button names remain Loom's. MIDI CC settings distinguish K5–K8 on the
Engine and Engine 2 pages; their parameter routing is tested independently of the label text.

Directed SMK UI tests and normal/large-type layout sweeps pass without clipping findings. Loom's executed
suite passes and all three rebuilt FM-1 artifacts remain byte-identical. Refreshed SMK apps are
594,312 / 594,312 / 594,296 bytes (Pro / Elite / MKE); RAM use and the flash map are unchanged. Browser
interaction in the external emulator and real hardware were not automated or exercised by the agent.
