# Scope PORT-B9 — the park does not render because the park was never loaded

> **PORT-B9 — Status: IN PROGRESS (claimed 2026-09-12 by PORT-B9)**
> Branch `scope/PORT-B9` from `feat/decomp-completion-next-steps-24a0d6`
> @ `3a71d5ad` (the PORT-A7 merge). Files owned: `portable/src/hostwin/ddraw.c`,
> `user32.c`, `gdi32.c`, `dinput.c`, `winmm.c`, `dsound.c`, `avifil32.c`,
> `msacm32.c`, `portable/src/browser/**`, `portable/cmake/browser.cmake`, plus
> additions to `portable/hostwin/include/ll_host.h`. `LEGOLAND/*.c` is
> READ-ONLY for this lane, so the VC6 gate has nothing to check.

---

## 1. Headline

**A7-1 is not a rendering defect.** The DirectDraw shim, `RenderView`,
`RenderGroundLayer` and `PaintTileLayer` all run correctly, every frame, over a
map whose **36,864 cells are entirely zero**. A cell with `tile == 0` paints
nothing (`render4.c:430`), so the terrain pass writes not one pixel and the map
area keeps whatever the previous screen left there. That is the whole of A7-1's
symptom.

The map is empty because the level script never ran a single keyword, and the
level script never ran a keyword because

```c
/* movie.c:239 */
extern const void* g_level_db_sections;   /* 0x004bb6f8 93 {keyword, handler} pairs */
```

declares a **744-byte table of 93 pairs as ONE `const void*`**. PORT-A7's rule —
"a word is a pointer slot when some declaration's type puts a pointer FIELD at
that address" — is therefore never asked about words 1..188 of that object. The
handler halves survive anyway (an exact function address is re-pointed by the
symbol path), but **all 92 keyword-string halves keep their raw x86 VAs**:

```c
/* gen-browser/globals.c:6885 */
__attribute__((aligned(16))) unsigned int g_level_db_sections[189] = {
    (unsigned int)&g_str_none, (unsigned int)&LevelKw_none,
    0x004bbdc4u, (unsigned int)&LevelKw_AGES,        /* <- "[INIT]"  is a raw VA */
    ...
    0x004bc028u, (unsigned int)&LevelKw_MAP,         /* <- "MAP"     is a raw VA */
```

`ParseKeywordSections` (`levelkw.c:930`) does `strcmp(words[0], table[i].keyword)`
against those, reads a wild address, never matches, and the script parses to
nothing. `MAP "one"` never fires, `LoadBaseMap` never runs, the map stays zero.

This is the **same class** PORT-A7 closed, in the one shape its census could not
see: A7 §5's residue test asked "is this word past the DECLARED EXTENT of its
object" and answered yes for all 188 of them — correctly, because the
declaration is four bytes long — and then concluded "so no declaration describes
that storage, so it is not a pointer". Here the storage is a pointer table and
the declaration is simply wrong about its size. A7's own residue table names it:
`g_level_db_sections 91`.

**Proved by repair, live in a tab**: `llFixKeywordTable()` translates the 91 raw
VAs through the object that contains each one and writes the linear address
back — the arithmetic `gen_link.py`'s interior-of-block rule would have done at
build time. All 91 then read as real keywords (`[INIT]`, `MAP`, `LOAD`, …), and
the very next park load runs `LevelKw_MAP` -> `LLIDB_FindElement("one")` ->
`LoadBaseMap`, which reads `ONE.MAP`'s header (**the map header changes from the
`.data` default 192x192 to 84x84**), loads the tile-set mapping
(`g_default_tile` 1 -> 32), the terrain element, and `g_perim_count` = 2 — and
then dies in the typed-callback class (§4).

**And the render path is proved good, positively**: writing `g_default_tile`
into every cell's `tile` field (`Cell +0x08`) from the page makes the isometric
terrain grid appear across the whole 640x340 map area on the next frame, with
no shim change at all (§3).

---

## 2. Every one of the brief's five candidates, ruled out with a measurement

The brief asked for (a)..(e) in order. All five are fine; the numbers come from
`window.llRender()`, this lane's window into the globals `renderview.c:150`'s
"HOST / BROWSER-RENDERER CONTRACT" block lists.

