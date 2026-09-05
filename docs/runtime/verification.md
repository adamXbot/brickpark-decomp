# Scope J verification

Baseline: `f8f5854481b2a87fb456a37b02ce581e9206b400`; branch `scope/J`; completion review date 2026-09-05. This verifies the documentation deliverable required by [Scope J](../SCOPE_J_runtime_spec.md). No compiler or original-game execution was run.

## Requirement completion

All192 tracked C files have a primary specification entry. The initial183-source pass and the current-main delta review covered all9 new sources,12 changed sources and8 new lane notes. The completion pass reviewed the recovered headers, decoded tables, behavior and original-bug notes, and WIP boundaries; parallel subsystem reviews closed recoverable omissions. The five required categories—layouts, rules/state machines, tables/constants, original bugs and callback slots—are accounted for in the [core](core-audit.md), [transport](transport-audit.md), [attraction](attractions-audit.md) and [presentation](presentation-audit.md) audits. The26 lane notes and the named format/callback/host references are indexed in [coverage](coverage.md).

“Partial” in source coverage identifies remaining source evidence limits, including absent table values, opaque fields or missing external bodies. It does not mean that a documentation section was deferred. Cross-page ownership, native service calls and historical matching residuals alone do not make the recovered contract partial. The per-file reasons replace the first pass's broad generic labels. [Coverage definitions](coverage.md), [outstanding boundaries](../RUNTIME_SPEC.md#outstanding-recovery-boundaries)

## Reproducible gate

The complete read-only Python gate is published in [checks.md](checks.md). It passes on the completed documentation:

- Every local Markdown destination and heading anchor resolves; table column counts and Git whitespace checks pass.
- All192 tracked C sources occur exactly once as primary coverage rows and are cited on their primary pages.
- All628 linked callback function names and virtual addresses match source definitions. All618 final direct class-slot assignments match the four registration providers; the three alternate library registrations were reviewed separately. Two power-station add handlers retain declaration names/addresses because their bodies are absent. [Callback matrix](callbacks.md)
- Source fingerprints recorded in audit rows match the current tree. Twelve named-topic presence checks pass; these check discoverability, not behavior equivalence.
- Every changed path is `docs/RUNTIME_SPEC.md` or Markdown below `docs/runtime/`. No C, tool, shared report or unrelated worktree file is changed.

The independent audits additionally check subsystem section order, decoded table cardinalities, source-ledger membership and selected arithmetic invariants. The attraction review verified eleven invariants, including one-based seat displacement, pan offsets, tower serializer aliases, Restaurant2 table overlap, cafe seat coverage, Joust timing and Balloonz platforms. Transport checked road/topology counts and all31 WIP notes. Presentation checked all57 source fingerprints and12 WIP notes. Their detailed results are recorded in the linked audits.

## Skeptical review corrections

The first review corrected ODF DLL/custom branching, worker-save removal and position copying, movement direction encodings, LoadPos scalar interpretation and the library null-clearing exception. The completion review then reconciled MapStats count offsets, BuildWalkPath interpolation, MoveLine units/completion limits, destructive narration pause, popup work-order payloads, two-bit z commands, fatal tiled rendering, carousel discharge, temple-slide allocation, Restaurant2 index/queue behavior, zero-based saved BNV references, school-car speed limits and transport geometry/raster tables. Current-main integration further corrected route energy/motor derivatives, model length outputs, Carousel allocation, live Copters path ordinals, Boating School capacity mutation, WAV framing and the numeric path-side mapping. These corrections are source-linked beside their contracts and in the [disagreement register](../RUNTIME_SPEC.md#reconciled-disagreements).

The gate initially caught a missing primary-page citation for sweep1; the completed palette contract restores that citation. This confirms the source-coverage check can fail rather than merely counting rows. Final checks were rerun after integration. The worktree and staged review are recorded in [WORKLOG.md](WORKLOG.md).

The documentation work is complete for this source snapshot. Save compatibility, pixel parity, full external-data recovery and execution equivalence remain untested, as required by the documentation-only scope.
