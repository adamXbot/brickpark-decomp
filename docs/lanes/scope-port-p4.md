# Scope PORT-P4 — PLAY lessons 2, 3 and 5 again, on the M15 + M16 + memmove tree

> **PORT-P4 — Status: DONE (2026-09-12)** — branch
> `scope/PORT-P4`, cut from the PORT-M16 merge (`be436d48`). A PLAY lane:
> nothing in `LEGOLAND/*.c` is touched. `portable/src/browser/main.c` and
> `index.html` gain READ-ONLY probes (named globals and four page hooks) and
> this lane's replays. Brief: `docs/SCOPE_PORT_WAVE.md`. Files: this note,
> `portable/src/browser/replays/p4-*.js`, the probe additions.

**Result in one line: every defect these three lessons were stopped by is
GONE — the scroll clamp has its real slack, the Mechanic's Hut is on screen,
gardeners and mechanics hire, the cheat ring reads back byte-for-byte and the
appraisal screen opens and closes — and all three lessons now stop somewhere
new. Two of the three stop at the same place, and it is not a new bug: it is
PORT-B12's P1-6 (`Draw3DPersonModel` never paints) seen from the gameplay
side. `Render3DPerson` (rin.c:534) is the game's ONLY hit path for people — it
publishes `g_hit_info.type = 0x306/0x307/0x308` solely when `g_raster_hit`
comes back set from the model draw — so a bloke that is not DRAWN cannot be
CLICKED, and "pick the Gardeners up" (lesson 2 objective 2) and "put the
Gardeners back" (lesson 5 objective 6) are both unreachable. P1-6 is not a
cosmetic render bug; it is a BLOCKER for the worker mechanic.**

| lesson | before (P2/P3) | now | stopped by |
| --- | --- | --- | --- |
| **2** | objective 1 of 5 (unwinnable, P2-1) | **objective 1 drains** (step 2 -> 3); objective 2 blocked | **P4-1** |
| **3** | never started (gated on lesson 2) | **objectives 1–6 of 8** — six hires, the scenery/PATHSCENERY step, the Space Tower, the power station and both shops | **P4-2** |
| **5** | objective 1 of 9 (P3-1 + P3-2) | **objectives 1–5 of 9** — both worker buildings reachable, 4 Gardeners AND 4 Mechanics hired, all three NEEDIN FLOWERS goals | **P4-1** |
| cheats | never fired (P1-5) | **`:PRAISEME`, `:EGYPT`, `:CASTLE`, `:COLDHARDCASH` all fire; the appraisal screen opens, pauses the sim and closes** | — |

