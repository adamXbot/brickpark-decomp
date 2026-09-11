# Scope PORT-M8 — the toolbar click, the `+0xb0` mixed slot, and twelve declarations

> **PORT-M8 — Status: IN PROGRESS (claimed 2026-09-12 by PORT-M8)**
> Branch `scope/PORT-M8` from `feat/decomp-completion-next-steps-24a0d6` @
> `825a90b5` (the PORT-M7 merge). Files touched: `LEGOLAND/*.c` (19) plus an
> append-only block in `portable/tests/test_callback_types.c`. `portable/**`
> otherwise and `docs/HANDOFF.md` belong to other lanes and were not touched.

---

## 1. Headline

**The park toolbar's `call_indirect` type mismatch is gone, and the `+0xb0`
slot is not an open question any more — it has a call site, and the call site
settles it.**

| | before | after |
| --- | --- | --- |
| A7-2, the MAP toolbar icon | `RuntimeError: function signature mismatch` in `MakeSprite` | the call goes through; `RenderFullMap` runs (§2) |
| `ObjDef +0xb0`'s consumer | "no call site anywhere in the recovered tree" (PORT-M3 §5, re-checked by PORT-M7 §2) | `printlist.c:641/660`, **six arguments**, found in the disassembly (§3) |
| `+0xb0` stores disagreeing with it | 7 (4 bodies) | **0** — registration-site adapters |
| `gen/pointers.md` "left raw" | 12 words | **1** (`g_vwin32`, a Win32 `INVALID_HANDLE_VALUE` sentinel — correct as it stands) |
| `gen/pointers.md` rejected declarations | 19 declarations, 30 words | **2 declarations, 7 words** (§4) |
| declared pointer words | 7348 | **7353** (+5: the copters' polyline `pts`, which were RAW) |
| re-pointed INTO a block | 693 | **698** |
| **raw pointer words (the A7 gate)** | 0 | **0** |
| PORT-M6 §1f multi-address extern sweep | closed | **still 0 tree-wide** (§6) |
| `progress.py --check` | 3281 exact / 42 WIP | **3281 / 42**, unchanged |

---

## 2. A7-2: the park toolbar's MAP icon

### 2a. Naming it

Reproduced on `legoland_browser_named` (`legoland_dbg.html`), served on `:8804`,
`?args=-nointro+WINDEBUG&beat=1000`, replaying PORT-A7 §4 exactly
(`llMove(320,240); llClick(260,188); llType('adam'); llClick(505,345);
llClick(252,362); llClick(577,419)` → the in-game screen, frame hash
`0x561128cd`, identical to A7's row 5). Clicking the fourth round toolbar
icon at game **(281, 445)** — the map icon — killed the module:

```
RuntimeError: function signature mismatch
  at MakeSprite           wasm-function[1646]:0xf6277
  at MakeSpriteDrawable   wasm-function[1400]:0xd79e2
  at PrintSprite          wasm-function[1154]:0xae868
  at GameFrame            wasm-function[563]:0x612e7
  at main                 wasm-function[40]:0x8dfb
```

which is PORT-A7's `[1628] <- [1383] <- [1137] <- [564] <- [40]` with the names
the `-g2` link supplies.

**A module byte offset does not survive a rebuild.** A7's `0xf4ac8` is not a
`call_indirect` in this tree's `legoland.wasm` (`name_trap.py --at 0xf4ac8`
answers "no call_indirect at the offset the stack reported"), which reads like
the frame was inlined and is not — the offset simply belongs to another build.
Reproduce the trap on YOUR build and use the offset that run prints; do it on
`legoland_dbg` (`legoland_browser_named`) and the stack arrives named, so the
offset only has to survive as far as the next command.

`name_trap.py --at 0xf6277 --wasm portable/build-wasm/legoland_dbg.wasm`:

```
== INDIRECT CALL TYPE MISMATCH
   caller      MakeSprite   (module offset 0xf6277)
   call site   call_indirect type 0 = (i32) -> void
```

and the two operands pushed are `local.get 0` (the sprite) and `local.get 1`,
loaded ten instructions earlier as `local.get 0; i32.load 8` — i.e. `s->image`.
That is `sprite2.c:253`, one line:

```c
// FUNCTION: LEGOLAND 0x00497b70
int MakeSprite(SpriteRec* s)
{
    if (s->flags & 0x20) {
        PushSetTarget(s);
        ((SpriteDrawFn)s->image)(s);          /* <- the call_indirect */
```

**The site, the slot and the bodies.** The slot is a function-based sprite's
painter, installed by `CreateFunctionBasedSprite` (0x004976c0) into the sprite's
`image` word. Exactly two files register one:

| file | declares the parameter | registers | the body really is |
| --- | --- | --- | --- |
| `bubblecache.c:150` | `void (*fn)(SpriteRec*)` ✔ | `DrawCachedTextSprite` | `void DrawCachedTextSprite(SpriteRec* s)` ✔ |
| `mapscreen.c:115` | `void (*fn)(void)` ✘ | `RenderFullMap` | `void RenderFullMap(void)` |

The disassembly settles both halves:

```
0x00497b70  MakeSprite
  push esi
  mov  esi, [esp+8]
  test byte ptr [esi+0x10], 0x20
  je   0x497b94
  push esi
  call 0x466560           ; PushSetTarget
  push esi                ; <- ONE argument
  call dword ptr [esi+8]  ; <- the slot
  add  esp, 8
```

so the slot passes the sprite; and

```
0x004567a0  RenderFullMap
  sub  esp, 0xf8
  push ebp
  push 0x4b5bfc
  call 0x47b3f0           ; ElemID
```

never reads the incoming dword, so the body genuinely takes none. This is
PORT-M3 §5's **mixed body set**: the declaration is wrong AND the body cannot
be re-declared, so `mapscreen.c` keeps the matched text for VC6 and, in the
portable arm, states the slot's real type and registers an adapter:

```c
#else
extern SpriteRec* CreateFunctionBasedSprite(void (*fn)(SpriteRec*), short w, short h);
#endif
extern void       RenderFullMap(void);                             /* 0x004567a0 */
#ifdef LEGOLAND_PORTABLE
static void ll_m8_render_full_map(SpriteRec* s) { (void)s; RenderFullMap(); }
#endif
```

with the one store site guarded the way PORT-M3/M7 guard theirs.

### 2b. What the toolbar does now

Same page, same replay, **after** the fix. The click is no longer a type
mismatch: the adapter is entered and `RenderFullMap` runs for the first time.
It then faults, on real work, one level deeper:

```
RuntimeError: memory access out of bounds
  at ll_m8_render_full_map   wasm-function[989]:0x93da9
  at MakeSprite              wasm-function[1646]:0xf6273
  at MakeSpriteDrawable / PrintSprite / GameFrame / main
```

Reproduced three times on fresh loads, frame hash `0xc91947e4` every time
(the map screen's chrome does draw before the fault). The faulting instruction is a
plain load, and the five instructions around it are

```
0x93d96  local.get 9 ; i32.load 12      ; set->code
0x93d9f  local.get 9 ; i32.load 0       ; set->base_slot
0x93da5  i32.const 2 ; i32.shl ; i32.add
0x93da9  i32.load 0                     ; <- faults
0x93dac  i32.const 63 ; i32.and
0x93db1  i32.const 48 ; i32.gt_u
```

which is `renderview.c:3044` verbatim:

```c
tcode = g_map_rows[y][x].tile;
set   = g_tile_info[tcode].set;
...
tcode = set->code[tcode - set->base_slot] & 0x3f;
if (tcode > 0x30)
```

**This is not a type defect and it is not in the closure.** `g_tile_info`
(0x00801f40) is emitted correctly as ONE 16384-byte uninitialised block with
`g_tile_recs` / `g_tile_slots` as interior aliases; it is filled at RUN TIME by
the level's tile loader, and `g_map_rows` likewise. A `set` that is neither
null nor a valid `TileSet` means the park's map and tile sets are not there —
which is **PORT-A7's A7-1** ("the level LOADS, then the host ring goes
completely quiet and the frame hash is unchanged 46 s later; the map area keeps
the previous screen's backdrop"). A7-2 was standing in front of A7-1; it is not
any more, and A7-1 now has a second, much sharper symptom to work from:
`g_tile_info[g_map_rows[y][x].tile].set` is garbage at
`renderview.c:3032/3044`. **No guard was added** — papering over it would hide
exactly the measurement the next lane needs. Recorded as **M8-1** in §7.

`RenderFullMap` is also `// WIP-FUNCTION ... (25.6%, 864/1161 strict)`, so the
body is a partial reconstruction; whoever takes M8-1 should establish whether
the map data is absent before reading anything into the WIP body.

### 2c. Every other toolbar position

From the in-game screen, one load, clicking each toolbar position in turn
(game pixels; the toolbar is the bottom two rows and the notepad/theme panel):

| # | position | frame hash after | trap |
| --- | --- | --- | --- |
| — | the in-game screen | `0x561128cd` | — |
| 1 | LEGOLAND button (54, 393) | `0x435a39a8` | none |
| 2 | bar 1 (152, 393) | `0x81390e74` | none |
| 3 | bar 2 (250, 393) | `0x1d6b9c38` | none |
| 4 | bar 3 (347, 393) | `0x968bdb6c` | none |
| 5 | build / path (40, 445) | `0x26899600` | none |
| 6 | query `?` (119, 445) | `0x5d07de64` | none |
| 7 | eraser (200, 445) | `0x52a12df4` | none |
| 8 | options sliders (359, 445) | `0x1984403c` | none |
| 9 | notepad (462, 424) | `0x3c6f4d10` | none |
| 10 | theme panel (581, 443) | `0xb40f87bd` | none |
| — | MAP (281, 445) | `0xc91947e4` | **M8-1**, §2b |

Every position answers. **The only menu that opens is the LOAD GAME screen**
(reached from the options group; "Load Game / adam" with eight EMPTY slots,
screenshot taken) — the rest raise the icon's bubble help (e.g. "Build a
section of path." for icon 5) and change the frame, but no panel appears,
because the centre of the screen is still the previous screen's backdrop
(A7-1). Isolated per-icon runs were done for the build icon and the MAP icon
(twice each, fresh loads); the rest of the table is one chained sweep, which is
what the note says it is.

---

## 3. `ObjDef +0xb0`: it has a call site, and it takes six arguments

PORT-M3 §5 listed `+0xb0` as "unknown — no call site", 14 six-argument bodies
against 4 four-argument ones, "wait for the consumer"; PORT-M7 §2 re-derived
the same answer from the declarations (every `+0xb0` declaration in all 258
sources is `void*`, so there is no typed function-pointer to disagree with) and
recorded it as the lane's one open item.

**The consumer exists.** It is not findable from the declarations because it
does not name an `ObjDef` at all — it reaches the class through a vtable of its
own naming. Sweeping every `call dword ptr [<reg> + 0xb0]` in the image,
function by function from the `// FUNCTION:` markers
(`scratchpad/port-m8-callslot.py`), returns exactly two, both in one function:

```
=== DrawAndClearPrintList (printlist.c:614)  call at 0x00485ac3  dword ptr [edx + 0xb0]
      0x00485ab1  mov  edx, [eax + 0xc]        ; the class, off the element
      0x00485ab4  push ecx                     ; &ctx.n
      0x00485ab8  push ebp                     ; clip
      ... six pushes ...
      >>> pushes immediately before: 6
=== DrawAndClearPrintList (printlist.c:614)  call at 0x00485b70  dword ptr [ecx + 0xb0]
      >>> pushes immediately before: 6
```

**Two sweeps, because one form is not enough.** VC6 emits an indirect slot call
two ways: `call dword ptr [reg + off]` and `mov reg, [base + off] ... call reg`.
`port-m8-callslot.py` finds the first, `port-m8-loadslot.py` the second (it
tracks the loaded register until it is called or redefined). The second sweep
is validated by the slots whose call sites are already known: it finds
`+0xa0`'s two (`RenderView` 0x0045b95a and `RenderFullMap` 0x004571a3, two
pushes — PORT-M7's `def->draw(def->ctx, base)`), `+0xac`'s
(`LLIDB_UnLoadLLSData` 0x0047c6c2), `+0xb8`'s (`LoadGame` 0x0047f517) and
`+0xbc`'s (`SaveGame` 0x0047e63d), none of which the first sweep sees.
**For `+0xb0` the second sweep adds nothing: the two sites above are all there
are in the image.** (The push count is a hint, not proof — it counts pushes
since the previous call, so it can include unrelated spills; for `+0xb0` it is
confirmed against the C below, which passes six.)

In the C the two sites are `printlist.c:641` and `:660`:

```c
owner->vtbl->DrawOver(owner, p->x, p->y, &p->ctx.n, &p->u.c.clip, p->u.c.mode);
```

`printlist.c:188` declares that vtable as `HitOwnerVtbl { char pad00[0xb0];
void (*DrawOver)(HitOwner*, int, int, int*, ClipRect*, int); }` and
`printlist.c:193` declares `HitOwner { char pad00[0x0c]; HitOwnerVtbl* vtbl; }`
— which is `interfaces.c:78`'s `RideElem { name; image; flags; RideDef* data
/* +0x0c */ }` under other names. **`HitOwnerVtbl` IS `RideDef`, `+0xb0` IS
`cb_interact`, and the canonical type is
`(elem, x, y, sq, clip, mode) -> void`.** It is reached for every
`0x2000`-flagged sprite whose print node carries a `0x103` blit context, i.e.
on the ordinary in-game draw path.

`scratchpad/port-m8-b0sweep.py` reads every `->cb_b0 =` / `->cb_interact =`
store in the tree against the DEFINITION of the body stored:

| arity | stores | bodies |
| --- | --- | --- |
| 6 | 37 | `Castle_Interact`, the eleven log-flume `LF*_Interact`, the ride `*_Interact` set, the fourteen `screen.c` `*_Draw` / `*_DrawOverlay` |
| — | 15 | PORT-M7's stale registration names, whose declarations M7 already gave the six-argument body's signature |
| **4** | **7** | **`CastleDummy_Interact`, `Track_Interact` (×4) — castleobj.c; `PottingShed_Draw`, `MechanicsHut_Draw` — screen.c** |

Re-run after the lane, counting only the arm the PORTABLE build compiles (the
script tracks `#ifndef LEGOLAND_PORTABLE` and resolves the file-local static
adapters, which carry no `// FUNCTION:` marker):

