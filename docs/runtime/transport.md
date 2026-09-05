# Transport: boats, flume, roads and coaster

This page consolidates recovered runtime behaviour. Addresses and offsets describe the original 32-bit executable; padding and unidentified fields remain unidentified. “Documented” means the recovered mechanics of that source are covered, not that its C is fully matched or that every asset byte has been recovered. “Partial” identifies concrete data or behavioural gaps below. The callback registration reference is [RIDE_CALLBACKS.md](../RIDE_CALLBACKS.md); its older slot descriptions must be read with the actual per-class usage described here.

## Coverage

| Source | Subsystem/material | Status and remaining boundary |
| --- | --- | --- |
| [bswater.c](../../LEGOLAND/bswater.c) | Lake records, launch, flood, carousel tick | Partial: 16×25 water artwork table is external, not decoded |
| [bswater2.c](../../LEGOLAND/bswater2.c) | Reservation mover, docking, reachability, ancillary ride records | Documented |
| [bswater3.c](../../LEGOLAND/bswater3.c) | Boat animation, tower cars, animation displacement | Partial: raw boat direction/arc table rows are external |
| [jcroute.c](../../LEGOLAND/jcroute.c) | River DFS and breadth-first flood | Documented |
| [junglecruise.c](../../LEGOLAND/junglecruise.c) | Boats, river placement, decoration teardown, drawing | Partial: 16×25 artwork table and complete decoration layouts external/unnamed |
| [roads.c](../../LEGOLAND/roads.c) | Road tile encoding, river boat animation | Partial: road table decoded; raw boat arc table rows external |
| [roads2.c](../../LEGOLAND/roads2.c) | Road allocation and route tree | Documented |
| [schoolcar.c](../../LEGOLAND/schoolcar.c) | Steering, lifecycle, coaster route/pool/save internals | Partial: model assets and remaining unnamed route fields |
| [schoolcar2.c](../../LEGOLAND/schoolcar2.c) | Turn geometry and manoeuvre selection | Documented |
| [schoolcar3.c](../../LEGOLAND/schoolcar3.c) | Pull-off manoeuvre, tube mesh topology/projection | Documented recovered pipeline; original matching residuals remain |
| [schoolcar4.c](../../LEGOLAND/schoolcar4.c) | Queue pop, obstruction, route frame integration, cubic geometry | Documented |
| [schoolcar5.c](../../LEGOLAND/schoolcar5.c) | Shade table, z commands, file reader, free stepping | Documented |
| [schoolcar6.c](../../LEGOLAND/schoolcar6.c) | RK4, bisection, z spans, shade ramp | Documented |
| [schoolcar7.c](../../LEGOLAND/schoolcar7.c) | Rotation, route reset/position save, rail offsets, LMS fixups | Partial: LMS payload schema remains opaque |
| [schoolcar8.c](../../LEGOLAND/schoolcar8.c) | Vector ops, route seats/hooks, geometry and rendering helpers | Documented recovered helpers; external asset tables remain |
| [coaster.c](../../LEGOLAND/coaster.c) | Track graph/editor, relocation save, cars/lights | Partial: allocated piece tail and some class descriptors unnamed |
| [coaster3d.c](../../LEGOLAND/coaster3d.c) | View, geometry, projection and polygon raster submission | Partial: full model templates not decoded |
| [coaster4.c](../../LEGOLAND/coaster4.c) | Piece constructors, appearance, view, save walk and shadows | Partial: external model tables; draw-order direction pairs decoded |
| [coaster5.c](../../LEGOLAND/coaster5.c) | Fit, joint wiring/profiling, car construction, sqrt tables | Documented recovered rules; external class masks remain |
| [coaster6.c](../../LEGOLAND/coaster6.c) | Run spans, height compatibility, rotation basis, seated model | Documented recovered rules; asset substitutions depend on model data |
| [coastermath.c](../../LEGOLAND/coastermath.c) | Matrix/vector/clip/joint helpers and fast roots | Documented recovered interfaces and edge behaviour |
| [lfentrance.c](../../LEGOLAND/lfentrance.c) | Entrance construction, rider state machine, layered draw | Documented |
| [lfmisc.c](../../LEGOLAND/lfmisc.c) | Boat, queue, path and rider micro-helpers | Documented relevant transport helpers |
| [logflume.c](../../LEGOLAND/logflume.c) | Class shims, drawing, placement and top-level save | Partial: external overlay offsets and image tables |
| [logflume2.c](../../LEGOLAND/logflume2.c) | Route/shape/probe and shared piece machinery | Partial: class geometry rectangles are asset-derived |
| [logflume3.c](../../LEGOLAND/logflume3.c) | Set-piece sub-route placement | Documented recovered floor plans; dimensions follow ODF footprints |
| [logflume4.c](../../LEGOLAND/logflume4.c) | Run clock, paths, depth-sorted track draw | Documented with corrected queue/splash/path interpretation |
| [logflume5.c](../../LEGOLAND/logflume5.c) | Piece-tree save/load, queue load, boat step | Documented |
| [logflume6.c](../../LEGOLAND/logflume6.c) | Spacing, fall, advance, draw, queue save | Documented with exact-endpoint caveat |
| [logflume7.c](../../LEGOLAND/logflume7.c) | Waiting-boat test, heading, polyline interpolation | Documented; endpoint safety claim unresolved |

Cross-checks use [anim2.c](../../LEGOLAND/anim2.c), [posstep.c](../../LEGOLAND/posstep.c), and the cited lane notes. They are supplementary sources, not additional rows in this page's assigned-file inventory.

## 1. Boating school

### Data structures

The lake list head is `0x004d823c`; each `BsWater` is 0x1c bytes. The station is identified by a packed two-byte map square, not a pointer to a building. [bswater.c](../../LEGOLAND/bswater.c), [bswater2.c](../../LEGOLAND/bswater2.c)

| Record | Offsets and meaning |
| --- | --- |
| `BsWater`, 0x1c | +00 u16 square; +02 u16 owning school; +04 int cardinal-arm mask; +08 int flood distance; +0c int DFS mark; +10 lake-list next; +14 frontier next; +18 predecessor/visited mark. [bswater2.c](../../LEGOLAND/bswater2.c) |
| `BsStation`, 0x34 | +00 u16 school square; +02/+04 near/far dock squares; +08 reachability **boolean** stored in a pointer-sized field; +0c frame; +10 playback direction; +14 queue count; +18 five visitor pointers; +2c next; +30 takings. The `void* route` spelling does not mean a route object. [bswater2.c](../../LEGOLAND/bswater2.c), [bswater3.c](../../LEGOLAND/bswater3.c) |
| `BsBoat`, 0x3f4; list `0x004cc03c` | +00 station key; +04/+08 current map x/y; +0c/+10 reserved destination x/y; +14/+18 current screen x/y; +1c 80 pairs of int rocking offsets (0x280 bytes); +29c 80 int sprite codes (0x140 bytes); +3dc entry side; +3e0 hull index; +3e4 mover state; +3e8 leg count; +3ec single rider; +3f0 next. [bswater3.c](../../LEGOLAND/bswater3.c) |

### Rules and state machines

A lake cell covers a 5×5 patch centred on its coordinate. Cardinal graph neighbours are five map squares apart. Placement paints the patch, claims its cells with map flags 8 and RF 2, and records the owning school. The breadth-first walk follows existing, same-owner cells, writes a predecessor and distance, and uses the two frontiers `0x004d8240/44`; the rebuild seeds the far dock. A separate DFS clears +0c and returns whether the near dock reaches the far dock. The flag stored at station +08 means “the docks are connected.” [bswater.c](../../LEGOLAND/bswater.c), [bswater2.c](../../LEGOLAND/bswater2.c)

`BoatingSchool_TryLaunch` refuses when another boat of this school is on the near jetty, targets it, or is still in launch state 1. Success creates one boat at `(school.x−1, school.y+5)`, fills rocking offsets with byte `0xf1`, clears sprite codes, and assigns the rider. Entry starts north (1), mover state starts at 1, and initial leg count is `(rand()&15)+4`, i.e. 4…19. Hull selection maps `rand() & 3` from 3 to 2, giving probabilities 1/4, 1/4, 1/2 for hulls 0, 1, 2. [bswater.c](../../LEGOLAND/bswater.c), [fable-a-bswater.md](../lanes/fable-a-bswater.md)

One graph crossing is precomputed as 80 subframes. At the crossing boundary, the pending destination becomes current and the mover dispatches as follows. [bswater2.c](../../LEGOLAND/bswater2.c)

| State | Behaviour |
| --- | --- |
| 1 | `BsBoat_StartLeg`: animate south, aim one map square south, set state 4, restart the school's jetty animation. [bswater3.c](../../LEGOLAND/bswater3.c) |
| 4 | `BsBoat_StepLeg(...,0)`: ordinary graph choice. [bswater2.c](../../LEGOLAND/bswater2.c) |
| 8 | `BsBoat_StepLeg(...,1)`: ordinary choice plus predecessor-based directional exclusion. [bswater2.c](../../LEGOLAND/bswater2.c) |
| 16 | `BsBoat_EndLeg`: docking countdown and eventual deletion. [bswater2.c](../../LEGOLAND/bswater2.c) |

The chooser starts from the current cell's arm mask. It removes north at the near jetty, then removes every arm whose destination is another boat's current **or reserved** square. State 8 removes an additional arm from the predecessor delta: horizontal predecessor removes south; otherwise nonnegative x delta removes east and negative x delta removes west. With an empty mask it animates toward the centre and stores entry −1. Otherwise a one-in-eight random pruning may reduce the surviving choices; the deterministic preference is straight, a random hand, the other hand, and finally reversal. The last fallback is not mask-tested, so the reservation system is not an absolute collision guarantee. This is different from claiming that the empty-mask arm itself moves: that arm returns after stalling. [bswater2.c](../../LEGOLAND/bswater2.c)

On a selected hop the destination is five squares away; the opposite direction bit becomes the new entry side. Decrementing the leg count to zero enters state 8. Reaching the far jetty instead enters state 16 with leg 3 and targets one square south. Docking leg 3 freezes offsets 64–79 at offset 64; leg 2 freezes offsets 0–63 at offset 64 and aims south again; leg 1 freezes the last seven offsets at offset 72 and aims south; leg 0 fades the engine, frees the boat, and returns the next list node. [bswater2.c](../../LEGOLAND/bswater2.c)

