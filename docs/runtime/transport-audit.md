# Transport documentation completeness audit

This audit covers source baseline `cf8e88c845dc4b109bbb2bd9f2a31d1177a9c9aa` after the parent merged updated main. It checks the 35-source transport inventory against [transport.md](transport.md), using the requirements in [Scope J](../SCOPE_J_runtime_spec.md). It is a documentation audit, not a compiler audit or an execution test.

## Stage plan and pass conditions

1. **Inventory evidence:** read every assigned source header, recovered data declaration, original-defect comment and behavioural note adjacent to WIP functions; record a per-source evidence row. Pass when the evidence rows exactly equal the 35-source inventory, with no duplicates.
2. **Close recoverable omissions:** compare layouts, rules, decoded tables, original defects and callback roles with the runtime page; add missing recovered material and record each resolution below. Pass when every requirement has either a runtime section or a specific source limitation.
3. **Verify and review:** check local Markdown links and anchors, exact inventory coverage, section order and whitespace; review uncertain claims against source operations. Pass when mechanical checks succeed and remaining limitations are explicitly identified.

Parallelism is at the parent Scope J task level: other agents audit the attraction and presentation groups while this agent owns transport. No C files or tools are edited; no compiler or game execution is part of these checks.

## Evidence inventory (updated after original-data recovery)

All 35 assigned source headers were read, their recovered table declarations inventoried, and behaviour-bearing comments at the 30 current `WIP-FUNCTION` markers compared with the runtime page. The rows below are the exact transport inventory after the main-branch delta, once each. “Closed” means the available evidence is documented; a remaining source limitation is not deferred prose or a claim of binary matching.

