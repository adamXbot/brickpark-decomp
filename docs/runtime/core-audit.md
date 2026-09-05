# Core documentation completion audit

Scope: the74 core sources assigned to [world](world.md), [persistence](persistence.md) and [assets](assets.md), including transport/UI material in mixed files. Final baseline is `f8f5854481b2a87fb456a37b02ce581e9206b400`; the first71 sources were reviewed at the preceding Scope J baseline and the integration delta is recorded below. Primary source membership is enumerated in [coverage](coverage.md).

## Evidence pass

The audit inventoried the top block comments of every assigned file, then searched their block comments and function notes for decoded layouts/tables, original bugs, uninitialized values, unchecked accesses, leaks and historical stand-in warnings. Compiler-allocation discussions were distinguished from gameplay behavior. Relevant consumers were read when declarations disagreed. The existing specification was checked against that evidence; this is a documentation audit, with no compiler or original-game execution.

| Requirement | Evidence and disposition |
| --- | --- |
| Map, object and cursor layouts | Existing Cell/grid/ObjDef/cursor tables retained; added the separate eight-byte PolyLine/WalkPath and twelve-byte WalkNode records. [mappath.c](../../LEGOLAND/mappath.c), [world](world.md) |
| Walking and route behavior | Added duplicate segment origins and omitted endpoints in BuildWalkPath; retained allocation failure and unreachable state1/state2 relaxation. Corrected the nonexistent InitMoveLine name to CalcMoveLine and documented its signed angle result and coordinate-boundary ambiguity. [mappath.c](../../LEGOLAND/mappath.c), [simcore.c](../../LEGOLAND/simcore.c), [bnvmove.c](../../LEGOLAND/bnvmove.c) |
| Visitor/staff records and tables | Appearance transfer globals and palette interpretation added; all26 high-level AI table slots remain identified. A whole-tree function-marker search did not recover the missing visitor high-level/state7 bodies, so they remain genuine source gaps. [blokeai.c](../../LEGOLAND/blokeai.c), [blokelist.c](../../LEGOLAND/blokelist.c), [world](world.md) |
| Statistics and economy | Expanded all category fields, publication destinations, income contributions and clamp order. Corrected local declaration comments reversing working and occupied counts and the header's wrong definition-count offset. Separated BuyItem's signed value at+28 from build/salvage cost+26. [bigsim.c](../../LEGOLAND/bigsim.c), [loaders.c](../../LEGOLAND/loaders.c), [world](world.md) |
| Map/runtime original bugs | Off-map IsObjectRunning dereference added. Existing door, allocation, route-parent, closed-list leak, count inflation and printf-argument defects verified against their notes. [sysmisc3.c](../../LEGOLAND/sysmisc3.c), [objmap2.c](../../LEGOLAND/objmap2.c), [workorder3.c](../../LEGOLAND/workorder3.c), [world](world.md) |
| Save/profile/script formats | Existing container, flattened records, event lists, string reader, pointer conversions and packed profile descriptions checked. Worker-save action/position correction remains explicit. String writer and profile name-region interpretation remain source limitations. [persistence](persistence.md) |
| Asset layouts and decoded constants | Added tile-info code width, BINV frame-list link, palette/TGA layouts, map-loader defaults, MIDI file layout, composition parameters and narration buffer rules. Existing COMP/CSP/ILF, morph, position and RIN contracts retained. [assets](assets.md), [rin.c](../../LEGOLAND/rin.c), [music.c](../../LEGOLAND/music.c) |
| Lifecycle and naming traps | Added destructive PauseCurrentTrack semantics, MIDI timer parameters/unchecked success, and sample-teardown ready-flag ordering. [audio4.c](../../LEGOLAND/audio4.c), [lifecycle.c](../../LEGOLAND/lifecycle.c), [assets](assets.md) |
| Legacy sweep provenance | Corrected the claim that current sweep2/sweep5 bodies necessarily contain stand-in tails: their historical warning/anchor declarations survive, but those anchor helpers have no calls in the current files. No absent behavior was synthesized from that warning. [sweep2.c](../../LEGOLAND/sweep2.c), [sweep5.c](../../LEGOLAND/sweep5.c), [assets](assets.md) |
| Mixed UI/transport files | The presentation audit covers misc3's dialog, work-order and preview contracts; transport covers anim2/posstep boat and road behavior. Their primary ownership remains unchanged in the inventory. [presentation audit](presentation-audit.md), [transport audit](transport-audit.md) |
| Callback registrations | Reproducible checks compare every final direct class-slot assignment and every linked implementation name/VA. Library initialization and null-clearing exceptions remain explicit. [callbacks](callbacks.md), [checks](checks.md) |

## Remaining source limitations

The remaining boundaries are absent external table contents and function bodies, ambiguous field meanings or file variants, and historical binary-provenance limits. They are listed where a runtime implementer encounters them. This audit found and closed documentation omissions; it does not convert those missing inputs into known behavior. The original brief explicitly requires unknowns to be identified rather than invented. [Scope J](../SCOPE_J_runtime_spec.md), [coverage](coverage.md)

The automated gate is published as [reproducible checks](checks.md). Its result, the independent subsystem reviews and final scope-isolation checks are recorded in [verification](verification.md).

## Integration delta from current main

Main advanced while Scope J was being reviewed. The branch was rebased onto `f8f5854481b2a87fb456a37b02ce581e9206b400`, then the nine new sources and twelve changed sources were assigned to their existing audit owners. Core added three files (925 lines,76 function bodies): audio5, pathmisc2 and tinystubs. All three were read in full, including declarations and behavior/bug notes. [audio5.c](../../LEGOLAND/audio5.c), [pathmisc2.c](../../LEGOLAND/pathmisc2.c), [tinystubs.c](../../LEGOLAND/tinystubs.c)

| New evidence | Integration result |
| --- | --- |
| Narration and sample sources | Full WAV header layout/parser, unvalidated format ID, no odd-byte padding, stale pan, status-result gating, restaurant two-effect source and FX+8 correction added to assets. [audio5.c](../../LEGOLAND/audio5.c), [audio lane](../lanes/fable-d-audio5.md) |
| Path/order helpers | All16 bodies covered: coordinate searches, list unlinks, allocation/count failures, connected-square refresh, span transitions,25-bit scan and corrected numeric side map. [pathmisc2.c](../../LEGOLAND/pathmisc2.c), [Codex D lane](../lanes/codex-d.md) |
| Microhelpers | All56 bodies accounted for across assets, world, persistence and presentation. New details include null-offset LOC fixups, suffix-first script deletion, four-byte-only block consumption, released keyboard pointer retention and exact timer/predicate arithmetic. [tinystubs.c](../../LEGOLAND/tinystubs.c), [Scope E lane](../lanes/scope-e.md) |
| Existing core changes | objmap2, savechunks2, workers2 and workorder3 add WIP/triage notes without new runtime operations. sysmisc's person projection changes local storage/read form while retaining its numeric transform; the sample-source delta is a scheduling note. Existing contracts remain valid. [Scope I lane](../lanes/scope-i.md), [sysmisc.c](../../LEGOLAND/sysmisc.c) |

The current primary inventories total192 sources: world36, persistence6, assets32, transport32, attractions29 and presentation57. Coverage reasons and new-source citations are checked by the same reproducible gate. This review includes the newly merged behavior rather than simply relabeling the old183-source document. [coverage](coverage.md), [checks](checks.md)