| # | candidate | measured | verdict |
| --- | --- | --- | --- |
| a | surface-to-surface `Blt` with a source rect, `DDBLT_KEYSRC`, `BltFast`; `Lock` on an off-screen surface | `ddraw.c:284-369` implements colour fill, straight copy, keyed copy and a nearest-neighbour stretch; `ddraw.c:446-475` `Lock` returns `row_of(s, top) + left*2` and `s->pitch` for the surface it is given | **fine** |
| b | `g_present`/`FlipPrimary` vs a client-rect blit in WINDEBUG | `g_present` non-zero; the steady frame is `PeekMessageA, GetDeviceState x2, Lock, Unlock, timeGetTime x21657, Blt, timeGetTime, Lock, Unlock, Blt, Lock, Unlock` and the HUD updates on every one of them | **fine** — the present path works, it is what puts the money bar on the canvas |
| c | `g_ddsd_bits`/pitch describing the LAST `Lock` | `ddsdBits` 11722288, `ddsdPitch` **1280** = 640 x 2 | **fine** |
| d | the software renderer drawing into a surface nobody presents | the toolbar, money bar and rating bar are drawn by `PrintSprite` into the SAME locked surface, in the same frame, and they reach the canvas | **fine** |
| e | `g_render_clip` / scroll units leaving an empty clip | `renderClip` `[0, 0, 639, 479]`, `g_clip_rect` `[0, 0, 640, 480]`, view `{origin (0,32), 640x340}`, `tileH` 16 / `tileW` 32 | **fine** |

And the two that settled it, which were not on the list:

* **`g_ground_last_x/y` tracks the live scroll.** `sysmisc3.c`'s
  `RenderGroundLayer` stores them *after* `PaintTileLayer` returns, so the
  terrain pass is not being skipped — it runs every frame, over an empty map.
  Arrow-key scrolling moves `g_scroll_x/y` and `groundLast` follows it exactly.
* **The map is 36,864 zero cells.** Row pointers are allocated and correct
  (192 rows, stride 5120), and a word-by-word scan of all 737,280 bytes finds
  **zero** non-zero words.

The reason `?beat=` could not see any of this is worth stating once: the map
renderer draws in SOFTWARE between one `Lock` and one `Unlock`. A frame that
paints ten thousand tiles and a frame that paints none make **exactly the same
host calls**. A7's "the host ring goes byte-identical-quiet" was true and was
never evidence about drawing.

---

## 3. The render path, proved by making it draw

On an ordinary unpatched build, in the park, from the page:

```js
// Cell +0x08 is `tile`; render4.c:430 paints g_tile_sprites[t] when t != 0
var a = llAddrs(), I = HEAP32, U = HEAPU8, f = llRender();
var rows = I[a.g_map_rows >> 2], W = f.mapHdr.width, H = f.mapHdr.height;
for (var y = 0; y < H; y++) {
  var r0 = I[(rows + y * 4) >> 2];
  for (var x = 0; x < W; x++) { var c = r0 + x * 20; U[c+8] = f.defaultTile; U[c+9] = 0; }
}
```

36,864 cells written; the next frame the **whole 640x340 map area fills with the
2:1 isometric diamond grid** (frame hash `0x561128cd` -> `0xc59c681c`, non-black
98.1% -> 99.1%, still 33.7 fps, no trap). Screenshot: the green tile grid over
the full map rectangle, the money bar and toolbar untouched above and below it.

The grid shows the previous backdrop through it because the default tile is
partly transparent and nothing clears the map area — in a real level the terrain
tiles are opaque. The loop dies a few seconds later with a `memory access out of
bounds`, which is expected and honest: a hand-filled cell array is not a
consistent map (no owners, no flags, no object chain), and the object pass
follows it. **The probe proves one thing and only one thing — the software
renderer, the locked surface, the `Blt` and the canvas all work — and that is
the thing A7-1 asked about.**

---

## 4. What is actually between here and a rendered park

With the keyword table repaired live, the park load runs and stops, twice, in
the **typed-callback class** (A7-2's class, PORT-M's). Both were named with
`legoland_browser_named` + `name_trap.py`.

### B9-2: `LoadObjectLibrary` always fails, so every OC_USEDLL class calls a cast forwarder

```
RuntimeError: function signature mismatch
  LLIDB_LoadData      wasm-function[783]:0x7b871   call_indirect (i32) -> void
  LevelKw_MAP         wasm-function[685]:0x71fe3
  LoadLevelDatabase   wasm-function[1029]:0x9cc75
  StartPark           wasm-function[568]:0x446c8
```