`BsWater_Relink` fills 2×2 inner corners between diagonal patches when the required orthogonal links agree. It strips the station-facing arm at each dock before filling, keeping water out of the building. [anim2.c](../../LEGOLAND/anim2.c)

### Tables and constants

| Item | Decoded values or known extent |
| --- | --- |
| Cardinal masks | N=1 `(0,−5)`, E=2 `(+5,0)`, S=4 `(0,+5)`, W=8 `(−5,0)`; table index 0,1,2,3 respectively. [bswater2.c](../../LEGOLAND/bswater2.c) |
| Lake artwork | 16 rows ×25 byte tile indices; lookup is base of TSM entry 0 plus byte slot. Full row values are not decoded in this source. A TSM record is 8 bytes `{element, loaded-u16-pointer}` at `0x0082adf4`. [bswater.c](../../LEGOLAND/bswater.c) |
| Boat motion data | `BsStep[4]` at `0x004b5118`, each `{int dx,dy,sx,sy}`; clockwise and anticlockwise `BsCurve[4]` at `0x004b5158/98`, each `{float startAngle,endAngle,offsetX,offsetY}`. The source says the shipped tables equal Jungle Cruise's, with angles drawn from 0/90/180/270/360 and corner offsets ±1, but does not print the individual rows. [bswater3.c](../../LEGOLAND/bswater3.c) |
| Animation scale | 80 samples, centre at sample 40, step scale 16, entry radius 640; heading samples compare points `j−3` and `j+4`, clamped to 0…79. Heading is `((ArcTan256(dx,dy)>>4)+6)&15`, then add `hull*16`. [bswater3.c](../../LEGOLAND/bswater3.c), [roads.c](../../LEGOLAND/roads.c) |

The animation has five cases: both sides absent gives all-zero offsets; missing exit drifts in for 40 samples then holds; missing entry holds 40 then drifts out; equal entry/exit makes a U-turn; otherwise straight travel or a true quarter-circle arc. Arc points use sine and negative cosine with radius 640 and the table's corner offset. These are boat animation curves; the log flume below uses a different, linear interpolation method. [roads.c](../../LEGOLAND/roads.c), [bswater3.c](../../LEGOLAND/bswater3.c)

### Original bugs and edge behaviour

- Launch dereferences a failed station lookup inside its existing-boat scan. Its fixed spawn offset only agrees with the recorded near jetty for the standard footprint. New water records leave +08 distance and +14 frontier link uninitialized until the walk reaches them; 5×5 off-map painting writes through a null cell. [bswater.c](../../LEGOLAND/bswater.c)
- `BsWater_RemoveOne` dereferences an empty list head before its null test. `BsBoat_StepLeg` uses missing water/station search results unguarded; its untested final reversal can choose a reserved direction. Entry −1 or 0 runs the entry-bit search to index 4, making the derived straight direction south. The state-8 y-delta store is dead. [bswater2.c](../../LEGOLAND/bswater2.c)
- `BsBoat_Destroy` faults on an empty head unless it equals the requested pointer; a missing boat in a nonempty list is left alone. The corner-fill helper uses failed station and neighbour lookups without checks. [lfmisc.c](../../LEGOLAND/lfmisc.c), [anim2.c](../../LEGOLAND/anim2.c)
- The copied road tile expression still shifts a promoted byte by eight even though only TSM entry 0 can result. Invalid arc-direction combinations can leave the curve pointer null; ordinary cardinal input avoids that case. [bswater.c](../../LEGOLAND/bswater.c), [bswater3.c](../../LEGOLAND/bswater3.c)

### Callback roles

These mover functions are internal callees: school placement/tick callbacks own station creation, admission and water rebuilding; `BoatingSchool_TryLaunch`, `BsBoat_StartLeg`, `BsBoat_StepLeg`, `BsBoat_EndLeg`, and `BsBoat_Animate` operate their records. Water remove delegates to `BsWater_RemoveOne`. Do not install a boat mover in an ObjDef slot merely because it implements a ride tick. [bswater.c](../../LEGOLAND/bswater.c), [bswater2.c](../../LEGOLAND/bswater2.c), [bswater3.c](../../LEGOLAND/bswater3.c)

## 2. Jungle cruise

### Data structures

`JcWater` has the same 0x1c layout as boating water: +00 square, +02 owner, +04 links, +08 **int breadth-first distance**, +0c DFS mark, +10 list next, +14 **frontier pointer**, +18 predecessor. Its head is `0x0062fd2c`. This reconciles `junglecruise.c`'s older pointer `rlink` at +08 and integer `f14`: `jcroute.c` observes arithmetic on +08 and list linking through +14. The predecessor is also a next-route hint when the flood is rooted at the dock. [jcroute.c](../../LEGOLAND/jcroute.c), [fable-a-jcroute.md](../lanes/fable-a-jcroute.md)

| Record | Layout |
| --- | --- |
| `JcStation`, 0x44 | +00 station square; +02 start square; +04 end square; +08 DFS result stored in a pointer-sized field; +14 queue count; +18 five queued visitors; +2c dispatch timer; +30 three prospective riders; +3c next; +40 accumulated value/takings. +0c…13 remain unnamed here. [junglecruise.c](../../LEGOLAND/junglecruise.c), [jcroute.c](../../LEGOLAND/jcroute.c) |
| `JcBoat`, 0x3f8; head `0x00616164` | +00 station key; +04/+08 current coordinate; +0c/+10 destination; +14/+18 screen coordinate; +1c 80 int-pair offsets; +29c 80 int sprite codes; +3dc entry side; +3e0 mover state; +3e4 leg countdown; +3e8 three rider pointers; +3f4 next. There is no boating-school hull field at +3e0. [junglecruise.c](../../LEGOLAND/junglecruise.c) |
| Decorations | Own and owner squares at +00/+02. Monkey fish has next at +08; tree and other decoration have their locally declared links. Complete shared decoration record meaning is not recovered here. [junglecruise.c](../../LEGOLAND/junglecruise.c) |

### Rules and state machines

Water placement probes same-owner neighbours, paints a 5×5 mask-selected patch, then fills the diagonal 2×2 inside corners. Station start `(station.x, station.y+5)` contributes a north link and the station end contributes a south link without the building itself being a water record. The existing square's owner wins before neighbour ownership is considered. Adjacent rivers of different stations do not link. [junglecruise.c](../../LEGOLAND/junglecruise.c)

The DFS honours +04 link bits and sets an integer success flag when it reaches its target. The breadth-first rebuild uses frontiers `0x0062fd30/34`, ignores link bits, and joins any existing same-owner cell five squares away. They are distinct graph interpretations, not interchangeable pathfinders. Neither rebuild stamps the seed's predecessor before expansion; a neighbour can re-enqueue the seed at depth 2. [jcroute.c](../../LEGOLAND/jcroute.c)

A party contains at most three riders. Launch scans same-station boats for current/reserved dock occupancy or launch state, then creates an animation-buffer boat at `(station.x,station.y+5)` with entry 1, state 1, leg count `(rand()&15)+4`, rocking bytes 0xf1 and sprite codes zero. The shared playback cursor `0x00629c54` runs 0…79. On wrap, the mover commits destinations and dispatches state 1 to departure, 4 to ordinary stepping, 8 to stepping with a route hint, and 16 to docking/removal. The second argument to `JcBoat_Step` enables predecessor-based exclusion; the old “turn table selector” comment is wrong. Its current/reserved-square blocking, optional one-in-eight pruning and ordered direction preferences are documented by the mover itself. [junglecruise.c](../../LEGOLAND/junglecruise.c), [anim2.c](../../LEGOLAND/anim2.c)

The docking buffer freezes follow the same four-count sequence as boating school. Rendering is split into open-water mode 0 and station mode 1, with station-contained boats selected by launch/arrival state or the station's own exit column. Hull and overlays at sprite-code +0x10/+0x20 interleave the riders. Heading codes 4…11 use seat order 0,1,2; other headings use 2,1,0, and codes above 8 swap seats 1 and 2. Only the station pass releases riders at state 16, leg 2, playback tick 79. [junglecruise.c](../../LEGOLAND/junglecruise.c), [fable-b-ridemisc.md](../lanes/fable-b-ridemisc.md)

Removing water or a monkey tree subtracts one station value; a monkey fish subtracts two. Fish removal restores the whole inclusive class footprint to base terrain first. The unsaved extra decoration subtracts no value, uses cursor strip `{0,−1,0,1}`, and clears map flag 0x40 on the three cells six squares left of its origin. [junglecruise.c](../../LEGOLAND/junglecruise.c)

### Tables and constants

| Data | Values |
| --- | --- |
| Water masks | N/E/S/W bits 1/2/4/8, displacement five cells. Artwork is 16×25 bytes at `0x004b72e4`; complete row values are not printed. All byte entries select tileset 0. [junglecruise.c](../../LEGOLAND/junglecruise.c) |
| Animation tables | Step table `0x004b7148`; clockwise arc `0x004b7188`; anticlockwise arc `0x004b71c8`; same 16-byte record meanings and geometry as boating school. Full per-entry values remain external. [roads.c](../../LEGOLAND/roads.c) |
| Facing | Sixteen headings, 22.5° per heading; rider angle `−(code*22.5+45)/360 * 2π`; side seats turn by ±6 headings, seat 2 rises 16 pixels. [junglecruise.c](../../LEGOLAND/junglecruise.c) |
| Projection | Whole-cell isometric x uses `(cx−cy)*(tw/2)−(tw+1)/2`; y uses `(cx+cy)*(th/2)`; scroll is converted from 24.8 and rocking is projected as x−y/x+y scaled by 1/512. The animation producer's “1/16 map units” and drawer's 1/512 scale are different coordinate stages. [junglecruise.c](../../LEGOLAND/junglecruise.c), [roads.c](../../LEGOLAND/roads.c) |

### Original bugs and edge behaviour

