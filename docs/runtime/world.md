# World, visitors, construction and staff

This page consolidates the recovered contracts at the Scope J baseline. `partial` means that behaviour outside the described functions, external table values, or an explicitly named field remains unknown; it does not mean a compiler match failed. All offsets describe the original 32-bit records.

## Map and object placement

### Data structures

| Record | Layout and ownership | Evidence |
| --- | --- | --- |
| Map/config | `lpConfig` at `0x004bcbf4`, saved size `0x44`: screen dimensions `u16 +0/+2`, scrolling margins `+4/+6`, acceleration `+8/+a`, caps `+c/+e`, viewport dimensions `+10/+12`; map width/height `u16 +14/+16`, visitor capacity `+1a`, viewport origin `+20/+22`, mechanic/gardener service switches `+34/+38` | [pathtile2.c](../../LEGOLAND/pathtile2.c), [savegame.c](../../LEGOLAND/savegame.c), [workers2.c](../../LEGOLAND/workers2.c) |
| Cell | `0x14` bytes: object/element `+00`, packed anchor x/y `+04/+05`, packed render-next coordinate `+06`, displayed tile `u16 +08`, base terrain tile `u16 +0a`, map flags `u16 +0c`, user flags `u16 +0e`, RF byte `+10`, life byte `+11`, door bits `u16 +12` | [objmap2.c](../../LEGOLAND/objmap2.c), [objrect.c](../../LEGOLAND/objrect.c), [map.c](../../LEGOLAND/map.c) |
| Grid | `GameMap`/`g_map_rows` at `0x00801400`; one allocation of `0x14041f` bytes, a 32-byte-aligned 256-pointer row table followed by 256 rows of 256 cells, row stride `0x1400`; allocation retained at `0x00667c9c` | [pathtile2.c](../../LEGOLAND/pathtile2.c) |
| ObjDef | `0xd0` bytes: entrance `int x/y +0c/+10`, flags `+1c`, class type `short +20`, signed exit bytes `+24/+25`, cost `short +26`, notice radius `+2a`, maximum life byte `+2c`, staff `short +2e`, mood value `+36`, footprint Rect `+3c`, LLIDB element `+c4`, visit counters `+c8`, rider list `+cc` | [objdoor.c](../../LEGOLAND/objdoor.c), [objrect.c](../../LEGOLAND/objrect.c), [bigsim.c](../../LEGOLAND/bigsim.c), [llidb_odf.c](../../LEGOLAND/llidb_odf.c) |
| Rect / edit cursor | Rect is 20 bytes `{left,top,right,bottom,next}`, inclusive edges; a plain Rect4 omits the link. Cursor blocks are `0x1834` bytes, origin `+1404`, status/error `+140c/+1410`, rect run `+1414`, next `+1830` | [objdoor.c](../../LEGOLAND/objdoor.c), [objmap2.c](../../LEGOLAND/objmap2.c), [workorder4.c](../../LEGOLAND/workorder4.c) |
| Terrain overlay | `0x24` bytes; saved first `0x14` bytes describe its placement, next at `+1c`; list head `OverlayList = 0x00667ca8`. Image low byte selects terrain frame, next byte selects bridge/switch | [simcore.c](../../LEGOLAND/simcore.c), [pathgfx.c](../../LEGOLAND/pathgfx.c), [savegame.c](../../LEGOLAND/savegame.c) |

### Rules and state machines

Load resources and LLIDB, allocate the grid with `LoadMapTiles`, then call `LoadBaseMap(name)`. It returns 1 on success and a negative result for a missing map element, fills an existing grid, creates perimeter overlays and binds their sprites. `InitGameMap` only resolves `CASTLE OBJ` and the 23-entry effects table; it does not allocate map cells. `RestoreBaseMap` restores one cell's displayed tile from its base tile; `ClearOverlays` frees the terrain chain. [loadmap.c](../../LEGOLAND/loadmap.c), [mapinit.c](../../LEGOLAND/mapinit.c), [maprestore.c](../../LEGOLAND/maprestore.c), [DECOMP.md](../DECOMP.md#loadbasemap-interface-for-host--wasm-integration)

