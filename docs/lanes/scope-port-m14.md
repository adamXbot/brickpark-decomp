# PORT-M14 — the eraser over a ride's pad

> **Status: IN PROGRESS (claimed 2026-09-12 by PORT-M14)**
>
> Branch `scope/PORT-M14`, cut at the PORT-M13 merge (`763a532d`).

The report, from PORT-M13 §5b: **an ERASER click on a plain path square of a
ride's own pad removes the RIDE**, observed twice. M13 did not chase it and
recommended a lane for it, with a named suspect — `BasicObjectDCalcCursor`
(`objmap2.c:506`, `0x00480bb0`) hands the destroy cursor `g_sel_def->rect`,
"the last selected class's footprint — which, right after building a Space
Tower, is 7x6".

## Verdict

**The game's own rule, correctly ported. Not a port defect, and the named
suspect is not guilty.**

A ride's "pad" is not one thing. It is two, and they belong to two different
owners:

* the **middle** of the pad is the ride's own footprint, painted with path
  graphics. Those cells belong to the ride. An eraser click there removes the
  ride — and the destroy cursor draws the ride's whole 7x6 outline before you
  click, which is the game telling you so.
* the **one-square border** around it is genuine path, owned by nothing, with
  a real routing square under it. An eraser click there does nothing at all:
  the cursor is red.

Nothing picks the wrong object, nothing picks the wrong rect, `g_sel_def` is
never stale on this path, and the shim's hit test is per-pixel and correctly
shaped. Every function in the decision chain is a byte-exact match, so the
recovered C *is* the disassembly for this question.

No source fix is owed. The deliverable is §6's plain-language note.

---

## 1. Where the pad comes from — `RefreshObjList`, and why it makes two kinds of cell

`workorder2.c:1175`, `RefreshObjList` (`0x0045d770`), is what paves a ride
when it is placed. It is a **100% match: 201 instructions / 743 bytes,
mismatch 0**. Its two loops are the whole answer:

```c
    /* loop 1 -- the accumulated DISJOINT set: the clearance cursor's rect
     * (the footprint EXPANDED BY ONE) minus every other cursor's rect, so
     * what is left is the one-cell RING. */
    for (p.y = g_obj_rects[i].top; p.y <= g_obj_rects[i].bottom; p.y++) {
        for (p.x = g_obj_rects[i].left; p.x <= g_obj_rects[i].right; p.x++) {
            ClearCellForPath(&p);                  /* evict whatever owned it */
            g_map_rows[p.y][p.x].rf &= ~3;
            AddPathTileGFX(&p, *(unsigned short*)g_path_tile_ptr);
            g_map_dirty |= 0x10;
            AddPathSquare(&p);                     /* <-- A REAL PATH SQUARE */
        }
    }
    ...
    /* loop 2 -- the INTERIOR of the selected rect, shrunk by one, which is
     * exactly the object's own footprint. */
    for (p.y = r->top + o->y + 1; p.y <= o->y + r->bottom - 1; p.y++)
        for (p.x = r->left + o->x + 1; p.x <= r->right + sel->origin.x - 1; p.x++)
            AddPathTileGFX(&p, *(unsigned short*)g_path_tile_ptr);
                                                   /* <-- GRAPHICS ONLY */
```

The ring gets `ClearCellForPath` (which evicts the previous owner),
`AddPathTileGFX` **and `AddPathSquare`**. The interior gets `AddPathTileGFX`
and nothing else — no eviction, no path square. The ride keeps its cells; they
just stop looking like a ride.

