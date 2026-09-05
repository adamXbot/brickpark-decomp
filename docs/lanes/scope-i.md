# Scope I — first checkpoint (2026-09-05)

Branch: `scope/I`, based on `f22f7cc7` (`origin/main`). Worktree:
`/Users/systemadmin/Documents/Development/Github/legoland/.claude/worktrees/scope-i`.
Scope F continues separately in `.claude/worktrees/scope-f` on `scope/F`.

## Result

Scope I has started. **One new exact function: `UpdatePersonPos`, 74
instructions / 213 bytes.** The rest of the scope is still partial; this is
an initial checkpoint, not a completion claim.

All twelve assigned files passed their baseline full-file audit and clean
`/W3 /O2 /Gy /Gd` compilation. The baseline contains **15** table entries,
although the scope prose says fourteen. Existing exact functions are preserved.

## Per-function status

Percentages below are strict index-for-index equality over the authoritative
extent, not aligned `matchfull` percentages. Counts and first divergent indices
are from this worktree. Except for `UpdatePersonPos`, these are baseline
measurements; an unworked row is not an exhaustion claim.

| Address | Function | Original instructions | Strict match | Audit OK | Marker | First divergence | Residual / checkpoint status |
| --- | --- | ---: | ---: | --- | --- | ---: | --- |
| 0x00471ca0 | `RemoveNewObjectMarker` | 53 | 90.6% | No | WIP-FUNCTION | 22 | 5; existing cursor-anchor floor confirmed by instruction reading; no repeated variants. |
| 0x004966a0 | `UpdateSampleSource` | 66 | 90.9% | No | WIP-FUNCTION | 46 | 6; four new snapshot / ordered-read variants regress to 27–28; retained baseline. |
| 0x004401b0 | `UpdatePersonPos` | 74 | 100.0% | Yes | FUNCTION | — | 0; exact after aggregate projection and removal of compensating volatile read. |
| 0x00482430 | `BuildPTPRoute` | 76 | 86.8% | No | WIP-FUNCTION | 16 | 10; allocation-only; six new field-read / temporary variants do not improve it. |
| 0x00499d60 | `UnlinkGardenerOrder` | 68 | 50.0% | No | WIP-FUNCTION | 34 | 34; baseline only, remaining work. |
| 0x004724a0 | `DrawPopUpInfo` | 962 | 98.6% | No | WIP-FUNCTION | 590 | 13; baseline only, remaining work. |
| 0x0048a3e0 | `GetObjectUID` | 191 | 89.5% | No | WIP-FUNCTION | 95 | 20; baseline only, remaining work. |
| 0x0046c7e0 | `LoadScriptEvent` | 124 | 79.0% | No | WIP-FUNCTION | 95 | 26; baseline only, remaining work. |
| 0x0046d850 | `ScrollIconPanel` | 121 | 71.1% | No | WIP-FUNCTION | 62 | 35; baseline only, remaining work. |
| 0x00470620 | `CheckWorkerOnMouseStatus` | 184 | 55.4% | No | WIP-FUNCTION | 102 | 82; baseline only, remaining work. |
| 0x0048f0f0 | `InitExitCheckBox` | 119 | 0.8% | No | WIP-FUNCTION | 0 | 118; existing notes already record five investigative passes; not unexplored. |
| 0x0045ff00 | `RenderCursor` | 454 | 39.9% | No | WIP-FUNCTION | 48 | 273; baseline only, remaining work. |
| 0x004608c0 | `PaintTileLayer` | 431 | 9.7% | No | WIP-FUNCTION | 27 | 389; baseline only, remaining work. |
| 0x0045b180 | `RenderView` | 903 | 57.8% | No | WIP-FUNCTION | 67 | 381; baseline only, remaining work. |
| 0x004567a0 | `RenderFullMap` | 1161 | 27.3% | No | WIP-FUNCTION | 0 | 844; baseline only, remaining work. |

## UpdatePersonPos — exact

- **Two unscaled isometric coordinates must be represented as one `Pos`
  local before either scaled output is stored.** `projected.x = bx - by;`
  then `projected.y = by + bx;` recovers `lea ecx,[ebx+ebp]` at instruction
  20 and all following projection instructions. The previous inline sums
  destructively reused bx (`add ebp,ebx`). The first aggregate experiment
  changed 74i/212B, strict 8 / rb 3 to 74i/213B, strict 6 / rb 2.
