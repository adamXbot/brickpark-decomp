# Transport documentation completeness audit

This audit checks the 30-source transport inventory against [transport.md](transport.md), using the requirements in [Scope J](../SCOPE_J_runtime_spec.md). It is a documentation audit, not a compiler audit or an execution test.

## Stage plan and pass conditions

1. **Inventory evidence:** read every assigned source header, recovered data declaration, original-defect comment and behavioural note adjacent to WIP functions; record a per-source evidence row. Pass when the evidence rows exactly equal the 30-source inventory, with no duplicates.
2. **Close recoverable omissions:** compare layouts, rules, decoded tables, original defects and callback roles with the runtime page; add missing recovered material and record each resolution below. Pass when every requirement has either a runtime section or a specific source limitation.
3. **Verify and review:** check local Markdown links and anchors, exact inventory coverage, section order and whitespace; review uncertain claims against source operations. Pass when mechanical checks succeed and remaining limitations are explicitly identified.

Parallelism is at the parent Scope J task level: other agents audit the attraction and presentation groups while this agent owns transport. No C files or tools are edited; no compiler or game execution is part of these checks.

## Evidence inventory

All 30 assigned source headers were read, their recovered table declarations inventoried, and behaviour-bearing comments at the 31 `WIP-FUNCTION` markers compared with the runtime page. The rows below are the exact transport inventory, once each. “Closed” means the available evidence is documented; a remaining source limitation is not deferred prose or a claim of binary matching.

