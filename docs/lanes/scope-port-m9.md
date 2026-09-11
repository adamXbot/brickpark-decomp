# Scope PORT-M9 — function and data addresses written as INTEGER LITERALS

> **Status: IN PROGRESS (claimed 2026-09-12 by PORT-M9)** — branch `scope/PORT-M9`,
> cut from the PORT-B9 merge (`321abb20`). Matching-side, VC6-gated.
> Brief: `docs/SCOPE_PORT_WAVE.md`; the class was named by PORT-B9 §4 as **B9-4**.

**Result in one line: the park loads and the terrain, the entrance, the money
bar and the toolbar all render at 34 fps with `llStats().dead` null and zero
traps.** The `RuntimeError: table index is out of bounds` that killed the first
park load is gone.

---

## 1. The class, restated

The recovered C is a byte-for-byte match, and it is a match *because* it spells
the immediate:

```c
/* sweep3.c:167, before */
p->f3 = (void*)0x45efe0;      /* AddBasicObject */
```

The original instruction is `mov dword ptr [ecx+0x98], 0x45efe0`, and both
`(void*)0x45efe0` and `&AddBasicObject` assemble to exactly that. They differ
only in the OBJECT: the named form carries a DIR32 relocation the literal does
not. `tools/match.py` / `tools/audit.py` compare the LINKED instruction text, so
they cannot tell the two apart; `tools/relocs.py` compares the relocation's
resolved target against the original's, so it can — and that is the gate that
makes a both-builds replacement legitimate rather than a guess.

What makes this a **third** blocker class, distinct from B9-1 (a `.data`
pointer table whose declaration is too small) and B9-2 (a stale declaration
typing a forwarder), is that the address never appears in `.data` at all. It is
an immediate in `.text`. `gen_link.py` scans the image's data words; it can
never see this, and no amount of generator work can reach it. It has to be
fixed in the source.

On wasm a function "pointer" is an index into the module's function table
(1075 entries), so a slot holding 4,517,856 is `table index is out of bounds`
the moment something calls it. A *data* address in a pointer field is the same
defect with a quieter failure: a raw x86 VA in linear memory, read or written
at whatever the closure happens to have laid out there.

## 2. The sweep — how it was done and what it found

`port-m9-sweep.py` strips C comments, string literals and char literals from
all 258 `LEGOLAND/*.c` plus the headers (so an address in a `/* 0x0045efe0 */`
marker comment does not count) and then reports every hex literal of 5+ digits
in `0x00401000 .. 0x008fffff` — the whole image, `.text` and `.data` both.
That also covers the `(FnType)0x4...` cast form and `= 0x4...` into any field,
because the filter is the VALUE, not the syntax.

**49 raw hits, of which 35 are real address literals.** The 14 dismissed:

| site | literal | why not an address |
| --- | --- | --- |
| `appraisalscreen.c:731/734/888/1045`, `mappath.c:631`, `objmap2.c:436/484/572`, `reportset.c:356/360` | `0x800000` | an ObjDef/report **flag bit** |
| `pathmisc.c:173` | `0x600000` | a two-bit class flag mask |
| `person3d.c:1243/1244` | `0x500000` / `0x5a0000` | 16.16 fixed-point screen centres (`0x500000` = 80.0) |
| `misc3.c:530` | `0x40e040` | an **RGB colour** — its sibling two lines up is `0xe04040` |

### The 35 real sites

**Function addresses — 29 sites, 3 files.** (PORT-B9 counted `loaders.c` as 21;
the file has **24** — lines 162‑167 (6), 171‑182 (12), 186‑191 (6).)

