# Scope PORT-P2 — PLAY tutorial lessons 2 and 3

> **PORT-P2 — Status: IN PROGRESS (claimed 2026-09-12 by PORT-P2)** — branch
> `scope/PORT-P2`, cut from the PORT-M14 merge (`cc6e926d`). A TESTING lane:
> nothing in the game or the shim is fixed here. Every defect is recorded with
> a replayable script, a hash, and an owner.
> Brief: `docs/SCOPE_PORT_WAVE.md`. Files: this note and
> `portable/src/browser/replays/p2-*.js`.

---

## 1. The scripts, decoded before playing

Both lessons are plain text inside `gamedata/disc/Legoland.res`, recovered with
`tools/leveldata.py`'s `res_index()`. Lesson 2 is `ObjList2.txt` on `TWO.MAP`,
lesson 3 is `ObjList3.txt` on `THREE.MAP`.

### Lesson 2 — "Money and planting things" (`ObjList2.txt`, map `TWO`)

`[INIT]`: `CURRENCY 1000`, `ENTRANCEFEE 10`, `GARDENER(48,5) 4`,
`LOOKAT 48,15`, `WORKERS 1,0`, `FEATURE RideWear 0`, `MAXBLOKES 20`,
`THEMEICON 0,1` (LEGOLAND only). `LOAD` FLOWERS / TREE 1 / HEDGE /
FOUNTAIN 1 / LEGO MEDIA SHOP, `ENABLE "LEGO SHOP 1"`.

`TWO.MAP` ships 116 objects: **113 HEDGE**, `ENTRANCE 1` at (51,28),
`LEGO MEDIA SHOP` at (36,23), `LEGO SHOP 1` at (40,36). The hedges form a
closed 8x10 pen at x 46..53, y 1..10 — the four gardeners are spawned at
(48,5), inside it.

| # | keyword(s) | what the player must do | new mechanism |
| --- | --- | --- | --- |
| 1 | `SELECTTHEME LEGOLAND` | pick the gardeners up out of the hedge pen, drop them outside, then click the flashing LEGOLAND button | **worker pick-up / put-down**, `GLUE`/`UNGLUE` regions, `FLASHBUTTON` |
| 2 | `NEED "FLOWERS" 1..5` (five objectives) | plant five flowers | **planting = a gardener work order**, not an instant build |
| 3 | `NEED "TREE 1" 2,4,6,8,10` | plant ten pine trees, watching the money bar | **money spent per plant**; reward `INTERVAL "money.txt"` |
| 4 | `SELECTMODE ERASE` + `REMOVE "LEGO MEDIA SHOP",0` + `REMOVE "LEGO SHOP 1",0` | erase both shops, watch money rise | **`REMOVE` goal, refunds, `UNGLUE`** |
| 5 | `NEEDIN "FOUNTAIN 1" 1, 36,31, 43,41` | build the fountain in the old Toy Shop rect | **`NEEDIN` — a goal with a rectangle** |
| — | `ENDLEVEL` | click End Level | **the end-level button and `ENDSCREENS`** |

### Lesson 3 — "Power and special rides" (`ObjList3.txt`, map `THREE`)

`[INIT]`: `CURRENCY 1200`, `ENTRANCEFEE 10`, `LOOKAT 39,10`, `WORKERS 1,0`,
`MAXBLOKES 20`, `FEATURE RIDEWEAR 0`, `FEATURE HUNGER 0`, **`FEATURE ENERGY 0`**
(power is turned ON mid-level by a reward), all four `THEMEICON`s 0.
`ENABLE` FLOWERS / TREE 1 / FOUNTAIN 1; `LOAD` SPACE TOWER RIDE /
SMALL POWER STATION / LEGO SHOP 1 / LEGO SHOP 2 / SPIDER RIDE.

`THREE.MAP` ships two objects only: `ENTRANCE 1` at (57,28) and
**`POTTING SHED` at (40,5)** — the "Greenhouse" the script tells the player to
click.

| # | keyword(s) | what the player must do | new mechanism |
| --- | --- | --- | --- |
| 1 | `NEEDGARDENERS 1` | click the Potting Shed once | **`NEEDGARDENERS`, worker creation from a building** |
| 2 | `NEEDGARDENERS 2..6` (five objectives) | click it five more times | worker cost / cap |
| 3 | `PATHSCENERY 25` + `NEED "FOUNTAIN 1",1` + `NEED "TREE 1",1` + `NEED "FLOWERS",1` | scenery beside 25% of the path, one of each | **`PATHSCENERY` — a percentage goal over the path network** |
| 4 | `NEED "SPACE TOWER RIDE",1` | build the Space Tower | reward `GIVE ... NOPOPUP`, then **`FEATURE ENERGY 1`** |
| 5 | `SELECTTHEME LEGOLAND` then `NEED "SMALL POWER STATION",1`, then a `[PERMANENT]` repeat of it | build a power station; unpowered rides flash blue/green | **power: supply, demand, the flash state** |
| 6 | `NEED "LEGO SHOP 1",1` + `NEED "LEGO SHOP 2",1` | two shops | reward **`FMV "Spider.avi"`** |
| 7 | `NEED "SPIDER RIDE",1` | build the Spider Ride | a "special" ride; reward `FLASHBUTTON MAP` |
| 8 | `NEED "SMALL POWER STATION",2` | use the **MAP button** to pick a spot, build a second station | **the MAP screen as a placement tool** (A7-2's old trap site) |
| — | `ENDLEVEL` | click End Level | |

### What these two lessons do NOT exercise

Neither script contains `LINK` or `LINK ALL`, so PORT-M13 §5a's four off-ring
entrance classes (`WATER WORKS SHOWER`, `dschool.odf`,
`WW ELEPHANT FOUNTAIN`, `WW WATER BLOCK`) cannot be reached from here: the nine
`LINK ALL` scripts are `ObjList6,7,8,9,10,11,12,14,15.txt`, all main-game
levels. Recorded as out of scope for this lane rather than tested.

---

## 2. Replay index

(filled in as each walk lands — `portable/src/browser/replays/p2-*.js`)

## 3. Findings

(filled in as each is measured)
