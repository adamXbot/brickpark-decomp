# Scope PORT-A2 — the ILP32 re-pointing blocker, and the spine past the loader

> **Status: deliverables 1-5 done, 2026-09-11.** Branch `scope/PORT-A2` from
> main `8e02a675`. PORT-A's follow-up. Nothing in `LEGOLAND/*.c` is touched, so
> the VC6 gate has nothing to check for this lane either. Verified from CLEAN
> build directories: native `legoland_linkcheck` + ctest 2/2 green; wasm32
> `legoland_linkcheck` runs, ctest 4/5 (`loadpos` is section 4's prototype
> conflict, not a regression -- it used to trap in `ShowWindow` three checks
> earlier).

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

### The ordered sequence, and where it stops

`--stages` walks InitSession's own order (startup.c 0x0047f880) with a line
before each step. The browser page reaches exactly the same point.

| # | step | result |
| --- | --- | --- |
| 1 | version block, mutex, one-instance test | as PORT-A left it |
| 2 | `RES_EnsureMounted(1)`, CD found at `$LL_CD_DIR` | `g_res_path = "D:\"` |
| 3 | `RES_OpenVolume` x3 | 595 + 905 + 574 members |
| 4 | `LoadStrings()` | `GetString(0xcb)` = `"LEGOLAND ERROR"` |
| 5 | `InitHostSystemGPU()` | 1 -- PORT-B's DDRAW |
| 6 | `InitScreen()` | 1 -- 640x480 display opened, window "LEGOLAND" |
| 7 | `InitInputSystem()` | 1 -- PORT-B's DINPUT |
| 8 | `RES_OpenFile(".\graphics\erase it.lls")` | 0x9dbb58, 1402 bytes |
| 9 | `LoadSprite("erase it.lls", 0)` | **`RuntimeError: unreachable` in `__BMPLoader`** |

Steps 5-7 are the whole of PORT-B's graphics and input init and they all
succeed, so **there is nothing here for PORT-B2 to fix**. The two things PORT-B2
should know are in §5.

## 3. Tests

### `legoland_tests` links the host shim now

`loadpos` trapped in `ShowWindow` because the closure the tests link,
`legoland_gen`, has a trapping stub for every DDRAW/USER32/GDI32/DINPUT/WINMM
name while PORT-B's real bodies live in `legoland_hostwin`, which nothing
linked. `cmake/tests.cmake` links the shim and swaps the closure for
`legoland_gen_browser` (the same closure with those stubs filtered out --
linking both would duplicate every one of them). PORT-B's `user32.c` and
`ddraw.c` tolerate node once five browser-only symbols exist: the four
`ll_canvas.js` entry points (`ll_js_display_open` touches `document`) and
`emscripten_sleep`. `portable/src/headless/node_shim.c` stubs exactly those
five and is linked into the node executables only -- **PORT-B's files are not
edited**, as the brief requires.

Two more things that had been hidden behind the `ShowWindow` trap:

* `loadpos` then spun for ever in the real missing-CD loop
  (`while (!RES_FindVolumeOnResPath(...)) MessageBoxA("Please insert the
  LEGOLAND CD-ROM into drive %s")`, sysmisc.c, no other exit). That prober does
  `root[0] = g_res_path[0]` and asks about `"<letter>:\"`, so the test's
  `g_res_path = "./"` was asking the host about a drive called `".:\"`. The
  answer is a drive letter: the test sets `"D:\"` and `tests.cmake` sets
  `LL_CD_DIR=gamedata/disc` for every test. **This is PORT-C's finding 2, and
  the answer is a drive letter, not a cleverer `GetVolumeInformationA`** -- the
  game only ever probes `<letter>:\`.
* `loadpos` now runs every matrix check and finds `earth.pos` in the mounted
  directory, then stops in `LoadPos` -- see §4. It is a real finding, not a
  regression: it is three checks further than the whole test used to reach.

### Verdict on the `llidb_icm` divergence: THE ORACLE WAS WRONG

`LLIDB_FindElement("BuIlD MeNu") image: got "(null)", want ""`.

`LLIDB_LoadICM` (data2.c 0x0047aff0) reads a length and then, for both the
label and the image:

```c
_read(fd, &len, 4);
if (len == 0) { g_llidb_pages[page][i].image = 0; }
else { ...MemAlloc(len + 1)...; _read(fd, ..., len); }
```

A zero length is stored as a **null pointer**, not as a pointer to `""`. The C
is doing exactly what the original does; `tools/oracle_icm.py` was emitting the
field as `""` because that is what the FILE holds, which is honest about the
file and wrong about the loaded record. `value == ""` and `len == 0` are the
same set, so the two are exactly interchangeable and the oracle can say which
it means. Fixed on the oracle side (`c_str_or_null`, which emits `0`), with
`LL_CHECK_STR` taught that a NULL `want` means "expect NULL" -- previously it
could not express that at all. **`llidb_icm`: 47 checks, 0 failed.**

## 4. THE FRONTIER: the 542 prototype conflicts are live

PORT-A found them and could not say whether they mattered. They matter: they
are now the only thing between this port and a drawn frame, and three are
proved live on paths the game takes.

| symbol | defined | declared | proved live in |
| --- | --- | --- | --- |
| `RES_CloseFile` | `int RES_CloseFile(RVol*)` sweep4.c | `void RES_CloseFile(void*)` in 13 files | `LoadPos` (loaders.c) and `__BMPLoader` (screen.c) |
| `RES_CloseVolume` | `int RES_CloseVolume(RVol*)` sweep4.c | `void` in startup.c | `InitSession`'s teardown, startup.c:140/153/161/183 |
| `DBPrintf` | `void DBPrintf(void)` sweep1.c | variadic in 30+ files | `__BMPLoader`'s failure branch |

**How to see one.** A signature mismatch is not a `TRAP` and not an undefined
symbol: wasm-ld emits a stub named `signature_mismatch:<name>` and, at `-O2`,
inlines its `unreachable` into the caller, so it arrives as a bare
`RuntimeError: unreachable` attributed to whatever function made the call.
Rebuild the same objects at `-O0` and the frame appears by name:

```
RuntimeError: unreachable
    at tests_O0.wasm.signature_mismatch:RES_CloseFile
    at tests_O0.wasm.LoadPos
    at tests_O0.wasm.test_loadpos