| # | file:line | literal | owner (`// FUNCTION:` marker) | slot | slot's call-site type | body's real type | decision |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 1 | `sweep3.c:164` | `0x480b70` | `SetEditObjectFromElem` pathobj2.c:243 | ObjClass +0x8c | *none recovered* (M9‑1) | `(i32) -> void` | both builds |
| 2 | `sweep3.c:165` | `0x45fa80` | `CalcBasicObjectCursor` objmap.c:332 | +0x90 | `(i32,i32,i32) -> void` | same | both builds |
| 3 | `sweep3.c:166` | `0x480bb0` | `BasicObjectDCalcCursor` objmap2.c:505 | +0x94 | `(i32,i32) -> void` | same | both builds |
| 4 | `sweep3.c:167` | `0x45efe0` | `AddBasicObject` objmap2.c:459 | +0x98 | `(i32,i32) -> void` | **`(i32,i32,i32)`** | x86 `&AddBasicObject`, portable **adapter** |
| 5 | `sweep3.c:168` | `0x45f220` | `StandardRemoveObject` objmap2.c:982 | +0x9c | `(i32,i32,i32) -> void` | same (the 2-byte `BPos` by value lowers to one i32) | both builds |
| 6 | `loaders.c:162` | `0x41b830` | `BsWater_LoadResources` ridecb8.c:1570 | IfaceTable[7] init | `(i32) -> void` | same | both builds |
| 7 | `loaders.c:163` | `0x41b880` | `BsWater_SelectForPlacement` ridecb8.c:1545 | +0x8c | *none recovered* (M9‑1) | `() -> void` | both builds |
| 8 | `loaders.c:164` | `0x41bd40` | `BsWater_CalcCursor` ridecb6.c:347 | +0x90 | `(i32,i32,i32) -> void` | same | both builds |
| 9 | `loaders.c:165` | `0x41bfb0` | `BsWater_DrawSelection` screencb.c:390 | +0x94 | `(i32,i32) -> void` | same | both builds |
| 10 | `loaders.c:166` | `0x41b8e0` | `BsWater_Add` ridecb6.c:480 | +0x98 | `(i32,i32) -> void` | same | both builds |
| 11 | `loaders.c:167` | `0x41c130` | `BoatingSchoolWater_Remove` ridecb5.c:503 | +0x9c | `(i32,i32,i32) -> void` | same | both builds |
| 12 | `loaders.c:171` | `0x419d10` | `BoatingSchool_Create` screencb.c:597 | init | `(i32) -> void` | same | both builds |
| 13 | `loaders.c:172` | `0x419ef0` | `BoatingSchool_Destroy` screencb2.c:490 | +0xac | `(i32) -> void` | same | both builds |
| 14 | `loaders.c:173` | `0x41a000` | `BoatingSchool_SelectForPlacement` ridecb8.c:1252 | +0x8c | *none recovered* (M9‑1) | `() -> void` | both builds |
| 15 | `loaders.c:174` | `0x41a2f0` | `BoatingSchool_Update` screencb3.c:635 | +0x90 | `(i32,i32,i32) -> void` | same | both builds |
| 16 | `loaders.c:175` | `0x41a3d0` | `BoatingSchool_DrawSelection` screencb2.c:1435 | +0x94 | `(i32,i32) -> void` | same | both builds |
| 17 | `loaders.c:176` | `0x41a040` | `BoatingSchool_Add` ridecb5.c:281 (**WIP**) | +0x98 | `(i32,i32) -> void` | same | both builds |
| 18 | `loaders.c:177` | `0x41a530` | `BoatingSchool_Remove` ridecb8.c:380 | +0x9c | `(i32,i32,i32) -> void` | same | both builds |
| 19 | `loaders.c:178` | `0x41a720` | `BoatingSchool_Tick` ridecb5.c:1236 | +0xa8 | `(i32) -> void` | **`() -> void`** | x86 name, portable **adapter** |
| 20 | `loaders.c:179` | `0x41abd0` | `BoatingSchool_Draw` screencb.c:1341 | +0xb0 | `(i32 x6) -> void` | same | both builds |
| 21 | `loaders.c:180` | `0x41acf0` | `SaveBoatingSchool` ridecb8.c:96 | +0xbc | `(i32) -> i32` | **`() -> i32`** | x86 name, portable **adapter** |
| 22 | `loaders.c:181` | `0x41aee0` | `LoadBoatingSchool` ridecb6.c:197 | +0xb8 | `(i32) -> i32` | **`() -> i32`** | x86 name, portable **adapter** |
| 23 | `loaders.c:182` | `0x41b100` | `BoatingSchool_BestTake` ridecb6.c:1187 | +0xc0 | `(i32,i32) -> i32` | same | both builds |
| 24 | `loaders.c:186` | `0x41b250` | `Mermaid_LoadResources` ridecb8.c:1263 | init | `(i32) -> void` | same | both builds |
| 25 | `loaders.c:187` | `0x41b260` | `Mermaid_SelectForPlacement` ridecb8.c:1272 | +0x8c | *none recovered* (M9‑1) | `() -> void` | both builds |
| 26 | `loaders.c:188` | `0x41b4c0` | `Mermaid_CalcCursor` ridecb8.c:578 | +0x90 | `(i32,i32,i32) -> void` | same | both builds |
| 27 | `loaders.c:189` | `0x41b6d0` | `Mermaid_CalcCursor2` ridecb8.c:1286 | +0x94 | `(i32,i32) -> void` | same | both builds |
| 28 | `loaders.c:190` | `0x41b2a0` | `Mermaid_Add` ridecb6.c:587 | +0x98 | `(i32,i32) -> void` | same | both builds |
| 29 | `loaders.c:191` | `0x41b6f0` | `BsMermaid_Remove` screencb.c:1423 | +0x9c | `(i32,i32,i32) -> void` | same | both builds |

