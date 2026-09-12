# Scope PORT-P3 — PLAY tutorial lessons 4 and 5

> **PORT-P3 — Status: IN PROGRESS (claimed 2026-09-12 by PORT-P3)** — branch
> `scope/PORT-P3`, cut from `feat/decomp-completion-next-steps-24a0d6`
> (`cc6e926d`). **Testing lane**: nothing in the game or the shim is edited.
> Findings carry evidence and an owner. Brief: `docs/SCOPE_PORT_WAVE.md`.
> Replays: `portable/src/browser/replays/p3-lesson4.js`, `p3-lesson5.js`.

**Result in one line: both lessons load, render and play — and both stop dead
at the objective that asks for a worker, because THE GAME CANNOT HIRE A
GARDENER OR A MECHANIC AT ALL.** Two independent defects, both of the same
class (one NAME, two ADDRESSES), both measured in the running tab, and a
tree-wide sweep that finds fifteen more instances of that class.

Everything else worked: the tutorial select screen, both briefings (every line
of shipped text inside the notepad), both parks, the script engine, DEGRADE's
red flash, the power-out flash, the build cursor, the object menus, buying a
ride (money 460 -> 400), work orders, the full-map screen, and **33–35 fps
throughout, `llStats().dead` null and `traps: []` for the whole session**.

---

## 0. A correction to the brief

The brief says lesson 5 is "the coaster (coaster\*.c, the track editor, RK4
physics, the 3D model draw), the log flume (logflume\*.c) and the Wild West
rides". **It is not.** Decoding the shipped scripts with `tools/leveldata.py`
plus the `.res` index:

* Lesson 5's "really big ride" is the **BOATING SCHOOL** — a *composite* ride
  (entrance + water pieces + mermaid) whose objective is `LOOPCOMPOSITE
  "BOATING SCHOOL", 5`, i.e. join one side to the other with water pieces.
* The Wild West half is `SPINNING BARRELS RIDE`, `SHERIFF`, `SALOON` and three
  cacti, placed into a marked corner (`NEEDIN ... (0,28),(26,0)`).
* **No tutorial lesson contains a coaster or a log flume.** `ObjList1..5.txt`
  never name one. The coaster and the flume first appear in **game level 2**
  (`ObjList7.txt`, map `GLTWO`) and again in `ObjList9/10/11/12`.

So this lane has no coaster to run and no fps figure for the 3D passes; the
right brief for that is a *game*-level lane (level 2 is the first). The fps
numbers below are for the ordinary isometric park, which is what both lessons
draw.

---

## 1. The two lesson scripts, decoded

`ObjList4.txt` and `ObjList5.txt` live in `gamedata/disc/Legoland.res`
(`leveldata.res_index()` lists them; they are plain text). Maps `FOUR.MAP`
(54x84) and `FIVE.MAP` (60x60) decode with `tools/leveldata.py`.

### Lesson 4 — "Mechanics and repairing rides" (`ObjList4`, `FOUR.MAP`)

`CURRENCY 450`, `ENTRANCEFEE 10`, `WORKERS 0, 1` (mechanics only),
`FEATURE AutoRepair 1`, `Ridewear 0`, `Energy 1`, `MAXBLOKES 10`,
LEGOLAND theme only. Seven objects are placed by the map:

| class | cell | reached? |
| --- | --- | --- |
| MECHANICS HUT | (46,6) | **no — off the reachable view (P3-1)** |
| SMALL POWER STATION | (37,5) | yes |
| SPIDER RIDE | (39,24) | yes |
| ENTRANCE 1 | (51,46) | yes |
| LEGO MEDIA SHOP | (39,40) | yes |
| SPACE TOWER RIDE | (31,52) | yes |
| LEGO SHOP 2 | (43,52) | yes |

| # | objective | check | played |
| --- | --- | --- | --- |
| 0 | (reward only) | `DEGRADE` the four rides to 24, `FLASHBUTTON QUERY` | **yes** — all four flash red |
| 1 | query a damaged ride | `SELECTMODE QUERY` **and** `SELECTTHEME LEGOLAND` | **yes** — reward `GIVE "COPTERS"` fired |
| 2 | build the Copters Ride | `NEED "COPTERS", 1` | **yes** — money 460 -> 400, reward `LOOKAT 45, 5` |
| 3 | get a mechanic | `NEEDMECHANICS 1` | **NO — P3-1 and P3-2** |
| 4 | drop her on the Space Tower | `FIXRIDES 0, 20` | unreachable |
| 5–8 | four more mechanics | `NEEDMECHANICS 2..5` | unreachable |
| 9 | watch them fix everything | `FIXRIDES 0, 100`; `ENDLEVEL` | unreachable |