| Source | Recovered evidence covered in transport.md | WIP markers | Remaining source limitation |
| --- | --- | ---: | --- |
| [bswater.c](../../LEGOLAND/bswater.c) | §1 launch/lake flood/painting, §6 carousel state/placement and defects | 1 | 16×25 artwork bytes external |
| [bswater2.c](../../LEGOLAND/bswater2.c) | §1 reservation/preferences/docking/DFS, §6 restaurant/plane/tower-animation views | 0 | Closed |
| [bswater3.c](../../LEGOLAND/bswater3.c) | §1 80-frame animation, §6 tower occupancy/draw slots/mattes/placement defects | 1 | Step/arc rows and asset-derived anchors external |
| [coaster.c](../../LEGOLAND/coaster.c) | §4 spawning/light tables, §5 graph/editor/save relocation/piece prefix and null-read defect | 0 | Some descriptor values and allocated piece tail unnamed |
| [coaster3d.c](../../LEGOLAND/coaster3d.c) | §5 view/geometry/projection/raster records, gradient-clamp defect, clip predicates | 3 | Raw source geometry/model arrays external; extent-tool caveat below |
| [coaster4.c](../../LEGOLAND/coaster4.c) | §5 constructors/view constants/draw ordering/save walk/support shadows | 1 | Raw support models external; footprint/cache overlay not fully typed |
| [coaster5.c](../../LEGOLAND/coaster5.c) | §5 fit/joint wiring/height profiling/car creation/fast-root construction | 0 | Referenced class masks external |
| [coaster6.c](../../LEGOLAND/coaster6.c) | §5 run endpoints/basis/seat substitution schema and default colours/names | 1 | Full model payload external |
| [coastermath.c](../../LEGOLAND/coastermath.c) | §5 matrix/vector/clip/joint helpers, root ABI/index edge cases | 1 | Extent-tool caveat below; no missing recovered math |
| [jcroute.c](../../LEGOLAND/jcroute.c) | §2 disjoint DFS/BFS fields, success flag, ownership and re-enqueued seed | 1 | Closed |
| [junglecruise.c](../../LEGOLAND/junglecruise.c) | §2 boat/deco layouts, water ownership, draw ordering, assignment/null defects | 2 | Water artwork external; seat offsets resolved through ridemisc2.c; larger deco allocation unknown |
| [lfentrance.c](../../LEGOLAND/lfentrance.c) | §3 entrance pieces/previews/queue/action timeline/layered drawing/uninitialized frame | 2 | Closed; geometry consumes class footprint data |
| [lfmisc.c](../../LEGOLAND/lfmisc.c) | §1 unlink, §3 queue/ordinals/drop helpers, §6 names/walk/person/pump teardown | 0 | Closed; local access views do not establish complete object sizes |
| [logflume.c](../../LEGOLAND/logflume.c) | §3 class shims, allocation/placement, ten sprite names, overlays/save/demolition defects | 1 | Overlay offsets and raw shape values external |
| [logflume2.c](../../LEGOLAND/logflume2.c) | §3 route links/probes/masks/drop expansion/packed references/unlink/0x41 defect | 1 | Class geometry rectangles and shape table values external |
| [logflume3.c](../../LEGOLAND/logflume3.c) | §3 tunnel/CSAW/hold-up/corner floor plans, ownership and pre-guard read | 2 | Closed; dimensions intentionally expressed from ODF footprints |
| [logflume4.c](../../LEGOLAND/logflume4.c) | §3 run clock/queue, all six paths, alternate draw buckets/order/mode inconsistency | 1 | Closed |
| [logflume5.c](../../LEGOLAND/logflume5.c) | §3 piece-tree/queue load, boat-step priority, invalid join orientation | 0 | Closed |
| [logflume6.c](../../LEGOLAND/logflume6.c) | §3 queue save, spacing/advance/drop timeline/draw and uninitialized outputs | 0 | Exact endpoint reachability not established |
| [logflume7.c](../../LEGOLAND/logflume7.c) | §3 waiting predicate, heading/shape mapping, polyline interpolation | 0 | Source's unreachable-endpoint claim not proved by callers |
| [roads.c](../../LEGOLAND/roads.c) | §1 boat-animation formulas, §4 all 60 road slots, rotation/zebra stamping/null writes | 1 | Raw boat step/arc records external |
| [roads2.c](../../LEGOLAND/roads2.c) | §4 road record/one-way entrance/frontier/predecessor rules | 0 | Closed |
| [schoolcar.c](../../LEGOLAND/schoolcar.c) | §4 steering/lifecycle/free-before-unlink, §5 pool/route/car/save/three geometry interfaces | 0 | Some route coefficients unnamed; model tables external |
| [schoolcar2.c](../../LEGOLAND/schoolcar2.c) | §4 tight/wide turns, heading rotation, chooser mask probabilities | 2 | Closed |
| [schoolcar3.c](../../LEGOLAND/schoolcar3.c) | §4 pull-off, §5 mesh shading/projection, 24 seam pairs and 12 triangle templates | 3 | Closed; matching residuals are not omitted geometry |
| [schoolcar4.c](../../LEGOLAND/schoolcar4.c) | §4 queue overread/obstruction, §5 elapsed clamp/lap states/cubic tessellation/model input | 2 | Closed |
| [schoolcar5.c](../../LEGOLAND/schoolcar5.c) | §5 shade ownership/z-command decoder/free integration/cubic refinement/model reader | 2 | Closed; external file bytes required to instantiate models |
| [schoolcar6.c](../../LEGOLAND/schoolcar6.c) | §5 RK4 tableau/vector interface/bisection/z-span clearing/555 and 565 ramps | 3 | Closed |
| [schoolcar7.c](../../LEGOLAND/schoolcar7.c) | §4 cardinal rotation defect, §5 route reset/save/rail offset and LMS pointer fixup | 0 | LMS schema beyond the recovered substitution lists remains opaque |
| [schoolcar8.c](../../LEGOLAND/schoolcar8.c) | §5 vector/seat/dispatch/geometry hooks, quarter-turn literals, shadow/clamp helpers | 0 | Seat-x/spacing and model templates external |

Supplementary source checks resolve caller/callee disagreements using [anim2.c](../../LEGOLAND/anim2.c), [posstep.c](../../LEGOLAND/posstep.c), [goldrush.c](../../LEGOLAND/goldrush.c) [rides.c](../../LEGOLAND/rides.c) and [ridemisc2.c](../../LEGOLAND/ridemisc2.c). Lane cross-checks are [fable-a-bswater.md](../lanes/fable-a-bswater.md), [fable-a-jcroute.md](../lanes/fable-a-jcroute.md), [fable-b-ridemisc.md](../lanes/fable-b-ridemisc.md), [codex-a.md](../lanes/codex-a.md), and the related mechanics summaries in [fable-a.md](../lanes/fable-a.md) and [fable-b.md](../lanes/fable-b.md). These are supporting evidence, not extra assigned-file rows.

### WIP note disposition

These are existing source markers, not new test results. The runtime page incorporates their behavioural content; compiler allocation, instruction scheduling and historical experiment logs remain source evidence rather than gameplay rules. No marker was changed.

