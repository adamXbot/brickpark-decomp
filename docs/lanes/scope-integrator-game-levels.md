# Integrator pass: reaching the game levels (2026-09-14)

Goal: play the campaign ("game") levels in the browser for the first time, and
run the three quirk A/Bs free play could not reach (Q7 Boating School, Q11
Jungle Cruise, Q14 coaster support shadow). Builds: default and
`-DLL_FAITHFUL=ON` wasm, served on fresh ports (a reused port serves a cached
module).

## 1. How to reach a game level

- `ObjList6..15.txt` in `gamedata/disc/Legoland.res` are game levels 1..10
  (`MAP "GLONE"` .. `"GLTEN"`); `ObjList1..5` are the lessons. The profile's
  fifteen bytes at `Profile%d.txt +0x34` are `g_level_done[0..14]`.
- Fastest route: from any park, press Shift and type `ILIKETOTRAVEL0N`
  (game level N = 1..9), `ILIKETOTRAVEL10`, or `ILIKETOTRAVELTn` (lesson n).
  `P4C.cheat(word)` in `replays/p4-04-cheats.js` does it. The cheat sets
  `g_level_rec->level` and `g_pending_state = 2`; the frame loop then drops to
  the game-level progress screen (Professor Voss, three bubbles).
- Progress screen: Accept = red bubble, click (582, 417). The briefing notepad
  follows; Thumbs Up at (577, 419) starts the park.
- Rides per level (GIVE/LOAD/PLACE in the object lists): the coaster and log
  flume first appear in game level 2; the Jungle Cruise in game level 6; the
  **Boating School in Lesson 5** (pre-PLACEd at load) and game level 9.

## 2. The crash: six of ten game levels died on Accept

Game levels 2, 4, 6, 7, 9 and 10 give the Catapult. Loading its class trapped:

```
RuntimeError: memory access out of bounds
EnsureObjectClassLoaded -> LLIDB_LoadData -> Catapult_Create -> Load_FXList
  -> siprintf -> vfiprintf -> printf_core -> strnlen -> memchr
```

Symbolised with `ninja -C portable/build-wasm legoland_browser_named`
(`legoland_dbg.wasm`, same function numbering and body sizes as
`legoland.wasm`) and a name-section parse.

