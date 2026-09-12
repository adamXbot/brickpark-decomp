# Scope PORT-P2 — PLAY tutorial lessons 2 and 3

> **PORT-P2 — Status: IN PROGRESS (claimed 2026-09-12 by PORT-P2)** — branch
> `scope/PORT-P2`, cut from the PORT-M14 merge (`cc6e926d`). A TESTING lane:
> nothing in the game or the shim is fixed here. Every defect is recorded with
> a replayable script, a hash, and an owner.
> Brief: `docs/SCOPE_PORT_WAVE.md`. Files: this note and
> `portable/src/browser/replays/p2-*.js`.

**Result in one line: Lesson 2 cannot be finished and Lesson 3 cannot be
started, and the reason is one silent port defect — `ClampScrollToMap` reads
four globals that belong to the coaster's 3D span clip, because
`g_view_left/top/right/bottom` name TWO different addresses in the recovered
sources and only one definition survives a single-namespace build. The map
scroll is 475 pixels too tight on every edge; on Lesson 2's 54x54 map that puts
the hedge pen holding all four Gardeners permanently off the right of the
screen.** A second, independent defect ends any level outright: a race between
the side panel's scroll animation and MAP mode loses the LEGOLAND theme button
for good, after which nothing can be built.

Lesson 1 was played to its congratulations screen on the way (it gates
Lesson 2) and is **fully playable**: 34.7 fps, `dead` null, `traps: []`, and
PORT-B10's PARK-1 and PARK-3 are both gone — the LINK goal satisfies and the
Space Tower renders as a tower.

---

## 1. The scripts, decoded before playing

Both lessons are plain text inside `gamedata/disc/Legoland.res`, recovered with
`tools/leveldata.py`'s `res_index()`. Lesson 2 is `ObjList2.txt` on `TWO.MAP`,
Lesson 3 is `ObjList3.txt` on `THREE.MAP`.

### Lesson 2 — "Money and planting things" (`ObjList2.txt`, map `TWO`)

`[INIT]`: `CURRENCY 1000`, `ENTRANCEFEE 10`, **`GARDENER(48,5) 4`**,
**`LOOKAT 48,15`**, `WORKERS 1,0`, `FEATURE RideWear 0`, `MAXBLOKES 20`,
`THEMEICON 0,1` (LEGOLAND only). `LOAD` FLOWERS / TREE 1 / HEDGE /
FOUNTAIN 1 / LEGO MEDIA SHOP, `ENABLE "LEGO SHOP 1"`.

`TWO.MAP` ships 116 objects: **113 HEDGE**, `ENTRANCE 1` at (51,28),
`LEGO MEDIA SHOP` at (36,23), `LEGO SHOP 1` at (40,36). The hedges form a
closed 8x10 pen, walls at x=46, x=53, y=1 and y=10 — and the four Gardeners
are spawned at **(48,5), inside it**:

```
      44444455555
      67890123456
   0  ...........
   1  ..########.     <- the pen: x 46..53, y 1..10, solid hedge
   2  ..#......#.
   5  ..#..G...#.     <- GARDENER(48,5) 4
   9  ..#......#.
  10  ..########.
```

| # | keyword(s) | what the player must do | new mechanism |
| --- | --- | --- | --- |
| 1 | `SELECTTHEME LEGOLAND` | pick the Gardeners up out of the pen, drop them outside, then click the flashing LEGOLAND button | worker pick-up / put-down, `GLUE`/`UNGLUE`, `FLASHBUTTON` |
| 2 | `NEED "FLOWERS" 1..5` (five objectives) | plant five flowers | **planting is a gardener WORK ORDER**, not an instant build |
| 3 | `NEED "TREE 1" 2,4,6,8,10` | plant ten pine trees, watching the money bar | money spent per plant; reward `INTERVAL "money.txt"` |
| 4 | `SELECTMODE ERASE` + `REMOVE "LEGO MEDIA SHOP",0` + `REMOVE "LEGO SHOP 1",0` | erase both shops, watch money rise | `REMOVE` goal, refunds, `UNGLUE` |
| 5 | `NEEDIN "FOUNTAIN 1" 1, 36,31, 43,41` | build the fountain in the old Toy Shop rect | `NEEDIN` — a goal with a rectangle |
| — | `ENDLEVEL` | click End Level | the end-level button and `ENDSCREENS` |

