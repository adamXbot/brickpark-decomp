# Scope PORT-M20 — M19-1: the Spider Ride asked its `.bnv` for "manbox00"

> **PORT-M20 — Status: DONE (2026-09-12)** — branch `scope/PORT-M20`, cut from
> `origin/main` `606600dc` (the PORT-M19 merge). Files changed:
> `LEGOLAND/mechrides.c` (one `#ifdef LEGOLAND_PORTABLE` arm around one
> `extern` declaration), `docs/LEGOLANDPROGRESS.HTML` (line numbers only),
> `docs/SCOPE_PORT_WAVE.md` (the status line), this file, and the replay
> `portable/src/browser/replays/m20-01-the-spider-ride-asked-for-manbox00.js`.
> Brief: `docs/SCOPE_PORT_WAVE.md`.

**Result in one line: `sprintf_w` is `sprintf`, and `mechrides.c` is the only
file in the tree that declares 0x0049e573 with a FIXED argument list instead of
`...`. On x86 the two prototypes are the same call; on wasm32 a variadic
callee's third parameter is a POINTER to the varargs buffer, so the four BNV
rides handed `sprintf` their SEAT NUMBER where it reads a `va_list`, and every
`"%02d"` path name they build came out `"…00"`. There is no `manbox00` in
`spideron.bnv`, so `GetObjectFromName` returns 0, `GetZSkew` divides 0 by 0,
`UpdateBlokeFromBNVPath` normalises a NULL orientation and writes nine NaNs to
address 0x10, `ApplyObjectOrientationToPerson` `fistp`s them to zero, and
`Draw3DPersonModel` then divides `0x40000000` by a zero z extent. The fix is
one declaration in a portable arm; VC6 sees the original text and `audit.py`'s
rows are byte-identical.**

| PORT-M19's reading | verdict |
| --- | --- |
| the rider's `p->matrix` (+0x58) is all zero, so `SetPersonRotation` "was never called for that person" | **the inference is wrong, the measurement is right.** `SetPersonRotation` is not on the rider's path at all: `ApplyObjectOrientationToPerson` (bnvpath.c:163) OVERWRITES +0x58 on every BNV tick, and nine `LL_FISTP(NaN)` are nine zeros (§3) |
| `ydepth` (+0x38) is a quiet NaN | **the same store, one line later.** `local.person->height = path->person_height + path->person_height` (bnvpath.c:373), and `person_height` is `GetZSkew`'s `0/0` (§3) |
| "the shipped x86 build would fault on the same `idiv`" | **it would not, because it never gets there.** The original pushes the seat VALUE (`push eax` at 0x004164a9) and gets `manbox<seat>`; the divisor is never 0. On x86 a NULL `object` would have faulted *earlier*, on the write to 0x10 (§4) |
| "which path creates a rider's `Person3D` without `Add3DBlokeToList`'s `UpdatePersonPos`" | **no such path.** The record is a perfectly ordinary `Person3D`; the BNV mount wrote garbage over its matrix (§3) |
| M19-3: `Person3D` +0x3c is `zboost` / `depth` | **not implicated.** +0x3c is written by `SpiderRide_Activate` and read by `Draw3DPersonModel`; both builds agree, and it holds 160.0f either way. The pair that matters is +0x38 (§3) |

---

## 1. What was measured, and how

`portable/build-wasm` (clean, `-DCMAKE_BUILD_TYPE=Release -DLL_ILP32=ON`)
served on **8900**, `legoland.html?args=-nointro+WINDEBUG&awake=1`, lesson 3,
this lane's own tabs (63 and 66 — **check `location.href` before trusting any
reading**, another lane navigated a shared tab out from under PORT-M17). Cold
load hash `0x8d5fc480` with a profile in slot 1, pre-fix and post-fix, which is
also the proof that the fix moves no front-end pixel.

Replay: `portable/src/browser/replays/m20-01-the-spider-ride-asked-for-manbox00.js`.

### 1.1 Reaching a rider in ninety seconds instead of twenty minutes

PORT-M19 reached the rider by finishing all eight of lesson 3's objectives
(247 flower beds, ~20 minutes). None of that is load-bearing. M19-1 needs
exactly two things:

1. **a Spider Ride standing, with its `SpiderRec`.** Poke the class's LLIDB
   element flags to `0x17` and the panel builds its icon by the game's own
   rule — `(flags & 0x13) == 0x13`, fpui2.c:602 — then build it from the panel
   with `M19.buildBig` (the footprint is 16x18, so the viewport sweep cannot
   find an anchor; M19's map search can). It went down at (11,13) on both runs.
2. **one bloke on the class rider list** with `state == 0` and `action == 0`.
   `M20.board()` writes the same `RiderNode {next, prev, bloke, ride_id,
   person}` at `ObjDef+0xcc` that `rides.c`'s `PutWorkerOnRide` (0x0049a0d0)
   writes, sets `flags62 |= 0x20`, and zeroes the state and the action.

Both are page-side writes of values the game itself writes; nothing in the game
is patched to get there. **Visitors**: lesson 3 starts at `g_visitor_limit` 0
and recomputes it from the park every tick, so one write is undone within a
second — a 20 ms interval holding it at 19 lets `SpawnVisitor` fill the park to
`maxBlokes` (19 blokes here, against M19's 11).

**Why no visitor ever boards by itself in this park**: `llLink('SPIDER RIDE')`
answers `SATISFIED` (its ring path square joins the network at (17,23), every
square `reachable: true`), the `SpiderRec` exists and is all zeros, and
`ObjDef+0xcc` stayed 0 over 25,000 frames with 19 visitors, a power station and
money climbing. Whatever makes the high-level AI choose a ride did not choose
this one; **that is not M19-1 and this lane did not chase it** (§7).

---

## 2. The root cause

### 2.1 One declaration

`LEGOLAND/mechrides.c:3135`, the only place in the tree that spells 0x0049e573
with a fixed argument list:

```c
extern int    sprintf_w(char* dst, const char* fmt, int v);  /* 0x0049e573 */
```

Every other file that names the same address declares it variadic — `Format`
in fpui2.c, loaders.c, loadmap.c, misc3.c, popup.c, popup2.c:

```c
extern int    Format(char* dest, const char* fmt, ...);      /* 0x0049e573 */
```

`gen_link.py` forwards both to libc (`portable/build-wasm/gen/aliases.c:9,10,32`):

```c
extern int ll_crt_sprintf_va(char*, const char*, void*) __asm__("sprintf");
unsigned int Format   (unsigned a0, unsigned a1, unsigned a2) { return ll_crt_sprintf_va((char*)a0, (const char*)a1, (void*)a2); }
unsigned int sprintf_w(unsigned a0, unsigned a1, unsigned a2) { return ll_crt_sprintf_va((char*)a0, (const char*)a1, (void*)a2); }
```

That third parameter is **`sprintf`'s `va_list`**, because that is how clang
lowers a variadic function for wasm32: the caller writes the arguments into a
stack buffer and passes its address. The forwarder is right for `Format`, whose
callers hand it a buffer pointer, and wrong for `sprintf_w`, whose callers hand
it an `int`.

**Nothing in the toolchain or in this project's gates can see it.** Both are
`(i32, i32, i32) -> i32` at the wasm level, so `wasm-ld` links them silently,
`linkreport.py` counts one clean forwarder and `name_trap.py` reports **0
mismatched signatures** (it did before this lane and it does after). The C
prototypes never meet: one is in `LEGOLAND/mechrides.c`, the other in a
generated TU.

### 2.2 The eleven-line repro

`scratchpad/abi/{game,aliases,main}.c` — mechrides.c's declaration and the
generated forwarder, copied verbatim, and nothing else:

```
== wasm32 (emcc, the portable build's target) ==
seat  1  fixed-proto -> "manbox00"   variadic-proto -> "manbox01"
seat  2  fixed-proto -> "manbox00"   variadic-proto -> "manbox02"
seat  3  fixed-proto -> "manbox00"   variadic-proto -> "manbox03"
```

`"00"` and not garbage because the seat number, used as a pointer, lands in
emscripten's reserved low memory, which reads back zero.

### 2.3 What the original does

`tools/disasm.py original/legoland.exe 0x16330` — `SpiderRide_Activate`'s case 0:

```
0x004164a4: call    0x416830          ; SpiderRide_SeatOf -> eax
0x004164a9: push    eax               ; the SEAT VALUE
0x004164aa: push    0x4b4704          ; "%02d"
0x004164af: push    0x4b4d9a          ; &g_spider_pathname[6]  (0x4b4d94 + 6)
0x004164b4: call    0x49e573          ; sprintf
```

A cdecl vararg *is* a pushed dword, and a fixed `int` parameter compiles to the
same push, which is why the fixed prototype was never wrong on x86 and is why
`audit.py` and `relocs.py` cannot tell the two apart (§6 — they do not).

### 2.4 The eight sites

All eight callers are the `"%02d"` seat suffix of a BNV path name, one pair per
BNV-path ride:

| ride | template | file:line |
| --- | --- | --- |
| Safari | `manBox??` 0x004b4cac | mechrides.c:3271, 3326 |
| Spinning Barrels | `BlokeBox??` | mechrides.c:3550, 3590 |
| **Spider** | `manbox??` 0x004b4d94 | **mechrides.c:3759, 3816** |
| Plane (Zoomer) | `manbox??` | mechrides.c:3995, 4055 |

So all four mechanical BNV rides carry M19-1; the Spider is simply the one
lesson 3 puts in front of you.

---

## 3. The chain from "manbox00" to a divide by zero

Read off the **live heap of the running game**, with `M20.bnv()` replaying
`GetBinVFrame` (sweep1.c:203), `GetObjectFromName` (bnvpath.c:135), `GetVertex`
(sweep1.c:222) and `GetZSkew` (math3d.c:195) over the loaded `spideron.bnv`:

| name asked for | object | vertex 0 | `GetZSkew` | `object->orientation[0..2]` |
| --- | --- | --- | --- | --- |
| `manbox01` | 0x19ef9f9 | 0x19efa2d | **-0.22060272** | (0.4880, -0.8728, -1.7e-10), length 1 |
| `manbox00` | **0** | **0** | **0/0 = NaN** | reads address 0x10 = **(0, 0, 0)** |

and then, in order:

1. `NewBNVPath` (bnvmove.c:313) — `path->person_height = GetZSkew(bin, object,
   vertex)` = **NaN**. The guard in `GetVertex` (`if (!p) return 0`) is what
   turns a null object into a null vertex instead of a fault, and `GetZSkew`
   reads its two floats at addresses 0x0c and 0x10, which are zero.
2. `UpdateBlokeFromBNVPath` (bnvpath.c:272) — `inverse_length = 1.0f /
   sqrt(0+0+0)` = **+inf**, and `object->orientation[i] *= inverse_length`
   writes **nine NaNs to addresses 0x10..0x33**.
3. `ApplyObjectOrientationToPerson` (bnvpath.c:163) — nine
   `LL_FISTP_SCALE_INPLACE(value, 65536.0f)` on NaN. `ll_fistp_f` truncates,
   wasm's non-trapping `i32.trunc_sat` gives 0 for NaN, and the nine zeros land
   on **`Person3D` +0x58 — `p->matrix`**.
4. `local.person->height = path->person_height + path->person_height` — **+0x38
   `ydepth` = 0x7fc00000**.
5. `Draw3DPersonModel` (person3d.c) — a zero matrix maps all 66 vertices onto
   the origin, `lo == hi == 0`, and
   `zscale = 0x40000000 / ((hi - lo) >> 5)` **divides by zero**
   (person3d.c:1351, inlined into `Render3DPerson`).

Every fact PORT-M19 measured on the wreck falls out of step 3 and step 4 — the
all-zero matrix, the quiet NaN `ydepth`, the `rot` of (0,0,0) (nothing on this
path ever writes +0x40), and the all-(0,0,0) `g_xverts`.

**A note for the next reader of a `.bnv`:** `LoadBinV` `HeapAlloc_w`s the file
and relocates in place, and the image's internal offsets are odd — the frame
lists, the nodes and the vertices all land at 1 mod 4. Read them through a
`DataView`, not `HEAP32`, or the index rounds down and you get garbage (this
lane wasted a reading on it). Unaligned loads are legal in wasm and were on
x86; it is not a defect.

---

## 4. The A/B — one build each way, same park, same cell, same poke

Lesson 3, a Spider Ride at (11,13), one bloke boarded by `M20.board()`:

| build | `path->object_name` | `path->person_height` | `p->ydepth` bits | `p->matrix` | bloke flags62 | the loop |
| --- | --- | --- | --- | --- | --- | --- |
| **pre-fix** | **`"manbox00"`** | **NaN** | **`0x7fc00000`** | **all zero** | `0xab` | **DEAD** |
| **fixed** | `"manbox09"` | `-0.22060275` | `0xbee1e5b0` | `[29538,0,-58502, 0,-65536,0, 58502,0,29538]` | `0xab` | alive |
| fixed, again | `"manbox12"` | `-0.22060271` | `0xbee1e5ad` | `[34589,0,-55665, 0,-65536,0, 55665,0,34589]` | `0xa9` | alive |
| fixed, again | `"manbox13"` | `-0.22060269` | `0xbee1e5ac` | `[34483,0,-55731, 0,-65536,0, 55731,0,34483]` | `0xa9` | alive |
| fixed, again | `"manbox15"` | `-0.22060281` | `0xbee1e5b4` | `[32888,0,-56687, 0,-65536,0, 56687,0,32888]` | `0xaf` | alive |

* the name always matches the seat `SpiderRide_SeatOf` handed out, and
  `g_spider_pathname` in the generated globals reads `manbox09` / `manbox12` /
  `manbox13` / `manbox15` where it read `manbox00`;
* `-0.2206027` is **exactly** what `GetZSkew` gives for that seat's first
  vertex read straight out of `gamedata/main/spideron.bnv` with a
  hand-calculator (`f10²/((2·f14−1)·f10 − f0c·f14)`, `f14` = 3666930.0), and
  `ydepth` is exactly twice it;
* each matrix is a real 16.16 rotation: 34589/65536 = 0.5278, 55665/65536 =
  0.8494, and 0.5278² + 0.8494² = 1.000;
* **`0xab` is the flags byte PORT-M19 measured on the broken rider.** The same
  bloke, in the same ride state, with the same bits — the only difference is
  the two digits at the end of the name.

**Pre-fix, the page dies within ~200 frames of the mount.** The trap this lane
saw was not M19's:

```
TRAP RUNTIME RuntimeError: Aborted(Runtime error: The application has
corrupted its heap memory area (address zero)!)
```

which is emscripten's zero-address sentinel catching step 2 — the nine NaN
stores at 0x10..0x33 — before `Draw3DPersonModel` reaches the `idiv`. Which of
the two fires first is a race inside one frame; both are the same missing
`manbox<seat>`, and it is worth recording that **this failure has two faces**
so the next lane does not read them as two defects.

**Fixed, the rider rides.** Boarded, mounted along the BNV path, seated and
alighted — `b->bnvpath` allocated, advanced and freed, `b->action` 0 → 1 → 5 →
… — and the loop ran on at **35.7 fps with 0 traps** for the whole cycle and
past it (~1,000 frames measured after the rider left, ~90,000 frames across the
session). Cold-load frame hash `0x8d5fc480` unchanged by the fix.

**Owed, and only this:** a pixel count for "the rider is DRAWN". The Browser
pane in this session was taken over by another lane before the static-mask
measurement finished (a whole-canvas mask gives control 1225/947/1753 px
against signal 1601/1322/1392 px — the park's nineteen walkers and the Spider's
own arm animation are the same order as one minifigure, exactly the noise floor
PORT-M17 hit and solved by hiding every other bloke with `flags62 |= 0x80`).
The measurement to run is PORT-M17 §5's, with `M20px.run()` in the replay:
hide every walker, unlink the rider's `RiderNode` for the control, relink it
for the signal. What IS established is that the divisor is no longer zero and
that `Draw3DPersonModel` runs to completion instead of trapping.

---

## 5. The class, swept tree-wide

The dangerous shape is: **a game `extern` with a FIXED argument list for an
address `gen_link.py` forwards to a VARIADIC libc function.** The forwarder
list is in `portable/build-wasm/gen/manifest.md`; three of its twelve targets
are variadic.

| forwarded name | libc target | declarations in `LEGOLAND/*.c` | verdict |
| --- | --- | --- | --- |
| `Format` | `sprintf` | 6 files, all `(char*, const char*, ...)` | correct |
| `DebugPrint` | `printf` | 2 files, `(const char* msg)` — and the forwarder itself calls `printf(a0)`, so the varargs buffer is built inside the generated TU | correct |
| **`sprintf_w`** | **`sprintf`** | **1 file, `(char*, const char*, int)`, 8 call sites** | **the bug** |

(`DebugPrintf` 0x0047f870 is the game's own empty logger, defined variadic in
sysstubs.c:158 — not a forwarder.) **One instance; the class is closed.**

The generalisation worth keeping, and it is a sibling of PORT-M10/M11's
by-value class and PORT-B12's overlapping `memcpy`: *on x86 a variadic
prototype and a fixed one are the same instruction stream, so the recovered C
is free to pick either and the byte gates certify both. On wasm32 they are two
different ABIs with ONE wasm signature, so the linker certifies both too.*
Anywhere a matching build cannot tell two spellings apart, the port can still
be wrong, and only a prototype-level sweep finds it.

**Worth adding to the round gate** (owner PORT-A, `gen_link.py` /
`linkreport.py`): the generator knows which libc targets are variadic and it
parses every game declaration already — it can refuse, or at least print, a
non-variadic declaration of a variadic forward. That is a two-line rule that
would have caught this before the first frame.

---

## 6. Gates

| gate | result |
| --- | --- |
| `tools/audit.py LEGOLAND/mechrides.c` | 18 rows, **byte-for-byte identical to the base tree** (`diff` of the two runs is empty); `SpiderRide_Activate` 376i/1228B mismatch 0 |
| `tools/relocs.py LEGOLAND/mechrides.c \| grep MISMATCH` | **empty** |
| `tools/progress.py --check` | regenerated (line numbers only, +14 in mechrides.c); **3281 exact / 42 WIP**, 665/675 exports exact (98.5%) |
| `portable/tools/extern_sweep.py` | **0** multi-address extern statements |
| `portable/tools/bvstruct_sweep.py` | **0 unaccepted** silent sites (1 silent, 5 noisy) — baseline |
| `tools/port_m10_bvstruct_sweep.py` | 0 silent, **0 slot-vs-body** |
| `portable/tools/addr_sweep.py` | baseline unchanged; `ctest -R addr_sweep` 2/2 pass |
| `portable/tools/name_trap.py` | `legoland_headless_debug` linked, **0 mismatched signatures** — *before and after*, which is the point of §2.1 |
| native, clean `rm -rf` | builds; **ctest 19/19** |
| wasm, clean `rm -rf` | builds; **ctest 26/26** |
| session | 0 traps on the fixed build across ~90,000 frames and four boardings; 35.7 fps; the only trap is the pre-fix arm, on purpose |

`tools/verify.py` was not run (hard rule 6). `relocs.py --all` is the
integrator's tree-wide sweep; this lane ran it on the one file it touched.

---

## 7. Owed

* **The pixel count for "the rider is drawn"** (§4), blocked on the Browser
  pane. The recipe and the helper (`M20px.run`) are in the replay.
* **Why no visitor boards the Spider Ride by itself** in a lesson-3 park where
  `llLink` says `SATISFIED`, the `SpiderRec` exists and 19 visitors walk the
  paths for 25,000 frames. PORT-M19 got riders after finishing the lesson, so
  something later in the script — or the park rating, or an attraction score —
  opens the ride. It is a separate question from M19-1 and it is the one that
  decides whether the *player* ever meets this bug; **worth a lane.**
* **The other three BNV rides** (Safari, Spinning Barrels, Plane) carry the same
  eight-site fix and were not driven. They should be, once one of them is
  reachable in a level.
* **PORT-A's gate rule** from §5: make `gen_link.py` print (or refuse) a
  non-variadic game declaration of a variadic CRT forward.
* M19-3's two struct-name disagreements stand and are still worth resolving
  (`Anim3D` +0x04, `Person3D` +0x3c); neither is implicated in M19-1.
