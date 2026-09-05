# Scope I — complete partial-scope pass (2026-09-05)

Branch: `scope/I`, based on `f22f7cc7fa95f2d5740f89f4b53ae2624cc9e474`.
Worktree: `/Users/systemadmin/Documents/Development/Github/legoland/.claude/worktrees/scope-i`.
Scope F remains in its separate `scope/F` worktree.

## Result and stopping rule

**All 15 assigned functions across 12 files have been reviewed: one new exact,
two improved partials, and twelve retained partials.** The brief's prose says
fourteen; its table contains fifteen. This completes the partial-scope pass under
`docs/PARALLEL_CONTRACT.md`: investigate reconstruction errors, measure useful
new source forms, and retire residuals with evidence. It does not mean all
fifteen functions are exact. Fourteen remain honestly marked `WIP-FUNCTION`.

`UpdatePersonPos` now matches all 213 original bytes. `PaintTileLayer` removes
a compensating volatile read and recovers the tile-loop register roles.
`RenderFullMap` improves the ILF address carrier. All **59 previously exact
bodies are preserved verbatim**, and the final twelve-file audit has **60 exact
functions**, zero extent failures, and clean `/W3` compilations.

“At its floor” below means the best measured form after the listed new tests
and review of earlier eliminated families. It is not a proof over all possible
C programs. No row is left as an uninvestigated baseline; reopen these floors
only with new reconstruction or compiler evidence.

## Final per-function measurements

Instructions are the authoritative original extent. Bytes are compiled/original.
Strict match is normalized, index-for-index equality; relocation values are
normalized by the audit. Residual is baseline → final strict mismatches. First
is the zero-based first divergence. Marker and audit `[OK]` are shown separately
from a whole-file PASS, which alone does not make a WIP exact.

| Address | Function | Instructions | Bytes | Strict match | Residual | First | Marker / audit OK |
| --- | --- | ---: | ---: | ---: | ---: | ---: | --- |
| 0x00471ca0 | `RemoveNewObjectMarker` | 53 | 155/154 | 90.6% | 5 → 5 | 22 | WIP-FUNCTION / No |
| 0x004966a0 | `UpdateSampleSource` | 66 | 175/175 | 90.9% | 6 → 6 | 46 | WIP-FUNCTION / No |
| 0x004401b0 | `UpdatePersonPos` | 74 | 213/213 | 100.0% | 8 → 0 | — | FUNCTION / Yes |
| 0x00482430 | `BuildPTPRoute` | 76 | 152/152 | 86.8% | 10 → 10 | 16 | WIP-FUNCTION / No |
| 0x00499d60 | `UnlinkGardenerOrder` | 68 | 198/193 | 50.0% | 34 → 34 | 34 | WIP-FUNCTION / No |
| 0x004724a0 | `DrawPopUpInfo` | 962 | 3141/3141 | 98.6% | 13 → 13 | 590 | WIP-FUNCTION / No |
| 0x0048a3e0 | `GetObjectUID` | 191 | 477/477 | 89.5% | 20 → 20 | 95 | WIP-FUNCTION / No |
| 0x0046c7e0 | `LoadScriptEvent` | 124 | 323/319 | 79.0% | 26 → 26 | 95 | WIP-FUNCTION / No |
| 0x0046d850 | `ScrollIconPanel` | 121 | 304/304 | 71.1% | 35 → 35 | 62 | WIP-FUNCTION / No |
| 0x00470620 | `CheckWorkerOnMouseStatus` | 184 | 672/672 | 55.4% | 82 → 82 | 102 | WIP-FUNCTION / No |
| 0x0048f0f0 | `InitExitCheckBox` | 119 | 480/469 | 0.8% | 118 → 118 | 0 | WIP-FUNCTION / No |
| 0x0045ff00 | `RenderCursor` | 454 | 1438/1429 | 39.9% | 273 → 273 | 48 | WIP-FUNCTION / No |
| 0x004608c0 | `PaintTileLayer` | 431 | 1322/1315 | 12.3% | 389 → 378 | 14 | WIP-FUNCTION / No |
| 0x0045b180 | `RenderView` | 903 | 2893/2880 | 57.8% | 381 → 381 | 67 | WIP-FUNCTION / No |
| 0x004567a0 | `RenderFullMap` | 1161 | 4216/4225 | 28.8% | 844 → 827 | 0 | WIP-FUNCTION / No |

## New exact: UpdatePersonPos

- **Represent the two unscaled isometric coordinates as one `Pos` before
  storing either scaled output.** `projected.x = bx - by` followed by
  `projected.y = by + bx` recovers `lea ecx,[ebx+ebp]` at instruction 20.
  The prior inline sums destructively reused bx. This changes the baseline
  74i/212B, strict 8 / register-blind 3 to 74i/213B, strict 6 / rb 2.