| Source | Recovered evidence covered in transport.md | WIP markers | Remaining source limitation |
| --- | --- | ---: | --- |
| [bswater.c](../../LEGOLAND/bswater.c) | §1 launch/lake flood/painting, §6 carousel state/placement and defects | 1 | Closed by original binary/data; see [transport-data.md](transport-data.md) |
| [bswater2.c](../../LEGOLAND/bswater2.c) | §1 reservation/preferences/docking/DFS, §6 restaurant/plane/tower-animation views | 0 | Closed |
| [bswater3.c](../../LEGOLAND/bswater3.c) | §1 80-frame animation, §6 tower occupancy/draw slots/mattes/placement defects | 1 | Closed by original binary/data; see [transport-data.md](transport-data.md) |
| [coaster.c](../../LEGOLAND/coaster.c) | §4 spawning/light tables, §5 graph/editor/save relocation/piece prefix and null-read defect | 0 | Closed by original binary/data; see [transport-data.md](transport-data.md) |
| [coaster3d.c](../../LEGOLAND/coaster3d.c) | §5 view/geometry/projection/raster records, gradient-clamp defect, clip predicates | 3 | Closed by original binary/data; see [transport-data.md](transport-data.md) |
| [coaster4.c](../../LEGOLAND/coaster4.c) | §5 constructors/view constants/draw ordering/save walk/support shadows | 1 | Closed by original binary/data; see [transport-data.md](transport-data.md) |
| [coaster5.c](../../LEGOLAND/coaster5.c) | §5 fit/joint wiring/height profiling/car creation/fast-root construction | 0 | Closed by original binary/data; see [transport-data.md](transport-data.md) |
| [coaster6.c](../../LEGOLAND/coaster6.c) | §5 run endpoints/basis/seat substitution schema and default colours/names | 1 | Closed by original binary/data; see [transport-data.md](transport-data.md) |
| [coaster7.c](../../LEGOLAND/coaster7.c) | §5 three-part entrance/energy/brake/clip planes/matrix conversion/model loader | 0 | Closed by original binary/data; see [transport-data.md](transport-data.md) |
| [coaster8.c](../../LEGOLAND/coaster8.c) | §5 all12 joint/route/seat/raster/model/palette/vector/save helpers | 0 | Closed; stale source energy/seat labels corrected |
| [coaster9.c](../../LEGOLAND/coaster9.c) | §5 all26 model/route/seat/clip/physics helper bodies and alias corrections | 0 | Closed; source confirms prior binary evidence and adds line-copy/draw-once detail |
| [coastertiny.c](../../LEGOLAND/coastertiny.c) | §5 all 47 route/seat/pool/timer/raster/model helpers and accessors | 0 | Closed by original binary/data; see [transport-data.md](transport-data.md) |
| [coastermath.c](../../LEGOLAND/coastermath.c) | §5 matrix/vector/clip/joint helpers, root ABI/index edge cases | 0 | MatMul now FUNCTION; historical extent caveat superseded |
| [jcroute.c](../../LEGOLAND/jcroute.c) | §2 disjoint DFS/BFS fields, success flag, ownership and re-enqueued seed | 1 | Closed |
| [junglecruise.c](../../LEGOLAND/junglecruise.c) | §2 boat/deco layouts, water ownership, draw ordering, assignment/null defects | 2 | Closed by original binary/data; see [transport-data.md](transport-data.md) |
| [lfentrance.c](../../LEGOLAND/lfentrance.c) | §3 entrance pieces/previews/queue/action timeline/layered drawing/uninitialized frame | 2 | Closed; geometry consumes class footprint data |
| [lfmisc.c](../../LEGOLAND/lfmisc.c) | §1 unlink, §3 queue/ordinals/drop helpers, §6 names/walk/person/pump teardown | 0 | Closed; local access views do not establish complete object sizes |
| [lfmisc2.c](../../LEGOLAND/lfmisc2.c) | §3 queue/corner, §4 traffic gate, §6 power-station add/audio sources | 0 | Closed; empty output/uninitialized source field preserved |
| [logflume.c](../../LEGOLAND/logflume.c) | §3 class shims, allocation/placement, ten sprite names, overlays/save/demolition defects | 1 | Closed by original binary/data; see [transport-data.md](transport-data.md) |
| [logflume2.c](../../LEGOLAND/logflume2.c) | §3 route links/probes/masks/drop expansion/packed references/unlink/0x41 defect | 1 | Closed by original binary/data; see [transport-data.md](transport-data.md) |
| [logflume3.c](../../LEGOLAND/logflume3.c) | §3 tunnel/CSAW/hold-up/corner floor plans, ownership and pre-guard read | 2 | Closed; dimensions intentionally expressed from ODF footprints |
| [logflume4.c](../../LEGOLAND/logflume4.c) | §3 run clock/queue, all six paths, alternate draw buckets/order/mode inconsistency | 1 | Closed |
| [logflume5.c](../../LEGOLAND/logflume5.c) | §3 piece-tree/queue load, boat-step priority, invalid join orientation | 0 | Closed |
| [logflume6.c](../../LEGOLAND/logflume6.c) | §3 queue save, spacing/advance/drop timeline/draw and uninitialized outputs | 0 | Closed; endpoint behavior specified, execution reachability untested |
| [logflume7.c](../../LEGOLAND/logflume7.c) | §3 waiting predicate, heading/shape mapping, polyline interpolation | 0 | Closed; FPU-dependent endpoint hazard specified |
| [roads.c](../../LEGOLAND/roads.c) | §1 boat-animation formulas, §4 all 60 road slots, rotation/zebra stamping/null writes | 1 | Closed by original binary/data; see [transport-data.md](transport-data.md) |
| [roads2.c](../../LEGOLAND/roads2.c) | §4 road record/one-way entrance/frontier/predecessor rules | 0 | Closed |
| [schoolcar.c](../../LEGOLAND/schoolcar.c) | §4 steering/lifecycle/free-before-unlink, §5 pool/route/car/save/three geometry interfaces | 0 | Closed by original binary/data; see [transport-data.md](transport-data.md) |
| [schoolcar2.c](../../LEGOLAND/schoolcar2.c) | §4 tight/wide turns, heading rotation, chooser mask probabilities | 2 | Closed |
| [schoolcar3.c](../../LEGOLAND/schoolcar3.c) | §4 pull-off, §5 mesh shading/projection, 24 seam pairs and 12 triangle templates | 3 | Closed; matching residuals are not omitted geometry |
| [schoolcar4.c](../../LEGOLAND/schoolcar4.c) | §4 queue overread/obstruction, §5 elapsed clamp/lap states/cubic tessellation/model input | 2 | Closed |
| [schoolcar5.c](../../LEGOLAND/schoolcar5.c) | §5 shade ownership/z-command decoder/free integration/cubic refinement/model reader | 2 | Closed; external file bytes required to instantiate models |
| [schoolcar6.c](../../LEGOLAND/schoolcar6.c) | §5 RK4 tableau/vector interface/bisection/z-span clearing/555 and 565 ramps | 3 | Closed |
| [schoolcar7.c](../../LEGOLAND/schoolcar7.c) | §4 cardinal rotation defect, §5 route reset/save/rail offset and LMS pointer fixup | 0 | Closed by original binary/data; see [transport-data.md](transport-data.md) |
| [schoolcar8.c](../../LEGOLAND/schoolcar8.c) | §5 vector/seat/dispatch/geometry hooks, quarter-turn literals, shadow/clamp helpers | 0 | Closed by original binary/data; see [transport-data.md](transport-data.md) |

