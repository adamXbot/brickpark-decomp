# Core documentation completion audit

Scope: the76 core sources assigned to [world](world.md), [persistence](persistence.md) and [assets](assets.md), including transport/UI material in mixed files. Final baseline is `cf8e88c845dc4b109bbb2bd9f2a31d1177a9c9aa`. The initial71-source pass,74-source integration and76-source final integration are distinguished below. Primary source membership is enumerated in [coverage](coverage.md).

## Initial source-only evidence pass

The audit inventoried the top block comments of every assigned file, then searched their block comments and function notes for decoded layouts/tables, original bugs, uninitialized values, unchecked accesses, leaks and historical stand-in warnings. Compiler-allocation discussions were distinguished from gameplay behavior. Relevant consumers were read when declarations disagreed. The existing specification was checked against that evidence; this is a documentation audit, with no compiler or original-game execution.

| Requirement | Evidence and disposition |
| --- | --- |
| Map, object and cursor layouts | Existing Cell/grid/ObjDef/cursor tables retained; added the separate eight-byte PolyLine/WalkPath and twelve-byte WalkNode records. [mappath.c](../../LEGOLAND/mappath.c), [world](world.md) |
| Walking and route behavior | Added duplicate segment origins and omitted endpoints in BuildWalkPath; retained allocation failure and unreachable state1/state2 relaxation. Corrected the nonexistent InitMoveLine name to CalcMoveLine and documented its signed angle result and coordinate-boundary ambiguity. [mappath.c](../../LEGOLAND/mappath.c), [simcore.c](../../LEGOLAND/simcore.c), [bnvmove.c](../../LEGOLAND/bnvmove.c) |
| Visitor/staff records and tables | Appearance transfer globals and palette interpretation added; all26 high-level AI table slots remain identified. A whole-tree function-marker search did not recover the missing visitor high-level/state7 bodies, so they were source gaps at this stage; the later AI evidence closes them. [blokeai.c](../../LEGOLAND/blokeai.c), [blokelist.c](../../LEGOLAND/blokelist.c), [world](world.md) |
| Statistics and economy | Expanded all category fields, publication destinations, income contributions and clamp order. Corrected local declaration comments reversing working and occupied counts and the header's wrong definition-count offset. Separated BuyItem's signed value at+28 from build/salvage cost+26. [bigsim.c](../../LEGOLAND/bigsim.c), [loaders.c](../../LEGOLAND/loaders.c), [world](world.md) |
| Map/runtime original bugs | Off-map IsObjectRunning dereference added. Existing door, allocation, route-parent, closed-list leak, count inflation and printf-argument defects verified against their notes. [sysmisc3.c](../../LEGOLAND/sysmisc3.c), [objmap2.c](../../LEGOLAND/objmap2.c), [workorder3.c](../../LEGOLAND/workorder3.c), [world](world.md) |
| Save/profile/script formats | Existing container, flattened records, event lists, string reader, pointer conversions and packed profile descriptions checked. Worker-save action/position correction remains explicit. String writer and profile name-region interpretation were source limitations at this stage, subsequently closed by the original-data pass. [persistence](persistence.md) |
| Asset layouts and decoded constants | Added tile-info code width, BINV frame-list link, palette/TGA layouts, map-loader defaults, MIDI file layout, composition parameters and narration buffer rules. Existing COMP/CSP/ILF, morph, position and RIN contracts retained. [assets](assets.md), [rin.c](../../LEGOLAND/rin.c), [music.c](../../LEGOLAND/music.c) |
| Lifecycle and naming traps | Added destructive PauseCurrentTrack semantics, MIDI timer parameters/unchecked success, and sample-teardown ready-flag ordering. [audio4.c](../../LEGOLAND/audio4.c), [lifecycle.c](../../LEGOLAND/lifecycle.c), [assets](assets.md) |
| Legacy sweep provenance | Corrected the claim that current sweep2/sweep5 bodies necessarily contain stand-in tails: their historical warning/anchor declarations survive, but those anchor helpers have no calls in the current files. No absent behavior was synthesized from that warning. [sweep2.c](../../LEGOLAND/sweep2.c), [sweep5.c](../../LEGOLAND/sweep5.c), [assets](assets.md) |
| Mixed UI/transport files | The presentation audit covers misc3's dialog, work-order and preview contracts; transport covers anim2/posstep boat and road behavior. Their primary ownership remains unchanged in the inventory. [presentation audit](presentation-audit.md), [transport audit](transport-audit.md) |
| Callback registrations | Reproducible checks compare every final direct class-slot assignment and every linked implementation name/VA. Library initialization and null-clearing exceptions remain explicit. [callbacks](callbacks.md), [checks](checks.md) |