- `JungleCruise_CountStationBoats` **assigns** the supplied station key into every boat and counts every boat if that key is nonzero (none if zero). This mutates boat ownership park-wide. [junglecruise.c](../../LEGOLAND/junglecruise.c)
- `JcBoat_Unlink` and decoration/water unlinks begin with unguarded list-head accesses. Route rebuild, launch, mover, and inner-corner fill can dereference missing station or water records. Painting and the extra decoration's off-map flag clear dereference null map cells. [junglecruise.c](../../LEGOLAND/junglecruise.c), [anim2.c](../../LEGOLAND/anim2.c)
- The BFS seed is re-enqueued; BFS ignores links while DFS honours them. This can change routes even when the link bitmap and physical cells disagree. [jcroute.c](../../LEGOLAND/jcroute.c)
- The mover's own note records that random pruning discards the current heading, and its last resort recomputes a previously rejected direction. These weaken blanket claims that boats can never collide; reproduce the actual chooser branches. [anim2.c](../../LEGOLAND/anim2.c)

### Callback roles

`MonkeyTree_Remove`, `MonkeyFish_Remove` and `JcDeco_Remove` are per-class removal handlers; the river's removal handler delegates to `JcWater_RemoveOne`. River and boat rendering are internal callees of the station/world passes. `JungleCruise_BuildRoute` reports connectivity, whereas `JungleCruise_RebuildRoute` maintains movement hints. [junglecruise.c](../../LEGOLAND/junglecruise.c), [jcroute.c](../../LEGOLAND/jcroute.c)

## 3. Log flume

### Data structures

All ten classes share runs and pieces: entrance, plain track, four special corners, CSAW, tunnel, drop and hold-up. There are **eight**, not nine, classes other than entrance/track; the older `logflume.c` sentence miscounts them. [logflume.c](../../LEGOLAND/logflume.c), [RIDE_CALLBACKS.md](../RIDE_CALLBACKS.md)

| Record | Offsets and meanings |
| --- | --- |
| `LFRun`, 0xd4; head `0x004cbe84` | +00 next; +04 flags (mask 1 loading, mask 2 run splash active); +08 station-side route piece; +0c unloading-side piece; +10 top-level piece list; +14 packed station square; +18 saved piece reference (full role unresolved); +1c shared animation frame; +20 boat-step countdown; +24 splash cursor; +28 splash length; +2c queue; +38 loading/waiting boat pointer; +3c boat count; +40 four inline 0x24-byte boats; +d0 piece count. [logflume4.c](../../LEGOLAND/logflume4.c), [logflume6.c](../../LEGOLAND/logflume6.c), [posstep.c](../../LEGOLAND/posstep.c) |
| `LFQueue`, 0x0c | +00 path pointer; +04 linked-list head; +08 tail. This is `LFRun+0x2c`, occupying +2c/+30/+34. The old `LFAnimRefs {int r[3]}` is an opaque spelling of these three pointers; `logflume4.c` declares only the two fields it reads. [logflume.c](../../LEGOLAND/logflume.c), [logflume5.c](../../LEGOLAND/logflume5.c), [lfmisc.c](../../LEGOLAND/lfmisc.c) |
| Queue path/node | Path: +00 count/capacity, +04 points pointer, then `count*12` point bytes in the allocation. Node: +00 next, +04 rider reference. The 12-byte queue-path point interpretation is not fully named by these declarations. [logflume5.c](../../LEGOLAND/logflume5.c), [logflume6.c](../../LEGOLAND/logflume6.c) |
| `LFBoat`, 0x24 | +00 coast countdown; +04 flags (1 moving, 2 falling, 4 in spray); +08 rider node; +0c/+10 draw x/y offset; +14 route piece; +18 float within-piece parameter z; +1c tilt/barrel animation frame; +20 float speed in pieces per step. Earlier “bit 0 waiting” annotations are inverted. [logflume6.c](../../LEGOLAND/logflume6.c), [logflume7.c](../../LEGOLAND/logflume7.c) |
| `LFPiece`, 0x38 | +00/+04 list next/prev; +08/+0c route forward/back; +10 flags; +14 u16 square; +18 kind; +1c orientation 0…3; +20 class definition; +24 run; +28 parent; +2c child list; +30/+34 route endpoints of a compound piece. List ordering, route ordering and parentage are separate relations. [logflume2.c](../../LEGOLAND/logflume2.c), [logflume3.c](../../LEGOLAND/logflume3.c), [logflume5.c](../../LEGOLAND/logflume5.c) |
| `PieceDraw`, 0x14 | +00 sprite; +04/+08 class draw parameters; +0c square; +10 cleared. A static instance is returned by the class draw path. [logflume.c](../../LEGOLAND/logflume.c) |
| `LFGeom`, 0x24; `LFPath`, 8 | Geometry: +00 direction mask, +04 four int x/y connection points. Boat path: +00 point count, +04 pointer to int-pair points. This boat path is distinct from the 12-byte-point visitor queue path. [logflume2.c](../../LEGOLAND/logflume2.c), [logflume4.c](../../LEGOLAND/logflume4.c) |

### Rules and state machines

Placement probes the four compass neighbours into one shared array at `0x004cbe20`. Every probe overwrites the previous result. Cell spacing is the flume footprint's width/height (`right−left`, `bottom−top`), not necessarily one map cell. Neighbours with kind 3 or 4 expose free ends; other kinds are removed before shape validation. A legal square has one or two connections. One neighbour extends a route; two neighbours splice routes, reversing same-facing ends when necessary. [logflume2.c](../../LEGOLAND/logflume2.c), [logflume6.c](../../LEGOLAND/logflume6.c)

A compound set piece expands into ordinary track sub-pieces. They inherit the run, point at the parent, carry flag mask 4 so they are not separately charged/drawn/demolished, and form both a parent-owned list and a private route. Parent +30/+34 publish the first/last endpoints. Ordinary sub-piece kinds are 1 straight, 2 turn, and 3 at an end. `LFBoat_Heading` treats kind 3 via its orientation directly; “3=drop” in its local comment is too broad to use as a universal class enum. `LFBoat_IsOnDrop` tests the parent class against LOG FLUME DROP. [logflume3.c](../../LEGOLAND/logflume3.c), [logflume7.c](../../LEGOLAND/logflume7.c), [lfmisc.c](../../LEGOLAND/lfmisc.c)

Special corners are five-cell L routes: corner 1 goes two south, turn, two east; corner 2 two west then south; corner 3 two north then west; corner 4 two east then north. The last whole cell in the class footprint determines the far-edge origins for corners 2/3. Tunnel/drop connect north–south; CSAW/hold-up connect west–east. Drop placement additionally rejects conflicting north +08 or south +0c route links. Sub-piece coordinates advance by the footprint-derived cell width/height; the number and arrangement of cells are class-specific, as tabulated below. [logflume2.c](../../LEGOLAND/logflume2.c), [logflume3.c](../../LEGOLAND/logflume3.c)

The other recovered set-piece floor plans, in units of cell width w and height h from a class-biased origin `(x0,y0)`, are below. Each pair is a relative grid position and the lists are in route order. C-SAW origin adds 3 map cells to class-top y, tunnel adds 6 to class-left x, hold-up adds 9 to class-top y. [logflume3.c](../../LEGOLAND/logflume3.c)

| Set piece | Route cells (relative x/w,y/h) |
| --- | --- |
| C-SAW, 8 | `(0,0),(1,0),(2,0),(3,0),(4,0),(5,0),(5,−1),(6,−1)`; endpoints face W/E. |
| Tunnel, 7 | `(0,0),(0,1),(0,2),(−1,2),(−2,2),(−2,3),(−2,4)`; endpoints N/S. |
| Hold-up, 10 | `(0,0),(1,0),(2,0),(2,−1),(2,−2),(3,−2),(4,−2),(5,−2),(5,−1),(6,−1)`; endpoints W/E. |

A parent link of −1 identifies station ownership; 0 means independent. Hit-testing resolves the parent first, uses the station run's square and a computed entrance rectangle, or a track cell rectangle shrunk by one, or the identified scenery class footprint. Computed rectangles are shared scratch at `0x004c2aa8/0x004c8d38`, so callers cannot retain independent results across another query. Unknown classes return no rectangle. [logflume2.c](../../LEGOLAND/logflume2.c)

The entrance builds its run, station piece, a column of channel pieces at footprint-height spacing and terminal pieces. It sets boat count 4 and initial piece count 6, then parks boats by walking backward from run +08: state/flags/frame zero, bottom-left draw point, z=1 and speed=0.1, one boat per piece. A compound piece's own square is skipped by `LFTrack_FindPiece` whenever it has sub-pieces; lookup then tests its children. [lfentrance.c](../../LEGOLAND/lfentrance.c), [posstep.c](../../LEGOLAND/posstep.c)

Each run tick advances an active **splash** cursor until its length, restarts boat positions if the circuit is incomplete, otherwise decrements +20 and calls each boat step when it becomes negative, resetting to 2. Thus boat stepping happens every third frame; a step is fractional travel, not automatically a whole square. On those ticks an empty, **nonmoving** boat at `run.f08->fwd` may become the waiting boat. If a visitor has reached the queue path's last point, the run sets loading, pops the visitor and installs the rider in that boat. Every frame also advances the queue, sets admission from capacity, and increments run frame modulo 16. [logflume4.c](../../LEGOLAND/logflume4.c), [logflume7.c](../../LEGOLAND/logflume7.c)

`LFBoat_Step` dispatches falling boats directly to the fall handler. Moving boats check room, advance z and, when a piece boundary is crossed, commit the next piece and test for entering a drop. Coasting boats decrement +00, attempt a start after it becomes negative if the run is not loading, and reset this counter to 1. Reaching `run.f0c->fwd` with a rider advances that visitor's action and empties the boat; reaching `run.f08->fwd` parks it. Parking clears moving and sets coast count 0x32. A boat is blocked only by a positive gap strictly below 0.8 on its own or next piece; equality, boats behind, and boats two pieces ahead do not block. [logflume5.c](../../LEGOLAND/logflume5.c), [logflume6.c](../../LEGOLAND/logflume6.c)

The path sampler is **piecewise linear** over equal parameter intervals between consecutive points. With n points, interval width is `1/(n−1)`; the selected adjacent pair is interpolated by the local fraction. Reversal indexes the pair from the far end. Four-point corners are three chords, despite the older “spline” and “cubic control polygon” labels. Advance adds speed to z, subtracts one only when the result is **greater than** 1, and returns a boundary-crossed flag; the caller updates the piece pointer. It refuses progress when the prospective drawing piece has no following piece. [logflume4.c](../../LEGOLAND/logflume4.c), [logflume6.c](../../LEGOLAND/logflume6.c), [logflume7.c](../../LEGOLAND/logflume7.c)