`llidb_odf.c:294` is `obj->fa4(obj->elem)`, declared `void (*fa4)(LLElem*)` at
`llidb_odf.c:77`. It sits on the arm `llidb_odf.c:290` takes when
`LoadObjectLibrary(obj, dllname) == 0`, and that arm is taken **always**:

```
HOST LoadLibraryExA(".\dlls\ENTRANCE.dll"): refused
HOST LoadLibraryExA(".\dlls\SHOPS1.dll"): refused
```

and **no `.dll` ships** — not in `gamedata/disc`, not in `gamedata/main`, not in
any of the three `.res` volumes. So the shipped game takes this path too; on x86
it is harmless. On wasm the slot holds

```c
/* gen-browser/aliases.c:277 */
extern void LegoShop1_LoadResources(unsigned int);
void LegoShop1_Create(void) { ((void (*)(void))&LegoShop1_LoadResources)(); }
```

a **cast forwarder of type `() -> void`** installed by `interfaces.c:894`
(`def->cb_create = LegoShop1_Create;`) because `interfaces.c:809` declares
`extern void LegoShop1_Create(void);` for `0x00439200`, whose definition is
`void LegoShop1_LoadResources(ShopElem* elem)` at `westtown.c:593`. Calling it
as `(i32) -> void` traps.

`gen-browser/manifest.md`'s "Cast forwarders: a stale name typed unlike its
body" already lists 72 of these and says "Fix the DECLARING file, do not bridge
it here". **26 of the 72 are declared `() -> void` with a `(i32) -> void` body**
— the `*_Create` / `*_Activate` family that lands in `+0xa4`/`+0xb0`, i.e.
exactly the ones this call site reaches. This is PORT-M6's "the rest need
declaration+slot moved together — PORT-M7".

Note the forwarder is not merely mistyped, it is **wrong**: it calls the body
with no argument, so even a type-correct trampoline would hand
`LegoShop1_LoadResources` a null `elem`. The fix has to be the declaration.

### B9-3: the next one, immediately behind it

With those 26 table slots neutralised (a probe, not a fix), the load reaches

```
RuntimeError: table index is out of bounds
  PutObjOnMap   wasm-function[967]:0x90fa8   call_indirect (i32, i32) -> void
  LevelKw_MAP   wasm-function[685]:0x721a6
```

`PutObjOnMap` calling a class's `cb_add(obj, pos)`. *Out of bounds*, not a
signature mismatch, means the slot held a value past the end of a 1075-entry
table — a raw x86 address, the §1 class again rather than the §4 class. Not
chased further; it is behind B9-2 and belongs to the same two owners.

### A7-2, localised: it is the MAP button, and it is `RenderFullMap`

A7 reported "a toolbar click" traps. It is one specific button. Clicking all six
in the in-game screen, on an ordinary build:

| toolbar control | game px | result |
| --- | --- | --- |
| LEGOLAND | (53, 396) | frame changes, no trap |
| build | (38, 446) | frame changes, no trap |
| query `?` | (117, 446) | frame changes, no trap |
| eraser | (198, 446) | frame changes, no trap |
| **map** | **(278, 446)** | **`RuntimeError: function signature mismatch`** |
| sliders | (358, 446) | frame changes, no trap |

```
  MakeSprite           wasm-function[1635]:0xf5437   call_indirect (i32) -> void
  MakeSpriteDrawable   wasm-function[1389]:0xd6b96
  PrintSprite          wasm-function[1143]:0xada14
  GameFrame            wasm-function[570]:0x618f1
```

and the source is unambiguous:

* `sprite2.c:199` — `SpriteRec* CreateFunctionBasedSprite(SpriteDrawFn fn, short w, short h)`,
  where `SpriteDrawFn` is `void (*)(SpriteRec*)`.
* `sprite2.c:253` — `((SpriteDrawFn)s->image)(s);` — the `(i32) -> void` call site.
* `mapscreen.c:103` — declares the SAME function as
  `CreateFunctionBasedSprite(void (*fn)(void), short w, short h)`.
* `mapscreen.c:105` — `extern void RenderFullMap(void);`
* `mapscreen.c:192` — `g_ms_sprite = CreateFunctionBasedSprite(RenderFullMap, 640, 340);`
* `renderview.c:2935` — `void RenderFullMap(void)`, genuinely `() -> void`.

