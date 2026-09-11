# Scope PORT-M11 — the two classes M10 and B11 left open

> **PORT-M11 — Status: IN PROGRESS (claimed 2026-09-12 by PORT-M11)** — branch
> `scope/PORT-M11`. Matching side: `LEGOLAND/*.c` under
> `#ifdef LEGOLAND_PORTABLE`, VC6-gated per file. `portable/**` is read-only for
> this lane except the paired `g_num_visitors` rename in
> `portable/src/browser/main.c` and `index.html`; `docs/HANDOFF.md` is read-only.
> Brief: `docs/SCOPE_PORT_WAVE.md`.

**Result in one line: the sound effects play, the ride can be taken off the map
again, and `llPark().numVisitors` finally counts visitors** — and the two sweeps
that had to see all three are now in the tree.

| class | before | after |
| --- | --- | --- |
| by-value struct, decl vs def (M10 §1b) | **21** silent sites | **0** |
| by-value struct, SLOT vs body (new here) | **22** sites | **0** |
| raw pointer words, declaration-independent (B11 §3) | **122** in 18 objects | **31** in 1 (the CRT's, §2d) |
| `CreateSoundBuffer` at startup | 0 or 1 | **22** (of 23; the 23rd is not in the archive) |
| `llAudio().plays` over the tutorial walk | — | **108** |

---

## 0. One thing the integrator must know first

**`scope/PORT-B11` was NOT in `feat/decomp-completion-next-steps-24a0d6` when this
lane started**, and this lane's brief requires B11's `llAudio()` and its DSOUND
trace to prove the sound-effect half. So `scope/PORT-M11` **merges
`scope/PORT-B11`** (`43ed12d3`, the only conflict was both lanes' status line in
`docs/SCOPE_PORT_WAVE.md`, resolved by keeping both). Merging M11 therefore
brings B11's five commits with it. If B11 lands first, this merge is a no-op.

---

## 1. The by-value-struct class — and M10 §1f's recommendation is the wrong way round

### 1a. What the original compiled

M10 §1f proposed giving `StandardRemoveObject`'s **definition** a portable-arm
scalar parameter, "which makes the majority spelling and the slot type correct
in one file". Three pieces of evidence say the opposite, and that change would
have broken fifteen files:

* **the slot's own type.** `objmap2.c:99` declares ObjClass +0x9c as
  `void (*remove)(void* obj, BPos bp, void* ctx)` and `objmap2.c:1895`
  (`RemObjFromMap`, 0x00459c90) calls it that way.
* **the frame.** `objmap2.c:977`'s own note: the by-value `BPos` is unpacked
  once into a function-scope `Pos` and "that single 8-byte aggregate (and not
  two ints plus a scratch `Pos`) is what makes the frame 0x1c bytes and keeps
  all four register pushes in the prologue".
* **the disassembly at the call.** `maprestore.c:38`, writing up the two path
  removers that sit in the same slot: the middle argument "is `RemObjFromMap`'s
  packed 2-byte cell coordinate, passed BY VALUE: the caller reads it as two
  separate bytes at `[esp+0x2c]` and `[esp+0x2d]`, **which only happens for a
  2-byte struct in one stack slot**."

And the count is the other way round too. Of the 22 files that declare
0x0045f220, **fifteen already spell the square as a 2-byte aggregate**
(`BPosW` in junglecruise, logflume, logflume9, ridecb2, ridecb5, ridecb6,
ridecb8, ridemisc3, screencb5, waterworks, bswater2 and screencb.c's
`StandardRemoveObject_B`; `CellPos` in catapult, input2, mechrides; `MapPos` in
coaster) and **seven** spell it scalar (goldrush, joust, ridecb9, screencb ×2,
screencb6, sweep3, westtown). So the seven are the outliers, and they are what
moved — in portable arms, with the square packed at the call site in M10's own
macro shape.

### 1b. The direction nothing could see: SLOT vs BODY

The decl/def sweep compares declarations of an address against the definition at
that address. It cannot see a body whose parameter disagrees with the **slot it
is stored in** — the body and every declaration of it can agree perfectly and
still be wrong, because the indirect call's type comes from the function-pointer
FIELD, not from any declaration of the callee.

That is the same silent wasm32 window: `void (*)(void*, BPos, void*)` and
`void (*)(void*, unsigned int, void*)` are both `(i32,i32,i32)->void`, so
`call_indirect` does not trap, `wasm-ld` says nothing, and
`portable/tests/test_callback_types.c` — which declares every one of these as
`(void*, void*, void*)` — type-checks the arity only.