`llStats().dead` was `null` and `traps` `[]` for every frame of every session —
roughly 190,000 presented frames across seven page loads — and fps stayed
**35.4–35.8** (the game's own `FlipPrimary` ceiling is 35.71) with a park, 119
flower beds, four rides and the full HUD.

---

## 0. What was measured, and with what

Served `portable/build-wasm` on **8864**,
`legoland.html?args=-nointro+WINDEBUG&awake=1`, this lane's own tab, virgin
IDBFS, profile `p4` in slot 1 with `g_level_done[0..4] = 1` written into the
on-disk profile record (PORT-P3's poke: `Profile%d.txt` is the raw 0x110-byte
`Profile` struct and the fifteen bytes at +0x34 ARE `g_level_done`; writing 1
is exactly what the game writes when a lesson is finished).

**The probes this lane added** (all reads; nothing in the game changes):

* `main.c`'s debug table gains 27 names — the script engine
  (`g_script_steps`, `g_script_cur`, `g_script_root`, `g_script_bytes`,
  `g_goal_kind_count`), the cheat ring and what cheats do
  (`g_type_buf`, `g_instant_appraisal`, `g_appraisal_minutes`, the
  interactive-music mailbox `g_imt_state/cmd/cmd_arg/theme`), the workers
  (`g_gardener_count`, `g_mechanic_count`, `g_gardener_list`,
  `g_mechanic_list`, `g_gardener_orders`, `g_gardener_order_count`,
  `g_mechanic_orders`, `g_worker_on_mouse`, `g_worker_on_mouse_type`), the
  hit machinery (`g_raster_hit`, `g_mouse_pixel`, `g_selection_lock`,
  `g_drag_lock`) and `g_popup_info` / `g_theme_icon`.
* `index.html` gains **`llGoals()`**, **`llWorkers()`**, **`llRing()`** and
  **`llHit()`**.

**Why that matters more than it sounds.** P1–P3 read progress off the canvas
and off `money`; both move for reasons that are not progress. `llGoals()` walks
`eventmake.c`'s `ScriptStep` list, so `llGoals().cur.id` **is** the objective
number on the notepad and an objective has drained when it advances. Every
"drains" in this note is that number moving, not a frame hash.

Two API notes for the next lane:

* `llGoals().cur.goals` is **empty** for a step in progress — the step's goal
  list is handed to the tick engine on entry. The list actually being checked
  is `g_script_event` (fpui3.c:603); `P4.unmet()` reads it.
* `TallyBuildFootprints` (mapbuild2.c:75) recomputes PATHSCENERY at most once
  every ten seconds, so the percentage lags the map. `P4L3.tally()`
  reimplements it in JS.

---

## 1. Findings

| # | sev | what | evidence | owner |
| --- | --- | --- | --- | --- |
| **P4-1** | **BLOCKER** | **No bloke can be clicked, anywhere, so no worker can be picked up, put down or queried.** `Render3DPerson` (rin.c:534) is the game's only hit path for people: it publishes `g_hit_info.type` 0x306/0x307/0x308 **only if `g_raster_hit` came back set from `Draw3DPersonModel`**. That body never paints (PORT-B12's P1-6), so the type is never published. Lesson 2 objective 2 ("pick the Gardeners up and put them outside the hedges") and lesson 5 objective 6 (`NEEDGARDENERS 0`) are both unreachable. **P1-6 is a gameplay blocker, not a cosmetic one.** | §2 | **the render/matching lane that owns P1-6** (`LEGOLAND/person3d.c:1092`) |
| **P4-2** | **HIGH** | **Lesson 3 stops at objective 7 of 8: after the Spider Ride is GIVEn, the LEGOLAND side panel will not offer it — and has also lost the Space Tower Ride and the Small Power Station, both of which were built FROM that panel minutes earlier.** The panel offers exactly Flowers, Pine Tree, Small Fountain, LEGO Clothes Shop, LEGO Toy Shop; `llClasses()` shows all eight classes loaded, the Spider with the same class flags (0x480666) as the two shops that ARE offered. The final objective also needs a SECOND power station, so the level cannot end. **Control**: in lesson 5 a Small Power Station is built and the panel still offers it, and a class GIVEn mid-level appears — so this is not "built once, gone". | §3.4 | **a front-panel / UI lane** (`fpui2.c`/`fpui3.c`'s object-list builder) — same family as PORT-P1's P1-3/P1-4 |
| **P4-3** | medium | **Three different TEXT BOXES fill with solid black, and stay that way.** The money readout's box (a band at game x 220..430, y 8..30, 63.6% black in its core columns, **unchanged by a MAP in/out round trip** that redraws the whole screen), the objective help bubble (75.9% black, held for 9 s and then redrawn clean), and the body panel of the object info pop-up. Each is a box the game fills and then prints into; the print lands (the pop-up's "Repair cost: 0" / "Scrap value: 5" are legible ON the black), so it is the FILL that is wrong, not the text. Neither bar is black at level load. | §4.3 | a render / GDI-text lane (PORT-B's `ll_font.c` / `gdi32.c`) — **unclassified**; the cheap first test is whether `SetBkMode(TRANSPARENT)` is honoured |
| P4-4 | info (harness) | **The page's canvas collapses to 0x0 CSS pixels when the Browser pane is small**, and `llPoint` divides by `r.width`, so EVERY game pixel maps to the same 2x2 rect and the driver silently clicks one spot. This lane lost three page loads to it before noticing. A `resize_window` to >= ~820x760 fixes it. | §5 | PORT-B (the page) |
| P4-5 | info (harness) | `?awake=1` keeps the GAME awake in a hidden pane but not the DRIVER: `llMove`/`llClick` sleep through `setTimeout`, which Chrome clamps to ~1 Hz, so one `llClick` costs **3.2 s** instead of 0.81 s. `P4.fast()` swaps in MessageChannel-pumped equivalents. | §5 | PORT-B (the page), or every play lane |
| P4-6 | note | The lesson-2 hedge pen's interior carries map flag **0x40 = GLUE** (levelkw3.c:206), so nothing can be planted inside it. That is the puzzle working as designed, not a port defect — recorded because it is the first thing a driver tries. | §2.2 | — (game rule) |
| P4-7 | note | `':'` is not a key. `GetTypedChar` (uimisc.c:527) maps DIK 0x2a/0x36/0x3a (the two Shifts and CapsLock) to -10 and turns -10 into `':'`, so `llType(':PRAISEME')` pushes only `PRAISEME` and no cheat fires — which looks exactly like the doubling bug and is not. Press Shift, then type the word. | §6 | — (game rule) |
| P4-8 | note | A **"You have a new object" pop-up swallows every click**: while one is up the game answers every hover with hit type 0x1 over a building that is plainly there and `g_edit_object` never changes. Read as "the hut is not clickable" or "the panel is empty" it invents two defects that are not there. `P4.dismiss()` first. | §3.1 | — (game behaviour; a harness trap) |

### What is CLOSED

| was | filed by | now |
| --- | --- | --- |
| **P2-1 / P3-1** the scroll clamp is 475 px too tight | PORT-P2 §3, PORT-P3 §3.1 | **CLOSED.** §2.1 — the east stop is 91904, P2's own closed form for the SHIPPED constants, to the unit |
| **P2-2** the LEGOLAND theme button lost for good | PORT-P2 §3 | **CLOSED.** The button survived every session; the theme row never emptied |
| **P3-2** no gardener and no mechanic can be hired | PORT-P3 §3.2 | **CLOSED.** §3.1, §4.1 — six Gardeners in lesson 3, four Gardeners and four Mechanics in lesson 5 |
| **P1-5** every ~7th typed character doubled | PORT-P1, PORT-B12 | **CLOSED.** §6 — 40+ characters, zero doubling |
| **P1-8** the appraisal screen | PORT-B12 | **CLOSED on the honest tree.** §6.2 |

---

## 2. Lesson 2 — objective 1 drains; objective 2 is P4-1

### 2.1 The clamp, closed

PORT-P2's whole case rested on one number. `ClampScrollToMap` (scrolltick.c:281)
reads its four edge slacks from `g_view_left/top/right/bottom`, four names
`coaster3d.c` also used for the 3D span clip; one C namespace kept one address,
so the clamps ran with **0** where the shipped binary has **243200**, and the
view stopped 475 px short of every edge. PORT-M15 renamed the scroll set to
`g_scroll_slack_*`.

Hold the cursor in the right-hand strip until `MouseScrollMap` stops:

| | `g_scroll_x` at the east stop |
| --- | --- |
| PORT-P2 measured, clamp with zero slack (`2y - vw`) | **-29696** |
| PORT-P2's closed form for the SHIPPED slack, `4096*mapw - 129280`, mapw 54 | **91904** |
| **PORT-P4 measures on this tree** | **91904** |

To the unit. And the consequence P2 predicted arithmetically is now visible:
the hedge pen's centre cell **(48,5) lands at screen x = 325** — the middle of
a 640-wide canvas — where P2 had it 160 px past the right edge. The pen is a
big empty green rectangle in the middle of the view.

Objective 1's goal check is `SELECTTHEME LEGOLAND` and nothing else (P2 read the
script right), and it **drains**: step 2 -> 3, `money` 1150 after the reward,
`traps` 0.

