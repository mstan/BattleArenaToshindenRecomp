# Original-disc executable overlay assessment

No executable overlays were found in the inspected USA **SCUS-94200** build.
The resident boot executable contains the game logic; the inspected CD loader
swaps graphics, animation, and sound data. This assessment does not justify
additional overlay AOT shards or a new binary release.

This is a bounded static investigation of the supported disc, not a universal
absence proof or an exhaustive gameplay test. Existing boot-executable AOT and
runtime fallback remain unchanged. Other regions or revisions are unassessed.

## Original-byte authority

The source was the original 14-track USA CUE/BIN dump. Track 01 has 14,662,368
bytes (6,234 raw sectors). Tracks 02 through 14 are CD audio.

| Source | SHA-256 |
| --- | --- |
| Track 01 | `13e58a261952047077217bff12e10a3fea7053ef793d93433e6ecdf54c9f285b` |
| `SCUS_942.00` | `928c495ca0e9221d034085e17a79e81789738b0c389898c1d5f2384f51330f66` |

The ISO directory contains `SYSTEM.CNF` (LBA 23, 68 bytes) and `SCUS_942.00`
(LBA 24, 278,528 bytes). The executable header loads `0x43800` bytes at
`0x80175000`, with entry PC `0x80196CA8`. Its loaded image ends at `0x801B8800`.

The ISO listing alone is insufficient: raw assets occupy LBAs **165 through
6083**, outside the directory. LBAs 160 through 164 and 6084 through 6233 have
zero user data. The inspection included those raw assets. No RAM capture,
historical compiled fragment inventory, or runtime cache served as authority
or build input.

## Loader and consumer evidence

Names below describe behavior inferred from the original instructions.

| Original address | Behavior and implication |
| --- | --- |
| `0x8019CCB4` | SDK `CdRead`: sets mode, sector count, and destination; starts the sector transfer. |
| `0x80177950` | Game read wrapper: takes BCD location, destination, and sector count; issues Setloc and `CdRead`. There are 17 direct calls to this wrapper. |
| `0x80177A34` | Asset-loading state machine. Together with the wrapper above, the range `0x80177950..0x8017913C` contains all 25 direct game calls to `CdRead`. The remaining direct call is the SDK synchronous wrapper at `0x8019E29C`. |
| `0x801B6810..0x801B69E0` | Resident BCD locations for textures, sound banks, animation data, and models. Counts are supplied by the accompanying tables and loader constants. |
| `0x80197A7C` | Texture uploads receive an image rectangle and raw pixel pointer. Character uploads use the loaded palette/pixel buffers; no loaded CPU entrypoint is called. |
| `0x80195344` | TMD model setup: reads object count at `+8`, adds the model base to vertex/normal/primitive offsets in `0x1C`-byte object records, and calls resident primitive conversion at `0x80193BA4`. This is data relocation, not executable relocation. |
| `0x80196EA0` | Sound-bank setup: sends the VAB header through `0x801A0AC8` and the body through `0x801A49E4`; the latter reaches SPU transfer code. Loaded banks have the `pBAV` byte signature. The sequence asset at LBA 538 begins `pQES`. |
| `0x8019DD94` | Sector-ready callback: transfers a sector and advances the destination by the configured word count. Completion signals the registered event callback. |
| `0x8019E148` | The CD data-completion callback installed by initialization/reset. It calls the resident BIOS event wrapper with event `0xF0000003`; it does not jump into the transferred bytes. |

The read destinations observed in these loader paths are asset buffers below
the resident executable. For example, character sound headers are read to
`0x80010000` or `0x80087800`; character texture/model work uses separate lower
RAM buffers; stage textures/models use `0x80106800` and `0x80126800`.
The menu read at `0x8018BC1C` loads `0x80` sectors into `0x80135000`, ending
exactly at the executable's `0x80175000` start.

The post-read paths inspected above perform texture upload, model-offset
fixup/primitive conversion, sound transfer, and normal state transitions. No
decompression or copy followed by execution of the resulting bytes was found.

The game's indirect dispatch was also checked. Thirteen switch tables and
the 45-entry action table at `0x801AC528` contain **224 pointers**, all inside
the resident executable. In particular, the action dispatch at `0x8017D02C`
uses an index from character state to look up a resident handler in that table;
it does not treat loaded animation data as a code pointer. The SDK has its own
resident callback and BIOS dispatch paths; this is not a formal proof of every
possible indirect target.

## Reproducible evidence and limits

[`overlay-assessment.json`](overlay-assessment.json) records source hashes,
the read callsites, dispatch-table bounds, hashes of the inspected code/data
ranges, and 162 original instruction words. Its `loader_evidence` uses the
shared framework's existing `verify_evidence` format. It is an assessment
record, not an overlay extraction profile or release input.

The word checks can be repeated using a framework checkout containing
`tools/aot_overlay_pipeline.py` (for example, commit
`0e0644631094805071740e51dc398c49c2ed869e`):

```python
import hashlib, json, sys
from pathlib import Path

# Supply the framework tools directory and your original CUE path.
sys.path.insert(0, sys.argv[1])
from aot_overlay_pipeline import Disc, verify_evidence

profile = json.loads(Path("docs/overlay-assessment.json").read_text("utf-8"))
disc = Disc(sys.argv[2])
source = profile["source"]
assert hashlib.sha256(Path(disc.binary).read_bytes()).hexdigest() == source["data_track_sha256"]
exe = disc.read(source["boot_exe"])
assert hashlib.sha256(exe).hexdigest() == source["boot_exe_sha256"]
verify_evidence(disc, profile["loader_evidence"])
for region in profile["ranges"]:
    offset = 0x800 + int(region["address"], 0) - int(source["boot_load_address"], 0)
    assert hashlib.sha256(exe[offset:offset + region["bytes"]]).hexdigest() == region["sha256"]
print("Original-disc identity, loader words, and inspected ranges verified")
```

A supplementary scan of every user-data byte outside the boot executable
found no further `PS-X EXE` header and no `jr $ra` instruction encoding
(`08 00 E0 03`), including unaligned matches. These observations support the
loader findings; they cannot independently rule out compressed, synthesized,
or unconventional machine code.

Validation on September 11, 2026 re-read the original disc, verified the
recorded identities/instructions/ranges, checked the 224 resident table
targets, and repeated the raw-sector/signature checks. There is no new
runtime binary to smoke-test or gameplay change to accept from this task.