The drop is driven by `t = index within the drop's sub-route + z`, after advancing the boat. Leaving the parent's last sub-piece clears falling. The following tests execute in source order and overlap at 5 and 10; this matters at exact boundaries. [logflume6.c](../../LEGOLAND/logflume6.c)

| t | Result |
| --- | --- |
| t<2 | Frame 0; retains current speed. |
| 2≤t<4 | Frame `trunc(((t−2)*0.5)*120)`; speed 0.05. |
| 4≤t≤5 | Frame 120; speed 0.1. |
| 5≤t≤10 | Frame `120−trunc(((t−5)*0.2)*120)`; speed increases 0.05; spray set when t<9 and cleared at/after 9. At t=5 this follows the preceding rule, giving speed 0.15. |
| t≥10 | Frame 0; speed reset 0.1, overriding the preceding increase at t=10. If t<10.5 and the run is not already splashing, set run mask 2, cursor 0, and take splash length from the sprite if available. |

The 10…10.5 window plus the active-run guard, rather than a separate “crossed ten” latch, gates the splash. The source describes this as one-shot under normal step sizes; absent art leaves the previous splash length. Boat spray uses barrel/matte artwork; the run's splash sprite is frozen to the shared +24 cursor at fixed offset `(−74,+149)`. [logflume6.c](../../LEGOLAND/logflume6.c), [logflume.c](../../LEGOLAND/logflume.c)

The rider's action byte at Bloke +60 runs the twelve states below; positions are 24.8. [lfentrance.c](../../LEGOLAND/lfentrance.c)

| Action | Meaning |
| --- | --- |
| 0 | Join this ride (flags mask 8); enqueue, set queueing mask 0x40; stop admission if full. |
| 1 | Queue-controlled wait. |
| 2 | Walk to jetty: two tiles left of the station, top edge. |
| 3 | Walk to boat: 2.5 tiles left, half a tile down. |
| 4 | Board: set riding mask 0x80; reset walk animation; set waiting boat state 1, forget run +38, clear loading. |
| 5 | Ride; boat owns movement. |
| 6 | Disembark: clear riding; teleport 2.5 tiles left/half a tile up; facing byte 3; walk to two left/one up. |
| 7 | Walk to the station square, one tile up. |
| 8 | Start the five-frame exit path at its last frame, stepping backward. |
| 9 | Follow exit path offsets relative to ObjDef queue/exit offset +24/+25 and station square. |
| 10 | Walk to the centred queue/exit square. |
| 11 | Leave class rider list and clear on-ride flag. |

Drawing interleaves five queue depth bands with entrance sprites. The x threshold is station-world-x minus 0x280; remaining bands distinguish y≤station−9, y=station−8…−6, y=station−3, and y=station−2…station. Only same-station visitors without riding flag are drawn in those bands. A boat draws on its current piece, the previous piece while z<0.5, and the next piece from z≥0.5, with half-tile clipping for overhangs; rider drawing sits between boat and matte. [lfentrance.c](../../LEGOLAND/lfentrance.c), [logflume6.c](../../LEGOLAND/logflume6.c), [posstep.c](../../LEGOLAND/posstep.c)

Save writes a 0xd4 run image and separately writes the **piece tree**, in preorder: dword 1, 0x38-byte piece with route pointers +08/+0c/+30/+34 converted to indices, class-name byte length and bytes without NUL, then its child list; dword 0 ends each list. Load reconstructs ownership/list links, resolves the class in LLIDB and relinks indexed route references. Queue save/load writes path count, `count*12` point bytes, rider count and rider ordinals, rebuilding `{path,head,tail}`. Run's loading boat reference is an index into slots at +40 with stride 36. Some old save-view structs begin at a boat's +08 to address rider/piece fields; they do **not** move the physical boat array to +48. [logflume.c](../../LEGOLAND/logflume.c), [logflume5.c](../../LEGOLAND/logflume5.c), [logflume6.c](../../LEGOLAND/logflume6.c)

### Tables and constants

Legal neighbour masks are exactly `{0x01,0x04,0x10,0x40,0x11,0x44,0x05,0x14,0x50,0x41}`. These use two-bit spacing: N=0x01, E=0x04, S=0x10, W=0x40. Geometry's ordinary direction bits are a different mask `{1,2,4,8}`. Corner index at `0x004c2af4` is 0…3. Placement error codes are 3 people, 4 footprint rejection, 2 insufficient bricks, 0x0e no connection. Real/ghost cursors are 0x1834 bytes at `0x007febc0`/`0x00810160`. [logflume2.c](../../LEGOLAND/logflume2.c), [logflume.c](../../LEGOLAND/logflume.c)

For tile dimensions tw/th, quadrant centres are TL=(tw,th), TR=(3tw,th), BL=(tw,3th), BR=(3tw,3th). Let dx=`(2*tw)>>2`, dy=`(2*th)>>2`. These are the complete decoded path records; offsets shown for the middle points are relative to endpoints. [logflume4.c](../../LEGOLAND/logflume4.c)

| Path record / point array | n | Points |
| --- | ---: | --- |
| `0x004c2b58` / `0x004cbe38` | 2 | TR, BL |
| `0x004c2b00` / `0x004c8d58` | 2 | BR, TL |
| `0x004c2be8` / `0x004cbde8` | 4 | TR, TR+(−dx,+dy), BR+(−dx,−dy), BR |
| `0x004c2bc0` / `0x004c2b30` | 4 | BR, BR+(−dx,−dy), BL+(+dx,−dy), BL |
| `0x004c2c10` / `0x004c2bc8` | 4 | BL, BL+(+dx,−dy), TL+(+dx,+dy), TL |
| `0x004c2c08` / `0x004c2b78` | 4 | TL, TL+(+dx,+dy), TR+(−dx,+dy), TR |

The shape-to-path table is straight kind 1 dir 1→BR/TL (reverse when heading E), dir 0→TR/BL (reverse N); corner kind 2 dir 3→TL/TR (reverse W), 0→TR/BR (reverse N), 1→BR/BL (reverse E), 2→BL/TL (reverse S). Headings are N/E/S/W=1/3/5/7; kind 3 maps orientation 0/1/2/3 directly to those headings. [logflume6.c](../../LEGOLAND/logflume6.c), [logflume7.c](../../LEGOLAND/logflume7.c)

Overlay sets are `{count,frames}` with 12-byte `{dx,dy,sprite}` frames. Corner 1 at `0x004b4808` is `{2,0x004b47f0}` (sprite fields `0x004b47f8/0x004b4804`); corner 3 at `0x004b4828` is `{2,0x004b4810}` (fields `0x004b4818/24`); hold-up at `0x004b4858` is `{3,0x004b4830}` (fields `0x004b4838/44/50`, middle sprite permanently null). Corners 2/4 draw one overlay each. Raw dx/dy values of these external frames are not decoded here. Entrance overlays generally use `(−100,−209)`, except entrance1 `(8,−113)`. [logflume.c](../../LEGOLAND/logflume.c), [lfentrance.c](../../LEGOLAND/lfentrance.c)

### Original bugs and disagreements

- Track update/add leave the selected run uninitialized when all four neighbour pointers are null. Later function notes explicitly supersede the header's speculative “neighbour-array sentinel”; update normally excludes this arm by its nonzero-neighbour count. [logflume.c](../../LEGOLAND/logflume.c)
- `LFTrack_Remove` reads `piece->run` before checking piece. `LFCorner_Place` reads the parent's square before checking parent. Entrance construction has unchecked piece-allocation results, including its central piece and reading the first end piece back. [logflume.c](../../LEGOLAND/logflume.c), [logflume3.c](../../LEGOLAND/logflume3.c), [lfentrance.c](../../LEGOLAND/lfentrance.c)
- Mask 0x41 repeats the first same-facing-end condition instead of testing its mirror, so two back-linked N/W ends are spliced rather than joined/reversed. [logflume2.c](../../LEGOLAND/logflume2.c)
- CSAW, hold-up and drop add handlers reset the shared corner index to zero; tunnel does not. The global remains an observable implicit input until another corner shim overwrites it. Static piece draw returns stale data if no piece exists. [logflume.c](../../LEGOLAND/logflume.c)
- Missing class LLS leaves overlay frame uninitialized; original argument-slot reuse feeds an element pointer as a frame, which LLS clamping makes appear as the last frame. Entrance's analogous path uses the square argument's address and can make its first slice/matte appear even without a valid frame. [logflume.c](../../LEGOLAND/logflume.c), [lfentrance.c](../../LEGOLAND/lfentrance.c)
- Station demolition builds cursor y from footprint **right** edge v[2], not top v[1], and charges `UseBricks` for paid pieces again rather than refunding them. [logflume.c](../../LEGOLAND/logflume.c)
- Invalid direction leaves `LFPiece_DrawJoin`'s shape uninitialized before byte-masked indexing. A zero displacement leaves advance heading uninitialized; unsupported kind/orientation leaves its draw point uninitialized, including reachable non-curve kinds. The spacing helper's preloaded uninitialized gap is overwritten on every admitted comparison path. [logflume5.c](../../LEGOLAND/logflume5.c), [logflume6.c](../../LEGOLAND/logflume6.c)
- `LFPath_Point(t=1)` reads point n (or −1 reversed); n=1 divides by zero. The header calls these unreachable, but advance only normalizes **>1**, so it does not establish safety for exact equality. All normal tables have 2/4 points; endpoint reachability needs a numeric/runtime test before claiming it impossible. [logflume7.c](../../LEGOLAND/logflume7.c), [logflume6.c](../../LEGOLAND/logflume6.c)
- The descriptions “waiting bit,” “doors open,” “spline,” and “one square every third frame” in earlier files are corrected by the callee bodies: moving bit, run splash, polyline, and fractional boat step respectively. [logflume4.c](../../LEGOLAND/logflume4.c), [logflume5.c](../../LEGOLAND/logflume5.c), [logflume6.c](../../LEGOLAND/logflume6.c), [logflume7.c](../../LEGOLAND/logflume7.c)

### Callback roles