`tools/port_m10_bvstruct_sweep.py` grew a third section for it. Slots are keyed
by **offset**, not by field name, because the same slot is spelled differently in
every file that types the record: `remove` in objmap2.c, `cb_remove` in
interfaces.c and castleobj.c, `cb_9c` in loaders.c, `f4` in sweep3.c. It found
**22**:

| slot | typed at | bodies that spelled it a scalar |
| --- | --- | --- |
| ObjClass **+0x9c** remove, `BPos` | objmap2.c:99 | `CastleLevel1_Remove`, `Fort_Remove`, `Temple_Remove`, `GoldRush_Remove` (goldrush.c); `Joust_Remove`, `TempleSlide_Remove` (joust.c); `MechanicsHut_Remove`, `PottingShed_Remove` (ridecb9.c); `DrivingSchool_Remove` (screencb.c); `FoodService_Remove` (screencb6.c); `Shop_Remove`, `JailCell_Remove` (westtown.c); `CtThunk_Remove`, `Track_Remove` (castleobj.c) |
| ObjDef **+0xa0** draw, `BPos` | renderview.c:304 | `Balloonz_`, `Carousel_`, `DrivingSchool_GetDrawDesc` (ridecb8.c); `JcMonkeyFish_GetDrawDesc` (screencb2.c); `JcMonkeyTree_`, `Restaurant2_GetDrawDesc` (screencb6.c); `MechanicsHut_`, `PottingShed_GetDrawDesc` (ridecb9.c) |

So **every** removal of a castle level, fort, temple, gold rush, jail cell,
joust, temple slide, mechanics hut, potting shed, western-town shop, driving
school, food service or castle track read a shadow-stack address as a map
square — and `GoldRush_Remove`, `Joust_Remove`, `TempleSlide_Remove` and
`JailCell_Remove` then took the ADDRESS of that local for their record lookups.
On the draw side, eight classes stamped an address into the descriptor's square
field (`DrawDesc.f0c`), and `JcMonkeyFish_GetDrawDesc`'s `f->pos.w == arg` scan
could never match a fish.

All 22 are closed with PORT-M3's `_vc6_body` rename: the matched body keeps its
text and its name for VC6, and the portable build exports a wrapper of the
slot's own shape over it. Two forward declarations moved with them
(castleobj.c:652/690), and `CtThunk_Remove` needed one more thing — it forwards
the square to the CtIface row's own handler, whose bodies (`Castle_Remove`,
`CastleDummy_Remove`) take `MapPos` by value, so the thunk must FORWARD the
aggregate rather than copy a dword. That is `CtRemove` beside `CtUpdate`.

`Track_Remove` never reads its square (it tears down the one track node the
module remembers), so nothing was wrong there; it still takes the slot's shape,
because the sweep cannot tell "ignores it" from "misreads it" and a gate that
has to be argued with is not a gate.

### 1c. The two the sweep could not see either

Both fixed in the sweep itself, before the source:

* **`union BPosW { unsigned short w; BPos b; }`.** The old `struct_info` gave up
  on a nested aggregate and returned `None`, which put every `BPosW` spelling of
  a map square in the "size unknown" bucket instead of the silent window. It is
  2 bytes — squarely in the window. Resolving nested aggregates recursively
  moved eleven sites into the silent list, including all of
  `StandardRemoveObject`'s and `RemoveAllBlokesFromRide`'s `BPosW` callers.
* **the preprocessor.** The class only exists in the portable build, so only the
  lines that build compiles count, and an `#ifndef LEGOLAND_PORTABLE` arm is VC6
  text. Without this the sweep can never reach 0 — the fix for this class is
  always a portable arm BESIDE the matched spelling, so every fix reads as a
  fresh hit, and M10's own three fixes (`AddObjectToBuildList`,
  `FreeBuildSlotAt`, `GetBuildAnimFrame`) were still being reported as live. The
  sweep now also follows `#define X X_vc6_body` and uses the WRAPPER's signature,
  which is what the rest of the portable link sees.

The sweep exits 1 when either live section is non-empty, so it can be a gate.

### 1d. Every silent site, and what happened to it

21 before, 0 after.