### Lesson 5 — "Big rides and the Wild West" (`ObjList5`, `FIVE.MAP`)

`CURRENCY 1000`, `WORKERS 1, 1`, `FEATURE AUTOREPAIR/ENERGY/RIDEWEAR 1`,
`MAXBLOKES 5`, `LOOKAT 50, 25`. `INIT` `PLACE`s an *incomplete* Boating School
at (45,41) with eleven `BOATING SCHOOL WATER` pieces and a `BOATING SCHOOL
MERMAID`, and `GLUE`s four regions. `MECHANICS HUT` (52,5), `POTTING SHED`
(40,5), `ENTRANCE 1` (57,28), 30 hedges and 15 cacti.

| # | objective | check | played |
| --- | --- | --- | --- |
| A | (reminder) | `FIXRIDES 0, 25` | active |
| 1 | gardeners, then 15 flowers in the hedges | `NEEDIN "FLOWERS" 5/10/15, (49,26),(51,18)` | **NO — P3-2**; the flower ORDERS are placed correctly and never built |
| 2 | put the gardeners back | `NEEDGARDENERS 0` | unreachable |
| 3 | a power station | `NEED "SMALL POWER STATION", 1` (+ `[PERMANENT]`) | unreachable in sequence (the class is in the menu and buildable) |
| 4 | four mechanics | `NEEDMECHANICS 1/3/4` (+ `[PERMANENT]`) | **NO — P3-1 and P3-2** |
| 6 | link the Boating School | `LINK "BOATING SCHOOL"` -> `GIVE` water + mermaid | unreachable |
| 7 | loop the water | `LOOPCOMPOSITE "BOATING SCHOOL", 5` -> `INTERVAL "boating school.txt"`, then `SAVE 400` | unreachable |
| 8 | the Western menu | `SELECTTHEME WESTERN` -> `GIVE` the six Western classes, `FMV "Western.avi"` | unreachable |
| 9 | build them in the marked corner | `NEEDIN` x3 in (0,28),(26,0); `ENDLEVEL` | unreachable |

---

## 2. How far each lesson goes, with hashes

Served `portable/build-wasm` on **8822**, page
`legoland.html?args=-nointro+WINDEBUG&beat=1000`, one fresh tab. Frames are
`llFrameHash()`; the load-bearing column is `llPark().money`, which is the
game's own `g_bricks`.

**Reaching lesson 4 or 5 at all needs the profile to have got there.**
`screens3.c InitTutorialScreen` only builds a marker icon for
`g_level_map->level - 1` or a level whose `g_level_done[i]` is 1, and
`bigscreens.c InitProgressScreen` sets `level = g_last_level + 1`. Rather than
replay lessons 1–3, this lane wrote the profile RECORD: `profiles.c`'s
on-disk `Profile%d.txt` is the raw 0x110-byte `Profile` struct and the fifteen
bytes at **+0x34** *are* `g_level_done[0..14]`. Setting five of them to 1 and
reloading lights all five markers (`p3UnlockLessons` in the lesson-4 replay).
A single click on a marker selects it, the notepad's Thumbs Up starts it.

| screen | hash | `llPark()` | fps |
| --- | --- | --- | --- |
| PLAYER DETAILS | `0x96453784` | — | 34.3 |
| slot 1 selected | `0x3aeacff0` | — | 34.8 |
| TITLE screen (a profile exists) | `0x46e23314` / `0x82efac95` | — | 35.6 |
| SELECT TUTORIAL LEVEL, all five | `0xe568e8d8` | — | 34.7 |
| Lesson 4 lit | `0x078eef44` | — | 34.0 |
| Lesson 4 BRIEFING | `0x0976b504` | **money 450** | 33.5 |
| **Lesson 4 park** | `0x0fc919e9` | money 450, people 1 | 33.8 |
| obj 1 done, COPTERS granted | `0x72ac5a40` | money 460 | 32.4 |
| COPTERS built | `0x068f3c4c` | **money 400**, visitorLimit 2 | 35.1 |
| obj 3 prompt, east clamp | `0x473fc700` | money 540, visitors 2 | 34.0 |
| Lesson 5 lit | `0x061a7e24` | — | 34.0 |
| Lesson 5 BRIEFING | `0x2f1399c4` | **money 1000** | 32.3 |
| **Lesson 5 park** | `0x167ceecc` | money 1000 | 33.7 |
| "Boating School Entrance" popup (90) | — | money 1000 | 33.7 |
| LEGOLAND menu (4 classes) | `0x5a4f7960` | money 1000 | — |
| full-map screen | `0xb7ee2bd8` | — | 33.4 |
| seven flower orders placed | `0x8c742ebc` | money 1000 (never charged) | — |