Supplementary source checks resolve caller/callee disagreements using [anim2.c](../../LEGOLAND/anim2.c), [posstep.c](../../LEGOLAND/posstep.c), [goldrush.c](../../LEGOLAND/goldrush.c) [rides.c](../../LEGOLAND/rides.c) and [ridemisc2.c](../../LEGOLAND/ridemisc2.c). Lane cross-checks are [fable-a-bswater.md](../lanes/fable-a-bswater.md), [fable-a-jcroute.md](../lanes/fable-a-jcroute.md), [fable-b-ridemisc.md](../lanes/fable-b-ridemisc.md), [codex-a.md](../lanes/codex-a.md), and the related mechanics summaries in [fable-a.md](../lanes/fable-a.md) and [fable-b.md](../lanes/fable-b.md). These are supporting evidence, not extra assigned-file rows.

### WIP note disposition

These are existing source markers, not new test results. The runtime page incorporates their behavioural content; compiler allocation, instruction scheduling and historical experiment logs remain source evidence rather than gameplay rules. No marker was changed.

| Source | Functions whose WIP notes were covered | Runtime content / residual category |
| --- | --- | --- |
| [bswater.c](../../LEGOLAND/bswater.c) | `BsWater_SetTile` | Patch stamping/null cells; allocation/scheduling residual |
| [bswater3.c](../../LEGOLAND/bswater3.c), [roads.c](../../LEGOLAND/roads.c) | `BsBoat_Animate`, `JcBoat_Animate` | Five path cases, 80 samples/heading; floating-point allocation residual |
| [coaster3d.c](../../LEGOLAND/coaster3d.c) | `Coaster3D_SetupView`, `Coaster3D_BuildPieceGeometry`, `Raster_SubmitPoly` | View/geometry/raster pipeline and defects; allocation/frame residuals; former extent issue fixed |
| [coaster4.c](../../LEGOLAND/coaster4.c) | `InitTrackDrawModes` | Twelve direction pairs and output modes; stack/array allocation residual |
| [coaster6.c](../../LEGOLAND/coaster6.c) | `CoasterCar_BuildRider` | Copied template substitutions; allocation residual |
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

The earlier baseline carried two extent-walker warnings. At the latest baseline, `MatMul` is promoted to FUNCTION and `Coaster3D_BuildPieceGeometry` explicitly says its full143-instruction/528-byte extent is now bounded correctly. The latter remains WIP for38 strict register-allocation mismatches. MatMul's adjacent prose still describes the old truncation warning while giving the full40-instruction/103-byte extent; it is historical text, not a current marker or unresolved runtime branch. The two changed source lines affect matching provenance, not C behavior. No compiler or matching tool was rerun here. [coastermath.c](../../LEGOLAND/coastermath.c), [coaster3d.c](../../LEGOLAND/coaster3d.c)

## Findings and resolutions