```

That is the technique to hand the next lane; `--profiling-funcs` (now on
`legoland_headless` and `legoland_tests`) is the half of it that costs nothing.

**Which side is right.** The definition is, every time: it has the body the
matcher validated against the original bytes. `sweep4.c`'s `int` return is real
(the callers in x86 cdecl simply ignored EAX). `sweep1.c`'s
`void DBPrintf(void)` is the awkward one -- the original at 0x00453a20 is an
empty function, and an empty cdecl body compiles to `ret` whether or not it
declares parameters, so `void DBPrintf(const char* fmt, ...) { }` should be
byte-identical AND fix all 30+ call sites at once. That is a matching lane's
call to make and to gate with `audit.py`; this lane did not touch
`LEGOLAND/*.c` at all.

**The cheap sweep**, for whoever takes it: `RES_CloseFile` is 13 files and
`RES_CloseVolume` is one, each a caller-side
`#ifdef LEGOLAND_PORTABLE` / `extern int ...` / `#else` / original / `#endif`
that the VC6 arm never sees. `linkreport.md`'s "Prototype conflicts" section is
the full list; sort it by whether the disagreement is a return type (harmless
in cdecl, fatal on wasm) or arity (`AddBasicObject` is defined with three
parameters and called with two from 21 files -- that one is a real recovery
question).

## 5. For PORT-B2

Nothing in the DDRAW/USER32/GDI32/DINPUT/WINMM shim is blocking the spine:
`InitHostSystemGPU`, `InitScreen` and `InitInputSystem` all return 1 and the
display opens at 640x480. Two things are still owed, both already on PORT-B's
list:

1. **`MessageBoxA` must be able to return `IDCANCEL` (2).** PORT-A predicted
   this; it is now measured. With the shim linked, `loadpos` sat in
   `RES_EnsureMounted`'s retry loop until ctest's 300-second timeout, printing
   `[MessageBox] CD Missing` for ever. In a test the answer is `LL_CD_DIR`; in
   the page, a viewer with no CD gets an unbreakable loop.
2. **The four `ll_canvas.js` entry points are the shim's whole JS surface**, and
   `emscripten_sleep` its whole ASYNCIFY surface. That is a good property; it is
   what let `node_shim.c` stub them in five short functions. Keep it.

## 6. Build hygiene

* `gen-browser` now DEPENDS on `portable/tools/linkreport.py`. `gen_link.py`
  imports it for the object scanner, the source scanner, the classifier and the
  wasm signature reader, so a change there changes the generated closure and
  used to leave it stale. **`portable/CMakeLists.txt`'s own `gen` command has
  the same gap** and only the integrator may edit that file.
* **`-sERROR_ON_UNDEFINED_SYMBOLS=0` is gone from the browser link.** Nothing
  breaks: the link is clean and the page runs the spine to the same point. It
  was there while PORT-A's `--ilp32` work was in flight. Note that it does NOT
  catch the prototype conflicts of §4 -- a signature mismatch is a wasm-ld
  warning and a poisoned call site, not an undefined symbol.
* Build from CLEAN directories after a generator change; the stale-object trap
  in HANDOFF.md is real.