```
arity 6: 44 store(s)   <- 37 + the four adapters at 7 sites
arity ?: 15 store(s)   <- PORT-M7's stale registration names
arity 4: (no bucket)
```

So 52 of the 59 stores already have the slot's type and seven (four bodies) do
not. Those four
get PORT-M3 §5's registration-site adapter in the portable arm — one adapter per
body, the slot's own six-argument type, dropping the two arguments x86 cdecl
dropped for free — with each store site `#ifndef`/`#else` guarded so VC6's text
is unchanged:

```c
static void ll_cb_b0_Track_Interact(int ll_a, int ll_b, int ll_c,
                                    MapPos* ll_p, void* ll_clip, int ll_mode)
{ (void)ll_clip; (void)ll_mode; Track_Interact(ll_a, ll_b, ll_c, ll_p); }
```

`screen.c`'s two were invisible to every previous sweep for a second reason:
that file declares them `extern void PottingShed_Draw();` — an EMPTY parameter
list, which says nothing at all, so no arity gate could compare it with
anything. The portable arm adds the real prototype next to the adapter.

**After this lane the `+0xb0` row of PORT-M3 §5 is closed**, and it is closed
against a call site rather than by declaring the bodies to agree with each
other.

---

## 4. PORT-A7 §6's twelve declaration findings, one at a time