Placement resets eight build-stat accumulators, stamps object footprints and updates category/area counts. `ENTRANCE 1` additionally sets the screen entrance origin. A class's entrance exists only for types other than 0, 2, 3, with an offset outside its footprint. A usable visitor door requires type 1, 4 or 5; otherwise use `(right+1, (top+bottom)/2)`. Park entrance caching uses a nonzero x as valid and targets the middle of the **left** edge, one tile clear. Door flags coexist for neighbouring objects in Cell `+12`; `ClearObjectUserFlags` actually clears these bits, not `+0e`. [mapbuild.c](../../LEGOLAND/mapbuild.c), [mapobj.c](../../LEGOLAND/mapobj.c), [objdoor.c](../../LEGOLAND/objdoor.c), [objrect.c](../../LEGOLAND/objrect.c)

Cursor validation scans each rect and retains the worst error, represented as the most negative status. `PropagateCursorStatus` sets flag 8 on every link and propagates the worst invalid link's error through the chain. Unless a link has flag `0x1000`, `CheckCursorFootprint` appends the shared spare cursor at `0x00830fc0` with a one-cell clearance ring and mode `0x1008`. Drag flag `0x800` locks to the larger axis; build drags round outward to complete footprints, while edit mode 2 publishes the raw selection and resets the footprint. [objmap2.c](../../LEGOLAND/objmap2.c), [objdoor.c](../../LEGOLAND/objdoor.c), [sysmisc3.c](../../LEGOLAND/sysmisc3.c), [simcore.c](../../LEGOLAND/simcore.c)

The render-order scan runs by column, top to bottom, using 4,096 eight-byte pending nodes at `0x00807f60`. A footprint is linked when the scan reaches its right edge, then scanning resumes at its saved column/bottom+1. The chain head is `0x007febb8`. `GetObjectUID` searches above, below, left, right for a matching class whose base plus entrance offset reaches the query tile; it never probes the query cell itself. [objmap2.c](../../LEGOLAND/objmap2.c), [objmap.c](../../LEGOLAND/objmap.c)

`RefreshObjectAtPos` saves/restores the global cursor, requests the class's placement effect `0x8f8`, and re-stamps eligible cells without preserving prior flags, changing life, or setting footprint/tall bits. Rect subtraction swap-erases overlaps and appends up to four disjoint remnants. Path removal from an object sweeps a one-cell-expanded footprint and then its interior again, because border updates can re-tile interior cells. [objrect.c](../../LEGOLAND/objrect.c), [mappath.c](../../LEGOLAND/mappath.c)

### Flags and constants

| Cell flag | Meaning | Cell flag | Meaning |
| --- | --- | --- | --- |
| `0x08` | basic object | `0x10` | path tile |
| `0x20` | construction/render base reservation | `0x40` | build blocked |
| `0x80` | placed footprint | `0x100` | no power |
| `0x200` | switched off/disconnected | `0x800` | worker/no-build reservation |
| `0x1000` | worker map stamp | `0x4000` | outstanding repair |
| `0x8000` | tall object, from class `0x800000` | `0x3000` | excluded during re-stamping |

The meanings above are operation-specific and can overlap; do not turn every bit into one universal enum without checking its consumer. Class `0x200000` permits building over an object and selects gardener maintenance; `0x400000` selects mechanic maintenance. [objmap2.c](../../LEGOLAND/objmap2.c), [objrect.c](../../LEGOLAND/objrect.c), [power.c](../../LEGOLAND/power.c), [workorder4.c](../../LEGOLAND/workorder4.c)

Validation errors are 1 blocked; 3/4 people result +1/-1; 5 `0x800`; 6 conflicting object; 7 off-map; 8 forbidden path; 9 required but unwalkable path; 10 underlying non-environment object without `0x200000`. Entrance direction bits for right/left/bottom/top are `4,8,1,2`; exit bits are `0x80,0x40,0x20,0x10`. [objmap2.c](../../LEGOLAND/objmap2.c), [objdoor.c](../../LEGOLAND/objdoor.c)

### Original bugs and limits

- `GetObjectUID` only probes below when the above probe was nonnull and does not check the below pointer. `ValidateCursor` uses map **height** for both right and bottom bounds. `RemObjFromMap` sends an uninitialized position to `UnmarkObjectTiles` for class types 1, 4, 5. The object exit path tests the enter callback before calling the leave callback. [objmap2.c](../../LEGOLAND/objmap2.c)
- New object-class creation zeroes a failed allocation. `ClearCellForPath`, `RemoveObjectPathTiles`, route-cell creation and several removal tails dereference unchecked cells; the build-walk-path allocator writes through a failed allocation. [objmap.c](../../LEGOLAND/objmap.c), [mappath.c](../../LEGOLAND/mappath.c), [objrect.c](../../LEGOLAND/objrect.c)
- Door direction helpers default to the top side when x is inside and y is not below, even if the offset is inside the rect. A park entrance at x=0 is recomputed on every lookup. [objdoor.c](../../LEGOLAND/objdoor.c)