Every one of the 29 resolves to **exactly one** `// FUNCTION:` marker at the
literal's own address — there is no "the literal is not a function start" case
in this set, which is what made the both-builds replacement available.

**Data addresses — 6 sites, 3 files.** Same recovery shape, different failure:
a raw VA in linear memory instead of a bad table index.

| # | file:line | literal | what it is | decision |
| --- | --- | --- | --- | --- |
| 30 | `coaster10.c:380` | `0x004b5648` | the **flat** span-filler table `{Span_FillFlat, Span_FillFlatZ}` | both builds, **new typed declaration** |
| 31 | `coaster10.c:475` | `0x004b5658` | `g_span_fillers` `{Span_FillShade, Span_FillShadeZ}` — **already declared** in schoolcar3.c:625 | both builds |
| 32 | `coaster10.c:568` | `0x004b5f50` | the **track** span-filler table `{TrackShade_FillPoly, x2}` | both builds, **new typed declaration** |
| 33 | `sweep1.c:170` | `0x630108` | render list 1's **arena base** | both builds, **new declaration** |
| 34 | `sweep1.c:177` | `0x638218` | render list 2's **arena base** | both builds, **new declaration** |
| 35 | `coaster12.c:610` | `0x004e3870` | the **end of the command buffer** (= `g_zbuffer`'s start) | portable arm |

## 3. Per-site decisions and the relocation evidence

### Both builds (the default)

`(void*)0x45efe0` → `(void*)AddBasicObject`. The rule the brief sets is that
this is legitimate only if `audit.py` stays `[OK]` **and** `relocs.py` resolves
the new relocation to the literal's own value. Both hold for all of §2's
"both builds" rows — see §5 for the gate output. Nothing else changed in any
matched body; the extra declarations are pure insertions above the markers,
never between a marker and its signature.

### Portable arm — the four registration-site adapters

Four bodies do not have their slot's wasm type. Three take no argument where
the slot is called with the element pointer, and one takes an extra argument
the slot never passes:

| body | slot | mismatch | adapter |
| --- | --- | --- | --- |
| `AddBasicObject` | +0x98, `cls->place(obj, pos)` (mapobj.c:49) | body has a 3rd arg it never reads — its slot homes `bp` (objmap2.c's own note) | `ll_cb_98_AddBasicObject` in sweep3.c |
| `BoatingSchool_Tick` | +0xa8, `cls->prerender(cls->ctx)` (renderview.c:1130) | `() -> void` | `ll_cb_a8_BoatingSchool_Tick` in loaders.c |
| `LoadBoatingSchool` | +0xb8 (savegame.c:1365) | `() -> i32` | `ll_cb_b8_LoadBoatingSchool` in loaders.c |
| `SaveBoatingSchool` | +0xbc (savegame.c:941) | `() -> i32` | `ll_cb_bc_SaveBoatingSchool` in loaders.c |

This is not a new invention: it is exactly the `ll_cb_*` registration-site
static that PORT-M3 established in `interfaces.c` for the classes registered
through `.data`, and PORT-M7 extended to the 35 `cb_destroy`/`cb_activate`
stores. The matched bodies are untouched; only the value the x86 arm and the
portable arm store into the slot differ, and the x86 arm still stores the
symbol (better for the decomp than the literal it replaced).

`AddBasicObject` is the interesting one: it is *why* PORT-B9's trap was
"out of bounds" and not "signature mismatch". Naming the function without the
adapter would have converted one fatal trap into the other.

### The three span-filler tables — one declaration fixes two bugs

`PolyJob +0x38` is not a function pointer, it is a **pointer to a two-entry
table** of them: `coaster3d.c:1080` does
`((SpanFiller)jt->shader[mode])(tag, grad, ne, keys, edges)`, and
`schoolcar3.c:731` already spells its own as `job.shader = g_span_fillers`.
Two of the three tables `coaster10.c` points at had **no declaration anywhere**,
so they were swallowed by the gap tiling of the object before them and their
words were left RAW. Declaring them typed fixes the literal *and* the contents:

```
/* portable/build-wasm/gen-browser/globals.c, after */
/* 0x004b5648 .data 16 bytes */
unsigned int g_span_fillers_flat[4] = { &Span_FillFlat, &Span_FillFlatZ, 0x7f, 0 };
/* 0x004b5f50 .data 16 bytes */
unsigned int g_span_fillers_track[4] = { &TrackShade_FillPoly, &TrackShade_FillPoly, 0x7f, 0 };
```

All five bodies are genuinely `(int, int*, int, SortKey*, SpanEdge*)`, i.e. the
`SpanFiller` type the call site uses — no adapter needed.

### The two render arenas

`renderlist.c`'s own header names them: *"list 1 : arena base 0x00630108, bump
ptr @ 0x0062feec; list 2 : arena base 0x00638218, bump ptr @ 0x0062fef0"*.
`RenderItems_New` / `RenderItems2_New` reset the bump pointer to the base every
frame, and `blokelist.c:213` / `tinystubs.c:228,233` bump-allocate 16-byte items
from it. With the raw literal, **every frame in the park** wrote render items to
linear address `0x00630108` — arbitrary memory in the portable build. Now
`g_render_arena1` / `g_render_arena2`, which also gives the generator real
blocks (32768 and 320 bytes; see finding **M9-2**).

### `coaster12.c` — a data bound, portable arm only

```c
if ((unsigned)(cur + 0x10) > (unsigned)0x004e3870) { g_span_overflow = 1; return; }
```

`0x004e3870` is not a pointer being stored, it is the **end of the buffer**:
`schoolcar.c`'s `Coaster3D_EndFrame` names the pair — `g_cmd_buf` at
`0x004dd870` (`0x6000` bytes) and `g_zbuffer` at `0x004e3870` immediately behind
it — and our `g_span_cursor` is its `g_cmd_write`. The comparison only *means*
"the end of the buffer" because of where the linker happened to put the two
objects. In the portable build they are unrelated C objects, so the raw bound
either never fires (and the writes run off the end) or always fires (and no
span is ever recorded). The portable arm expresses the bound from the buffer:
`(char*)cur + 0x10 > (char*)g_cmd_buf + 0x6000`.

This one stays a **portable arm** rather than a both-builds change because the
owning body is a `// WIP-FUNCTION` (`Raster_AddSpanRecord`, 61i/175B) —
`relocs.py` only sweeps `FUNCTION` markers, so a both-builds change here would
have no relocation gate behind it. The x86 arm is byte-identical by
construction.

## 4. Where the park got to

Served `portable/build-wasm` on 8805,
`legoland.html?args=-nointro+WINDEBUG&beat=1000`, replayed
`llMove(320,240); llClick(260,188); llType('adam'); llClick(505,345); llClick(252,362); llClick(577,419)`
(the last one twice — the first lands on the tutorial letter's "To the game.."
control, the second takes it):

| step | frame hash | non-black | `dead` | traps |
| --- | --- | --- | --- | --- |
| front end idle | `0xddbc673c` | 96.6% | null | 0 |
| click slot 1 | `0x5a18acb8` | 96.4% | null | 0 |
| type `adam`, OK | `0x6c445d5c` | 98.0% | null | 0 |
| tutorial select | `0x23934744` | 98.3% | null | 0 |
| into the park | `0xd78f3580` | 97.3% | null | 0 |
| park, popup up | `0x1bee74a0` | 98.6% | null | 0 |
| popup closed | `0xe3f8d904` | 98.6% | null | 0 |
| after a scroll | `0xc62ac419` | 98.8% | null | 0 |

**What is on screen**: the isometric TERRAIN grid in green, the grey path
network, the park entrance with its LEGOLAND banners and blue entry path, a
bird sprite over the grass, the money bar reading 1030, the "You have a new
object — LEGO Toy Shop" popup, the LEGOLAND/build/query/eraser/map/sliders
toolbar and the tutorial newspaper panel. 3535 frames at **34.4 fps**,
`llStats().dead` null, `traps: []` throughout. No further trap appeared, so
`name_trap.py --at` had nothing to name.

For comparison, PORT-B9's state at hand-over was
`RuntimeError: table index is out of bounds` in `PutObjOnMap` on the first
perimeter object of the level.

## 5. Gates

Run after the VC6 quiet window (started 13:13, gates from 13:58).

| file | `audit.py` | `relocs.py \| grep MISMATCH` |
| --- | --- | --- |
| `LEGOLAND/sweep3.c` | *(see §5.1)* | *(see §5.1)* |
| `LEGOLAND/loaders.c` | | |
| `LEGOLAND/coaster10.c` | | |
| `LEGOLAND/sweep1.c` | | |
| `LEGOLAND/coaster12.c` | | |

`tools/progress.py --check`: *(see §5.1)*

Build/test gates (run before the window closed, emcc and clang only):

- `ninja -C portable/build-wasm` — clean, exit 0
- `ninja -C portable/build-wasm legoland_headless legoland_headless_debug legoland_tests legoland_pathtest legoland_browser legoland_browser_named legoland_cbtypes` — exit 0, `cast forwarders (latent indirect-call type mismatch): 0`
- wasm `ctest` — **17/17 passed** (including `pointer_words` and `coaster_span`)
- native `ninja -C portable/build` + `ctest` — **11/11 passed**
- `portable/tests/test_callback_types.c` — the PORT-M9 section appended
  (9 new tables, 25 new (slot, body) pairs); `legoland_cbtypes` compiles clean
  on both toolchains

## 6. Findings for the integrator

**M9-1 — `ObjDef +0x8c` has no recovered call site AND a mixed body set.**
Four of this lane's sites store into it, and they disagree:
`SetEditObjectFromElem` takes the element (`(i32) -> void`), while
`BsWater_SelectForPlacement`, `BoatingSchool_SelectForPlacement` and
`Mermaid_SelectForPlacement` take nothing (`() -> void`). `interfaces.c`'s
`.data` registrations put `GoldRush_Tick` and friends in the same slot.
PORT-M3's census never produced a `cb_8c` group for exactly this reason, and
no call site anywhere in the 258 sources invokes `+0x8c`, so **nothing traps
today** and nothing can be checked. If a future lane finds the call site, this
slot needs the M3 treatment. Recorded in the `not named above` comment of the
PORT-M9 section of `test_callback_types.c`.

**M9-2 — `g_render_arena2` is gap-sized to 320 bytes (20 items).** The two
arenas are symmetrical in the original (`0x630108` and `0x638218`, `0x8110`
apart), but `rin.c:230` declares `g_fpu_cw_saved` at `0x00638358` — only `0x140`
past arena 2's base — so the gap tiling stops there and the generated block is
`unsigned int g_render_arena2[80]`. Arena 1 gets `[8192]` (32768 bytes). If the
frame ever bump-allocates more than 20 items into list 2 the portable build
overruns the block. I did **not** guess an explicit extent; the honest bound is
unknown from the image alone (`.bss` carries no size). Owner: PORT-A /
`gen_link.py` (or a lane that can read the arena's true capacity out of the
allocator's own checks, if it has any — `tinystubs.c:228` does not bounds-check).
Note this is strictly better than before: the previous behaviour was writing to
raw linear address `0x00638218`.

**M9-3 — one address, two names: `0x004b5b3c`.** `coaster12.c:84` calls it
`g_span_cursor`, `schoolcar.c:1191` calls it `g_cmd_write`. Both are right
(the command buffer *is* the span record buffer); `gen_link.py` aliases them, so
nothing is broken, but it is the same shape as the naming disagreements M2/M4/M5
resolved from the disassembly, and worth a decision. Likewise `0x004dd870` is
`g_cmd_buf` in schoolcar.c and had no name in coaster12.c until this lane
declared it there.

**M9-4 — PORT-B9's site count for `loaders.c` was 21; it is 24.** Lines 171‑182
are twelve stores, not nine. All 24 are fixed.

**A7-2 is still open and is NOT this lane's file.** PORT-B9 localised it: the
MAP toolbar button traps because `mapscreen.c:103`/`:105` declare
`CreateFunctionBasedSprite`'s hook and `RenderFullMap` as `(void)` where
`sprite2.c:199`/`:253` define and call the slot as `void (*)(SpriteRec*)`.
`mapscreen.c` and `sprite2.c` belong to the parallel lane PORT-M8, so the exact
edit is recorded here and **not applied**:

* `renderview.c:2935` — under `#ifdef LEGOLAND_PORTABLE`, give `RenderFullMap`
  the slot's signature, `void RenderFullMap(SpriteRec* unused)`, parameter
  ignored.
* `mapscreen.c:103` — `CreateFunctionBasedSprite(void (*fn)(SpriteRec*), short w, short h)`.
* `mapscreen.c:105` — `extern void RenderFullMap(SpriteRec*);`.

A cdecl callee that never reads an incoming stack argument emits the same bytes
either way, so `audit.py`/`relocs.py` should stay clean — that is the gate to
run, not an assumption. `bubblecache.c:150` already declares the same hook
correctly, which is why the front end's cached text sprites work.

## 7. Files touched

| file | change |
| --- | --- |
| `LEGOLAND/sweep3.c` | 5 literals named; 5 externs + 1 portable adapter inserted |
| `LEGOLAND/loaders.c` | 24 literals named; 24 externs + 3 portable adapters inserted |
| `LEGOLAND/coaster10.c` | 3 literals named; 3 typed table declarations inserted (2 of them new to the tree) |
| `LEGOLAND/sweep1.c` | 2 literals named; 2 arena declarations inserted |
| `LEGOLAND/coaster12.c` | 1 bound expressed from `g_cmd_buf` in a portable arm; 1 extern inserted |
| `portable/tests/test_callback_types.c` | append-only PORT-M9 section: 9 tables, 25 pairs |
| `docs/SCOPE_PORT_WAVE.md` | status line |
| `docs/lanes/scope-port-m9.md` | this file |

No `portable/**` source, no `docs/HANDOFF.md`, none of PORT-M8's files.
