# Scope PORT-M19 — P4-2: lesson 3's LEGOLAND build panel

> **PORT-M19 — Status: DONE (2026-09-12)** — branch `scope/PORT-M19`, cut from
> `origin/main` `63ee1432`. Brief: `docs/SCOPE_PORT_WAVE.md`. Files: this note,
> `portable/src/browser/replays/m19-01-the-lesson-3-panel-has-every-class.js`,
> read-only probe additions to `portable/src/browser/main.c` and `index.html`.
> **Nothing in `LEGOLAND/*.c` is touched, and nothing needed to be.**

**Result in one line: P4-2 is NOT A DEFECT and there is nothing to fix — on
this tree lesson 3's panel holds every class the script gives it, at every
step, and the lesson finishes all EIGHT of its player objectives including the
Spider Ride. What P4-2 measured is the panel's saved SCROLL, not its contents,
and this note shows the same enumerator returning two different answers from
one list. The lane closes P4-2 and opens M19-1, a NEW BLOCKER found in the same
reproduction: once the Spider Ride is standing with riders the game loop dies
on an integer divide by zero in `Draw3DPersonModel`.**

---

## 0. What was measured, and with what

`portable/build-wasm` served on 8885, `legoland.html?args=-nointro+WINDEBUG&awake=1`,
pane 900x820 (P4-4), virgin IDBFS, PORT-P3's profile poke, PORT-P4's prelude and
`p4-02-lesson3.js` driving. One cold lesson-3 run, start to ENDLEVEL.

**The probe this lane added** — because the question P4-2 asks cannot be
answered by clicking the panel:

* `main.c`'s `LL_DBG_TABLE` gains ten names: `g_object_list`,
  `g_object_list_mode`, `g_list_menu`, `g_list_scroll`, `g_menu_index`,
  `g_menu_dirty`, `g_theme_new_count`, `g_panel_state`, `g_submenus`,
  `g_scroll_flags`.
* `index.html` gains **`llPanel()`**: the 12-byte `{next, ObjDef*, keep}` nodes
  of `g_object_list` (fpui5.c:340) in list order with each node's cost and its
  three menu elements, plus every loaded `ObjDef` with its LLIDB element flags
  decoded and **`ObjectLinkedList`'s own predicate re-evaluated**, so a class
  that is missing says which clause dropped it.

---

## 1. Findings

| # | sev | what | evidence |
| --- | --- | --- | --- |
| **P4-2** | **CLOSED — not a defect** | Lesson 3's LEGOLAND panel holds every GIVEn class at every step (`llPanel().count` 3 → 4 → 5 → 7 → 8 in lock-step with the five rewards), the Spider Ride is offered and BUILDS, step 14 and step 15 both drain and the script list empties at ENDLEVEL. **All eight player objectives complete.** | §2, §3 |
| **M19-1** | **BLOCKER (new)** | With a Spider Ride standing and carrying riders the game loop DIES: `RuntimeError: divide by zero` in `Draw3DPersonModel`'s `zscale = 0x40000000 / ((hi - lo) >> 5)` (person3d.c:1351, inlined into `Render3DPerson`), reached from `SpiderRide_Interact`. The rider's `p->matrix` is ALL ZERO, so every vertex transforms to the origin and the z extent is 0. **Lesson 3 therefore still cannot be ENDED**, even though every objective drains. Owner: the person3d / ride lane that owns P1-6. | §4 |
| M19-2 | note (harness) | `P4L3.panelNames()` and `P4.armByName()` inherit `g_list_scroll[menu]`, which the panel SAVES per menu and restores across rebuilds, close/open and a MAP round trip. Two consecutive sweeps of ONE unchanged eight-node list returned eight names and then seven. **An arming sweep is not a reading of the list.** | §2.3 |
| M19-3 | note | `Anim3D` +0x04 is `AnimOwner*` in blokeanim.c and `Frame3D*` in person3d.c, and `Person3D` +0x3c is `zboost` in person3d.c and `depth` in mechrides.c. Both builds see the same offsets so neither is a port divergence, but one name in each pair is wrong and the second pair is live on M19-1's path. | §4.3 |

### What is CLOSED