### Callback contract

Standard add/remove use ObjDef `+98/+9c`; cursor calculation and placement effects also use class-specific slots, including `+90`. Basic path placement checks bounds and mask `0x08e8`, adds a basic object then a path square. Coaster path placement performs no corresponding checks or object creation. Basic-path removal refunds salvage at the object's origin; coaster path removal gives no refund. See [callback matrix](callbacks.md) for class-specific overrides. [pathbuild.c](../../LEGOLAND/pathbuild.c), [maprestore.c](../../LEGOLAND/maprestore.c), [objrect.c](../../LEGOLAND/objrect.c)

## Paths and route searches

### Data structures

Path squares are maximal walkable rectangles, linked from `0x0066b44c`, with inclusive Rect at `+08` and visited bit at `+20`. Neighbour collection fills `0x0066a45c` in four null-terminated groups: above, below, left, right; corners are excluded and each scan jumps over a found rectangle's span. The separate count `0x00669254` excludes the four terminators. [mappath.c](../../LEGOLAND/mappath.c), [pathsq.c](../../LEGOLAND/pathsq.c)

PTP nodes are 16 bytes `{next +0,parent +4,x +8,y +c}`; open head `0x0066b450`, wave count `0x00669250`, found node `0x0066b454`, output route `0x0066b458`. The visited bitmap at `0x00669258` has **six dwords/row, 192 bits/row and 192 rows**. The earlier 256×256 claim in `bnvmove.c` conflicts with the indexed stride and adjacent-global boundary recovered in `workorder4.c`. [bnvmove.c](../../LEGOLAND/bnvmove.c), [workorder4.c](../../LEGOLAND/workorder4.c)

Auto-path nodes are `0x28` bytes: next `+0`, parent `+4`, position `+8`, entry cost `+10`, g/h/f `+14/+18/+1c`, terrain class `+20`, direction `+24`. Open/closed heads are `0x00668fc0/0x00668fc4`. Some declarations call `+20` “axis”; `GetRouteNode` uses it as terrain class, so the name alone is not authoritative. [objrect.c](../../LEGOLAND/objrect.c), [workers3.c](../../LEGOLAND/workers3.c), [simcore.c](../../LEGOLAND/simcore.c)

### Rules

Walkability is RF bit 0, or map flag `0x10` with RF bit 1 clear. Off-map cells are treated as blocked for neighbour tests. `SuggestNextMove` returns -2 for no start square, -1 for no destination square/route, 2 for a shared square and centred destination, otherwise 1 for the next rectangle's closest point, clamped into the current rectangle if squared distance exceeds `0x18000`. PTP is breadth-first in N/E/S/W order and returns 0 for failure, 1 for a leg, 2 for a short route/direct target. Worker `PTPVisitTile` permits RF-blocked cells when map `0x800` is set. [pathbuild.c](../../LEGOLAND/pathbuild.c), [bnvmove.c](../../LEGOLAND/bnvmove.c), [workorder4.c](../../LEGOLAND/workorder4.c)

PTP walk-back retains the start and next three nodes; shortcut results 0/1/2 select b/c/d. Missing b or c returns0. A permitted first diagonal returns1; after a straight first pair, missing d returns0. The third-delta test can allow2, while a fully straight three-step window returns1. A diagonal requires its intervening tile to have RF bit 1 clear **or** map `0x800` set. The third-step test compares against the first delta and samples from the start again. This corrects `workorder3.c`'s “+1d bit 3 blocks” description: the access is byte `+0d`, and the bit permits the tile. [workorder3.c](../../LEGOLAND/workorder3.c), [workorder4.c](../../LEGOLAND/workorder4.c)

Auto-path routing is best-first over f. New nodes initialize g to `INT_MAX`, h to Manhattan distance, f to entry cost+h. Equal-f new nodes precede existing nodes. Turning costs 4 unless direction masks intersect. Successful routes lay `PATH CONTROL` after clearing cells that are not already paths. [objrect.c](../../LEGOLAND/objrect.c), [workers3.c](../../LEGOLAND/workers3.c), [simcore.c](../../LEGOLAND/simcore.c), [simcore2.c](../../LEGOLAND/simcore2.c)

### Tables and constants