### 2.2 Objective 2, and the two ways it is refused

`NEED "FLOWERS" 1` counts BUILT flowers, and a flower is not a build: it is a
gardener WORK ORDER (misc3.c:395) that a free Gardener has to walk to. The four
Gardeners are spawned at (48,5) **inside** a closed hedge pen — walls at x=45
and x=52, y=1 and y=10.

**Inside the pen.** Every interior cell reads map flag **0x40**, which is GLUE
(levelkw3.c:206 / `EventTick_Glue` eventtick.c:542), and objmap2.c:1827 turns
that into cursor error 1. The pen is glued *deliberately* so the player cannot
build his way out of the puzzle. Measured as an 11x14 grid of `llCellAt`:

```
      44444455555
      56789012345
   1  .HHHHHHHH..     H = hedge (owner, flags 0xc0)
   2  .HggggggH..     g = GLUEd empty ground (no owner, flags 0x40)
   ...
  10  .HHHHHHHH..
```

453 cells on TWO.MAP carry 0x40. **Game rule, not a port defect (P4-6).**

**Outside the pen.** The order is accepted — cell owner "Flowers", flags
0x8800, `g_gardener_order_count` 0 -> 5 — and is never serviced. After 60 s all
five `WorkOrder`s still read `assigned: 0`, while three of the four Gardeners
have taken an order into their own `+0x50` slot and are still at cells (50,4),
(47,3), (47,6), (49,8) — inside the pen. They cannot walk out. `cur.id` stays 3.