| Scope J requirement | Evidence and resolution |
| --- | --- |
| Record sizes, offsets and meanings | Six subsystem layout sections include water/boat/deco, flume/run/queue, school-car/road, track/joint/solver/raster and ancillary records. This pass filled decoration removal views, the 0x1c `PolyVtx` distinction and helper access views. [transport.md](transport.md), [junglecruise.c](../../LEGOLAND/junglecruise.c), [coaster3d.c](../../LEGOLAND/coaster3d.c), [lfmisc.c](../../LEGOLAND/lfmisc.c) |
| Rules and state machines | Added exact queue equality/malformed-state behaviour, packed piece/rider references, drop expansion, preview cursors, alternate draw ordering, loading-bay dispatch/seat handling, and ancillary helper effects. Existing 120-frame flume timeline, manoeuvre 0…5 table, graph reservation rules and 8→4→0x10 coaster states remain documented. [transport.md](transport.md), [lfentrance.c](../../LEGOLAND/lfentrance.c), [logflume2.c](../../LEGOLAND/logflume2.c), [logflume4.c](../../LEGOLAND/logflume4.c), [schoolcar8.c](../../LEGOLAND/schoolcar8.c) |
| Every decoded table | Added ten flume sprite names, six quarter-turn literals, three eight-slot geometry interfaces, six traffic-light sprite sequences, rider substitution defaults, 24 seam pairs and 12 triangle templates. The 60 road slots, six flume paths, RK4 tableau, ten vector operations, draw-direction pairs and tower slot/matte tables were checked against source. Extern-only raw bytes are enumerated as limits below. [roads.c](../../LEGOLAND/roads.c), [logflume.c](../../LEGOLAND/logflume.c), [logflume4.c](../../LEGOLAND/logflume4.c), [schoolcar.c](../../LEGOLAND/schoolcar.c), [schoolcar3.c](../../LEGOLAND/schoolcar3.c), [schoolcar6.c](../../LEGOLAND/schoolcar6.c), [schoolcar8.c](../../LEGOLAND/schoolcar8.c), [coaster.c](../../LEGOLAND/coaster.c), [coaster6.c](../../LEGOLAND/coaster6.c), [bswater3.c](../../LEGOLAND/bswater3.c) |
| Original bugs | Inventoried comments containing original-bug, uninitialized, unchecked, inconsistency and dead-store language. Added the flume run-list null-head fault, alternate rail-mode inconsistency, queue coherence assumptions, unchecked drop-parent access, useless doubled-dimension stores and z-fill dead store. Existing named defects remain in each subsystem's original-bugs section. [logflume2.c](../../LEGOLAND/logflume2.c), [logflume4.c](../../LEGOLAND/logflume4.c), [lfmisc.c](../../LEGOLAND/lfmisc.c), [schoolcar6.c](../../LEGOLAND/schoolcar6.c) |
| Callback roles and slots | Each subsystem ends with callback roles, distinguishing ObjDef shims from boat/car helpers, descriptor geometry hooks and solver hooks. Full class registration coverage is owned by the parent Scope J callback page; transport does not claim helpers occupy ObjDef slots. [transport.md](transport.md), [RIDE_CALLBACKS.md](../RIDE_CALLBACKS.md) |
| Disagreement reconciliation | Preserved explicit `LFAnimRefs`→`{path,head,tail}`, `TrackJoint` direction/float-height, river distance/frontier and flume spline→polyline reconciliations. New corrections are listed below. [transport.md](transport.md) |
| Source-linked, honest coverage | All 35 assigned files occur once in the coverage table. The original-data stage closes all14 former partial rows, and the latest source additions are fully covered without adding partials. [transport.md](transport.md) |

The skeptical review corrected these concrete overclaims:

- The Jungle seat table at 0x0081cb80 is generated by `JungleCruise_BuildWobbleTable`, not an unresolved external asset. Cross-group review found its builder in ridemisc2.c, and the complete ellipse formula/constants now appear beside the consumer. [junglecruise.c](../../LEGOLAND/junglecruise.c), [ridemisc2.c](../../LEGOLAND/ridemisc2.c)

- `SchoolCar +c6` is a randomized speed ceiling, not horn pitch: the motion consumer compares it with speed. [schoolcar2.c](../../LEGOLAND/schoolcar2.c), [goldrush.c](../../LEGOLAND/goldrush.c)
- Geometry interfaces at 0x004dd5e0 dispatch cubic/line/arc math, not head/middle/tail cars. [schoolcar.c](../../LEGOLAND/schoolcar.c), [schoolcar4.c](../../LEGOLAND/schoolcar4.c), [schoolcar8.c](../../LEGOLAND/schoolcar8.c)
- Mesh instances 0…28 copy 18 seam pairs and instance 29 copies 24. “24 per segment” was wrong; the totals are 546 pairs and 360 triangles. [schoolcar3.c](../../LEGOLAND/schoolcar3.c)
- Raster high-nibble **OR** requires at least one flagged projected vertex, not every vertex. The z-only span pass writes zero and uses an inclusive half-pixel-width threshold. [coaster3d.c](../../LEGOLAND/coaster3d.c), [schoolcar6.c](../../LEGOLAND/schoolcar6.c)
- Flume shape-array bases +0/+0x10/+0x18 are not equally eight bytes apart; preview geometry and ordinary draw mode have separate contracts. [logflume2.c](../../LEGOLAND/logflume2.c), [lfentrance.c](../../LEGOLAND/lfentrance.c), [logflume4.c](../../LEGOLAND/logflume4.c)
- Carousel zero-revolution handling stops immediately after `GetAllBlokesOffRide`, because that callee always returns 1 after requesting rider action changes. The stop/start returns skip the positioning pass. This replaces the misleading “wait for riders to leave” caller-header description. [bswater.c](../../LEGOLAND/bswater.c), [rides.c](../../LEGOLAND/rides.c)

