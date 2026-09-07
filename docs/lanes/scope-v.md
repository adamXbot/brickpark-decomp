# Scope V — event tick handlers and goal checks: 61 of 62 exact

**Status: in progress, 2026-09-07.** Recovered the interrupted session's
three saved exact fixes (`EventTick_Lookat`, `EventTick_Connect`, and
`EventTick_Link`). A fresh whole-file audit reports 61 exact functions and
one WIP, `EventTick_Clear` (177i/576B versus 177i/577B, 24 strict mismatches
as of the `scope/V-clear` checkpoint at the end of this file; 32 before it).
The two files compile cleanly at `/W3`. Relocation checks resolve 62 positions
in `eventgoal.c` and 125 in the 45 exact `eventtick.c` bodies, all agreeing;
ten eventtick references remain unresolved (literals, a local jump table,
and the annotated function-pointer array that the parser does not recognize).
CLEAR now retains a closer full-body candidate: 120 strict differences reduced
to 32, with the original instruction count and one byte still missing. Its WIP
marker remains. This scope is not complete and has not been integrated.

The three recovered levers are documented beside their exact bodies: one
non-escaping Pos for LOOKAT's projected offsets, a shared Pos for CONNECT's
instance bytes, and LINK's shared three-int aggregate with the reused byte
temporary assigned before each coordinate's delta and sum. Scratch probes
for CLEAR are in `/tmp/v_finish`; they are local experiments, not deliverables.

## Per function

`eventgoal.c` — every goal check was exact on the first compile: they are
one shape (`if (HintTimerDue() && !ShowGoalHint(e)) { h = NewTimedEvent(K,
1); h->fields = args; QueuePendingEvent(h); }`), kinds 7..19 (below).

| address | name | insns / bytes | audit |
| --- | --- | ---: | --- |
| 0x00468f80 | `GoalCheck_ParkVisitors(e, count)` kind 7, f1c | 18 / 52 | OK |
| 0x00468fc0 | `GoalCheck_Gardeners(e, count)` 8, f1c | 18 / 52 | OK |
| 0x00469000 | `GoalCheck_Gardeners2(e, count)` 8, f1c = -count | 19 / 54 | OK |
| 0x00469040 | `GoalCheck_Mechanics(e, count)` 9, f1c | 18 / 52 | OK |
| 0x00469080 | `GoalCheck_Mechanics2(e, count)` 9, f1c = -count | 19 / 54 | OK |
| 0x004690c0 | `GoalCheck_Save(e, slot)` 13, f1c | 18 / 52 | OK |
| 0x00469100 | `GoalCheck_Happiness(e, a, b)` 14, f1c/f14 | 20 / 59 | OK |
| 0x00469140 | `GoalCheck_Hunger(e, a, b)` 15, f1c/f14, f18 = 1 | 21 / 66 | OK |
| 0x00469190 | `GoalCheck_Hunger2(e, a, b)` 15, f18 = 0 | 21 / 66 | OK |
| 0x004691e0 | `GoalCheck_Rides(e, a, b)` 16, f1c/f14 | 20 / 59 | OK |
| 0x00469220 | `GoalCheck_RideVisitors(e, elem, count)` 17, elem/f1c | 20 / 59 | OK |
| 0x00469260 | `GoalCheck_Composite(e, elem, a, b)` 18, both clamped at 0, f1c = a, f14 = b | 31 / 83 | OK |
| 0x004692c0 | `GoalCheck_Unused(e, a, b, c)` 19, f18/f14/f1c — dead | 22 / 66 | OK |
| 0x00469310 | `GoalCheck_Coverage(e, which, amount)` 10, f1c = which, f14 = amount | 20 / 59 | OK |
| 0x00469350 | `GoalCheck_PathScenery(e, amount)` 11, f14 | 18 / 52 | OK |
| 0x00469390 | `GoalCheck_LoopComposite(e)` — hint only | 8 / 21 | OK |

`eventtick.c`:

| address | name | insns / bytes | audit | note |
| --- | --- | ---: | --- | --- |
| 0x00469980 | `CountNewThemeElem` | 86 / 247 | OK | 5 → 0: the class's theme is +0x5c, not +0x58 (fpui2.c); brief's `RefreshThemeElements` |
| 0x00469a80 | `MarkElemNew` | 14 / 45 | OK | first try; brief's `RefreshThemeMenu` |
| 0x00469ab0 | `MarkElemUnavailable` | 13 / 46 | OK | first try; movie3.c's name |
| 0x00469ae0 | `EventTick_Unimplemented` | 8 / 27 | OK | first try |
| 0x00469b00 | `EventTick_Unimplemented2` | 8 / 27 | OK | first try |
| 0x00469b20 | `EventTick_Give` | 16 / 44 | OK | first try |
| 0x00469b50 | `EventTick_Kind3` | 7 / 22 | OK | first try |
| 0x00469b70 | `EventTick_Take` | 7 / 22 | OK | first try |
| 0x00469b90 | `EventTick_Addbricks` | 8 / 27 | OK | first try |
| 0x00469bb0 | `EventTick_Currency` | 7 / 22 | OK | first try |
| 0x00469bd0 | `PlaceScriptObject` | 35 / 111 | OK | first try |
| 0x00469c40 | `EventTick_Place` | 9 / 26 | OK | first try |
| 0x00469c60 | `ClearSfxFade` | 7 / 18 | OK | first try; brief's `sub_469c60` (the CLEAR sample's fade callback) |
| 0x00469c80 | `EventTick_Clear` | 177 / 576 vs 177 / 577 | **WIP** | 120 → 32 → 24 strict differences, first at insn 110; see the register-assignment checkpoint at the end |
| 0x00469ed0 | `EventTick_Unglue` | 29 / 76 | OK | first try |
| 0x00469f20 | `EventTick_Glue` | 29 / 74 | OK | first try |
| 0x00469f70 | `EventTick_Extendpark` | 5 / 14 | OK | first try |
| 0x00469f80 | `EventTick_Fmv` | 18 / 62 | OK | first try |
| 0x00469fc0 | `EventTick_Interval` | 33 / 112 | OK | first try |
| 0x0046a030 | `EventTick_Message` | 5 / 14 | OK | first try |
| 0x0046a040 | `SetFeatureFlags` | 40 / 162 | OK | first try (jump table in .text at 0x0046a0e4, tail call) |
| 0x0046a120 | `EventTick_Feature` | 9 / 26 | OK | first try |
| 0x0046a140 | `SetReportParam` | 12 / 34 | OK | first try; brief's `SetReportMode` |
| 0x0046a170 | `EventTick_Report` | 11 / 30 | OK | first try |
| 0x0046a190 | `EventTick_Gardener_Mechanic` | 33 / 83 | OK | first try |
| 0x0046a1f0 | `EventTick_Workers` | 19 / 55 | OK | first try |
| 0x0046a230 | `EventTick_Degrade` | 78 / 204 | OK | 27 → 0: `amount` is an unsigned char (byte compare) |
| 0x0046a300 | `EventTick_Capacity` | 12 / 40 | OK | first try; levelkw3.c's name |
| 0x0046a330 | `EventTick_Capacityscale` | 9 / 29 | OK | first try |
| 0x0046a350 | `EventTick_Capacitycap` | 9 / 29 | OK | first try |
| 0x0046a370 | `EventTick_Entrancefee` | 5 / 20 | OK | first try (`(short)e->f1c`) |
| 0x0046a390 | `EventTick_Endlevel` | 8 / 25 | OK | first try |
| 0x0046a3b0 | `EventTick_Lookat` | 37 / 106 | OK | recovered shared projected-offset Pos; see source |
| 0x0046a420 | `EventTick_Themeicon` | 9 / 26 | OK | first try (byte second argument) |
| 0x0046a440 | `EventTick_Addflag` | 9 / 26 | OK | first try |
| 0x0046a460 | `EventTick_Bridges` | 9 / 26 | OK | first try |
| 0x0046a480 | `EventTick_Breifingfile_Briefingfile` | 7 / 22 | OK | first try |
| 0x0046a4a0 | `EventTick_Hintsfile` | 7 / 22 | OK | first try |
| 0x0046a4c0 | `EventTick_Flashbutton` | 9 / 26 | OK | first try; levelkw3.c's name |
| 0x0046a4e0 | `EventTick_Purge` | 3 / 11 | OK | first try |
| 0x0046a4f0 | `EventTick_Need` | 29 / 67 | OK | 16 → 0: nest the failure, trailing `return 1` |
| 0x0046a540 | `EventTick_Needat` | 43 / 108 | OK | 23 → 0: the bounds test written inline (lazy y read) |
| 0x0046a5b0 | `EventTick_Needin` | 87 / 221 | OK | 77 → 0: volatile view of `g_map_rows` at the lookup (below) |
| 0x0046a690 | `EventTick_Connect` | 68 / 158 | OK | recovered shared base-coordinate Pos; see source |
| 0x0046a730 | `IsLinkableClass` | 12 / 32 | OK | 9 → 0: one `||` condition; brief's `sub_46a730` |
| 0x0046a750 | `EventTick_Link` | 165 / 430 | OK | recovered shared coordinate/byte temporary; see source |