So the lesson is exactly as designed: **the Gardeners have to be carried out.**

### 2.3 They cannot be picked up, and the reason is one function

`fpui2.c:1583` routes hit type **0x307** to `WorkerPopUp` (fpui4.c:470), which
is the pick-up. The **only** producer of 0x307 is `Render3DPerson`
(rin.c:534):

```c
Draw3DPersonModel(p);
...
if (g_raster_hit == 0)
    return;                     /* <- nothing is published */
...
case 2:  g_hit_info.type = 0x307; break;
```

`g_raster_hit` is ORed by the rasteriser (tri3d.c:305) when it paints the pixel
under the mouse and cleared by `SetRasterOrigin` on entry. There is **no cell
lookup for blokes** the way there is for objects: the hit test IS the draw. So
PORT-B12's P1-6 ("Draw3DPersonModel never reaches its vertex loops") does not
only lose the picture — it loses the input.

**Measured, in the pen.** Four Gardeners at cells (47,3), (47,5), (48,8),
(50,8), all on screen, on plain grass, inside a fence, with nothing else to hit:

| | |
| --- | --- |
| probes (8 px grid over the pen) | **578** |
| hit types returned | `0x109` 311, `0x103` 193, `0x10a` 74, plus 0x1/0x100 |
| **0x306 / 0x307 / 0x308** | **0** |
| `g_worker_on_mouse` after 48 clicks on the Gardeners' own cells | **0** |

and the pen is *visibly empty* in the frame: four Gardeners in memory, not one
pixel of them on the canvas. The rasteriser itself is alive — `g_raster_hit`
reads 1 on the front-end screens, which draw a 3D minifigure.

**Reproduced twice, on two maps** — see §4.2 for the lesson-5 run: 968 probes
around eight workers (Gardeners *and* Mechanics) on FIVE.MAP, zero person hits.

**This also answers the brief's last question.** Visitors are not drawn in the
tutorial park either; the Gardener case is the same measurement with the blokes
held still by a fence, which is why it is the one worth quoting.

---

## 3. Lesson 3 — six hires, the scenery step, three builds, then P4-2

### 3.1 Hiring works — P3-2 is closed