## Earlier source-only checks and limitations (superseded below)

The failable checks are: exact-set comparison of the two 32-row evidence/coverage inventories against the assigned JSON inventory; enumeration of all 31 WIP markers; checking the five required subsection headings in order for all six subsystems; local Markdown target/anchor existence; topology/road table cardinalities; and `git diff --check` for the two owned files. The initial 30-file pass passed all checks; the final 32-file pass and results are recorded in the delta section below. These checks validate document integrity and bounded source comparisons; they are not execution tests.

Before original-data recovery, the source-only audit recorded these limits. The binary/data stage below resolves the external data/type boundaries; the endpoint comparison caveat remains part of the runtime contract:

- **Unprinted external tables/assets:** boating/Jungle 16×25 water artwork; four-entry motion/arc rows; flume overlay frame offsets/shape values and ODF footprint values; raw coaster source geometry, support models, seat positions/spacing and loaded model records. Their known addresses, extents, indexing and consumers are documented; no table values were invented. [bswater.c](../../LEGOLAND/bswater.c), [bswater3.c](../../LEGOLAND/bswater3.c), [junglecruise.c](../../LEGOLAND/junglecruise.c), [roads.c](../../LEGOLAND/roads.c), [logflume.c](../../LEGOLAND/logflume.c), [logflume2.c](../../LEGOLAND/logflume2.c), [coaster3d.c](../../LEGOLAND/coaster3d.c), [coaster4.c](../../LEGOLAND/coaster4.c), [schoolcar8.c](../../LEGOLAND/schoolcar8.c)
- **Incomplete types/semantics:** larger decoration allocations, unnamed portions of the 0xa4 coaster piece and its raised footprint/cache overlay, remaining route-node fields, and LMS fields beyond the recovered pointer fixups/substitution lists. A local removal or rendering view cannot prove a universal full struct. [junglecruise.c](../../LEGOLAND/junglecruise.c), [coaster.c](../../LEGOLAND/coaster.c), [coaster4.c](../../LEGOLAND/coaster4.c), [schoolcar.c](../../LEGOLAND/schoolcar.c), [schoolcar7.c](../../LEGOLAND/schoolcar7.c), [coaster6.c](../../LEGOLAND/coaster6.c)
- **Unproved runtime reachability:** `LFPath_Point(1)` indexes outside a2-point array (the later raw audit distinguishes4-point precision), while advance normalizes only values greater than 1. The claim that exact equality is unreachable cannot be established by these source guards. The page specifies the actual comparisons and leaves reachability open. Malformed saved indices, impossible headings and empty-head inputs likewise retain their observed unchecked behaviour. [logflume6.c](../../LEGOLAND/logflume6.c), [logflume7.c](../../LEGOLAND/logflume7.c), [lfmisc.c](../../LEGOLAND/lfmisc.c), [schoolcar7.c](../../LEGOLAND/schoolcar7.c)

Completing this documentation audit does not turn those external bytes or unresolved source interpretations into recovered facts. No C/tool edits, compilation, gameplay tests, commits or pushes were performed by this transport agent; the parent task checkpoints and rebases the shared documentation worktree.

## Updated-main delta audit

Main gained [coaster7.c](../../LEGOLAND/coaster7.c) and [coastertiny.c](../../LEGOLAND/coastertiny.c) during the final review. The transport inventory is now 32 files. Both new sources were read in full, together with their relevant mechanics, original-defect and prototype notes in [fable-d-coaster7.md](../lanes/fable-d-coaster7.md) and [scope-e.md](../lanes/scope-e.md). They contain 10 and 47 `FUNCTION` markers respectively, with no new WIP markers; this is an inventory of existing source labels, not a fresh binary-matching result.