| Terrain | Class | Entry cost |
| --- | --- | --- |
| Path/RF walkable | 0, or 1 if its square has terminal flag 2 | 1 |
| Build blocked; RF blocked; non-overbuildable object | 5 | -1 |
| Overbuildable object, class `+2a > 1` | 4 | 20 |
| Other overbuildable object | 3 | 9 |
| Open ground | 2 | 3 |

These are entry costs, not multiples of the open-ground cost. [objrect.c](../../LEGOLAND/objrect.c)

Eight-neighbour bits clockwise are `N=1, NE=2, E=4, SE=8, S=0x10, SW=0x20, W=0x40, NW=0x80`; diagonal retention needs both flanking cardinals (`0x05,0x14,0x50,0x41`). RF shape: 0/1 cardinal neighbours→`0x10`; corner→8; T→4; cross→`0x20`; straight→0. Rebuild clears `0x3c` and preserves other bits. Heading numbers use a different ring: 0 NW, 1 N, 2 NE, 3 E, 4 SE, 5 S, 6 SW, 7 W; 8 means none. `Pos_Step2/4` accepts only odd cardinal headings. [pathtile2.c](../../LEGOLAND/pathtile2.c), [workers.c](../../LEGOLAND/workers.c), [posstep.c](../../LEGOLAND/posstep.c)

The 5×5 plaza mask assigns bit 24 to top-left and bit 0 to bottom-right. Nine candidate 3×3 placements are tried top-left first; table addresses are masks `0x004b9558`, dx `0x004b957c`, dy `0x004b95a0`. The decoded masks, in order, are `0x01ce7000, 0x00e73800, 0x00739c00, 0x000e7380, 0x000739c0, 0x00039ce0, 0x0000739c, 0x000039ce, 0x00001ce7`; dx is `-2,-1,0` repeated three times, dy is `-2,-2,-2,-1,-1,-1,0,0,0`. [Decoded lane table](../lanes/fable-c-workorder4.md) Path checkerboarding chooses base+1 for odd x+y, base+2 for even. [workorder4.c](../../LEGOLAND/workorder4.c), [sysmisc3.c](../../LEGOLAND/sysmisc3.c)

Switches 0..3 at `0x00832be0` match image selector switch+1. Frame 0 stamps a 2×10 bridge at `(x+15,y)`; other frames stamp 10×2 at `(x+10,y+6)`. Deck cells receive environment objects, walkable RF, flags `+0x48/-0x8000`, own anchors and path graphics; the switch is marked even with no overlay list. [simcore.c](../../LEGOLAND/simcore.c)

### Original bugs and limits

`AdjustTileRFFlags` has undefined returns on straight/cross branches, unused by callers. Invalid `Pos_Step2/4` headings leave output untouched. Maps exceeding 192 cells can alias/overrun PTP visited rows despite the 256-cell grid. `PTPVisitTile` does not check allocation; shortcut and bridge stamping accesses lack bounds guards. Closed auto-path nodes are unlinked without being freed; `RemoveOpenNode` assumes its target exists in a nonempty list. Some fresh route-node parent fields remain uninitialized. [pathtile2.c](../../LEGOLAND/pathtile2.c), [posstep.c](../../LEGOLAND/posstep.c), [workorder4.c](../../LEGOLAND/workorder4.c), [simcore.c](../../LEGOLAND/simcore.c), [simcore2.c](../../LEGOLAND/simcore2.c), [pathmisc.c](../../LEGOLAND/pathmisc.c)

### Callbacks

Path-square merging depends on neighbour-group order; path/RF queries feed movement and ride class callbacks. These are shared services, not a new ObjDef callback class. [mappath.c](../../LEGOLAND/mappath.c), [pathbuild.c](../../LEGOLAND/pathbuild.c)

## Visitors and staff

### Data structures

Visitors occupy a fixed pool of `0xac`-byte Blokes at `0x0066b57c`, linked from `FirstBloke=0x0066b574`; bit 0 of `+62` marks occupancy. Hired workers use `NewBlokeWOList`, a separate heap allocation of the same size, and trade lists. This corrects `workers3.c`'s over-broad “all people in one pool” prose. Person3D records are `0x94` bytes with their own prev/next list at `0x00655a3c`. [blokeai.c](../../LEGOLAND/blokeai.c), [workers.c](../../LEGOLAND/workers.c), [workers2.c](../../LEGOLAND/workers2.c), [workers3.c](../../LEGOLAND/workers3.c)