Every verdict below is read off the image. The census numbers are
`portable/build-wasm/gen-browser/pointers.md` before and after.

### 4a. `g_copters_poly0..4` — **the declaration is wrong, and it was a LIVE defect**

`mechrides.c:842-846` declared five `void*` at 0x004b41c8 / 4208 / 4240 / 4270 /
4298 holding 6, 7, 6, 5, 4. Those are point COUNTS, and the word after each is
a POINTER the declaration never mentioned. The image lays the five out as
`[points][header]` pairs:

```
0x004b41c8 = { 6, 0x004b4198 }    0x004b4198 .. 0x004b41c8 = 6 x {int,int}
0x004b4208 = { 7, 0x004b41d0 }    0x004b41d0 .. 0x004b4208 = 7 x {int,int}
0x004b4240 = { 6, 0x004b4210 }    0x004b4210 .. 0x004b4240 = 6 x {int,int}
0x004b4270 = { 5, 0x004b4248 }    0x004b4248 .. 0x004b4270 = 5 x {int,int}
0x004b4298 = { 4, 0x004b4278 }    0x004b4278 .. 0x004b4298 = 4 x {int,int}
```

— every header's second word is the address of the array immediately below it,
and every array's length is its header's count. That is `goldrush.c:467`'s
`PolyLine { int count; Pos* pts; }`, the shape `g_goldrush_polyline`
(0x004b4608 = `{ 5, ... }`) already carries; `BuildWalkPath` (0x00412100) is
handed `&g_copters_polyN` here exactly as it is handed `&g_goldrush_polyline`
there.