For each corner family `LFCorner1`…`LFCorner4`, and `LFTunnel`, `LFCsaw`, `LFHoldUp`, `LFDrop`, the function suffixes map to ObjDef offsets: `Tick` +8c (placement selection), `Update` +90, `Update2` +94, `Add` +98, `Remove` +9c, shared `LFPiece_Draw` +a0, `Create` +a4, `Destroy` +ac and `Interact` +b0. Here +b0 is the actual per-square overlay **draw** hook. Corner shims first select their global index; corners copy footprints while other set pieces pass the class footprint in place. Shared geometry callbacks are Shape/Probe/Geom/Place, distinct from ObjDef callbacks. [logflume.c](../../LEGOLAND/logflume.c), [logflume2.c](../../LEGOLAND/logflume2.c)

Entrance uses `LFEntrance_Tick` +8c, `Update` +90, `Update2` +94, `Add` +98, `Remove` +9c, `Create` +a4, `Activate` +a8, `Destroy` +ac, `Interact` +b0, `LoadLogFlume` +b8, `SaveLogFlume` +bc and `LFEntrance_Extra` +c0. Activate drives the rider state machine and run ticks; Interact draws the layered station. Plain track uses its `LFTrack_*` equivalents, including +a0 `LFTrack_Draw`; the eight scenery classes share `LFPiece_Draw`. [logflume.c](../../LEGOLAND/logflume.c), [lfentrance.c](../../LEGOLAND/lfentrance.c), [RIDE_CALLBACKS.md](../RIDE_CALLBACKS.md)

## 4. Driving school and roads

### Data structures

`RoadRec` is 0x20 bytes, list head `0x004cbeac`: +00 next; +04 frontier next; +08 u16 school key; +0c/+10 block origin ints; +14 kind byte; +15 byte distance; +18 predecessor; +1c usage count (other byte fields not fully named). A road record represents **4×4 map cells**, correcting the loose “one map square” wording in `anim2.c`. [roads2.c](../../LEGOLAND/roads2.c), [coaster.c](../../LEGOLAND/coaster.c), [anim2.c](../../LEGOLAND/anim2.c)

`SchoolCar` is 0xd0 bytes. [schoolcar2.c](../../LEGOLAND/schoolcar2.c)

| Offset | Field |
| --- | --- |
| +00/+04 | List next / u16 owning school |
| +08/+0c | Screen coordinate |
| +10/+14 | World coordinate, 16.16 |
| +18/+20 | Current map int-pair / next manoeuvre start int-pair |
| +28/+2c | Velocity x/y |
| +30…af | Sixteen 8-byte 16.16 x/y waypoints |
| +b0/+b4 | Cached float unit vector toward front waypoint |
| +b8/+b9/+ba/+bb | Body frame / unnamed byte / road heading / waypoint count |
| +bc/+be/+c0 | u16 stall / lifetime / horn timer |
| +c2/+c3/+c4/+c5 | Pull-off/lifecycle flag (misleading `on_road` name) / livery / manoeuvre / unnamed byte |
| +c6/+c8/+cc | u16 top speed / speed / driver pointer |

### Rules and state machines

Road rebuilding clears a school's frontier, distances and predecessor marks, seeds its kind-6 entrance, then expands breadth-first. Entrance expansion removes north/south/west, leaving an east-only outgoing gate. The first ply reaching a block claims it and fixes its predecessor; the entrance retains no predecessor. Cars use these parent pointers as a shortest-path tree back to school. [roads2.c](../../LEGOLAND/roads2.c)

Admission permits a new car only when `5*CountSchoolCars(school) < CountSchoolRoadTiles(school)`. Spawn also rejects another car within one square in both axes. A new car has one of three liveries, random horn pitch, lifetime 9000 and horn timer 1200; the per-frame sweep decrements timers and retires cars on its lifecycle condition, returning road usage. The +c2 label must not be read literally as “safely on road”: the pull-off manoeuvre explicitly sets it and the driver clears it before selecting a new manoeuvre. [coaster.c](../../LEGOLAND/coaster.c), [schoolcar.c](../../LEGOLAND/schoolcar.c), [schoolcar2.c](../../LEGOLAND/schoolcar2.c)

Manoeuvres append paths rather than moving a car instantly. Waypoint 0 is the steering target; popping shifts the queue. With no waypoints speed/velocity stop. Otherwise steering normalizes the target displacement, rounds the heading to one of sixteen body directions, and limits turns exceeding two heading steps to a one-step nudge per frame. A zero-length target yields zero unit vector/velocity. The blockage test first requires squared distance ≤0x40000 in 24.8, then distance to the forward one-unit probe ≤0x10000. It walks the whole list and returns the **last** matching car. [schoolcar.c](../../LEGOLAND/schoolcar.c), [schoolcar4.c](../../LEGOLAND/schoolcar4.c)

| Code | Action |
| --- | --- |
| 0 | Stop |
| 1 | Left: `SchoolCarManoeuvreA` |
| 2 | Straight: `SchoolCarManoeuvreB` |
| 3 | Right: `SchoolCarManoeuvreC` |
| 4 | Pull off: `SchoolCarManoeuvreD` |
| 5 | Arrival: A followed by B |

`SchoolCarNextManoeuvre` follows the walked route, falling back when ownership, road existence or predecessor geometry cannot resolve it. While the horn timer runs, `SchoolCarNextManoeuvreHorn` chooses among the next block's same-school, non-entrance left/straight/right exits. No usable block ahead yields code 4; a usable block with zero onward exits yields code 2. It consumes a random number even before the missing-road check. [schoolcar2.c](../../LEGOLAND/schoolcar2.c)

Road tiling first claims all sixteen cells (RF 2, map flag 8, driving-school element, block key) and removes existing footpaths. It fills a 4×4 scratch tile array, then stamps according to rotation. Zebra bit 0x10 replaces one interior strip; reading tile slot 0x1a or 0x1b back decides whether to turn the east–west row or north–south column into walkable path (RF 3). The four exterior middle-edge neighbours are reshaped if already paths. Restitching into a corner/T/crossroads removes a zebra and refunds its cost; an isolated block retains its prior tile and zebra. [roads.c](../../LEGOLAND/roads.c), [anim2.c](../../LEGOLAND/anim2.c)

### Tables and constants

Road headings use NW,N,NE,E,SE,S,SW,W = 0…7, with only odd directions active. Offset rotation is N `(side,−forward)`, E `(forward,side)`, S `(−side,forward)`, W `(−forward,−side)`. Left subtracts 2, right adds 2 modulo 8; position helpers move 2 or 4 map squares. [schoolcar7.c](../../LEGOLAND/schoolcar7.c), [posstep.c](../../LEGOLAND/posstep.c)

The free-driving mask uses left=1, straight=2, right=4. Mask 0/2 returns straight, 1 left, 4 right. Masks 3/5/6 choose their two exits evenly using random bit 1; mask 7 chooses straight when bit 1 is set (1/2), otherwise left/right from bit 0 (1/4 each). This is not uniform random selection among three exits. [schoolcar2.c](../../LEGOLAND/schoolcar2.c)

Curve points are in 1/256 map units before conversion to 16.16. Tight turns use `(0x68,0), (0x10c,0x2c), (0x1a8,0x7a), (0x200,0xcc)`. Wide turns double sideways offsets to `0,0x58,0xf4,0x198`, take a leading half-block step and append a fifth straight point after the turn. Left negates sideways offsets. Global `0x004c11c0 !=0` selects tight-left/wide-right; zero selects wide-left/tight-right. Pull-off uses `(0x0d,±0x28)`, `(0x21,±0x54)`, turns ±2 and adds a half-block endpoint; it sets +c2. [schoolcar2.c](../../LEGOLAND/schoolcar2.c), [schoolcar3.c](../../LEGOLAND/schoolcar3.c)

Kind byte packs `shape | ((rotation&3)<<5)`, with zebra bit 0x10. The tiler calls shapes 1 dead end, 2 straight/default, 3 corner/T-detail arm, 4 junction outer, 5 crossroads centre, 6 stem cap, 7 capped both ends. The restitcher chooses shape 3 for two adjacent neighbours, 4 for three and 5 for four; these call-site values supersede the imprecise “3 T junction” wording in the tiler's header. [roads.c](../../LEGOLAND/roads.c), [anim2.c](../../LEGOLAND/anim2.c)

The complete decoded `0x004b4c08` road table has fifteen rows of four **hexadecimal** u16 slots; all have tileset high byte zero. TSM records at `0x0082c67c` resolve a word as loaded base of `word>>8` plus low byte. [roads.c](../../LEGOLAND/roads.c)

| Piece | Slots by rotation 0,1,2,3 | Meaning |
| ---: | --- | --- |
| 0 | 00,01,02,03 | Outer kerb corner |
| 1 | 06,07,04,05 | Kerb edge |
| 2 | 0a,0c,0f,09 | Left cap corner |
| 3 | 0b,0d,0e,08 | Right cap corner |
| 4 | 12,14,16,10 | Left cap half |
| 5 | 13,15,17,11 | Right cap half |
| 6 | 18,1d,1c,19 | Zebra end |
| 7 | 1a,1b,1a,1b | Zebra centre |
| 8 | 20,21,1e,1f | Junction kerb |
| 9 | 24,25,22,23 | Junction corner |
| 10 | 27,28,29,26 | Junction inner |
| 11 | 2a,2a,2a,2a | Tarmac |
| 12 | 2e,2b,2c,2d | Junction detail |
| 13 | 31,32,2f,30 | Junction detail |
| 14 | 34,35,36,33 | Stem cap |

For scratch `tiles[row][column]`, map `(x+r,y+c)` receives rotation 0=`tiles[c][r]`, 1=`tiles[3−r][c]`, 2=`tiles[3−c][3−r]`, 3=`tiles[r][3−c]`. Zebra uses interior scratch row 1 for rotations 0/1, row 2 for 2/3. East–west crossing is map `(x…x+3,y+1)`; north–south is `(x+2,y…y+3)`. Traffic lights are one global two-axis cycle, 140 green ticks and 40 all-red ticks per axis, drawn at all four corners of kind-5 road blocks. [roads.c](../../LEGOLAND/roads.c), [coaster.c](../../LEGOLAND/coaster.c)

### Original bugs and edge behaviour