| Bloke offset | Recovered field | Evidence |
| --- | --- | --- |
| `+00/+04` | next / Person3D | [blokeai.c](../../LEGOLAND/blokeai.c) |
| `+0c/+0e/+10/+1c` | plan u16 / low-level state u16 / saved step / scratch timer | [blokelist.c](../../LEGOLAND/blokelist.c) |
| `+14`, `+24/+28`, `+2c/+30` | focused class element, move target, saved target | [simcore.c](../../LEGOLAND/simcore.c), [workers2.c](../../LEGOLAND/workers2.c) |
| `+36`, `+50/+54`, `+5c` | seat/job byte (100 discard), work order / BNV path, tick count | [workers3.c](../../LEGOLAND/workers3.c), [savegame.c](../../LEGOLAND/savegame.c) |
| `+60/+62/+64` | action byte / flags word / route scratch | [bigsim.c](../../LEGOLAND/bigsim.c) |
| `+68/+6c` | world x/y, 24.8 | [workers3.c](../../LEGOLAND/workers3.c) |
| `+72/+73/+74/+75` | heading / requested heading / AI phase / walk delay | [workers.c](../../LEGOLAND/workers.c), [simcore2.c](../../LEGOLAND/simcore2.c) |
| `+78/+7a/+7c/+80` | signed stay timer / signed mood / u16 tiredness / tiredness increment byte | [simcore2.c](../../LEGOLAND/simcore2.c), [workers3.c](../../LEGOLAND/workers3.c) |

Ride SeatSlots are 20 bytes in the class `+cc` doubly linked chain, binding a Bloke at `+08`, seat at `+0c`, and Person3D at `+10`. The `+10` Person3D interpretation follows the explicit `Bloke->person` assignment in `savegame.c`; `blokelist.c` calls it a ride owner, so that header label is retained as a disagreement. Render-list insertion takes the slot, not the Bloke. MoveLine is 12 bytes `{int x,y; short dx,dy}` with position and velocity in 1/256 tile units. [blokelist.c](../../LEGOLAND/blokelist.c), [savegame.c](../../LEGOLAND/savegame.c), [bnvmove.c](../../LEGOLAND/bnvmove.c)

### Rules

Spawn clock increments every tick; at >=30 and below the visitor cap, successful creation resets it, randomizes heading/AI phase with `rand() & 7`, sets walk delay 1 and initializes AI. Failure leaves the elapsed clock accumulating. ControlPeople checks leaving, then riders with flag `0x20` run only low-level AI; other visitors update stay bookkeeping and every 16 ticks adjust mood event 8 and scan surroundings. State 0 runs high-level AI, then a now-nonzero state runs low-level AI. New plans clear action, step, state and scratch timer before immediate dispatch; dispatch sets walk delay 1 and clears `+64`. [simcore2.c](../../LEGOLAND/simcore2.c), [bnvmove.c](../../LEGOLAND/bnvmove.c), [blokelist.c](../../LEGOLAND/blokelist.c)

Tiredness tiers are `{1000,2400,4000,7000}`. At >=7000, a visitor outside flag mask `0x28` receives plan 3 (go home). Otherwise tiredness can increase by `+80` every 32 ticks while <=7000; a rider exactly at 7000 can still increase. Disabling `0x00832990` skips this check and makes the tier helper return 1. Mood changes by `adjustment[event] * scale / 100`, then clamps to [-30000,30000]. [workers3.c](../../LEGOLAND/workers3.c), [simcore2.c](../../LEGOLAND/simcore2.c)

The 9×9 scan contributes mood channels: 2 off-map count; 3 scenery value and `-20/Manhattan distance` for empty cells except self; 4 type-3 value shifted by visit count; 5 types 4/5; 6 type 1; 7 age-2 for age>=3. Objects must be anchor/eligible cells and within their class's Chebyshev notice radius. Type 3 can attract with integer `15/(visits+1)` percent, if near and not already focused/riding/state15; types 1/4/5 require adjacent access tiles, attractiveness>50 and plan6. [simcore.c](../../LEGOLAND/simcore.c)

Movement starts at tile centres `(tile<<8)+0x80`; unit velocity is atan2-derived, scaled by 256 and rounded using floor(v*256+0.5). `InitMoveLine` returns a256-unit turn angle; Bloke heading bytes`+72/+73` instead use octants0…7, with8 meaning no heading. Random walking follows an available nonreverse exit at corners, excludes reverse and its neighbours at junctions, and wanders at dead ends. Turns save prior low-level state at `+10`, enter state5 and reset delay. Off-map is an obstacle/path edge; a person currently on RF-blocked terrain may leave it. [bnvmove.c](../../LEGOLAND/bnvmove.c), [workers.c](../../LEGOLAND/workers.c), [pathbuild.c](../../LEGOLAND/pathbuild.c)