A click on the Greenhouse is a click on an **object** (hit `0x103`), not on a
bloke, so this path never needs the person rasteriser. It needs
`PopUpInfoSetUp` to recognise the class, which is exactly what the `g_popup`
shear broke: `fpui2.c` gave the name the request block at 0x007fdec0 while
`bighelp.c`/`popup.c` gave it the 376-byte `PopUpUI` at 0x007fdea4, so the
`elem_shed` / `elem_hut` compares at +0xf0/+0xf4 read zeros. PORT-M15 split them
into `g_popup_info`.

| click | hit | `g_gardener_count` | money | step |
| --- | --- | --- | --- | --- |
| 1 | 0x103 | 0 -> 1 | 1200 -> 1170 | 0 -> 1 |
| 2 | 0x103 | 1 -> 2 | 1170 -> 1140 | 1 -> 2 |
| 3 | 0x103 | 2 -> 3 | 1140 -> 1110 | 2 -> 3 |
| 4 | 0x103 | 3 -> 4 | 1110 -> 1080 | 3 -> 4 |
| 5 | 0x103 | 4 -> 5 | 1080 -> 1050 | 4 -> 5 |
| 6 | 0x103 | 5 -> 6 | 1050 -> 1020 | 5 -> 8 (6 and 7 are reward-only steps) |

30 bricks each, one objective per click, `traps` 0 throughout.

**Caveat that cost this lane a measurement (P4-8).** The first attempt read
`hit: 0x1` on every click and hired nobody — because a "You have a new object"
pop-up was up and swallowing the input. While one is open the game answers
every hover with 0x1 over a building that is plainly there. `P4.dismiss()`
first.

### 3.2 The scenery step

Step 8 is `PATHSCENERY 25` + `NEED "FOUNTAIN 1"/"TREE 1"/"FLOWERS"` — all four
together. Measured: 119 flower beds placed along the path (all built by the six
Gardeners, cell flags 0x8080), one Small Fountain, one Pine Tree, `P4L3.tally()`
= **32%** against the required 25, and the step drained 8 -> 9 within 4.3 s of
the last piece. Money 1200 -> 892.

Two mechanics worth recording: `TallyBuildFootprints` counts the **perimeter**
of each `PathSquare` rectangle as the denominator and recomputes at most every
ten seconds; and the Small Fountain's 4x4 footprint gives cursor error 10
("something under it", objmap2.c:1837) on every square a 1x1 flower accepts, so
it needs `P4.clearArea()` to find it room.

### 3.3 The rides

| step | build | money | drain |
| --- | --- | --- | --- |
| 9 | Space Tower Ride | 892 -> 852 | 9 -> 11 (10 is `SELECTTHEME LEGOLAND`, already satisfied) |
| 11 | Small Power Station | 852 -> 817 | 11 -> 13 (12 is the PERMANENT re-check) |
| 13 | LEGO Toy Shop + LEGO Clothes Shop | 837 -> ~807 | 13 -> 14, with the `FMV "Spider.avi"` reward — the AVI stub is a documented non-trapping no-op, nothing played and nothing hung |

### 3.4 P4-2 — the panel loses the rides