So the map screen stores a `() -> void` body in a slot the sprite layer calls as
`(i32) -> void`. On x86 cdecl the caller pushes an argument the callee never
reads: harmless, and the shipped game does exactly this. On wasm it is fatal.
`bubblecache.c:150` declares the same function correctly
(`void (*fn)(SpriteRec*)`) and its `DrawCachedTextSprite` matches — which is why
the front end's cached text sprites work and only the map screen dies.

**Recipe for PORT-M**: under `#ifdef LEGOLAND_PORTABLE`, give `RenderFullMap`
the slot's signature — `void RenderFullMap(SpriteRec* unused)` at
`renderview.c:2935` with the parameter ignored — and bring `mapscreen.c:103`'s
and `:105`'s declarations to match. A cdecl callee that never reads an incoming
stack argument emits the same bytes either way, so `audit.py` and `relocs.py`
should both stay clean; that is the gate to run, not an assumption.

---

## 5. A7-3: the one-byte reads, measured and dismissed

It is neither the msvcrt `_read` pattern nor anything in ddraw/gdi. It is the
game's own line reader:

* `levelkw.c:865` — `ReadLine` does `RES_ReadFile(f, &c, 1)` per character.
* `res.c:39` — `RES_ReadFile` is **completely unbuffered**: it forwards straight
  to `ReadFile(v->handle, buf, count, &got, 0)` with `count` = 1.

So every byte of every level script is one host `ReadFile`. Measured over one
park load (`?trace=1&tracegrep=ReadFile&tracekeep=400000`):

| | |
| --- | --- |
| host `ReadFile` calls during the load | 1194 |
| of those, **1 byte** | **1153 (96.6%)**, all on volume handle 2 |
| the other 41 calls | 209,334 bytes on handle 4 |
| so: share of CALLS that are one byte | 96.6% |
| share of BYTES they move | 0.55% |

A7's "298 of 300" was the same ratio seen through a 300-line window.

**The cost.** `kernel32.c`'s `ReadFile` bottoms out on MEMFS. Benchmarked in the
page against the real `Legoland.res` (16.4 MB, preloaded):

```
20,000 one-byte FS.read calls: 1.3 ms  ->  0.07 us per read
```

1153 reads therefore cost about **0.08 ms of an 822 ms park load — 0.01%**. Even
once the keyword table is fixed and the parse reads all 4,648 bytes of
`ObjList1.txt` plus the 19,341-byte `Template.txt` and a 12 KB `ObjList`, the
whole script layer is under 3 ms.