| Source | Functions whose WIP notes were covered | Runtime content / residual category |
| --- | --- | --- |
| [bswater.c](../../LEGOLAND/bswater.c) | `BsWater_SetTile` | Patch stamping/null cells; allocation/scheduling residual |
| [bswater3.c](../../LEGOLAND/bswater3.c), [roads.c](../../LEGOLAND/roads.c) | `BsBoat_Animate`, `JcBoat_Animate` | Five path cases, 80 samples/heading; floating-point allocation residual |
| [coaster3d.c](../../LEGOLAND/coaster3d.c) | `Coaster3D_SetupView`, `Coaster3D_BuildPieceGeometry`, `Raster_SubmitPoly` | View/geometry/raster pipeline and defects; allocation/frame residuals, geometry extent issue |
| [coaster4.c](../../LEGOLAND/coaster4.c) | `InitTrackDrawModes` | Twelve direction pairs and output modes; stack/array allocation residual |
| [coaster6.c](../../LEGOLAND/coaster6.c) | `CoasterCar_BuildRider` | Copied template substitutions; allocation residual |
| [coastermath.c](../../LEGOLAND/coastermath.c) | `MatMul` | Row-major multiply; extent issue |
| [jcroute.c](../../LEGOLAND/jcroute.c) | `JungleCruise_TraceRoute` | DFS link/owner/success rules; tail-call phase/allocation residual |
| [junglecruise.c](../../LEGOLAND/junglecruise.c) | `JungleCruise_UpdateRiverTile`, `JungleCruise_UpdateRiverAnim` | Water ownership/painting, two draw passes/seat order; allocation residuals |
| [lfentrance.c](../../LEGOLAND/lfentrance.c) | `LFEntrance_Add`, `LFEntrance_Activate` | Station pieces, allocation edges and complete rider state progression; allocation residuals |
| [logflume.c](../../LEGOLAND/logflume.c) | `LFEntrance_Remove` | Wrong cursor y edge and repeated brick charge; allocation residual |
| [logflume2.c](../../LEGOLAND/logflume2.c), [logflume3.c](../../LEGOLAND/logflume3.c) | `LFDrop_Place`, `LFTunnel_Place`, `LFCorner_Place` | Sub-piece floor plans and pre-guard parent read; coordinate allocation/scheduling residuals |
| [logflume4.c](../../LEGOLAND/logflume4.c) | `LFTrack_DrawAlt` | Near/far order, mode inconsistency, no-boat early return and final sprite; register residual |
| [schoolcar2.c](../../LEGOLAND/schoolcar2.c) | `SchoolCarManoeuvreC`, `SchoolCarManoeuvreA` | Four-point turns, tight/wide choice; argument-register residuals |
| [schoolcar3.c](../../LEGOLAND/schoolcar3.c) | `Coaster3D_BuildTrackMesh`, `Coaster3D_DrawMesh`, `Coaster3D_InitTrackTopology` | Sweep/shading/triangles/topology; allocation/index-stride residuals |
| [schoolcar4.c](../../LEGOLAND/schoolcar4.c) | `SchoolCarBlockedAhead`, `TrackCurve_GatherParams` | Last qualifying obstruction, sorted cubic sample parameters; allocation residuals |
| [schoolcar5.c](../../LEGOLAND/schoolcar5.c) | `Route_StepFree`, `TrackCurve_Refine` | 70 subdivisions per second, endpoint landing/chord-deviation extrema; frame/x87 residuals |
| [schoolcar6.c](../../LEGOLAND/schoolcar6.c) | `Route_StepToPieceEnd`, `ZBuffer_FillPoly`, `Shade_BuildRamp` | Bisection, zero-filled spans, ramps; frame/register/x87 residuals |

Two WIP markers explicitly describe an extent-walker limitation. `MatMul` records a 40-instruction/103-byte full body while the old audit window stops at 10 instructions/28 bytes. `Coaster3D_BuildPieceGeometry` records 143 instructions/528 bytes through 0x004286df, versus the tool's 112-instruction/449-byte window. Both notes report `ESCAPES`; neither establishes a missing runtime branch. Those numbers are quoted from the existing source and were not remeasured here. [coastermath.c](../../LEGOLAND/coastermath.c), [coaster3d.c](../../LEGOLAND/coaster3d.c)

## Findings and resolutions