| New evidence | Requirement addressed and resolution |
| --- | --- |
| Energy derivative and station brake | Route +28 is total energy, replacing speed labels in layouts, reset, snapshot/bisection and save prose. Added `sqrt(2*(E−PE)/mass)`, nonpositive clamp, power>FLT_MIN/travel<0.05 gate, and station energy-rate formula with distance scale0.0015875f. The acceleration-named 0x41e7e0/0x41dd70 aliases supply per-node mass0.1f and its sum; reset initializes PE+0.5*m*0.1². [coaster7.c](../../LEGOLAND/coaster7.c), [coastertiny.c](../../LEGOLAND/coastertiny.c) |
| Entrance track | Added 0x24 template and 0x58 geometry layouts, A↔B↔C addresses, exact footprint-relative endpoints, two-map-square stepping, equal parameter slices, corner constants1.5/0.5 and empty-table terminator underwrite. Inclusive `(x,y)…(x+1,y+1)` spans2×2 cells, correcting the lane's loose “1×1” phrase. [coaster7.c](../../LEGOLAND/coaster7.c) |
| Math/raster helpers | Added four half-plane coefficients, 3×3→4×4 transpose, by-value x87 restore, bit-preserving support scaling2.5/8.2, up vectors, shade slices and cursor4.8f evaluation. Corrected copy-only `RoutePos` field-order and arc-slot `CubicUpVector` name disagreements using field consumers/installers. [coaster7.c](../../LEGOLAND/coaster7.c), [coastertiny.c](../../LEGOLAND/coastertiny.c), [schoolcar7.c](../../LEGOLAND/schoolcar7.c), [schoolcar8.c](../../LEGOLAND/schoolcar8.c) |
| Model helpers | Added CRLF record-count boundaries, directory/read/restore failure behaviour, palette/accessors, and optional byte-length output. Following the output pointer through the LFM caller establishes that `g_coaster_tab_b2` holds raw byte lengths despite its pointer-typed declarations and older auxiliary-pointer prose. [coaster7.c](../../LEGOLAND/coaster7.c), [coastertiny.c](../../LEGOLAND/coastertiny.c), [schoolcar.c](../../LEGOLAND/schoolcar.c) |
| Route/seat/pool helpers | Added exact deadline > comparison, sentinel-inclusive node visitation, state1-only waiting count/max, returned seat pointers, ignored free pointer and unchecked pool count. Renamed the pool count's meaning from high-water mark to current usage. Circuit-close/raster-restore hooks are explicitly no-ops. [coastertiny.c](../../LEGOLAND/coastertiny.c) |
| Shared new-source corrections | Cross-checked full carousel allocation0x2c versus the old0x24 access view; tower stop parts`4,3,4,3,2,1,2,1`; and boating water-count's assignment into every boat. These are supplementary reads, not additional transport inventory members. [ridemisc4.c](../../LEGOLAND/ridemisc4.c), [ridetiny.c](../../LEGOLAND/ridetiny.c), [codex-d.md](../lanes/codex-d.md) |

The delta resolves recovered meanings, not all downstream implementations. Raw cursor-mode/support-template values, full LMS schema, class masks/footprints and the pre-existing endpoint-reachability question remain bounded source limitations. The solver callbacks expose energy conservation, but the externally declared mass/potential/travel/distance helpers still define numerical details beyond these two new bodies. [coaster7.c](../../LEGOLAND/coaster7.c), [coastertiny.c](../../LEGOLAND/coastertiny.c)

### Earlier source-only check results

These historical checks passed after the source-only delta edits on2026-09-05, before the original-data stage below:

| Check | Result |
| --- | --- |
| Assigned source membership | Both runtime coverage and audit evidence tables equal the 32-file inventory, with 32 unique rows and no omissions/extras |
| Header / marker inventory | 32 headers; all 31 existing WIP functions named in the disposition table; new files have exactly 10+47 FUNCTION markers and no WIP markers |
| Required structure | All six subsystems have the five required subsections in order |
| Local links / anchors | 424 links in transport.md and 147 in transport-audit.md resolve |
| Road data comparison | All 60 hexadecimal table values match the 15×4 source-header table |
| Mesh topology comparison | All 24 seam pairs and 12 triangle triples match the source construction; expanded totals are 546 pairs / 360 triangles |
| Curve literals | All six quarter-turn decimal float literals match the source |
| Coverage semantics | 18 documented rows and 14 partial rows; partials describe source boundaries, not deferred writing |
| Rebased source baseline | f8f5854481b2a87fb456a37b02ce581e9206b400 is an ancestor of the current documentation branch |
| Repository integrity | `git diff --check` passes for both owned files; no uncommitted LEGOLAND/ or tools/ changes |
| Audit completion | No unresolved placeholders; no compiler or game execution claimed |

The mechanical checks were read-only Python assertions and Git queries. The source comparisons check document integrity and the specified decoded data; they do not certify every arithmetic path against the executable.

## Original binary/data recovery stage

The user requested completion of the remaining partials using original evidence. This stage read the SHA256-pinned executable and original game assets, without compiling or executing the game. Its pass conditions were: decode each omitted static table; settle layout/helper disagreements from original instructions; identify full asset paths; publish runnable read-only extraction checks; reduce a partial status only after its actual contract was closed. Parallel work remained at the parent Scope J task level. [transport-data.md](transport-data.md)