| address | what was wrong | fix |
| --- | --- | --- |
| `0x0045f220` `StandardRemoveObject` | 7 scalar declarations against a `BPos` definition and a `BPos` slot | portable arms in goldrush.c, joust.c, ridecb9.c, screencb.c (both `StandardRemoveObject` and `StandardRemoveObject_W`), screencb6.c, sweep3.c, westtown.c |
| `0x0048a2e0` `RemoveAllBlokesFromRide` | 5 scalar declarations against `rides.c:400`'s `RideTile` | the same five files |
| `0x0045efc0/d0` `ApplyConsTileMap`/`ApplyDestrTileMap` | sweep2.c's portable forwarder took `(int, int)` where objmap2.c passes `(MapObj*, BPos)` — and `StandardRemoveObject` is the caller | the forwarder carries the caller's shape (it reads neither argument, so nothing was broken; see `Track_Remove` above for why it changed anyway) |
| `0x00401f30/0x00402150` `SchoolCarNextManoeuvre{,Horn}` | goldrush.c `unsigned short` vs schoolcar2.c's `BPosW` | goldrush.c portable arm |
| `0x00412650` | coaster.c:1515 calls it `GetSchoolRecord(int)`; the definition is `Road_FindStartPiece(BPosW)` (ridecb6.c:966) — **a name disagreement as well**, recorded in §5 | coaster.c portable arm |
| `0x0041a530`, `0x0041b6f0`, `0x0041c130` the three boating-school removes | loaders.c declares all three `unsigned int` (address-taken only, stored into `cb_9c`) | loaders.c portable arm |
| `0x0041b0d0`, `0x0041caa0` `BoatingSchool_AddTake_I`, `_RebuildRouteI` | ridecb6.c's deliberate `int` aliases of its own `BPosW` definitions | ridecb6.c portable arm |
| `0x0042fb00/0x0042fb60` `Restaurant2_Start/StopSound` | ridecb3.c `unsigned short` vs audio5.c's `BPosW` / ridemisc4.c's `RideTile` | ridecb3.c portable arm |
| `0x0043d7c0` `MechanicsHut_EvictRiders` | ridecb9.c `unsigned int` vs ridemisc.c:93's `BPosW` | ridecb9.c portable arm |
| `0x004373c0` `JungleCruise_RebuildRoute` | **the other way round**: the definition is scalar (junglecruise.c:340 `int id`, and it masks with `(unsigned short)` itself), two of the three declarations agree, and ridecb2.c:247's `BPosW` is the outlier | ridecb2.c's declaration moves to the scalar, unpacking at the call |

The 6 in the NOISY bucket that remain are all function-POINTER parameters
(`RombergFn`, `IconInputFn`, `SpriteDrawFn`, `ShopTile` at `0x00439c90`), where
the arity differs and wasm-ld already warns; they are not this class.

### 1e. Proof: the ride comes off the map

Replayed on the committed build, `?args=-nointro+WINDEBUG&trace=1&tracegrep=DSOUND`
served on 8810, B10's walk to the built Space Tower, then the toolbar's DEMOLISH
(the eraser, game (199, 444)) and a click on the ride (game (327, 210)):

| reading | before the click | after |
| --- | --- | --- |
| `editState` | 0 → **2** when the eraser is armed | 2 |
| `llPark().money` | 1000 | **1040** — the salvage refund, `AddBricks(GetObjSalvageValue(...))` at objmap2.c:996 |
| `llPark().visitorLimit` | 4 | **3** — the ride's capacity came back off |
| `llPark().numVisitors` / `people` | 4 / 4 | **3 / 3**, still equal |
| the map | the Space Tower on its pad | **grass** — pad, tower and footprint all gone, the loader's path untouched |
| `llStats()` | — | **`dead` null, `traps: []`**, 34.0 fps, 4 521 frames |
| the remove sample | — | the 33 792-byte 44 100 Hz buffer **played** (`-> AUDIBLE`) |

That last row is both halves of this lane at once: `g_remove_sample` is an
interior alias of `g_game_fx`, so it only has a sample at all because of §2.

---

## 2. The unbounded-table class

### 2a. The mechanism, and why the gate was blind

