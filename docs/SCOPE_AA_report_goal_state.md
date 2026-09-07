# Scope AA — report / goal-state table tier (inventory group 12) (2026-09-07)

> **Status: DONE — 14 of 14 exact, closed into `main` 2026-09-07.** Branch
> `scope/AA`. Notes: `docs/lanes/scope-aa.md`. Object prefix `/tmp/saa_`.
> Cut from inventory group 12 (`tools/inventory.py` / `docs/lanes/scope-n.md`).

**Read `docs/PARALLEL_CONTRACT.md` first; it carries everything not written
here** — including the relocation step of the gate
(`$PY tools/relocs.py LEGOLAND/<file>.c`, zero `MISMATCH` lines).

NEW-FUNCTION scope, **14 functions, ≈1,580 instructions** (one dead stub),
one new file. `ResetAppraisalDeadline` (0x0044db40) is **already exact** in
`eventgoalprim.c` — do not recreate it.

## What this tier is

The cluster sits between `LoadReport` / appraisal helpers (scope Y) and the
matched blokelist / sweep neighbours. Several bodies are already named by
callers:

- `movie3.c`: `ClearSim832b9c` (0x0044db20), `ClearAppraisalState`
  (0x0044db80), `SetLevelGoalState` (0x0044dc70)
- `gameframe.c`: `sub_44db90` (0x0044db90) — "the appraisal-due tick"
- `blokelist.c` header documents action-table entries at 0x0044f170 /
  0x0044ebf0 and the seat-list "join" at 0x0044f4a0

Tables at **0x004b8300** / **0x004b836c..0x004b8380** reach several of the
larger bodies; name handlers from what they do and from those table slots.

**Do not attempt 0x004453a0** (the 8,085-instruction appraisal screen) —
unassignable until the matcher takes a window parameter.

## `LEGOLAND/goalstate.c` — clear/set helpers, table handlers, seat join

| address | provisional name | insns | reached by; evidence |
| --- | --- | ---: | --- |
| 0x0044db20 | `ClearSim832b9c` | 2 | `ResetLevelGlobals` (movie3.c name) |
| 0x0044db30 | `sub_44db30` | 2 | DEAD — match anyway |
| 0x0044db80 | `ClearAppraisalState` | 4 | movie3.c; zeroes 0x00832978 and `g_instant_appraisal` |
| 0x0044db90 | `AppraisalDueTick` | 60 | gameframe.c `sub_44db90`; rename from body |
| 0x0044dc70 | `SetLevelGoalState` | 9 | movie3.c |
| 0x0044ebf0 | `sub_44ebf0` | 92 | table at 0x004b8370; blokelist action table |
| 0x0044ed00 | `sub_44ed00` | 32 | called by 0x0044f610 |
| 0x0044ed70 | `sub_44ed70` | 328 | table at 0x004b8374 |
| 0x0044f170 | `sub_44f170` | 3 | table at 0x004b836c |
| 0x0044f180 | `sub_44f180` | 201 | called by 0x0044f610 |
| 0x0044f3d0 | `sub_44f3d0` | 15 | `RemoveBlokeFromRide` (rides.c) |
| 0x0044f400 | `sub_44f400` | 14 | called by 0x0044f610 |
| 0x0044f4a0 | `JoinSeatList` | 121 | blokelist.c "join" routine; rename from body |
| 0x0044f610 | `sub_44f610` | 699 | table at 0x004b8380 — largest; leave until siblings land |

**Order:** the five tiny clear/set stubs first → table micro-stubs →
medium helpers → `JoinSeatList` → `sub_44f610` last.

Sizes are the inventory's; `tools/matchfull.py` prints the authoritative
extent. Confirm callers from disassembly (`tools/disasm.py
original/legoland.exe 0x<RVA> <n>`, RVA = VA − 0x400000).

## Owned elsewhere — do not create or edit

`eventgoalprim.c` (owns 0x0044db40), `appraisal.c` / `reportset.c` (Y),
`eventtick.c` / `eventgoal.c` (V), every F/G/H ride file, Codex-F's files
(`coaster10.c`, `ridemachine2.c`, `uistubs2.c`), `softblit2.c`, every
existing `.c`. Declare callees `extern` with address comments.