| Previous partial reason | Resolution and evidence |
| --- | --- |
| Boating/Jungle water artwork (`bswater.c`, `junglecruise.c`) | Both400-byte16×25 tables are identical; all rows and hashes published |
| Boat arc/step rows (`bswater3.c`, `roads.c`) | All three4×16-byte tables decoded at both copies; duplicate equality asserted |
| Jungle decoration extents (`junglecruise.c`) | Original allocators prove fish12/tree8/deco8, matching the removal views |
| Seat/cursor values (`schoolcar8.c`, `coastertiny.c`) | Seat-x0/4/8, spacing16, cursor modes0/2/1; cursor mode2 selects pair1's position slot2 |
| Coaster descriptor masks (`coaster.c`, `coaster5.c`) | Five descriptors decoded, common payload0x34 and ordinary stride0x38 reconciled; entire-flags-zero/opposite-heading predicate recovered |
| Allocated piece tail (`coaster.c`, `coaster4.c`) | 0x2c graph+0x14 footprint+0xc cached position+0x58 inline geometry =0xa4; original helper returns address+4c |
| Route-node fields (`schoolcar.c`) | Two complete0x38 cursors, seats, midpoint/potential energy, clip rectangle and links account for0xec; wheel-phase increment returnszero |
| Source geometry/support models (`coaster3d.c`, `coaster4.c`, `coastertiny.c`) | All four geometry vectors, ordering/radii,12/8 vertex templates,5/4 normals and16/8 triangle rows decoded |
| LMS/material payload (`schoolcar.c`, `schoolcar7.c`) | All offsets/counts and16-byte triangle formats identified by three original render passes; all10 LMS/LFM pairs and10 LTXs validated; palette145/name lists manifested |
| Flume overlay/image values (`logflume.c`) | Overlay+4 counts compound child pieces, not screen offsets or rider queue entries; shape11 values, entrance path pairs, four ILF tables decoded |
| Class footprints (`logflume2.c`) | Ten flume ODFs plus castle footprint decoded with complete `Objdesc/` paths; common static0…2 grid distinguished from track ODF0…1 |

Every resolution above has the exact addresses/types/values, hashes and executable read-only checking method in [transport-data.md](transport-data.md). That recovery stage retained32 source files; the later main delta below adds two. Supplementary original helpers and assets are evidence, not additional assigned C files.

The skeptical review caught and corrected five new interpretation hazards before delivery: overlay counters traverse compound children rather than the rider queue; frame fields are thresholds rather than x/y offsets; the geometry template starts0x4b5e00 rather than the stale header's eight-byte-earlier address; the castle descriptor's next string is not a+34 field; and LMS+08 counts triangles, while built-in support models share fewer face normals. LFM dwords are palette/texture indices even where old locals use pointer types. The entrance path builder also repeats each segment's initial sample because its coordinate store precedes interpolation update. [transport-data.md](transport-data.md)

### Latest main delta: coaster8.c and lfmisc2.c

Both new sources were read in full against baseline263cf60b173a8d054e356c5916341cc664033713. They contain12 and6 FUNCTION markers respectively and no WIP markers; no new lane note names either file. All18 behaviors now have runtime-page prose in the appropriate five-part subsystem sections. [coaster8.c](../../LEGOLAND/coaster8.c), [lfmisc2.c](../../LEGOLAND/lfmisc2.c)

| Requirement/evidence | Resolution |
| --- | --- |
| Coaster layouts and hooks | Added0x18 surface/8-byte model-image views, explicit0x20 inline seat methods, zeroed0xec allocation and first-free-seat search |
| Coaster rules and errors | Added opposite−1→1/invalid→0, whole-word sloped predicate, surface acquisition0/1 and pixel pitch, name output0/1/NUL, low24-bit palette index/−1, cross-product order, rider ordinal/−1 |
| Source-label disagreements | State component1 remains total energy and sum+ c4 remains potential energy; seat+18 requests occupied car destruction and differs from pointer-clearing0x4273e0; saved ordinal depends on matching list order, not a new “session only” file constraint |
| Flume queue and geometry | Added pop action/flag/output/unlink/free order, unchanged empty output, all-node walk-state-zero stepping, ABI equivalence, bottom-right signed shift arithmetic/dead escaped stores |
| Driving light gate | Exact NS rejects3/7, EW rejects1/5, all-red rejects all; diagonals pass with either light, including both-on case; no implicit heading validation |
| Power station | Both+98 handlers call basic placement then positional audio(1,1); distinct12-byte FX records choose crystal first/small second;16-byte source obj+04 unwritten |