- Queue pop always copies sixteen entries, so its final copy reads `wp[16]`, the raw float bits at +b0/+b4, into `wp[15]`. The decremented count normally makes the poisoned last slot inactive. [schoolcar4.c](../../LEGOLAND/schoolcar4.c)
- `RotateByHeading` returns uninitialized stack dwords for every noncardinal heading. `Pos_Step2/4` instead leave the destination unchanged for those inputs. [schoolcar7.c](../../LEGOLAND/schoolcar7.c), [posstep.c](../../LEGOLAND/posstep.c)
- Car removal frees the record before unlinking, then compares a dangling pointer (the head path caches next first). [schoolcar.c](../../LEGOLAND/schoolcar.c)
- Off-map road claiming and zebra writes dereference null cells; only the final exterior probes check bounds. Rotation outside 0…3 stamps no tiles even though earlier claiming has run. [roads.c](../../LEGOLAND/roads.c)

### Callback roles

`NewRoadRecord`, `DrivingSchool_WalkStep`, `Road_SetTile`, `Road_Restitch`, car spawning/ticking and manoeuvres are helpers under the driving-school and road placement callbacks. Driving-school rider admission calls car creation after the occupancy test; traffic-light draw and car ticks are class-driven park passes. The function-letter manoeuvres are not ObjDef slots. [roads2.c](../../LEGOLAND/roads2.c), [coaster.c](../../LEGOLAND/coaster.c), [schoolcar2.c](../../LEGOLAND/schoolcar2.c)

## 5. Coaster graph, physics, rendering and save

### Data structures

There is one castle/coaster at `0x00829ae0`. Its node sentinel begins at record +04. The recovered allocations distinguish a 0x50 node prefix from a **0xa4 allocated track piece**. A universal 0x24 footprint interpretation at node +2c is also unsafe: raised pieces use +40 for cached world position and +4c for their render object, overlapping the old footprint padding. Preserve the known prefix/variant fields; the rest of the 0xa4 tail remains incompletely named. [coaster.c](../../LEGOLAND/coaster.c), [coaster4.c](../../LEGOLAND/coaster4.c), [schoolcar.c](../../LEGOLAND/schoolcar.c)

| Record | Layout |
| --- | --- |
| Castle/coaster | +00 state 0 none/1 open/2 closed; +04 node sentinel; +a8 head end; +ac head `JointSlot`; +c0 tail end; +c4 tail slot; +d8 route; +e0 station/route timing; +e4 car-list sentinel; +10c inline three-hook route interface. Complete allocation extent not asserted here. [coaster.c](../../LEGOLAND/coaster.c), [schoolcar7.c](../../LEGOLAND/schoolcar7.c), [schoolcar8.c](../../LEGOLAND/schoolcar8.c) |
| `TrackNode`, 0x50 prefix | +00 state bits (bit 0 raised, bit 1 ring); +04/+06 16-bit map coordinates; +08 class; +0c descriptor; +10 owner; +14 head joint; +20 tail joint; +2c footprint rect/link prefix. [coaster.c](../../LEGOLAND/coaster.c), [coaster6.c](../../LEGOLAND/coaster6.c) |
| `TrackJoint`, 12 | +00 **int direction mask** 1/2/4/8, −1 free; +04 **float world height**; +08 neighbour. Initialization is `{−1,0.0,NULL}`. The old `height` name at +00 in coaster.c/coaster4.c/schoolcar.c is wrong; direction wiring in coaster5 and float comparisons in coaster6 settle the layout. [coaster5.c](../../LEGOLAND/coaster5.c), [coaster6.c](../../LEGOLAND/coaster6.c) |
| `JointSlot`, 0x14 | +00 candidate mask; +04 four signed-short x/y pairs. [coaster.c](../../LEGOLAND/coaster.c) |
| `TrackDesc`, 0x38 | +00 raised/anchor flag; +04 tail height h0; +08 head height h1; +0c/+14 two 8-byte joint parameter blocks initially `{0x0f,0}`; +1c draw, +20 build, +24 query, +28 place, +2c remove; +30 carries-path. Remaining final word not named. [coaster.c](../../LEGOLAND/coaster.c), [coaster6.c](../../LEGOLAND/coaster6.c) |
| `TrackFit`, 0x18; scratch `0x004d8250` | +00 flags, +04 owner, +08 head candidate index (−1 absent), +0c head geometry, +10 tail candidate index, +14 tail geometry. [coaster4.c](../../LEGOLAND/coaster4.c) |
| `CoasterRoute`, 0x15c | +00 state; +04 time started; +08 deadline; +0c 0x14-byte `RoutePos`; +20 dimension/context; +24 float parameter; +28 speed; +2c 0x40-byte physics interface; +6c owner; +70 embedded 0xec route-node sentinel (next at route +158). [schoolcar.c](../../LEGOLAND/schoolcar.c), [schoolcar5.c](../../LEGOLAND/schoolcar5.c), [schoolcar7.c](../../LEGOLAND/schoolcar7.c) |
| `RoutePos`, 0x14 | +00 track node; +04 render geometry object; +08 three-float world position. Its **saved** form is only 12 bytes: class/node key pair plus sub-object index. [schoolcar7.c](../../LEGOLAND/schoolcar7.c) |
| `RouteNode`, 0xec | +40 spacing; +44 route position; +78/+98 two 0x20-byte seats, occupant at seat +0c; +e4/+e8 previous/next links. `schoolcar.c`'s older +e4 “payload” guess is superseded by seat/node micro-helper evidence. [schoolcar8.c](../../LEGOLAND/schoolcar8.c), [codex-a.md](../lanes/codex-a.md) |
| `CoasterCar`, 0x28 | +00 state/broken-or-borrowed-model flag; +04 seat mesh; +08 seated-rider model; +0c visitor; +10/+14 list links; +18 seat association; +1c creation time; +20 draw hook; +24 destruction hook. State 1 waiting, 2 riding, 4 eviction candidate are list selection states; do not assume all uses of this flag word are a single boolean. [coaster5.c](../../LEGOLAND/coaster5.c), [schoolcar8.c](../../LEGOLAND/schoolcar8.c), [codex-a.md](../lanes/codex-a.md) |

The solver at route +2c has +00 set-state, +04 get-state, +08 derivative, +0c…+30 ten vector-operation hooks, +34 dimension, +38 state vector pointer and +3c route context pointer. A physical pool slot is `{int n; float values[20]}`, 0x54 bytes. The route's own two-component state starts at +20 (dimension), +24 (parameter), +28 (speed); general vector operations keep their n field. [schoolcar6.c](../../LEGOLAND/schoolcar6.c), [schoolcar8.c](../../LEGOLAND/schoolcar8.c)

Render records: a `TrackVtx` is 0x14 bytes `{int x,y,z,clip,shade}`. A 0x58 piece geometry has direction vectors +00/+0c, origin +18, profile/radius coefficients from +24, parameter interval +44/+48 and position/direction hooks +4c; fields are variant-dependent. `PolyJob` has kind/tag +00/+04, AND/OR clipping masks +08/+0c, flat value +10, dx1/dx2/dy1/dy2/area floats +14…24, four vertex slots +28…34, two-filler table pointer +38. `SpanEdge` is 0x30: short y bounds +00/+02, direction +04, five 16.16 attributes +08 and five steps +1c. [coaster3d.c](../../LEGOLAND/coaster3d.c), [schoolcar3.c](../../LEGOLAND/schoolcar3.c)

Z-buffer commands are variable length: +00 int n, +04 short final y, +06 short unnamed, then n eight-byte `{short x; short signed-y; int dx/dy-in-16.16}` entries at +08. Thus next command is `8+8*n` bytes later. Regions are 0x1c `{x0,y0,x1,y1,clipCode,link,next}` with next +18. [schoolcar5.c](../../LEGOLAND/schoolcar5.c), [schoolcar7.c](../../LEGOLAND/schoolcar7.c)

### Rules and state machines

A closed track is a circular list through tail-joint neighbours with the castle's node as sentinel. An open track is two null-terminated chains from its head/tail joints. Removing a closed-circuit piece reopens it and makes both adjacent ends free; removing an open piece is allowed only at the two outer ends. Both-end attachment closes the circuit and notifies the ride. The span constructor joins the two partner geometries to each other; the chain constructor joins each partner to the new piece. The castle's footprint chain appends each placed piece, allowing the whole track to follow a dragged castle. [coaster.c](../../LEGOLAND/coaster.c), [coaster4.c](../../LEGOLAND/coaster4.c), [coaster5.c](../../LEGOLAND/coaster5.c)

Fit first rejects an already-closed coaster and a square touching neither candidate end. Anchored pieces check each present end independently against their own endpoint heights and use the chain constructor. Unanchored pieces use the span constructor: when both partners exist, a combined span-height check runs; when either is absent, the function returns success after the geometric fit. This last branch corrects coaster5.c's prose claim that a span can only be placed across a two-ended gap. [coaster5.c](../../LEGOLAND/coaster5.c), [coaster4.c](../../LEGOLAND/coaster4.c)

An anchored/raised descriptor fixes a run endpoint's height. Between anchors, classify sloped joints using descriptor plus **direction** masks; count those joints as height-change capacity. `TrackJoinPieces` distributes total rise evenly across that count, raising z only across sloped spans and leveling intervening flats. A zero count has no rise to distribute. End fitting accepts any run with sloped capacity; an entirely flat run must already equal the proposed float endpoint height. [coaster5.c](../../LEGOLAND/coaster5.c), [coaster6.c](../../LEGOLAND/coaster6.c)

The editor publishes one ghost per legal joint candidate, head candidates first, from 0x1834-byte cursors at `0x0081ce00`, with mode 0x2032. Height-piece update rejects a failed fit with 0x0e. Height-plus-path rejects summed endpoint steps ≥2 with 0x0b, allowing one step. Flat track snaps within one cell on each axis, then confirms the snapped square against the created ghost list. [coaster.c](../../LEGOLAND/coaster.c)

Reset queries owner +10c for the station geometry (`0x006103a8`), parameter 0.1 and map-derived world position, seats the train, stamps time and sets state 8. Initial speed is the sum of route-node +c4 values plus the sum of +c0 values times `0.1*0.1*0.5`; those per-node coefficient meanings remain only partly named. Dispatch becomes true when the longest wait exceeds 5000 ticks or more than three cars wait. Each route node has two seats; free-seat search includes the sentinel. Car spacing is reconstructed from the previous car's position, stepping 30 world units with tolerance 4.8. [schoolcar7.c](../../LEGOLAND/schoolcar7.c), [schoolcar.c](../../LEGOLAND/schoolcar.c), [schoolcar8.c](../../LEGOLAND/schoolcar8.c)