`extern FXEntry g_game_fx[];` with no bound gives `cdecl.py` nothing to compute
an extent from (`full_type` returns `None` when `count is None`), so
`gen_link.py`'s pointer scan skips the object outright (`if ty is None or not
cdecl.has_pointer(ty): continue`) and every pointer word inside it stays a raw,
unrelocated Win32 VA. `gen/pointers.md` reports "raw pointer words: 0"
truthfully, because it counts only the words the scan VISITED — so a whole class
sat below it. PORT-B11 §3 found it and measured it; this closes it.

### 2b. The bounds, each from the count the game's own code passes

| declaration | bound | evidence |
| --- | --- | --- |
| `g_game_fx[]` mapinit.c:18, loaders.c:318 | `[0x17]` | `Load_FXList(g_game_fx, 0x17)` mapinit.c:44 |
| `g_map_fx[]` lifecycle.c:33 | `[23]` | `Kill_FXList(g_map_fx, 23)` — the same address |
| `g_money_fx[]` audiomisc.c:179, money.c:176 | `[2]` | audiomisc.c:187/195 |
| `g_joust_fx[]` joust.c, joust2.c, narration2.c | `[1]` | joust.c:707/753 |
| `g_entrance_fx[]` screencb5.c:424 | `[1]` | screencb5.c:446/464 |
| `g_plane_fx[]`, `g_safari_fx[]`, `g_rest2_fx[]` ridemisc4.c | `[2] [1] [3]` | mechrides.c:1006/950, screencb.c:265 |
| `g_spacetower_fx[]`, `g_spider_fx[]` ridemachine.c | `[1] [1]` | mechrides.c:1144/1058 |
| `g_power_table[]` power.c:83 | `[66]` | measured in the image, below |
| `g_near_offsets[]` workorder2.c:298 | `[12]` | `FindBrokenCellNear`'s own loop, workorder2.c:914 |
| `g_gfx_dirs[7]` rin.c:244 | `[8]` | word 7 was a raw VA to `".\graphics\textures\"` |

`g_power_table`'s 66 is measured, not guessed: `[64]` is `"Miniland Denmark"`,
`[65]` is `{0x004d8bb0, 0}` — a zeroed `.data` buffer, i.e. the empty-string
terminator the walk at power.c:142 stops on — and the next named object,
`g_dir_bit_table`, starts at `0x004b9550` = `0x004b9340 + 66*8`.

A bound on an extern that is only ever INDEXED is not a codegen lever: there is
no `sizeof` and no whole-array arithmetic at any of these sites, `audit.py`'s
rows are byte-identical to base at every one, and `relocs.py` is clean — so both
builds take the same line and none of this needed a portable arm.

### 2c. What the closure did with them

| object | before | after |
| --- | --- | --- |
| `g_game_fx` `0x004b9228` | 12 fragments, 3 raw name words, and `Load_FXList`'s stride walked off the end of the first fragment after entry 2 | **ONE 280-byte object**, 14 interior aliases (`g_place_sample`, `g_remove_sample`, `g_snd_close`, `g_snd_click`, `g_snd_fp_remove`, `g_snd_denied`, `g_snd_fp_denied`, `g_sample_hire`, `g_clear_sfx`, `g_sample_fire`, `g_snd_theme`, `g_sample_gardener`, `g_sample_mechanic`, `g_snd_btnflash`), **all 23 names re-pointed** |
| `g_power_table` `0x004b9340` | 63 raw name words | 528 bytes, all re-pointed |
| `g_plane_fx` `0x004b79d0` | 1 raw word in a 184-byte tile | 24 bytes — and `g_zoomer_loop_sample` (ridesave.c:789) turns out to be `g_plane_fx[0].sample` at `0x004b79d8`, an interior alias now |
| `g_money_fx`, `g_joust_fx`, `g_gfx_dirs` | 2 + 1 + 1 raw | 0 |

```
raw pointer words (declaration-independent scan)   122 in 18 objects  ->  31 in 1
gen/pointers.md declared pointer words 7659, in no emitted block 0, raw 0
```

### 2d. The 31 that remain, and why they are not ours

All of them are in the 4 524-byte gap-tiled block behind `g_near_offsets`: the
CRT's `_matherr` name table (`"exp"`, `"pow"`, `"log10"`, `"sinh"`, …). No
LEGOLAND declaration covers them, nothing in this port calls `_matherr`, and the
bound did not shrink the emitted object because the next NAMED symbol is that far
away — only `gen_link` can name what is in between. **PORT-A**, and the reason is
written into workorder2.c beside the bound.

### 2e. Proof: THE SOUND EFFECTS LOAD, AND THEY PLAY

`?args=-nointro+WINDEBUG&trace=1&tracegrep=DSOUND`:

```
at startup, before the front end draws:
  HOST DSOUND CreateSoundBuffer  x22       (B11 measured 0 or 1)
  4882 / 44104 / 33792@44100 / 141290 / 129794 / 141008 / 265004 / 172068 /
  5524 / 9480 / 4278 / 10120 / 12220 / 11712 / 40480 / 48132 / 48132 /
  24288 / 26312 / 54648 / 44528 / 27048 bytes
```

**22 of the 23.** The 23rd is `Drilling.wav`, which B11 §3 established is
genuinely absent from `Legoland.res` — the shipped game failed to load it too.

Over the walk, with `llAudio()` read at each step:

| step | `plays` | `voices_created` |
| --- | --- | --- |
| front end, first clicks | 3 | 3 |
| the park, first popup closed | 22 | 22 |
| the LEGOLAND menu | 31 | 31 |
| the Space Tower BUILT | 37 | 37 (incl. the first `44100Hz/1ch/16bit`) |
| the ride DEMOLISHED | 96 | 96 (the second 44 100 Hz) |
| end of session | **108** | 108 |

`plays_blocked: 0`, `refused: 0`, `underruns: 0`, `state: "running"`, and the
trace lines read `HOST DSOUND Play … -> AUDIBLE`. The page's own readout says
`sound running, 108 played`.

---

## 3. `0x00832bd0` is `g_power_supply`, not `g_num_visitors`

`appraisalscreen.c:149` was the only file in the tree that used that name.

* `power.c:170` is the only WRITER in the game: `g_power_supply += power`, once
  per generator the power sweep walks; `scrolltick.c:197` takes it back off.
* `power.c:128`, `powerhelp.c:100`, `fpui2.c:198` and `scrolltick.c:113` all
  declare the same address `g_power_supply`.
* the appraisal line that read it (string `0x14b`) sits in section 7 beside the
  rides-RUNNING count (`0x14c`), not in section 6 with the visitor mood and age
  bars (`0x148`/`0x149`).

The real count is `g_visitor_count` at `0x006661bc`: `pathobj2.c:458` zeroes it
on load, `rides.c:161` and `goalstate.c:467` move it as blokes arrive and leave,
`eventtick2.c:251` is what the PARK VISITORS goal tests.

So `llPark().numVisitors` has been reading the park's power supply, which is why
it stayed 0 through B10's and M10's whole tutorial walk whatever the park did.
That is the half that needed the page to move with the rename, and why M10
recorded it rather than doing it: `gen_link.py` only emits an alias for a name
some TU declares, so dropping `g_num_visitors` from appraisalscreen.c without
moving `LL_DBG_TABLE` slot 62 and `index.html`'s `llPark()` would have broken the
browser link. **Both move here** — `LEGOLAND/appraisalscreen.c` (both builds; an
identifier is not a codegen lever and the audit rows are byte-identical),
`portable/src/browser/main.c` slot 62, `portable/src/browser/index.html`.

**It works.** `llPark()` in the park now reads `numVisitors: 3, people: 3`; after
the ride is built, `4 / 4`; after it is demolished, `3 / 3`. It was `0` at every
one of those points before.

*(This is the only `portable/**` edit in this lane, and it is the one the brief
allows. PORT-A9 owns `portable/tools/**`; nothing there was touched.)*

---

## 4. Gates

| gate | result |
| --- | --- |
| `audit.py` on all 29 touched `LEGOLAND/*.c` | see the table below — **every row byte-identical to base**, 0 REJECT / FAIL / COMPILE FAILED |
| `relocs.py` on all 29 | **0 MISMATCH** |
| `progress.py --check` | **3281 exact / 42 WIP** |
| `portable/tools/extern_sweep.py` | clean — "the class is closed" |
| `tools/port_m10_bvstruct_sweep.py` | **silent 0, slot 0** (21 and 22 before) |
| raw pointer words, declaration-independent | **122 → 31**, all CRT (§2d) |
| `gen/pointers.md` | declared 7659, in no emitted block 0, **raw 0** |
| wasm: `emcmake cmake` + `ninja` + the five extra targets | exit 0, no wasm-ld signature warning, "imported with more than one signature: 0" |
| wasm `ctest` | **20/20** |
| native: `cmake` + `ninja` + `legoland_tests legoland_cbtypes` + `ctest` | **14/14** |
| the page, 8810 | 4 521 frames, **`traps: []`, `dead` null**, 34.0 fps, 99.0% non-black |

---

