# Scope J verification

Baseline: `cf8e88c845dc4b109bbb2bd9f2a31d1177a9c9aa`; branch `scope/J`; review date 2026-09-05. All 199 source contracts are documented, with 0 partial and 0 unassigned entries. This verifies the documentation deliverable required by [Scope J](../SCOPE_J_runtime_spec.md); no compiler or original-game execution was run.

## Requirement completion

The six primary pages account for all 199 tracked C files. Reviews covered every assigned source header/function note, all 27 lane notes and the required format/callback references. Layouts, rules/state machines, tables/constants, original bugs and callback slots are consolidated in the [core](core-audit.md), [transport](transport-audit.md), [attraction](attractions-audit.md) and [presentation](presentation-audit.md) audits. Three integrations incorporated main's 16 newly added sources across the initial 183-source and final 199-source baselines. [Coverage](coverage.md)

The 63 entries that were partial after source-only consolidation were investigated against the original executable and shipped assets. Six evidence pages close the named table, helper, field and stream boundaries; coverage was changed only after those checks passed. Reserved bytes, native host interfaces and original undefined inputs remain explicitly described. This is complete Scope J coverage, not a percentage of matching C or measured runtime equivalence. [Evidence index and limits](../RUNTIME_SPEC.md#coverage-closure-and-verification-limits)

## Reproducible gates

The read-only Python gate in [checks.md](checks.md) checks local destinations/heading anchors, table headers/widths, source coverage, current callback assignments, source fingerprints, required topics, whitespace and allowed paths. It verifies 199 distinct primary source rows, 630 linked callback name/address pairs and 618 final direct class-slot assignments. All 83 named classes and three alternate library registrations are indexed; the two formerly declaration-only station add handlers now link to their current source bodies. The 60 recorded source fingerprints comprise 58 presentation sources and two newly integrated core files. [Callback matrix](callbacks.md)

Every changed path is `docs/RUNTIME_SPEC.md` or Markdown below `docs/runtime/`. No C, tools, shared reports, original binaries or game assets are committed. The allowed-path comparison uses the PR merge base, so main's independently added source files are not counted as Scope J changes.

| Recovery gate | Checked evidence |
| --- | --- |
| [Core](core-data.md) | Four published recipes:19 instruction ranges and displayed tables;19 character model/LOC members with full consumption;595 resource paths/610nodes/581 physical offsets; three outfit-list members with exact active ordinals/defaults |
| [Transport](transport-data.md) | Four published recipes validate47 original static blocks, geometry/animation records, complete model/palette payloads and the recovered helpers; primary inventory35, all30 WIP notes reviewed |
| [Attractions](attractions-data.md) |38 static table views,24 complete helper functions/1,517 instructions and five full-path asset members, including the Carousel capacity10 definition;30 primary sources,77 helper ownership entries and eleven original arithmetic invariants |
| [Presentation](presentation-data.md) |Four published recipes:13 instruction ranges, nine static manifests,36 exact goal formats/1,811 NUL-inclusive bytes,959 unique type3 sprites/5,285 frames and five Copters layers with32 frames each;58 source fingerprints and all12 WIP notes |
| [High-level AI](ai-data.md) |26 slots/18 distinct nonnull targets,19 complete raw ranges, seven jump tables with instruction-boundary targets,15 strings and coordinate defaults; six existing worker targets connected to source contracts |
| [Low-level AI](ai-low-data.md) |16 slots,28 complete functions/1,800 reachable instructions/4,668 bytes, eight direction vectors, every direct branch and all22 immediate external call targets resolved to source or CRT |

The original executable digest is `c50865b60bfcb26c0a7329a75fb772b10ae234af324f669901906e5abb0e2bd9`. PE readers reject virtual-only BSS as file data. Asset recipes use exact directory paths, bounds and member hashes; duplicate leaf names and shared physical payloads are accounted for. The supplied recipes are read-only and execute no game instructions. [Core evidence](core-data.md)

## Skeptical review and corrections

Source/consumer review corrected ODF initialization, script framing, profile editor/disk aliases, Person3D scale/rotation, MoveLine units/completion, BNV depth fields, hunger naming and movement-angle reuse. Independent subsystem review corrected flume interpolation endpoints, coaster inline geometry and energy, ride slot bases/queue overlap, carousel discharge, Copters scalar/stream formats, popup producer behavior and type3 command decoding. The disagreement register retains the rejected interpretations beside the adopted contracts. [Disagreements](../RUNTIME_SPEC.md#reconciled-disagreements)

Independent instruction review confirmed the AI element/class displacement mismatch, plan5 admission stall, Join allocation leak, Cafe cleanup asymmetry, special-tile callback gate and state9 restoration. A separate reviewer checked the new texture/narration/music source contracts and original outfit loader. Original defects are specified as observable operations, without guessing their normal-play frequency. [High-level AI](ai-data.md), [low-level AI](ai-low-data.md), [core audit](core-audit.md)

The gates caught a missing primary citation, corrupted table rows, an obsolete duplicate animation, an incorrect inherited594-member count and a verification gap in indirect jump-table/dispatch membership. Each was corrected and the affected gate rerun. The complete review and delivery record is in [WORKLOG.md](WORKLOG.md).

## Limits

No Scope J source contract remains partial. Original-game execution, save round-trips, pixel/audio parity and a replacement runtime were not performed. Original unchecked indices, stale/uninitialized storage, native COM/device behavior and floating-point precision effects are documented; their results across host environments require execution testing. Static instruction and asset checks support the recovered contracts but do not prove end-to-end equivalence.