Because the `void*` spelling named the count word and said nothing about the
pointer word, the browser closure emitted all five `pts` as **raw x86
addresses**:

```c
/* before */ unsigned int g_copters_poly0[16] = { 0x00000006u, 0x004b4198u, ... };
/* after  */ unsigned int g_copters_poly0[16] = { 0x00000006u,
                 (unsigned int)(__UINTPTR_TYPE__)((char*)(g_copters_spr_names) + 40), ... };
```

Every copter's walk path was built from a number that means nothing in the
rebuilt layout. **Fixed for both builds** — `&g_copters_polyN` is the only use
in the tree, so no byte can move, and `audit.py` says none did.

### 4b. `g_span_vtx` — **the declaration is wrong; it is a count**

`coaster11.c:150` declared `void* g_span_vtx` (0x004b560c, holding 2). Its
only writer is

```
0x0041f030  Span_SetVertexBuf
  mov eax, [esp+4]
  mov dword ptr [0x4b5608], 0x1c        ; g_span_vtx_stride = sizeof PolyVtx
  mov dword ptr [0x4b560c], eax
```

called from `Raster_ClipPoly` (0x0041ef60) as `Span_SetVertexBuf(n + 1)` with
`n` the vertex-index argument, and every read is `(int)g_span_vtx` used as the
inclusive bound of the per-vertex dword lerp in `Span_ClipPlane`
(`while (k <= (int)g_span_vtx)`). Nothing dereferences it. Retyped `int` for
both builds, with `Span_SetVertexBuf(int v)` and the one caller passing
`n + 1` — the same two dwords through the same registers, no cast anywhere, and
`audit.py` unchanged.

### 4c. `g_music_sys` — **the declaration is wrong in nine files; `startup.c` had it right**

`startup.c:23` declares it `int` and `startup.c:255` is the only writer in the
whole tree:

```c
g_music_sys = FindCommandSwitch(cmdline, "-nomusic") == 0;
```

The image initialises the word to 1; every other use in all ten files is a
truth test (`if (g_music_sys)`, `if (!g_music_sys)`,
`if (g_music_sys && g_music_ready)`); nothing dereferences it and nothing ever
stores a pointer. Nine `extern void*` lines (audio2.c, audio3.c, music.c,
musicthread.c, resaudio2.c, sweep5.c, sysmisc3.c, sysstubs.c, unref7.c) now say
`int`, and all ten comments now say what the word is ("music ENABLED flag")
instead of "music engine instance". Nine census rows retired. Both builds: a
4-byte truth test is a 4-byte truth test, and `audit.py` is unchanged in all
nine files.

### 4d. `g_menu_help` — **the declaration is wrong about the image; what a set slot MEANS is not decidable**

`movie.c:211` declared `void* g_menu_help[4]` at 0x004bb18c. The address and
the bound are proved:

```
0x00475fe0  SetMenuHelp(int slot, ...)
  mov eax, [esp+4] ; test eax,eax ; jl ... ; cmp eax,4 ; jge ...
  mov ecx, [esp+8]
  mov dword ptr [eax*4 + 0x4bb18c], ecx
```

and the neighbour below it is `g_build_followups[29]`, whose own bound is
confirmed independently by `SelectNextBuildObject`'s loop
(`mov esi, 0x4bb0a8 ... add esi, 8 ; cmp esi, 0x4bb190 ; jb`, i.e. last record
at 0x004bb184, table ending at 0x004bb18c).