## Source-only audit boundary

At the end of the source-only pass, the boundaries were absent external table contents and function bodies, ambiguous field meanings or file variants, and historical binary-provenance limits. The later executable/data recovery below supersedes those table and schema gaps. They are listed where a runtime implementer encounters them. The initial audit found documentation omissions; the subsequent evidence pages recover the previously missing inputs directly. The original brief explicitly requires unknowns to be identified rather than invented. [Scope J](../SCOPE_J_runtime_spec.md), [coverage](coverage.md)

The automated gate is published as [reproducible checks](checks.md). Its result, the independent subsystem reviews and final scope-isolation checks are recorded in [verification](verification.md).

## Integration delta from current main

Main advanced while Scope J was being reviewed. The branch was rebased onto `f8f5854481b2a87fb456a37b02ce581e9206b400`, then the nine new sources and twelve changed sources were assigned to their existing audit owners. Core added three files (925 lines,76 function bodies): audio5, pathmisc2 and tinystubs. All three were read in full, including declarations and behavior/bug notes. [audio5.c](../../LEGOLAND/audio5.c), [pathmisc2.c](../../LEGOLAND/pathmisc2.c), [tinystubs.c](../../LEGOLAND/tinystubs.c)

| New evidence | Integration result |
| --- | --- |
| Narration and sample sources | Full WAV header layout/parser, unvalidated format ID, no odd-byte padding, stale pan, status-result gating, restaurant two-effect source and FX+8 correction added to assets. [audio5.c](../../LEGOLAND/audio5.c), [audio lane](../lanes/fable-d-audio5.md) |
| Path/order helpers | All16 bodies covered: coordinate searches, list unlinks, allocation/count failures, connected-square refresh, span transitions,25-bit scan and corrected numeric side map. [pathmisc2.c](../../LEGOLAND/pathmisc2.c), [Codex D lane](../lanes/codex-d.md) |
| Microhelpers | All56 bodies accounted for across assets, world, persistence and presentation. New details include null-offset LOC fixups, suffix-first script deletion, four-byte-only block consumption, released keyboard pointer retention and exact timer/predicate arithmetic. [tinystubs.c](../../LEGOLAND/tinystubs.c), [Scope E lane](../lanes/scope-e.md) |
| Existing core changes | objmap2, savechunks2, workers2 and workorder3 add WIP/triage notes without new runtime operations. sysmisc's person projection changes local storage/read form while retaining its numeric transform; the sample-source delta is a scheduling note. Existing contracts remain valid. [Scope I lane](../lanes/scope-i.md), [sysmisc.c](../../LEGOLAND/sysmisc.c) |

That integration brought the primary inventories to192 sources: world36, persistence6, assets32, transport32, attractions29 and presentation57. Coverage reasons and new-source citations are checked by the same reproducible gate. This review includes the newly merged behavior rather than simply relabeling the old183-source document. [coverage](coverage.md), [checks](checks.md)

## Executable and asset recovery pass

The user requested full coverage after the source-only PR. This pass adds read-only original evidence while preserving the documentation-only scope; the later197-source integration is recorded below. [Core data and runnable checks](core-data.md)

