# Scope PORT-M13 — "the game keeps telling me to link the Space Tower"

> **PORT-M13 — Status: IN PROGRESS (claimed 2026-09-12 by PORT-M13)** — branch
> `scope/PORT-M13`, cut from the PORT-M12 merge (`f4b81cc3`). Matching side
> plus the shim. Brief: `docs/SCOPE_PORT_WAVE.md`.

**Result in one line: the LINK goal is NOT broken. It tests exactly ONE
square — the square the ride's entrance arrow points at — and it wants a path
there that is joined, side to side, to the park entrance. Every connected path
shape this lane laid by hand satisfied it; every disconnected one did not.
What IS broken, and is fixed here, is the shim's mouse: the game's cursor was
losing a fifth of every movement whenever the canvas is not displayed at
exactly 640x480, so a drag ends short of where the player aimed.**

---

## 1. The rule, as recovered

`EventTick_Link` (`LEGOLAND/eventtick.c:939`, `0x0046a750`) is the whole goal.
For each placed instance of the named class — or, for `LINK ALL`, for every
rendered object whose class `IsLinkableClass` accepts (`eventtick.c:916`: type
1, 4 or 5) — it computes

```
link square = instance base square (Inst +0x0e/+0x0f)
            + the CLASS's entrance offset (ObjDef +0x0c/+0x10)
```

and then

* if `CellAt(link square)` is off the map -> `outside`: `GoalCheck_Connect`
  queues the "connect" hint and the handler returns 0. **For ever** — this is
  the arm PORT-M10's corrupted build-slot tile fell into.
* else if `TileJoinsPathNetwork(link square)` is false -> `missing`:
  `GoalCheck_Link` queues the "link" hint and the handler returns 0.
* the goal is met only when EVERY instance passes.

`TileJoinsPathNetwork` (`pathmisc2.c:179`, `0x00482b60`) is two tests:

1. `FindPathSquare` (`pathsq.c:67`, `0x00481790`) — is there a **PathSquare**
   whose inclusive rect contains that cell? Path squares are the maximal
   walkable rectangles chained from `0x0066b44c`;
2. that square's **flag 2** — "reachable from the park entrance".

Flag 2 is not a property of the square; it is re-flooded wholesale by
`ResolveEntrancePathSquare` (`pathmask.c:128`, `0x00482a40`), which clears
flag 2 on every square and then flood-fills it out from the square containing
the PARK ENTRANCE's own walk-to tile through `MarkPathSquareReachable`
(`pathmask2.c:144`, `0x004829c0`). The park entrance's tile is cached by
`UpdateEntranceTile` (`objdoor.c:260`, `0x00482a90`) as the middle of the
`ENTRANCE 1` footprint's LEFT edge, one square clear. The refresh runs when
forced or when 4000 ms have passed (`tinystubs.c:364`), and
`UpdateHelpTick` forces it on any frame where `g_map_dirty & 0x10` says a path
changed (`fpui3.c:662`) — so laying a square takes effect the same frame, not
after a 4 s wait. Measured: the goal flips within one tick of the drag.

**The flood fill is four-connected, and corners do not count.**
`CollectPathSquareNeighboursCounted` (`pathobj2.c:268`, `0x004819a0`) scans
exactly four lines around a rect — the row above, the row below, the column
left, the column right — so a path square that meets another only at a
diagonal corner is not its neighbour. This is the trap that looks worst on an
isometric screen, where two squares that share a corner look joined.

### The Space Tower's own numbers

From `SPACE TOWER RIDE.ODF` in `gamedata/disc/Legoland.res`, confirmed against
the running heap:

| field | value |
| --- | --- |
| class type (+0x20) | 1 — linkable |
| entrance offset (+0x0c/+0x10) | **(+2, −3)** |
| footprint rect (+0x3c) | (−2, −2, 4, 3) |
| exit offset (+0x24/+0x25) | (−2, +4) |

So with the ride's base at (63, 35) the link square is **(65, 32)** — one row
NORTH of the footprint, at column base+2. Its cell's door bits (Cell +0x12)
read **2**, and `docs/runtime/world.md` gives the direction bits as
right/left/bottom/top = 4/8/1/2: 2 is TOP. **The square the goal tests is the
square the entrance arrow on screen points at.** The exit arrow, on the far
side, is not tested by anything.