- **Remove a compensating volatile read when the underlying web is fixed.**
  On that aggregate source shape, plain `pos.x -= Get_XScroll()` closes the
  remaining six differences and both scroll blocks: strict/rb/ob = 0/0/0.
  Retaining the volatile with a separately named short result bottoms at
  three allocation differences, so it is not part of the recovered source.
- The staged body passes the authoritative audit, 100% `matchfull`, clean
  `/W3`, and an independent COFF check of **all 213 bytes after resolving all
  seven address relocations**. `sysmisc.c` exact count grows from 5 to 6;
  its other six functions retain their original audit results.
- Mechanics preserved: direction update; read map x/y before querying tile
  dimensions; isometric scale and signed shift by 9; subtract screen scroll;
  record depth before adding the screen origin and half the unsigned height;
  adjust the walking position; update animation frame only when bit 0x100
  of `flags62` is clear. No API, struct, global, or callee names/types changed.
  No original bug was corrected or new behavior inferred.

## First four: reconstruction and triage

- `RemoveNewObjectMarker`: 53i/155B versus 53i/154B, strict/rb/ob = 5/5/5,
  first 22. Read all original instructions. The five differences are one
  coherent source-versus-destination cursor anchor and its four offsets;
  both sides access the same elements. Preserve the original skip-one
  behavior after compaction. The existing note already measures pointer,
  lockstep-counter, temporary, increment-order and free-volatile families;
  its best 5-mismatch form remains at its recorded floor. Those experiments
  were not repeated.
- `UpdateSampleSource`: 66i/175B on both sides, strict/rb/ob = 6/3/6,
  first 46. Read all switch arms and their shared final store. Ordered
  volatile reads of all three inputs obtain the load ordering but rotate
  the result out of eax, preventing its one-store tail merge with case 1
  (28 strict). Two ordered reads, a plain whole-Pos snapshot, and ordered
  x reads with a plain y give 28, 28, and 27 respectively. All retain the
  unknown-kind uninitialized-position behavior. Baseline retained; no
  exhaustive new floor proof is claimed.
- `BuildPTPRoute`: 76i/152B on both sides, strict/rb/ob = 10/0/10,
  first 16. Read the parent walk and all shortcut arms. A named c->x/c->y
  pair in either read order is unchanged at 10; a Pos wrapper costs 13;
  a free volatile c->x read costs 11, c->y remains 10, and a volatile
  parent read costs 11 plus a byte. The existing source-order and pointer
  copy negatives stand: this is a b/c allocation-rank floor for the tested
  families. No extra guard or reference was added to change behavior.

Scratch comparisons resolve stack homes by control-flow push depth for the
reachable direct branches. The switch arms of `UpdateSampleSource` were
also checked explicitly: all enter with an 8-byte local frame and one saved
register. Blind scores for the larger, unworked rendering bodies are not
used to claim a classification; their jump tables and imported calls require
additional control-flow handling.

## Brief discrepancy and next work

`InitExitCheckBox` is described in the scope as unexplored and essentially
wrong. Its source actually records five passes proving that the large strict
count is predominantly an instruction shift from a missing saved zero
register, not 118 independent reconstruction errors. Its semantic body is
116 instructions; the audit's 119-instruction comparison includes trailing
padding. Read that record before considering another variant search.

Next in the assigned order: `DrawPopUpInfo`, then `GetObjectUID`,
`LoadScriptEvent`, `UnlinkGardenerOrder`, and the remaining UI/render bodies.
For `RenderView`, the brief specifically requests testing the two recorded
statement-placement corrections together. For `RenderFullMap`, pursue the
six listed reconstruction facts rather than the known cold-arm layout floor.
These bodies received baseline audits only at this checkpoint.

## Validation and scope boundaries

Baseline: 59 existing exact functions across twelve files.
All experimental source copies were audited as whole files, with no decline
in existing exact counts. Final `sysmisc.c`: PASS, 6 exact / 1 partial.
Only its assigned WIP body and its explanatory note were changed. All
previously exact bodies, other scope files, shared documentation, tools,
original executable, main checkout and Scope F worktree were left untouched.
Scratch evidence remains under `scratchpad/scope-i/`; compiler objects use
`/tmp/si_` names (audit.py uses its own safe per-process names). No game
binary or scratch output is included in the checkpoint.