Cause: `catapult.c` declared the FX table `extern void* g_catapult_fx;` with the
comment "(4 entries)". No other file sized it, so the closure emitted 8 bytes
where `Load_FXList` walks 4 x 12, and `g_catapult_sample` (0x004b40d0, entry
0's `+0x08`) became a separate 112-byte block. Entries 1..3's name pointers
were read from foreign storage. A sweep of every `Load_FXList`/`Kill_FXList`
table found one more undersized table: `g_entrance_fx`, sized 8 by a wrong
local 8-byte `FXEntry` in `screencb5.c`; its sample slot is `ridecb7.c`'s
`g_entrance_pay_sample` (0x004b6670).

Fix, both builds: `catapult.c` declares `extern FXEntry g_catapult_fx[4]` with a
local 12-byte typedef; `screencb5.c`'s typedef is 12 bytes with the sample at
`+0x08`. The closure now emits one block each with the sample names as interior
aliases at `+0x8`. VC6: audit rows identical (23 and 10), relocs 0 MISMATCH.
Game levels 1 and 6 load and run on the fixed build (visitors, income, no trap).

This is a new silent class — undersized table storage — with no gate. A gate
lane is owed: for every `(table, count)` walker with a known record size, the
emitted extent must be at least `count * size`.

## 3. Q7 measured (Lesson 5)

Lesson 5's pre-placed Boating School runs `BoatingSchool_Add`. The station
record is `frame +0x0c`, `backwards +0x10`, `next +0x2c`; head
`g_bs_stations`. The animation is **one-shot**: count 0..100 reversed, reset
with the flag set, one forward pass, then idle.

| | default | faithful |
| --- | --- | --- |
| seed | 0 | 9999 |
| direction flag | 1 (flipped at 100) | 0, never flips (357 frames) |
| counter observed | 306 at sim 1181 | 11575 at sim 2352 |

A canvas diff over the building read 0 even after re-arming the counter; the map
layer is dirty-redrawn, so that reading is inconclusive, not negative.

## 4. Q11 and Q14: where the drive stopped

Game level 6 on the fixed build: the Small Power Station objective drained and
the script gave the Jungle Cruise, its water and the Temple. Station footprint
(-2,-7)..(3,7), price 55; water price 5; dock squares at station +(2,2) and
+(2,-3). Map 119 x 102.

Placement then stopped working. In order:
1. an in-park REPORT notepad (screen mode 7) swallowed every click while the
   cursor still read valid; it closed after clicks at (525, 370);
2. a "You have a new object" pop-up (Temple) stayed up although
   `P5.modalUp()` said clear; its red X is at (556, 243);
3. with both gone, a valid Shrub cell still would not place and money froze.

Not diagnosed. Start the next attempt from a fresh load of game level 6, clear
every new-object pop-up by its X first, and check `g_ui_flags` (no debug row
yet) for the in-game-clicks bit before placing.

Q14 needs elevated track: supports only draw a shadow when `pos.z <= -0.1`, and
the height pieces (`SQUARE_TRACK_HEIGHT*`) are given late in game level 6. The
coaster classes are loaded but not offered.

## 5. Tooling notes

- Reading any unexported `Module.X` in this build calls `abort()` and kills the
  runtime. Use `llAddrs()` and the heap views.
- `PORT-P4`'s claim commit 5775185c removed `llFind`/`llClasses`/`llMapObjects`
  from `index.html`; the page still has `llClasses` from a loaded replay.
- Debug rows 152..156 added: `g_shadow_v`, `g_shadow_src`, `g_level_rec`,
  `g_pending_state`, `g_cur_profile`.

## 6. Q11 resumed (2026-09-14, afternoon)

**Why placement stopped earlier.** Three separate things, none a port defect:
the in-park REPORT screen swallowed clicks; the "You have N new objects" pop-up
swallowed clicks, and `P5.modalUp()` does not see it (read its count instead:
`g_newobj.count` at `g_popup_info + 0x14 + 0xa0`, 0 when closed; the red X is at
(554, 243)); and a window blur while the clock was already frozen leaves it
frozen for good (`LegoLandWindowProc` thaws on `WM_SETFOCUS` only if it was not
frozen at `WM_KILLFOCUS`), which stops income. `HandleMapClick` builds an
ordinary class straight away (`WorkOrderBuildObject` -> `BuildObject`); only
garden classes (flag 0x200000) queue a gardener order.

**The first river loops enclosed their own doors.** A Jungle Cruise station's
door is on its EAST side (anchor + (4, 0)); its route start and end squares are
anchor + (0, 10) and anchor + (0, -10). A river laid round the east side boxes
the door in and `llLink` reports "no reachable path square". Lay it round the
WEST side. Working layouts, game level 6:

| station | river squares |
| --- | --- |
| (90, 70) | (85,60) (80,60) (80,65) (80,70) (80,75) (80,80) (85,80) |
| (94, 18) | (89,8) (84,8) (84,13) (84,18) (84,23) (84,28) (89,28) |

Both doors then link to the park network with no path work. Boats launch.

**A negative dispatch timer is a wait, not a failure.**
`JungleCruise_TryLaunchBoat` refuses while any boat of that station sits on or
heads for the route start square or is still in state 1; the timer keeps
counting down until a launch succeeds and then resets to 150.

**The Q11 A/B.** `JungleCruise_Tick` launches only while
`take > 6 * JungleCruise_CountStationBoats(st)` (take is 12 here). Default
counts a station's own boats, so a station stops at two and a second station
can still launch. Faithful re-stamps every boat with the asking station's key
and counts all of them, so once the park has two boats anywhere, every station
is blocked. The measurement: can station (94, 18) launch while (90, 70) runs
two boats.

**Shared origin for both builds.** Serve `portable/` itself (port 8968) and
open `/build-wasm/legoland.html` and `/build-wasm-faithful/legoland.html`: one
origin, one IndexedDB, so a park saved in one build loads in the other. The
page helpers are in `portable/build-wasm*/q11.js` (build dirs, not tracked).

**Q11 default arm (game level 6, both rivers built west, links SATISFIED).**
Station (90, 70) had one boat on the water, keyed (90, 70). Station (94, 18)
had no visitors of its own, so its dispatch check was triggered by writing one
live visitor pointer into its first rider slot (`+0x30`) and zeroing its timer
(`+0x2c`) — the same poke in both builds. Within 100 ms station (94, 18)
launched a boat keyed (94, 18); station (90, 70)'s boat kept its key (90, 70);
the timer reset to 150 and the rider slots cleared.

**Saving the park for the faithful arm did not work** through the game's own
Save screen (options (360, 443) -> Save (516, 222) -> slot 1 -> name -> the
slot's tick (99, 163) -> "Save over game?" tick (285, 137) -> Accept
(546, 412)): no `.sav` file was ever written. Not diagnosed; the faithful arm
rebuilds the same park instead.

**Three input traps met while rebuilding the faithful park (all harness, not game defects).**
- *The advisor's objective bubble opens the REPORT screen.* It covers the lower
  right of the map; a build click aimed there opens `InitScreen7` (the report /
  interval screen), which swallows every click while the cursor underneath still
  reads valid. Escape closes it; its close button is `accept_on_report` at
  (522, 364) top-left. Always centre the view on a target before clicking.
- *Edit mode can drop to 0 while the edit object stays set.* `g_edit_object`
  survives, so a helper that checks only the armed name never re-clicks the
  panel, and in mode 0 no click builds. Check `g_edit_mode == 1` (debug row 159)
  as well.
- *The affine screen fit goes stale* after panel clicks or scrolls while target
  pixels still look in range; one hover landed on map reference (5, 112). Re-fit
  (re-centre) and nudge until `llSel().input.mapRef` is the target cell.
- *Screenshot-derived click points drift with the page header.* The page's
  status lines above the canvas change height, so the canvas origin in a
  screenshot moves (17 screenshot px between two shots here). Derive a point
  from the CURRENT screenshot, or better, hover a small grid and click where
  `llHit().hitType` reads `0x2` (an icon). Escape on the in-park screen opens
  the options screen's EXIT confirmation — the green tick there quits the level.
- *Theme tab and tool positions come from `g_if_icon_pos` (0x004bb04c).* Tabs'
  top-left corners are (8, 379), (105, 379), (202, 379), (299, 379) for LEGOLAND,
  Western, Castle and Adventurers; the tool row starts at y 418 (x 12, 92, 172,
  252, 332). The options tool's hit area reaches above its corner, so a click at
  (342, 395) opens OPTIONS; click a tab near its top-left, e.g. (320, 383).
- *Jungle Cruise Water is a child panel entry* (`keep` 0, parent THE JUNGLE
  CRUISE): the panel draws a collapsed bar, so no slot click arms it. Placing a
  station arms it automatically (`SelectNextBuildObject`), and it stays armed
  while edit mode stays 1 — lay every water square before doing anything else.

**Q11 faithful arm — MEASURED.** Faithful park rebuilt on the same origin:
stations (80, 80) and (94, 18) with closed west-side rivers (plus a stray,
route-less station at (85, 60) that cannot reach the count). Station (80, 80)
had two boats keyed (80, 80). The same poke on station (94, 18) — a live
visitor in rider slot `+0x30`, timer `+0x2c` zeroed — re-keyed BOTH boats to
(94, 18) within 100 ms, and station (94, 18) did not launch: riders 1, timer
-105, because the re-stamped count of 2 made `take 12 <= 6 * 2`. Default, by
contrast, kept the other station's boat key and launched at once. Q11 is
confirmed in both directions.

## 7. Q14 progress (faithful park, game level 6) — paused

- The coaster classes were made available the way `GIVE` does: each element's
  flags `(flags & ~0x10000) | 2` (0x15 -> 0x17) and `g_menu_dirty = 1`. The
  Castle tab then lists the Sensory Coaster Entrance and, as child entries, the
  four track pieces.
- **Direct arm.** `g_edit_object` holds exactly the class-table address of the
  armed class. Writing the Sensory Coaster Entrance's address there and
  `g_edit_mode = 1` gave the entrance's 11 x 15 cursor, valid at (100, 86); one
  click placed it (90 coins) and the game auto-armed Coaster Track. This avoids
  the panel's second page, which the slot scan could not reach reliably.
- **Why any fitted piece suffices.** `Track_Add`/`TrackH_Add` set `*cell |= 6`
  on the node `TrackPlaceIfFits` returns; `DrawTrackNode` asks for the support
  shadow (`mode 0`) only for nodes with state bits 6, and
  `Coaster3D_DrawPieceSupport` draws it only when the support is elevated
  (`pos.z <= -0.1`). A piece fits only when `TrackFitCheck` finds a joint at one
  of its ends, so the first piece must meet an entrance joint.
- **Stopped** during the hover scan for valid track cells: the Browser pane had
  been hidden long enough that the page stopped running tasks (screenshots fail
  with "not compositing", and even an instant JavaScript read times out). The
  pane must be displayed to continue; reloading would lose the park.
- Remaining for Q14: scan the entrance perimeter with Tall Track Piece armed,
  place one fitted tall piece, read `g_shadow_v` minus `g_shadow_src` (rows
  152/153) against the support position, then repeat on a default-build park.