Renames: `RefreshThemeElements` → `CountNewThemeElem`, `RefreshThemeMenu` →
`MarkElemNew` (it clears flag 2, sets 0x10000 and counts the class under its
theme), `sub_469ab0` = movie3.c's `MarkElemUnavailable` (defined here),
`SetReportMode` → `SetReportParam`, `sub_469c60` → `ClearSfxFade`,
`sub_46a730` → `IsLinkableClass`, `EventTick_Maxcapacity_…` →
`EventTick_Capacity` and `EventTick_Flashbuttoff_Flashbutton` →
`EventTick_Flashbutton` (levelkw3.c's constructor names). Scope X's seven
primitives are declared with our reading of their bodies: `HintTimerDue`
0x00468d10, `ShowGoalHint` 0x00468d30, `NewTimedEvent` 0x00468cd0,
`QueuePendingEvent` 0x00468c80, `QueueNeedHint` 0x00468d80,
`QueueConnectHint` 0x00468dc0, `QueueLinkHint` 0x00468e00 — one rename each
for the integrator if X chooses otherwise.

## Historical residuals before the 61-function checkpoint

The interrupted session recorded the following residuals. LOOKAT, CONNECT
and LINK have since been recovered as exact; the current CLEAR residual is
described in the latest checkpoint below. These notes preserve earlier probes
and must not be read as proof that other source forms cannot match.

- **`EventTick_Clear`** (172/177 instructions): the frame is exact (0x1854 —
  the four-int footprint `Rect f` with only `top`/`bottom` ever stored is
  the two spilled sums and the two empty slots; the never-stored `next`
  home is the original's uninitialised read before the loop) and every
  block is in place. The original keeps `next` in ebx, the render-list cell
  in edi and the base x in ebp (loaded through edx and moved), base y in
  edx; ours puts `next` in edi, so it must be spilled around the two
  `rep movsd` cursor copies (+5 instructions). Tried: `for` vs `while`
  loops, `case 2:` vs `default:` (the default moves the cell into edi but
  loses the uninitialised read), block-scoped locals, declaration orders,
  scalar/inline/Rect footprints, `unsigned char` base coordinates.
- **`EventTick_Lookat`** (37/37): VC6 sinks `x + y` to its `imul` (`add
  edi,esi`) where the original forms both sums first (`lea ecx,[edi+esi]`)
  and puts `g_map` in edx, `x - y` in eax. Twelve spellings compile to the
  same body (statement order, named sums or products, inline helpers with
  products or sums as arguments, a Pos copy, `e->pos` re-reads, pointer
  reads). One lever did land: `* 256` for the final scale — with `<< 8`
  VC6 folds `(v >> 9) << 8` into `sar 1 / and`.
- **`EventTick_Connect`** (68/68, 158/158 bytes): one register pair — the
  instance cursor is ecx and the y sum edx in the original, reversed here;
  the fail-path pushes follow. Fourteen spellings measured.
- **`EventTick_Link`** (165/165, 430/430 bytes): the two link-square sums in
  both loops accumulate into `d->dx`'s register (eax) in the original and
  into the zero-extended byte's here. Both operand orders, int/byte locals,
  `& 0xff`, pre-read `dx`/`dy`; all compile to ours.

## Mechanics recovered

**Calling convention.** `UpdateHelpTick` (fpui3.c) calls
`g_event_tick[e->kind](e)` for every due event; 1 = done (freed unless its
flags say it survives the step), 0 = still waiting (goal events re-check
every tick). Kinds 2..32 act once; 33..37 are the first goals.

**Step events** (the constructor field names are eventmake.c's): GIVE →
`MarkElemAvailable(elem, popup, 0)` and flag 0x20000 on the element; kind
3 → `MarkElemNew`; TAKE → `MarkElemUnavailable`; ADDBRICKS →
`SetCurrency(GetBrickCount() + n)`; CURRENCY → `SetCurrency(n)`; PLACE →
`PlaceScriptObject(elem, &pos)` (`sub_457870(0)`, `GetTileCentre`, the
view moved to the tile centre, the class's +0x90 place callback with
0x8f8, `RefreshObjList(g_obj_list)`, `PutObjOnMap(class, elem, &g_view)`,
`sub_457870(1)`); CLEAR → three passes over the render list (unpowered
objects whose class hangs off a flag-0x10 menu, then all unpowered, then
all), each object whose footprint (class offsets +0x3c..+0x48 plus its base
square) overlaps the area is removed through the destroy cursor (saved and
restored whole; its origin at +0x1404 is the tree's `g_query_block`), with
the demolition sample looping and faded by `ClearSfxFade` after 3 s;
UNGLUE/GLUE → cell flag 0x40 off/on over the area; FMV → freeze clock,
pointer 0, `sub_496e60(1, 15)`, `PlayMovie(text, 1, 1)`, thaw, kill the
advisor help, restore the step help; INTERVAL → `ShowInfoPanel(0)`,
`LoadHelpTextFor(text)` and the front-end interval screen (mode 2, icons 1,
screen -1, screen mode 7), retried three ticks then dropped with a debug
line; FEATURE → `SetFeatureFlags(idx, v)` (the 12 switches in
levelkw3.c's `g_feature_names` order: Terraces `g_path_overlay_active`,
RideWear `g_ride_wear`, PlantWear (no store), AutoStud `g_auto_stud`
0x00832988, Energy `g_power_available`, Hunger `g_visitor_tire`,
Inspector `g_inspector_on` 0x00832978 + `sub_44db40()`, Autorepair
`g_map->f3c`, Adv_Tune `g_map->f2c`, MoneyBar `g_bricks_full`,
CapacityCalc `g_show_capacity`, FreePlayAtEnd `g_832ba8`; then
`PopInfoSizeMayChange()` as a tail call); REPORT →
`SetReportParam(f18, f1c, f14)` → `g_report_setters[idx](b, a)` (25
setters at 0x004b7e38 → 0x004443b0..0x00444970, the arguments reversed);
GARDENER/MECHANIC → f1c workers generated at the tile between
`sub_457870(0/1)`; WORKERS → `g_map->f38` / `->f34` = word != 0 unless the
word is negative; DEGRADE → the first n instances' cells get damage
`rate * v / 100` where lower (n = 0 or > count means all), cell lookup not
NULL-checked; MAX/MIN CAPACITY → `g_visitor_cap` / `g_visitor_cap_extra`;
CAPACITYSCALE/CAP → `SetSimTuningA/B(idx, v)`; ENTRANCEFEE →
`(short)f1c`; ENDLEVEL → `StopScript(1)` once `g_inspector_on` is 0;
LOOKAT → `g_scroll_x = ((((x-y)*w) >> 9) - (view_w >> 1)) << 8` and the
same with `(x+y)*h`, `view_h`; THEMEICON/ADDFLAG/BRIDGES → the immediate
setters with a `char` second argument; BRIEFINGFILE/HINTSFILE → the file
setters; FLASHBUTTON → `FlashButton(bits, on)`; PURGE → `g_script_purge =
1`.

**Goal events.** NEED: `ObjCount(elem) >= n`, else `QueueNeedHint(e, elem,
n - have)` and `g_need_shortfall = n - have`. NEEDAT: the cell at the tile
has flag 0x80 and `cell->obj == elem`. NEEDIN: count of cells in the area
with flag 0x80, that object, and the cell's own square equal to (x, y);
`>= n`. CONNECT: every instance's link square (class +0x0c/+0x10 plus the
instance's byte square) is on the map and passes `!cell->flags & 0x10` —
the original's precedence slip, `(!flags) & 0x10` is always 0, so any
on-map square passes; off-map → `QueueConnectHint`. LINK: the same
squares must also `TileJoinsPathNetwork`; off-map/non-path counts as not
connected (CONNECT hint wins), unjoined as not linked (`QueueLinkHint`);
with no class named, every render-list object of a linkable class (type
1, 4 or 5) is checked.

**`CountNewThemeElem`**: LEGOLAND and COMMON share counter 0; WESTERN 1,
CASTLE 2, ADVENTURERS 3 (the four bytes at 0x007fe114, bigscreens.c's
`DragState`, here `g_theme_new_count[4]`).

## Globals named for the first time

`g_need_shortfall` 0x0066878c, `g_theme_new_count` 0x007fe114 (the tree's
`g_drag_state` bytes), `g_clear_sfx` 0x004b92fc, `g_auto_stud`
0x00832988, `g_inspector_on` 0x00832978 (movie3.c: "zeroed by
ClearAppraisalState"), `g_report_setters` 0x004b7e38, `g_view` as a Pos
(0x007fffc4/c8). `g_query_block` (0x00811564) is `g_destroy_cursor.origin`
(0x00810160 + 0x1404); the tree's separate declarations name the same
memory.

## Original bugs reproduced

- CONNECT and LINK: `!cell->flags & 0x10` (precedence).
- DEGRADE, NEEDAT, NEEDIN: the bounds-checked cell pointer is NULL off the
  map and dereferenced anyway.
- CLEAR pass 0 looks up an object's power before testing whether it has a
  parent menu at all.

## Extern-type divergences (caller-side levers)

- `SetThemeIcon`, `AddLevelFlag`, `SetBridges` take `(int, char)` here: the
  ticks load the byte (`mov cl, byte ptr [eax+0x1c]`). levelkw2.c declares
  `(int, int)` and pushes a whole int — both callers compile as their
  originals.
- `PutObjOnMap(ObjDef*, LLElem*, Pos*)`; gameframe.c's `place` callback is
  `(LLElem*, Pos*, int)` here.
- `MarkElemAvailable(LLElem*, int, int)`, `ObjCount(void*)`,
  `UpdateDamagedCell(Cell*, Pos*)` — local types.
- `LLElem.flags` is a dword here (`test al,1 / and al,0xfd / or
  eax,0x10000` are byte-narrowed by the compiler).

## Levers, with evidence

- **A volatile view of a pointer global stops its hoist out of a nested
  loop (NEEDIN).** The original reloads `g_map_rows` at every cell
  (`mov eax,[0x801400]` inside the success arm); every plain spelling let
  VC6 strength-reduce `&g_map_rows[y]` into an outer-loop induction
  variable with an extra frame slot (77 mismatches, 236 vs 221 bytes). An
  `int*` to the count and a `volatile int` count did nothing;
  `(*(Cell** volatile*)&g_map_rows)[y][x]` at the one lookup is exact.
- **A footprint compare that spills two of four sums is a `Rect` local
  (CLEAR).** Scalar sums or an inline helper taking the four sums are sunk
  into the compares; `Rect f; f.top = …` gives the aggregate its four slots
  (frame 0x1844 → 0x1854, the original's) and the original's two stores.
- **`do { next = Next(next); } while (next && cond)` is the "skip past"
  loop (CLEAR).** A `for(;;)` with breaks or a `do { …; if (!next) break; }
  while (cond)` gets rotated with the call peeled; the `&&` in the
  do-while condition keeps one call and `jle L / jmp OUT`.
- **`unsigned char amount` for a byte compare (DEGRADE):** `cmp byte ptr
  [cell+0x11], bl / jbe` needs the amount typed as the byte; an `int`
  gives a zero-extended dword compare.
- **A lazily read second coordinate needs the bounds test written out
  (NEEDAT).** The inline `CellAt(x, y)` evaluates both arguments first;
  the original reads `e->pos.y` after the x tests (and saves edi around the
  height compare).
- **`if (have < need) { hint; return 0; } return 1;`** exiles the success
  return (NEED); the guard-first spelling keeps it inline.
- **`if (t == 1 || (t > 3 && t <= 5)) return 1; return 0;`** is the
  compare chain `cmp 1 / je / cmp 3 / jle / cmp 5 / jle` with the zero
  return inline (IsLinkableClass); two separate `if … return 1` inline the
  one return.
- **The named `pagesize`-style locals of scope U did not apply here**: in
  CONNECT and LINK the class offsets are hoisted (CONNECT) or reloaded per
  iteration (LINK, across the call) by VC6 unaided.
- **Inert / not needed**: `#pragma intrinsic` unnecessary (no string
  functions); the inline `CellAt` helper is exact wherever the coordinates
  are already in registers (DEGRADE, CONNECT, LINK).

## Verification

```sh
$PY tools/audit.py LEGOLAND/eventgoal.c   # 16 x [OK], PASS
$PY tools/audit.py LEGOLAND/eventtick.c   # 45 x [OK], 1 x [WIP ], PASS
$PY tools/relocs.py LEGOLAND/eventgoal.c  # 0 MISMATCH
$PY tools/relocs.py LEGOLAND/eventtick.c  # 0 MISMATCH (10 UNRESOLVED literals)
ALPHATEAM_VC6_ROOT="$PWD/toolchain" "$LEGOLAND_CL" /nologo /c /W3 /O2 /Gy /Gd /Fo/tmp/sv_w3.obj LEGOLAND/eventgoal.c
ALPHATEAM_VC6_ROOT="$PWD/toolchain" "$LEGOLAND_CL" /nologo /c /W3 /O2 /Gy /Gd /Fo/tmp/sv_w3.obj LEGOLAND/eventtick.c
```

## Earlier resumption checkpoint — 2026-09-07 (`45eb4d3c`)

The recovered 61 exact functions were committed before further work on CLEAR.
Its unmodified 172i/560B candidate remains the retained WIP; no experimental
variant has been promoted. The note above its marker now corrects the former
claim that it was five instructions longer: the gate counts five fewer.

The fresh probes establish a bounded negative, not impossibility. Simple
reordering, scalar/Pos/array/union views, compound footprint sums, inline
helpers taking values or pointers, grouped local snapshots, copy assignments
versus intrinsic memcpy, packed-byte read/write forms, and several equivalent
loop/guard spellings did not recover the full body. Whole-footprint loads plus
adds can give the original next/cell register pair but introduce other stores
and reloads. A volatile byte reload of the y coordinate changes allocation,
yet pins it too late or moves the wrong coordinate into ebp. Neither is an
acceptable promotion. Diagnostic compiler-option and C++ probes also failed;
the required `/O2 /Gy /Gd` toolchain remains unchanged.

For a future pass, start from the original facts: instruction 19 loads next
into ebx from its never-initialized home; the object is in edi; at 77 the x
byte loads into dl and transfers to ebp at 79, then y reuses edx at 80–81.
At 110 sq.y spills before the cursor copy, 112 moves x from ebp to ecx,
113 stores cursor origin.y from edx, 114 reloads sq.y as a byte, and 115
stores sq.x from ecx. This is followed by the original selected-position
and class stores. The cursor restore reloads saved_def from the stack.
Scratch generators, source, full comparisons and scores are `/tmp/v_finish/`;
no scratch code is required by the project or included in the commits.

Four extern names now agree with the already integrated definitions by
address: `ResetAppraisalDeadline` (0x0044db40), `GoalCheck_Need`
(0x00468d80), `GoalCheck_Connect` (0x00468dc0), and `GoalCheck_Link`
(0x00468e00). Their caller-side types are preserved. The older rename and
residual discussions above are historical.


## Latest CLEAR checkpoint and version evidence — 2026-09-07

*(Superseded by the register-assignment checkpoint at the end of this file,
which took the same body from 32 to 24 strict mismatches; the notes below
describe the 32-strict state.)*

The retained body then had **177 instructions / 576 bytes, 32 strict
mismatches**, versus the English original's 177 / 577. The former retained
body had 172 / 560 and 120 mismatches. A disassembly of the complete COFF
function confirms that the new body ends at its own `ret`; the count is not
a prefix trimmed from a longer function.

Separate footprint-field loads and offset additions preserve the original
`next` / current-cell register pair (ebx / edi) and both footprint spills.
A distinct low-byte read of `sq.y`, into a byte local after filling `sq`,
retains the reload seen in the original. The first strict difference is now
instruction 79: x stays in edx and y in ebp, whereas the original transfers
x to ebp and keeps y in edx. The cursor setup and a few later register choices
still differ. The WIP marker states that residual; it is not ready to merge.

Fresh whole-file audits report 16 exact goal functions and 45 exact tick
functions, plus CLEAR as WIP. Both files compile cleanly with `/W3` and the
required `/O2 /Gy /Gd`. Exact-body relocation checks still give 62 + 125
resolved references, zero mismatches, and the same ten unresolved tick
references described at the top. Logs are in `/tmp/v_finish/retained-*.txt`.
Only CLEAR and its explanatory notes changed in this checkpoint.

The additional-media reports in the F/G/H integration worktree were read
without changing its owned files or repeating its extraction work. A separate
CLEAR-specific comparison located one matching prologue in each reviewed
executable, then independently bounded the complete routine using that
executable's own export boundaries:

| Reviewed executable | CLEAR address | Complete body | Normalized instructions / widths / internal branch offsets |
| --- | --- | --- | --- |
| English reference | `0x00469c80` | 177i / 577B | reference |
| Dutch copy | `0x00469ff0` | 177i / 577B | identical |
| Czech copy | `0x00469cd0` | 177i / 577B | identical |

Input SHA-256 values:

- English: `c50865b60bfcb26c0a7329a75fb772b10ae234af324f669901906e5abb0e2bd9`
- Dutch: `e7012b899a75049a666d6f7b473cee84e95bceb58e7518a92fca89145ddf2683`
- Czech: `8b4f8c2f800046d07b71fe1c6f8e22c1853152c5b313c13a032f021b08aa8557`

This comparison masks external call/global addresses; it establishes the same
instruction structure, not full runtime equivalence of the builds. English
remains the sole acceptance reference. The local script, result metadata and
paired listings are `/tmp/v_finish/cross_versions.py`,
`Clear-cross-versions.json`, and `Clear-{english,dutch,czech}.txt`. No game
binary, asset, extracted media or F/G/H research file is part of this commit.

Additional bounded probes covered aggregate-return helpers; byte-coordinate
views and integer types; footprint load/add interleavings; separate byte
reload timing; and reuse of coordinate temporaries in non-escaping aggregates.
The retained recipe is `batch34.c` / `Clear1097`. Later grouped-temporary probes
reached 22 index mismatches but added a non-original `and 0xff`, changed the
stack homes and remained four bytes too long; that score alone is not grounds
to prefer them. No probe produced a complete exact match. Scratch generators,
variants and comparisons remain local under `/tmp/v_finish/`.

## Japanese demo follow-up — 2026-09-07

The Japanese demo also contains the complete CLEAR instruction structure:
one unique masked-prefix match at `0x00469c00`, independently bounded using
the demo's own export addresses, gives 177 instructions / 577 bytes. Its
normalized instructions, instruction widths and internal branch offsets
match the English reference at `0x00469c80`. External calls and global
addresses were normalized; this is not a claim of full runtime equivalence.
The demo executable's SHA-256 is
`31f54cb742518a494df1683a67e3da6ca44c0d7329d123a682261d252d489e73`.

The shifted CLEAR address must not be inferred from the unchanged addresses
reported for the eight F/G/H targets. English remains the sole matching
reference. No source or usable symbols were recovered in the media review.
The reported Japanese text-input and demo-screen differences remain separate
research leads, not changes to V's assigned functions.

Follow-up probes tested qualified small-coordinate copies (96 candidates)
and inline byte-position conversion helpers (108 candidates), starting from
the older and retained C forms. Neither family improved the saved 32 strict
differences or produced a complete exact match. The code checkpoint remains
`11edace8`: 61 of 62 functions exact, with CLEAR held at WIP. No additional
C change resulted from this follow-up.

Local evidence is `/tmp/v_finish/japanese-demo/STATUS.md`,
`compare_clear.py`, `Clear-cross-versions.json`, and the paired listings;
the new probe families are `gen42.py` / `gen43.py` under `/tmp/v_finish/`.
This committed summary contains no game binary, asset, installer, extracted
disassembly, or F/G/H-owned research file.

## CLEAR register-assignment checkpoint — 2026-09-07 (`scope/V-clear`)

`EventTick_Clear` is still WIP, but the residual is now half the block it
was: **177 instructions / 576 bytes against the original 177 / 577, 24
strict differences, first at instruction 110** (was 32 strict, first at
instruction 79). `tools/matchfull.py` reports 156/177 = 88.1%, up from
148/177 = 83.6%. Whole-file audit is PASS with 45 exact in `eventtick.c`
and 16 exact in `eventgoal.c` (61 of 62), `/W3` clean, `tools/relocs.py`
zero MISMATCH.

Everything from the prologue through the footprint overlap test is now
instruction-for-instruction exact, including the parts that were the whole
point of the earlier residual: `next` in ebx, the render-list cell in edi,
base x in ebp loaded through edx and moved (insns 77-79), base y kept in
edx (80-81), and both footprint spills at `[esp+0x24]` / `[esp+0x2c]`.
Register-blind the whole body is 5 instructions off; register- *and*
offset-blind it is 3.

### What the original's cursor-setup group does

Instructions 110-119, with `sq` at `[esp+0x18]` (x) / `[esp+0x1c]` (y) and
`g_query_block` = `g_destroy_cursor.origin` at `0x811564` / `0x811568`:

```
110  mov [esp+0x1c], edx      sq.y = by            (hoisted above the copy)
111  rep movsd                saved = g_destroy_cursor
112  mov ecx, ebp             base x into a byte-capable register
113  mov [0x811568], edx      origin.y from y's own register
114  mov dl, [esp+0x1c]       y's byte RELOADED from sq.y's home
115  mov [esp+0x18], ecx      sq.x from the copy
116  mov [0x811564], ecx      origin.x from the copy
117  mov [0x667c54], cl       g_sel_bpos.b.x from the copy's low byte
118  mov [0x667c58], eax      g_sel_def = d
119  mov [0x667c55], dl       g_sel_bpos.b.y from the reloaded byte
```

So the original serves x's byte from a *register copy* even though x lives
in ebp, which has no byte form, and serves y's byte from a *memory reload*
even though y lives in edx, which does. The retained body has those two
roles exactly the other way round; that is the entire remaining residual,
plus the register renames it forces on instructions 134-153 and 163-167 and
the one byte it costs (original `mov ecx, [0x667c58]` at 134 is 6 bytes,
ours `mov eax, ...` is 5 — the 576-vs-577 deficit is that single encoding).

### Levers discovered, with evidence

1. **Which coordinate carries a byte need decides the whole assignment.**
   Taking one coordinate's `g_sel_bpos` byte out of memory instead of out of
   the base coordinate removes that coordinate's byte need, and VC6 then
   hands out `ebx` / `ebp` / `edx` so that the byte-needing coordinate gets
   the byte-capable `edx` and `next` gets `ebx`. Measured over 224 variants
   (`/tmp/svclear_Q.c`): the byte-free coordinate always lands in ebp.
   - byte-free x (`px = *(volatile unsigned char*)&sq.x`) → `next=ebx
     x=ebp y=edx`, footprint block exact, 24 strict. **This is the body now
     committed.**
   - byte-free y → `next=ebx x=edx y=ebp`, 30-33 strict, footprint broken.
   - neither byte-free (two byte needs) → `next=ebp`, 109-112 strict; the
     two coordinates take ebx/edx and `next` is pushed onto ebp.

2. **The order of `bx = c->x` / `by = c->y` swaps which coordinate gets edx
   and which gets ebx** in the two-byte-need case (128 variants,
   `/tmp/svclear_P.c`): `bx` first gives `x=edx y=ebx`, `by` first gives
   `x=ebx y=edx`. Declaration order (`int bx, by` vs `int by, bx`) has no
   effect. With a byte-free coordinate present the lever is inert.

3. **The footprint statement order is load-bearing and now pinned**:
   `f.top`, `f.bottom`, `f.left`, `f.top += by`, `f.right`,
   `f.bottom += by`, `f.left += bx`, `f.right += bx`. The previously
   committed `f.bottom` / `f.top` opening reorders insns 82-89.

4. **Aggregate vs field-wise origin store trades the copy against the
   ordering.** `g_destroy_cursor.origin = sq;` makes VC6 emit exactly one
   register copy in the group (177 instructions, the original's count) but
   groups the two stack stores before the two absolute stores. Writing
   `origin.y` / `origin.x` as separate fields reproduces the original's
   interleaving — `sq.y`(110), `origin.y`(113), `bpos.x`(117),
   `g_sel_def`(118) all land exactly — but drops the copy, giving 176
   instructions / 575 bytes (best save-block residual 9 of 20, total 59).
   Sweeps: 1860 variants `/tmp/svclear_R.c`, 3240 `/tmp/svclear_S.c`.

5. **The footprint field order of `ObjDef` is pinned without relying on the
   struct comment.** `d[0x3c]` is added to the ebp coordinate and compared
   `<=` against `e->area[0x30]`, and `d[0x44]` is compared `>=` against
   `e->area[0x28]`; with the area layout fixed by the exact UNGLUE / NEEDIN
   bodies this forces `d[0x3c]` = left, `d[0x44]` = right, `d[0x40]` = top,
   and therefore ebp = x, edx = y. Any reading that swaps x and y in this
   body is excluded.

### Bounded negatives added by this session

- **No non-volatile spelling produces the byte reload at 114.** Ten
  spellings tested (`/tmp/svclear_X.c`): `(unsigned char)sq.y`,
  `((BPos*)&sq.y)->x`, `((unsigned char*)&sq)[4]`,
  `*(unsigned char*)((char*)&sq + 4)`, a `unsigned char*` variable indexed
  at `[4]` and at `[0]`, a `Pos*` variable dereferenced before and after the
  stores, and the plain `&sq.y` byte cast. All forward the stored register
  and emit `mov [0x667c55], dl` with no load. Only a volatile-qualified
  access emits `mov dl, byte ptr [esp+0x1c]`, which is the original's exact
  instruction — but volatile on the y side moves y into ebp.
- **Forwarding is not distance-limited**: with six statements between the
  `sq.y` store and the `(unsigned char)sq.y` read VC6 still forwards.
- **The `saved = g_destroy_cursor` aggregate copy does not kill forwarding**
  either, so putting the `sq.y` store above the `rep movsd` does not buy the
  reload (8 variants, `/tmp/svclear_N0.c`; it also breaks the footprint).
- **Carrier temps do not create the `mov ecx, ebp` split.** `int sx = bx;`
  coalesces when `bx` has no later use, and when a later use of `bx` is
  added to block coalescing the reference count moves the wrong way, so the
  assignment never reaches `x=ebp` (168 variants `/tmp/svclear_W.c`, 192
  `/tmp/svclear_Y.c`, 2592 `/tmp/svclear_O.c` — none produced `x=ebp`).
- **Mirroring the whole save block to match the original's *kind* sequence**
  (stack store, copy, absolute store, byte load, stack store, absolute
  store, byte store, absolute store, byte store) reaches save-block residual
  10 of 20 but again loses the copy, 176 instructions / 575 bytes (56
  variants, `/tmp/svclear_Z.c`).
- Byte source read back from the just-stored global
  (`(unsigned char)g_destroy_cursor.origin.x`) behaves exactly like reading
  the coordinate: the byte need lands on the coordinate, not on a temp.
- Re-reading `c->x` in the save block is not viable: `c` is in edi, which
  the `rep movsd` clobbers, and VC6 does not re-materialise it (≈158 strict).

### Measured floor

Across roughly 10,000 generated variants this session the best reachable
state is 24 strict with the footprint block exact, and every variant that
fixes the two byte roles loses either the ebp assignment or the copy. To
close CLEAR, the next pass needs a construct that gives base x a byte use
which VC6 does **not** treat as a register constraint on x itself — so that
x can still be handed ebp and the byte need is settled by a split copy —
while y's byte comes out of `sq.y`'s home slot without a volatile
qualifier. Nothing in the levers index currently produces that pair; the
two known ways to get a byte out of memory (volatile access) and to get a
non-coalescing copy (interference) both move the assignment the wrong way.

Scratch for this session is `/tmp/svclear_*` (generators `svclear_gen*.py`,
scorers `svclear_probe2.py`, `svclear_sbs.py`, `svclear_bytes.py`,
`svclear_alloc2.py`, `svclear_hasload.py`). No binaries, assets or extracted
disassembly are committed.

## CLEAR reconstruction hypotheses, second pass — 2026-09-07

The first pass varied one C shape ~10,000 ways and plateaued at 24 strict.
This pass changed the reconstruction hypothesis instead and measured five
candidate readings of the source. All five are negative for the byte reload
at instruction 114; the state is unchanged at **177i/576B, 24 strict, first
difference at 110**, whole-file audit PASS, 45 exact in `eventtick.c` plus
16 in `eventgoal.c`. What the pass did buy is a sharp statement of why the
group cannot close with any construct in the levers index, and two new
structural facts about the group (see the last two subsections).

### H1 — asymmetric source symbols: negative

If the two byte stores named different objects, x's byte could come from a
register and y's from the aggregate's home. Swept every combination of
which object each byte store reads (`bx` / `sq.x`, `by` / `sq.y`) crossed
with which side of `saved = g_destroy_cursor` each `sq` field store falls
on, both origin spellings and both tail orders: 144 variants
(`/tmp/svclear2_A.c`) plus 128 more with the copy spelling swept
(`/tmp/svclear2_B.c`). **Zero of the 272 emit any byte load from the stack
in the group**, and moving either `sq` store above the copy breaks the
footprint block (19 of 24 differ, total 106-111 strict). VC6 propagates
`sq.y`'s stored value to the byte store regardless of which symbol the
source names, so the two stores naming different objects is not what
produces the asymmetry.

### H2 — `sq` genuinely address-taken across the copy: negative

`&sq` does escape, but only *after* the group: `lea ecx, [esp+0x18]` at
0x00469e04 for `d->query(d->inst, &sq)` and `lea eax, [esp+0x18]` at
0x00469e35 for `RemoveObjectPathTiles`. Both are downstream of the byte
reload at 0x00469ddf, so no callee can be making memory canonical for
`sq.y` at that point, and no callee in the group receives a pointer into
`sq`. Modelling the copy as something VC6 must treat as writing through
pointers does not help either: `memcpy(&saved, &g_destroy_cursor,
sizeof(saved))` with `#pragma intrinsic(memcpy)`, `cp = &g_destroy_cursor;
saved = *cp;`, and `sp = &saved; *sp = g_destroy_cursor;` — crossed with
matched and unmatched restore spellings — all emit the same `mov ecx, 0x60d
/ lea edi / rep movsd` and all still forward the byte (128 variants,
`/tmp/svclear2_B.c`). The escape reading is excluded, so the volatile probe
cannot be replaced by an escape.

### H3 — the copy is a different type than assumed: negative

The copied extent is pinned and matches the retained layout: `mov ecx,
0x60d` is 1549 dwords = 6196 = 0x1834 bytes, exactly `sizeof(Cursor)` as
declared, and `origin` at +0x1404 is inside that extent, so
`saved = g_destroy_cursor` copies the whole cursor including its origin and
the origin stores that follow are genuine re-writes. Origin-as-its-own
aggregate, origin written through a `Pos*` alias, a `union { Pos p;
unsigned char b[8]; int w[2]; }` view of `sq` with the byte read as `b[4]`,
filling `sq` by aggregate copy from a register-resident `Pos t`, and an
inlined `PosLoY(const Pos*)` helper were all tested (13 hand-written cases
`/tmp/svclear2_D.c`, plus 256 origin spellings `/tmp/svclear2_E.c`). None
produces a stack byte load; the union and aggregate-copy paths compile
identically to the plain field assignment.

### H4 — destination widths and the register pool: negative

The destination widths are already right (`Pos` of ints for `origin`, byte
pair for `g_sel_bpos`) and the `dword`-then-`byte` store order follows the
source. The stronger form of this hypothesis — that the coordinates are CSE
temps of `c->x` / `c->y` handed registers out of the temp pool after `next`
takes ebx, rather than enregistered named locals — was swept exhaustively:
for each of the four x use sites and four y use sites independently, name
the local or re-read the cell, crossed with assignment and tail order, 1024
variants (`/tmp/svclear2_C.c`). Result: 128 variants do reach `x=ebp`, but
all of them are the ones whose x *byte* re-reads `c->x`, and all cost 158
strict and 584 bytes because `c` lives in edi, which the `rep movsd`
clobbers, so VC6 spills and reloads it. No variant in the batch emits a
stack byte load, and none reaches a clean footprint block.

### H5 — identical-arm join and dead uses: negative

`if (d) g_sel_bpos.b.x = (unsigned char)bx; else g_sel_bpos.b.x =
(unsigned char)bx;`, the ternary `sx = d ? bx : bx;`, and dead round trips
(`sx = bx; ... bx = sx;`) all fold to byte-identical code with the plain
form — 573 bytes, 111 strict, the same register assignment. VC6 collapses
identical arms before allocation, so the trick has no purchase here (14
cases, `/tmp/svclear2_F.c`).

### The two structural facts this pass did establish

1. **One coordinate always goes through a temp, and it is the one whose
   `sq` store lands *after* the copy.** In the original that is x: `mov
   ecx, ebp` at 112 and then ecx serves `sq.x`(115), `origin.x`(116) and
   `bpos.x`(117), while y is stored straight from its own register at 110
   and 113. The retained body has the same one-temp shape with the roles
   swapped — `mov esi, edx` and esi serving the y stores. The role follows
   the volatile probe: whichever field is read out of memory has its store
   hoisted above the copy and keeps its own register.

2. **The original writes `sq` and `origin` field by field, y pair first.**
   Stores 110 / 113 / 115 / 116 are `sq.y`, `origin.y`, `sq.x`,
   `origin.x` — an interleaving that a chained assignment reproduces
   exactly: `g_destroy_cursor.origin.y = sq.y = by;` then
   `g_destroy_cursor.origin.x = sq.x = bx;`. With that spelling the
   save-block residual drops to 10 of 21, its best value yet, and
   instructions 110-111 and 118 land exactly (252 variants,
   `/tmp/svclear2_G.c`). But field-wise stores need no aggregate temp, so
   the copy at 112 disappears and the body comes out 176 instructions,
   shifting the whole tail. Aggregate origin buys the copy and loses the
   order; chained origin buys the order and loses the copy.

### The floor, stated precisely

Every mechanism that makes a coordinate's byte come out of memory also
moves that coordinate into ebp. Volatile casts, a `volatile Pos sq`
declaration, a `struct { int x; volatile int y; }` local, and
`((volatile Pos*)&sq)->y` all produce the original's exact `mov ?l, byte
ptr [esp+0x1?]` instruction, and in all of them the volatile-read
coordinate takes ebp and the other takes edx (138 variants,
`/tmp/svclear2_H.c`). The original needs the opposite pairing: ebp holds
the coordinate whose byte *is* wanted in a byte register — which is exactly
why it must spend `mov ecx, ebp` — and edx holds the coordinate whose byte
comes from memory even though dl was available. The inversion reproduced
from four independent directions (byte-need counting, reference counting,
volatile placement, and store hoisting), so it is a property of this
compiler under every spelling reachable here, not a scheduling accident.

Two variants beat 24 on raw count and were rejected: `struct { int x;
volatile int y; }` for `sq` with the chained origin reaches 23 strict but
breaks the footprint block (9 of 24 differ) and needs a fabricated type
with a volatile field, and the chained-origin/volatile-x body reaches
save-block residual 10 but 176 instructions. The committed body keeps the
footprint block exact and the instruction count right, which is the more
reusable evidence.

Closing CLEAR needs a construct outside the current levers index: a byte
use of base x that VC6 does not treat as a register-class constraint on x
itself, paired with a plain (non-volatile) memory read of `sq.y`'s low
byte. Scratch for this pass is `/tmp/svclear2_*` (`svclear2_gen.py`,
`svclear2_gen[A-H].py`, `svclear2_probe.py`).

## CLEAR clean-sheet pass: the forwarding mechanism, measured — 2026-09-07

Third pass, reconstructed from the disassembly without inheriting the earlier
passes' C shape. It did not close CLEAR: the state is unchanged at **177
instructions / 576 bytes against the original 177 / 577, 24 strict
differences, first at instruction 110**, whole-file audit PASS with 45 `[OK]`
in `eventtick.c` (plus CLEAR as WIP) and 16 in `eventgoal.c`, `tools/relocs.py`
zero MISMATCH, `/W3` clean. What it did buy is the *mechanism* behind
`mov dl, byte ptr [esp+0x1c]` at 0x00469ddf, established on scratch functions
rather than inferred from the body, and it **corrects a lever** that the
earlier passes and the levers index both relied on.

### The corrected lever

"A store to ANY global kills CSE of an unrelated load" ([LEVERS.md](../LEVERS.md)
Reads, from D-era evidence) is true for a re-load of a GLOBAL or of a field of
an address-taken struct. It is **false for store-to-load forwarding of a
local's own stored value**: VC6 SP3 forwards a local's stored register to a
later narrowing read across any number of ordinary global stores. So the
hypothesis that the original's asymmetry (x's byte from `cl`, a register copy;
y's byte from a reload of `sq.y`'s home) falls out of a symmetric source plus
the position of the global stores is **excluded by direct measurement**.

`/tmp/svclear3_probe.c`, `probe2..probe6.c`, each function a five-to-ten line
scratch shape compiled `/O2 /Gy /Gd` and read instruction by instruction:

- **Global stores do not kill it.** Zero, one and three global stores between
  `sq.y = v` and `g_b.y = (unsigned char)sq.y` all forward (`P_A`, `P_B`,
  `P_C`); so does a store to a `volatile`-qualified global (`P_T1`, `P_T2`) and
  a store of a pointer into a global (`g_sel_def = d` before the read, all 162
  such full-body variants below). Address-taken or not makes no difference
  (`P_D`).
- **The 6196-byte copy does not kill it.** Struct assignment, intrinsic
  `memcpy` of a struct, `memcpy` of a local `char[6196]` through decayed
  pointers, `*sp = g_big` through a pointer variable, and `saved = *g_bigp`
  from an opaque global pointer all forward (`P_E`, `P_J`, `P_Q1`..`P_Q7`).
  VC6 will even sink the `rep movsd` *below* the byte store when nothing stops
  it.
- **No access-path spelling kills it.** Store through a `Pos*` alias and read
  the field; store the field and read through the alias; read through a
  differently-tagged struct pointer; store through an `int*` to the field;
  union int-member store with a named byte-member read; a byte-quartet struct
  written with an `int` store; `*(unsigned char*)&sq.y`; the escape
  established *before* the store; the read used twice — ten shapes, all
  forward (`P_R1`..`P_R10`). This reproduces the earlier passes' ten negatives
  from a different direction.
- **Three things do kill it**, and each emits the original's exact
  instruction:
  1. a **call** between the store and the read while the aggregate's address
     escapes (`P_K` → `mov dl, byte ptr [esp+8]`);
  2. a **volatile** access (`P_L`) — the retained body's probe;
  3. a **store through a pointer VC6 cannot resolve** (`P_T3`, `P_T5`:
     `*g_ip = v` with `g_ip` a global `int*`). `P_T5` reproduces the
     original's asymmetry exactly and with no `volatile` anywhere: x's byte
     comes from the register (`mov byte ptr [..], cl`) and y's from
     `mov dl, byte ptr [esp+4]`.
- **The new kill does not survive address propagation** (`P_U1`..`P_U6`):
  `Pos* q = &g_o; q->y = v;` folds to `mov [<abs>], eax` and forwards again,
  as does a pointer to a global scalar, a pointer into `sq` itself, and a
  second address-taken local. So mechanism 3 always costs the pointer load
  (`mov reg, [g_ptr]`) that the original's group does not contain — the group
  at 0x00469db9..0x00469df8 has only stack stores, absolute stores and the
  `rep movsd`. That is why the residual survives: the original needs a kill
  where no reachable construct puts one.

### Steps 1-3: the symmetric family, 486 variants (`/tmp/svclear3_gen3.py`)

`{by | sq.y | chained}` for `origin.y` x `{bx | sq.x | chained}` for
`origin.x` x `{bx | sq.x | memory}` for the x byte x `{by | sq.y | memory}`
for the y byte x `sq.y`'s store before/after the copy x `g_sel_def` before /
between / after the two byte stores.

| family | n | strict | shape |
| --- | ---: | ---: | --- |
| both bytes plain (any spelling, any order) | 216 | **108** | 174i/570B, first diff 19, no byte load |
| x byte from memory, y plain | 108 | 59 | 176i/574B, first diff 112 |
| y byte from memory, x plain | 108 | 69 | 176i/574B, first diff 79 |
| both bytes from memory | 54 | 35 | 177i/578B, two byte loads |

The first row is the informative one: **all 216 plain variants compile to
byte-identical code.** Origin spelling, chaining, which symbol each byte store
names, `sq.y`'s store position and `g_sel_def`'s position are all inert once
both bytes are register-sourced; the two byte needs alone drive the allocation
to `next=ebp`, x and y into `edx`/`ebx` (loaded with `xor ebx,ebx / mov bl,
[edi+5]`), and the footprint block breaks 19 instructions earlier. This
confirms the first pass's byte-need lever from an independent starting point.

**Emitted store order is not source order here.** In every plain variant VC6
regroups the group's stores by REGISTER — all of one coordinate's stores, then
the other's — and hoists one coordinate's stack store above the `rep movsd`
itself. With `sq.y` written first in source it hoists `sq.x`. So the
original's `mov [esp+0x1c], edx` at 0x00469dd1, above the copy, is the
scheduler's choice and is not evidence that the source stores `sq.y` before
`saved = g_destroy_cursor`. Measured directly: hoisting `sq.y` in the SOURCE
costs 2 strict (26 against 24).

### Steps 3+5: the aggregate-origin family, 324 + 216 variants

`/tmp/svclear3_gen4.py` (byte sources x store positions x `g_sel_def` position
x `g_sel_bpos` spelling x copy spelling) and `/tmp/svclear3_gen5.py` (byte
store ORDER and the memory read's placement).

- **`g_sel_bpos` must be two field stores.** Filling a local `BPosW` and
  assigning it whole gives one word store: best 64 strict, 581 bytes, against
  27 for the same 162 configurations written as two byte fields.
- **The copy spelling is completely inert.** `saved = g_destroy_cursor` and
  `memcpy(&saved, &g_destroy_cursor, sizeof(saved))` — with or without
  `#pragma intrinsic(memcpy)` — compile byte-identically (24 strict either
  way). `/O2` already implies `/Oi`.
- **The struct TAG is inert.** `sq` declared as a distinct `struct SqPos {
  int x, y; }` with casts at `d->query` and `RemoveObjectPathTiles` is
  byte-identical at 24 strict. A union of `Pos` with a named byte field at
  offset 4, read as the y byte, drops to 108 (no byte load).
- **Both sq stores after the copy** beats hoisting `sq.y` (24 vs 26), and
  **the memory read placed before the origin store** beats placing it at its
  use (24 vs 27).
- **`g_sel_def`'s position** is worth at most one: 24 before both byte stores,
  24 between them, 25 after both.
- **The byte-store order is inert at the optimum**: x-byte-first and
  y-byte-first both reach 24.
- **Byte-typed temps coalesce** and do not manufacture the original's
  `mov ecx, ebp` split: `unsigned char yb = (unsigned char)sq.y` defined
  before the copy and used after it (so its range crosses the copy and
  interferes with `by`) still forwards, in all eight shapes tried
  (`/tmp/svclear3_gen2.py`).

### Outcome

Nothing beat the retained body, and the best non-volatile spelling in this
pass is 105 strict, so the body committed by the previous pass stands
unchanged: 24 strict, 177i/576B, footprint block exact, one `volatile` byte
probe on `sq.x`. The floor statement from the second pass is unchanged but its
reason is now sharper and is a general rule rather than a property of this
body: **the group needs a memory-kill between `sq.y`'s store and its byte
read, and VC6 SP3 provides exactly three, all of which cost either an
instruction the original does not have (a call, an unresolvable pointer store)
or a qualifier the source cannot plausibly carry.** Anyone reopening CLEAR
should start by finding a fourth kill, not by permuting statements: the
statement space is now measured flat.

Scratch for this pass is `/tmp/svclear3_*` (`svclear3_probe.py` scorer,
`svclear3_harness.py`, `svclear3_gen[1-6].py`, `svclear3_probe[2-6].c`,
`svclear3_dump.py`). No binaries, assets or extracted disassembly are
committed.

## Fourth pass — the colouring model, 2026-09-07 (integrator session)

Scratch `/tmp/svclear4_*` (`svclear4_probe.c`, `svclear4_variants.py`,
scored with the third pass's `svclear3_probe.py`). CLEAR is unchanged at 24
strict; this pass measured the ALLOCATION rather than the byte reload, and it
turns the residual into one precise question.

**The four colourings of the coordinates are decided by register byte needs
alone**, measured on the real function with the footprint block otherwise
identical:

| register byte need at allocation | x | y | `next` | first difference |
| --- | --- | --- | --- | ---: |
| neither (both bytes from `volatile` memory, or no byte stores at all) | edx | ecx → ebp | ebx | 79 |
| x only (`(unsigned char)bx`, y from a `volatile` read) | edx | ecx → ebp | ebx | 79 |
| **y only** (retained body: `volatile` on `sq.x`, `(unsigned char)by`) | **ebp** | **edx** | **ebx** | 110 |
| both (the plain HandleMapClick idiom) | edx | ebx | ebp | 19 |

The original is the third row: x moved to ebp at 0x00469d5f, y kept in edx.
So at allocation time y had a register byte need and x did not — yet the
emitted group serves x's byte from a copy (`mov ecx, ebp`) and y's from a
byte load of `sq.y`'s slot. Both byte sources are therefore resolved AFTER
the colouring is fixed, and any exact spelling must (1) give y a byte need
the allocator sees and (2) still emit y's byte as a load. `volatile` on
`sq.y` fails (1): its temp is not merged with `by`, so the colouring flips
to row two — that is the 176-instruction body `vy_c`, register-blind 2 and
offset-blind 2 against the original, the closest structure found so far:

```c
saved_def = g_sel_def;
sq.y = by;
saved = g_destroy_cursor;
g_destroy_cursor.origin.y = by;
py = *(volatile unsigned char*)&sq.y;
sq.x = bx;
g_destroy_cursor.origin.x = bx;
g_sel_bpos.b.x = (unsigned char)bx;
g_sel_def = d;
g_sel_bpos.b.y = py;
```

Measured negatives for the copy and the colouring (all compile to one of the
two families above unless noted):

- **`unsigned char` coordinates** (the cell fields ARE bytes, loaded with
  `xor edx,edx / mov dl`): VC6 gives byte locals stack homes and reloads
  them with `and edx, 0xff` — 88 strict, frame 0x185c. `short` / `unsigned
  short` likewise (160+). `unsigned`, `long`, `unsigned long` are identical
  to `int`. The coordinates are `int` locals assigned from byte fields, as in
  `HandleMapClick`.
- **`sq.x = (unsigned char)bx`** emits `and edx, 0xff`: VC6 does not track
  the zero-extension of a byte load, so no conversion yields a bare copy.
- **Every separate temp for x coalesces back onto `bx`** — a second `int`,
  a byte local, a `BPos` local filled field-wise or assigned whole, a
  double-defined temp, a temp re-read from `sq.x`, redefining `bx` after the
  store, chained `origin.x = sq.x = bx` and its triple, and the FGH
  identical-arm join `if (c) x2 = bx; else x2 = bx;` on six different
  conditions (the join folds when the arms are a plain copy; FGH's arms held
  a computation). None produces an un-coalesced `mov ecx, ebp`.
- **Reads through an `__inline` helper's `Pos*` parameter** (four bodies,
  four call placements) are forwarded exactly like direct field reads:
  inlining precedes forwarding.
- **No `bx`/`by` at all** — `sq` filled from the cell up front and used for
  the footprint sums — is the same family: VC6 keeps the address-taken
  aggregate in registers until `&sq` escapes at the call, so the stores land
  where they do in the original anyway.
- **`memcpy` for the save AND the restore**, with and without `#pragma
  intrinsic`, is byte-identical to struct assignment: the intrinsic is
  expanded before allocation, so it is not a call boundary that would spill
  `by`.
- **Priority nudges** (a duplicate `sq.y` store, `origin.y` moved last or
  after the x stores, `origin.y = sq.y`, y loaded first, x's stores via the
  aggregate origin) do not move y ahead of x for edx.
- **Scratch probes**: a sibling-field store between the `sq.y` store and its
  byte read, `saved_def` and `sq` as one aggregate, a union byte member, a
  `((unsigned char*)&sq)[4]` view, an `((int*)&sq)[1]` view, a store through
  `unsigned int*`, and a loop-carried `&sq` escape all forward
  (`/tmp/svclear4_probe.c`). A `volatile int` read of `sq.y` is narrowed by
  VC6 to the same byte load as a `volatile unsigned char` read.

The open question, stated so it can be searched: **what source construct
gives `by` a register byte need that the allocator honours, while emitting
that byte as a load from `sq.y`'s home?** Equivalently, a byte use of `by`
that VC6 rematerialises from memory after `by`'s last dword use even though
edx is free. Nothing in the statement space of one basic block does it.

### Fourth pass, continued — the pattern is unique in the binary

- **Binary-wide scan** (`/tmp/svclear4_scan.py`): over the 2,517 exact
  bodies' ORIGINAL code, "dword store to an esp slot from register R, then a
  byte load from the same slot with R not redefined and no call between"
  occurs in **no exact function**. CLEAR's 0x00469dd1/0x00469ddf pair is the
  only instance. The nearest relatives are `LFTrack_Add` (0x0040c780, a call
  between: `Pos p` spilled across `LFPiece_Alloc`, reloaded as bytes — VC6
  narrows a spilled int's reload to the width consumed) and
  `CalculateMapRenderOrder` / `CalculateFullMapRenderOrder` (the register was
  reused). So the reload is a **spill re-materialisation**, not a forwarding
  failure, and the construct that causes it has no sibling in the tree to
  copy from.
- **Where the retained body's `mov esi, edx` comes from.** The aggregate read
  `g_destroy_cursor.origin = sq` after a `volatile` access to `sq` forwards
  each field through a FRESH temp that is not merged with `bx`/`by`; the temp
  coalesces back only if the source is dead afterwards. Without the volatile
  access the aggregate read forwards straight to the coordinate (eight
  spellings, all row four). A volatile read of `sq.x` does not stop
  `(unsigned char)sq.y` forwarding to `dl` (`B5h`: colouring right, x byte
  from memory). A volatile STORE to any of the group's globals (origin,
  `g_sel_def`, a volatile `g_query_block` view) is not a barrier at all.
- **Re-defining `by` through a volatile view of `sq.y`** (`by = *(volatile
  int*)&sq.y;` after the origin store) is a load with full affinity, but the
  colouring still goes to row two because `(unsigned char)bx` — however it is
  spelled — is a register byte need on x.
- **`register`** on the coordinates or on `next`, and the coordinates'
  declaration order, are ignored.

The residual is therefore ONE allocator decision: VC6 hands edx to whichever
coordinate carries a register byte need, and the original hands it to y while
x's byte is served by a copy. Every construct measured that gives y the need
also serves it from `dl`; every construct that serves y from memory removes
the need. `vy_c` (176 instructions, register-blind 2) is the closest body and
is one colouring flip away; the retained 24-strict body keeps the colouring
and mirrors the byte sources. Neither is promotable.

## Fifth pass — the fourth memory kill, and why it does not close CLEAR

Scratch `/tmp/svclear5_*` (`svclear5_bb.c`, `svclear5_agg.c`, `svclear5_split.c`,
`svclear5_trig.c`, `svclear5_scan.py`, `svclear5_variants.py`, `svclear5_[b-f].py`),
scored with the third pass's `svclear3_probe.py`. CLEAR is unchanged at **24
strict, 177i/576B against 177i/577B, first difference at instruction 110**;
whole-file audit PASS with 45 `[OK]` in `eventtick.c` (CLEAR as WIP) and 16 in
`eventgoal.c`, `tools/relocs.py` zero MISMATCH, `/W3` clean. The pass found a
**fourth memory kill** — the first one in four passes that costs no
instruction — and then proved that it does not help, for a reason that also
rules out every remaining construct in the space.

### The fourth kill: a block copy into a sibling member of the same aggregate

Third-pass rule was: VC6 SP3 forwards a local's stored register to a later
narrowing read across anything except a call, a `volatile` access, or a store
through a pointer it cannot resolve. There is a fourth:

> **A block copy (`rep movsd`) whose destination is a member of a local
> aggregate kills the cached value of EVERY member of that aggregate.**

`/tmp/svclear5_agg.c` and `/tmp/svclear5_trig.c` pin the trigger exactly:

| shape | forwards? |
| --- | --- |
| `Pos sq; Big saved;` two locals, `saved = g_big` between store and read | yes (control) |
| `struct { Pos sq; Big saved; } L;` — copy into `L.saved`, read `L.sq.y` | **no** |
| same with `saved` declared first, or the aggregate in a one-element array | **no** |
| same but the aggregate's address is never taken | **no** — address-taken is irrelevant |
| aggregate address-taken but the copy destination is a SEPARATE local | yes |
| `saved` address-taken by a later call, or restored through a `Big*` | yes |
| the copy spelled `memcpy(&saved, &g_big, sizeof)` | yes |
| copy into `*(Big*)buf` with `char buf[6196]`, or through a local `Big*` | yes |
| `union { Pos sq; char raw[8]; }` with the copy in a separate local | yes |

So it is the *shared enclosing object*, not escape and not the pointer
spelling. `P_C1` in `/tmp/svclear5_agg.c` emits the original's exact shape —
`mov [esp+X], eax` / `rep movsd` / `mov [glob], eax` / `mov al, byte ptr
[esp+X]` with eax never redefined — and, because the sibling stored *after*
the copy still forwards, it even reproduces CLEAR's asymmetry: one
coordinate's byte from a register, the other's from its home. This is the
first kill that adds exactly one instruction and no operand the original's
group lacks, and it is a general VC6 alias-analysis fact worth reusing.

### Why it does not close CLEAR: the byte need goes with the byte source

Wrapping `sq` and `saved` in one aggregate in the real function
(`/tmp/svclear5_variants.py`) gives 176 instructions with the byte load
present and the correct group *structure*, but the colouring flips to row two
(x in edx, y in ebp) exactly as `volatile` does — 76 strict, first difference
79. Making the reload a re-definition of `by` itself (`by = L.sq.y;`, so the
byte use stays on `by`'s own web) does not rescue it: six spellings
(`/tmp/svclear5_f.py`) all compile to the identical 76-strict body. The rule
from the fourth pass survives its first non-`volatile` test: **whatever serves
a coordinate's byte from memory removes that coordinate's byte need, whichever
of the four kills produces the load.**

Layout matters too and cannot be fixed. The original's frame is `pass` 0x10,
`saved_def` 0x14, `sq` 0x18, `f` 0x20, `saved` 0x30, so the aggregate that
would reproduce it is `struct { Pos sq; Rect f; Cursor saved; }` — but putting
`f` inside an address-taken aggregate memory-homes all four of its fields and
costs four instructions (181i, 101 strict). With only `sq` and `saved` in the
aggregate, `sq` lands at 0x28 and the two footprint spills move.

### The residual, restated as a register count

Reading the load block sharpens the fourth pass's model. Both coordinates are
`unsigned char` cell fields loaded with `xor edx, edx / mov dl, [edi+4|5]`, so
both have a byte need at their DEFINITION; that need is soft and is satisfied
by using edx as the load scratch and copying out (`mov ebp, edx` at 0x469d5f).
What is hard is the count across the `rep movsd`:

- the copy owns ecx (0x60d), esi (source) and edi (destination), so only
  **eax, ebx, edx and ebp survive it**;
- exactly four values must: `d` (eax, live to the indirect call), `next`,
  `bx`, `by`;
- of those four registers only **eax, ebx and edx are byte capable**, and eax
  is pinned to `d`.

So if both coordinates carry a register byte need, VC6 must move `next` out of
ebx: that is row four, measured directly (`/tmp/svclear5_p0_both_plain.c`,
first difference 19, `next` in ebp from instruction 19, x in edx, y loaded
with `xor ebx, ebx / mov bl, [edi+5]`). The original instead leaves `next` in
ebx and puts x in ebp — a strictly worse assignment for a byte need — and then
pays `mov ecx, ebp` to store `cl`. **The original's x therefore carries no
byte-register need at allocation time, yet its byte is emitted from a register
copy.** That is the whole residual, and it is now a falsifiable statement
rather than a preference.

### The five directions, measured

1. **End `by`'s register phase at the origin store.** Dead byte uses that
   would leave a need behind are all eliminated before allocation: a doubled
   `g_sel_bpos.b.y` store (either order), a dead `unsigned char` local fed
   from `by`, and a folded truncating comparison all compile byte-identically
   to the plain volatile body (`/tmp/svclear5_b.py`, 176i/71 strict each).
2. **An x byte need the colouring does not see.** Reading the value back out
   of the global that was just written — `(unsigned char)g_destroy_cursor.origin.x`,
   `*(unsigned char*)&...origin.x`, both with and without the aggregate origin
   store, and the same for y — forwards completely: six variants
   (`/tmp/svclear5_e.py`), all byte-identical to the plain row-four body at
   108 strict. VC6 forwards a global store to a same-address load in the block,
   so a global read-back is not a memory operand at allocation.
3. **Register pressure at the copy.** A fifth value live across it (`inst =
   d->inst` named early or in the group, `Pos* sp = &sq`, both together)
   changes the frame and costs two instructions without producing a spill or a
   fix-up (`/tmp/svclear5_c.py`, 176i/109-110 strict, first difference 0).
   With four values and four surviving registers the allocator is exactly
   saturated, never over-subscribed, so there is nothing to spill.
4. **Scratch characterisation.** Besides the fourth kill above: a basic-block
   boundary between the store and the read is **not** a kill — an `if`/`endif`,
   a read inside a conditional, a store hoisted above a short-circuit
   condition chain, a loop, identical arms, a `goto` join, and the full CLEAR
   shape with the copy inside the conditional all forward
   (`/tmp/svclear5_bb.c`, nine cases). Neither is a copy ever un-coalesced:
   `tx = bx` with the source redefined dead, redefined live, defined in
   identical arms, or written `-(-bx)` / `bx ^ (v & 0)` all propagate, and so
   does a copy whose source is read after the temp's byte use — VC6 never
   needs two registers for two names holding the same value
   (`/tmp/svclear5_split.c`, eight cases). **A byte need can therefore never be
   moved off the coordinate's own web by a copy.**
5. **Asymmetric coordinate types** (unmeasured before; both-narrow was
   measured in pass four). `unsigned char by` with `int bx` is the only
   variant in the family that reaches 177 instructions WITH the byte load, but
   the byte local takes a stack home, the frame grows to 0x1858, the footprint
   block needs `and ebp, 0xff`, and the colouring is still row two: 40 strict
   (39 with the y byte read from `sq.y`). `char`, `short` and `unsigned short`
   on either coordinate cost 155-161 strict (`/tmp/svclear5_d.py`).

### The fix-up copy is unique in the binary too

`/tmp/svclear5_scan.py` scans all 2,517 exact bodies for
`mov <byte-capable r32>, <ebp|esi|edi>` followed by a use of that register's
low byte. Four hits: CLEAR's own `mov ecx, ebp` / `mov byte ptr [0x667c54],
cl`, two `mov ecx, edi` / `shl eax, cl` (a shift-count copy, forced by the
instruction, `JcBoat_Step` and `BsBoat_StepLeg`), and `PaintPathRect`
0x0045cb20's `mov edx, esi` / `and dl, 1` — a copy forced because `and`
destroys its operand and esi is the loop-carried `x + y`. **No exact function
contains a byte-class fix-up copy of the kind CLEAR needs**, which matches the
fourth pass's finding that the store/byte-reload pair is also unique. Both
halves of the residual are single occurrences in the tree, so there is no
sibling to copy a spelling from.

### State and what a sixth pass should attack

The committed body is unchanged and remains the best measured: 24 strict,
177i/576B, footprint block and load block exact, one `volatile` byte probe on
`sq.x`. `vy_c` (176i, register-blind 2) is still the closest structure and
remains one colouring flip away.

Anyone reopening CLEAR should not look for a fifth memory kill — the fourth
one is free and still loses the colouring — and should not look for a way to
split the x web, because copies of equal values provably never survive. The
one unexplored hypothesis left is the allocator's *priority order*: the
original's assignment is what a priority-based allocator produces if `next` is
coloured before x, and row four is what it produces if x is coloured first. If
some source change can lower x's web priority below `next`'s without changing
the emitted references — the reference counts of the two coordinates are
symmetric in every spelling tried — the fix-up copy should appear on its own.

### Sixth check — a late-dropped mask as the x byte source (integrator, 2026-09-07)

The fifth pass's reading — x carried no byte need at allocation, its byte is a
late byte-class fix-up copy — suggests a dword operation on `bx` that VC6
discards after allocation once the store is narrowed. Measured
(`/tmp/svclear6_*`): `(unsigned char)(bx & 0xff)` and `(unsigned char)(bx |
0x100)` are folded to `(unsigned char)bx` by the front end (identical to
`vy_c`); a named `mx = bx & 0xff` keeps the `and` (580 bytes, 34 strict);
`(unsigned char)((bx << 8) >> 8)` costs two instructions. No dword expression
on a coordinate survives to allocation and dies afterwards. The residual
stands as the fifth pass stated it.

### Sixth check, continued — the tie-break at instruction 79 (integrator)

Two ways the colouring could be moved without touching the emitted code were
measured negative (`/tmp/svclear6_*`):

- **A register made to look busy at 79–83** so that y cannot transit through
  ecx and x is evicted to ebp as in the original: reading `saved_def =
  g_sel_def` earlier in the block moves its load to where it is read (first
  difference 73–79, no sinking); keeping `c->obj` live for the query call
  costs a reload and the frame (182 instructions).
- **Pinning `next` in ebx while both coordinates need bytes**, which would
  give y edx and x ebp with the copy: declaration order of `next`/`c`, a
  `while` loop with the explicit `c = next`, `next = c; next = GetNext(next)`
  in case 2, and `default:` for case 2 all leave row four (or worse); the
  row-four body is robust to how `next`'s web is spelled.

Current best bodies are unchanged: the retained 24-strict body (colouring
right, byte sources mirrored) and `vy_c` (structure right, colouring flipped).
