# Scope PORT-A2 — the ILP32 re-pointing blocker, and the spine past the loader

> **Status: IN PROGRESS (claimed 2026-09-11).** Branch `scope/PORT-A2` from
> main `8e02a675`. PORT-A's follow-up. Nothing in `LEGOLAND/*.c` is touched, so
> the VC6 gate has nothing to check for this lane either.

## 1. THE BLOCKER: pointers to unnamed data (`gen_link.py --ilp32`)

**Fixed.** `--resmount` now opens all three volumes and lists their members:

```
HOST CreateFileA("D:\Legoland.res", access=0x80000000, disp=3)
HOST GetFileSize(1) / SetFilePointer(1, 16399660) / ReadFile(1, 24426 bytes)
legoland_headless: RES_OpenVolume("Legoland.res") = 0x9c88a8
legoland_headless:     ManPan.lowerbod04.3d    38996 bytes @ 4308040
legoland_headless:     altman.txt             80 bytes @ 4417236
legoland_headless:   volume "LEGOLAND" handle 1: 595 members, 17239686 bytes
legoland_headless:   volume "GRAPHICS2" handle 2: 905 members, 120408430 bytes
legoland_headless:   volume "GRAPHICS1" handle 3: 574 members, 20018301 bytes
```

2074 members across the three volumes, sizes and offsets that match the files,
and `RES_CloseVolume` returns 0. Before this change the same probe opened
`"D:\.res"` and returned 0 from `RES_OpenVolume("")`.

### PORT-A's diagnosis was half right, and the half that was wrong matters

PORT-A sketched the fix as *gap blocks*: "it already computes the gaps between
known addresses, so a word pointing into a gap can get a synthetic
`ll_gap_<start>` block". Measured against the real exe, **there are no gaps**.
The generator sizes every global by the distance to the next named address, so
the rebuilt globals already tile the initialised data almost completely:

| section | bytes | covered by the rebuilt globals |
| --- | --- | --- |
| `.rdata` 0x4ab000 | 36,864 | 35,856 |
| `.data` 0x4b4000 | 3,670,016 | 3,669,492 |

So the three literals `"Legoland.res"`, `"Graphics2.res"`, `"Graphics1.res"`
(0x004bcbf8, 0x004bcc08, 0x004bcc18) are not in a gap at all. They are *inside*
the block the generator emits for 0x004bcbf4 — the address that `g_map`,
`g_game`, `g_screen`, `lpConfig` and ten more names share, a 4-byte pointer
whose block runs 472 bytes to the next named address and swallows the string
literals that follow it. The defect was never missing storage; it was that
re-pointing **required the word to equal a symbol address exactly**. The
literals are at offset 4, 20 and 36 of a named block, and nothing re-pointed an
interior address.

The fix is therefore *offset* re-pointing, with gap blocks as the fallback for
the case where nothing covers the target:

1. `w` equals a known symbol's address → `&sym` (what PORT-A already did);
2. `w` lands inside a rebuilt block → `(char*)&sym + off`;
3. `w` lands inside a region the game itself defines under a name → the same,
   against that name (an offset into an object the game owns is an offset into
   that object);
4. nothing covers it → synthesise `ll_gap_<start>` for the region between the
   two nearest known addresses, initialised from the exe, and point into that;
5. `w` is outside `.rdata`/`.data`/`.rsrc` → left raw.

`.text` is excluded from 2–4 on purpose: a wasm function "address" is a table
index, so an offset into the middle of a body means nothing, and an exact
function address is already handled by case 1 with a real declaration.

### Which words are pointers: the declaration, never the value

This is the part that has to be got right, and the value of the word is a
catastrophically bad guide. Re-pointing every in-image-looking word re-points
**1,650** of them, and **1,209 are string TEXT**: a three-character string at
the end of a word is an in-range little-endian integer (`"tan\0"` =
0x006e6174, `"log\0"` = 0x00676f6c, both between 0x401000 and 0x836000). 264 of
those false positives are in the `GUID_NULL` block alone, which is 34 bytes of
GUID followed by several kilobytes of swallowed `.rdata` string literals.

