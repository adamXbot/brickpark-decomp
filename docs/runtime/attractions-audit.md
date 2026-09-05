# Attraction documentation completeness audit

This audit compares the 27 assigned attraction source files with [attractions.md](attractions.md). Completion means every recoverable runtime contract in the reviewed headers and behavioural/WIP notes is represented, corrected against current consumers, or explicitly recorded as an upstream gap. It does not mean every original function or asset has been recovered. [Scope J brief](../SCOPE_J_runtime_spec.md)

## Stage plan and pass conditions

1. Inventory all 27 headers, block comments and WIP markers; inspect runtime descriptions, decoded constants and original-bug notes. Pass when every assigned source has a reviewed coverage row.
2. Reconcile the draft with those sources and trace disputed fields through allocation, indexing and state transitions. Pass when each actionable omission/disagreement has a recorded resolution or an explicit evidence boundary.
3. Validate both Markdown files: all local links resolve, every assigned source is cited, subsystem sections have the required order, and no factual prose block lacks a citation. Pass when the checks below report no failures. [Scope J brief](../SCOPE_J_runtime_spec.md)

## Audit results

The documentation pass is complete for the available evidence: all 27 assigned source headers, runtime/bug comments and 18 `WIP-FUNCTION` markers were reviewed. The 27 files contain 35,730 source lines. Matching residuals that concern register allocation or instruction scheduling remain decompilation history; runtime facts inside those notes are included. Source-level gaps remain labelled in [attractions.md](attractions.md), with transport details linked to [transport.md](transport.md). This satisfies the documentation boundary without treating unavailable assets or helper bodies as recovered. [Scope J brief](../SCOPE_J_runtime_spec.md), [parallel contract](../PARALLEL_CONTRACT.md)

### Source coverage ledger

Every row is reviewed. “External” identifies missing evidence in the current source, rather than unfinished documentation. WIP counts are literal markers in the cited file; they do not measure behavioural completeness. [attractions.md](attractions.md)

| Assigned source | WIP markers | Covered contract and remaining evidence boundary |
| --- | --- | --- |
| [castleobj.c](../../LEGOLAND/castleobj.c) | 0 | Six-class dispatch, castle placement/removal, coaster adapter/save sentinel, driving-school rider scripts; graph/physics details cross-linked to transport. |
| [catapult.c](../../LEGOLAND/catapult.c) | 0 | Record layout, launch/flight/landing stages, seat geometry, payload-copy save and removal order; arm-layer and landing-offset bytes external. |
| [goldrush.c](../../LEGOLAND/goldrush.c) | 1 | Gold/Fort/CastleLevel1/Temple customer scripts, pan occupancy and target arithmetic, Restaurant2 overlay layers, school-car integration. |
| [goldrush2.c](../../LEGOLAND/goldrush2.c) | 0 | Gold pose transforms, all sixteen decoded facing/rotation rows and path offsets; lane notes used to resolve helper roles. |
| [goldrush3.c](../../LEGOLAND/goldrush3.c) | 1 | Kneel/stand arithmetic, admission and traffic-light claim behavior, Restaurant2 entry/exit movement. |
| [goldrush4.c](../../LEGOLAND/goldrush4.c) | 0 | Gold render cuts including equal-boundary duplication, school-car stop rollback and straight maneuver. |
| [interfaces.c](../../LEGOLAND/interfaces.c) | 0 | Provider slot roles, resource installation sequence and legacy callback-name reconciliation; full class matrix is shared Scope J documentation. |
| [joust.c](../../LEGOLAND/joust.c) | 1 | Joust spectator/jouster routes and counters, Temple Slide offered lanes, paths and exit routes; walk/end threshold bytes external. |
| [joust2.c](../../LEGOLAND/joust2.c) | 1 | Joust cycle/freeze/FX fields, Copters layers, balloon platform selection, lake-arm test and mechanical helper contracts. |
| [mechrides.c](../../LEGOLAND/mechrides.c) | 5 | Six rider machines, layouts, resources, render state changes, decoded limit tables and faults; several machine/allocator bodies and table values external. Tower helpers reconciled with transport sources. |
| [ridecb1.c](../../LEGOLAND/ridecb1.c) | 0 | Carousel/Balloonz/EarthSlide layers, Entrance1 directions/payment, ChuckWagon customers and staff rendering; all described fixed limits and array faults recorded. |
| [ridecb2.c](../../LEGOLAND/ridecb2.c) | 1 | Jungle station rider stages, boat admission, queue behavior and save/load fixups; route and vehicle details shared with transport. |
| [ridecb3.c](../../LEGOLAND/ridecb3.c) | 2 | Carousel/Balloonz machines and poses, restaurant counters, waiter phases and off-by-one reads; external waiter deltas and some seat coordinates identified. |
| [ridecb4.c](../../LEGOLAND/ridecb4.c) | 0 | Restaurant overlays, all 32 cafe IDs mapped to eight tables, seat allocation and painter masks; cafe positions/orientations/depth bytes external. |
| [ridecb5.c](../../LEGOLAND/ridecb5.c) | 3 | Boating station placement/customer stages and road/diagonal callbacks; boating queue coordinates external and route data linked to transport. |
| [ridecb6.c](../../LEGOLAND/ridecb6.c) | 0 | Jungle/water/monkey helper callbacks, driving save order and count-loader failures; transport owns route expansion. |
| [ridecb7.c](../../LEGOLAND/ridecb7.c) | 0 | Boating save/removal, food-cart draw/resources and mechanics-hut scripts including firing leak. |
| [ridecb8.c](../../LEGOLAND/ridecb8.c) | 0 | Carousel/Balloonz placement, food/scenery callbacks, Brolly descriptor, Dragon BBQ sound, boating loader append behavior. |
| [ridecb9.c](../../LEGOLAND/ridecb9.c) | 1 | Jungle construction/water helpers, Potting Shed stages and leak, garden selection/fallback; transport receives route details. |
| [ridemisc.c](../../LEGOLAND/ridemisc.c) | 0 | Favourite-selection faults, instance/admission helpers, queues and Copters contracts. |
| [ridemisc2.c](../../LEGOLAND/ridemisc2.c) | 0 | Restaurant1 five decoded waypoint rows, seat/queue helpers and Jungle three-seat ellipse formula; remaining ten Restaurant1 rows external. |
| [ridemisc3.c](../../LEGOLAND/ridemisc3.c) | 1 | Shared removal faults, Carousel stop, Balloonz record, Restaurant2 movement and petrol-pump helper; extern consumers retain named boundaries. |
| [rides.c](../../LEGOLAND/rides.c) | 0 | Shared records, visitor score/enjoyment/initialization, membership/admission, worker insertion and immediate release helper. |
| [ridesave.c](../../LEGOLAND/ridesave.c) | 0 | Ordinary chunk layouts, one-based rider references, independent BNV indices, live Tower-pointer corruption and loader replacement/leaks. |
| [waterworks.c](../../LEGOLAND/waterworks.c) | 1 | Water Works rectangular triggers, delayed sounds, garden/elephant and staff behavior, record layouts and original faults. |
| [westtown.c](../../LEGOLAND/westtown.c) | 0 | Town door records, jail/Shop1 drawing, self-paving/resource behavior and loader arming bug. |
| [westtown2.c](../../LEGOLAND/westtown2.c) | 0 | All nine Western Town scripts, waypoints/actions/door stages, including generated jitter and freeze behavior. |

