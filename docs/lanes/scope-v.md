# Scope V — event tick handlers and goal checks: 61 of 62 exact

**Status: in progress, 2026-09-07.** Recovered the interrupted session's
three saved exact fixes (`EventTick_Lookat`, `EventTick_Connect`, and
`EventTick_Link`). A fresh whole-file audit reports 61 exact functions and
one WIP, `EventTick_Clear` (172i/560B versus 177i/577B, 120 strict mismatches).
The two files compile cleanly at `/W3`. Relocation checks resolve 62 positions
in `eventgoal.c` and 125 in the 45 exact `eventtick.c` bodies, all agreeing;
ten eventtick references remain unresolved (literals, a local jump table,
and the annotated function-pointer array that the parser does not recognize).
No new Clear variant has passed the authoritative gate, and none has replaced
the saved baseline. This scope is not complete and has not been integrated.

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
| 0x00469c80 | `EventTick_Clear` | 172 / 560 vs 177 / 577 | **WIP** | 139 → 120: see below |
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

## The four WIP bodies (all at the original's instruction count)

Each is a register-allocation residual with the code shape already the
original's; every spelling the disassembly could justify was measured and
none moved the assignment. They are worth a fresh eye, not more permutation.

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
$PY tools/audit.py LEGOLAND/eventtick.c   # 42 x [OK], 4 x [WIP ], PASS
$PY tools/relocs.py LEGOLAND/eventgoal.c  # 0 MISMATCH
$PY tools/relocs.py LEGOLAND/eventtick.c  # 0 MISMATCH (10 UNRESOLVED literals)
ALPHATEAM_VC6_ROOT="$PWD/toolchain" "$LEGOLAND_CL" /nologo /c /W3 /O2 /Gy /Gd /Fo/tmp/sv_w3.obj LEGOLAND/eventgoal.c
ALPHATEAM_VC6_ROOT="$PWD/toolchain" "$LEGOLAND_CL" /nologo /c /W3 /O2 /Gy /Gd /Fo/tmp/sv_w3.obj LEGOLAND/eventtick.c
```