Step 14 is `NEED "SPIDER RIDE", 1`, and the reward for step 13 was
`GIVE "SPIDER RIDE"`. The pop-up announcing it drew correctly ("You have a new
object — Spider Ride — 80 — Professor Voltage didn't intend to create the
Spider Ride…"). **The class is then not in the panel.**

The LEGOLAND panel has four icon slots at game y 89/153/217/281 with scroll
arrows at y 44 and y 344. Enumerated by arming every slot and reading
`g_edit_object`'s display name, the reachable set is:

```
Flowers | Pine Tree | Small Fountain | LEGO Clothes Shop | LEGO Toy Shop
```

and that is all, re-checked **four ways in the same session**: plain; after a
panel close/open; after a MAP in/out round trip (which re-runs
`InitGameInterface`); and after ten clicks on the down arrow (the panel pixels
change on every one, so the arrow IS being hit).

Missing: **Spider Ride** (type 1, never built), **Space Tower Ride** (type 1)
and **Small Power Station** (type 0) — and the last two were built *from this
panel* twenty minutes earlier in the same session, so the list was right then.
`llClasses()` shows all eight classes present, and the Spider carries the same
class flags (0x480666) as the two shops that ARE offered, so the class record is
not the discriminator. Money was 1121 against the Spider's 80, so it is not
affordability.

The final objective needs a **second** Small Power Station, so the level cannot
be ended either way. **Lesson 3 therefore finishes 6 of its 8 player
objectives.**

**The obvious explanation is ruled out by a control in lesson 5.** "A class
leaves the panel once one has been built" would account for the Space Tower and
the first power station (though not for the Spider, which was never built). It
is wrong: in lesson 5 the same `Small Power Station` was armed, built
(money 593 -> 558, cell flags 0x80 at (2,12)) and the panel re-enumerated
immediately afterwards —

```
before:  Flowers | Hedge | Small Power Station | Boating School Entrance
after:   Flowers | Hedge | Small Power Station | Boating School Entrance
```

— unchanged. Lesson 5's panel also picks up a class GIVEn mid-level (the
Boating School Entrance, 90) correctly. So whatever lesson 3's panel is doing
is specific, not a general rule, and the lesson-5 park is the A side of the A/B
whoever takes this should start from.

**Not fully classified.** It was observed in one cold run (re-checked four ways
within it, plus the lesson-5 control); a second cold run of lesson 3 is owed,
and the right owner is a lane that can read the object-list builder rather than
drive it. It is the same family as PORT-P1's P1-3 ("the LEGOLAND tab hides
itself") and P1-4 (a stray store setting 0x400 on a panel icon), and PORT-M16
has already shown once that a lost icon in this module was a pointer landing in
the wrong slot rather than anything temporal — so **check the icon slots before
assuming the list is short**.

---

## 4. Lesson 5 — five objectives, and the same wall as lesson 2

### 4.1 Both worker buildings, and eight hires

PORT-P3 stopped here at objective 1 of 9, blocked twice: the MECHANICS HUT at
(52,5) was 183 px beyond the clamp, and nothing could be hired. Both are gone.

| | |
| --- | --- |
| POTTING SHED (40,5) | centres at screen (325,169), `owner` "Greenhouse" |
| MECHANICS HUT (52,5) | centres at screen (341,208), `owner` "Mechanic's Hut" |
| four clicks on the shed | hit 0x103 each, `g_gardener_count` 0 -> 4, money 1000 -> 880 |
| four clicks on the hut | hit 0x103 each, `g_mechanic_count` 0 -> 4, money 880 -> 760 |

Then 18 flower beds inside the NEEDIN rectangle (49,26)..(51,18) drained
**steps 3 -> 6 in one move** — all three `NEEDIN "FLOWERS" 5 / 10 / 15` goals —
money 760 -> 666, `traps` 0.

### 4.2 Step 6 is `NEEDGARDENERS 0`

`EventTick_Needgardeners` (eventtick2.c:398) with `f1c <= 0` passes only when
`GetGardenerCount()` reaches zero, and `GetGardenerCount` is a plain read of
`g_gardener_count` (tinystubs.c:115). The only way to remove a Gardener is the
worker pop-up — `PopUpInfoSetUp` case 0x307/0x308 -> `WorkerPopUp`. Which is
P4-1.

**The second reproduction.** Four Gardeners and four Mechanics, centred one at a
time, swept at 8 px in a +-40 px box:

| | |
| --- | --- |
| probes | **968** |
| hit types returned | `0x100` 821, `0x103` 147 |
| **0x306 / 0x307 / 0x308** | **0** |
| `g_worker_on_mouse` | **0** |

Different map, different lesson, both worker kinds: the same answer.

**Lesson 5 therefore finishes 5 of its 9 player objectives.** Everything after
step 6 — the power station, `NEEDMECHANICS 1/3/4`, `LINK "BOATING SCHOOL"`,
`LOOPCOMPOSITE`, the Western theme — is behind it. (The `NEEDMECHANICS` goals
themselves are *not* blocked: hiring works. Only `NEEDGARDENERS 0` is.)