### Reconciliations and recovered omissions

| Issue found during audit | Resolution written into the specification |
| --- | --- |
| Apparent Spider/Plane/Barrels seat overlap | Physical first slots and release displacements imply valid seat1 at physical slot0. The preceding timer/frame is reached only by invalid ID0; external allocator bodies remain unavailable. [mechrides.c](../../LEGOLAND/mechrides.c) |
| Tower layout and admission | `0xb4` record, four `0x24` cars at `+14`, occupancy at `+a4`; serializer's biased `+24` view reaches the same eight rider pointers. In-service flags are derived from occupancy, and current rider join timer is200. [mechrides.c](../../LEGOLAND/mechrides.c), [bswater2.c](../../LEGOLAND/bswater2.c), [bswater3.c](../../LEGOLAND/bswater3.c), [ridesave.c](../../LEGOLAND/ridesave.c) |
| Gold “48 south” wording | Edge target is `P+128`, kneel is `P-80`; standing after reaching kneel changes world to `P+48`, then targets `P+128`. This is an 80-unit return target, not a proven permanent 48-unit drift from the edge. State7 completion/snap semantics remain outside the recovered consumer evidence. [goldrush.c](../../LEGOLAND/goldrush.c), [goldrush3.c](../../LEGOLAND/goldrush3.c), [bnvmove.c](../../LEGOLAND/bnvmove.c) |
| Missing Gold details | Added CastleLevel1's complete seven-stage route, independently drawn x/y jitter, and render cuts with equality duplication. [goldrush.c](../../LEGOLAND/goldrush.c), [goldrush4.c](../../LEGOLAND/goldrush4.c) |
| Temple Slide's nominal four seats | Allocation offers only0 and3, falls back to0 if both occupied, yet computes full across all four. Missing-record `-1` can become seat255. Exact entry/exit stages and threshold boundaries are documented. [joust.c](../../LEGOLAND/joust.c) |
| Joust boolean, timing and missing placement | The helper tests the whole byte for nonzero; post-increment `>150` means152 idle visits from0. Added spectator/jouster coordinates and retained the whole-tick freeze on a missing placement. [joust.c](../../LEGOLAND/joust.c), [joust2.c](../../LEGOLAND/joust2.c) |
| Carousel wait claim | Zero revolutions calls a helper that always returns1 and advances nonqueued riders, then stops immediately; it does not wait for physical disembarkation. Added four-seat `0x24` layout, full machine transitions, frame/depth/path values. [bswater.c](../../LEGOLAND/bswater.c), [rides.c](../../LEGOLAND/rides.c) |
| Balloonz frame and holding conditions | BNV uses the copied wheel plus half-cycle, while z-frame is separately written back. Car2 alone does not hold the platform. Loader sets the ObjDef bit and never arms the build sprite. [ridecb3.c](../../LEGOLAND/ridecb3.c), [screencb2.c](../../LEGOLAND/screencb2.c) |
| Restaurant2 counters and waiter | Added full waypoints/gates, post-decrement queue underflow, meal timer order, step33/34 access facts and forward-only coordinate accumulation. External adjoining bytes cannot establish whether the overrun was intended sharing. [ridecb3.c](../../LEGOLAND/ridecb3.c), [ridemisc3.c](../../LEGOLAND/ridemisc3.c) |
| Cafe seat count and ordering | There are32 seat IDs,16 paired geometry entries and16 chair masks; all eight four-seat table mappings and masks are recorded. Teardown deletes25 sprites but retains stale pointers. [ridecb4.c](../../LEGOLAND/ridecb4.c) |
| Missing small helper contracts | Added Potting Shed target-relative movements/firing leak, Brolly prefix/offset/variant rules, food-draw customer selection, Jungle ellipse and queues, Boating/Jungle rider stages, Catapult's discarded sprite lookup. [ridecb9.c](../../LEGOLAND/ridecb9.c), [ridecb8.c](../../LEGOLAND/ridecb8.c), [ridecb1.c](../../LEGOLAND/ridecb1.c), [ridecb7.c](../../LEGOLAND/ridecb7.c), [ridemisc2.c](../../LEGOLAND/ridemisc2.c), [ridecb2.c](../../LEGOLAND/ridecb2.c), [ridecb5.c](../../LEGOLAND/ridecb5.c), [catapult.c](../../LEGOLAND/catapult.c) |
| Saved “sample”/“instance” fields | Resolved to BNV path and RiderNode/Person3D fields. Rider positions are one-based, z-index0 is absent, but BNV index0 is valid RUN (`0/1/2=RUN/ON/OFF`). Ordinary empty loads retain the old head; nonempty loads replace/leak it. [ridesave.c](../../LEGOLAND/ridesave.c), [mechrides.c](../../LEGOLAND/mechrides.c) |

