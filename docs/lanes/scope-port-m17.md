# Scope PORT-M17 — P1-6: every minifigure vertex was multiplied by zero

> **PORT-M17 — Status: DONE (2026-09-12)** — branch `scope/PORT-M17`, cut from
> the PORT-M16 merge (`be436d48`). Files changed: `LEGOLAND/person3d.c` (three
> `#ifdef LEGOLAND_PORTABLE` macro arms), `portable/src/browser/main.c` (23 new
> `LL_DBG_TABLE` rows), `docs/LEGOLANDPROGRESS.HTML` (one line number),
> `docs/SCOPE_PORT_WAVE.md` (the status line), this file, and the replay
> `portable/src/browser/replays/m17-01-the-visitors-are-multiplied-by-zero.js`.
> Brief: `docs/SCOPE_PORT_WAVE.md`.

**Result in one line: `FMUL`/`FMULA`/`FMULP` are inline asm in the original and
`mov ecx, <multiplier>` takes the multiplier's 32 BITS; every multiplier
`Draw3DPersonModel` hands them is a `float` lvalue that `TOFIX` has already
overwritten IN PLACE with a 16.16 INTEGER, so its float VALUE is a denormal near
4e-41, and the portable arms passed that VALUE into `LL_FMUL16`, whose `(int)`
cast turns it into 0. Every transformed vertex of every visitor was multiplied
by zero, all three corners of all 79 triangles collapsed onto one point,
`crs1 - crs2` came out 0 instead of negative, the back-face test threw the lot
away and not one pixel was ever written. The fix is `LL_ASINT` on the
multiplier in the portable arm — three lines, portable arms only, VC6 text
untouched, `audit.py` byte-identical.**

| PORT-B12's candidate | verdict |
| --- | --- |
| **1. `GetVideoSurface` returns 0 — the surface is not locked during the walk** | **REFUTED.** `g_surface`, `g_pitch`, `g_width`, `g_rows`, `g_mouse_pixel` and `g_clip_x0..y1` are all live and all move every frame. The surface IS locked (§2) |
| **2. `Draw3DPersonModel`'s portable `FMUL`/`FMULA`/`FMULP`/`TOFIX`/`SHADE` arms** | **THE ROOT CAUSE**, and it is `FMULA`/`FMULP`/`FMUL`'s multiplier, not `TOFIX` and not `SHADE` (§4) |
| 3. `Push/PopRenderingStatus` pairing, `g_video_locked` a split object | not implicated. `g_video_locked` is one object, the pair is balanced, the lock holds across `RenderView` (§2) |
| B12 §3.5 "`Draw3DPersonModel` never reaches its vertex loops" | **A FALSE NEGATIVE.** The loops run and write 3·66 + 66 words every frame — they wrote ZEROES, and a frame-to-frame diff cannot see a loop that keeps storing the same constant (§3) |
| owed: the same probe on the TUTORIAL park | **done (§6). Blokes draw there too with the fix, and drew in neither park without it.** `StartFreePlayPark` is not implicated; there was never a free-play-specific difference |

---

## 1. What was measured, and how

`portable/build-wasm` (clean, `-DCMAKE_BUILD_TYPE=Release -DLL_ILP32=ON`) served
on **8843**, `legoland.html?args=-nointro+WINDEBUG&awake=1`, this lane's own tab
(tab-58 — another lane navigated the shared tab out from under the first
attempt; **check `location.href` before trusting any reading**), virgin IDBFS
before every run, driven with PORT-P1's `P1.start()` prelude unchanged. Cold-load
frame hash `0x093021ac` on every build in this lane, pre-fix and post-fix, which
is also the proof that the fix moves no front-end pixel.

Replay: `portable/src/browser/replays/m17-01-the-visitors-are-multiplied-by-zero.js`.

---

## 2. The `GetVideoSurface` gate is CLEAR — with no game-side tracing at all

B12 wanted a host trace or an `LL_DBG_TABLE` row to answer this. It needs
neither, because of **who writes the globals on the far side of the gate**. In
the whole tree:

| function | address | callers |
| --- | --- | --- |
| `SetRenderTarget` (rin.c declares it `SetRasterTarget`) | 0x00485f30 | **exactly one**, rin.c:556 |
| `SetMousePixel` (declared `SetRasterOrigin`) | 0x00488700 | **exactly one**, rin.c:557 |
| `Render_SetViewport` | 0x00441800 | **exactly one**, rin.c:558 |

