# Scope PORT-P5 — PLAY lessons 2, 3 and 5 on the PORT-M17 tree

> **PORT-P5 — Status: DONE (2026-09-12)** — branch
> `scope/PORT-P5`, cut from the PORT-P4 merge (`63ee1432`). A PLAY lane:
> nothing in `LEGOLAND/*.c` is touched. `portable/src/browser/main.c` gains
> eight READ-ONLY debug-table rows (the side panel's own list) and this lane's
> replays. Brief: `docs/SCOPE_PORT_WAVE.md`.

**Result in one line: PORT-M17 closed P4-1 completely — blokes are clickable,
`WorkerPopUp` fires, a gardener can be picked up, carried and put down — and
with it BOTH lessons P4 could not finish now finish: LESSON 2 RUNS TO
`ENDLEVEL` (all 8 objectives) and LESSON 3 RUNS TO `ENDLEVEL` TWICE (all 8
objectives, on two cold runs). P4-2 DOES NOT REPRODUCE: on both cold runs the
Spider Ride is in `g_object_list` with `keep = 1` and arms from the panel, and
so are the Space Tower Ride and the Small Power Station — P4's five-name list
is entries 0..4 of an eight-entry list read through a four-slot scroll window
by a driver whose "is the panel open" test is a canvas pixel that answers TRUE
while `g_menu_index == 5`. Lesson 5's `NEEDGARDENERS 0` — P4's second
BLOCKER — drains: four gardeners picked up and dropped on the Potting Shed,
4 → 3 → 2 → 1 → 0, step 6 → 7. One NEW defect: `CheckWorkerOnMouseStatus`
(workers2.c:1235) reads the cursor Y from the WRONG ADDRESS, so a worker can
never be put down on plain ground — only onto a work order or a worker
building. P4-3 is still present and is now pinned to one 200x19 rectangle and
to the exact moment it turns black.**

| lesson | PORT-P4 reached | PORT-P5 reaches | stopped by |
| --- | --- | --- | --- |
| **2** | objective 1 of 5 | **ENDLEVEL — every objective, the level ends and lesson 3 lights** | — |
| **3** | objectives 1–6 of 8 | **ENDLEVEL — every objective, TWICE, on two cold runs** | — |
| **5** | objectives 1–5 of 9 | **objectives 1–13 of 19**, including `NEEDGARDENERS 0` and `LINK "BOATING SCHOOL"` | the `LOOPCOMPOSITE` water loop (a build puzzle, not a defect — §5.4) |

`llStats().dead` was `null` and `traps` `[]` in every frame of every session
below, fps 35.4–35.8 throughout, and `llPark().ghosts` was **0** in every park
(none of these was loaded from a save — PORT-M18's warning observed).

---

## 1. Findings

| # | sev | what | evidence | owner |
| --- | --- | --- | --- | --- |
| **P5-1** | **HIGH** | **A picked-up worker can never be put down on ordinary ground.** `CheckWorkerOnMouseStatus` (workers2.c:1235) tests `g_cursor.y2 < 0x20 \|\| g_cursor.y2 >= 0x174`, and `y2` is declared at `CursorState +0x0c` = **0x00813a4c**. The original reads **0x00813a48** — `g_input.point.y`, the cursor Y. 0x813a4c is `g_input.mouse_a.MASK`, a constant **1** written once by `SetupControllers`, so `1 < 0x20` is true on every drop and the body takes `goto fail` → `g_drag_lock = 1` → `SetWorkersPositionAtMouse()`, i.e. the worker stays glued to the cursor for ever. Shared by the VC6 build: this is a transcription error in a WIP body, not a wasm artefact. **`relocs.py` cannot see it — the gate only checks MATCHED functions and this one is WIP (55.4%).** | §3.2, and the A/B in §3.3 | **a matching / decomp lane that owns `workers2.c`** (`CheckWorkerOnMouseStatus`, 0x00470620) |
| **P5-2** | medium (harness, but it invented P4-2) | **`P4.panelOpen()` answers TRUE while the panel is shut.** It samples canvas pixel (5,200); measured in tutorial lesson 5 with `g_menu_index == 5` and `g_object_list` empty, it returned `true`. Every helper built on it (`P4.armByName`, `P4L3.arm`, `P4L5.flowers`, `P4L3.panelNames`) then skips opening the panel, so the slot clicks land on the MAP, nothing arms, and `P4.armed()` keeps returning the previously-armed name — which reads back as a short panel list. The honest oracle is `g_menu_index` (0..3 = that theme's list is built, 5 = none). | §4.1 | PORT-B (the page) / every play lane |
| **P5-3** | medium | **P4-3 is still present, and it is ONE rectangle with a trigger.** Game **x 228..427, y 3..21** — a 200x19 band inside the top status bar's gold frame — goes **pure black (92.0–92.7 % of pixels exactly `#000000`)** and stays black. It is 0.0 % for the whole of lesson 3 up to and including step 13, and is 92.7 % the first time it is sampled after `g_script_cur` reaches **step 14** — i.e. across step 13's reward, `FMV "Spider.avi"` + `GIVE "SPIDER RIDE"`. Six 1 s samples and a MAP round trip leave it identical. The money digits print on top of it in white, so the FILL is wrong and the text is right (P4's reading confirmed, and narrowed). | §6 | a render / GDI-text lane — **and test the AVI stub first**, not `SetBkMode` |
| P5-4 | note | **The tutorial HELP NOTEPAD is a third modal that eats every click**, distinct from P4-8's "You have a new object" pop-up. It is full-canvas, `llPark().screenMode` stays **7**, `llStats().modal` reads `—`, and every hover answers hit `0x100`. It cost this lane four measurements read as "the ERASER button does nothing" and "the shop cannot be selected". Dismissed by turning the page (the flashing arrow, game **(458,447)**) to the last page and then the Thumbs Up at **(577,419)**. `P5.clearModals()` does it and proves it. | §2.3 | — (game behaviour; a harness trap) |
| P5-5 | note | **`g_hit_info` is STICKY.** `Render3DPerson` writes it when a person's model paints the mouse pixel and nothing clears it when the next hover hits nothing, so the FIRST probe of a fresh sweep reads back the PREVIOUS sweep's last person hit. A driver that trusts one read clicks a pixel 150 px from any bloke and concludes the pick-up does not work. `P5.findPerson` parks the cursor on a known person-free pixel and requires the record to fall away before believing a hit. | §3.1 | — (a harness trap) |
| P5-6 | note | **The Spider Ride's footprint is 16x18 cells** (`left -7, top -8, right 8, bottom 9`), so `P4.buildNamed` answers "no legal square in view" on a park that has room for everything else. THREE.MAP has 57 anchors that fit it; `P4.clearArea(4)` finds none of them. This is what actually stopped the first attempt at lesson 3's last objective, and it is not P4-2. | §4.3 | — (game data) |
| P5-7 | note | **P1-7's ghosts were never in the way.** `llPark().ghosts` read 0 in every park this lane played, because a tutorial lesson entered from the progress screen is BUILT, never LOADED. PORT-M18's warning applies only to a lane that resumes from a save. | throughout | — |

### What is CLOSED

| was | filed by | now |
| --- | --- | --- |
| **P4-1** — no bloke can be clicked, so no worker can be picked up, and lessons 2 and 5 are unfinishable | PORT-P4 §2.3, §4.2 | **CLOSED by PORT-M17.** §3.1: 960 probes over four penned Gardeners answer **186 person hits** across all four Bloke records (P4 measured 0 in the same sweep); `WorkerPopUp` fires and parks the bloke in `g_wp_obj`; §5.2 fires four Gardeners and drains `NEEDGARDENERS 0` |
| **P4-2** — lesson 3's LEGOLAND panel loses the Spider Ride, the Space Tower and the Small Power Station, and the lesson cannot end | PORT-P4 §3.4 | **DOES NOT REPRODUCE**, on two cold runs, read from `g_object_list` itself. §4 — all eight classes are in the list with `keep = 1`, all eight arm, and **lesson 3 reaches `ENDLEVEL` both times** |

---

## 2. What was measured, and with what

Served `portable/build-wasm` on **8875**,
`legoland.html?args=-nointro+WINDEBUG&awake=1`, this lane's own tab, virgin
IDBFS, profile `p5` in slot 1 with `g_level_done[0..4] = 1` written into the
on-disk record (PORT-P3's poke, unchanged). Canvas asserted at **642x482 CSS
px** before the first click (PORT-P4's P4-4) and `P4.fast()` installed (P4-5).
Cold-load frame hash `0xe5b5d741` on the build carrying this lane's probes,
`0x093021ac` on the base — the difference is the page's own header, not a game
pixel.

### 2.1 The probes this lane added

`portable/src/browser/main.c` gains **eight** rows, 133..140, all reads. They
are the side panel's own list, which nothing in the table exposed before:

```
g_object_list      0x00668e40  ObjNode* {next, ObjDef*, keep}
g_object_list_mode 0x00668e34  0 = build list, 1 = research
g_menu_index       0x004baff8  0..3 = that theme's list is built, 5 = none
g_list_menu        0x00668e64  the menu the list was last built for
g_menu_dirty       0x0066871c  "rebuild the panel next tick"
g_menus            0x004bafa8  Menu[4], the theme element NAMES
g_submenus         0x004baffc  Menu[4], SCENERY / FOOD STORES / SHOPS / ATTRACTIONS
g_side_icons       0x006687c8  the panel's own icon chain
```

`replays/p5-00-prelude.js` adds, on top of PORT-P4's `P4`: `P5.guard()` (the
stolen-tab assertion), `P5.objectList()`, `P5.classGate()`, `P5.panelTop()`,
`P5.openPanel()` / `P5.menuIndex()` / `P5.arm()` (the honest panel oracle),
`P5.personSweep()`, `P5.findPerson()` / `P5.carry()` / `P5.drop()` /
`P5.workerCells()`, `P5.cursor()`, `P5.clearModals()` and `P5.blackPct()` /
`P5.blackWatch()`.

### 2.2 Why reading `g_object_list` matters more than it sounds

PORT-P4 answered "what does the panel offer" by ARMING every icon slot and
reading `g_edit_object` back. That cannot tell three different things apart:

* the class is not in the list at all (the `GIVE` did not take, or it failed
  `ObjectLinkedList`'s gate);
* it is in the list as a **child** (`keep == 0`) of a collapsed parent, so
  `MakeUpObjectList` draws a bar where its icon would be;
* it is in the list, with an icon, **outside the four-slot scroll window**.

P4-2 is the third. §5.3 is the first — the Boating School's Water Way, which is
genuinely un-armable until its bar is expanded, and which is what a real
instance of P4-2 would look like.

### 2.3 Three modals, not two

PORT-P4 documented the "You have a new object" pop-up (P4-8). There are three,
and each answers hovers with a constant hit type while it is up:

| modal | closed by | what it looks like to a driver |
| --- | --- | --- |
| "You have a new object" | the close box, game (545,235) | every hover reads hit `0x1` |
| **the tutorial HELP NOTEPAD** (P5-4) | the flashing page arrow, **(458,447)**, to the last page, then Thumbs Up **(577,419)** | every hover reads hit `0x100`, `screenMode` stays 7, `llStats().modal` reads `—` |
| the briefing / appraisal notepad | Thumbs Up (577,419) | as above |

The help notepad cost this lane four measurements: with it up the ERASER button
at (204,442) does nothing, `editState` will not change, and the two shops answer
`destroyCursor.status = -1` on every cell they own. All three of those read as
separate defects and none of them is one. `P5.clearModals()` tries all three
points and proves the result by hovering two map pixels and requiring two
different hit records.

---

## 3. Lesson 2 — P4-1 is closed, and the lesson runs to `ENDLEVEL`

### 3.1 Blokes can be clicked

PORT-P4's own measurement, repeated on the PORT-M17 tree. Four Gardeners in the
hedge pen on TWO.MAP, the pen centred at screen (325,200), a 4-px sweep in a
56x60 px box around each Gardener's own cell:

| | PORT-P4 (pre-M17) | **PORT-P5 (post-M17)** |
| --- | --- | --- |
| probes | 578 | **960** |
| hit types | `0x109` 311, `0x103` 193, `0x10a` 74, 0x1, 0x100 | `0x109` 580, `0x103` 194, **`0x307` 184**, **`0x306` 2** |
| **person hits** | **0** | **186** |
| distinct Bloke records seen | — | **5** (the four Gardeners as `0x307`, plus one visitor as `0x306`) |
| `g_worker_on_mouse` after clicking one | 0 | **the Bloke's own address** |

Reproduced on a second cold run of the same lesson: 960 probes, 186 person hits,
the same four `0x307` records. A click on any of those pixels routes through
`PopUpInfoSetUp` case 0x307 to **`WorkerPopUp`** (fpui4.c:470) — which IS the
pick-up: it parks the bloke in `g_wp_obj` (= `llWorkers().carrying`), sets
`w->state = 13`, clears the work list and raises `g_drag_lock`. Measured:
`carrying` 0 → `22847296`, `carryKind` `0x307`, `dragLock` 0 → 1.

**P4-1 is closed.** The pop-up PORT-P1 and PORT-P4 could never open opens.

### 3.2 P5-1 — and it cannot be put down again

The carried Gardener follows the cursor exactly (its 24.8 world position at
`+0x68/+0x6c` tracks the hovered cell to the unit), and **no click anywhere on
open ground puts it down**. Four cells tried, `g_drag_lock` stayed 1 each time.

`CheckWorkerOnMouseStatus` (workers2.c:1201, `0x00470620`) is the drop, on the
left button's release (`g_cursor.buttons & 2` is `mouse_a.state` bit 1,
"released this tick" — measured rising to exactly 2 on every `mouseup`, so the
gesture does reach the function). Its ground path is

```c
if (g_cursor.y2 < 0x20 || g_cursor.y2 >= 0x174)
    goto fail;                       /* -> g_drag_lock = 1 -> stays on the cursor */
```

and `y2` is declared at `CursorState +0x0c` = **0x00813a4c**. The shipped binary
reads **0x00813a48**:

```
0x004706dd: mov  eax, dword ptr [0x813a48]      ; g_input.point.y -- the cursor Y
0x004706e2: cmp  eax, 0x20
0x004706e5: jl   0x470895                       ; fail
0x004706eb: cmp  eax, 0x174
0x004706f0: jge  0x470895                       ; fail
...
0x00470701: mov  eax, dword ptr [0x813a44]      ; g_input.point.x -- which we DO get right
```

0x00813a4c is bighelp.c's `g_input.mouse_a.MASK` — the Controller bit the left
button is bound to, written once by `SetupControllers` and never again.
Measured live: **1**, for the whole session. `1 < 0x20`, so the test fails on
every drop, on every pixel, for ever. Neither 0x00813a4c nor any other word of
the `mouse_a` pair appears anywhere in the function's 664 bytes.

### 3.3 The A/B, on one build, one word

Arm B writes **0x21** into `mouse_a.mask` and nothing else. 0x21 still contains
bit 0 — the left button — so the events the game receives are identical and the
release is still detected; the only change is that the value the broken test
reads is now inside [0x20, 0x174).

| | cursor Y at the drop | value the test reads | `g_drag_lock` after | the Gardener |
| --- | --- | --- | --- | --- |
| **A — untouched** | 246 | **1** | **1** | stays on the cursor |
| **B — `mask = 0x21`** | 246 | **33** | **0** | **lands on cell (46,13)** |

and then walks away on its own — (46,13) → (46,18) → (47,24) over six seconds,
out of the pen, and starts servicing flower orders. The mask was put back to 1
afterwards; nothing else was written and no game code changed. Replay
`p5-04-the-worker-drop-reads-the-wrong-address.js`.

**Classification.** A port defect it is not: the same `y2` is read by the VC6
build. It is a **transcription error in a WIP body** —
`CheckWorkerOnMouseStatus` is 55.4 % (82/184 strict) — so fixing it should move
the match as well as the behaviour. **`relocs.py` cannot find it**: the
relocation gate only checks MATCHED functions and skips this one outright
(`relocs.py LEGOLAND/workers2.c` reports nine functions attempted, none of them
this one). That gap is worth a line in `docs/HANDOFF.md`.

### 3.4 The route that does work, and the whole lesson

`CheckWorkerOnMouseStatus` tries two things **before** the broken test:
`WorkerHitOnRide()` and then `WorkOrderUnderHit`/`WorkOrderNearHit`, both on hit
type **0x103**. So a worker CAN be put down — onto a cell that carries a work
order, or onto a worker building. That is enough to play the lesson:

| step | what | measured |
| --- | --- | --- |
| 2 | `SELECTTHEME LEGOLAND` | the LEGOLAND button, step 2 → 3, money 1510 |
| 3..7 | `NEED "FLOWERS"` 1..5 | three orders east of the pen at (53,5)/(53,7)/(53,9), each `assigned: 1` (PORT-P4 measured `assigned: 0` for ever) and BUILT — cell flags `0x8800` → `0x8080`; 3 → 5 in one move, then 5 → 7 → 8 |
| 8..12 | `NEED "TREE 1"` 2,4,6,8,10 | twelve Pine Trees, 8 → 12 → 15 |
| 15 | `SELECTMODE ERASE` + `REMOVE` both shops | LEGO Media Shop (36,23) money +25, LEGO Toy Shop (40,36) money +30, 15 → 16 |
| 16 | `NEEDIN "FOUNTAIN 1" 1` in (36,31)..(43,41) | one Small Fountain at (38,33), money 2446 → 2441 |
| — | **`ENDLEVEL`** | `llGoals().stepCount` **0**, `curAddr` **0**, `gameMode` 3 → 2, `screenMode` → 6 (the tutorial screen), **lesson 3 lit** |

0 traps, `dead` null, 35.4–35.8 fps, `ghosts` 0 throughout. Replay
`p5-01-lesson2.js`.

---

## 4. Lesson 3 — `ENDLEVEL`, twice, and P4-2 does not reproduce

### 4.1 What P4-2 actually was

Two cold runs. On both, at step 14 — the objective PORT-P4 could not pass —
`g_object_list` holds **eight** nodes, every one `keep = 1`, and
`P5.panelTop()` arms every one of them from the panel:

```
Flowers | Pine Tree | Small Fountain | LEGO Clothes Shop | LEGO Toy Shop
       | Small Power Station | Space Tower Ride | SPIDER RIDE
```

PORT-P4's five names are **entries 0..4 of that list**, in order. Two things
produce that reading and neither is a game defect:

1. **The panel is a four-slot window with a sticky scroll.** The slots are at
   game y 89/153/217/281, the arrows at y 44 and y 344, and `MakeUpObjectList`
   (fpui2.c:1196) ends with
   `RedrawObjectList(p, 0, g_list_scroll[g_list_menu_u.word & 0xff])` — it
   **restores** the scroll offset on every rebuild. So the offset survives a
   panel close/open *and* a MAP round trip, which is exactly what PORT-P4
   re-checked with. `P4L3.panelNames()` never clicks the UP arrow, so it can
   only ever enumerate a four-entry window from wherever the offset happens to
   be. Measured here: consecutive enumerations of the same eight-entry list
   returned `{Pine Tree, Small Fountain, LEGO Clothes Shop, LEGO Toy Shop,
   Small Power Station, Space Tower Ride}` from offset 1, and
   `{LEGO Clothes Shop, LEGO Toy Shop, Small Power Station, Space Tower Ride}`
   from offset 3 with the down arrow already at its stop.
2. **P5-2 — `P4.panelOpen()` lies.** It samples canvas pixel (5,200). Measured
   in lesson 5 with `g_menu_index == 5` (the panel SHUT, `g_object_list`
   EMPTY), it answered **true**, so `armByName` skipped opening the panel,
   every slot click landed on the MAP, nothing armed, and `P4.armed()` kept
   returning the previously-armed name. `g_menu_index` is the oracle: 0..3 =
   that theme's list is built, 5 = none.

### 4.2 The panel's real gate, for whoever does find a defect here

`ObjectLinkedList` (fpui2.c:572, `0x00475720`) rebuilds `g_object_list` from the
LLIDB on **every** `TestMenu`, so nothing is remembered between rebuilds. Per
class it needs

| | |
| --- | --- |
| `(elem->type_flags & 0x13) == 0x13` | loaded (0x1) \| **AVAILABLE (0x2)** \| type bit (0x10) |
| `d->parent` (ObjDef +0x58) `== "BUILD MENU"` | else the class is a **CHILD** and `InsertChildIntoList` (fpui.c:1087) puts it under its parent with `keep = 0` |
| `d->theme` (+0x5c) `==` this theme, or `"COMMON THEME"` with `g_menu_index == 0` | |
| `d->submenu` (+0x60) `==` one of the four `g_submenus` | |

`EventTick_Give` (eventtick.c:354) → `MarkElemAvailable` (`0x00469900`) is what
sets the 0x2 (and `elem->flags |= 0x20000`, the "new object" pop-up bit), and it
raises `g_menu_dirty`; fpui3.c:694 consumes that with `UpdateMenu`
(panelui.c:95), **which does nothing at all when `g_menu_index == 5`** and the
flag is cleared either way. So a `GIVE` while the panel is shut does not rebuild
the list — the next theme-button click does, which is why it is not observable.

Measured at lesson 3 step 0, all eleven classes, before any `GIVE`:

| class | `type_flags` | gate | parent | theme | submenu |
| --- | --- | --- | --- | --- | --- |
| Flowers | **0x17** | pass | BUILD MENU | COMMON THEME | SCENERY MENU |
| Pine Tree | **0x17** | pass | BUILD MENU | COMMON THEME | SCENERY MENU |
| Small Fountain | **0x17** | pass | BUILD MENU | LEGOLAND THEME | ATTRACTIONS MENU |
| Spider Ride | 0x15 | — | BUILD MENU | LEGOLAND THEME | ATTRACTIONS MENU |
| Space Tower Ride | 0x15 | — | BUILD MENU | LEGOLAND THEME | ATTRACTIONS MENU |
| Small Power Station | 0x15 | — | BUILD MENU | COMMON THEME | ATTRACTIONS MENU |
| LEGO Toy / Clothes Shop | 0x15 | — | BUILD MENU | LEGOLAND THEME | SHOPS MENU |
| Greenhouse / Park Entrance / Path | 0x15 / 0x1015 | — | BUILD MENU | COMMON THEME | — |

The panel at that moment holds exactly the three `0x17` rows and nothing else.
**`type_flags`, not the ObjDef flags at +0x1c, is the discriminator** — PORT-P4
compared +0x1c (`0x480666` for the Spider and for the shops that WERE offered)
and concluded the class record was not the discriminator, which is right; the
panel simply never reads that field.

### 4.3 The list, step by step — cold run 2

| after | step | `g_object_list` (all `keep = 1`, `parent = BUILD MENU`) | arms from the panel |
| --- | --- | --- | --- |
| level load | 0 | *(empty — `g_menu_index` 5; the list is built on the first theme click)* | — |
| six hires | 8 | Flowers, Pine Tree, Small Fountain | all 3 |
| the scenery step | 9 | + **Space Tower Ride** (`tf 0x20017` — the "new" bit) | all 4 |
| Space Tower BUILT | 11 | + **Small Power Station**; the Space Tower **still there** (`tf 0x17`) | all 5 |
| Power Station BUILT | 13 | + LEGO Clothes Shop, LEGO Toy Shop; the Power Station **still there** | all 7 |
| both shops BUILT | **14** | + **SPIDER RIDE** (`tf 0x20017`) | **all 8** |

"A class leaves the panel once one has been built" is refuted directly: the
Space Tower and the Small Power Station are in the list *with* `tf 0x17` after
being built, and arm.

### 4.4 What really stopped the first attempt — P5-6

`P4.buildNamed('Spider')` answers **"no legal square in view"**. The Spider
Ride's footprint is `left -7, top -8, right 8, bottom 9` — **16x18 cells**.
`P4.clearArea(4)` looks for a 9x9 block and finds it nowhere useful; THREE.MAP
has **57** anchors whose full 16x18 footprint is clear. Centre on one and the
build takes: money 903 → 823 (cold run 1, cell (10,33)) and 910 → 830 (cold run
2, cell (18,32)), and step 14 drains.

### 4.5 Both runs end the level

| | cold run 1 | cold run 2 |
| --- | --- | --- |
| six hires, steps 0 → 8 | 1200 → 1020, 30 each, hit 0x103 each | identical |
| PATHSCENERY 25 | 119 beds, tally 45 % | 96 beds, tally 45 % |
| Space Tower / Power Station / two shops | 9 → 11 → 13 → 14 | identical |
| **Spider Ride** | built, 14 → 15 | built, 14 → 15 |
| second Small Power Station | built | built, money 900 → 865 |
| **`ENDLEVEL`** | `stepCount` **0** | `stepCount` **0**, `traps` 0 |

Replay `p5-02-lesson3.js`.

---

## 5. Lesson 5 — 18 of 19, through `NEEDGARDENERS 0`

### 5.1 The opening, reproduced

PORT-P4's readings to the unit: POTTING SHED (40,5) and MECHANICS HUT (52,5)
both centre on screen, four clicks each give 4 Gardeners and 4 Mechanics at 30
bricks apiece (1000 → 880 → 760), hit `0x103` on all eight, and 18 flower beds
inside (49,18)..(51,26) drain steps **3 → 6 in one move** — all three
`NEEDIN "FLOWERS" 5 / 10 / 15`.

### 5.2 Step 6 — `NEEDGARDENERS 0`, the objective PORT-P4 could not reach

`EventTick_Needgardeners` (eventtick2.c:398) passes only when
`GetGardenerCount()` reaches 0, and a Gardener is only removed by
`ControlGardeners` (workers3.c:270) when its `slot` (+0x36) reads 100.
**`WorkerHitOnRide` (`0x00470270`) is what puts it there**: it converts the
cursor to a cell, compares that cell's class element against
`g_popup_info.elem_shed` (0x007fdfb0) when a 0x307 is on the mouse — `elem_hut`
(0x007fdfb4) for a 0x308 — calls `PutWorkerOnRide`, and writes 100 into the
bloke. So the gesture is: **pick the Gardener up and drop it on the
Greenhouse.** That route runs on hit type 0x103, which
`CheckWorkerOnMouseStatus` handles *before* P5-1's broken test.

| | pixel | hover hit | `g_gardener_count` | `g_drag_lock` | step |
| --- | --- | --- | --- | --- | --- |
| 1 | (345,156) | 0x103 | **4 → 3** | 1 → 0 | 6 |
| 2 | (329,164) | 0x103 | **3 → 2** | 1 → 0 | 6 |
| 3 | (373,166) | 0x103 | **2 → 1** | 1 → 0 | 6 |
| 4 | (397,207) | 0x103 | **1 → 0** | 1 → 0 | **6 → 7** |

**P4-1's second consequence is closed with it.** 0 traps.

One loose end worth a line: `g_worker_on_mouse` (0x007fdff0) is **not** cleared
when the worker is removed — it keeps pointing at the freed Bloke. Nothing in
this lane tripped over it (the next pick-up overwrites it), but a lane reading
`llWorkers().carrying` as "is something in hand" will get a false positive after
a firing. Use `g_drag_lock`.

### 5.3 Steps 7..18, and the Boating School

| step | what | measured |
| --- | --- | --- |
| 7..12 | `NEED "SMALL POWER STATION" 1` and `NEEDMECHANICS 1/3/4/1` | one Small Power Station at (2,12), money 832 → 797, drains **7 → 13 in one move** (the four Mechanics already satisfy every NEEDMECHANICS step) |
| 13 | `LINK "BOATING SCHOOL"` | `llLink()` diagnoses it outright: *"MISSING: no reachable path square on the link square"*. The link square is (49,43); its PathSquare (column 49, y34..48) exists but reads `reachable: false`, and the nearest reachable limb is the path at (53,40)/(54,40). The gap is exactly (50,40)..(52,40). One PATH drag from (53,40) to (49,40) and the verdict flips to **SATISFIED**; 13 → 14 |
| 14 | `LOOPCOMPOSITE "BOATING SCHOOL" 5` | see below; three water drags close the loop and it drains **14 → 18 in one move** (LOOPCOMPOSITE, the reward step, `SAVE 400` and the FMV/THEMEICON step all at once) |
| 18 | `SELECTTHEME` WESTERN | the WESTERN tab, 18 → **19**; the Western panel holds all six GIVEn classes (Cacti Cluster, Cactus, Apache Cactus, Sheriff's office, Saloon, Spinning Barrels Ride) |
| 19 | `NEEDIN` Saloon / Sheriff / Spinning Barrels in (0,0)..(26,28) | Saloon at (7,11) money 811 → 781; Sheriff's office at (18,12) money 708 → 678; the **Spinning Barrels Ride does not fit** — §5.4 |

**The Water Way is a CHILD, and this is what a real P4-2 looks like.** The two
classes `GIVE`n at step 13 are not top-level entries:

```
Boating School Water Way  keep=0  tf=0x21017  parent=BOATING SCHOOL  submenu=(none)
Little Mermaid            keep=0  tf=0x20017  parent=BOATING SCHOOL  submenu=(none)
```

so `MakeUpObjectList` takes the `ListChildrenBar` arm and draws a collapsed BAR
where their icons would be — and the children are skipped entirely. They cannot
be armed at all until the bar is expanded, which is exactly what the game's own
hint says: *"Click the arrow below the Boating School Entrance."* The arrow is
at game **(63,330)** with the list scrolled to the top, and clicking it sets
**bit 8 in the PARENT element's `type_flags`** — measured `0x17` → `0x1f` —
which is the flag `MakeUpObjectList` tests
(`prev->obj->elem->type_flags & 8`). After that, `P5.panelTop()` reaches
`Boating School Water Way` on the second page.

Lesson 3 has **no children at all** — every one of its eleven classes carries
`parent = "BUILD MENU"` — which is the last reason P4-2 cannot be what it was
filed as.

The water itself is built by **dragging** between the loop's open ends; a single
click on a valid square places nothing and spends nothing (two clicks at (39,36)
left money at 846 both times). `llSel().editCursor.valid` marks the open ends
and everything else answers cursor error 14. Three drags — (39,31)→(49,31)
(846→836), (39,36)→(39,51) (836→821), (34,41)→(39,46) (821→811) — closed the
loop, and the third drained four script steps at once.

### 5.4 Where it stops, and why that is not a defect

`EventTick_Needin` (eventtick.c:854) counts cells where
`(cell->flags & 0x80) && cell->obj == e->elem && cell->x == x && cell->y == y`
— i.e. the object's **anchor** must be inside the rectangle; a footprint that
spills outside does not count. The Spinning Barrels Ride's footprint is
`left -4, top -5, right 8, bottom 11` = **13x17 cells**, against a goal
rectangle of 27x29 that already carries the park's own path network.

Measured both ways:

* **the game's own cursor**: 3,996 probes across four views covering the whole
  of (0,0)..(26,28) return **zero** legal squares — cursor error 10
  ("something under it", objmap2.c:1837) 3,356 times, error 1 180, error 4 151,
  error 7 164, error 9 145;
* **geometrically**: zero anchors inside the rectangle have a clear 13x17
  footprint — still zero if the Saloon and the Sheriff's office are treated as
  absent, and still zero if the dead-end path column at x=10 (y16..28) is
  treated as erased. The path at x=10 (y10..29), the block at x5..10 / x16..20
  (y8..15), the column at x=27 and the main path along y=29 leave no 13-wide
  gap that a 17-tall footprint can also clear.

So the last objective needs the player to **demolish part of the park's path
network**, which this lane did not do (`FIXRIDES` and `NEEDMECHANICS` are still
live goals and the entrance link depends on that network). It is a build puzzle,
not a port defect, and it is recorded here so the next lane does not re-file it
as one. **Lesson 5 therefore finishes 18 of its 19 objectives.**

Replay `p5-03-lesson5.js`.

---

## 6. P4-3 — still present, now one rectangle and one moment

### 6.1 The rectangle

Game **x 228..427, y 3..21** — a 200x19 band inside the top status bar's gold
frame. A column read at x=300 and at x=420:

```
y 0..2    247,219,156 / 239,203,107   the frame
y 3..21   0,0,0                       PURE BLACK
y 22..31  148,113,16 ... 16,8,24      the frame again
```

and at x=210 the same rows read `49,73,165` / `49,81,165` — the blue plate the
band should be. The share of exactly-`#000000` pixels in that rectangle is
**91.7 – 93.3 %**, identical across six samples 1 s apart and across a MAP
in/out round trip. An ASCII crop (threshold 60, `.` = dark, from game (196,0),
240 x 32) shows what is left:

```
################################................................................########
################################...............#######....########...#########..########
################################..............#########..#########...##########.########
################################.............###....###..###..............####..########
################################.............###....###..###.............####...########
################################.............##.....###..###............####....########
################################.............##.....###..########......####.....########
################################.............###....###..#########.....######...########
################################.............####...###........####.......####.########
################################..............#########.........###........###.########
################################...............########.........###.........###########
```

— the money value ("833") printed in white **on top of** the black. So the FILL
is wrong and the text is right: PORT-P4's reading, now with the shape to prove
it and a rectangle to aim at.

### 6.2 The moment

Cold run 2 of lesson 3 sampled this rectangle at every objective:

| after | step | % black |
| --- | --- | --- |
| level load | 0 | **0.0** |
| six hires | 8 | **0.0** |
| the scenery step | 9 | **0.0** |
| Space Tower built | 11 | **0.0** |
| Power Station built | 13 | **0.0** |
| **first sample after the step reaches 14** | 14 | **92.7** |
| both shops built, Spider built, `ENDLEVEL` | — | 91.7 – 92.0 |

Step 13's reward is `GIVE "SPIDER RIDE"` (with the pop-up bit) **and**
`FMV "Spider.avi"`. The same rectangle is black in lesson 5 by step 14
(93.9 %), where the script has no FMV before step 17 — so the FMV is not
sufficient on its own, and the common factor is a `GIVE` whose pop-up is raised.
Earlier `GIVE`s in lesson 3 (the Space Tower at step 8, the power station, the
two shops) left it at 0.0, so it is not every pop-up either.

**Not classified.** What this lane hands over is: one rectangle, a
0.0 % → 92.7 % transition inside one script step, and the fact that nothing
repaints it afterwards — not a MAP round trip through `InitGameInterface`, not
twelve seconds of park income changing the number printed on it. The cheap first
test is no longer `SetBkMode(TRANSPARENT)` alone; it is **what the AVI stub and
the "new object" pop-up do to that band of the status bar, and who is supposed
to repaint it.**

---

## 7. Owed to other lanes

* **A matching lane that owns `workers2.c`** — P5-1. `CheckWorkerOnMouseStatus`
  (0x00470620) reads `0x00813a4c` where the original reads `0x00813a48`; the
  field `y2` at `CursorState +0x0c` is a phantom and both tests belong on
  `g_cursor.point.y` (+0x08). The body is WIP at 55.4 %, so the fix should move
  the match. The A/B is `p5-04-*.js` and needs no game change to reproduce.
* **`docs/HANDOFF.md` / the integrator** — the relocation gate does not see WIP
  functions, and P5-1 is a wrong-address bug inside one. A sweep of *declared*
  struct offsets against the original's absolute operands, WIP bodies included,
  would have caught it; `relocs.py LEGOLAND/workers2.c` reports nine functions
  attempted and this is not one of them.
* **A render / GDI-text lane** — P5-3 (P4-3), with §6's rectangle and timing.
* **PORT-B (the page)** — P5-2: `P4.panelOpen()`'s pixel test is wrong often
  enough to have invented P4-2. Expose `g_menu_index` in the page's own helpers
  (have `llSel()` carry it) so no driver has to sample a canvas pixel to learn
  whether a menu is open. P4-4 (the canvas collapsing to 0x0 CSS px) and P4-5
  (the driver's `setTimeout` clamp) are both still open and both still cost a
  lane its first session.
* **Whoever plays a level next** — read §2.3 (three modals, not two) and P5-5
  (`g_hit_info` is sticky) before the first click, and never enumerate the
  object panel without scrolling to the top first. `P5.objectList()` answers
  "what does the panel hold" in one read and is never wrong about it.
* **Nothing is owed to a P4-2 fixing lane.** P4-2 does not reproduce; the
  measurements such a lane would have needed are in §4.2 and §4.3 in case it
  ever does.

---

## 8. Gates

| gate | result |
| --- | --- |
| `LEGOLAND/*.c` | **untouched** — this is a PLAY lane |
| `portable/src/browser/main.c` | +8 `LL_DBG_TABLE` rows (133..140) and their `extern` declarations; no behaviour |
| wasm clean build | `emcmake cmake … -DLL_ILP32=ON` + `ninja legoland_browser`, **exit 0** |
| live run | 0 traps and `dead` null across seven page loads and five parks, 33.3–35.8 fps |
| `tools/verify.py` | **not run** (integrator's gate) |