| Scope J requirement | Evidence and resolution |
| --- | --- |
| Record sizes, offsets and meanings | Six subsystem layout sections include water/boat/deco, flume/run/queue, school-car/road, track/joint/solver/raster and ancillary records. This pass filled decoration removal views, the 0x1c `PolyVtx` distinction and helper access views. [transport.md](transport.md), [junglecruise.c](../../LEGOLAND/junglecruise.c), [coaster3d.c](../../LEGOLAND/coaster3d.c), [lfmisc.c](../../LEGOLAND/lfmisc.c) |
| Rules and state machines | Added exact queue equality/malformed-state behaviour, packed piece/rider references, drop expansion, preview cursors, alternate draw ordering, loading-bay dispatch/seat handling, and ancillary helper effects. Existing 120-frame flume timeline, manoeuvre 0…5 table, graph reservation rules and 8→4→0x10 coaster states remain documented. [transport.md](transport.md), [lfentrance.c](../../LEGOLAND/lfentrance.c), [logflume2.c](../../LEGOLAND/logflume2.c), [logflume4.c](../../LEGOLAND/logflume4.c), [schoolcar8.c](../../LEGOLAND/schoolcar8.c) |
| Every decoded table | Added ten flume sprite names, six quarter-turn literals, three eight-slot geometry interfaces, six traffic-light sprite sequences, rider substitution defaults, 24 seam pairs and 12 triangle templates. The 60 road slots, six flume paths, RK4 tableau, ten vector operations, draw-direction pairs and tower slot/matte tables were checked against source. Extern-only raw bytes are enumerated as limits below. [roads.c](../../LEGOLAND/roads.c), [logflume.c](../../LEGOLAND/logflume.c), [logflume4.c](../../LEGOLAND/logflume4.c), [schoolcar.c](../../LEGOLAND/schoolcar.c), [schoolcar3.c](../../LEGOLAND/schoolcar3.c), [schoolcar6.c](../../LEGOLAND/schoolcar6.c), [schoolcar8.c](../../LEGOLAND/schoolcar8.c), [coaster.c](../../LEGOLAND/coaster.c), [coaster6.c](../../LEGOLAND/coaster6.c), [bswater3.c](../../LEGOLAND/bswater3.c) |
| Original bugs | Inventoried comments containing original-bug, uninitialized, unchecked, inconsistency and dead-store language. Added the flume run-list null-head fault, alternate rail-mode inconsistency, queue coherence assumptions, unchecked drop-parent access, useless doubled-dimension stores and z-fill dead store. Existing named defects remain in each subsystem's original-bugs section. [logflume2.c](../../LEGOLAND/logflume2.c), [logflume4.c](../../LEGOLAND/logflume4.c), [lfmisc.c](../../LEGOLAND/lfmisc.c), [schoolcar6.c](../../LEGOLAND/schoolcar6.c) |
| Callback roles and slots | Each subsystem ends with callback roles, distinguishing ObjDef shims from boat/car helpers, descriptor geometry hooks and solver hooks. Full class registration coverage is owned by the parent Scope J callback page; transport does not claim helpers occupy ObjDef slots. [transport.md](transport.md), [RIDE_CALLBACKS.md](../RIDE_CALLBACKS.md) |
| Disagreement reconciliation | Preserved explicit `LFAnimRefs`→`{path,head,tail}`, `TrackJoint` direction/float-height, river distance/frontier and flume spline→polyline reconciliations. New corrections are listed below. [transport.md](transport.md) |
| Source-linked, honest coverage | All 30 assigned files occur once in the coverage table. Thirteen “partial” rows identify only remaining source boundaries. The other 17 cover recovered mechanics without claiming complete binaries/assets. [transport.md](transport.md) |

The skeptical review corrected these concrete overclaims:

- The Jungle seat table at 0x0081cb80 is generated by `JungleCruise_BuildWobbleTable`, not an unresolved external asset. Cross-group review found its builder in ridemisc2.c, and the complete ellipse formula/constants now appear beside the consumer. [junglecruise.c](../../LEGOLAND/junglecruise.c), [ridemisc2.c](../../LEGOLAND/ridemisc2.c)