| was | filed by | now |
| --- | --- | --- |
| **P4-2** the L3 panel loses the Spider Ride, the Space Tower and the Small Power Station | PORT-P4 §3.4 | **CLOSED, not reproducible.** §2 — the list holds all eight; the enumerator, not the panel, was short |
| "lesson 3 cannot end" (P4-2's consequence) | PORT-P4 | **half closed.** Every objective drains (§3); the ENDLEVEL button is then unreachable for a different reason, M19-1 |

---

## 2. P4-2, read instead of clicked

### 2.1 What the panel is a view of

The side panel is not a view of "the classes this level has". It is a view of
ONE linked list, `g_object_list`, which `ObjectLinkedList` (fpui2.c:573)
rebuilds from scratch each time the theme menu changes. It keeps a class only
if all four of these hold:

```c
if ((e->type_flags & 0x13) == 0x13)          /* loaded | available | ODF type */
    if (d->parent == e_build)                /* hangs off "BUILD MENU"        */
        if (d->theme == e_theme && d->submenu == e_menu)   /* or COMMON THEME */
            InsertObjectNode(d);             /* sorted by cost, cheapest first */
```

`0x2` is the bit `MarkElemAvailable` (movie3.c:293) sets and the bit GIVE
therefore turns on; `0x10` is the LLIDB TYPE nibble for an ODF (llidb.c:29).
So "the panel lost a class" has exactly four places it can come from, and
`llPanel()` reports all four per class.

### 2.2 The list, step by step

`ObjList3.txt` **ENABLE**s three classes at INIT and only **LOAD**s the other
five; the five arrive one reward at a time. The list tracks that exactly, and
the flag watcher (a 120 ms poll of every element's `type_flags` and refcount)
recorded every transition of the session:

| script step | event | `llPanel().count` | element flags |
| --- | --- | --- | --- |
| 0..5 | six Gardeners hired, one objective per click (1200 → 1020) | — | no change |
| 8 | the level as loaded: FLOWERS, TREE 1, FOUNTAIN 1 | **3** | `0x17`; the other five `0x15` |
| 8 → 9 | `REWARD GIVE "SPACE TOWER RIDE", NOPOPUP` | **4** | SPACE TOWER RIDE `0x15` → **`0x20017`** |
| 9 → 11 | Space Tower built (730 → 686); `REWARD GIVE "SMALL POWER STATION"` | **5** | SMALL POWER STATION `0x15` → **`0x20017`** |
| 11 → 13 | station built (696 → 661); `REWARD GIVE "LEGO SHOP 1" NOPOPUP` + `GIVE "LEGO SHOP 2"` | **7** | both `0x15` → **`0x20017`** |
| 13 → 14 | both shops built; `REWARD FMV "Spider.avi"` + `GIVE "SPIDER RIDE"` | **8** | SPIDER RIDE `0x15` → **`0x20017`** |

`0x20000` is the "new" flash bit, cleared by the first render of the class's
icon (`RenderBuildObjectIcon`, fpui2.c:1116) — which is why each row settles
back to `0x17` a few seconds later, and that transition is in the log too. In
the whole session **not one element's `0x2` bit ever went back down**, and no
refcount ever moved: nothing unloaded, nothing was TAKEn, nothing was marked
unavailable. `llPanel().missing` — classes whose predicate holds but which are
not in the list — was **empty at every reading**.

At step 14, the moment PORT-P4 filed P4-2, the list reads

```
Flowers 1 | Pine Tree 4 | Small Fountain 5 | LEGO Clothes Shop 25 |
LEGO Toy Shop 30 | Small Power Station 35 | Space Tower Ride 40 | Spider Ride 80
```

— eight nodes, sorted by cost as `InsertObjectNode` keeps them, all `keep = 1`
(top level, no children bar involved).

### 2.3 The enumerator, and why it gives two answers

`P4L3.panelNames()` arms each of four icon slots, clicks the down arrow, and
repeats six times. `MakeUpObjectList` (fpui2.c:1196) lays the nodes out `0x42`
apart in a `0x154`-tall box and, whenever the icons are taller than the box,
ends with

```c
RedrawObjectList(p, 0, g_list_scroll[g_list_menu_u.word & 0xff]);
```

`g_list_scroll` is a per-menu SAVED offset. It survives the rebuild, a panel
close/open and a MAP round trip — which is the game's own design, the panel
remembers where you left it, and it is also why an arming sweep measures the
sweep's own history as much as the list.

Measured on ONE unchanged eight-node list, back to back in the same session:

| | returned | `g_list_scroll[0]` after |
| --- | --- | --- |
| `panelNames()` #1 (from scroll 0) | **8 of 8**, Flowers … Spider Ride | **-250** |
| `panelNames()` #2 (inheriting -250) | **7 of 8** — **"Flowers" is gone** | -250 |

The class it loses is the cheapest and topmost, because the panel starts
already scrolled. PORT-P4 ran `panelNames()` after **every** build inside
`rides()` — six times — each call inheriting the previous one's offset, and
then re-checked "four ways" with sweeps that all begin from the offset the last
one left. Every one of those re-checks shares the same blind spot, which is why
re-checking did not catch it.

That is the reading this lane can demonstrate. It is not a claim that PORT-P4's
exact session state is recoverable — it is not. What is established is that
(a) the list is complete on this tree, (b) the lesson finishes, and (c) the
instrument PORT-P4 used returns different sets from one identical list.

### 2.4 The other thing worth knowing before re-opening it

The ONLY game-code difference between PORT-P4's base (`be436d48`) and this tree
(`63ee1432`) is **`LEGOLAND/person3d.c`** — PORT-M17's `LL_ASINT` fix. Nothing
in `fpui*.c`, `eventtick*.c`, `movie3.c`, `llidb*.c` or `levelkw*.c` changed
between the two. So if anyone wants to argue P4-2 was real on P4's tree and is
fixed on this one, that is the entire surface available, and it does not touch
the object list.

---

## 3. Lesson 3, finished

| step | goal | what closed it | money |
| --- | --- | --- | --- |
| 0..5 | `NEEDGARDENERS 1..6` | six clicks on the Greenhouse (40,5), hit `0x103` each, one objective per click | 1200 → 1020 |
| 8 | `PATHSCENERY 25` + `NEED FOUNTAIN 1 / TREE 1 / FLOWERS` | 247 flower beds along the paths (cell flags `0x8080`), one Small Fountain, one Pine Tree | 1020 → 730 |
| 9 | `NEED "SPACE TOWER RIDE" 1` | armed from the panel, built at (9,17) | 730 → 686 |
| 11 | `NEED "SMALL POWER STATION" 1` | built at (2,8) | 696 → 661 |
| 13 | `NEED "LEGO SHOP 1" + "LEGO SHOP 2"` | Toy Shop (19,18), Clothes Shop (8,7); `FMV "Spider.avi"` reward fired (the AVI stub is a documented non-trapping no-op) | 671 → 636 |
| **14** | **`NEED "SPIDER RIDE" 1`** | **armed from the panel and BUILT at (20,42)** | **756 → 677** |
| **15** | **`NEED "SMALL POWER STATION" 2`** | a second station; `llGoals().stepCount` → **0**, the script list is empty | — |

Steps 10 and 12 are the `SELECTTHEME` and `[PERMANENT]` re-checks and drained
with their neighbours, exactly as PORT-P4 recorded.

**One harness lesson worth carrying.** The Spider Ride's footprint is
`(-7,-8)..(8,9)` — **16 x 18 cells**. `P4.buildNamed` sweeps the 640x340
VIEWPORT on a 14 px grid, and in a park carrying 247 flower beds there is no
legal anchor in any single view: it reported `no legal square in view` with the
class correctly ARMED (`P4.armed()` = "Spider Ride", `editState` 1) and cursor
error 10 ("something under it"). Read as "the panel will not give me the
Spider", that is P4-2 again, from the other end. `M19.buildBig()` searches the
MAP for an anchor whose whole footprint is free, centres on it, and the ride
goes down first try — 266 such anchors existed at the time `buildNamed` gave up.

0 traps and 35.6–35.7 fps through all of the above.

---

## 4. M19-1 — the Spider Ride kills the game loop

### 4.1 The trap

After the Spider Ride is standing and has taken riders, the page shows

```
TRAP RUNTIME RuntimeError: divide by zero  -- the game loop is dead
```

Symbolised against `legoland_dbg.wasm`'s name section (`ninja -C
portable/build-wasm legoland_browser_named`; the function indices in the page's
stack are the same in both links), innermost first:

```
  1411  Render3DPerson          <- the trap, at module offset 0xd30d7
  1694  RenderBlokeIn3D
  1059  SpiderRide_Interact
  1190  DrawAndClearPrintList
   597  GameFrame
    57  main
```

### 4.2 The division, and why its divisor is zero

There is exactly one integer division on that path — `Draw3DPersonModel`'s,
inlined into `Render3DPerson`:

```c
    lo = g_xverts[2];  hi = g_xverts[2];
    for (i = 0; i < nverts; i++) { t = g_xverts[i*3+2]; if (t<lo) lo=t; if (t>hi) hi=t; }
    zscale = 0x40000000 / ((hi - lo) >> 5);          /* person3d.c:1351 */
```

`M19.postMortem()` reads the wreck (the loop is dead, the heap is intact):

| | |
| --- | --- |
| `g_xverts` vertices 0..63 | **every one (0, 0, 0)** |
| z extent over them | `lo = hi = 0`, `(hi - lo) >> 5 = 0` |
| `g_xverts` vertices 64..78 | non-zero — a previous, larger model's leftovers |
| blokes alive | 11 |
| blokes with an ALL-ZERO `p->matrix` (+0x58) | **exactly 1** |
| that bloke's flags (+0x62) | **`0xab`** — bit `0x80` set, i.e. the rider `SpiderRide_Interact` draws |
| every other bloke's matrix | a proper 16.16 rotation (`±65536` entries) |

`SetPersonRotation` (math3d.c:402) writes `m[4] = 0x10000` unconditionally and
then negates it, so an all-zero matrix cannot come out of it: it was **never
called for that person**. A zero matrix maps every vertex to the origin, the z
extent is 0, and the shipped x86 build would fault on the same `idiv`.

That person's `rot` is (0,0,0) — and a rotation of 0 would still give
`[65536,0,0, 0,-65536,0, 0,0,65536]`, not zeros — and its `ydepth` (+0x38) is a
quiet NaN, `0x7fc00000`.

`Add3DBlokeToList` (blokeai.c:262) calls `UpdatePersonPos` — hence
`SetPersonDirection` → `SetPersonRotation` — on every `Person3D` the moment it
is created, so this record either never went through that path or was written
over afterwards. **This lane did not close that, and it is the next step for
whoever takes M19-1.**

### 4.3 Two struct-name disagreements on that path (M19-3)

Both are the same offsets in both builds, so neither is a port divergence; they
are recorded because M19-1 runs straight through the second.

* `Anim3D` +0x04 is `AnimOwner* owner` in blokeanim.c and `Frame3D* frames` in
  person3d.c. +0x00 is the frame count in both, which is what
  `BlokeSetFrame`/`BlokeAnimNextFrame` reduce modulo, so nothing is currently
  wrong — but one of the two readings of +0x04 is.
* `Person3D` +0x3c is `float zboost` in person3d.c ("extra depth gain when f2c
  is set") and `float depth` in mechrides.c. `SpiderRide_Activate` writes
  `b->person->depth = GetUnitDepth(...)` there, and `Draw3DPersonModel` then
  reads it as `zboost` and multiplies by it whenever `p->f2c` is set — which is
  the same `+0x2c` mechrides calls `zsprite` and sets on every rider. Measured
  on the offending rider: +0x2c is a sprite pointer, +0x30 is 1, +0x3c is
  160.0f. So the ride's depth IS the renderer's z gain, deliberately or not.

---

## 5. Gates

| gate | result |
| --- | --- |
| `LEGOLAND/*.c` touched | **none** — so `audit.py` / `relocs.py` rows are unchanged by construction |
| `tools/progress.py --check` | 665/675 exports exact (98.5%); **3281 exact / 42 WIP** |
| `portable/tools/addr_sweep.py` | baseline, unchanged |
| `portable/tools/bvstruct_sweep.py` | 0 unaccepted silent sites (1 silent, 5 noisy) |
| `tools/port_m10_bvstruct_sweep.py` | 0 silent, 0 slot-vs-body |
| `portable/tools/name_trap.py` | `legoland_headless_debug` linked, **0 mismatched signatures** |
| `portable/tools/extern_sweep.py` | **0** multi-address extern statements |
| native, clean `rm -rf` | builds; **ctest 19/19** |
| wasm, clean `rm -rf` | builds; **ctest 26/26** |
| session | 0 traps up to the Spider Ride; 35.6–35.7 fps; the one trap is M19-1 |

## 6. Owed

* **M19-1** — the rider whose `Person3D` never gets a rotation matrix. This
  lane located the division, proved the divisor is zero and identified the one
  bloke responsible; it did not find which path creates that person without
  calling `SetPersonRotation`. Owner: the person3d / ride lane (P1-6's family).
* `portable/build-wasm/replays` is a symlink this lane made by hand; the build
  does not create it, and every `p4-*`/`m19-*` replay `fetch`es through it.