Per-frame integration computes elapsed seconds from the game clock and clamps it to 0.8. State bit 8 uses the primary owner hook, bit 0x10 the secondary hook, otherwise free running. Each returns consumed time; accumulated time must reach `dt−1e−6`. Between partial steps the cursor advances through geometry, resetting the parameter to the next object's +44. Crossing a piece boundary changes 8→4; entering the castle sentinel after a lap changes 4→0x10. The primary/secondary hooks are owner +110/+114; the secondary wrapper saves/restores the derivative callback around its call. [schoolcar4.c](../../LEGOLAND/schoolcar4.c), [schoolcar8.c](../../LEGOLAND/schoolcar8.c)

Free motion subdivides dt into `ceil(dt*70)` steps. Each saves parameter and speed, integrates, and if it overshoots the object's end parameter, restores the train and speed and bisects time within that step. Every bisection probe starts from the same snapshot, with local solver time zero; stop when the bracket is within 0.001 seconds, then restore and integrate the final midpoint once more. This lands at the piece transition within time tolerance rather than extrapolating past it. [schoolcar5.c](../../LEGOLAND/schoolcar5.c), [schoolcar6.c](../../LEGOLAND/schoolcar6.c)

`Phys_Step` is classical RK4. It allocates four contiguous pool vectors: original state, accumulated result, current derivative increment and probe state. Initialize the accumulator with the starting state; each stage forms a probe from starting state plus the tableau node times the preceding derivative increment, evaluates the derivative at stage time, scales by dt, then adds the weighted increment. Install the final accumulated state and advance the solver clock. The 30-slot pool is a stack with a high-water mark, not a free list. [schoolcar6.c](../../LEGOLAND/schoolcar6.c), [schoolcar.c](../../LEGOLAND/schoolcar.c)

The 3D engine is separate from ordinary isometric sprites. View setup copies/inverts the base transform, maps screen centre back to map space, derives the eye from tile/view centres, and composes translation by minus eye. Geometry templates scale by 20 world units per map square. The mesh is a swept six-point ring with projected 0x14-byte vertices; lighting is the ring normal dotted with the segment basis/view coefficients plus ambient. Triangles reject back-facing area and inadequate combined low clip bits. Clip bits 1/2/4/8 mean inside left/right/top/bottom; 0xf0 means a vertex is inside a registered clip rectangle. [coaster3d.c](../../LEGOLAND/coaster3d.c), [schoolcar3.c](../../LEGOLAND/schoolcar3.c)

Polygon submission clips when vertex-mask AND is not 15, closes the vertex ring, computes fixed-point attribute gradients and splits it into two scanline-edge chains. High clip bits select whether z is interpolated and which filler runs. The downward edge arm carries all active attributes; the upward arm only carries x, an original asymmetry. Edges sort by top y. Z commands encode two chains by the sign of y and are filled into the module's 640×480 16-bit z buffer. Their producer must already order entries by scanline; the command decoder/filler does not sort them. The decoder has eight `ZKey` and eight `ZEdge` slots, stores absolute y as the key and sign as the chain selector, and initializes only the final edge's last-scanline field to `ylast−1`; the filler increments it to create its stopping sentinel. End-frame clears abandoned frames or flushes regions and commands, then rewinds the command buffer. [coaster3d.c](../../LEGOLAND/coaster3d.c), [schoolcar5.c](../../LEGOLAND/schoolcar5.c), [schoolcar6.c](../../LEGOLAND/schoolcar6.c), [schoolcar.c](../../LEGOLAND/schoolcar.c)

`MakeRotation` derives an orthonormal frame using world up: row 1 is world-up cross direction, row 2 is direction cross row 1, row 0 is direction, then normalize each. Track-profile tessellation begins with the two interval endpoints and recursively adds a parameter where the cubic deviates most from its straight chord by more than one world unit. Candidate extrema solve `3*c3*u²+2*c2*u+(c1−chordSlope)=0`; only roots within the interval count. The collected parameters are sorted ascending. Shared collector globals are curve pointers `0x004dd644/48`, write pointer `0x004dd64c`, count `0x004dd650`; initial best deviation is FLT_MIN (`0x00800000`). [coaster6.c](../../LEGOLAND/coaster6.c), [schoolcar4.c](../../LEGOLAND/schoolcar4.c), [schoolcar5.c](../../LEGOLAND/schoolcar5.c)

Support draw samples the middle of a piece and orders its solid and ground-shadow model according to basis row 2 against the light. Shadow builds twelve vertices: four ground quad points, four raw-position ground points at z=0, four points lifted to the plane through the support origin. Model rendering receives identity basis because these are already world coordinates. Train rider construction copies a shared seated model template, substitutes two colour families and chest/face part names from the 0x38-byte appearance block, then owns the instance separately. [coaster4.c](../../LEGOLAND/coaster4.c), [coaster6.c](../../LEGOLAND/coaster6.c)

The save blob begins with size +00, count +08, 8-byte node/link array pointer +0c, car-state count/pointer +10/+14, extra count/pointer +18/+1c. Internal pointers become blob-relative offsets. Node references are class id plus packed square; route state records are 0x20 bytes containing raw route state, parameter/speed words, elapsed/remaining time, then a 12-byte saved route position. Restore rebases timers against the current clock. A zero blob length returns sentinel −1, not null. Route position stores the index into the render-object chain at object +50; world position is recomputed on load. [coaster.c](../../LEGOLAND/coaster.c), [schoolcar.c](../../LEGOLAND/schoolcar.c), [schoolcar7.c](../../LEGOLAND/schoolcar7.c)

The archive loads parallel `.lms` mesh, `.lfm` template and `.ltx` image data. LMS fixup rebases pointer fields +0c,+10,+14,+18,+20,+28 within the loaded block; the underlying payload is still opaque. Model-list loading reads both `<name>.obj` and `<name>.txt`, fails on a short whole-file read, and releases the first image if the second fails. [schoolcar7.c](../../LEGOLAND/schoolcar7.c), [schoolcar4.c](../../LEGOLAND/schoolcar4.c), [schoolcar5.c](../../LEGOLAND/schoolcar5.c)

### Tables and constants

The vector table at `0x004dcbd0` is fully decoded. Relative solver offsets are table offset +0x0c; dimension follows ten entries. [schoolcar6.c](../../LEGOLAND/schoolcar6.c), [schoolcar8.c](../../LEGOLAND/schoolcar8.c), [codex-a.md](../lanes/codex-a.md)

| Slot | Address / function | Operation |
| ---: | --- | --- |
| 0 | `0x004212a0 PhysVec_Add` | out=a+b |
| 1 | `0x004212e0 PhysVec_Subtract` | out=a−b |
| 2 | `0x00421320 PhysVec_Copy` | Copy all 0x54 bytes, including inactive components |
| 3 | `0x00421340 PhysVec_Scale` | v*=k |
| 4 | `0x00421360 PhysVec_MaxAbs` | Maximum absolute component |
| 5 | `0x004213a0 PhysVec_ScaleAdd2` | out=ka*a+kb*b |
| 6 | `0x00421400 PhysVec_Zero` | Set n and clear positive-length components |
| 7 | `0x00421430 PhysVec_AddScaled` | out+=k*a |
| 8 | `0x004214f0 CarPool_Alloc` | Allocate contiguous stack slots |
| 9 | `0x00421510 CarPool_Free` | Rewind stack slots |

| Table/constant | Decoded content |
| --- | --- |
| RK4 `0x004b5660`, four 8-byte float pairs | Nodes `[0,1/2,1/2,1]`, weights `[1/6,1/3,1/3,1/6]`. [schoolcar6.c](../../LEGOLAND/schoolcar6.c) |
| Vector pool | 30×0x54 at `0x004dcc00`, pointers `0x0082ac60`, high-water mark `0x004dd5d8`. [schoolcar.c](../../LEGOLAND/schoolcar.c) |
| Ring/topology | Six points 60° apart, radius 1.8, x=0; outward 2D normals. Up to 30 rings; 24 index pairs and 12 triangles per repeated segment. Pair references use bit 31 to choose a half; seam encoding closes ring wraps. Positions `0x006121c8`, normals `0x006126d8`, triangles `0x00612708`, pairs `0x006148b8`, projected vertices `0x006139c8`. [schoolcar3.c](../../LEGOLAND/schoolcar3.c) |
| Track draw order `0x00611710` | Direction-pair→mode: 1→4=2, 4→1=1; 2→8=2, 8→2=1; 1→2=1, 2→1=2; 2→4=2, 4→2=1; 4→8=1, 8→4=2; 8→1=1, 1→8=2. These are direction bits at joint +00, correcting the “heights” label in the older comment. [coaster4.c](../../LEGOLAND/coaster4.c), [coaster6.c](../../LEGOLAND/coaster6.c) |
| View template rows | `[1.60334,−1.60334,0,0]`; `[0.801688,0.801688,1.96416,0]`; `[3.19995,3.19995,0,32767.5]`; `[0,0,0,1]`. Base-view selected components are halved. [coaster4.c](../../LEGOLAND/coaster4.c) |
| Z buffer / commands | `0x004e3870`, 640×480×2=614400 bytes; command storage `0x004dd870`, write pointer `0x004b5b3c`. [schoolcar.c](../../LEGOLAND/schoolcar.c) |
| Colour ramps | One 64-entry u16 ramp (128 bytes) per 24-bit RGB colour; 1024 ramp pointers at `0x00829c60`, allocation base `0x00829c54`, white fallback `0x00579878`. [schoolcar5.c](../../LEGOLAND/schoolcar5.c) |
| Shade ramp construction | Entries 0…32 interpolate black→colour by component/32; entries 32…63 colour→maximum by `(max−component)/31`. Entry 32 written twice. Format selector 2 uses 5/6/5 (red shift 11); otherwise 5/5/5 (red shift 10), green shift 5. Floats 0.03125 and 0.032258064 are the recorded reciprocals. [schoolcar6.c](../../LEGOLAND/schoolcar6.c) |
| Shade clamp | 192-byte biased lookup maps input −64…127 to 0…63 by saturation. [schoolcar8.c](../../LEGOLAND/schoolcar8.c) |
| Seat models | Sex 0 `sit.lomansit`, sex 1 `sit.logirlsit`; appearance +04/+08 leg/arm RGB, +0c/+20 two 20-byte part names, +34 visitor. [coaster5.c](../../LEGOLAND/coaster5.c), [coaster6.c](../../LEGOLAND/coaster6.c) |