- `SchoolCar +c6` is a randomized speed ceiling, not horn pitch: the motion consumer compares it with speed. [schoolcar2.c](../../LEGOLAND/schoolcar2.c), [goldrush.c](../../LEGOLAND/goldrush.c)
- Geometry interfaces at 0x004dd5e0 dispatch cubic/line/arc math, not head/middle/tail cars. [schoolcar.c](../../LEGOLAND/schoolcar.c), [schoolcar4.c](../../LEGOLAND/schoolcar4.c), [schoolcar8.c](../../LEGOLAND/schoolcar8.c)
- Mesh instances 0…28 copy 18 seam pairs and instance 29 copies 24. “24 per segment” was wrong; the totals are 546 pairs and 360 triangles. [schoolcar3.c](../../LEGOLAND/schoolcar3.c)
- Raster high-nibble **OR** requires at least one flagged projected vertex, not every vertex. The z-only span pass writes zero and uses an inclusive half-pixel-width threshold. [coaster3d.c](../../LEGOLAND/coaster3d.c), [schoolcar6.c](../../LEGOLAND/schoolcar6.c)
- Flume shape-array bases +0/+0x10/+0x18 are not equally eight bytes apart; preview geometry and ordinary draw mode have separate contracts. [logflume2.c](../../LEGOLAND/logflume2.c), [lfentrance.c](../../LEGOLAND/lfentrance.c), [logflume4.c](../../LEGOLAND/logflume4.c)
- Carousel zero-revolution handling stops immediately after `GetAllBlokesOffRide`, because that callee always returns 1 after requesting rider action changes. The stop/start returns skip the positioning pass. This replaces the misleading “wait for riders to leave” caller-header description. [bswater.c](../../LEGOLAND/bswater.c), [rides.c](../../LEGOLAND/rides.c)

## Checks and remaining source limitations

The failable checks are: exact-set comparison of the two 30-row evidence/coverage inventories against the assigned JSON inventory; enumeration of all 31 WIP markers; checking the five required subsection headings in order for all six subsystems; local Markdown target/anchor existence; topology/road table cardinalities; and `git diff --check` for the two owned files. Final results are recorded after running those checks. These checks validate document integrity and bounded source comparisons; they are not execution tests.

The remaining limits are specific source limits:

- **Unprinted external tables/assets:** boating/Jungle 16×25 water artwork; four-entry motion/arc rows; flume overlay frame offsets/shape values and ODF footprint values; raw coaster source geometry, support models, seat positions/spacing and loaded model records. Their known addresses, extents, indexing and consumers are documented; no table values were invented. [bswater.c](../../LEGOLAND/bswater.c), [bswater3.c](../../LEGOLAND/bswater3.c), [junglecruise.c](../../LEGOLAND/junglecruise.c), [roads.c](../../LEGOLAND/roads.c), [logflume.c](../../LEGOLAND/logflume.c), [logflume2.c](../../LEGOLAND/logflume2.c), [coaster3d.c](../../LEGOLAND/coaster3d.c), [coaster4.c](../../LEGOLAND/coaster4.c), [schoolcar8.c](../../LEGOLAND/schoolcar8.c)
- **Incomplete types/semantics:** larger decoration allocations, unnamed portions of the 0xa4 coaster piece and its raised footprint/cache overlay, some route coefficients, and LMS fields beyond the recovered pointer fixups/substitution lists. A local removal or rendering view cannot prove a universal full struct. [junglecruise.c](../../LEGOLAND/junglecruise.c), [coaster.c](../../LEGOLAND/coaster.c), [coaster4.c](../../LEGOLAND/coaster4.c), [schoolcar.c](../../LEGOLAND/schoolcar.c), [schoolcar7.c](../../LEGOLAND/schoolcar7.c), [coaster6.c](../../LEGOLAND/coaster6.c)
- **Unproved runtime reachability:** `LFPath_Point(1)` indexes outside its point array, while advance normalizes only values greater than 1. The claim that exact equality is unreachable cannot be established by these source guards. The page specifies the actual comparisons and leaves reachability open. Malformed saved indices, impossible headings and empty-head inputs likewise retain their observed unchecked behaviour. [logflume6.c](../../LEGOLAND/logflume6.c), [logflume7.c](../../LEGOLAND/logflume7.c), [lfmisc.c](../../LEGOLAND/lfmisc.c), [schoolcar7.c](../../LEGOLAND/schoolcar7.c)

Completing this documentation audit does not turn those external bytes or unresolved source interpretations into recovered facts. No C/tool edits, compilation, gameplay tests, commits or pushes were performed by this transport audit.