- **Remove the compensating volatile read after correcting the coordinate
  representation.** Plain `pos.x -= Get_XScroll()` then closes the remaining
  six differences: strict/rb/ob = 0/0/0. Keeping the volatile read and naming
  the short return still leaves three allocation differences.
- Authoritative audit `[OK]`, full-body `matchfull` 100%, clean `/W3`, and an
  independent COFF relocation check of **all 213 bytes and seven address
  relocations** agree. `sysmisc.c` grows from five to six exact functions.
- Mechanics retained: update direction; read map x/y before tile dimensions;
  form and scale the isometric pair with signed shift by 9; subtract scroll;
  record depth before screen origin and half the unsigned height; adjust
  walking position; update the animation frame only with flags62 bit 0x100
  clear. No additional behavior or bug fix was introduced.

## Residual triage and measured levers

The reconstruction pass read the original instructions and the existing notes
before experiments. Previously eliminated families were not repeated. Scratch
sources were audited as whole files so each experiment preserved existing exact
functions. The retained source is the best measured legal form, with original
bugs and call mechanics preserved.

### RemoveNewObjectMarker

- At its recorded cursor-anchor floor, 5/53 strict
mismatches (rb 5, ob 5), first 22, 155/154 bytes. Instruction reading
confirms that all four array accesses remain equivalent; the skip-one bug
is preserved. The recorded pointer/counter/order/volatile families already
test the coupled anchor and induction scheduling; they were not repeated.

### UpdateSampleSource

- At its measured scheduling/tail-merge floor,
6/66 strict (rb 3, ob 6), first 46, 175/175 bytes. Ten additional ordered
input/snapshot/aggregate spellings were measured. Three ordered volatile
reads get the desired input order but rotate the final y out of eax and
lose the case-1 tail merge (28 strict). Ordinary pair accumulation/copy,
input and scroll pairs, and named accumulators give 27-32. Keep this form;
changing the shared tail to improve one arm regresses the other.

### BuildPTPRoute

- At its measured allocation floor, 10/76
strict, rb 0, ob 10, first 16, 152/152 bytes. Naming c's two field values
in either order stays at 10; grouping them in Pos costs 13; a free
volatile read of c->x costs 11, c->y stays at 10, and the parent read
costs 11 plus one byte. These extend the recorded declaration/loop/store
order negatives without adding a guard or a non-original reference.

### UnlinkGardenerOrder

- At its measured shared-tail placement floor,
34/68 strict (rb 34, ob 34), first 34, 198/193 bytes. An additional
zero-trip do/while exit around the head/search join costs 45 and flips the
head guard (first 23); it cannot make the early head tail survive. Existing
identical-tail and source-order evidence still applies. The diagnostic's
missing vararg is preserved.

### DrawPopUpInfo

- At its measured 13/962 scheduling floor,
first 590, 3141/3141 bytes. A new non-volatile escape was tested by making
halfw a sibling of an EXISTING escaped name/info/line buffer, rather than
dead padding. It forces a real ordinary memory home, but the aggregate
repositions the frame: name/info score 904 and both line member orders
score 421 (3155 bytes). It cannot retain the original halfw slot. No
additional call or global store was introduced; the best form stays here.

### GetObjectUID

- At its measured two-load placement floor,
20/191 strict, rb 8, ob 20, first 95, 477/477 bytes. Naming horizontal
coordinates in plain locals or a Pos remains byte-identical. Free
volatile reads of the horizontal x or y input give 96/187 and enlarge
the body. Together with the documented row-table and map-pin experiments,
this rules out the remaining cheap call-argument temporary route.

### LoadScriptEvent

- At its measured cold-block-order floor,
26/124 strict, first 95, 323/319 bytes. Moving the entire terminator
handler out of the loop behind a goto gives the SAME object. A saved read
result with a break and shared post-loop dispatch gives 37 or 47 by
polarity, moving the first mismatch to 85/69. The original two-predecessor
head-zero join cannot be placed last by these forms; keep the proven
ownership cleanup and name-error leak rather than trading them for score.

### ScrollIconPanel

- At its measured four-value allocation floor,
35/121 strict, first 62, 304/304 bytes. In addition to the recorded
nx/ny aggregate and write-back sweeps, carrying SnapIconScroll's RESULT
in a one-field struct or Pos is byte-identical. Its web rank does not
move. Keep the asymmetric horizontal/vertical snapping and short ABI.

### CheckWorkerOnMouseStatus

- At the recorded dead-rematerialization floor. Baseline audit confirms 184i/672B and 82 strict mismatches, first 102;
this is the missing mov ebp,1 plus the cmp/test choice described in N+6,
not 82 independent errors. The out-of-range height already clobbers ebp
before the join, so a new live consumer would change the original work.
The six recorded passes were reviewed, not repeated.

