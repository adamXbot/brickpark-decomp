# F/G/H integration and execution validation — 2026-09-07

The user assigned F, G and H together and approved a new worktree on current
main, consolidation of the best branches, stronger verification and a
side-by-side original/rebuilt execution pilot.

Branch: `codex/fgh-integration`, in `.worktrees/fgh-integration`.
Base: fetched `origin/main` at `6a95613e`. Incorporated scope F at `cb2a9113`,
G at `180a40c6` and H at `fc4c2c80`, with no merge conflicts. Main's corrected
`MatMul3x3` marker is preserved. Original executable SHA-256:
`c50865b60bfcb26c0a7329a75fb772b10ae234af324f669901906e5abb0e2bd9`.

## Measured checkpoint

**40 of 48 assigned functions pass the combined gate; eight remain WIP.**
This is 83.33% of the assigned function count, not the overall game's progress.

| Scope | Exact assigned targets | Total |
| --- | ---: | ---: |
| F | 15 | 16 |
| G | 14 | 19 |
| H | 11 | 13 |
| Combined | 40 | 48 |

All 30 files compile without warnings at `/W3 /O2 /Gy /Gd`. Their official
whole-file audits pass. The combined check finds 419 annotated functions,
408 normalized exact bodies, and zero regressions of existing normalized exact
markers. There are 11 WIPs in these files: the eight assigned targets plus three
previously excluded functions. No WIP is promoted on execution evidence alone.

## Actual reconstruction correction

- **JungleCruise_UpdateRiverAnim, 0x00432d00:** H's prior body was 422 instructions,
  1466 bytes and zero normalized differences, yet its branches at indices 216
  and 329 went to the wrong arithmetic tails. The original branches at
  `0x00432fdc` and `0x00433176` lead to tails that also subtract 16 from the
  screen-height offset. Both seats 1 and 2 receive this adjustment; the previous
  C adjusted only seat 2. Add `soA.y -= 0x10` and `soB.y -= 0x10` to the two
  seat-1 cases. The compiler reuses the correct tails without changing the
  normalized instruction stream or total size. All branch destinations,
  widths, annotated addresses and 16 floating-literal references now pass.
  The file's 15 exact markers pass the official audit.
- **Execution regression:** 864 seat-block cases pass, covering both directions,
  16 headings, all seats, three animation indices and three initial heights.
  Reintroducing the historical C bug still passes normalized matching, but
  branch validation flags precisely indices 216/329 and execution fails all
  288 seat-1 cases. A wrong 22.25-degree constant also passes normalized matching
  and branch checks, but literal checks catch four references and execution
  catches 810 cases. Both negative controls are required by the test.

This is a reconstruction fix reproducing the original game's behavior, not a
change to the original game. It corrects the earlier H completion evidence.

## Execution pilot and shared investigation

- **SchoolCarBlockedAhead:** 2160 cases pass against the original code and a
  separate arithmetic model. Return values, car memory, helper-call counts,
  non-stack writes, preserved registers, stack balance and x87 state agree.
  Boundary, overflow, list-order, exceptional-heading and all four rounding
  modes are included. First-hit and exclusive-boundary mutations are detected.
  The real conversion helper at `0x458930` executes; replacing it with host
  integer truncation would be incorrect. The initial checkpoint had 25 normalized differences / 179 versus 180 bytes.
  The September 7 compiler investigation below closes that residual; the
  retained exact C also passes all 2160 cases and both negative controls.
- **Transfer F's empty-control-block scheduling technique into H:** 37 new
  LFEntrance candidates plus baseline test an otherwise empty condition/loop
  around `next`, the coordinates, and split qx/next definitions. 34 total
  variants retain 10 differences / 672 bytes; four have 95 / 675. None improves
  the original crosswise temporary allocation. Evidence:
  `scratchpad/fgh/shared-scheduling/probe.py` and `results.json`.
- **Transfer it into G's register-allocation problem:** 42 new BlockedAhead
  candidates plus baseline add empty conditions/loops at seven lifetime
  boundaries for `p`, `c` and `hit`. All 43 produce the same normalized object
  body, 25 differences / 179 bytes. No source change is retained. Evidence:
  `scratchpad/fgh/shared-scheduling/blocked.py` and `blocked-results.json`.
- **Address diagnostic over all nine WIPs:** comparing aligned relocation
  operands found no candidate wrong callee/global annotation. This is a
  diagnostic, not an exactness gate: unresolved operands and imperfectly
  aligned WIP instructions do not count as verified addresses.

These results do not establish that the remaining C spellings are impossible.
They bound the tested methods and give a reusable way to distinguish observed
behavioral errors from differences in the compiler's output.

## Compiler investigation: SchoolCarBlockedAhead is now exact