Source codegen scheduling/matching comments were reviewed and retain their provenance role rather than becoming gameplay rules. The two original semantic hazards are preserved: opposite of−1 yields1, and both power-station callbacks pass an uninitialized unused source word. Queue empty-output behavior and pointer reload are specified. The source header's loose energy and seat method names were explicitly corrected using existing original instruction evidence. [coaster8.c](../../LEGOLAND/coaster8.c), [lfmisc2.c](../../LEGOLAND/lfmisc2.c), [transport-data.md](transport-data.md)

### Latest main delta: coaster9.c

Source baseline cf8e88c845dc4b109bbb2bd9f2a31d1177a9c9aa adds26 coaster helpers with no WIP markers. All source/header/body comments and the relevant codex-e lane notes were read. The runtime page now covers each helper; original bytes settle the misleading pending/link naming and distinguish the derived speed getter from the raw energy setter. No additional external-table boundary appeared. [coaster9.c](../../LEGOLAND/coaster9.c), [codex-e.md](../lanes/codex-e.md)

| Evidence group | Material change versus previous binary recovery |
| --- | --- |
| Model EOL/copy/find-record/index helpers | Corrected “first token” to full CRLF-delimited line; recorded index0 and end-pointer edge cases and no-length-guard copying |
| Pending/active/clip helpers | Separated draw-completed bit0 and piece-match predicate from actual clip-list insertion/unlink; added exact retained links and bounds update |
| Physics state/sqrt/travel/mass/PE | Added copy argument direction, ST(0) binary32 spill and tangent-length travel formula; confirmed derived speed versus stored energy; pinned one additional float block |
| Seat occupancy/release/update and offset evaluation | Existing binary contracts confirmed, including second transform argument, release versus clear, saved translation restoration and position−up×offset |
| Car view and model draw | Added exact separated view assignments,16-byte model projection stride, low32-bit RDTSC accumulation and three-pass surface gate; existing model schemas/pass meanings unchanged |
| Two modified source lines | MatMul becomes FUNCTION; geometry builder retains WIP with corrected full extent. Current WIP total falls31→30; no C body changed |

### Final original-data checks

All checks passed on2026-09-05 after the35-source delta:

| Failable check | Actual result |
| --- | --- |
| Published extraction recipes | All four Python fences in transport-data.md executed successfully in page order: endpoint arithmetic,47 static block hashes/decodes, bounded original instruction spans, model/material/texture/palette and full-path archive schemas |
| Assigned source sets | Runtime coverage and audit evidence tables both contain exactly35 unique assigned sources, with no omissions/extras;35 documented and0 partial |
| Header / marker inventory | All35 headers read;30 current WIP markers remain covered; coaster9 has26 FUNCTION markers andzero WIP markers |
| Required organization | All six subsystems contain the five required subsections in order |
| Local links and anchors | All targets and explicit anchors resolve across all three owned pages |
| Baseline |cf8e88c845dc4b109bbb2bd9f2a31d1177a9c9aa is an ancestor of the documentation branch |
| Repository integrity | `git diff --check` passes for all three pages; no uncommitted LEGOLAND/ or tools/ changes |
| Completion semantics | No audit placeholders remain; the remaining equality/reachability caveat is specified behavior and an execution-validation limit, not missing recovered contract |

The model checks validate all10 LMS/LFM pairs and10 LTX files, all145 palette entries, six registered model names and10 texture names. Archive checks validate11 ODF footprints andfour ILF lists at their15 full directory paths, including payload digests and original loader offsets. The original instruction recipe asserts each bounded span decodes through its stated end. These checks validate the published evidence and schemas; they do not execute gameplay or certify floating-point trajectories.

All 35 coverage rows are now documented; no recoverable source/data omission is carried forward as a partial. Unused serialized fields and original unchecked invalid-input behaviour retain explicit names/offsets or preservation rules. The one reachability qualification is `LFPath_Point(1)`: its2-point out-of-range index and4-point precision-dependent result and callers' strict `>1` normalization are specified exactly, without claiming that ordinary play reaches or avoids equality. WIP labels remain binary-matching provenance, not undocumented runtime mechanics. [transport.md](transport.md), [transport-data.md](transport-data.md)