### InitExitCheckBox

- At the recorded constant-web floor. Contrary
to the scope brief's "unexplored" description, five prior passes already
investigated this body. Re-read the original: three dword zero stores,
no loop-weighted fourth use or byte consumer, and no second reaching
definition for the zero variable. Audit stays 118/119 strict, first 0,
480/469 bytes; the real emitted code has 116 instructions. No extra
fourth store, narrowed store, or fabricated saved-register use was added.

### RenderCursor

- At its measured scratch-rotation floor plus
the already-retired depth-5 merge limit: 273/454 strict, first 48,
1438/1429 bytes. A named x accumulator, named y accumulator, and separate
origin Pos copy are each byte-identical. They do not change the first
scratch temporary. No sixth tail instruction or extra callback was added;
the documented corpus proof for the five-instruction merge still stands.

### PaintTileLayer

- Improved, then stopped at the remaining
allocation/frame floor: 378/431 strict, first 14, 1322/1315 bytes, no
ESCAPES. The old volatile halfw shim is REMOVED. Grouping halfw with the
existing escaped tile Pos keeps an ordinary memory home and refuses the
unwanted second-cell induction variable with the original 0x34 frame.
Keeping dx/dy between halfw and tile gives the best measured form below.
The tile loop now uses the original ecx/ebx/edi roles. With CFG-resolved
stack homes, edit distances (strict/rb/ob/both) improve from
187/59/179/46 to 90/45/77/32; raw strict improves 389 -> 378. Grouping all
seven intervening homes regresses to 395 with ESCAPES and was rejected.
The remaining head allocation and frame homes do not close. The second
cell's unshifted object-without-callback bug remains unchanged.

### RenderView

- At the measured geometry/prologue and
frame-reference floor, 381/903 strict, first 67, 2893/2880 bytes. BOTH
documented statement-placement corrections were tested together: limits
after the quadrant switch (y first), and sort-count reset after base in
the non-null object arm. Result: strict 549, first 5, 2887 bytes. Proper
CFG/stack/import-aware edit distances all regress: strict/rb/ob/both
321/189/249/90 -> 353/220/287/118. The paired experiment does not solve
the split prologue. Retain this baseline with the two known, behaviorally
unobservable placement discrepancies explicitly documented above.

### RenderFullMap

- Improved, then stopped at the documented
shared-tail layout and register/frame floors: 827/1161 strict,
first 0, 4216/4225 bytes, no ESCAPES. An explicit ILFTable carries the
sprite+8 address to the loop condition, re-derived at each latch. VC6 now
emits add eax,8 / mov eax,[eax] there; the head still CSEs the initial
count load, so this is a partial reconstruction of the address carrier.
Raw strict improves 844 -> 827, both-blind region cost 465 -> 455, and
CFG/stack/import-aware edit distances improve 666/568/536/406 ->
664/562/526/401 (strict/rb/ob/both). No volatile access was added.
All six register facts were addressed: ordinary escaped scale homes
restore uncached LineTo calls but cost 873-880; escaping def or chain
costs 897/1066; a shared BPos copy is inert at 844; a free def reload
costs 845; forcing the ILF head load volatile costs 846. Combining the
scale/def home forms with the address loop costs 876-884. These changes
do not solve the independently-retired cold-arm layout bit; those regressions are rejected.
The stack frame remains 0xf4 against the original 0xf8, and the current
address spelling is the best measured reachable improvement.

## Rendering evidence and comparison method

- **Resolve stack homes through control flow, including jump tables and
  imported-call cleanup.** The final renderer comparisons use COFF relocation
  targets for switch tables and stdcall imports, cached import-call registers,
  and the original IAT addresses. Push depth agrees at branch joins; there are
  no conflicting reachable stack depths. Absolute frame homes are relative to
  entry ESP. Register normalization preserves 8/16/32-bit widths; offset-blind
  comparison suppresses stack-home identities, not object-field offsets.
- **Distinguish index shifts from independent errors.** The strict counts in
  the table come from the authoritative audit. Renderer edit distances use
  dynamic programming with insertion, deletion and substitution cost one.
  Their “strict” column already resolves stack homes, so it is a diagnostic
  metric distinct from the table's raw strict count. SequenceMatcher region
  costs are called out separately and are not substituted for edit distances.
- `PaintTileLayer`: raw strict 389 → 378, frame still 0x34. Corrected-home
  edit distances strict/rb/ob/both: 187/59/179/46 → 90/45/77/32. The selected
  aggregate groups `halfw`, `dy`, `dx`, and the already escaped `tile`; it
  preserves ordinary `halfw` reloads without the derived second-cell IV.