### Verification

The static validation checks both documents' local Markdown destinations and heading anchors, all27 assigned source citations, trailing whitespace and prose-block citations. It checks the13 attraction subsystems each retain exactly the required order: structures, rules, tables, bugs, callback roles. The main specification contains394 links; every link and all13 section-order checks pass. Seven independent arithmetic checks pass: one-based seat displacement; pan offsets; Tower runtime/serializer rider-pointer equivalence; Restaurant2 x[33]/y[0] address alias; cafe seat IDs covering0..31 exactly; Joust's152-visit post-increment; Balloonz's six platform values. [attractions.md](attractions.md)

No compilation or C, tools, shared-document, commit or push mutation was performed by this audit. This is a documentation/evidence check under Scope J's no-code boundary. [Scope J brief](../SCOPE_J_runtime_spec.md), [parallel contract](../PARALLEL_CONTRACT.md)

### Meaningful remaining limits

The current source still lacks literal cafe position/orientation/depth rows, Restaurant2 waiter deltas, ten Restaurant1 waypoint rows, Catapult layer/landing offsets, Temple Slide walk/end thresholds, Tower seat/direction rows and some mechanical timing/seat tables. Their addresses, shapes and consumer behavior are retained wherever known. Original BNV/sprite assets and external allocator/machine bodies are also required for a complete replacement simulation; no values have been invented to close these gaps. [ridecb4.c](../../LEGOLAND/ridecb4.c), [ridecb3.c](../../LEGOLAND/ridecb3.c), [ridemisc2.c](../../LEGOLAND/ridemisc2.c), [catapult.c](../../LEGOLAND/catapult.c), [joust.c](../../LEGOLAND/joust.c), [mechrides.c](../../LEGOLAND/mechrides.c)

The Gold pan arithmetic and Restaurant2 adjacent-table reads are fully specified as current source operations. Their larger effects remain qualified: no recovered state7 arrival consumer proves a snap to the Gold target, and absent waiter table bytes do not prove intentional data sharing. These boundaries are substantive source unknowns, not deferred documentation work. [goldrush.c](../../LEGOLAND/goldrush.c), [goldrush3.c](../../LEGOLAND/goldrush3.c), [bnvmove.c](../../LEGOLAND/bnvmove.c), [ridecb3.c](../../LEGOLAND/ridecb3.c), [ridemisc3.c](../../LEGOLAND/ridemisc3.c)