This is the same shape `docs/runtime/world.md` records from the other side
("`RefreshObjList` accumulates a DISJOINT rectangle set … and re-lays path on
whatever is left, then re-tiles the interior of the selected footprint") and
the same one M13 §5a ran into from the LINK side ("the interior of the pad
gets path GRAPHICS and **no path square**").

## 2. The measurement — the pad, cell by cell

Replay: B10's route to the park (profile `adam`, TUTORIAL Lesson 1), then the
Space Tower built with `llClick(330,200)`, money 1030 → 990. `llLink()`:
base **(63,35)**, footprint rect **(-2,-2,4,3)** → cells x 61..67, y 33..38 —
**7 wide × 6 tall, 42 cells**. The clearance ring is x 60..68, y 32..39.

`llPad('SPACE TOWER RIDE')`, one token per cell as
`x{where | flags | owner | T=path-tile F=footprint S=path-square}`:

```
32: 60{r|0x10|-|T-S} 61{r|0x10|-|T-S} … 68{r|0x10|-|T-S}
33: 60{r|0x10|-|T-S} 61{i|0x90|Space Tower Ride|TF-} … 67{i|0x90|Space Tower Ride|TF-} 68{r|0x10|-|T-S}
34: 60{r|0x10|-|T-S} 61{i|0x90|Space Tower Ride|TF-} … 67{i|0x90|Space Tower Ride|TF-} 68{r|0x10|-|T-S}
35: 60{r|0x10|-|T-S} 61{i|0x90|…} 63{i|0x190|…} … 67{i|0x90|…}        68{r|0x10|-|T-S}
36..38: as 34
39: 60{r|0x10|-|T-S} … 68{r|0x10|-|T-S}
```

| | flags | owner | path tile | footprint bit | PathSquare |
| --- | --- | --- | --- | --- | --- |
| pad **interior** (61..67 × 33..38) | `0x90` | **Space Tower Ride**, base (63,35) | yes | **yes** | **no** |
| pad **ring** (x 60/68, y 32/39) | `0x10` | **none** | yes | no | **yes** |
| a hand-laid **main path** square | `0x18` | **Path**, base = *itself* | yes | no | yes |

`0x90` is `0x80` (placed footprint) | `0x10` (path tile). `0x18` is `0x08`
(basic object) | `0x10` — `pathbuild.c` adds a basic object *and* a path
square, so every path square you lay is its own one-cell object. The whole-map
census makes that explicit:

```
Path:              108 cells, 108 distinct bases   <- one object per square
Park Entrance:     120 cells,   1 distinct base, every cell flags 0xc0
Space Tower Ride:   42 cells,   1 distinct base    <- exactly 7x6
```

## 3. What `HandleMapClick` does with each — and it is byte-exact

`gameframe.c:955`, `HandleMapClick` (`0x00457a70`), **100% match: 751
instructions / 2889 bytes, mismatch 0**. It classifies the cell under the
pointer on two flag masks:

```c
if (cell->flags & 0x888) {              /* something claims this cell */
    ...
    if (cell->flags & 0x88) {           /* an object, not a work order */
        g_hit_info.type = 0x103;
    }
    g_sel_def  = ((MapObj*)g_hit_info.obj)->def;
    g_sel_bpos.w = g_hit_info.cell.sq.w;      /* the owner's BASE cell */
} else { ... g_hit_info.type = 0x109 / 0x10d ... }   /* a bare tile */
```

Run the three flag values through it:

* interior `0x90` → `& 0x888` = `0x80` claims it, `& 0x88` = `0x80` promotes
  it → **type 0x103, `g_sel_def` = the ride, `g_sel_bpos` = (63,35)**.
* main path `0x18` → `& 0x888` = `0x08`, `& 0x88` = `0x08` → **type 0x103,
  `g_sel_def` = Path, `g_sel_bpos` = that very cell**.
* ring `0x10` → `& 0x888` = **0** → the bare-tile arm → **type 0x109**,
  `g_sel_def` left at the 0 it was zeroed to at the top of the call, and the
  cursor explicitly failed: `memset(&rect,0,…); SetCursorError(&cursor, 1)`.

Then, and only for the three object types, the class's **+0x94** slot draws
the cursor. `sweep3.c:128` records what `SetStandardCallbacks`
(`0x00480cd0`, 7i/55B, mismatch 0) installs there:
`BasicObjectDCalcCursor  /* 0x00480bb0  +0x94 */` — and `ObjDef +0x94` is
`gameframe.c`'s `update2`. So `d->update2(d->inst, &pos)` **is** the named
suspect, and it does this (`0x00480bb0`, **90i/281B, mismatch 0**):

```c
    g_destroy_cursor.rect = g_sel_def->rect;
```

### The suspect is innocent, for two independent reasons

1. **`g_sel_def` cannot be stale here.** It is zeroed at the top of the same
   `HandleMapClick` call and reassigned from `cell->obj->def` — the owner of
   the cell under the pointer — about forty lines before `update2` is called.
   `d` *is* `g_sel_def`. The footprint it hands the cursor is the footprint of
   the object you are pointing at, by construction.
2. **Off an object cell the hook never runs at all.** M13's reading was that
   `BasicObjectDCalcCursor` "off an object cell leaves the destroy cursor on
   the pointed square but gives it `g_sel_def->rect`". It cannot: a cell that
   is not an object cell takes the `else` arm, which never reaches
   `update2`. Measured over the ring: rect `(0,0,0,0)`, status `-1`, error 1.

## 4. The four hovers, measured live

Aimed cell by cell through the game's own `g_input.map` readout (the
projection is isometric, so the pixel is solved for, not computed):

| pointed at | cell flags | owner | hit type | `g_sel_def` | cursor origin | cursor rect | outline pts | valid |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| **(64,35)** pad interior | `0x90` | Space Tower Ride | `0x103` | Space Tower Ride | **(63,35)** | **(-2,-2,4,3)** = 7x6 | **26** | **yes** |
| **(52,31)** main path | `0x18` | Path | `0x103` | Path | (52,31) | (0,0,0,0) = 1x1 | 4 | yes |
| **(60,35)** pad ring | `0x10` | — | `0x109` | *null* | (60,35) | (0,0,0,0) | 4 | **no, error 1** |
| **(55,35)** bare grass | `0x00` | — | `0x109` | *null* | (55,35) | (0,0,0,0) | 4 | **no, error 1** |

The cursor over the pad interior is a **26-point, 7x6 outline around the whole
ride** — visible in the frame as the white diamond around the pad. The game
shows you it is about to take the ride. Over a path square it is a 4-point,
one-square outline.

## 5. The three clicks

| click | before | after | ride | money |
| --- | --- | --- | --- | --- |
| **ring** (60,35) | `0x10`, no owner, PathSquare | `0x10`, no owner, PathSquare — **unchanged** | 1 → 1 | 1090 → 1090 |
| **main path** (52,31) | `0x18`, owner Path, PathSquare | `0x00`, PathSquare gone; **(51,31) and (53,31) still `0x18` Path** | 1 → 1 | 1090 → 1090 |
| **pad interior** (64,35) | `0x90`, owner Space Tower Ride | `0x00`; the ride's 42 cells all cleared | **1 → 0** | 1090 → **1130** (+40 refund) |

So, as the brief asks it to be proved: **erasing a path square erases that
square and only that square** (its two neighbours were re-read afterwards and
are untouched), and **erasing the ride requires clicking the ride** — which
the pad's middle *is*.

### One sub-finding, also not a defect

Cell **(65,32)** is a ring cell (flags `0x10`, no owner, has a PathSquare) and
yet hovering it reports hit type `0x103` with `g_sel_def` = the ride. That is
the `else if (g_hit_info.type == 0x103)` arm taking the hit type that came
*in* from the renderer's per-pixel sprite test: `g_hit_info` is
`printlist.c`'s `g_hit_ctx` (**both are 0x004bdd00 — one object under two
names**), which the blitter fills when the pointer is over the pixels it
actually drew (`g_blit_hit`, `rlepaint.c`). The Space Tower is a tall object;
its sprite is painted over the ground cells behind it. Pointing at the tower's
pixels selects the tower even where the ground under those pixels is path.
That is correct isometric behaviour, not a bounding-box leak — a screen-space
sweep of the hit test (10 px grid, 23 × 17 probes) draws a **footprint-shaped
rhombus plus a narrow stalk for the tower**, with the main path winning where
*it* is drawn in front:

```
py    x=250 ---------------------- x=470
130   .....oooRRR............
140   ......RoooR............
150   ......RRRooo...........
160   ......RRRRRooo.RR......
170   ......RRRRR..oooR......
180   ....RRRRRRR...Rooo.....
190   ..RRRRRRRRRRRRR.Rooo...
200   RRRRRRRRRRRRRRRRRR.ooo.
210   RRRRRRRRRRRRRRRRRR...oo
220   .RRRRRRRRRRRRRRRRR.....
230   ..RRRRRRRRRRRRRRR......
240   ....RRRRRRRRRRR........
250   ......RRRRRR...........
260   ........RR.............
      R = hit claims the Space Tower   o = hit claims another object
```

A bounding-box hit test would have filled the enclosing rectangle. It does
not.

## 6. For the user: why the eraser ate your ride

> **Because the middle of that grey pad *is* the ride.**
>
> When you build a ride the game lays its pad in two parts, and they only look
> like one thing:
>
> * The **middle** of the pad is the ride itself, with a path pattern painted
>   on top of it. It is no more a path than the ride's roof is — it is the
>   ride's own ground, drawn grey. Erasing any square of it erases the whole
>   ride (and refunds you what it cost).
> * The **single row of squares around the edge** is real path. That is the
>   path the game means when it says "we also built a path from the ride to
>   the park entrance".
>
> **How to tell them apart before you click.** Hover the eraser and watch the
> outline:
>
> * a **small outline on one square** — that is a path square, and only that
>   square will go;
> * a **big outline around the whole ride** — you are on the ride, and the
>   whole ride will go;
> * a **red cursor, nothing highlighted** — the eraser has nothing to take
>   there (the pad's border path behaves this way; so does bare grass).
>
> The big outline is the warning, and it appears before the click. If you see
> the whole ride light up and you only wanted a path square, move one square
> further out, onto the pad's edge or beyond.
>
> Two related things that are easy to trip over:
>
> * The ride's **tall parts** count as the ride wherever they are *drawn*, not
>   just where they stand. Point at the tower's body and you are pointing at
>   the tower, even though the grass or path behind it is what is on the
>   ground there.
> * **Dragging** the eraser is a rectangle selection and takes everything the
>   rectangle covers. That one really will clear a ride and its surroundings
>   together, by design.

## 7. Gate table

Nothing under `LEGOLAND/` was touched, so the VC6 view is unchanged by
construction; the audits below are the *evidence* for the verdict, not a
regression check.

| gate | result |
| --- | --- |
| `audit.py LEGOLAND/gameframe.c` | `[OK] 0x00457a70 HandleMapClick 751i/2889B mismatch=0`; PASS, 0 REJECT |
| `audit.py LEGOLAND/objmap2.c` | `[OK] 0x00480bb0 BasicObjectDCalcCursor 90i/281B mismatch=0`; `[OK] 0x0045f220 StandardRemoveObject 169i/544B mismatch=0`; `[OK] 0x0045dee0 SetObjRectFlags 116i/410B mismatch=0`; `[WIP] 0x0045f810 ValidateCursor 205i/617B mismatch=5` (the known store-schedule reorder, same instruction count and byte length); PASS |
| `audit.py LEGOLAND/workorder2.c` | `[OK] 0x0045d770 RefreshObjList 201i/743B mismatch=0`; PASS |
| `audit.py LEGOLAND/sweep3.c` | `[OK] 0x00480cd0 SetStandardCallbacks 7i/55B mismatch=0`; PASS |
| `audit.py LEGOLAND/mapinit.c` | `[OK] 0x00481c50 AddPathSquare 19i/51B mismatch=0`; PASS |
| `audit.py LEGOLAND/mappath.c` | `[OK] 0x004779d0 ClearCellForPath 167i/497B mismatch=0`; PASS |
| `relocs.py --all` | **0 MISMATCH**; 254 files, 3281 functions, 28290 relocations, 26663 matched, `"mismatches": 0` |
| `progress.py --check` | **3281 exact / 42 WIP** (665/675 exports exact, 98.5%) — unchanged |
| `portable/tools/extern_sweep.py` | 0 multi-address extern statements — the class is closed |
| `portable/tools/bvstruct_sweep.py` | 0 unaccepted silent sites (1 silent, 5 noisy — the accepted pre-existing rows) |
| `tools/port_m10_bvstruct_sweep.py` | SLOT vs BODY: 0 sites |
| wasm build | configure + `ninja` + all seven named targets link; **ctest 24/24** |
| native build | configure + `ninja` + `legoland_tests` `legoland_cbtypes`; **ctest 17/17** |

## 8. Files touched

| file | what |
| --- | --- |
| `docs/SCOPE_PORT_WAVE.md` | the PORT-M14 status block |
| `portable/src/browser/main.c` | eight globals added to `LL_DBG_TABLE`: `g_hit_info`, `g_sel_def`, `g_sel_bpos`, `g_destroy_cursor`, `g_drag_class`, `g_query_extra`, `g_edit_cursor`, `g_tile_info` |
| `portable/src/browser/index.html` | `llSel()`, `llCellAt(x,y)`, `llPad(name)` and their entries in the header's hook list |
| `docs/lanes/scope-port-m14.md` | this file |

**No `LEGOLAND/*.c` was edited.**

## 9. For the integrator

* **PARK/M13-5b closes as NOT A DEFECT.** The line for the wave doc is in the
  report below; the player-facing explanation is §6.
* M13 §5b's suspicion of `BasicObjectDCalcCursor` should be recorded as
  **cleared**, with the reason: `g_sel_def` is reassigned from the pointed
  cell's owner in the same call, and off an object cell the hook is not
  reached at all.
* Still open from M13 and untouched here: §5a's four shipped classes whose
  entrance offset is off their clearance ring (original data, not a port
  question).
* Two small things seen in passing, neither chased, neither a defect: the
  **Park Entrance** is a single 120-cell object whose every cell carries
  `0x40` (build blocked), so the destroy cursor is always invalid over it —
  the entrance cannot be erased; and a removed object leaves a **stale
  `cell->obj` pointer** behind with its flags cleared (visible above as
  "42 cells, flags `0x0`, owner Space Tower Ride" after the removal), which is
  `world.md`'s already-recorded "several removal tails dereference unchecked
  cells" family and matches the original.