- `RenderView`: both requested statement-placement corrections were tested
  together, not inferred from separate failures. The paired source regresses
  both raw strict 381 → 549 and all four corrected-home edit distances.
  The retained WIP explicitly documents both unobservable placement
  discrepancies; this pass does not claim they were corrected.
- `RenderFullMap`: all six observations received concrete source experiments:
  (1) ordinary escaped scale homes, including dimensions and tile-bound forms;
  (2) a def spill grouped with escaped tile data and a free def reload;
  (3) a shared BPos copy hoisted before dispatch, retaining its two-byte type
  and original dword argument push, without reading beyond it through a u32;
  (4) an escaped chain home; (5) uncached LineTo imports, achieved with the
  scale/dimension form but regressing the rest of the function; (6) an explicit
  sprite+8 address slot in the ILF loop. Only (6) improves the retained body.
  Combinations with scale/def homes still regress. The initial ILF count load
  remains CSE'd; only the latch gains `add eax,8 / mov eax,[eax]`.
  Raw strict 844 → 827; corrected-home edit distances
  666/568/536/406 → 664/562/526/401; both-blind SequenceMatcher region cost
  465 → 455. The independently retired shared-tail layout problem remains.

## Mechanics, names, ABI and original behavior

The only changed C bodies are `UpdatePersonPos`, `PaintTileLayer` and
`RenderFullMap`; all other changes are evidence notes immediately above assigned
markers. The retained mechanics of every other body are those already recorded
in its source notes and original disassembly: marker removal/compaction,
sample-source projection, parent-path routing, popup rendering, object UID
probes, script-event deserialization, gardener-order unlinking, icon scrolling,
worker mouse status, exit checkbox initialization, cursor rendering, and view
rendering. No callee, global, symbol or function was named or renamed. No extern
prototype, shared struct or ABI declaration changed.

- `PaintTileLayer` retains clipping and quadrant selection, its two-cell tile
  loop, path overlays, terrain and bridge drawing, and rendering lock/status
  calls. Only local storage changes. In particular, the second cell's object
  with no draw callback still draws at the original unshifted `(px, py)`.
- `RenderFullMap` retains its drawing order, callbacks, scaling and
  original ILF layer loop. The sprite+8 slot is recomputed at the latch; no call
  or store intervenes between the latch test and the next body reload, so the
  explicit carrier preserves the old pointer-read behavior.
- Other recorded oddities are preserved: `RemoveNewObjectMarker` skips the
  next shifted entry after compaction; `UpdateSampleSource` retains the unknown
  kind's uninitialized position; `LoadScriptEvent` retains the name-error leak
  and its raw-free versus destructor distinction; `UnlinkGardenerOrder` retains
  the diagnostic's missing vararg; `ScrollIconPanel` retains its asymmetric
  axis snapping and short return ABI. No extra consumer/store/callback was
  fabricated to force a desired register or merge.

## Validation and integration handoff

Final whole-file `audit.py` counts, all PASS with zero extent failures:

| File | Exact before | Exact after |
| --- | ---: | ---: |
| fpui5.c | 4 | 4 |
| sysmisc.c | 5 | 6 |
| workorder3.c | 1 | 1 |
| popup.c | 2 | 2 |
| objmap2.c | 14 | 14 |
| savechunks2.c | 1 | 1 |
| fpui4.c | 6 | 6 |
| workers2.c | 9 | 9 |
| screens2.c | 11 | 11 |
| bigrender.c | 4 | 4 |
| render4.c | 2 | 2 |
| renderview.c | 0 | 0 |
| **Total** | **59** | **60** |

All twelve files compile without warnings under VC6 SP3 `/W3 /O2 /Gy /Gd`.
A boundary comparison against `f22f7cc7` confirms every previously exact body is
verbatim identical and every non-comment C token outside the three changed
assigned bodies is identical. `git diff --check` is clean. No shared docs,
tools, original executable, game assets, main checkout or Scope F files were
edited by this scope. Compiler objects use `/tmp/si_` or audit's per-process
paths; scratch sources and evidence are untracked under `scratchpad/scope-i/`
and are not part of the branch commits.

**Integration remains the integrating session's responsibility.** The parallel
contract explicitly forbids this scope from running `verify.py`, `progress.py`
or `coverage.py`, or editing the generated progress reports. None was run or
edited. The GitHub workflow's `progress.py --check` compares generated HTML/SVG
with marker counts and source lines; those reports necessarily need the
integration refresh after this change. Therefore this branch is ready for PR
review with its scope checks complete, but it does not claim a green generated
report check or final integrated verification. The integrating session should
merge, run verification alone, and regenerate the reports as the contract says.