Track geometry variants use line x/y=`origin+t*direction`, constant z; cubic z=`((c3*t+c2)*t+c1)*t+c0` with derivative `(3*c3*t+2*c2)*t+c1`; arc x/y is the two basis vectors weighted by cos/sin plus centre, with horizontal tangent. Off-centre rail hooks use their own radii +24/+28. Preserve existing quarter-turn float bit patterns if matching the binary's arithmetic. [schoolcar4.c](../../LEGOLAND/schoolcar4.c), [schoolcar7.c](../../LEGOLAND/schoolcar7.c), [schoolcar8.c](../../LEGOLAND/schoolcar8.c)

Fast-root setup fills 64 mantissa approximation pairs and 256 exponent factors. Sqrt pair is `{v/2,1/(2v)}`; reciprocal-sqrt pair `{3/(2v),−1/(2v³)}` for `v=sqrt(1+i/64)`, i=0…63. Sqrt exponent is `sqrt(2^(i−127))`; reciprocal factor is its reciprocal except index 0 explicitly 1. These are generated tables, not guessed constant lists; source addresses are sqrt pairs `0x00610c40`, factors `0x00610e44`, reciprocal pairs `0x00610a20`, factors `0x00611244`. [coaster5.c](../../LEGOLAND/coaster5.c)

### Original bugs and edge behaviour

- `RemoveTrackNode` reads owner before testing its argument. Node search compares the sentinel's meaningless square before ordinary pieces. [coaster.c](../../LEGOLAND/coaster.c)
- Raster gradient clamp stores the out-of-range value first, then zeroes a dead temporary; values outside ±0x200000 reach the filler unchanged. [coaster3d.c](../../LEGOLAND/coaster3d.c)
- Support shadow “5-unit snapping” casts the coordinate before multiplying by 0.2 and 5, so it truncates to whole units instead. The first four shadow templates copy the same source x into both x and y. [coaster4.c](../../LEGOLAND/coaster4.c), [schoolcar8.c](../../LEGOLAND/schoolcar8.c)
- Car construction overwrites its first +08 texture result with the seated-rider model result; both loaders still run. Piece owner is redundantly assigned by constructor and joint wiring. Curve initialization retains a degenerate branch, and root-table setup retains a compared-constant self-assignment. These have no identified gameplay effect but are original operations. [coaster5.c](../../LEGOLAND/coaster5.c), [coaster4.c](../../LEGOLAND/coaster4.c), [coaster3d.c](../../LEGOLAND/coaster3d.c)
- An empty/nonpositive z command writes the final-edge field below edge[0], into key scratch. The producer is assumed never to emit such a command; fixed eight-entry arrays are not a dynamic capacity guarantee. A vertical direction makes the world-up cross products in `MakeRotation` degenerate, with no special handling. [schoolcar5.c](../../LEGOLAND/schoolcar5.c), [coaster6.c](../../LEGOLAND/coaster6.c)
- Saving a route object absent from its piece's +50 chain writes the chain length without reporting failure. [schoolcar7.c](../../LEGOLAND/schoolcar7.c)
- Fast roots use ST(0) input/output and unmasked `bits>>23`, including sign, as exponent index; their assembly writes scratch registers below ESP without reserving stack. Unknown classes/directions return 0, also a valid first index. Vector operations do not verify the second vector's dimension; copy includes unused values and zero retains nonpositive n while clearing none. [coastermath.c](../../LEGOLAND/coastermath.c), [schoolcar8.c](../../LEGOLAND/schoolcar8.c), [codex-a.md](../lanes/codex-a.md)

### Callback roles

Track's five descriptor hooks (+1c draw, +20 build, +24 query, +28 place, +2c remove) are internal polymorphic geometry operations, separate from ObjDef. Flat/height/path-height ObjDef update functions are `Track_Update`, `TrackH_Update`, `TrackHP_Update`; `Track_Update90` is the older misleading name for flat update. Car draw/kill hooks are +20/+24. Route owner primary/secondary step hooks are +110/+114 and the solver has its own three system callbacks plus ten vector callbacks. [coaster.c](../../LEGOLAND/coaster.c), [coaster5.c](../../LEGOLAND/coaster5.c), [schoolcar6.c](../../LEGOLAND/schoolcar6.c), [schoolcar8.c](../../LEGOLAND/schoolcar8.c)

## 6. Ancillary ride mechanics in the transport sources

### Data structures

`CarouselRec` is 0x24 bytes: +00 next, +04 square, +06 boarded, +07 aboard, +08 signed frame, +0c flags, +10 revolutions, +14 half-rate counter, +18 expected visitors, +1c idle timer, +20 four seat bytes. Rider seat +36 is one-based; pose +35=1 selects riding placement. [bswater.c](../../LEGOLAND/bswater.c)

`TowerRec` is 0xb4 bytes with square +00, seated/riders/joined bytes +02/+03/+04, next +08, four 36-byte cars at +14…a3, eight seat bytes +a4, signed animation frames +ac/+ad and timer +b0. Each car has flags +00 (bit 0 in service), lift height +08, state +0c, reset field +14, rider nodes +18/+1c; +04/+10/+20 remain unnamed. Head is `0x0062fda8`. [bswater2.c](../../LEGOLAND/bswater2.c), [bswater3.c](../../LEGOLAND/bswater3.c)

Restaurant2's 0x40-byte record has next +00, square +04, four animation/dwell bytes +06…09, walking +0c, queued/seated bytes +10/+11, ready +14, phase +18, idle +1c, six activity counters +20…34 and waiter x/y +38/+3c. Plane's local removal view has square +00 and next +20; the full flight record is elsewhere. [bswater2.c](../../LEGOLAND/bswater2.c)

### Rules and state machines

Carousel running increments the frame every second tick, wraps at 64 and decrements revolutions. At zero revolutions it waits for all riders to leave before stopping. Boarding flag 0x4000 waits until boarded equals expected visitors, then starts the ride; idle timer zero closes admission and enters boarding. Each seated pose-1 rider uses `BlokeBox%02d` with one-based seat and the platform frame; the z sprite is locked to that frame. [bswater.c](../../LEGOLAND/bswater.c)

Tower in-service state is **derived from occupied seats on each count**: clear all four car service/state fields, then for each occupied seat mark car `seatIndex>>1` in service with state 2 and reset height/+14. Drawing order is cars 1,2, tower body, cars 0,3. Car body always draws; in-service cars add depth placement, their two 3D riders and foreground matte. Queue movement sums all animation translations through the current part/frame and adds the tower base. Each 16-byte step has whole dx/dy and 1/256 dx/dy; the accumulated displacement is 24.8. [bswater3.c](../../LEGOLAND/bswater3.c), [bswater2.c](../../LEGOLAND/bswater2.c)

Restaurant2 allocation zeroes the whole record then initializes it again, with only station square and waiter x=0x143 nonzero. Plane removal unlinks, fades sound and frees. [bswater2.c](../../LEGOLAND/bswater2.c)

### Tables and constants

Tower car body sprite layers for car 0…3 are `[6,4,0,2]`; cached anchors are at `0x0062fd88`. Matte table at `0x0062fd64` is `[NULL,"SpaceTower Seat2 Matte.lls","SpaceTower Seat3 Matte.lls",NULL]`. Names `g_spacetower_state` at that address and `g_spacetower_phase` at +0c in older mechanical-ride sources are incorrect: they are table elements 0 and 3. Cars 0/3 use outer map offsets and 1/2 inner offsets, two squares from their reference. [bswater3.c](../../LEGOLAND/bswater3.c)

Carousel uses running mask 1, boarding mask 0x4000, 64 platform frames, divisor 2 and four seats. Its BNV placement depth arguments are −1617853.25 and −1618109.0. [bswater.c](../../LEGOLAND/bswater.c)

### Original bugs and edge behaviour

Plane and tower remove functions both dereference an empty list head before checking it. Tower body draw has no default for an invalid car index, using an uninitialized layer that shares the cached y-offset's stack slot. Tower depth placement retains an otherwise unused `Get_XScroll()` call after eliminating its x arithmetic. The queue-step helper ignores its supplied part parameter and derives it from the rider again. [bswater2.c](../../LEGOLAND/bswater2.c), [bswater3.c](../../LEGOLAND/bswater3.c)

### Callback roles

`Carousel_TickInstance`, `Restaurant2_NewRecord`, `PlaneRide_RemoveRecord`, `SpaceTower_CountSeated`, `SpaceTower_DrawCar`, `SpaceTower_DrawCarUnder/Over`, `SpaceTower_PlaceCar`, `SpaceTower_StepAnim` and `Anim3D_OffsetAt` are helper functions reached from the attractions' ObjDef handlers. Their placement in water source files does not imply shared boat state or callback registration. [bswater.c](../../LEGOLAND/bswater.c), [bswater2.c](../../LEGOLAND/bswater2.c), [bswater3.c](../../LEGOLAND/bswater3.c)

## Remaining implementation boundaries

This page gives the decoded road table, flume paths/masks/timeline, physics tableau/vector interface, car/tower slot maps and recovered state machines. A renderer still needs the external water tile rows, boat arc-table rows, flume overlay offsets, coaster model/template data, class footprints and some descriptor masks. The C declarations establish their locations/strides without publishing every byte; no missing values have been invented. [bswater.c](../../LEGOLAND/bswater.c), [roads.c](../../LEGOLAND/roads.c), [logflume.c](../../LEGOLAND/logflume.c), [coaster3d.c](../../LEGOLAND/coaster3d.c)

Exact LFPath endpoint reachability, overloaded coaster-car state meanings, the raised piece's overlapping footprint/cached fields and unnamed parts of the 0xa4 allocation deserve runtime tests or further reverse engineering. Existing WIP markers concern matching evidence and must not be interpreted as proof that a recovered behavioural claim was executed here; this documentation pass ran no compiler or gameplay tests. [logflume6.c](../../LEGOLAND/logflume6.c), [logflume7.c](../../LEGOLAND/logflume7.c), [schoolcar.c](../../LEGOLAND/schoolcar.c), [coaster4.c](../../LEGOLAND/coaster4.c), [coaster5.c](../../LEGOLAND/coaster5.c)
