# Scope PORT-M10 — the park's game-side defects: PARK-1, PARK-3, PARK-2

> **PORT-M10 — Status: IN PROGRESS (claimed 2026-09-12 by PORT-M10)** — branch
> `scope/PORT-M10`, cut from the PORT-B10 merge (`7a665353`). Matching side:
> `LEGOLAND/*.c` under `#ifdef LEGOLAND_PORTABLE`, VC6-gated per file.
> `portable/**` and `docs/HANDOFF.md` are READ-ONLY for this lane (PORT-B11 is
> running in parallel on the shim). Brief: `docs/SCOPE_PORT_WAVE.md`.

**Result in one line: PARK-1 and PARK-3 are one bug, and it is a NEW blocker
class that every gate in this wave is blind to.** A by-value struct parameter
that one TU spells as a struct and the defining TU spells as a scalar is the
same dword on x86 cdecl and a **pointer** on wasm32 — and because both lower to
exactly ONE `i32` parameter, **wasm-ld reports no signature mismatch,
`linkreport.py`'s conflict vote sees nothing, and nothing traps.** The callee
silently reads the low half of a shadow-stack address as its argument.

Also here: PORT-A8's A8-1 closed (wasm signature conflicts tree-wide **29 → 0**,
not the 11 A8 counted), PORT-M9's M9-3 decided, PARK-2 handed to PORT-B11 with
a reading, and one wrong global name that made PARK-1 look worse than it was.

---

## 1. PARK-1 and PARK-3 — one root cause

### 1a. What the game's own memory said

PORT-B10 reported that `LINK "SPACE TOWER RIDE"` never completes however much
path is laid. Replaying B10 §1's walk on
`legoland.html?args=-nointro+WINDEBUG&beat=1000` (served on 8807) and reading
the game's globals through `llAddrs()` / `ll_dbg_addr`, plus a memory scan for
the records that are not in `LL_DBG_TABLE`:

| what | value read | expected |
| --- | --- | --- |
| the Space Tower's class record (found from `g_odf_head`, named by the `LLElem` that points at it) | `type` 1 (linkable), `entrance` (+0x0c) = **(+2, −3)**, footprint (+0x3c) = (−2,−2,4,3) | — |
| its 42 footprint cells (`g_map_rows`) | base `bx,by` = **(63, 35)** on every one | (63, 35) |
| **its instance record** (`ObjDef +0x04` → `ObjInst +0x0e/+0x0f`) | **(114, 10)** | (63, 35) |
| `LEGO SHOP 1`'s instance record — placed by the LEVEL LOADER, not by the player | **(74, 61)** — correct | (74, 61) |
| its `TowerRec` (the heap block malloc'd straight after the `ObjInst`) | `tile` = **0x0a72** = (114, 10) | 0x233f |
| the 11 path squares (`0x0066b44c` chain, located by scanning for plausible rects) | the ride's pad `[60,32,68,32]`, `[60,33,60,39]`, `[68,33,68,39]`, `[61,39,67,39]` **all carry flag 2** (reachable from the entrance) | flag 2 |
| the live goal events (`ScriptEvent`, `kind` at +0x0c) | kind 33 `NEED` + **kind 37 `LINK` for `SPACE TOWER RIDE`**, `flags` 2, at the END of the pending list | — |

So the path network, `ResolveEntrancePathSquare`'s flood fill, `FindPathSquare`
and the goal event were all **correct**, and the only wrong number in the park
was the ride's own map square, in the two records that are written when it is
BUILT.

`EventTick_Link` (eventtick.c:938, 0x0046a750) computes the link square as
`inst->x + d->dx`, `inst->y + d->dy`. Checked against the disassembly at
`0x0046a77c`–`0x0046a78e` (`mov cl,[esi+0xe]` / `mov dl,[esi+0xf]` / the two
adds / the `[0x4bcbf4]` bounds test): the recovered C is exact. With the
instance record reading (114, 10) the link square is (116, 7), `CellAt` is off
the 84×84 map, the `outside` arm fires and the handler returns 0 **for ever** —
`TileJoinsPathNetwork` is never even reached, which is why no amount of path
helped.

**`goalstate.c`, `mappath.c`, `pathmisc.c`, `pathmisc2.c`, `pathsq.c`,
`objmap.c`, the route search, PORT-M6's un-aliased open/closed lists, PORT-M5's
fistp sites and the renderer are all innocent.**

### 1b. The root cause: a silent wasm32 by-value-struct ABI mismatch

`AddObjectToBuildList` (0x00450b90) records the map tile of an object that is
still under construction. Two TUs disagree about its second parameter:

* `LEGOLAND/popup.c:122` declared `extern int AddObjectToBuildList(ObjDef* d, BPos bp);`
  — `BPos` is `struct { unsigned char x, y; }`, **by value**. popup.c:30 already
  recorded that this reading is right: the parameter *is* the packed map tile.
* `LEGOLAND/sweep1.c:302` **defines** it as `int AddObjectToBuildList(int obj, short type)`
  and stores `g_buildlist[i].type = (unsigned short)type;`.

On x86 cdecl those are the same dword on the stack. That is why `audit.py`,
`relocs.py` and every byte gate have always been green on both files, and why
this survived PORT-M1, M2, M4, M5, M6, M7, M8, M9 and A8.

On **wasm32 they are not the same.** clang passes a by-value struct *directly*
only when it is a **single-element** struct; a two-member 2-byte struct goes
**indirect** — a pointer to a byval temp on the shadow stack. Both spellings
still lower to exactly one `i32` parameter, so:

* `wasm-ld` sees `(i32, i32) -> i32` on both sides and warns about nothing;
* PORT-A8's `wasm_sig_conflict_detail` cannot see it — there is no conflict;
* `test_callback_types.c` cannot see it — it type-checks slots, not parameters;
* nothing traps. `sweep1.c` just stores the low 16 bits of a stack ADDRESS.

Measured with a three-file reproduction (`/tmp/port-m10-abi/`, emcc -O2 against
native clang -O2, the real popup.c declaration against the real sweep1.c body):

```
native clang : slot0 type = 0x233f   (x=63 y=35)   correct
emcc wasm32  : slot0 type = 0x132c   (x=44 y=19)   a shadow-stack pointer
```

and in the live game the value is `0x0a72` → (114, 10), stable across runs and
identical in the optimised `legoland.html` and the `-g2` `legoland_dbg.html`.

**The size boundary, measured** (`sz_call.c` / `sz_def.c`, same harness):

| struct | wasm32 | wasm-ld | verdict |
| --- | --- | --- | --- |
| `{u8}` — one member | direct, value arrives | — | safe |
| `{int}` — one member | direct, value arrives | — | safe |
| `{u8,u8}` 2B | **indirect**, a pointer arrives | **silent** | **THE CLASS** |
| `{u8,u8,u8}` 3B | **indirect** | **silent** | **THE CLASS** |
| `{u8,u8,u8,u8}` 4B | **indirect** | **silent** | **THE CLASS** |
| `{int,int}` 8B (`Pos`) | indirect | **warns** (arity 1 vs 2) | already gated |
| `{short,short,short}` 6B | indirect | **warns** | already gated |

So the silent window is exactly: **a multi-member aggregate of 4 bytes or
fewer**, because that is the only size that occupies one dword on x86 *and* one
`i32` on wasm32. Anything bigger changes the wasm arity and wasm-ld already
shouts; anything single-member is passed direct and is safe.

### 1c. The chain from there is mechanical

A ride carries class flag 0x80000, so `BuildObject` (popup.c:173, 0x0045eb30)
puts it on the 256-entry construction list instead of placing it at once, and
its map tile travels through the build slot:

```
BuildObject            bp = (63,35) from *pos      -> AddObjectToBuildList(def, bp)
                                                      slot.key := 0x0a72   <-- CORRUPTED
  AddObjectToMapByCursor(obj, pos, 0x20)           -> cells stamped (63,35)  (correct: it uses *pos)
buildtick.c tick       ObjectIsBuilt(slot.obj, slot.key.tile)
  ObjectIsBuilt        pos.x = tile.x; pos.y = tile.y  -> (114,10)
    PutObjOnMap(obj, obj->cls, &pos)
      cls->place(obj, pos) = SpaceTower_Add   (mechrides.c:299 SpaceTower_Place)
        tile.b = (114,10) -> SpaceTower_AddRecord   TowerRec.tile := 0x0a72
        AddBasicObject(obj, pos, 0)
          AddObjectToMap(obj, bp=(114,10), 0)    -> every cell off-map, SILENTLY skipped
          CreateObjectInstance(def, &key=(114,10)) -> ObjInst.key := 0x0a72
```

Two details made this invisible from the frame:

* the footprint cells are stamped by `AddObjectToMapByCursor` (mappath.c:575)
  from `*pos`, which is still correct, so **the ride looked perfectly placed**;
* `AddObjectToMap` (objmap2.c:409) guards every cell with `MapCellAt` and skips
  an off-map one without a word, so the second, wrong stamp writes nothing.

**PARK-3 is the same bug.** `SpaceTower_FindRecord` (ridemisc4.c, 0x0043ac40)
looks its `TowerRec` up by the cell the ride stands on; with the record filed
under (114, 10) the tower's own draw callback could never find it and drew its
base alone — B10's "squat block, about 2 tiles across". After the fix the tower
draws as a tower (§4's screenshot).

### 1d. The fix

popup.c's declaration is the side that moves: sweep1.c's body is the
definition, and 0x00450b90 has exactly one caller. Under
`#ifdef LEGOLAND_PORTABLE` the parameter is declared the way the definition
reads it and the tile is packed at the call site — the shape the tree already
uses for `AddBasicObject`'s third argument:

```c
#ifndef LEGOLAND_PORTABLE
extern int   AddObjectToBuildList(ObjDef* d, BPos bp);          /* 0x00450b90 */
#else
extern int   AddObjectToBuildList(ObjDef* d, unsigned short bp); /* 0x00450b90 */
#define AddObjectToBuildList(_d, _bp) \
    AddObjectToBuildList((_d), (unsigned short)((_bp).x | ((_bp).y << 8)))
#endif
```

`BPos.x` is at +0x00 and `.y` at +0x01, so little-endian `x | (y << 8)` is the
same 16 bits `sweep1.c` stores and the same 16 bits `buildtick.c`'s `BuildKey`
union reads back as `{x, y}`. VC6 never sees any of it.

### 1e. Two more of the same class, fixed with it

`tools/port_m10_bvstruct_sweep.py` (new, this lane) sweeps every declaration
and definition in `LEGOLAND/*.c` for the pattern and reports the silent window
separately from the window wasm-ld already covers. It found three live ones:

| address | declared (by-value struct) | defined (scalar) | what it broke |
| --- | --- | --- | --- |
| `0x00450b90` `AddObjectToBuildList` | popup.c:122 `BPos` | sweep1.c:302 `short` | **PARK-1 + PARK-3** |
| `0x00450c00` `FreeBuildSlotAt` | mappath.c:421 `BPos` | pathmisc.c:99 `unsigned short` | the scan compares a stack address against every slot key, matches nothing, so a cancelled build's slot leaks and `g_build_count` is never decremented |
| `0x00450cf0` `GetBuildAnimFrame` | renderview.c:353 `BPos` | buildtick.c:175 `short` | the same scan fails, the index is left at 256 and `g_build_slots[256].timer` is read PAST THE END of the table, so an object under construction draws at an arbitrary build-animation frame |

All three are fixed the same way, in the declaring file's portable arm.

### 1f. Two more of the class that are NOT fixed here — for the integrator

The same sweep reports two more, both in the *opposite and more dangerous*
direction (the **definition** takes the struct, so the body will DEREFERENCE
whatever arrives, and most callers hand it a plain integer):

| address | defined (by-value struct) | declared scalar by | why it is not fixed here |
| --- | --- | --- | --- |
| `0x0045f220` `StandardRemoveObject` | objmap2.c:983 `(MapObj*, BPos, Cursor*)` | goldrush.c:243, joust.c:555, ridecb9.c:886, screencb.c:712 (`StandardRemoveObject_W`), screencb6.c:38, sweep3.c:130, westtown.c:179 — and input2.c:218 / mechrides.c:425 spell it `CellPos`, coaster.c:320 `MapPos` | ten files and the `+0x9c` slot type move together; it is the standard remove handler, so **every object removal in the game reads memory at a tiny address instead of a square** |
| `0x0048a2e0` `RemoveAllBlokesFromRide` | rides.c:400 `(RideObject*, RideTile)` — `RideTile` is a 2-byte UNION, also indirect | goldrush.c:244, joust.c:556, screencb.c:713, screencb6.c:39, westtown.c:180, mechrides.c:426 (`CellPos`) | same shape, six files |

Neither is on PARK-1/2/3's path and neither is exercised by the tutorial, so
this lane recorded them rather than rushing a ten-file change it could not
fully replay. **The recommended fix is one point, not ten:** give the
DEFINITION a portable-arm scalar parameter with PORT-M3's rename trick
(`#define StandardRemoveObject StandardRemoveObject_vc6_body` before the
marker, then a wrapper of the slot's own shape exported over it), which makes
the majority spelling and the slot type correct in one file; then only
input2.c, mechrides.c and coaster.c (and mechrides.c for the second) need a
portable arm. **Run `tools/port_m10_bvstruct_sweep.py` in the round gate** —
it is the only thing in the tree that can see this class.

---

## 2. PORT-A8's A8-1 — wasm signature conflicts 29 → 0

A8 attributed 11; the live count on this build is **29** (PORT-M9's merge added
declarations of the same shape). All 29 are `LEGOLAND/*.c` declarations that
disagree with the object defining the body; all are address-taken only, so
nothing traps today. Measured with A8's own detector —
`linkreport.wasm_sig_conflict_detail` over every object of the browser link,
with `legoland_cbtypes.dir` / `test_callback_types.c` excluded (that TU
deliberately declares every body, so including it turns the count into 275).

**Cluster A — 23 in `screen.c`.** The interface table declares these class
callbacks with an EMPTY parameter list (`extern void Foo ();` — K&R, not
`(void)`), which leaves the symbol with an empty wasm signature.

A K&R declaration is **completed**, not contradicted, by a later prototype with
the same return type, so 22 of them are closed by one block of completing
prototypes in a portable arm at the end of the file — **not one matched line is
touched**, and parameters are spelled `void*`/`int` (the `i32` the real types
lower to) because no call is made through them here. The 23rd,
`BoatingSchool_BestTake`, returns `int` (ridecb6.c:1188), so its return type has
to be swapped rather than completed and it gets its own `#ifndef`/`#else`.

| symbol | body | defined in |
| --- | --- | --- |
| `AddBasicPath` | 2 args | pathbuild.c:135 |
| `RemoveSoundObject` | 3 | input2.c:372 |
| `BoatingSchool_Create` / `_Destroy` | 1 / 1 | screencb.c, screencb2.c |
| `BoatingSchool_Add` | 2 | ridecb5.c |
| `BoatingSchool_Update` | 3 | screencb3.c |
| `BoatingSchool_DrawSelection` | 2 | screencb2.c:1436 |
| `BoatingSchool_Remove` | 3 | ridecb8.c:381 |
| `BoatingSchool_Draw` | 6 | screencb.c |
| `BoatingSchoolWater_Remove` | 3 | ridecb5.c |
| `BoatingSchool_BestTake` | 2, returns `int` | ridecb6.c:1188 |
| `Mermaid_LoadResources` / `_Add` / `_CalcCursor` / `_CalcCursor2` | 1 / 2 / 3 / 2 | ridecb8.c, ridecb6.c |
| `BsMermaid_Remove` | 3 | screencb.c |
| `BsWater_LoadResources` / `_Add` / `_CalcCursor` / `_DrawSelection` | 1 / 2 / 3 / 2 | ridecb8.c, ridecb6.c, screencb.c |
| `MonkeyTree_Remove` / `MonkeyFish_Remove` | 3 / 3 | junglecruise.c:442/505 |
| `JungleCruise_DrawSelection` | 2 | ridecb9.c:506 |

**Cluster B — PORT-M8's icon-input class at the sites M8-4 did not list.**
`bigscreens.c` `MapIconInput` + `OptionsIconInput` and `bighelp.c`
`PU_CloseInput` / `PU_NextInput` / `PU_Delete2Input`: all four-argument in their
bodies (`screens3.c:707`, `uimisc.c:291`, and `uimisc.c:2` / `fpui5.c:300`'s
`_vc6_body` rename exporting the slot's shape), all already CALLED
four-argument from `uimisc3.c:372/932` and `movie.c:263/375/410`, and
`bigscreens.c:52` already types the `Icon +0x2c` slot four-argument. Swapped in
the same `#ifndef`/`#else` shape M8 used for the progress-screen icons twenty
lines above.

**Plus one**: `interfaces.c:1594` declares `LFHoldUp_Remove(void)` against
`logflume.c:926`'s three arguments — the `cb_remove` slot's own shape, which
every other remove handler in that table already carries.

**Verified: 29 before, 0 after.** Note for the integrator: the pairs cannot be
added to `portable/tests/test_callback_types.c` from here — `portable/**` is
read-only for this lane while PORT-B11 is running.

---

## 3. PARK-3 — the Space Tower's verdict

**Same root cause as PARK-1 (§1c), and fixed by the same one-line change.** It
is not a rounding site, not a transposed texel formula and not a callback
signature: the ride's `TowerRec` was filed under an off-map square, so
`SpaceTower_FindRecord` could never return it and the tower's draw callback had
nothing to draw above the base.

Proof: §4's screenshot. The tower stands on its pad with its striped column,
the ride's canopy and its car, at the height the side-panel icon shows.

`GetBuildAnimFrame` (§1e) is a second, independent defect on the same drawing
path — it is what decides the frame a building draws while it is still under
construction, and it was reading past the end of `g_build_slots`. It is fixed
here too, but it is not what PARK-3 was.

---

## 4. Proof: the tutorial now advances past objective 2

Replayed from a virgin IDBFS on the committed build. The front end is
unchanged — the tutorial-select hash is `0xe59fe2c4`, B10's value to the digit.

| step | reading |
| --- | --- |
| the park | money 1000, people 3, visitorLimit 3 |
| LEGOLAND menu -> Space Tower -> BUILD IT | money **1010 -> 970** (the ride costs 40) |
| the ride standing | people **4**, visitorLimit **4** |
| **the Space Tower's `ObjInst` key** | **(63, 35)** — was (114, 10) | 
| `LEGO SHOP 1`'s `ObjInst` key | (74, 61), unchanged |
| **the `LINK "SPACE TOWER RIDE"` goal event** | `flags` **2 -> 0x82** and the pending list has advanced PAST it — was `flags` 2 at the end of the list, for ever |
| **the mini-notepad** | *"Now you need to make a path so that the visitors can get to the shop. Click the flashing PATH button then click to make a path."* — that is **objective 3**, the LEGO SHOP link. B10's stuck prompt, *"link the Space Tower Ride to the main path"*, is gone. |
| the Space Tower on the map | drawn as a TOWER (screenshot) |
| both of B10 §1's path drags | still fine, `editState` 1, no trap |
| the session | ~2 750 frames, `traps: []`, `dead` null, 32–35 fps |

**On `llPark().numVisitors`: it is not a visitor count.** `g_num_visitors`
0x00832bd0 is a WRONG NAME — `appraisalscreen.c:149` is the only file that
spells it that way, and `power.c:119/161/193/196` (the only WRITER:
`g_power_supply += power`), `powerhelp.c:100`, `fpui2.c:198` and
`scrolltick.c:113` all call the same address `g_power_supply`, the park's total
power generation. It is 0 in the tutorial because there is no generator, and it
would have stayed 0 whatever happened to the LINK goal. The real count is
`g_visitor_count` at 0x006661bc (goalstate.c:178, pathobj2.c:136, rides.c:161,
simcore2.c:27, eventtick2.c:72).

**This lane did not rename it**, because `portable/src/browser/main.c`'s
`LL_DBG_TABLE` and `index.html`'s `llPark()` both use the name and
`gen_link.py` only emits an alias for a name some TU declares — dropping it
from appraisalscreen.c would break the browser link. It needs a paired change:
appraisalscreen.c `g_num_visitors` -> `g_power_supply`, and
`main.c`/`index.html` swapped to `g_visitor_count` (and `llPark().numVisitors`
renamed, or pointed at 0x006661bc). **Integrator / PORT-B.**

---

## 5. PARK-2 — the verdict: shim side, for PORT-B11

**The bubble has no erase path, in the original or here, and that part is
faithful.** `BubbleHelp` (bighelp.c:296, 0x00455370) and `HTBubbleHelp`
(0x004557c0) only DRAW; the map area under a bubble is repainted every frame,
and the interface panel is repainted only when something marks the interface
dirty (`PrintSprite(g_ci_interface_bg, 0, 0, 0, ctx)` in gameframe.c:603, inside
the interface-rebuild block). `FlipPrimary` blits one persistent back surface,
as B10 says, so a bubble that lay over the panel leaves residue until the panel
is next rebuilt — and that is the original's design, not a port defect.

**The question is upstream, and it is not in the bubble code.** The map bubble
is raised by gameframe.c:617 on `g_hit_type & 0x100` — the hit type the SPRITE
HIT TEST reported for the cell under the cursor — with no viewport rectangle
test anywhere on the path. B10's cursor was at game (577, 419), inside the
interface panel, and the game still answered a map hit type (0x10a,
`GetString(0xd4)`, "Outside your park"). In the original the panel's own
sprites carry hit contexts and a cursor over them yields a panel hit type, so
the map arm is never taken and the bubble is never raised there.

**So the thing to fix is why the panel's sprites do not claim the hit**, which
is `PrintSprite`'s hit-context path in the shim — the same machinery PORT-B6
had to fix for the front-end icons (`ddraw.c`'s live-surface registry, the
12-byte hit record at 0x004bdd00 that PORT-A had to merge). `g_hit_type`
0x00667c5c-family and the `BlitCtx`/`HitInfo` record are the places to look.
**Owner: PORT-B11.** No `LEGOLAND/*.c` change is indicated.

---

## 6. M9-3 — 0x004b5b3c is `g_cmd_write`

PORT-M9 left one address with two defensible names: `coaster12.c:84` called it
`g_span_cursor`, `schoolcar.c:1191` calls it `g_cmd_write`.

`schoolcar.c` settles it. `Coaster3D_EndFrame` (0x00423140) is the CONSUMER: it
walks from `g_cmd_buf` (0x004dd870) to this word, hands each record to
`ZBuffer_RunCommand`, and then **rewinds the word to `g_cmd_buf`** at the end of
every frame. The word is that buffer's write pointer, and it belongs to the
`g_cmd_buf` / `ZBuffer_RunCommand` family that already names the record a
COMMAND. A "span" is what one command CARRIES (the record's first dword is a
count and the payload is `count * 8` bytes), not what the pointer is — and
`g_span_cursor` orphaned the buf/write pair M9 itself had just completed by
declaring `g_cmd_buf` in coaster12.c.

Renamed in coaster12.c for both builds: an identifier is not a codegen lever,
and the audit rows are byte-identical to base.

`0x004dd870` needs no decision — M9 gave it the same name (`g_cmd_buf`) in both
files already.

---

## 7. Gates

| gate | result |
| --- | --- |
| `audit.py LEGOLAND/popup.c` | 2 `[OK]` + `DrawPopUpInfo` `[WIP]` mismatch=13 — **byte-identical to base** |
| `audit.py LEGOLAND/mappath.c` | 4 `[OK]` mismatch=0 — identical to base |
| `audit.py LEGOLAND/renderview.c` | 2 `[WIP]` 381 / 864 — **identical to base** |
| `audit.py LEGOLAND/screen.c` | 3 `[OK]` mismatch=0 |
| `audit.py LEGOLAND/bigscreens.c` | 5 `[OK]` mismatch=0 |
| `audit.py LEGOLAND/bighelp.c` | 3 `[OK]` mismatch=0 |
| `audit.py LEGOLAND/interfaces.c` | 15 `[OK]` mismatch=0 |
| `audit.py LEGOLAND/coaster12.c` | 22 `[OK]` + 2 `[WIP]` 34 / 35 — **identical to base** |
| `relocs.py` on all eight files | **0 MISMATCH** |
| `progress.py --check` | **3281 exact / 42 WIP**, 665/675 exports (report regenerated and committed) |
| `portable/tools/extern_sweep.py` | clean — "the class is closed" |
| `linkreport.wasm_sig_conflict_detail` (browser link, cbtypes excluded) | **29 -> 0** |
| wasm: `emcmake cmake` + `ninja` + the seven extra targets | exit 0, no wasm-ld signature warning |
| wasm `ctest` | **20/20 passed** |
| native: `cmake` + `ninja` + `legoland_tests legoland_cbtypes` + `ctest` | **14/14 passed** |
| the page, `?args=-nointro+WINDEBUG&beat=1000` | ~2 750 frames, `traps: []`, `dead` null, 32–35 fps |

---

## 8. Files touched

| file | change |
| --- | --- |
| `LEGOLAND/popup.c` | `AddObjectToBuildList`'s second parameter: portable arm + packing macro (PARK-1, PARK-3) |
| `LEGOLAND/mappath.c` | `FreeBuildSlotAt`: same class |
| `LEGOLAND/renderview.c` | `GetBuildAnimFrame`: same class |
| `LEGOLAND/screen.c` | cluster A — 22 completing prototypes in a portable arm, `BoatingSchool_BestTake` swapped |
| `LEGOLAND/bigscreens.c` | cluster B — `MapIconInput`, `OptionsIconInput` four-argument in the portable arm |
| `LEGOLAND/bighelp.c` | cluster B — `PU_CloseInput`, `PU_NextInput`, `PU_Delete2Input` |
| `LEGOLAND/interfaces.c` | `LFHoldUp_Remove` three-argument in the portable arm |
| `LEGOLAND/coaster12.c` | M9-3: `g_span_cursor` -> `g_cmd_write`, both builds |
| `tools/port_m10_bvstruct_sweep.py` | **new** — the only thing in the tree that can see the §1b class |
| `docs/LEGOLANDPROGRESS.HTML` | regenerated (line numbers moved) |
| `docs/SCOPE_PORT_WAVE.md`, `docs/lanes/scope-port-m10.md` | status line, these notes |

No `portable/**`, no `docs/HANDOFF.md`, no `tools/verify.py` run.