Worker ticks run mechanics then gardeners: tick++, high AI if idle, low AI if now active, update person, remove if slot100; capture next before processing. Hire cap is 15 per trade; kind2 gardener, kind3 mechanic. Inside-hut hires use plan5; outside hires use idle `0x10/0x11`. Gardener hut placement mutates the caller's position by (-2,+1). Carried workers enter state13 with cursor screen position adjusted (-75,-77); cancel restores saved world coordinates and idle plan. `UpdatePerson` suppresses position refresh when flag `0x80` is set despite some other uses calling that bit “in 3D”. [workers.c](../../LEGOLAND/workers.c), [workers2.c](../../LEGOLAND/workers2.c), [workers3.c](../../LEGOLAND/workers3.c), [blokemisc.c](../../LEGOLAND/blokemisc.c)

### Tables and constants

The high-level table at `0x004b8368` has 26 slots: 0,4,7,8,9,a,b,c are `Bloke_DoNothing`; 5 null; 1=`0x44f170`,2=`0x44ebf0`,3=`0x44ed70`,6=`0x44f610`,d=`0x44fe80`,e=`0x450250`,f=`0x450450`; 10/11 gardener/mechanic idle,12/13 build,14=`0x450330`,15=`Garderner_Repair`,16=`Mechanics_Repair`,17=`0x44fe10`,18/19=`0x49a4a0/0x49a4d0`. Historical address-only entries are not assigned invented names here. Low-level dispatch at `0x004bd34c` refreshes path state then calls the indexed handler. [blokeai.c](../../LEGOLAND/blokeai.c), [blokemisc.c](../../LEGOLAND/blokemisc.c)

LEGO appearance palette at `0x004b7ac0`, RGB triples: 0 `#000000`,1 `#007bc6`,2 `#732910`,3 `#008c4a`,4 `#bdcede`,5 `#f71821`,6 `#ffffff`,7 `#ffd600`. Departure scores enter a 25-byte ring at `0x00832bb0`, index `0x00832bc9`, bucketed by runtime thresholds (numeric contents not decoded here). Worker refunds are 30 bricks. [blokelist.c](../../LEGOLAND/blokelist.c), [workers.c](../../LEGOLAND/workers.c), [blokemisc.c](../../LEGOLAND/blokemisc.c)

### Original bugs and unresolved behaviour

Texture getters discard their comparison and always return `chest girly2`; invalid sex leaves the lookup pointer uninitialized. Type-3 retargeting writes tile units into a normally 24.8 target. Mood-event indexing is unchecked. The low-level table contents and several high-level handlers remain external to this recovered subset. [blokeai.c](../../LEGOLAND/blokeai.c), [simcore.c](../../LEGOLAND/simcore.c), [simcore2.c](../../LEGOLAND/simcore2.c)

### Callbacks

The AI tables are distinct from ObjDef callbacks. Ride renderers enqueue seat slots between sprite layers; free people enter the print list once by Person3D depth `+54`. That print list is sorted and doubly linked, correcting earlier tree terminology. [blokelist.c](../../LEGOLAND/blokelist.c), [blokeai.c](../../LEGOLAND/blokeai.c), [workorder3.c](../../LEGOLAND/workorder3.c)

## Construction, work orders, statistics and power

### Data structures

Construction has 256 slots of 12 bytes at `0x006664f8`, live count `0x006670f8`: object, packed tile key at `+04`, timer. WorkOrder is `0x3c`: next `+0`, class **element** `+4`, tile `+8`, Rect array `+10`, count `+14`, assigned `+18`, worker `+1c`, kind byte `+20`, biases `+24/+28`, heading byte `+2c`, no-money flag `+30`, float accrued amount/top-up `+34/+38`. Initial y bias is -1 and heading is 1. [buildtick.c](../../LEGOLAND/buildtick.c), [workorder2.c](../../LEGOLAND/workorder2.c)

