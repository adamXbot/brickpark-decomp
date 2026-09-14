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