**Lesson 4 reaches objective 3 of 9. Lesson 5 reaches objective 1 of 9.**
No trap of any kind was named all session: `llStats().dead` stayed `null` and
`traps` stayed `[]` across roughly 60 000 presented frames in three page loads.

**fps: 32.3 – 35.6, mean ~33.9.** `FlipPrimary` (sysmisc.c:569) spins until
28 ms have passed, so the game's own ceiling is 35.71 fps; the port is at
~95% of it with a park, a composite ride, four rides flashing their damage
recolour and the full HUD. Nothing here is near the frame budget. (No coaster
figure — see §0.)

---

## 3. Findings

| # | sev | what | evidence | owner |
| --- | --- | --- | --- | --- |
| **P3-1** | **blocker** | **The map view clamps ~475 px short of every edge, so the north-east of both tutorial maps — including the MECHANIC'S HUT the lessons tell you to click — can never be brought on screen.** `scrolltick.c`'s `ClampScrollToMap` reads its four slack values `g_view_left/top/right/bottom` (0x004b95f4..0x004b9600, shipped **243200** each) as **0**, because `coaster3d.c`/`coaster10.c` declare the same four NAMES at 0x008299ac..0x008299b8 and gen_link emitted those (zeroed) ones. | §3.1 | **PORT-M** (rename one set), PORT-A confirms the extent fallout |
| **P3-2** | **blocker** | **No gardener and no mechanic can ever be hired.** `fpui2.c` declares `g_popup` at **0x007fdec0**; `bighelp.c` and `popup.c` declare the same name at **0x007fdea4**. gen_link emits one object, at 0x007fdea4, so every `g_popup` field `PopUpInfoSetUp` touches is **0x1c (28 bytes) low**. `InitPopUpInfo` stores the shed/hut class elements at +0x10c/+0x110; `PopUpInfoSetUp` compares against +0xf0/+0xf4, which read 0 — so the "this is a hut, hire someone" arm never fires. It also stomps three pointer fields. | §3.2 | **PORT-M** |
| **P3-3** | high | **Fifteen more names are declared at two different addresses**, tree-wide, including `g_ui_flags`, `g_frame_ticks`, `g_tile_sprites` (coaster.c loses), `g_view`, `g_screen`, `g_road_tiles`, `g_lls_accept_on_report` (screens3.c loses — the tutorial screen's own Accept sprite). | §3.3, the sweep script | **PORT-M / PORT-A** |
| P3-4 | medium | `llLink(name)` / `llPad(name)` answer `found: true, insts: []` for a class that IS placed (`MECHANICS HUT`, `ENTRANCE 1`, `FLOWERS`): they match on the ODF *element* name while the placed instance carries the display name ("Mechanic's Hut", "Park Entrance"). `llCellAt` shows the object perfectly. A probe limitation, not a game defect, but it reads as one. | §3.4 | PORT-B (page probes) |
| P3-5 | low | The lesson-4 objective panel clips its last line: the INTRO text ends "...Click the LEGOLAND menu when you're" with "done." missing. The PROMPT (four lines) fits. Not measured against the original's box. | screenshot `p3-l4-*`; text in `ObjList4.txt` | undecided — needs the original |
| P3-6 | info | The QUERY readout on a damaged ride draws the ride thumbnail and a wrench badge but no damage number or bar, while the script says "click on a ride to find out **how much** damage it has". May be faithful. | `p3-l4-query-panel.png` | undecided |
| P3-7 | info (harness) | A hidden Browser pane drops the page to **0.3 fps**: the port yields through `emscripten_sleep` (setTimeout), which Chrome throttles to 1 Hz in a hidden tab. A quiet oscillator makes the tab "audible" and restores 34 fps. Worth knowing for every P lane. | `p3KeepAwake()` | — |

### 3.1 P3-1 — the scroll clamp reads zeros

`scrolltick.c:28-31`

```c
extern int g_view_left;         /* 0x004b95f4 */
extern int g_view_top;          /* 0x004b95f8 */
extern int g_view_right;        /* 0x004b95fc */
extern int g_view_bottom;       /* 0x004b9600 */
```

`coaster3d.c:104-107` (and `coaster10.c:48`, `unref3.c:55-58`)

```c
extern int g_view_left;                                         /* 0x008299ac */
...
g_view_left = lpConfig->x;            /* coaster3d.c:469 — and it WRITES them */
```

The exe has 243200 (0x3b600) at each of 0x004b95f4..0x004b9600 (read straight
out of `original/legoland.exe` through `gen_link.load_image`). The generated
`globals.c` defines `g_view_left[1]` **at 0x008299ac, uninitialised**, and has
no symbol at 0x004b95f4 at all: the four initialised words were absorbed into
the preceding pointer, which is emitted as
`g_cursor_sprite_put[5] = { &g_cursor_sprites, 0x3b600, 0x3b600, 0x3b600, 0x3b600 }`
(`render5.c:89` declares it as a bare `CursorSprite*`, so its extent ran to the
next known symbol, `g_view_w` at 0x004b9604).

**Measured, twice, on two different maps.** Hold the cursor in the right-hand
margin until `MouseScrollMap` (pathtile2.c) stops moving the view, then read
the scroll pair:

| map | `g_scroll_x` | `g_scroll_y` | `view_w<<8` | `2*y - vw` (the clamp with slack 0) |
| --- | --- | --- | --- | --- |
| FOUR.MAP | **-29696** | 67072 | 163840 | **-29696** exact |
| FIVE.MAP | **-5120** | 79360 | 163840 | **-5120** exact |

That is edge clamp 1 of `ClampScrollToMap` — `d = x - 2y - hl + vw`, pushed
until `d <= 0` — settling exactly on `hl == 0`. With the shipped 243200 the
half-value `hl` is 121600 in 8.8, i.e. **475 px of extra eastward travel**.

Consequence, measured: at the clamp the east-most reachable map column is
`cx - cy = 31` (FOUR.MAP) / `36` (FIVE.MAP). The Mechanic's Hut is at
`cx - cy = 40` (FOUR) and `47` (FIVE) — 143 px and 183 px beyond the canvas,
both comfortably inside the 475 px the slack would have given. The hut is
really there (`llCellAt(46,6)` -> owner "Mechanic's Hut", base (46,6)); the
lesson's own reward `LOOKAT 45, 5` is trying to show it to the player and the
clamp refuses.

The full-map screen is not a way round it: clicking the hut on the minimap
draws the requested viewport rectangle in green over the hut and leaves the
real (white) one where it was, with `g_scroll_x/y` unchanged — `mapscreen.c:326`
runs the same clamp (`p3-l5-mapscreen-clicked.png`).

**Fix is a rename, not a value.** The two sets are unrelated (a scroll slack
box vs the coaster's 3D clip rectangle) and in the original they are different
memory; in the portable build they are the same four dwords, so a coaster
draw would also stomp the scroll geometry.

### 3.2 P3-2 — `g_popup` is sheared by 0x1c, and hiring is the casualty

```
bighelp.c:456   extern PopUpUI   g_popup;   /* 0x007fdea4 */
popup.c:661     extern PopUpUI   g_popup;   /* 0x007fdea4 */
fpui2.c:1443    extern PopUpInfo g_popup;   /* 0x007fdec0 */     <-- 0x1c higher
```

Both spellings agree on the ABSOLUTE address of every field (elem_shed is
0x007fdfb0 = 0x7fdea4+0x10c = 0x7fdec0+0xf0), so the original links correctly.
The portable build emits **one** object: `gen-browser/globals.c` has
`g_popup[95] __attribute__((alias("g_info_icon_g")))` and `extents.md` row
`0x007fdea4 | 376 (0x178)`. `fpui2.c` therefore indexes the 0x007fdea4 object
with 0x007fdec0-based offsets.

**Measured in the tab.** Click the Greenhouse in lesson 5, then dump the
object (its base is at linear 2071808 in that session; `p3PopupShear()` finds
it from any clicked object's pointer):

```
+0x000  0x103        <- fpui2 PopUpInfo.type   (belongs at +0x01c; +0x000 is Icon* icon_mech)
+0x004  13629936     <- fpui2 .obj             (+0x020)          (+0x004 is pad_a8)
+0x008  1320         <- fpui2 .ref             (+0x024)          (+0x008 is Sprite* spr_full)
+0x00c  580          <- fpui2 .pos.x  = the click's SCREEN X     (+0x00c is Sprite* spr_norepair)
+0x010  92           <- fpui2 .pos.y  = the click's SCREEN Y
+0x0bc  20269448     <- fpui2 .cls    (the ObjDef)               (+0x0d8 in PopUpUI)
+0x0c4  16716640     <- fpui2 .cell                              (alias g_popup_cell is +0xe0)
+0x0c8  1320         <- fpui2 .cellpos (0x528 = cell 40,5)
+0x0dc  0x103        <- fpui2 .kind
+0x0e0  1            <- fpui2 .active
+0x0e8  1            <- fpui2 .resize
+0x0f0  0            <- fpui2 .elem_shed   READS ZERO
+0x0f4  0            <- fpui2 .elem_hut    READS ZERO
+0x10c  13629936     <- InitPopUpInfo's REAL elem_shed ("POTTING SHED")
+0x110  13630236     <- REAL elem_hut ("MECHANICS HUT")
+0x114  13628176     <- REAL elem_path
+0x118  13628516     <- REAL elem_entrance
```

Every one of `PopUpInfoSetUp`'s stores is exactly 0x1c below where the other
two TUs read it, and the two comparands it needs are zero. So in

```c
ce = g_popup.cls->elem;            /* = the POTTING SHED element, non-zero */
if (ce == e) { ... CanHireGardener(); GenerateGardener(&pos, 1); }
```

`e` is 0 and the arm never runs — for the shed **and** for the hut. The same
function routes worker pick-up and put-down (types 0x306/0x307/0x308), so the
whole worker UI is dead with it, which is also `FIXRIDES`' only input.

**Symptom, both lessons:** clicking the Greenhouse (lesson 5, cell (40,5),
`llSel()` gives `hit 0x103`, `selDef.name "Greenhouse"`, `selDef.elem "POTTING
SHED"`, `editMode 0`, 1000 bricks in hand) hires nobody and costs nothing —
`CanHireGardener` (sysstubs.c:326) would have taken 30 bricks. Four clicks,
money 1000 -> 1000. Lesson 4's Mechanic's Hut cannot even be clicked (P3-1).

**The work-order half is fine**, which is what makes the diagnosis tidy:
ordering seven flowers inside the hedge zone leaves seven cells with
`owner "Flowers"`, `flags 0x8800`, `isFootprint false` — orders correctly
raised, waiting for a worker who can never be hired, so `NEEDIN "FLOWERS" 5`
never satisfies and money is never charged.

Also latent: `+0x000`, `+0x008` and `+0x00c` are `Icon*` and two `Sprite*` in
the real layout, and a click overwrites them with 0x103 / a cell id / a screen
x. Nothing has dereferenced them yet this session; the "no repair" and "full"
badges are what would.

A live poke of the two element pointers into +0xf0/+0xf4 does NOT unblock the
hire, because `PopUpInfoSetUp` calls `ResetInfoStruct()` before the comparison
and that clears the block — the poke is gone by the time the compare runs.
The fix has to be the declaration.

### 3.3 P3-3 — the class, swept

`extern <type> NAME ...;  /* 0x00...... */` across all 258 game sources:
**2675 names, 17 declared at more than one address.** Winner = the address
gen_link actually emitted (from `globals.c` / `aliases.c`).

| name | winner (gen_link) | losing spelling(s) | note |
| --- | --- | --- | --- |
| `g_view_left/top/right/bottom` | 0x008299ac.. (own definitions) | `scrolltick.c` 0x004b95f4.. | **P3-1** — the clamp reads the coaster's zeroed clip rect |
| `g_popup` | 0x007fdea4 (alias of `g_info_icon_g`) | `fpui2.c` 0x007fdec0 | **P3-2** — every field 0x1c low |
| `g_ui_flags` | 0x00813a40 (alias of `g_cursor`) | `logflume2.c`, `screencb6.c`, `unref4.c` 0x008003e8 | the flume and a screen callback read the cursor's flags |
| `g_tile_sprites` | 0x00805f60 (own) | `coaster.c` 0x0082c680 | the coaster's tile sprites are the map's |
| `g_view` | 0x007fffc4 (`EditCursor`+0x1404) | `sysmisc2.c` 0x004bcbf4 | two of three spellings win |
| `g_screen` | 0x004bcbf4 (alias of `g_game`) | `printlist.c` 0x00668078 | |
| `g_road_tiles` | 0x004cbeac (alias of `g_road_list`) | `roads.c` 0x004b4c08 | |
| `g_frame_ticks` | 0x006681fc (own) | `schoolcar.c` 0x0060f910 | the school car's timing |
| `g_snd_click` | 0x004b92c0 (`g_game_fx`+0x98) | `fpui4.c` 0x004b929c | one UI click plays FX entry 12 instead of 11 |
| `g_lls_accept_on_report` | 0x004bf694 (own) | `screens3.c` 0x004bef70 | the tutorial screen's own Accept sprite name |
| `g_avi_open_count` | 0x00665f48 (own) | `movie.c` 0x00668f98 | |
| `g_carousel_bnv` | 0x0061608c (own) | `screencb2.c`, `screencb6.c` 0x00616090 | +4 shear |
| `g_carousel_zspr` | 0x006160b8 (own) | `ridecb3.c`, `screencb2.c`, `screencb6.c` 0x006160c0 | +8 shear |
| `g_bz_bnv` | 0x00616010 (own) | `screencb2.c`, `screencb6.c` 0x00616018 | +8 shear |

A winner that is an ALIAS carries its offset (`g_snd_click` is
`g_game_fx+0x98` = 0x004b92c0, not the array's base) — a check that names an
alias by its target alone reads these wrong.

The sweep, in full — every address comment in the tree is already there to
check against, and no VC6 gate can see any of this because the bytes are
identical:

```python
DECL = re.compile(
    r'^\s*extern\s+[^;()]*?\b(\w+)\s*(?:\[[^\]]*\])*\s*;\s*/\*\s*(0x00[0-9a-fA-F]{6})')
addrs = defaultdict(set); where = defaultdict(list)
for fn in sorted(os.listdir('LEGOLAND')):
    if fn.endswith('.c'):
        for i, line in enumerate(open('LEGOLAND/' + fn), 1):
            m = DECL.match(line)
            if m:
                addrs[m.group(1)].add(m.group(2).lower())
                where[m.group(1)].append((fn, i, m.group(2).lower()))
bad = {n: v for n, v in addrs.items() if len(v) > 1}      # 17 of 2675
```

Some are certainly benign (the `unref*.c` entries agree with the winner). The
four small-delta ride ones (`g_carousel_*`, `g_bz_bnv`) are the same shear
shape as `g_popup`: one struct spelled from two bases. This belongs in the
round gate beside `bvstruct_sweep.py`.

### 3.4 P3-4 — `llLink`/`llPad` miss classes that are placed

`llLink('MECHANICS HUT')` -> `{found: true, insts: []}` while
`llCellAt(46,6)` -> `{owner: "Mechanic's Hut", base: [46,6], flags: 0xc0}`.
Same for `ENTRANCE 1` ("Park Entrance") and `FLOWERS` before they are built.
The probe matches the LLIDB element name against the instance list of the ODF
that carries the display name. Ten minutes were lost to "the hut was never
instantiated" before `llCellAt` settled it.

---

## 4. How to reproduce

```
emcmake cmake -S portable -B portable/build-wasm -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DLL_ILP32=ON -DPython3_EXECUTABLE=$PY
ninja -C portable/build-wasm
ninja -C portable/build-wasm legoland_browser legoland_browser_named
python -m http.server 8822 --directory portable/build-wasm
# open legoland.html?args=-nointro+WINDEBUG&beat=1000, then in the console:
#   <paste p3-lesson4.js>  ; await p3KeepAwake()
#   (create a profile in slot 1 once) ; await p3UnlockLessons(5) ; location.reload()
#   await p3Lesson4()      // stops at objective 3
#   <paste p3-lesson5.js>  ; await p3Lesson5()   // stops at objective 1
```

Screenshots were taken with `llSnapshot()`/`llCrop()` POSTed to a scratch
receiver rather than through the pane, because a hidden pane crops the
`computer screenshot` frame (and see P3-7).

## 5. Files touched

| file | change |
| --- | --- |
| `docs/SCOPE_PORT_WAVE.md` | the PORT-P3 status line |
| `docs/lanes/scope-port-p3.md` | these notes |
| `portable/src/browser/replays/p3-lesson4.js` | lesson 4 replay + `p3UnlockLessons` / `p3KeepAwake` |
| `portable/src/browser/replays/p3-lesson5.js` | lesson 5 replay + `p3PopupShear` |

No `LEGOLAND/*.c`, no `portable/src/hostwin/**`, no `portable/src/browser/`
`{main.c,index.html,ll_canvas.js}`, no `portable/tools/**`, no
`portable/cmake/**`, no `docs/HANDOFF.md`. `tools/verify.py`, `audit.py` and
`match.py` were not run.