**Recommendation to PORT-A: do nothing.** A read-ahead cache in `ReadFile` would
have to be invalidated on every `SetFilePointer` (which `RES_ReadFile` issues
whenever the volume's current member changes) to buy a hundredth of a percent.
A7 called it "a performance note, not a defect"; it is not even that. Recorded
here so the next lane does not re-measure it.

---

## 6. B5 — profiles across a reload: CLOSED

PORT-A7 §8's patch, applied exactly as written, because it is the same two files
this lane already owns.

1. `portable/cmake/browser.cmake` — `-lidbfs.js` on `legoland_browser` and
   `legoland_browser_named`. No `-sASYNCIFY` change; both already have it.
2. `portable/src/browser/main.c` — `ll_mount_profiles` / `ll_flush_profiles`
   (`EM_JS`), the pre-`WinMain` spin on `g_syncfs_pending` through
   `emscripten_sleep`, and `emscripten_set_interval(ll_flush_profiles_cb, 5000)`.
   The bare `mkdir` is **removed** rather than kept as a fallback: both arms of
   `ll_mount_profiles` (the mount and the caught-exception path) already make
   the directory, and a second `mkdir` over a mounted filesystem only produces a
   confusing `EEXIST` line.
3. `#include <emscripten/eventloop.h>` — where `emscripten_set_interval` lives in
   emsdk 6.0.9. (A7's patch did not mention it; without it the build fails.)

**Verified**, A7 §8 step 4's recipe:

```
FS.readdir('/gamedata/profiles')     ['.', '..', 'Profile1.txt']
FS.stat(...).size                    272
```

after a **page reload**, and PLAYER DETAILS draws **`adam` in slot 1** without
the walk (front-end hash `0x9d9b7c50` on an empty store -> `0xddbc673c` with the
profile restored). The link cost A7 was worried about is not real on this
machine: `legoland_browser` relinks in well under a minute.

---

## 7. What runs in the park

Everything except the map. All on an ordinary build, in-game screen, no patches.

| | |
| --- | --- |
| park fps | **33.4** measured over **77 s** (2580 presented frames), 0 traps |
| the sim clock | `g_sim_frame` advances **2579 in 2580 frames** — one sim tick per presented frame, exactly |
| frame hash over 77 s | **unchanged** — nothing animates, because there is nothing in the park |
| visitors | none: no level, no blokes |
| arrow-key scroll | **works, all four**, isometric 2:1 |
| a click in the map area | frame changes, no trap |
| toolbar | 5 of 6 buttons work; the map button traps (§4) |

Scroll, from `g_scroll_x/g_scroll_y` (24.8 fixed point, shown `>> 8`):

| key | from | to | delta |
| --- | --- | --- | --- |
| Right | (-320, 160) | (-96, 272) | +224, +112 |
| Down | (-96, 272) | (-96, 496) | 0, +224 |
| Left | (-96, 496) | (-544, 496) | -448, 0 |
| Up | (-544, 496) | (-544, 272) | 0, -224 |

x moves twice y's step on the diagonals, which is the 2:1 diamond, and
`g_ground_last_x/y` follows each one — the terrain pass really is re-running for
the new scroll position every frame. The frame hash does not change because the
map is empty.

---

## 8. Blockers, with owners

| # | what | owner | state |
| --- | --- | --- | --- |
| **A7-1** | the park's map area does not render | PORT-B9 | **DIAGNOSED, not a shim defect.** The shim, `RenderView`, `RenderGroundLayer` and `PaintTileLayer` are all correct (§2, proved positively in §3). The map is 36,864 zero cells. Root cause is **B9-1** |
| **B9-1** | `movie.c:239` declares the 93-pair level keyword table as one `const void*`, so 92 of its 93 keyword-string words keep raw x86 VAs; **no level keyword ever dispatches**, `MAP` never runs, `LoadBaseMap` never runs, the map is never filled | **PORT-A** (`gen_link.py` / `cdecl.py`), or a matching lane via the declaration | **new, and it is the park's root blocker.** Repaired live (§1) and the loader immediately runs. Two ways: declare the table properly in `movie.c` (a `{const char*, KeywordFn}` array of 93 — a matching-side change), or teach the generator that a *declared extent smaller than the emitted block* is a declaration to distrust rather than a licence to leave the tail raw. A7 §5's residue rule classifies all 188 tail words as "past the declared extent, therefore not described, therefore not a pointer" — right premise, wrong conclusion for a pointer table |
| **B9-1a** | the same shape elsewhere: A7 §5's residue list is `g_level_db_sections 91`, `g_event_tick`, `g_report_setters`, `g_lt_action_handlers`, `g_track_desc_*` — `name_trap.py --at` reports them as "by callback table" | PORT-A | **new.** `g_level_db_sections` is proved live; the others are the same census row and should be checked the same way |
| **B9-2** | `llidb_odf.c:294` `obj->fa4(obj->elem)` calls a `() -> void` cast forwarder. Reached on EVERY OC_USEDLL class because no `.dll` ships and `LoadLibraryExA` is refused. 26 of the manifest's 72 cast forwarders are this exact shape | **PORT-M7** | **new**, §4. `manifest.md`'s "Cast forwarders" table is the list; `interfaces.c:809` / `westtown.c:593` is the worked example |
| **B9-3** | `PutObjOnMap`'s `cb_add(obj, pos)` gets a table index past the end of the table | PORT-A (B9-1's class) / PORT-M | **new**, §4, behind B9-2 |
| **A7-2** | a toolbar click is a `call_indirect` type mismatch | PORT-M | **LOCALISED**: it is the **MAP** button, and it is `RenderFullMap` declared `(void)` (`mapscreen.c:103`/`:105`, `renderview.c:2935`) in a `void (*)(SpriteRec*)` slot (`sprite2.c:199`/`:253`). The other five toolbar buttons work. Recipe in §4 |
| **A7-3** | the loader's one-byte host reads | PORT-B9 | **CLOSED as "do nothing"**, §5. Game-side (`levelkw.c:865` through the unbuffered `res.c:39`), 96.6% of calls, 0.55% of bytes, **0.08 ms of an 822 ms load** |
| **B5** | profiles do not survive a page reload | PORT-B9 | **CLOSED**, §6 |
| B3 | `while (KillSprite(x) == 0) ;` unguarded in four places | a matching lane | unchanged, not reached this lane |
| B4 | the main menu's Free-play bubble does not take a click | unknown | unchanged, not retested |

---

## 9. What this lane added to the page, and how to replay

`portable/src/browser/main.c`'s `LL_DBG_TABLE` goes from 7 entries to 60. One
macro table generates both `ll_dbg_addr(i)` and `ll_dbg_name(i)`, so an index, a
name and an address cannot drift apart, and the page builds its labels from the
module rather than from a hand-kept copy.

| hook | what it answers |
| --- | --- |
| `llAddrs()` | `{name: linear address}` for all 60 |
| `llRender()` | the whole render + level-loader state, decoded: map header, rows, scroll, tile geometry, clip rects, `g_ddsd_*`, the surfaces, `g_ground_last_*`, and how far `LoadBaseMap` got |
| `llRes(bucket?)` | the RES master directory walked — buckets and members. `res.c`'s `RES_OpenFile` splits at the last backslash and looks the bucket up, so a name not in this listing is a name the game cannot open |
| `llKeywordTable()` | the 93 keyword pairs, with what each keyword pointer actually reads as |
| `llFixKeywordTable()` | translates the raw VAs and writes them back. **A diagnostic, not a fix** |
| `?tracekeep=N`, `?tracegrep=RE` | resize / filter the host-call ring. 300 lines cannot reach back past 1153 one-byte reads to the call that opened the file |

Replay, exactly (server `cd portable/build-wasm && python3 -m http.server 8803`):

```js
// http://localhost:8803/legoland.html?args=-nointro+WINDEBUG
llMove(320,240); llClick(260,188); llType('adam'); llClick(505,345);  // main menu 0x954b0578
llClick(252,362);                                                     // tutorial   0x23934744
llClick(577,419);                                                     // in-game    0x561128cd, 822 ms

llRender()            // mapHdr 192x192 (the .data default), mapLoaded 0, mapElem 0
llKeywordTable()      // rawKeywordPointers: 91, every one reading ""
llFixKeywordTable()   // {fixed: 91, unmapped: []}  -> then reload and walk in again
```

With a profile already in IDBFS the first screen is `0xddbc673c` and slot 1 is
`adam`, so the popup step is skipped — that is B5 working, not a regression.

| screen | hash |
| --- | --- |
| PLAYER DETAILS, empty profile store | `0x9d9b7c50` |
| PLAYER DETAILS, `adam` restored from IDBFS | `0xddbc673c` |
| NEW PROFILE popup | `0x6b1a9df1` |
| main menu | `0x954b0578` |
| tutorial select | `0x23934744` |
| **in-game screen** | **`0x561128cd`** (identical to A7 row 5) |
| in-game, after the synthetic-tile probe (§3) | `0xc59c681c`, non-black 99.1% |
| in-game, after a map-area click | `0x85185824` |
| in-game, after LEGOLAND / build / query / eraser | `0x7718633c` / `0x29cd7eb8` / `0x90ab31a4` / `0x45e7d250` |

---

## 10. Gates

| gate | result |
| --- | --- |
| wasm clean build, all seven targets | builds; `legoland.wasm` 1,338,787 bytes, `legoland_dbg.wasm` 1,391,862 |
| wasm `ctest` | **17/17** |
| native clean build | builds |
| native `ctest` | **11/11** |

One note for whoever runs these from a CLEAN native directory: plain
`ninja -C portable/build` does **not** build `legoland_tests`,
`legoland_linkcheck` or `legoland_gen`, so `ctest` there reports 10 "Not Run"
plus `pointer_words` FAIL ("no manifest.md ... build the closure before running
this test"). That is the harness, not a regression —
`ninja -C portable/build legoland_tests legoland_linkcheck legoland_gen` first
and it is 11/11. The wasm side has no such trap because its extra targets are
named explicitly in the brief's build line.
| `LEGOLAND/*.c` touched | **none** — no audit/relocs run needed |
| generated host traps | 0, unchanged |
| page: front end and in-game screen, traps | **0**, 33-34 fps |

Scratch scripts (not committed, `port-b9-` prefixed): `port-b9-build-wasm.sh`,
`port-b9-names.py` / `port-b9-names2.py` (do these globals exist in the
closure), `port-b9-kwtable.py` / `port-b9-kw2.py` (which emitted object owns
each raw keyword VA), `port-b9-forwarders.py` / `port-b9-fwd2.py` (cast
forwarders to table slots, via `name_trap.Module`).