| Former partial reason | New evidence and disposition |
| --- | --- |
| State7 and MoveLine coordinate disagreement | Original state7 tests twice-speed radius before stepping; completion restores saved state without snapping position. The accumulator adds eight fractional bits to already24.8 world inputs. |
| Names, power and work-order tables | All83 male/90 female/107 surname entries,65 power rows,17 cooldown rows and12 nearby offsets extracted with order and hashes. |
| Departure and hunger field meanings | Reset/override instructions establish allfive mood thresholds; food removal and UI/goal consumers establish hunger atBloke+7c. Older tiredness/age labels are aliases. |
| Script framing | Original writer writes length bytes without NUL and has no error-counter side effect; reader and writer now agree. |
| Profile name/flags conflict | The editor aliases temporary length at+1e, causing self-overwrite at30/31chars. This is separate from the32-byte raw on-disk name region. [Presentation evidence](presentation-data.md) |
| Flattened saved words | Exact source-offset maps cover every saved byte group. Per-plan scratch, reserved values, stale pointers and uninitialized stack bytes are represented explicitly; raw preservation does not require invented semantic names. |
| Character model/animation data | All15 active morph files validate complete stream consumption, indices and normal counts; manifests use the actual New/kind path rather than obsolete duplicate leaf names. |
| LOC and BNV record gaps | Four real LOC variants prove32-byte stems, texture-ID base, patch count and6-byte patch records. BNV tail c/10 are consumed by GetZSkew; unused+8 is retained without an invented operation. |
| Person field confusion | Scale is+10..18, Euler rotation+40..48. Mask sprite+2c and offsets+24/+28 are tied to the Z-buffer consumer. Reserved frame/animation tails remain explicit. |
| Position-stream variants | Records-per-stream precedes stream count; Copters consumes+4 as float y, while+8 is DWORD4. A universal float-XYZ declaration is rejected. [Attraction evidence](attractions-data.md) |
| ODF initialization/finalization | Finalizer patches only three Water Works names; it invokes no callback. Ordinary, failed-DLL and successful-library initialization paths are now distinguished. |
| Two missing callback bodies | Both power-station add bodies are recovered from the image: basic placement followed by their kind2 sound source. |
| Boat and geometry external tables | Complete transport tables and model schemas are supplied by the independent [transport recovery](transport-data.md). |

The byte/asset checks caught a duplicate leaf-name selection and an incorrect inherited594-member count. Full-tree traversal gives595 paths,581distinct physical offsets and610nodes. Morph models and LOC blocks consume their selected members exactly. This is evidence that the recovery gates reject plausible but incorrect interpretations.

## Second current-main integration

Main advanced to`263cf60b173a8d054e356c5916341cc664033713`; its five new files bring the inventory to197. Scope J merged that revision without source edits. Core read savemisc2.c and musicthread.c in full (1,275 lines,18 leaf helpers and the3,161-instruction worker). All new save, texture, narration and music contracts are integrated above. The music review corrects two overclaims in the header: a mismatched segment filename/name does not by itself prove a failed COM lookup, and the download loop does not null-check its slots. The byte-versus-dword icon framing and run-only TSF deduplication are now explicit.

| Source | Audit | SHA-256 prefix |
| --- | --- | --- |
| [savemisc2.c](../../LEGOLAND/savemisc2.c) | Full header/body/bug review; integrated in persistence/assets | `f0def8e6c11c` |
| [musicthread.c](../../LEGOLAND/musicthread.c) | Full header/body/bug review; integrated in persistence/assets | `75e4c15c713c` |

## AI closure and final independent review

All26 high-level slots and16 low-level states now have complete contracts, including visitor admission/reservation and11 formerly missing low-level helpers. The two evidence pages validate47 complete function ranges, high-level jump tables and the immediate low-level call frontier. [High-level AI](ai-data.md), [low-level AI](ai-low-data.md)

Independent review checked the core byte/asset recipes, ODF finalizer, writer, BNV depth consumer and outfit loader. A separate review checked the new texture/narration and music worker against current source, confirming the documented failure paths and command/notification transitions. All76 primary core sources are documented; no core source remains partial. [Final verification](verification.md)

The final target update to`cf8e88c845dc4b109bbb2bd9f2a31d1177a9c9aa` adds coaster9/ridemachine and raises total coverage to199. No core source bytes changed in that update, so the76-source core audit and its original-data checks remain current. [Transport integration](transport-audit.md), [attraction integration](attractions-audit.md)