### Why a freshly built ride is normally linked already

Placing a buildable class runs `RefreshObjList(&g_obj_list)` before
`BuildObject` (`workers2.c:605` -> `workorder2.c:1176`, `0x0045d770`). That
function takes the edit-cursor chain's flag-0x1000 cursor — the footprint plus
`CheckCursorFootprint`'s **one-cell clearance ring** — subtracts every other
cursor's rect (the footprint itself), and stamps what is left as REAL path:
`ClearCellForPath`, `AddPathTileGFX`, `AddPathSquare`. The inset interior gets
path GRAPHICS only, with no path square, which is why the ride appears to
stand on a paved pad.

The ring is the footprint expanded by one square, and every linkable class
shipped with the game puts its entrance offset on that ring. Checked over all
46 type-1/4/5 ODFs in the archive: **42 of 46 have the entrance exactly on the
ring**; the exceptions are listed in §5.

So the entrance square is essentially always covered by a path square the
moment the ride is built, and the only thing that can be missing is flag 2 —
the join to the rest of the park. Measured twice: when the new ring lands
exactly ONE square away from existing path, the game bridges the gap with a
single extra square ((55,32) and (59,35) in two placements). That single
bridge is the "Did you notice we also built a path from the ride to the park
entrance?" the tutorial's `first ride.txt` interval boasts about. **Two
squares away and there is no bridge**, and the player has to lay it.

### The script line the player is quoting

`ObjList1.txt` in the archive, the tutorial-1 script:

```
####### Space Tower ride must stay linked
[OBJECTIVE]
	[PERMANENT]
		INTRO  "You forgot to link the Space Tower Ride with a path."
		PROMPT "Click the PATH button and then link the Space Tower Ride to the main path."
		LINK "SPACE TOWER RIDE"
```

`[PERMANENT]` means it is re-checked until the level ends, so it comes back
the moment the link is broken — which is correct, and is what the player saw.
Note also that it names the CLASS, not one ride: a second Space Tower built
somewhere else and left unlinked keeps the same prompt on screen with nothing
to say which ride it means.

---

## 2. What was measured

Driven through the page on the committed wasm build,
`legoland.html?args=-nointro+WINDEBUG` on 8812, with two new page readers
(§4). The park is PORT-B10's replay, then the ride's pad is cut off from the
main path by erasing the nine squares of row 31 above it. Ring = x 60..68,
y 32..39; entrance square E = (65, 32); the main path is row 31 and the
columns at x=44 and x=77.

| # | what was laid, by hand, with the PATH tool | link square's PathSquare | flag 2 | verdict | LINK event flags | reachable squares |
| --- | --- | --- | --- | --- | --- | --- |
| 0 | nothing — the ride as the tutorial leaves it, on its scripted pad | `[60,32,68,32]` | yes | **SATISFIED** | `0x82` | 8 of 12 |
| G | the pad cut off. Row 31 still meets the pad at its two top CORNERS, (59,31)/(60,32) and (69,31)/(68,32) | `[60,32,68,32]` | **no** | MISSING | `0x02` | 5 of 13 |
| N1 | one square at (65,31) — on the entrance side, right at the arrow, but not reaching the main path | `[60,32,68,32]` | no | MISSING | `0x02` | 5 of 14 |
| N2 | a second drag (64,31)->(60,31) closes the gap | `[60,32,68,32]` | yes | **SATISFIED** | `0x82` | 9 of 13 |
| E | a straight run down the pad's EAST side, (69,32)->(69,36), from the main path — the side AWAY from the entrance | `[60,32,69,32]` | yes | **SATISFIED** | `0x82` | 10 of 14 |
| W | a straight run up the pad's WEST side, (59,39)->(59,32) | `[59,32,68,32]` | yes | **SATISFIED** | `0x82` | 9 of 13 |
| S1 | a run along the pad's SOUTH edge, (64,40)->(60,40) — touches the pad, reaches no main path | `[60,32,68,32]` | no | MISSING | `0x02` | 5 of 14 |
| S2 | the L closed: (59,40)->(59,31) up the west side. The path **never touches the entrance side at all** | `[59,32,68,32]` | yes | **SATISFIED** | `0x82` | 10 of 14 |