Those three statements are *immediately after* `if (!GetVideoSurface(&vs)) return;`
and *immediately before* `Draw3DPersonModel(p)`. So `g_surface` / `g_pitch` /
`g_width` / `g_rows` / `g_mouse_pixel` / `g_clip_x0..y1` stay at their `.bss`
zeroes for as long as no person has ever cleared the gate, and hold the last
drawn person's window the moment one has. Added to `main.c`'s table as indices
84..106 (`g_clip_x0..y1` and `g_vp_left/top/right/bottom` are the same four
objects under two names — tri3d.c's and sweep1.c's — and the generated globals
alias them correctly, so one read covers both).

Live, in a running free-play park with 30 visitors:

```
g_surface      12572136 .. 12776918   (moves every frame)
g_ddsd_bits    12399456               the locked back surface
g_surface - g_ddsd_bits = sy*1280 + sx*2   for the person drawn last
g_pitch 1280   g_width 640   g_rows 480   g_mouse_pixel 12906124
viewport       (0,0,160,120) for a person fully on screen
               (0,0,68,77) / (137,0,160,120) / (0,77,74,120) when clipped
```

**The video surface is locked while `DrawAndClearPrintList` walks the list**, the
`IntersectRect` clip is doing real work, and the raster target is the right
pixel inside the right surface. Candidate 1 is dead. (It was always going to
be: `RenderView` runs *inside* gameframe.c:591's
`PushRenderingStatusAndLockVideoSurface` … :720 `PopRenderingStatus`, and that
function stores `g_video_locked = 1` unconditionally.)

---

## 3. The vertex loops DO run — B12 §3.5 is a false negative

`g_xverts` reads back all zeroes and never changes between frames, which B12
read as "the loops are never reached". Write a sentinel over both scratch arrays
instead of diffing them:

```js
for (let i=0;i<600;i++) I[(g_xverts>>2)+i]  = 0x5ca1ab1e;
for (let i=0;i<300;i++) I[(g_vert_key>>2)+i] = 0x5ca1ab1e;
// 600 ms later:
```

| array | words overwritten | expected |
| --- | --- | --- |
| `g_xverts` | **198 of 600** | 3 · nverts, nverts = 66 |
| `g_vert_key` | **66 of 300** | nverts |

Exactly the two loops' footprints. The loops run on every one of the
~19,900 `Draw3DPersonModel` calls this lane counted — **they were writing
zeroes**, and a diff of two snapshots of a constant is empty. The lesson is
general: *to prove a store never happens, poison the target and see whether the
poison survives; never diff two samples of it.*

The mesh is also fine — a heap scan for `Frame3D` records (0x38 bytes, `n_verts`
+0x18, `verts` +0x1c, `FaceSet` +0x20, `n_normals` +0x24, every frame pointing at
the SAME `FaceSet`) finds the bloke animation's frame array at a 0x38 stride:
**66 vertices, 213 normals, 79 triangles of which 67 are Gouraud.**

---

## 4. The root cause

### 4.1 The trace

A throwaway instrumented build (the counters and the `printf` in a HOST TU —
see §7 for why they cannot live in a game TU) reporting every 240th
`Draw3DPersonModel` call, in a free-play park with 28 visitors:

```
HOST M17 call=19680 nv=66 nf=79 ng=67 zs=29093 lo=-590527 hi=590527
         ox=5242880 oy=5898240 fxyz=29295/65536/29295 par=0
         p1=0/67 p2=0/12
         v0=5242880,5898240 v1=5242880,5898240 v2=5242880,5898240 crs=0/0
         surf=12488816 pitch=1280 vp=0,0,160,120 px=0 fill=0/0/0/0
```

Four facts in one line:

* `v0 == v1 == v2 == (5242880, 5898240)` = **exactly `(ox, oy)`** = `(0x500000,
  0x5A0000)`. All three corners of triangle 0 are the same point, so
  `dx1 = dy1 = dx2 = dy2 = 0` and `crs1 = crs2 = 0`.
* `ox` and `oy` are **exactly** `0x500000` / `0x5A0000`, i.e.
  `ox = 0x500000 - ((hi - lo) >> 1)` had `(hi - lo) >> 1 == 0`: the scaled
  bounding box `sc[]` is all zeroes too.
* `p1=0/67 p2=0/12` — `crs1 - crs2 < 0` is false for **every** face, in both
  passes, because 0 is not negative. The back-face test rejects the whole model.