So `gen_link.scan_pointer_decls()` reads the game's own declarations instead:
pointer depth and array extent out of `extern <type> <name><dims>; /* 0x... */`.
`extern const char* g_volume_names[3];` contributes exactly three pointer
slots; `extern const char kThemeSame[];` and `extern const GUID_ GUID_NULL;`
contribute none. An unbounded `extern char* g_male_names[];` is bounded by the
data — the table ends at the first word that could not be a pointer, which is
where the literals it points at begin.

Result, from `gen/manifest.md` (wasm32, `-DLL_ILP32=ON`):

```
- words re-pointed at a symbol address (ilp32): 272
- pointer words re-pointed INTO a block (ilp32): 441
- synthesised ll_gap_ blocks for unnamed data: 0 (0 bytes)
- pointer words left raw (outside 0x4ab000..0x836000): 12
```

441 interior re-points, every one of them a real pointer: the asset-name tables
(`g_copters_spr_names` → `mcop_gs.lls`, `g_lf_track_names` → `fc1a_m.lls`,
`g_ds_samples` → `Car02.wav`, `g_catapult_sample` → `Dunk01.wav`), the name
tables (`g_male_names` 83, `g_female_names` 90, `g_surnames` 107), the volume
table, `g_ride_part_names`, `g_report_names`, `g_capacity_names`, `g_gfx_dirs`.
No string text, no integers.

### The gap path is real, it is just not needed by this exe

Zero gap blocks is a measurement, not an untested branch. Proof, by pretending
0x004bcbf4 has no extern-only name so that nothing covers the literals:

```
dropped from game-data: ['g_game', 'g_map', 'g_screen', 'lpConfig', ...]
extern unsigned char ll_gap_004bcbf4[472];
__attribute__((aligned(16))) unsigned char ll_gap_004bcbf4[472] = { ... };
g_volume_names[0] = (unsigned int)(__UINTPTR_TYPE__)((char*)(ll_gap_004bcbf4) + 36)
```

The block is synthesised from the exe, declared in the prologue with the type
its definition has, and pointed into with the right offset. It will start
earning its keep the moment the rule widens (below) or a pointer lands in the
~1 KB of `.rdata` / 8 KB of `.rsrc` that nothing names.

### Known limit: pointer members of struct arrays are still raw

The declaration rule only sees a global that IS a pointer (or an array of
them). `extern FPTableEntry g_fp_table[0x86];` and
`extern BuildFollowUp g_build_followups[29];` are arrays of structs with
`char*` members, and those members keep their raw x86 values. Measured: a
column test over such arrays (`[N]` known, planned size divisible by `N`,
a 4-byte column whose every non-zero entry is an in-image address, `N >= 8`)
would recover **49 more real pointers in one global**, `g_build_followups`
(`"CASTLE OBJ"`, `"SQUARE_TRACK"`, `"SQUARE_TRACK_HEIGHT"`), and still needs no
gap block. It is not implemented: it is a second heuristic with an arbitrary
minimum, and nothing on the spine needs it yet. `g_fp_table`'s own name column
is not recovered by it either (its planned size is not divisible by 134, so the
stride cannot be derived) — that one wants a real struct layout, i.e. the
matching lanes' `FPTableEntry`.

## 2. The spine, and what it hits next

### `RuntimeError: unreachable` on `RES_CloseVolume` — a prototype conflict that is LIVE

With the volumes opening, the probe died immediately afterwards in
`RES_CloseVolume` with a bare `RuntimeError: unreachable` and no `TRAP` line —
the exact failure mode PORT-A warned about. `startup.c:34` declares
`extern void RES_CloseVolume(void* vol);` while `sweep4.c:54` defines
`int RES_CloseVolume(RVol* v)`. On wasm those are two function types, so
wasm-ld replaces the `void`-typed call with a trapping stub.

This is the **first of PORT-A's 542 prototype conflicts proved to be live on a
path the game actually takes**: `startup.c` calls `RES_CloseVolume(*p)` at lines
140, 153, 161 and 183 — the failure paths of `InitSession`'s volume mount, and
the session teardown. Fixing the harness's own declaration to `int` made it
work and return 0. **For a matching lane: `sweep4.c` is right (it has the
body), `startup.c`'s prototype is wrong.** That is the shape of the remaining
541, and it is worth a lane of its own.