Gardener order head/tail/count are `0x0079a8b0/b4/b8`; mechanic `0x0079a8c0/c4/c8`. Worker heads/counts are gardener `0x0079a8a8/bc`, mechanic `0x0079a8ac/cc`. Park-funded repairs use head `0x0079a8d4`, size `0x28`: next, 20-byte Rect, x/y `+18/+1c`, float accrued `+20`, float top-up `+24`. The latter corrects `workorder.c`'s “owner pointer” guess. [workorder.c](../../LEGOLAND/workorder.c), [workorder2.c](../../LEGOLAND/workorder2.c)

`MapStats` at `0x00832800` is `0x3f0`: scan x/y `+04/+08`, phase `+0c`, six `0x2c` category records at `+10`; published count `+118`, income `+11c`, limits `+120/+124`. It is the statistics accumulator called “MapAI” in older files. [bigsim.c](../../LEGOLAND/bigsim.c), [DECOMP.md](../DECOMP.md#global-names-from-the-export-table)

### Rules and state machines

Build duration is cost clamped to 50..150 ticks; animation frame is `min(timer*frames/duration,frames-1)`, where layered sprites use maximum frame count. Completion frees reserved cells, plays effect `0x8f8`, places the object and clears its slot. A nominal 65×11 progress bar is disabled by an always-zero helper. Gardener-service objects become orders with `0x800` footprints; others are paid/built immediately, with message3 on insufficient funds. [buildtick.c](../../LEGOLAND/buildtick.c), [workers2.c](../../LEGOLAND/workers2.c)

Order kind1 copies one class footprint; kind2 copies the caller's rect chain. The mechanic list refuses creation at count225. Assignment picks the nearest unassigned order by squared tile distance, first wins ties, then back-links worker/order and issues plans `0x12/0x13` for object builds or `0x15/0x16` for repair/span jobs. Gardener object targets are footprint centres; mechanic `GetOrderCentre` targets the middle of the longer top/left edge, retaining half-tile precision. Span targets include order biases. Returning an unreachable job moves it to the tail unless already last; freeing decrements count. Hit testing checks only the first rect. [workorder.c](../../LEGOLAND/workorder.c), [workorder2.c](../../LEGOLAND/workorder2.c), [workers3.c](../../LEGOLAND/workers3.c)

| Build action | Gardener | Mechanic |
| --- | --- | --- |
| 0 | Aim at saved target, state12, turn, action b | Same |
| b | Within squared `0x9000`: 64; otherwise 6b | Within squared `0x10000`: 65; otherwise 6b |
| 64 / 65 | Route result2→walk and `6a OR (!f64&1)`; result1→walk, use6a when f64 bit0 set; failure returns order, idle plan, state4 wait`0x70` | Same with own plan/route action |
| 6a | Wait`0x70`, return64 | Wait`0x70`, return65 |
| 6b | Animation1, frame`0x28`, flag`0x100` | Attempt build; failure returns job with no-money flag |
| 6c | Finish animation, restore walk, clear`0x100` | Wait for cell building bit`0x20` to clear |
| 6d | Attempt build, release/order-next; failure idle/message1 | Release, set +46=1, next/idle |

The action table is recovered from the source header; it is not a timing promise in seconds. [workers2.c](../../LEGOLAND/workers2.c)

Gardener repair uses actions0 route;6 wait`0x70`;7 flags`0x108`, animation1, face order;8 finish animation;9 repair;10 free/next. Route failure walks/releases the order, idle plan`0x10`, state4 wait`0x10`. Mechanic repair uses 0 aim→b, b distance`0x10000`→6b else64,64 route,6a wait,6b repair,6c free/next. Repairs charge integer bricks only once accrued amount>=1, preserve the fraction plus top-up, tick cell life, and clear `0x4000` at maximum. Insufficient funds sets order`+30`; mechanic also raises message2. Non-object cells skip the mechanic repair step. [bigsim.c](../../LEGOLAND/bigsim.c)

Park-funded repair initializes amount to 1.5×top-up, charges the integer portion, draws paying/no-money sprites, and removes completed repairs. Repair destination selection follows class `0x200000` and gardener-service, otherwise `0x400000` and mechanic-service, otherwise automatic payment. Mechanic takeover requires holder action<`0x6b` for either order kind; gardener uses `<0x6b` for objects and `<9` for spans. [workers.c](../../LEGOLAND/workers.c), [workers2.c](../../LEGOLAND/workers2.c), [workorder2.c](../../LEGOLAND/workorder2.c)

`DoMapAI` spends up to 256 steps: phase0 clears category accumulators and rewinds; phase1 scans cells and anchor objects, counting working objects, salvage and staff; phase2 publishes and computes income clamped by limits and map capacity, then restarts. Its six-category publication loop reuses the outer step counter, resuming at step7. With no low-six map-screen bits, it counts object definitions instead. [bigsim.c](../../LEGOLAND/bigsim.c)

Bricks are a signed int at `0x004b90f8` (initial10000), lock at `0x004b90fc` (initial1); locked reads return `INT_MAX`, additions/uses do not change currency. Free-play draws a time bar. Power is a park-wide pool: served demand=`demand-unserved` must fit supply; distance and adjacency do not matter. Recheck/shed follows render-list order, not priority. Spare percentage is `100-demand*100/supply` only if supply>demand, otherwise0; generator-add returns before the consumer-side percentage update, so do not assume every call refreshes the readout. Salvage is cost×remaining/max-life, or cost if max-life is0; repair cost is cost-salvage. [money.c](../../LEGOLAND/money.c), [sweep1.c](../../LEGOLAND/sweep1.c), [power.c](../../LEGOLAND/power.c)

“Running” rejects switched-off `0x200` always, rejects blackout `0x100` only when the global power-available switch is set. During a total blackout the AI can therefore consider objects running. [sysmisc3.c](../../LEGOLAND/sysmisc3.c)

### Tables and constants

Power table `0x004b9340` has 65 eight-byte `{name,power}` entries, case-insensitive, terminated by an **empty name**; complete numeric contents are not decoded in this source. Recovered examples: Small Power Station +800, Crystal Power Station +2500; consumers -2 through Castle Obj -400. Power globals: percent`0x832bcc`, supply`bd0`, demand`bd4`, unserved`bd8`, unserved count`bdc`. [power.c](../../LEGOLAND/power.c)

Nearby-broken-cell table `0x004bff28` has 12 offsets: cardinal one-step, two-step cardinals, diagonals; inspection reads condition from the anchor cell. Message table `0x004ba8e0` has 17 `{topic,cooldown,last shown}` records; only a higher-ranked message past cooldown is shown. Raw per-entry offsets/cooldowns are not expanded by these headers. [workorder2.c](../../LEGOLAND/workorder2.c)

### Original bugs and limits

Clearing a worker's assignments increments the order count without adding nodes, eventually reaching the mechanic225 cap permanently. Repair raising, order hit-testing and completion assume nonnull map cells/objects. `UnlinkGardenerOrder` formats three conversions with two arguments, reading stack garbage. Worker-on-mouse code reloads an uninitialized found-order local when nothing was found, but its found flag prevents use. Map worker stamping only sets `0x1000`; it does not clear stale marks itself. [workorder.c](../../LEGOLAND/workorder.c), [workers2.c](../../LEGOLAND/workers2.c), [workorder2.c](../../LEGOLAND/workorder2.c), [workorder3.c](../../LEGOLAND/workorder3.c), [workorder4.c](../../LEGOLAND/workorder4.c)

### Callbacks

Class cursor/placement callbacks provide work-order footprints; worker high-level plans execute the jobs. Work-order rendering and popup integration are documented with their UI consumers. [workorder2.c](../../LEGOLAND/workorder2.c), [misc3.c](../../LEGOLAND/misc3.c), [presentation.md](presentation.md)

### Additional recovered leaf contracts

`CanHireGardener` and `CanHireMechanic` are spending operations: after the availability and fewer-than-15 checks, successful calls consume30 bricks. Callers must not treat them as pure predicates. [sysstubs.c](../../LEGOLAND/sysstubs.c)

`FreeBuildSlotAt` scans keys even in inactive slots and can decrement the active count again; `MarkObjectTiles` uses the first `0xffff` sentinel among128 four-byte entries without checking for duplicate keys. Rect subtraction appends remnants without a capacity check. [pathmisc.c](../../LEGOLAND/pathmisc.c), [objrect.c](../../LEGOLAND/objrect.c)

`ClampPopUpToScreen` keeps x at least130 when panel mode is not2, top at least37, and bottom at most367; its return value can still be the computed bottom limit when the original y is retained. Popup text measurement acquires a DC without releasing it. Work-order previews and LLIDB selection-dialog behavior remain only partially described here. [misc3.c](../../LEGOLAND/misc3.c)

Bloke `+7c` is called tiredness by visitor control and hunger by ride/food code. Both accesses are recorded; a single physiological interpretation is not established by those names. [simcore2.c](../../LEGOLAND/simcore2.c), [workers3.c](../../LEGOLAND/workers3.c), [rides.c](../../LEGOLAND/rides.c)