### 4.3 The ride info pop-up — P4-3

PORT-P1 never reached an object pop-up; this lane opened one by accident and
then on purpose. Clicking any placed piece of the Boating School opens a titled
frame that follows the cursor, carrying:

* the display name — **"Boating School Water Way"** — in the title bar,
* **"Repair cost: 0"** and **"Scrap value: 5"**,
* a wrench icon and a red close box,
* and a body panel that draws **solid black** where the notepad paper should be.

The two text lines are correct and legible — printed **on top of** the black —
and the frame, the title bar, the wrench and the close box all draw properly.
So the defect is in the FILL of the box, not in the text.

**And it is not confined to the pop-up.** Two other text boxes on the same
screen go black the same way, and they are measurable. Sampling the canvas
directly (percentage of pure-black pixels in a rectangle, one reading a second
for twelve seconds):

| box | game rect | % black | behaviour |
| --- | --- | --- | --- |
| the money readout | x 220..430, y 8..30 | **56.9**, and 63.6 in its core 20-px columns | **persistent** — 12 readings at 1 s apart all identical, and a MAP in / MAP out round trip (which rebuilds the whole screen through `InitGameInterface`) leaves it unchanged |
| the objective help bubble | x 420..630, y 330..375 | **75.9** | held for nine seconds, then redrawn clean (0.1) when the bubble's text changed |
| the object pop-up's body | follows the cursor | — | for as long as the pop-up is up |

Neither bar is black at level load: the lesson-5 opening frame shows a clean
`1000` on a clean bar, and the black band appears later in the session.

Filed as **P4-3, medium, unclassified** — this lane is not to investigate the
renderer. The cheap first test for whoever does: all three are boxes the game
fills and then prints into, and the shape of the failure (an opaque fill in the
background colour under legible text) is what an ignored `SetBkMode(TRANSPARENT)`
looks like. PORT-B2 made GDI text real in `ll_font.c`; that is where to start.

The **worker** pop-up cannot be opened at all, which is P4-1 and not a separate
finding.

---

## 5. Two harness findings the next play lane should have first

**P4-4 — the canvas collapses and the driver clicks one pixel.** `llPoint`
maps a game pixel through `r.left + gx * r.width / 640`. In a small Browser
pane the page's CSS gives the canvas `width: 0px`, so `r.width` is 2 and every
game pixel in the frame maps into the same 2x2 rect — the driver clicks one
spot, the walk diverges, and nothing anywhere says why. Three page loads went
into this before `getBoundingClientRect()` was read. Fix from the driver side:
`resize_window` to at least ~820x760 and assert `Module.canvas.getBoundingClientRect().width === 642`
before the first click. Proper fix is a minimum size in the page — **PORT-B**.

**P4-5 — `?awake=1` wakes the game, not the driver.** PORT-A10's MessageChannel
yield keeps the GAME at 35.7 fps in a hidden pane, and PORT-M16 §1 already
warned what a throttled pane does to a lane's conclusions. But `llMove` and
`llClick` sleep through `setTimeout`, which Chrome still clamps to ~1 Hz, so the
DRIVER runs at a quarter speed while the game does not: measured, one
`llClick` costs **3.2 s** hidden and **0.81 s** with the same events pumped
through a MessageChannel. `P4.fast()` installs the fast versions over
`window.llMove`/`llClick`; the events the game receives are identical. Worth
folding into the page — **PORT-B**.

---

## 6. The cheats — P1-5 and P1-8 closed on the honest tree

### 6.1 The ring reads back exactly

Every cheat is a `strnicmp` of the 20-byte ring's TAIL at a fixed offset, so one
duplicated byte kills all of them silently. PORT-B12 proved the cause
(`memcpy` on overlapping memory at input.c:368) and demonstrated the fix on a
throwaway build; the integrator has since put the `memmove` in a
`LEGOLAND_PORTABLE` arm. Forty-plus characters typed, in one park:

| typed | ring afterwards | tail | effect, measured |
| --- | --- | --- | --- |
| `:PRAISEME` | `...PRAISEME:PRAISEME` | `[11]` = `:PRAISEME` | `g_instant_appraisal` **0 -> 1** |
| `:EGYPT` | `ISEME:PRAISEME:EGYPT` | `[14]` = `:EGYPT` | `SetTheme(1)`; `g_imt_state` is 1 so sysmisc2.c:134 takes the `SetThemeInTransition` arm — mailbox arg **0 -> 1** |
| `:CASTLE` | `RAISEME:EGYPT:CASTLE` | `[13]` = `:CASTLE` | `SetTheme(3)`; mailbox arg **1 -> 3** |
| `:COLDHARDCASH` | `:CASTLE:COLDHARDCASH` | `[7]` = `:COLDHARDCASH` | `AddBricks(5000)`: money **2420 -> 7460** (the extra 40 is park income over the interval) |

**No doubling anywhere.** Each ring is the previous one shifted left by exactly
the number of characters typed. P1-5 is closed.

**P4-7, the control.** `llType(':PRAISEME')` leaves the ring reading
`............PRAISEME` with byte[11] still 0xF2 and fires nothing: `':'` is not
a key. `GetTypedChar` (uimisc.c:527) maps DIK 0x2a/0x36/0x3a — the two Shifts
and CapsLock — to -10 and turns -10 into `':'`. Press Shift, then type the word.
This reads exactly like the doubling bug; a lane that has not read uimisc.c will
re-file P1-5.

### 6.2 The appraisal screen

`:PRAISEME` sets `g_instant_appraisal`, `AppraisalDueTick` pauses the sim and
`RunAppraisalScreen` draws. Measured in tutorial lesson 2 (PORT-B12 reached it
in free play, on a throwaway build):

* `g_sim_frame` froze at **16629** while `llStats().frames` kept rising at
  **35.7 fps** — the page presenting identical frames over a paused sim. Note
  `g_screen_mode` does **not** change (it stays 7): the appraisal draws over
  the park, so the frozen sim frame is the signal, not the screen mode.
* The frame is the **REPORT notepad** with the inspector minifigure and
  "Objectives: Congratulations you have built a thriving Park!", frame hash
  `0x5ec9a2a4`.
* One click at game (577,419) closes it and the sim resumes: 16629 -> 16688,
  and on to 17790 while the next reading was taken. `traps` 0 throughout.

**P1-8 is closed on the merged tree.**

---

## 7. Owed to other lanes

* **The render lane that owns P1-6**: the finding is bigger than it was filed
  as. `Draw3DPersonModel` not painting costs the **input**, not just the
  picture — `Render3DPerson` is the only publisher of `g_hit_info` for people,
  so every worker interaction in the game is behind it. Two tutorial lessons
  cannot be finished. PORT-B12's own lead is still the one to test first:
  `GetVideoSurface` returns 0 whenever `g_video_locked == 0`, and sprites would
  still draw because `PrintSprite` pushes its own lock.
* **A front-panel / UI lane**: P4-2. Reproduce from a cold run of lesson 3,
  then read the object-list builder rather than driving it. Check the icon
  slots (P1-4's 0x400 store, M16's parked `Icon*`) before assuming the list is
  short.
* **PORT-B (the page)**: P4-4 (assert a minimum canvas size, or stop dividing
  by a zero-width rect) and P4-5 (pump the driver's sleeps off a
  MessageChannel). Both are three-line changes and both cost this lane real
  time.
* **Whoever plays a game level next**: the probes added here
  (`llGoals`, `llWorkers`, `llRing`, `llHit`) make "did the objective drain"
  and "can this pixel see a person" one-line questions, and `P4.dismiss()`,
  `P4.centerOn()`, `P4.plant()` and `P4.buildNamed()` are a working park
  driver. Read §5 before the first click.