But the `.data` initialiser at 0x004bb18c is `100, 140, 200, 10000` — four
numbers, which no `void*[4]` can hold. `SetMenuHelp` is the only writer in the
image, its only caller is `tinystubs.c:311`'s `for (i=0;i<4;++i)
SetMenuHelp(i, 0)` clear loop, and **nothing in the image reads the array at
all**. So the type is wrong about the image and the truth beyond that is not
recoverable from the binary. Declared as the words the image holds
(`unsigned int g_menu_help[4]`, `SetMenuHelp(int slot, unsigned int text)`),
which is at least true of the only bytes there are; the open question is
recorded as **M8-2** in §7.

### 4e. `g_level_markers` — **a real framing error, in the eight-byte-rotated half**

`bigscreens.c:103` declared `LevelMarker g_level_markers[10]` at **0x004beb88**
with its fields rotated (`{str_id, x, y, lit, dim, lit_name, dim_name}`);
`screens3.c:276` declares the same table at **0x004beb80** with
`{lit_name, dim_name, str_id, x, y, lit, dim}`. PORT-A7 §2a called both
descriptions "correct about the same memory". They are not quite:

* Every field `bigscreens.c` actually READS lands at the same address under
  either framing — `g_level_markers[i].x` is `0x004beb8c + 28*i` both ways, and
  so are `str_id`, `y`, `lit`, `dim`. That is why no byte moves.
* But the rotated frame's two trailing name pointers are **the NEXT record's**:
  `B + 28i + 0x14 = S + 28(i+1)`. So the tenth record's pair fell on
  `g_progress_click_level` (0x004bec98, `0xffffffff`) and the dword above it —
  the four words PORT-A7's census rejected.

`screens3.c`'s framing is the true one and its ten records are complete: record
0's `str_id` is 0x004beb88 = 0x19 and the ten ids run 0x19..0x22 with sensible
screen coordinates, the table is exactly 0x004beb80 .. 0x004bec98, and
`g_progress_click_level` follows it. `bigscreens.c` now frames it the same way
(the struct and the address moved together), so:

```
/* before */ 0x004beb70 g_mode_wplus[6]  (its tail holding record 0's names)
             0x004beb88 g_level_markers[70]  interior: g_progress_click_level+0x110
/* after  */ 0x004beb70 g_mode_wplus[4]
             0x004beb80 g_level_markers[70]
             0x004bec98 g_progress_click_level[2]
```

All twenty name pointers are re-pointed, none is rejected, and
`g_progress_click_level` is its own object instead of an interior alias of the
marker table. **A7's suggestion that "the bound could be 9" would also have
removed the rejection, but it would have made `bigscreens.c`'s own
`for (i = 0; i < 10; i++)` loop an out-of-bounds read; reframing keeps the loop
correct.**

### 4f. `MeshDesc g_track_mesh` — **NOT a game-source defect: `cdecl.py` mis-parses `T (*x)[N]`**

PORT-A7 called this "either the wrong struct or the wrong address". It is
neither. `schoolcar3.c:277`'s `MeshDesc` is right and 0x004b5f60 is right —
`globals.c` shows its three pointer fields at +0x10/+0x14/+0x18 holding
`&g_track_verts`, `&g_mesh_pairs`, `&g_mesh_tris`, all three re-pointed. The
six rejected words are at +0x20 and +0x24, which are **past the struct**: they
are the first floats of `g_support_shadow_templates`.

The cause is the tool. `scratchpad/port-m8-cdecl-probe.py` asks `cdecl.py` for
the layout it gives this type:

```
object g_track_mesh addr 0x4b5f60 typename MeshDesc
  size 0x28 kind struct                 <- should be 0x1c
  field verts off 0x10 size 0x4 ptr True
  field pairs off 0x14 size 0x4 ptr True
  field tris  off 0x1c size 0x4 ptr True   <- should be 0x18
  pointer_offsets ['0x10','0x14','0x18','0x1c','0x20','0x24']   <- should be 3
```

`int (*pairs)[2]` is a POINTER TO an array of two ints (4 bytes); the parser
reads it as `int* pairs[2]`, an ARRAY OF two pointers (8 bytes), so every field
after it moves and three pointer words are invented. `int (*tris)[3]` costs
twelve bytes the same way.

**Blast radius: exactly two struct types tree-wide** — `schoolcar3.c`'s
`MeshDesc` and `unref2.c`'s identical copy; `grep -rn '(\*[A-Za-z_][A-Za-z0-9_]*)\[[0-9]'`
over `LEGOLAND/*.c` returns six lines, four of them those fields and two of
them local variables. Nothing is live today (the three real pointers are
re-pointed anyway, by the exact-symbol path), and the file is
`portable/tools/cdecl.py`, which this lane does not own. Recorded as **M8-3**
for a PORT-A lane, with the probe script.

### 4g. `g_vwin32` — **not a defect at all**

`unref5.c:85`'s `void* g_vwin32` (0x004b85c4) is a Win32 HANDLE and
`0xffffffff` is `INVALID_HANDLE_VALUE`; `unref5.c:572-578` tests
`g_vwin32 != (void*)-1` and resets it to `(void*)-1` on close. The declaration
is right, the value is right, and the census's veto does the right thing (the
word is left exactly as the image has it). It is the one row that stays in
`pointers.md`, and it should.

### 4h. The census, before and after — measured, not asserted

The pre-lane tree (`825a90b5`, the 258 sources exactly as merged) was built into
a build directory of its own (`scratchpad/port-m8-basebuild.sh`) and the two
generated closures diffed. **`gen-browser/aliases.c` is byte-identical** (cast
forwarders 0 on both), and `manifest.md` differs in exactly these lines and no
others:

```
- globals defined:                          2042 -> 2043
- objects merged from interior-aliased:     44   -> 43     (the g_level_markers row, 4e)
- symbols re-pointed into (ilp32):          308  -> 313    (+5, the copters, 4a)
- pointer words re-pointed INTO a block:    693  -> 698
- pointer words left raw:                   12   -> 1      (g_vwin32 only, 4g)
- declared pointer words:                   7348 -> 7353
- pointer claims the image rejects:         30 words / 19 declarations -> 7 / 2
- objects the gap tiling would have split:  44   -> 43
- **raw pointer words: 0  ->  0**
- Conflicting wasm signatures:              the same 11 names, unchanged
- cast forwarders:                          0 -> 0
```

`globals.c` differs in **68 lines**, every one of them accounted for:

* the five copters `pts` words, raw `0x004b4198` / `0x004b41d0` / `0x004b4210` /
  `0x004b4248` / `0x004b4278` becoming `(char*)&sym + off` expressions that
  resolve to the same addresses (plus the five forward declarations they need);
* `g_mode_wplus` 24 → 16 bytes (it is the string `"w+"` at +0; the eight bytes
  it loses are `g_level_markers` record 0's two name pointers, which are now
  inside the marker table and still re-pointed);
* `g_level_markers` moving from `0x004beb88` to `0x004beb80` — **the same 280
  bytes of data, shifted eight bytes earlier**, with the twenty name pointers
  re-pointed and none rejected;
* `g_progress_click_level` becoming its own 8-byte object at `0x004bec98`
  instead of an interior alias at `g_level_markers+272`.

`gen/extents.md` loses exactly one row, the one that named the defect:

```
- | 0x004beb88 | g_level_markers | ... -> 280 (0x118) | 272 | merged |
    `g_progress_click_level`+0x110 ([9].lit_name) |
```

---

## 5. `bigscreens.c:57` — the last two-argument icon-input spelling

PORT-M7 §8.5's item. The Icon `+0x2c` slot and `g_icon_handler1/2`
(0x006687bc / 0x006687c0) are called with four arguments — `fpui.c:772/791`
`g_icon_handler2(0, g_mouse_btn_a, 0, 0)`, and `CheckFocussedIcon` through the
slot — and `appraisal.c:68`, `appraisalscreen.c:21`, `screens2.c` and (since
PORT-M7) `mapscreen.c:62` all say so. `bigscreens.c` still said
`typedef char (*IconInputFn)(Icon*, int)`, with the `+0x2c` field and the four
bodies it registers declared to match.

Those four bodies are DEFINED four-argument, in `screens3.c`:
`ProgressLevelInput` (0x0048bb60, `screens3.c:1358`), `ProgressAcceptInput`
(0x0048bc20, `:1383`), `ProgressGoBackInput` (0x0048c020, `:1452`),
`ProgressTutorialInput` (0x0048c090, `:1473`) — all
`char (Icon* p, int buttons, int a3, int a4)`. Nothing traps today, because
clang takes a function's wasm type from its definition and `bigscreens.c` only
takes addresses; the two-argument spelling is what a reader and every arity
gate see. Moved to four arguments in a **portable arm** — the typedef, the
`+0x2c` field and the four externs together, exactly as PORT-M7 moved
`mapscreen.c`'s `IconHandler` — so VC6's text is character-for-character what
it was.

**And it is not quite "the last" one.** `frontend2.c:67/73`, `bighelp.c:421`,
`iconui.c:37` and `mapscreen2.c:74` carry the same two-argument spelling of the
`+0x2c` field. None of them can trap, and this is measured rather than assumed:
**every call through the slot in the whole tree is four-argument** —

```
fpui.c:778    return p->input(p, ev, g_mouse.x - p->x, g_mouse.y - p->y);
fpui.c:781    return p->input(p, ev, g_mouse.x - p->x, g_mouse.y - p->y);
uimisc.c:661  if (entry->chosen) p->input(p, 2, 0, 0);
fpui.c:772    return g_icon_handler2(0, g_mouse_btn_a, 0, 0);
fpui.c:791    return g_icon_handler1(0, g_mouse_ev, 0, 0);
```

— and both files declare it four-argument. The four above only take addresses.
`bigscreens.c` is the one PORT-M7 named and the one this lane was asked for;
the rest are recorded as **M8-4** so the class can be finished in one pass.

---

## 6. PORT-M6's multi-address extern sweep, re-run

`scratchpad/port-m8-multidecl.py` is PORT-M6 §1f's gate, rewritten (M6's own
script was scratch and is not in the tree): every `extern` DATA statement with
more than one declarator whose trailing comment names more than one address.
This is the class where the C is correct and only the tooling's reading is
wrong — `pathmisc2.c`'s `extern void *g_route_open, *g_route_closed;
/* 0x00668fc0, 0x00668fc4 */`, which every scanner read as ONE object at the
first address, so `g_route_open` WAS `g_route_closed` in the portable build.
No byte-level gate can see it.

Self-tested positive against both of M6's spellings (`a, b` with
`/* 0xA, 0xB */` and with `/* 0xA .. 0xB */`) and negative against a function
declaration with two addresses in its comment; then run over all 258 `.c` and
every `.h`:

```
0 multi-address extern statement(s)
```

**The class is still closed. Nothing to fix.**

---

## 7. Findings this lane records rather than fixes

| # | what | owner |
| --- | --- | --- |
| **M8-1** | **The park MAP screen faults in `RenderFullMap`.** With A7-2 fixed the map toolbar icon reaches `renderview.c:3032/3044`, where `g_tile_info[g_map_rows[y][x].tile].set` is neither null nor a valid `TileSet` and `set->code[...]` reads out of bounds (the faulting `i32.load` sits at module offset `0x93da9`/`0x93d92` of `legoland_dbg.wasm` depending on the build; reproduced three times on fresh loads, frame hash `0xc91947e4` every time). `g_tile_info` (0x00801f40) and `g_map_rows` are uninitialised blocks the LEVEL fills, and both are emitted correctly, so this is the same root cause as **A7-1** (the park's map area never repaints, the loader goes quiet after the first read burst) seen from a second direction. `RenderFullMap` is also only 25.6% matched — establish whether the map data is there before reading anything into the WIP body. | a PORT-B lane, with A7-1 |
| **M8-2** | `g_menu_help[4]` (0x004bb18c) is initialised `{100, 140, 200, 10000}`, is written only by `SetMenuHelp` (whose only caller writes 0), and is **read nowhere in the image**. Address and bound are proved; the name and the meaning of a set slot are a guess. Declared as plain words by this lane. | a matching lane, if the four numbers ever turn up elsewhere |
| **M8-3** | **`portable/tools/cdecl.py` parses `T (*x)[N]` as an array of N pointers.** `MeshDesc` comes out 0x28 bytes instead of 0x1c with 6 pointer offsets instead of 3 (§4f, `scratchpad/port-m8-cdecl-probe.py` reproduces it in four lines). Two struct types in the tree are affected and nothing is live today, but the fix belongs with the file's owner. | a PORT-A lane |
| **M8-4** | Four files still spell the Icon `+0x2c` input slot with two arguments: `frontend2.c:67/73` (which also has the `IconInputFn` typedef), `bighelp.c:421`, `iconui.c:37`, `mapscreen2.c:74`. **Nothing traps**: every call THROUGH the slot in the whole tree is four-argument (`fpui.c:778`, `fpui.c:781`, `uimisc.c:661`, plus `fpui.c:772/791` through `g_icon_handler1/2`), and those files declare it four-argument; the four above only take addresses, and `frontend2.c`'s own bodies already carry PORT-M3's rename-pattern twins. Spelling only, and cheap to finish. | a matching lane |
| **M8-5** | PORT-M3 §5's `ObjDef +0x8c` row (27 × `() -> void` vs 2 × `(i32) -> void`) stays open, and this lane can now say why with evidence rather than by inheritance: **both** image sweeps (`call dword ptr [reg + 0x8c]` and `mov reg, [base + 0x8c] ... call reg`) return **zero** sites, where the same pair finds every other ObjDef slot's consumer. `+0x8c` really has no caller in the shipped binary, so nothing local can decide its type and PORT-M7's rule — make each declaration agree with its own body — is the best available. Revisit only if a consumer turns up. | a PORT-M lane |

---

## 8. Gate results

Every row measured on the committed tree, `LEGOLAND_CL` = the wibo VC6 `cl`.
`relocs.py` exits 2 on a clean file (bit 1 = "unresolved or skipped"), so the
column is `grep -c MISMATCH`, per HANDOFF §4.

**The per-file gate is not the counts, it is the ROWS.** The pre-lane tree
(`825a90b5`) was checked out into a scratch tree of its own and `audit.py` run
over the same eighteen files there; `diff` of the two outputs is **empty for
all eighteen** — same functions, same instruction counts, same byte counts,
same `mismatch=0` on every `[OK]` row and the same percentage on every `[WIP]`
row. Not one VC6 byte moved, including in the three files with WIP bodies.

| file | what changed | `audit.py` | rows vs `825a90b5` | `relocs.py` MISMATCH | `/W3` |
| --- | --- | --- | --- | --- | --- |
| `mapscreen.c` | A7-2: the sprite-painter slot + the `RenderFullMap` adapter (§2) | 12 `[OK]`, 0 `[WIP]` | identical | 0 | 0 |
| `castleobj.c` | two `+0xb0` adapters, 5 guarded stores (§3) | 39 `[OK]`, 0 `[WIP]` | identical | 0 | 0 |
| `screen.c` | two `+0xb0` adapters + their real prototypes, 2 guarded stores (§3) | 3 `[OK]`, 0 `[WIP]` | identical | 0 | 0 |
| `mechrides.c` | `g_copters_poly0..4` are `PolyLine` (§4a) | 47 `[OK]`, 1 `[WIP]` | identical | 0 | 0 |
| `coaster11.c` | `g_span_vtx` is an `int` count (§4b) | 17 `[OK]`, 2 `[WIP]` | identical | 0 | 0 |
| `audio2.c` | `g_music_sys` `void*` → `int` (§4c) | 9 `[OK]`, 0 `[WIP]` | identical | 0 | 0 |
| `audio3.c` | " | 22 `[OK]`, 0 `[WIP]` | identical | 0 | 0 |
| `music.c` | " | 12 `[OK]`, 0 `[WIP]` | identical | 0 | 0 |
| `musicthread.c` | " | 0 `[OK]`, 1 `[WIP]` | identical | 0 | 0 |
| `resaudio2.c` | " | 4 `[OK]`, 0 `[WIP]` | identical | 0 | 0 |
| `sweep5.c` | " | 5 `[OK]`, 0 `[WIP]` | identical | 0 | 0 |
| `sysmisc3.c` | " | 10 `[OK]`, 0 `[WIP]` | identical | 0 | 0 |
| `sysstubs.c` | " | 42 `[OK]`, 0 `[WIP]` | identical | 0 | 0 |
| `unref7.c` | " | 44 `[OK]`, 0 `[WIP]` | identical | 0 | 0 |
| `startup.c` | the `g_music_sys` comment (it already said `int`) | 7 `[OK]`, 0 `[WIP]` | identical | 0 | 0 |
| `movie.c` | `g_menu_help` is four words (§4d) | 16 `[OK]`, 0 `[WIP]` | identical | 0 | 0 |
| `tinystubs.c` | `SetMenuHelp`'s second parameter, with it | 56 `[OK]`, 0 `[WIP]` | identical | 0 | 0 |
| `bigscreens.c` | `g_level_markers` reframed (§4e) + the icon-input spelling (§5) | 5 `[OK]`, 0 `[WIP]` | identical | 0 | 0 |

**350 `[OK]`, 4 `[WIP]`, 0 `REJECT` / `FAIL` / `COMPILE FAILED`, 0
`MISMATCH`, 0 `/W3` warnings**, across eighteen files.

Tree-wide:

| gate | result |
| --- | --- |
| exact marker SET vs `825a90b5` (HANDOFF §4's "nothing was un-closed") | **3281 before, 3281 after, 0 lost, 0 gained**; 0 duplicate addresses tree-wide |
| `progress.py --check` | **3281 exact / 42 WIP**, `665/675 exports exact (98.5%)`. `--check` called the report stale because the insertions moved line numbers, so it is regenerated and committed; `--check` then exits clean |
| wasm `ctest` | **17/17** (incl. `headless_spine`, the pinned title-screen regression, and `pointer_words`, the A7 gate) |
| native `ctest` | **11/11** |
| `legoland_cbtypes` (`-Werror=incompatible-function-pointer-types`) | builds on **both** targets |
| `grep -c '(\*)' gen-browser/aliases.c` on a clean wasm build | **0**, and `aliases.c` is byte-identical to the pre-lane tree's |
| `gen-browser/manifest.md` "Conflicting wasm signatures" | the **same 11 names**, unchanged |
| **`raw pointer words`** | **0** |
| `legoland_headless -nointro`, 90 s alarm | exits **142** (the alarm — i.e. still in the game loop), 11 output lines, **no `RuntimeError` / `TRAP` / `signature mismatch` / `unreachable`** |
| the page, in a tab | §2b and §2c — the in-game screen at `0x561128cd`, every toolbar position answers, the MAP icon's type mismatch gone |
| PORT-M6 §1f multi-address extern sweep | **0** tree-wide (§6) |
| `+0xb0` stores live in the portable build that disagree with the call site | **0** (44 six-argument + 15 PORT-M7 aliases; the arity-4 bucket is empty) |

The whole-tree `relocs.py --all` sweep is the integrator's (~30 min, alone).

---

## 9. What the integrator must know

1. **Three `LEGOLAND/*.c` files changed for BOTH builds, not in a portable
   arm**, because in each case the disassembly proves the declared type wrong
   and the use sites make byte motion impossible — `mechrides.c` (the five
   copters polylines), `coaster11.c` (`g_span_vtx` and
   `Span_SetVertexBuf`'s parameter), `movie.c` + `tinystubs.c`
   (`g_menu_help` and `SetMenuHelp`'s second parameter), plus the ten
   `g_music_sys` declarations. `audit.py` is the proof and it is in §8, and it is stronger than
   "no REJECT": **every audit row in all eighteen files is byte-identical
   to the same row at `825a90b5`**, WIP percentages included. Everything
   else is an `#ifdef` insertion.
2. **`g_level_markers` moves address in the closure**, from `0x004beb88` to
   `0x004beb80` (§4e). This is a generated-output change with no byte change in
   the image: the same 280 bytes, the same values, the same targets, and
   `g_progress_click_level` becomes its own object instead of an interior
   alias. `globals defined` goes 2042 → 2043 and `objects merged from
   interior-aliased names` 44 → 43 for that reason; neither is a regression.
3. **`raw pointer words` is still 0** and the `pointer_words` ctest still
   passes. The A7 gate is intact; what changed is that five words that were
   raw-and-unclaimed (nothing declared them) are now declared and re-pointed,
   and eleven of the twelve "left raw with a reason" rows are gone because the
   declarations that produced them were wrong.
4. **A7-2 is closed and A7-1 is now reachable from a second direction.** The
   park's MAP toolbar icon no longer trips a `call_indirect` type mismatch; it
   faults inside `RenderFullMap` on the level's tile data (**M8-1**, §2b). That
   is a real defect that was previously hidden, not one this lane introduced —
   the adapter only lets the call through. No guard was added.
5. `portable/tests/test_callback_types.c` gained an append-only PORT-M8 section
   (two tables, 35 entries, `ll_m8_callback_type_pairs()`); PORT-M3's and
   PORT-M7's sections are untouched. `legoland_cbtypes` builds on both targets.
6. **PORT-M3 §5's `+0xb0` row is closed** and its `+0x8c` row is not — the
   method that closed `+0xb0` (sweep the IMAGE for
   `call dword ptr [<reg> + <off>]`, not the declarations) is the one to reuse,
   and `scratchpad/port-m8-callslot.py` takes the offset as an argument
   (**M8-5**).
7. Scratch scripts, all `port-m8-` prefixed and none committed:
   `port-m8-callslot.py` and `port-m8-loadslot.py` (the two forms of an
   indirect slot call — **both are needed**, §3), `port-m8-b0sweep.py` (every
   `+0xb0` registration against its body's definition), `port-m8-xref.py`
   (readers of a data word, function by function so a jump table cannot desync
   the sweep), `port-m8-cdecl-probe.py` (M8-3 in four lines),
   `port-m8-multidecl.py` (PORT-M6 §1f's gate, §6), `port-m8-basebuild.sh`
   (the pre-lane closure, §4h). The two slot sweeps are the reusable ones and
   a future lane should probably promote them into `portable/tools/`.