* `px=0 fill=0/0/0/0` — `DrawGouraudTri`, `DrawGouraudTexTri`, `DrawFlatTri` and
  `DrawFlatTexTri` are **never entered**. Nothing downstream of person3d.c is
  implicated: tri3d.c's portable arms are innocent.

`lo`/`hi` (`±590527`) and `zscale` (29093) are healthy because they are computed
from `g_xverts` **before** the `FMULP` scaling; the per-vertex loop is where the
zeroes appear.

### 4.2 Why

```c
float fx, fy, fz;                 /* and ydep, zb */
fx = p->scale_x * 0.447f;         /* 0.447 as a float            */
TOFIX(fx);                        /* now holds the INTEGER 29295 */
...
FMULA(sc, box, 0, fx);            /* mov ecx, fx  -> 29295       */
FMULP(vptr, 0, fx);               /* mov ecx, fx  -> 29295       */
```

`TOFIX` is `fld x / fmul k65536 / fistp dword ptr x` — it overwrites the float's
storage with a 16.16 integer and leaves the variable declared `float`. Every
later use goes through an `__asm { mov ecx, fx }`, which is a **32-bit bit
move**. The portable arms were:

```c
#define FMUL(r, a, b)     ((r) = LL_FMUL16((a), (b)))
#define FMULA(d, s, n, m) (*(int*)((char*)(d)+(n)) = LL_FMUL16(*(int*)((char*)(s)+(n)), (m)))
#define FMULP(p, n, m)    (*(int*)((char*)(p)+(n)) = LL_FMUL16(*(int*)((char*)(p)+(n)), (m)))
```

and `LL_FMUL16(a, b)` is
`((int)(((long long)(int)(a) * (long long)(int)(b)) >> 16))`. That `(int)(b)` is a
**float-to-int conversion of the VALUE**, and the value of a `float` whose bits
are 29295 is the denormal 4.10e-41, which converts to **0**. 65536 (fy, 1.0 in
16.16) is 9.18e-41 — also 0.

So `FMULA` zeroed the scaled bounding box (hence `ox`/`oy`), `FMULP` zeroed
every transformed vertex (hence the degenerate triangles), and the whole visitor
population rasterised to nothing, silently, with no trap. `FMUL(t, t, zscale)`
and `FMUL(crs1, dx1, dy2)` were always fine — `zscale`, `dx1`, `dy2` are `int`.
`FMUL(yy, yy, ydep)` and `FMUL(t, t, zb)` had the same defect but could not show
it today: `ydep` is only `TOFIX`'d when `p->ydepth != 0` (every visitor reads 0)
and `zb` only matters when `p->f2c != 0` (also 0 for visitors). They are fixed
with the rest.

### 4.3 The fix

```c
#define FMUL(r, a, b)     ((r) = LL_FMUL16((a), LL_ASINT(b)))
#define FMULA(d, s, n, m) (*(int*)((char*)(d)+(n)) = \
                              LL_FMUL16(*(int*)((char*)(s)+(n)), LL_ASINT(m)))
#define FMULP(p, n, m)    (*(int*)((char*)(p)+(n)) = \
                              LL_FMUL16(*(int*)((char*)(p)+(n)), LL_ASINT(m)))
```

`LL_ASINT(x)` is `(*(int*)&(x))` (ll_portable.h:16, and the portable build
already compiles `-fno-strict-aliasing`): the identity on an `int` lvalue and
**exactly `mov ecx`** on a `float` one. `a` is an `int` lvalue at all five
`FMUL` sites and is left alone; the comment in the source says so, and says what
to do if that ever changes. Only the `#else` arms move, so VC6 sees the
original text byte for byte.

### 4.4 The class, swept tree-wide — three sites, and they were all here

`/private/.../scratchpad/sweep_fmul.py` (scratch): every `LL_FMUL16` site in
`LEGOLAND/*.c` whose operand names a local declared `float` or `double` in that
file, excluding the ones already wrapped in `LL_ASINT`.

| file | `LL_FMUL16` sites | float-valued multipliers |
| --- | --- | --- |
| `person3d.c` | 10 | **3 — the macros, fixed here** |
| `tri3d.c` | 111 | 0 (`recipA/B`, `spanrecip`, the `x/y/z/u/v/s` deltas are all `int`) |
| `math3d.c` | 9 | 0 (`TransformVectorsL`'s `m`, `x`, `y`, `z` are `int`; the sweep's 18 hits there are name collisions with `float` locals in *other* functions of the same file) |