## 5. Open, for the integrator

1. **`0x00412650` has two names.** `ridecb6.c:966` DEFINES it
   `Road_FindStartPiece(BPosW school)`; `coaster.c:1515` declares it
   `GetSchoolRecord(int school)` and `coaster.c:1528` uses the result as a
   `SchoolRec*` with an entry square at +0x0c/+0x10. Both readings are
   defensible from the call sites and this lane did not have the disassembly
   budget to settle it, so only the ABI was fixed (coaster.c's declaration now
   takes the aggregate). A PORT-M naming pass should decide it — the same shape
   as M2's `NewScriptEvent` and M5's five.
2. **`0x0041b6f0` likewise**: `screencb.c:1470` defines `BsMermaid_Remove`,
   `ridecb8.c:374` declares `Mermaid_Remove`, `loaders.c:196` the former.
3. **The 31 CRT raw words** (§2d) are `gen_link`'s, not a declaration's.
4. **`tools/port_m10_bvstruct_sweep.py` belongs in the round gate**, as M10 said
   — and now with a non-zero exit status so it can be one. It is still the only
   thing in the tree that can see either direction of this class.
5. **The pointer-bearing unbounded arrays this lane did NOT bound** are the ones
   whose element counts are not visible in the source — they are indexed by
   runtime values (tile codes, bloke states, event kinds): `g_tile_info` /
   `g_tile_recs` / `g_tile_slots` / `g_sp_tile_info` (all `0x00801f40`),
   `g_text_cache`, `g_track_pieces`, `g_track_cursors`, `g_castle_parts`,
   `g_castle_foot_elems`, `g_cursor_sprites`, `g_pieces`, `g_bloke_anim_ref`,
   `g_tower_seat_anim`, `g_station_geometry`, and the three function-pointer
   tables `g_event_tick` / `g_lowlevel_ai` / `g_lt_action_handlers`. **None of
   them carries a raw pointer word today**, so guessing a bound would have been
   risk without measurement. Listed here so the next lane does not have to
   re-derive the list.
6. **`portable/hostwin/include/ll_portable.h` is force-included into every
   `LEGOLAND/*.c`.** Four of this lane's files had to define their own 2-byte
   square typedef in a portable arm because they had none; a single
   `LL_SQUARE`-shaped helper there would make the next instance of this class a
   one-line fix. `portable/**` is not this lane's to edit.

---

## 6. Files touched

| file | change |
| --- | --- |
| `tools/port_m10_bvstruct_sweep.py` | preprocessor-aware, nested aggregates resolved, the SLOT vs BODY section, non-zero exit |
| `goldrush.c` | `StandardRemoveObject` + `RemoveAllBlokesFromRide` + both `SchoolCarNextManoeuvre`s; 4 remove bodies |
| `joust.c` | the same two callees; `Joust_Remove`, `TempleSlide_Remove`; `g_joust_fx[1]` |
| `ridecb9.c` | `StandardRemoveObject`, `MechanicsHut_EvictRiders`; 2 remove + 2 draw bodies |
| `screencb.c` | `StandardRemoveObject`, `_W`, `RemoveAllBlokesFromRide`; `DrivingSchool_Remove` |
| `screencb6.c` | the same two callees; `FoodService_Remove`; 2 draw bodies |
| `westtown.c` | the same two callees; `Shop_Remove`, `JailCell_Remove` |
| `castleobj.c` | `CtRemove`; `CtThunk_Remove` forwards the aggregate; `Track_Remove`; 2 forward declarations |
| `ridecb8.c`, `screencb2.c` | 3 + 1 draw bodies |
| `sweep2.c`, `sweep3.c` | the `ApplyTileMap` forwarders; the +0x9c slot declaration |
| `coaster.c`, `loaders.c`, `ridecb2.c`, `ridecb3.c`, `ridecb6.c` | the remaining silent declarations |
| `mapinit.c`, `lifecycle.c`, `audiomisc.c`, `money.c`, `joust2.c`, `narration2.c`, `screencb5.c`, `ridemisc4.c`, `ridemachine.c`, `power.c`, `workorder2.c`, `rin.c` | array bounds (§2b) |
| `appraisalscreen.c` | `g_num_visitors` → `g_power_supply`, both builds |
| `portable/src/browser/main.c`, `index.html` | the paired half of that rename |

No `docs/HANDOFF.md`, no `portable/tools/**`, no `tools/verify.py` run.