**0x00402490, 63 instructions, 180 bytes, 100%; official audit `[OK]`,
committed marker `// FUNCTION: LEGOLAND 0x00402490`.** All branch destinations
and instruction widths agree. The annotated list-head address and both
-65536.0f literal references pass; the two conversion calls execute the actual
original helper in the differential test. All seven functions in schoolcar4.c
pass the official audit, and the 30-file combined check finds zero regressions.

The new method reduced the function to its necessary types and compared its
compiler output with the full translation unit. Those outputs were identical.
A deliberately simplified linear-distance diagnostic exposed the desired
register arrangement with coordinate reloads, disproving the old claim that
only two arrangements were reachable. That diagnostic changed behavior and
was never a production candidate.

The useful search direction was to keep candidates with a new register
arrangement even when their total mismatch score was worse. Duplicating both
projection assignments across an `if (hit)` produced the original first-half
allocation, but added a dead load/test and changed later arithmetic. Refining
that join to the X assignment alone, with its conversion result already named,
reproduced the entire original function:

```c
stepX = (int)(c->ux * -65536.0f);
if (p)
    ahead.x = c->wx - stepX;
else
    ahead.x = c->wx - stepX;
ahead.y = c->wy - (int)(c->uy * -65536.0f);
```

Both arms intentionally assign the same value. No test or branch remains in
the generated code. This compiler-shaping source reproduces the original
`c=ebx, p=ebp, X=esi, Y=edi` allocation, the coordinate reloads at indices 37/40,
and the Y-first distance schedule. It does not claim to recover the original
author's exact source spelling.

| Controlled full-file variant | Instructions | Bytes | Normalized differences |
| --- | ---: | ---: | ---: |
| Retained join with separate X conversion | 63 | 180 | 0 |
| Same join, Y conversion also named | 63 | 180 | 0 |
| Same-value ternary instead of if/else | 63 | 180 | 0 |
| Negated join condition | 63 | 180 | 0 |
| Remove the else arm | 67 | 199 | 48, escaping layout |
| Replace the join with a plain assignment | 68 | 208 | 57, escaping layout |
| Move the X conversion inside the arms | 64 | 186 | 31 |

All controls use the required `/W3 /O2 /Gy /Gd` flags. Separate diagnostic
checks of C/C++, inlining settings, processor targets and speed/size choices
did not find a solution; no alternative flags were retained. Nested arithmetic
helpers, aggregate layouts, widened conversion spellings and local regrouping
also did not solve the function. The isolated proof and ablations supersede the
previous source comment's impossibility claim; the old evidence remains in Git.

Local reproduction evidence is in `scratchpad/fgh/compiler-lab/`: `lab.py`,
`joins.py`, `join_refine.py`, `join_parts.py`, `exact_reduce.py`, the per-candidate
side-by-side listings, `blocked-exact-execution.json`, and
`check-after-blocked.json`. No object files or game data are committed.

## Transfer check on the eight remaining targets

After the school-car match, 108 warning-clean candidates applied the same
identical-arm assignment technique at the remaining allocation and scheduling
sites: SpaceTower (18), LFEntrance (10), track mesh (20), Joust (16), Jungle
Cruise (8), Temple Slide (8), StepSchoolCar (12), and raster submission (16).
These included naming the input before the join and removing prior shims in
controlled scratch candidates. None improved its function's retained matching
result. Six initial C89 declaration-order failures were corrected or excluded
from those counts. No changes to these eight source bodies were retained.

This bounds this transfer attempt, not the reachable C spellings. The successful
school-car reduction also shows why a previous search floor should not be
reported as a proof of impossibility. Evidence: `compiler-lab/transfer.py`,
`transfer_more.py`, `transfer_final.py` and their result files under the local
scratch directory above.

## Remaining matching work

The subsequent [additional-media review](fgh-additional-media.md) identifies
distinct Czech and Japanese demo builds, the shared Dutch/German/Spanish build,
and the Focus reissue's compatibility setup. All eight target instruction and
branch layouts remain unchanged across the compared executable families, so
the media review does not change the matching checkpoint below.

### Resumed compiler pass — 2026-09-07

**Unchanged at 40/48. No production C, markers, shared declarations or compiler
flags changed.** The initial 30-file combined check again passes with 419
annotated functions, 408 normalized exact bodies and no normalized regressions.
Because the tracked C is unchanged at the end of this pass, that baseline also
describes the retained result. The five pre-existing findings outside the
assigned targets remain as documented below.