The class is closed. The generalisation worth keeping: **wherever an `__asm`
arm moved a 32-bit operand with `mov`, the portable arm must move BITS. A
`float` local that `TOFIX`/`fistp` has overwritten in place is the trap, and the
compiler cannot warn, because `LL_FMUL16`'s `(int)` cast is a perfectly legal
conversion.** `FIXV`/`LL_ASINT` already existed in this file for exactly this
reason — the macros just did not use them.

---

## 5. The A/B — free play, on ONE build each way

A plain hidden-vs-shown canvas diff is useless in this park: the scenery animates
1,500–6,500 pixels a frame on its own (the entrance flag, the two LEGOLAND
banners, a ride's ornament), which is the same order as thirty minifigures —
that noise floor is exactly why B12 read P1-6 as "confirmed but un-narrowed".
So: build the set of pixels that are **identical across five consecutive frames
with every bloke hidden** (`flags62 |= 0x80`, which `RenderPeople`
(renderlist.c:148) skips), and count changes only inside it. The control is one
more HIDDEN frame against the same mask, which must be near zero by
construction.

| build | static mask | control (hidden vs hidden) | signal (shown vs hidden) | figures on screen |
| --- | --- | --- | --- | --- |
| **pre-fix** | 302,461 / 307,200 | **23 / 0 / 25 px** | **23 / 0 / 25 px** | 6 |
| **fixed** | 302,311 / 307,200 | **14 / 14 / 21 px** | **3575 / 3449 / 3828 px** | 15 |

Pre-fix the signal **is** the control, sample for sample, with six minifigures on
camera. With the fix it is two orders of magnitude above it, the bounding box is
the whole park (28,39)-(608,294), and it works out at ~250 px per on-screen
figure. Both arms are the same source tree with the same build flags; the only
difference is the three macro lines. Zero traps either way, 35.7 fps either way.

A narrower reading of the same thing, taken on a quiet patch of path around the
one visitor then on camera: **hidden vs hidden 0 px, hidden vs shown 347 px.**

`g_xverts` now reads real transformed vertices
(`-89930, -651495, 152318, …`) where it read 198 zeroes before.

---

## 6. The tutorial park — owed by B12, and the answer is "no difference"

Lesson 1 driven with PORT-P2's `P2` helpers (profile `m17`, progress screen,
Lesson 1, both briefing pages, the park; the ride aim at cell (60,56) came out
off-screen on this camera so only the path objective landed, which is enough —
the tutorial gives `visitorLimit` 3 and three visitors arrive). `gameMode` 3,
`numVisitors` 3.