Every "SATISFIED" row is the same event going from flags `0x02` (live and
unmet) to `0x82` (met this step), the pending-event list advancing past it,
and — on the first one — a fresh kind-37 event for `LEGO SHOP 1` appearing,
i.e. the tutorial moving on to objective 2. `g_visitor_count` held at 4 the
whole time, which is the level's own cap (`MAXBLOKES 5`, `visitorLimit` 4).

Two placements at other spots, both on open grass well away from the scripted
pad, both **SATISFIED the moment the ride finished building**, with the ring
and its one-square bridge laid by the game: base (53,36) -> ring x 50..58
y 33..40, bridge (55,32); base (53,37) -> ring x 50..58 y 34..41, bridge
(59,35).

### The rule proved on the nose

The table above never separates "the entrance square" from "the pad", because
the pad always covers the entrance square. Two direct probes close that, by
poking the class's entrance offset in the live heap (a diagnostic write, no
code change) while the pad stayed fully joined to the park on all four sides:

| entrance offset | link square | what is on it | verdict |
| --- | --- | --- | --- |
| (+2, −3) — shipped | (65, 32) | the pad's ring, flag 2 | **SATISFIED** |
| **(+2, +5)** — poked one row south of the ring | (65, 40) | plain grass, cell flags 0 | **MISSING** |
| (+2, +5), then ONE path square laid by hand at (65,40) | (65, 40) | its own PathSquare `[65,40,65,40]`, flag 2 | **SATISFIED** |
| (+2, −3) — restored | (65, 32) | the ring | **SATISFIED** |

The middle row is the whole finding: a ride standing on a fully connected
paved pad, with path on all four sides, **fails** the goal when the one square
its entrance points at is not path; and one square laid on that spot — joined
to the network only through the pad beside it — passes it. The goal follows
the entrance offset, not the ride.

---

## 3. The verdict, and the one real defect

### 3a. The reported behaviour is the game's own rule

**Not a port defect.** `EventTick_Link`, `TileJoinsPathNetwork`,
`FindPathSquare`, `ResolveEntrancePathSquare`, `MarkPathSquareReachable`,
`AddPathTile`/`AddPathSquare`/`PathSquareAdded`, `RefreshObjList`'s ring
stamping and `ScreenToMapRef` all behave exactly as the recovered sources say,
and the sources match the original (PORT-M10 checked `EventTick_Link`'s link
square against the disassembly at `0x0046a77c`–`0x0046a78e`). A player whose
path stops one square short of the arrow, or reaches the pad but not the park
entrance, or meets the pad only at a corner, gets the prompt — and is meant
to.

The five suspects the brief listed, closed one at a time:

| suspect | result |
| --- | --- |
| (a) overlap hazards in the path/route/flood-fill code | `portable/tools/cdecl.py --overlaps` reports **three** pairs across `pathsq.c`, `mappath.c`, `pathmisc*.c`, `pathbuild.c`, `objdoor.c`, `goalstate.c`, `eventtick.c`, and **none is on the LINK path**: `eventtick.c` `g_obj_list`/`g_view` is inside `PlaceScriptObject`, `pathobj2.c` `g_edit_changed`/`g_edit_object` is inside `SetEditObjectFromElem`, and `goalstate.c` `g_level_cfg`/`g_map` share an address with no function touching both. Nothing to probe at -O2. |
| (b) a by-value/return-by-value struct on the goal or path path | `portable/tools/bvstruct_sweep.py` **0 unaccepted silent sites** (the one silent site is A9's accepted `LegoMedia_Remove`); `tools/port_m10_bvstruct_sweep.py` **0 silent, 0 slot-vs-body**. `EventTick_Link` and everything it calls take pointers and ints. |
| (c) M5's rounding sites in the cursor-to-tile conversion | `ScreenToMapRef` (`objmap2.c:650`, `0x0045be90`) is **integer only** — two `/` and two `%` by the tile size and a four-way quadrant test; there is no float, no `fistp`, and no truncate-vs-round choice anywhere on the path. Reimplemented in JS from the recovered source and checked against `g_input.map_x/map_y` over dozens of pixels: **every pixel the shim actually delivered mapped to the cell the game recorded**, with no exception. |
| (d) a split record or an unbounded table in the path/door data | `g_path_square_neighbours` `0x0066a45c` is planned at **1020 words** by `gen_link.py` (A9's whole-element tiling), and the four scans of `CollectPathSquareNeighboursCounted` can add at most 4x84 = 336 entries on this 84x84 map. `g_entrance_tile` `0x0066b460` appears in `extents.md` correctly merged as an 8-byte `Pos` (four TUs agree, `highlevelai.c`'s 4-byte `g_entrance_tile_x` is the interior member). `rawwords.md` flags nothing in this family. |
| (e) the shim's mouse-position mapping | **THE DEFECT.** §3b. |

### 3b. The port defect: the shim's relative mouse lost a fifth of every move

`portable/src/browser/ll_canvas.js` differences the absolute pointer into the
`DIMOUSESTATE` lX/lY deltas the game reads. It rounded each delta
independently while remembering the REAL pointer position:

```js
var dx = Math.round(x - LL.lastX);
if (dx || dy) LL.push(LL.EV_MOUSEMOVE, dx, dy, 0);
LL.lastX = x;                    /* <- the remainder is thrown away */
```

`x` is an integer only when the canvas is displayed at exactly 640x480. The
canvas carries `max-width: 100%` and no width, so any window narrower than the
canvas plus the log panel shrinks it, and a page zoom does the same. In this
session it measured **508 CSS px — a scale of 0.79375**, so one client pixel is
1.26 game pixels and every event dropped 0.26 of a pixel on the floor.

Measured by sweeping the pointer one game pixel at a time and reading
`g_input.point` (`bighelp.c:39`, the cursor pixel the game actually has):

```
asked   300 301 302 303 304 305 306 307 308 309 310 311 312 313 314 315 316
before  300 300 301 302 303 304 304 305 306 307 308 308 309 310 311 312 312   12 of 16
after   300 300 301 302 304 305 305 306 307 309 310 310 311 312 314 315 315   15 of 16
```

The old line loses a pixel in four and **never catches up** — the gap grows
monotonically 0,1,1,1,1,1,2,2,2,2,2,3,3,3,3,3,4. The fixed line tracks with a
bounded error of at most one pixel and self-corrects (302 is one short, 304 is
exact again). A 600-move walk out and back returns `g_input.point` to exactly
(320, 240) after the fix; before it, an aiming loop that corrected by the
isometric basis oscillated and could not converge on a named square at all.

Sweeping the pointer 200 pixels across the map arrived about 50 pixels short,
which on a 32x16 isometric tile is **three squares**. The same line had a
second failure: a move smaller than half a game pixel was swallowed whole, so
on a canvas scaled past 2x the cursor did not move at all.

The fix accumulates the DELIVERED delta instead of re-anchoring on the real
pointer, so the remainder survives to the next event:

```js
if (dx || dy) LL.push(LL.EV_MOUSEMOVE, dx, dy, 0);
LL.lastX += dx; LL.lastY += dy;
```

**How much of the report this explains.** The game hides the OS pointer while
it draws its own (`ll_js_set_cursor`), so a human aims with the game's cursor
and the drift does not by itself put a path square on the wrong tile. What it
does do is make the cursor crawl behind the hand — badly enough that a drag
across the map ends three squares short of where it was aimed and that
edge-scrolling fires at the wrong moment — and the goal it feeds is a
one-square test. It is a real defect on the path the report describes, and it
is the only one in the port that this lane found. It is fixed.

### 3c. What to tell the player

Written for the user, in §6.

---

## 4. Two page readers, and eleven names

`portable/**` was read-only for PORT-M10, so that lane had to find the path
squares and the goal event by SCANNING the heap. This lane may edit the shim,
so `LL_DBG_TABLE` in `portable/src/browser/main.c` gained the eleven globals
the question is actually about — `g_path_squares`, `g_goal_list`,
`g_script_event`, `g_entrance_tile`, `g_entrance_tile_time`,
`g_path_gfx_batch`, `g_entrance_elem`, `g_cursor_mapref`, `g_edit_object`,
`g_map_dirty`, `g_input` — and `index.html` gained two readers built on them:

* **`llLink(name)`** replays `EventTick_Link` per instance and reports every
  intermediate: the class's entrance offset, type and footprint, the link
  square, the Cell there (flags and door bits), the PathSquare covering it and
  its flags, the verdict, the whole path-square list with its reachable bits,
  the live event list (where the kind-37 LINK goal sits), the notepad's unmet
  objectives, the park entrance's cached tile, and the cursor as the GAME sees
  it. Defaults to `SPACE TOWER RIDE`.
* **`llPathMap(cx, cy, r, name)`** draws the path CELLS around a square as
  ASCII (`=` path, `#` object, `E` the link square, `P` the park entrance's
  tile). A one-square gap in a path is invisible on a 640x480 isometric canvas
  and obvious here.

`g_script_event` (`0x00668784`) and `g_goal_list` (`0x00668728`) are two
different lists and this lane needed both: `UpdateHelpTick` (`fpui3.c:646`)
walks `g_script_event`, which is where the LINK goal itself lives and where
its flags say met/unmet; `g_goal_list` is what the GoalCheck_* primitives feed
and what the notepad renders. Reading the first one is how every row of §2's
table was taken.

---

## 5. Two things found on the way — for the integrator, not fixed here

**5a. Four linkable classes whose entrance is not on their clearance ring.**
Every ride's entrance offset is meant to land on the one-square ring
`RefreshObjList` paves, which is what makes a freshly built ride linked. Of
the 46 type-1/4/5 classes in the archive, four do not:

| class | type | entrance | footprint | what it means |
| --- | --- | --- | --- | --- |
| `WATER WORKS SHOWER` | 1 | (−3, −3) | (0,0,0,0) | three squares clear of a single-cell footprint — the ring does not reach it |
| `dschool.odf` | 4 | (−2, +4) | (0,0,0,0) | same shape |
| `WATER WORKS ELEPHANT FOUNTAIN` | 1 | **(0, 0)** | (−3,−3,5,5) | the entrance is the BASE CELL, inside the footprint. `ObjHasEntrance` (`objdoor.c:190`) rejects it, but `EventTick_Link` does not consult `ObjHasEntrance` — it uses the raw offset. The interior of the pad gets path GRAPHICS and **no path square**, so `FindPathSquare` finds nothing there |
| `WATER WORKS WATER BLOCK` | 1 | **(0, 0)** | (0,0,0,0) | same |

Nine shipped scripts carry `LINK ALL` (`ObjList6,7,8,9,10,11,12,14,15.txt`),
which is `EventTick_Link`'s `e->elem == 0` arm: it walks every rendered object
and checks every class `IsLinkableClass` accepts. On those levels a player who
builds one of these four could hold the goal open with a ride that has no
square to link. Whether the original shipped with the same numbers is not in
doubt — these are the archive's own ODF bytes — so this is an original-data
hazard rather than a port defect, but it is worth a check on a `LINK ALL`
level before anyone calls those levels completable.

**5b. The eraser over the pad took the RIDE, twice.** Erasing single squares
of the main path works exactly as expected. Two eraser clicks aimed (and
verified through `g_input.map_x/map_y`) at a plain path square of the ride's
own pad removed the whole ride and its pad instead, refunding it. A drag in
eraser mode is a rectangle selection (`UpdateMapDrag`, edit mode 2) and is
expected to take what it covers, but the single clicks are not. The place to
look is `BasicObjectDCalcCursor` (`objmap2.c:506`, `0x00480bb0`): off an
object cell it leaves the destroy cursor on the pointed square **but gives it
`g_sel_def->rect`**, the last selected class's footprint — which, right after
building a Space Tower, is 7x6. This lane did not chase it: it is a separate
report, it is not what the player described, and it deserves its own
measurement of `g_sel_def` and `g_destroy_cursor`. **Recommended as PORT-M14.**

---

## 6. For the user: how to link a ride, and why the prompt was right

> A ride is "linked" when **one particular square** is paved and that paving
> joins up, side by side, all the way back to your park's entrance.
>
> The square is the one the ride's **entrance arrow** points at — not the ride
> itself, and not the exit arrow on the other side. For the Space Tower it is
> the square just above the ride's paved pad, two squares right of the middle.
> The game checks that one square and nothing else.
>
> Three things follow, and they are the three ways to get stuck:
>
> 1. **The path has to reach the park entrance, not just the ride.** A path
>    that runs up to the ride but stops in the middle of the grass counts for
>    nothing, however long it is. Follow it back with your eye: it has to join
>    the grey paths that lead to the gate.
> 2. **Corners do not join.** Two paths that meet only at a diagonal corner
>    are two separate paths as far as the game is concerned, even though on
>    the tilted view they look like one. Paths join edge to edge — up, down,
>    left, right — and that is all.
> 3. **One missing square is a break.** A single unpaved square anywhere along
>    the route breaks it, and at this zoom a one-square gap is very hard to
>    see. If the prompt will not go away, walk the path back from the ride to
>    the gate looking for the gap rather than laying more path.
>
> The good news is that you normally do not have to do any of this: when you
> build a ride the game paves a border right round it, which always includes
> the entrance square, and if that border lands **one square** away from an
> existing path it bridges the gap for you. It is when you drop the ride two
> or more squares clear of every path that the job becomes yours — and then
> all you need is a line of path from any edge of the ride's paved border back
> to the main path. It does not have to arrive on the entrance side; the
> border round the ride carries the connection the rest of the way.
>
> One last thing, because the prompt does not say it: the objective names the
> RIDE TYPE, not one ride. If you have built two Space Towers and only linked
> one, the prompt stays up and points at neither.

---

## 7. Gates

No `LEGOLAND/*.c` file was touched, so the per-file `audit.py` and
`relocs.py` rows cannot have moved; both are reported tree-wide instead.

| gate | result |
| --- | --- |
| files changed vs `f4b81cc3` | `docs/SCOPE_PORT_WAVE.md`, `docs/lanes/scope-port-m13.md`, `portable/src/browser/index.html`, `portable/src/browser/ll_canvas.js`, `portable/src/browser/main.c` — **no `LEGOLAND/*.c`** |
| `tools/relocs.py --all \| grep MISMATCH` | **empty** |
| `tools/progress.py --check` | **3281 exact / 42 WIP**, 665/675 exports (98.5%) — unchanged, no report regeneration needed |
| `portable/tools/extern_sweep.py` | clean — "the class is closed" |
| `portable/tools/bvstruct_sweep.py` | **0 unaccepted silent sites** (1 silent, accepted; 5 noisy) |
| `tools/port_m10_bvstruct_sweep.py` | **0 silent, 0 slot-vs-body** (6 noisy / size-unknown, all pre-existing) |
| wasm: `emcmake cmake` + `ninja` + the seven extra targets | exit 0, **no wasm-ld warning of any kind** |
| wasm `ctest` | **24/24 passed** |
| native: `cmake` + `ninja` + `legoland_tests legoland_cbtypes` + `ctest` | **17/17 passed** |
| the page, `?args=-nointro+WINDEBUG` on 8812 | 3 493 frames, `traps: []`, `dead` null, 33.5 fps (avg 33.5), the tutorial past objective 2 |

---

## 8. Files touched

| file | change |
| --- | --- |
| `portable/src/browser/ll_canvas.js` | **the fix** — the relative mouse accumulates the delivered delta instead of re-anchoring on the real pointer |
| `portable/src/browser/main.c` | `LL_DBG_TABLE` + 11: `g_path_squares`, `g_goal_list`, `g_script_event`, `g_entrance_tile`, `g_entrance_tile_time`, `g_path_gfx_batch`, `g_entrance_elem`, `g_cursor_mapref`, `g_edit_object`, `g_map_dirty`, `g_input` |
| `portable/src/browser/index.html` | `llLink(name)` and `llPathMap(cx,cy,r,name)` |
| `docs/SCOPE_PORT_WAVE.md`, `docs/lanes/scope-port-m13.md` | status line, these notes |

No `LEGOLAND/*.c`, no `tools/verify.py` run.