Local evidence: `scratchpad/fgh/resume-lab/`, including `baseline.json`, the
per-candidate C/listings, aggregate result JSON and a README describing the
experiment limits. There were 599 warning-clean comparison runs in the 13
sweeps (595 distinct full-source texts), including baseline and diagnostic
variants; none matched. This is an experiment count, not 599 independent
methods or verified equivalent implementations. Two additional callback-view
variants still failed compilation and were excluded. Several families repeat
earlier negative results; their counts must not imply new search coverage.

- **Track mesh (156):** scalar/record copies, same-size callback argument
  records and assignment-result variants retain the two familiar outcomes:
  the correct-sized plain record with the wrong allocation, or the volatile
  read with the six-index scheduling window and extra byte. No candidate
  achieves the required copy-before-store and both pushes from the saved
  register. Callback views were isolated to the scratch caller.
- **Raster submission (85):** cursor initialization positions and duplicate-arm
  consumers do not improve the 142-difference retained body. Changes in
  register allocation often add instructions or leave the original extent.
- **Joust (24):** predicate joins do not reproduce both required shared tails.
  The retained one-difference body remains four bytes too large; a sampled
  instruction score cannot establish the correctness of either destination.
- **Log-flume entrance (36) and Space Tower (39):** split coordinate definitions,
  duplicate-arm consumers, dead-local reuse and staged stores do not improve
  the retained ten-difference results.
- **StepSchoolCar (70):** partial four-byte record copies again either lose
  both dead stores or reproduce the existing eight-byte-copy result. This
  confirms a documented negative; it does not identify a new source form.
- **Temple Slide (134):** with world reads moved after the screen-coordinate
  call, a duplicate-arm field read conditioned on `r` or `ty` recovers the
  original first 121 instructions, including the order of the call and world
  loads. It retains an extra test and forces earlier stack cleanup, so it is
  not a match. Naming the loaded value before that join removes the effect
  and returns to the wrong loop-head allocation. Reusing dead coordinate
  locals and equivalent `ty` arithmetic also fail. Every sampled Temple
  candidate in this pass has an escaping layout; its reported byte count is
  a truncated comparison window, not its complete function size.
- **Jungle Cruise (55):** removing the queue pointer view, changing seat
  representations and adding duplicate-arm consumers do not recover the
  original allocation without added work. The unsigned narrow-seat probes
  are semantically excluded: decrementing zero wraps before the later array
  access. They are diagnostics only, regardless of their compilation result.

No lower score in a truncated window was treated as progress, and no behavioral
equivalence claim was made for an unverified diagnostic. These results bound
the attempted constructions; they do not prove that the remaining matches are
impossible. The retained code and the eight-function table below are unchanged.

| Function | Scope | Normalized differences | Bytes ours/original |
| --- | --- | ---: | ---: |
| SpaceTower_Activate | F | 10 | 698/698 |
| Coaster3D_BuildTrackMesh | G | 6 | 442/441 |
| Raster_SubmitPoly | G | 142 | 742/743 |
| StepSchoolCar | G | 83 | 1147/1147 |
| TempleSlide_Update | G | 18 | 1122/1122 |
| Joust_Update | G | 1 | 2262/2258 |
| LFEntrance_Activate | H | 10 | 672/673 |
| JungleCruise_Tick | H | 11 | 1111/1114 |

Joust's one normalized difference is not a one-byte fix: its conditional tail
branch and a separate case-0x16 walk-tail destination account for four excess
bytes. The earlier per-scope notes retain the detailed attempted forms.

The split into worktrees did not introduce a compile failure or a regression
of the existing matches. Consolidation is useful for sharing evidence and
verification, but it alone does not resolve these function-level code-generation
differences. Extending differential execution to the other eight WIPs remains
future work; this checkpoint does not claim they have all been behaviorally tested.

## Existing issues outside the 48 assigned targets

The expanded check flags five other functions. Recompiling their sources from
the base `6a95613e` reproduces the same issues, so none was introduced by this
integration. Their source is unchanged by this work; they are not included in
the 48-target completion count.

| Function | Existing finding |
| --- | --- |
| LFCorner4_Interact | branch destination at instruction 14 |
| LFTrack_Update | two address mismatches |
| LFPiece_TickCommon | four address mismatches |
| PlaneRide_Create | three address mismatches |
| Copters_Activate | eight address mismatches and three jump-table discrepancies |

Their normalized markers alone should not be treated as full correctness
evidence. The findings are recorded for the owners/integration review; they
were not silently included in the passing assigned-target count.

## Reproduction and artifacts

`validation/fgh/README.md` documents the checks and limitations. Its committed
`results/` directory contains the compact assigned-target checkpoint and both
execution reports. The code uses the existing matching utilities read-only;
no shared tool or progress report was modified. The emulator dependency was
installed in a temporary directory, leaving the project's virtual environment
unchanged. No game data, object files or experiment candidates are committed.