The canvas-diff probe does **not** work here: the tutorial's camera has to be
panned to the visitors, the pan eases for seconds after the cursor leaves the
edge (PORT-M15's `g_scroll_slack_*`), and three visitors walk off camera inside
the measurement. Use the **Z-buffer witness** instead, which needs no pixel
timing at all:

`RenderZBufferObject` (tri3d.c:630) zeroes the whole 128x120 dword Z buffer at
the top of **every** `Draw3DPersonModel`, and the only code that writes it again
is the four tri3d fillers' `*zp = zspan`, once per pixel drawn. So the count of
non-zero dwords in `g_zbuf` is literally *how many pixels were rasterised for
the last 3D person of that frame*.

| tutorial Lesson 1, 40 samples at 110 ms | max non-zero Z cells | samples > 0 |
| --- | --- | --- |
| blokes shown | **350** (of 15,360) | **33 / 40** |
| blokes hidden | **0** | **0 / 40** |

The same probe in the free-play park: **shown max 353, 18 of 40 samples > 0;
hidden max 0, 0 of 40** (and see §6.1 for the run where the hidden arm was not
zero).

350 cells is one whole minifigure silhouette; the samples that read 0 are the
frames whose last person had a clipped viewport (`vp=159,0,160,120` — one pixel
wide). **Blokes draw in the tutorial too.**

And they drew in *neither* park before the fix: these three macros are on the
only path any 3D person takes in any park, and the pre-fix trace reports
`fill=0/0/0/0` — not one of the four fillers entered — on every sampled call.
**P1-6 was never free-play specific, and `StartFreePlayPark` (uimisc2.c:404) is
not implicated.** That closes B12 §3.5's last open item.

---

### 6.1 `flags62 |= 0x80` is NOT a universal "hide" — a correction for the whole wave

Every lane in this wave (P1 §, B12 §3.1, and §5 above) uses `flags62 |= 0x80` as
"hide this bloke", on the strength of `RenderPeople` (renderlist.c:148) skipping
`flags62 & 0xa0`. **rin.c:520 reads the same bit the other way round:**

```c
if (rider && (rider->bloke->flags62 & 0x80))
    IP_RenderBlokeIn3DNow(rider->bloke);
```

So a bloke sitting in a ride vehicle is drawn *because* 0x80 is set — the ride
renders its own occupants, and the bit is how it knows which. In a free-play park
with a ride that has occupants *at that moment* the "all blokes hidden" arm
still rasterises minifigures. Measured on the same build, same park, minutes
apart: one 40-sample window read **shown max 350 / hidden max 212** and a later
one **shown max 353 / hidden 0 in 40 of 40** — it depends on whether a vehicle
happens to be loaded. The tutorial (no ride with riders) is always the clean
**350 / 0**. The walking visitors are the whole of the *difference* either way,
so no conclusion in this lane or in B12 changes — but a hidden-arm reading of
zero is not something to rely on, and an A/B that assumes it will mis-report
intermittently.

---

## 7. Two notes for the next lane

### 7.1 A game TU may not gain data or rodata — the nm gen_link reads will refuse it

The first instrumented build put `int ll_m17[40];` and a `printf` format string
into `LEGOLAND/person3d.c`. The build died in `gen_link.py`:

```
/usr/bin/nm cannot read .../person3d.c.o: invalid data segment index: 116
```

Game translation units today contain **no data and no rodata of their own** —
every global is generated, and every string is an `extern`. Adding either gives
the object a data segment that the nm `gen_link.py` picks up cannot read, so the
closure cannot be computed at all. Nor can the counters simply live in a host
TU as *data*: `closure_filter.py` only drops generated stubs that match its
FUNCTION regex, so `gen_link.py` emits its own `ll_m17` into `stubs.c` and
`wasm-ld` reports a duplicate symbol. **A game-side trace hook has to be a
CALL to a function a host TU defines** (`ll_m17_set/bump/flush` in
`portable/src/hostwin/dsound.c` for this lane, reverted; `fprintf(stderr, "HOST
M17 …")` so the page's `printErr` routes it to `noteTrace` as well as the
console). Worth teaching `closure_filter.py` to drop generated DATA too, if
another lane wants a cheap counter array.

### 7.2 The shared Browser pane, and a build directory that does not keep files

Seven tabs from different lanes were open on one pane; the first `P1.start()` of
this lane was lost because another lane navigated the tab it was running in
mid-walk. Create your own tab, pass `tabId` on every call, and **read
`location.href` back** before trusting a measurement.

Files copied into `portable/build-wasm/` (the replays, for `fetch`) disappeared
once without an explicit clean. Do not keep anything there that is not a build
product; re-copy the replays right before you fetch them, and check with `curl`.

---

## 8. Gates

| gate | result |
| --- | --- |
| `audit.py LEGOLAND/person3d.c` | all `[OK]` / `[WIP]`, rows **identical to base**: `Draw3DPersonModel` 1023i/3511B vs orig 1023i/3523B, mismatch **377**; `PASS: 0 function(s) failed the extent gate` |
| `relocs.py LEGOLAND/person3d.c` | **0 MISMATCH** |
| VC6 `/W3 /O2 /Gy /Gd` on person3d.c | clean (no diagnostics) |
| `progress.py --check` | `665/675 exports exact (98.5%); 3281 exact functions total; 42 WIP` — **unchanged** (the report was regenerated: `Draw3DPersonModel`'s cited line moves 1089 -> 1108) |
| `portable/tools/extern_sweep.py` | `0 multi-address extern statement(s) -- the class is closed` |
| `portable/tools/bvstruct_sweep.py` | `0 unaccepted silent site(s), 1 silent, 5 noisy` (baseline) |
| `tools/port_m10_bvstruct_sweep.py` | `SLOT vs BODY ... 0 site(s)` (baseline) |
| `portable/tools/addr_sweep.py` | the 6 baseline ADDR rows + `Track_Update`, unchanged (no declaration in the tree moved) |
| native clean build + `ctest` | `legoland_linkcheck legoland_tests` built, **19/19 passed** |
| wasm clean build + `ctest` | all seven targets built, **26/26 passed** |
| browser, free play | 30 visitors, 0 traps, 35.7 fps, cold-load hash `0x093021ac` unchanged |
| `tools/verify.py` | **not run** (integrator's gate) |

Nothing owed to another lane. PORT-B12's P1-6 and its tutorial follow-up are
both closed; B12 §3.5's "never reaches its vertex loops" should be read with §3
above.