**Note for whoever reads the script next: objective 1's goal check is
`SELECTTHEME LEGOLAND` and nothing else.** The script never verifies that the
Gardeners were moved; the prose says to, the goal only wants the button. So
objective 1 passes with the Gardeners still trapped — and objective 2 then
cannot, because there is no free Gardener to service the planting order.

### Lesson 3 — "Power and special rides" (`ObjList3.txt`, map `THREE`)

`[INIT]`: `CURRENCY 1200`, `ENTRANCEFEE 10`, `LOOKAT 39,10`, `WORKERS 1,0`,
`MAXBLOKES 20`, `FEATURE RIDEWEAR 0`, `FEATURE HUNGER 0`, **`FEATURE ENERGY 0`**
(power is turned ON mid-level by a reward), all four `THEMEICON`s 0.
`ENABLE` FLOWERS / TREE 1 / FOUNTAIN 1; `LOAD` SPACE TOWER RIDE /
SMALL POWER STATION / LEGO SHOP 1 / LEGO SHOP 2 / SPIDER RIDE.

`THREE.MAP` (60x60) ships two objects only: `ENTRANCE 1` at (57,28) and
**`POTTING SHED` at (40,5)** — the "Greenhouse" the script tells the player to
click.

| # | keyword(s) | what the player must do | new mechanism |
| --- | --- | --- | --- |
| 1 | `NEEDGARDENERS 1` | click the Potting Shed once | `NEEDGARDENERS`, worker creation from a building |
| 2 | `NEEDGARDENERS 2..6` (five objectives) | click it five more times | worker cost / cap |
| 3 | `PATHSCENERY 25` + `NEED "FOUNTAIN 1",1` + `NEED "TREE 1",1` + `NEED "FLOWERS",1` | scenery beside 25% of the path, one of each | `PATHSCENERY` — a percentage goal over the path network |
| 4 | `NEED "SPACE TOWER RIDE",1` | build the Space Tower | reward `GIVE ... NOPOPUP`, then **`FEATURE ENERGY 1`** |
| 5 | `SELECTTHEME LEGOLAND`, `NEED "SMALL POWER STATION",1`, then a `[PERMANENT]` repeat | build a power station; unpowered rides flash blue/green | power: supply, demand, the flash state |
| 6 | `NEED "LEGO SHOP 1",1` + `NEED "LEGO SHOP 2",1` | two shops | reward **`FMV "Spider.avi"`** |
| 7 | `NEED "SPIDER RIDE",1` | build the Spider Ride | a "special" ride; reward `FLASHBUTTON MAP` |
| 8 | `NEED "SMALL POWER STATION",2` | use the **MAP button** to pick a spot, build a second station | the MAP screen as a placement tool (A7-2's old trap site) |
| — | `ENDLEVEL` | click End Level | |

### What these two lessons do NOT exercise

Neither script contains `LINK` or `LINK ALL`, so PORT-M13 §5a's four off-ring
entrance classes (`WATER WORKS SHOWER`, `dschool.odf`,
`WW ELEPHANT FOUNTAIN`, `WW WATER BLOCK`) cannot be reached from here: the nine
`LINK ALL` scripts are `ObjList6,7,8,9,10,11,12,14,15.txt`, all main-game
levels. Out of scope for this lane rather than tested.

---

## 2. How far each lesson goes

Served `portable/build-wasm` on 8821, `legoland.html?args=-nointro+WINDEBUG&beat=1000`,
own tab, virgin profile `p2`.

| lesson | how far | stopped by |
| --- | --- | --- |
| **Lesson 1** (not in the brief; it is the GATE) | **COMPLETE** — congratulations screen, Lesson 2 unlocked | — |
| **Lesson 2** | **objective 1 of 5**. Briefing both pages, both "new object" popups, the park, `SELECTTHEME LEGOLAND` met. Objective 2 (`NEED "FLOWERS" 1`) **never completes**: the flower is ORDERED (cell flags 0x8800, owner `Flowers`) and the order is never serviced, because every Gardener is behind hedges the view cannot reach | **P2-1** |
| **Lesson 3** | **NOT REACHED.** The progress screen greys Lessons 3-5 until Lesson 2 is finished and a click on a grey line does nothing | **P2-1**, through Lesson 2 |

### Screens, with hashes

Hashes are from the committed build with the profile `p2`; screens with live
animation (the front end's bubbles, the advisor AVI) move between takes, as
PORT-B10 §1 warns. The `llPark()` column is the assertion.

| screen | `llPark()` | fps | `dead` / `traps` | hash |
| --- | --- | --- | --- | --- |
| PLAYER DETAILS, slot 1, `p2` typed | — | 34.5 | null / [] | `0xb3a606a9` |
| TITLE (six bubbles; `p2`'s OK lands here, not on the tutorial list) | — | 34.5 | null / [] | `0x46e23314` |
| **"Select tutorial level"**, Lesson 1 ticked, 2-5 GREY | — | 33 | null / [] | `0xc42a93d4` |
| Lesson 1 briefing p1 / p2 | — | 34 | null / [] | `0xe8db675c` |
| Lesson 1 THE PARK | money **1000**, people 3, limit 3 | 34.0 | null / [] | `0x18dc8b84` |
| Lesson 1 LEGOLAND menu (Toy Shop 30, Space Tower 40) | money 1070 | 33.9 | null / [] | `0xf194c494` |
| Lesson 1 "Your First Ride!" — every line inside the notepad | money **1060** (ride cost 40) | 32.6 | null / [] | `0xcd1e6d94` |
| Lesson 1 **MAP mode** — the whole park as a diamond, view box drawn | — | 33 | null / [] | `0xdd36aa18` |
| Lesson 1 CONGRATULATIONS ("...then select the next level: Money and Planting Things") | money 1150 | 34 | null / [] | `0x1eecb4ed` |
| progress screen, **Lesson 1 red cross, Lesson 2 ticked** | — | 33 | null / [] | `0x5713db54` |
| Lesson 2 briefing p1 / p2 | — | 33 | null / [] | `0x69f9326c` |
| Lesson 2 "You have 2 new objects" — LEGO Media Shop 25, Hedge 2 | money 1000 | 32 | null / [] | `0x5ccad2a0` |
| Lesson 2 THE PARK | money **1000**, people 6, limit 6 | 32-35 | null / [] | `0x043fada8` |
| Lesson 2 after `SELECTTHEME LEGOLAND` (Flowers 1, Hedge 2, Media Shop 25, Toy Shop 30) | money 1870 | 34 | null / [] | `0xe664ff24` |
| Lesson 2 **the clamp's rightmost view** — the hedge pen is off the right edge | money 2020 | 34 | null / [] | `0xb677eec8` |

`llStats().dead` was `null` and `traps` `[]` for every frame of every session,
across six cold loads; **no trap was named all lane**, so `name_trap.py --at`
and `--continue` had nothing to name and `?beat=`'s last ring was never
consulted in anger. **A7-2 is closed again**: the MAP button, which killed the
module for PORT-B9 and PORT-M9, opens and closes MAP mode with no trap, and
its view jump works.

**A caveat on fps.** The figures above are `llStats().fps` taken live while
driving. A 20 s soak could not be taken: with the Browser pane hidden the page
is `document.hidden` and rAF throttles to **0.29 fps** whatever tab is fronted,
which is a measurement artifact and not the port. Worth knowing for the next
lane that drives a hidden pane — measure fps from the panel's live readout
during input, not from a quiet soak.

---

## 3. Findings

| # | severity | what | owner |
| --- | --- | --- | --- |
| **P2-1** | **BLOCKER** | the map scroll clamp is 475 px too tight on every edge; Lesson 2 is unwinnable and Lessons 3-5 unreachable | **PORT-M** (a game declaration: one name, two addresses) |
| **P2-2** | **BLOCKER** | the LEGOLAND theme button is lost permanently to a race between the side panel's animation and MAP mode; nothing can be built for the rest of the level | **PORT-B / PORT-M** — needs a lane; the race is named, the mechanism is not settled |
| **P2-5** | **HIGH — a CLASS, 17 instances** | P2-1 is not alone: **17 names are declared at two different addresses** across the recovered sources, and in every one of them one side of the game is silently reading the other side's object | **PORT-M**, as a sweep |
| **P2-3** | note | Lessons 2-5 are gated on finishing Lesson 1, so no P2/P3 walk can start at its own lesson | original game behaviour — recorded so the next lane does not lose time to it |
| **P2-4** | note | the bottom autoscroll strip is `y > 472`, under the interface panel | original game behaviour (`MouseScrollMap`, the map header's own margins) |

### P2-1 — `ClampScrollToMap` reads the coaster's 3D clip rect

**Symptom.** Lesson 2's first task is *"The Gardeners are stuck behind those
hedges. Pick them up and put them outside the hedges"*. The pen cannot be
brought on screen. At the furthest right the view will go, the pen's centre
cell (48,5) is at game **x = 800** and its nearest corner (46,10) at
**x = 688**, on a 640-pixel screen. Neither route in gets there:

* **edge autoscroll** — hold the cursor in any of the four strips for as long
  as you like and the view slides along the clamp's boundary and stops;
* **the MAP screen's jump** — enter MAP mode, click the pen in the overview,
  come back, and the centre cell is unchanged at the clamp's rightmost point.
  (The same jump aimed north-west moves the view fine, so the jump works; it
  is clamped by the same code.)

The level's own opening `LOOKAT 48,15` is refused for the same reason: it asks
for a view with `x - y = 33` and the clamp allows at most 13, so Lesson 2 opens
looking at the wrong part of the park. Lesson 1's `LOOKAT 81,40` is clamped
too.

**Cause.** `ClampScrollToMap` (`scrolltick.c:281`, `0x00461290`) opens with

```c
hl  = g_view_left   >> 1;      /* 0x004b95f4 */
ht  = g_view_top    >> 1;      /* 0x004b95f8 */
hr  = g_view_right  >> 1;      /* 0x004b95fc */
hb  = g_view_bottom >> 1;      /* 0x004b9600 */
```

and those four halves are the slack in all four EDGE clamps — the clamps that
actually bind. **In the shipped `legoland.exe` the four dwords at
0x004b95f4..0x004b9600 are `0x0003b600` each** (243200; halved, 121600, which
is 475 pixels in the 8.8 fixed units the scroll origin uses). Verified two
ways: the generated `globals.c` carries them as the tail of the block it had to
call `g_cursor_sprite_put`, and a scan of the original's own code at
0x00461290 finds operands 0x004b95f4, 0x004b95f8, 0x004b95fc, 0x004b9600,
0x004b9604, 0x004b9608, 0x004b960c and 0x004b9610 — **all eight, and none of
0x008299ac..0x008299b8** — inside the function's first 0x50 bytes.

But **three other translation units declare the same four names at a different
address**:

```
LEGOLAND/scrolltick.c:28-31   extern int g_view_left;   /* 0x004b95f4 */   <- the scroll clamp
LEGOLAND/coaster3d.c:104-107  extern int g_view_left;   /* 0x008299ac */   <- the 3D span clip
LEGOLAND/coaster10.c:48       extern int g_view_left;   /* 0x008299ac */
LEGOLAND/unref3.c:55-58       extern int g_view_left;   /* 0x008299ac */
```

On x86 that is harmless: the original has no names, and each object file's
reference was assembled against its own literal address. In the portable build
there is one C namespace, and `gen_link` defines `g_view_left/top/right/bottom`
at **0x008299ac** — the rect `Coaster3D_SetViewport` writes
(`coaster3d.c:469-472`) and `Coaster3D_ClipPoint` reads. Four bytes-per-name
lower, 0x004b95f4 is then left unnamed and is swallowed into the preceding
block, which is why `globals.c` emits

```c
/* 0x004b95f0 .data 20 bytes */
__attribute__((aligned(16))) unsigned int g_cursor_sprite_put[5] = {
    (unsigned int)&g_cursor_sprites, 0x0003b600u, 0x0003b600u, 0x0003b600u, 0x0003b600u };
```

for a name `render5.c:89` declares as a single `CursorSprite*`. The four
constants are still in memory; nothing can reach them. `g_view_w`, `g_view_h`,
`g_view_ox` and `g_view_oy` (0x004b9604..0x004b9610) are NOT affected — they
have no second declaration and gen_link places them correctly, which is why
only the four edge clamps are wrong and the four axis clamps are right.

**Evidence, measured.** `P2.clampProbe()` in
`portable/src/browser/replays/p2-02-lesson2-scroll-clamp.js` drives the view
into each corner. On `TWO.MAP` it reaches exactly three extreme
`(g_scroll_x, g_scroll_y)` points, and each sits **exactly** on two of the
clamp's four edge lines evaluated with `hl = ht = hr = hb = 0`
(vw = 640<<8, vh = 340<<8, mapw = maph = 54):

| point | on line | line with `h* = 0` | line with the shipped `0x3b600` |
| --- | --- | --- | --- |
| `(-29696, 67072)` | 1 and 3 | `x = 2y - 163840` / `x = 104448 - 2y` | `x = 2y - 42240` / `x = 226048 - 2y` |
| `(-81920, 40960)` | 1 and 2 | `x = 2y - 163840` / `x = -2y` | |
| `(-81920, 93184)` | 3 and 4 | `x = 104448 - 2y` / `x = 2y - 268288` | |

Solving the two binding lines gives the rightmost the view can ever go, as a
closed form in the map width:

```
x_max(now)      = 4096 * mapw - 250880
x_max(shipped)  = 4096 * mapw - 129280        (121600 units = 475 px further)
```

and that is confirmed on **two different maps**, because it predicts the
measured extreme to the unit:

| map | mapw | predicted `x_max` | MEASURED `g_scroll_x` at the right stop |
| --- | --- | --- | --- |
| `ONE.MAP` (Lesson 1) | 84 | **93184** | **93184** |
| `TWO.MAP` (Lesson 2) | 54 | **-29696** | **-29696** |

With the shipped constants the same formula puts the Lesson 2 stop at
`x_max = 91904`, i.e. scroll x = 359 px, and the pen's centre cell (48,5) at
game **x = 325 — the middle of the screen**. The measurement and the arithmetic
agree that the pen is meant to be centre-screen at the clamp's own limit and is
instead 160 px past the right edge.

**Why Lesson 1 survived it.** `ONE.MAP` is 84x84 and everything Lesson 1 needs
(the entrance, the shop at (74,61), the whole main path) is inside the 475 px
the clamp wrongly withholds. The defect is invisible until a level puts
something near a map corner — which Lesson 2's script does deliberately.

**What it does NOT block.** By the same formula, Lesson 3's `THREE.MAP` is
60x60, so `x_max = -5120` (scroll x = -20 px) and the `POTTING SHED` at (40,5)
lands at game x = 576 — on screen. So Lesson 3's first objective is NOT
predicted to be blocked by P2-1; Lesson 3 is unreachable only because Lesson 2
gates it. That prediction is arithmetic, not a measurement, and is the first
thing to check once P2-1 is fixed.

**Owner: PORT-M.** It is the wave's known "one name, two addresses" class, the
same shape as PORT-M6's `g_route_open`/`g_route_closed` pair, and it is silent
in every existing gate — the bytes match (no `LEGOLAND/*.c` is wrong), there is
no signature mismatch, no trap, no raw pointer word, and `extern_sweep.py` sees
four well-formed externs with readable address comments. What would catch the
class is a sweep for **one name declared at two different addresses across
translation units**; this lane found it by hand and there may be more.

### P2-2 — the LEGOLAND theme button disappears, and the level ends

**Symptom.** The theme row at game y 394 goes from `[LEGOLAND][ ][ ][ ]` to
four empty pills. The button then has no bubble help, a click on it does
nothing, `llPark().editState` never leaves 0, and the side panel never opens
again — so no object can be armed and nothing can be built or planted for the
rest of the level. Three further clicks at 1.8 s apart and two more MAP round
trips do not bring it back. Reproduced three times; it ends the level.

**Trigger — it is a race.** `P2.themeButtonRace()` in
`portable/src/browser/replays/p2-03-theme-button-lost.js`: open the LEGOLAND
side panel, arm an object, enter MAP mode, jump the view, come back.

* with **1500 / 800 / 1500 ms** between the steps, the button survives (twice);
* with **1300 / 700 / 1300 ms**, it is lost (three times, one of them the first
  iteration on a freshly entered level).

Each piece on its own is harmless, all measured on fresh levels: MAP in and
out with the panel closed; MAP plus a view jump with the panel closed; panel
open, MAP plus jump, nothing armed; panel open, object armed, MAP plus jump at
1500 ms; and panel opened with MAP entered 300 ms into its animation.

**Ruled out: the profile.** `UpdateThemeIconsFromProfile` (`screens3.c:741`)
hides theme *i* when `CurProfile+0x30+i` is 0, and `InitGameInterface`
(`bigscreens.c:1075`) calls it on every resume — applying the level's own
`THEMEICON` flags only `if (g_state_810140 != 0)`, i.e. only when the game was
resumed from a SAVE (`savegame.c:1438`; `loadmap.c:787` zeroes it on a fresh
load). That looked like the answer and is not: on this profile those four
bytes read **[1,0,0,0]** — LEGOLAND unlocked — both before and after the button
vanishes, so the profile arm would SHOW it. Something is losing the `Icon`
itself.

**Where to look.** Two writers touch the same panel state in the same frame.
`UpdateSidePanelScroll` (`fpui3.c:475`) walks the object-list icons one step
per tick and only at the END of the run sets `g_panel_state.f00 = 2`,
`f0c = 0x86` and calls `RemoveObjectListIcons(0xd2)`; `InitGameInterface`
(`bigscreens.c:941`, the resume path a screen change takes) writes exactly
those two fields itself at :1067-:1074 and calls the same
`RemoveObjectListIcons`, then rebuilds the theme icons. Note also that
`g_theme_icons` (a `ThemeIcons` struct, `bigscreens.c:833`) and
`g_theme_icon[4]` (an `Icon*` array, `screens3.c:219` and
`eventgoalprim.c:33`) are two names for 0x007fdd70 with two different types —
worth a look under PORT-A9's alias rules before anything else, since the
builder writes through one name and the show/hide pass reads through the other.

**Owner: undecided between PORT-B and PORT-M, and it needs its own lane.** It
may also be original behaviour that a human player never hit because a human
cannot click the MAP button 1.3 s after the LEGOLAND button by accident —
that is the first thing to settle, and it cannot be settled from here.

### P2-5 — the same class, sixteen more times

P2-1 was found by following one symptom back. Having found it, the class is
cheap to enumerate, and it is not a one-off. Group every
`extern <type> NAME;  /* 0xADDR */` in `LEGOLAND/*.c` by NAME and print the
names that carry more than one address:

```python
DECL = re.compile(r"^\s*extern\s+[^;{}]*?\b(\w+)\s*(?:\[[^\]]*\])*\s*;"
                  r"[ \t]*/\*[ \t]*(0x[0-9a-fA-F]{6,8})", re.M)
```

(The `[ \t]*` before the comment is load-bearing: with `\s*` the pattern
crosses newlines and pairs each `extern` with the address comment of the NEXT
declaration, which invents three false hits in `blokeanim.c` — its comments sit
ABOVE their declarations. Those three were checked by hand and agree with
`data2.c`.)

**2675 externs carry an address comment; 17 names carry two addresses.** For
each, the address `gen_link` actually binds is the one under "bound to" — read
off `globals.c`'s block comments and `.set` alias directives — and every
translation unit in the "**loses**" column is, in the portable build, reading
and writing the wrong object:

| name | bound to | and so these lose | what the loser is |
| --- | --- | --- | --- |
| `g_view_left` | `0x008299ac` | `scrolltick.c:28` (wants `0x004b95f4`) | **P2-1**, the scroll clamp |
| `g_view_top` | `0x008299b0` | `scrolltick.c:29` (`0x004b95f8`) | P2-1 |
| `g_view_right` | `0x008299b4` | `scrolltick.c:30` (`0x004b95fc`) | P2-1 |
| `g_view_bottom` | `0x008299b8` | `scrolltick.c:31` (`0x004b9600`) | P2-1 |
| `g_avi_open_count` | `0x00665f48` (advisor's tally) | `movie.c:232` (`0x00668f98`) | **the FMV player's own open count** — `movie.c:12` says `AVIFileInit()` is called the first time any movie is open and `AVIFileExit()` when the count falls to 0. Lesson 3's reward for the two shops is `FMV "Spider.avi"`, so this is on the path P2 could not reach; **check it first** |
| `g_ui_flags` | `0x00813a40` | `logflume2.c:1330`, `screencb6.c:82`, `unref4.c:89` (`0x008003e8`) | the log flume and the screen callbacks read the in-game UI flag word instead of their own |
| `g_popup` | `0x007fdea4` | `fpui2.c:1443` (`0x007fdec0`) | `0x007fdec0` is +0x1c INSIDE the 376-byte `PopUpUI` (`extents.md`), so `fpui2.c` gets the struct's head where it wants a member — an interior alias the name collision defeats |
| `g_snd_click` | `0x004b92c0` | `fpui4.c:315` (`0x004b929c`) | a different sample handle: one UI file plays the wrong click |
| `g_road_tiles` | `0x004cbeac` | `roads.c:187` (`0x004b4c08`) | the road builder's own tile table |
| `g_tile_sprites` | `0x00805f60` | `coaster.c:1976` (`0x0082c680`) | the coaster's sprite table |
| `g_screen` | `0x004bcbf4` (`g_game`) | `printlist.c:247` (`0x00668078`) | the depth-sorted print list's screen pointer |
| `g_view` | `0x007fffc4` (`EditCursor+5124`) | `sysmisc2.c:162` (`0x004bcbf4`, `g_game`) | the mirror image of the row above — two files, two meanings, one name |
| `g_frame_ticks` | `0x006681fc` | `schoolcar.c:1190` (`0x0060f910`) | the driving school's own tick |
| `g_lls_accept_on_report` | `0x004bf694` | `screens3.c:295` (`0x004bef70`) | front-end accept flag |
| `g_carousel_zspr` | `0x006160b8` | `ridecb3.c:307`, `screencb2.c:200`, `screencb6.c:112` (`0x006160c0`) | adjacent ride-callback sprite slots |
| `g_carousel_bnv` | `0x0061608c` | `screencb2.c:202`, `screencb6.c:113` (`0x00616090`) | as above |
| `g_bz_bnv` | `0x00616010` | `screencb2.c:258`, `screencb6.c:108` (`0x00616018`) | as above |

Two things to say about this table before anyone acts on it.

**It is a lower bound.** It only sees externs whose address comment is on the
same line; PORT-M4 drove "externs without a readable address comment" down to
2, so coverage is near-total, but a name that is declared in one file with a
comment and in another without one is invisible to it.

**Not every row is necessarily a defect.** Some may be a descriptive name
honestly reused for two different objects in code that is never reached — the
four `unref3.c` rows under `g_view_*` are on the winning side and `unref4.c`
is unreferenced — in which case the fix is a rename rather than a repoint. But
each row is a place where one translation unit's reads land on another's
memory, silently, with no byte-gate, relocation, signature or trap signal, and
P2-1 shows what one of them costs. **This is a sweep PORT-M should own, and it
is worth a permanent check in the same family as `extern_sweep.py` and
`bvstruct_sweep.py`.**

### P2-3 — the lessons are gated (not a defect, recorded for the next lane)

The progress screen draws "Select tutorial level" with Lesson 1 in black and a
green tick and Lessons 2-5 in grey; a click on a grey line changes nothing but
the cursor. Finishing Lesson 1 flips it to a red cross and moves the tick to
Lesson 2. The unlock is in the profile and **survives a full page reload**
(IDBFS), so a lane only pays for it once. `p2-01-lesson1-unlock.js` is the
whole of Lesson 1 from a virgin profile, ~3 minutes.

One more thing the brief's replay does not have: on this build a profile's OK
lands on the **TITLE screen**, not on the tutorial list. The six bubbles, by
their own bubble help, are `(200,62)` inert / `(500,72)` Quit LEGOLAND /
`(95,175)` Select a player / `(340,215)` Watch LEGOLAND Movies / `(95,355)`
Saved games / `(275,375)` **Start LEGOLAND Game**, which is the way in.

### P2-4 — the bottom autoscroll strip is under the panel (not a defect)

`MouseScrollMap` (`pathtile2.c:408`) reads the map header's own margins, and on
every level here they are screen 640x480 with an 8-pixel edge: the four live
strips are `x < 8`, `x > 632`, `y < 8` and **`y > 472`**. 472 is underneath the
interface panel, so a cursor held at the visible bottom of the map (y 390-470)
scrolls nothing at all and reads as "the view will not scroll down". It does,
at y 477. Twenty minutes lost to it here; written down so the next lane does
not.

---

## 4. Replay index

| file | what it is |
| --- | --- |
| `portable/src/browser/replays/p2-01-lesson1-unlock.js` | a cold load to the end of Lesson 1 — the gate. Also defines `P2.aim` / `P2.pan` / `P2.centre` / `P2.objs`, which the other two need |
| `portable/src/browser/replays/p2-02-lesson2-scroll-clamp.js` | Lesson 2 to where it stops, plus `P2.clampProbe()` (the three extreme scroll points and where the Gardener pen lands) and `P2.mapJumpProbe()` (the MAP screen clamps the same way) |
| `portable/src/browser/replays/p2-03-theme-button-lost.js` | `P2.themeButtonRace()`, `P2.themeButtonDead()`, `P2.profileThemes()` — P2-2's repro, its permanence, and the innocent explanation ruled out |

**The one thing worth stealing from these files** is `P2.aim(cx, cy)`. Every
objective in Lessons 2 and 3 is "do something to THIS cell", and a replay that
hard-codes pixels is valid for one scroll position only. `P2.aim` closes the
loop against `g_input.mapRef` — the cell the GAME says the cursor is on — using
the basis `screen +32 px in x -> cell (+1,-1)`, `screen +16 px in y ->
cell (+1,+1)`, so `dpx = 16*(dcx-dcy)`, `dpy = 8*(dcx+dcy)`. It lands in one
iteration and self-corrects, which matters because PORT-M13's mouse fix makes
the delivered cursor track to within a pixel but not exactly.

---

## 5. What was checked and found NOT broken

* **PORT-B10's PARK-1 (`LINK` never satisfies) is gone.** Lesson 1's
  `LINK "SPACE TOWER RIDE"` was SATISFIED on the frame the ride finished
  building, with the ride placed at (60,56) so its clearance ring landed one
  square from the reachable row-52 path; `LINK "LEGO SHOP 1"` went from MISSING
  to SATISFIED on a single three-square drag at (77,53)..(77,55), which is
  exactly the gap `llLink('LEGO SHOP 1').squares` said was there. PORT-M13's
  account of the rule holds in play.
* **PORT-B10's PARK-3 (the squat Space Tower) is gone.** It renders as a tall
  tower with its rocket and gantry, matching the side-panel icon.
* **Visitors arrive.** `llPark().numVisitors` is 3 on arrival in Lesson 1 and 6
  in Lesson 2, and tracks `visitorLimit` as objects are built and erased
  (3 -> 4 on the ride, 4 -> 1 when the shop is erased).
* **Money is right at every step.** 1000 on arrival, -40 for the Space Tower
  (the panel says 40), +30 for erasing the Toy Shop (the panel says 30), and
  it accrues with visitors throughout.
* **A7-2 is closed.** MAP mode opens, draws the whole park as a diamond with a
  view box, accepts a target, and closes — no trap, `dead` null.
* **The text is right.** Both Lesson 1 briefing pages, "Your First Ride!", the
  Lesson 1 congratulations page and both Lesson 2 briefing pages render with
  every line complete inside the notepad, in the game's own face (PORT-B10 §2
  and PORT-B11 hold).
* **The order half of planting works.** Arming FLOWERS and clicking a free cell
  does place a `Flowers` object with cell flags **0x8800** and owner `Flowers`
  — the work order exists and is on the right cell. Only the servicing half is
  untested, because no Gardener can be freed.
* **Saved progress survives a page reload.** The Lesson 1 completion was read
  back from IDBFS across four full reloads.

---

## 6. What this lane could not do

* **Lesson 2 objectives 2-5 and the whole of Lesson 3** — money-per-plant,
  `NEEDGARDENERS`, `PATHSCENERY`, `FEATURE ENERGY` and the power flash, the
  `FMV "Spider.avi"` reward, the Spider Ride, and the MAP screen used as a
  placement tool are all still untested. They are behind P2-1.
* **A diagnostic patch to get past P2-1 was attempted and abandoned**, and the
  reason is worth recording: the four hijacked globals are in BSS, so they
  cannot be located in the running heap by their initialiser, and the obvious
  trick of modelling emcc's BSS layout from `globals.c`'s declaration order
  does not hold — predicted offsets between `g_scroll_y` and `g_map_dirty` /
  `g_query_extra` / `g_map_ready` missed by 48, 1.25 MB and 64 bytes
  respectively (only the adjacent `g_view_dirty` at +48 came out right). A lane
  that wants to poke that block will have to add a name to
  `LL_DBG_TABLE`, which is PORT-B's file, not this one's.
