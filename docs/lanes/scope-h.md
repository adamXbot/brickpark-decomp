# Scope H — log flume, jungle cruise and animation partials

Started 2026-09-05 from `origin/main` at `f22f7cc` on branch `scope/H`,
in `/Users/systemadmin/Downloads/legoland/legoland-scope-h`. Scope G's
checkout and uncommitted changes remain separate. Ownership follows
`docs/SCOPE_H_partials_flume_jungle.md` and `docs/PARALLEL_CONTRACT.md`.

**First checkpoint:** `LFCorner_Place` is now exact (574 instructions /
1825 bytes). Whole-file gates pass with **169 exact functions and 12 WIPs**;
all 168 previous exact bodies remain intact. This starts scope H; the twelve
remaining partials are not claimed complete.

## Stage plan and checks

1. Verify the original binary and existing toolchain; audit the nine owned
   files and save baseline output under `scratchpad/scope-h/`. Require PASS
   and preserve each file's exact-function count.
2. Read the existing notes and original instructions before testing new
   levers. Investigate independent files in parallel. Keep experiments in
   scratch; audit the whole file after each owned source edit. Retain only
   semantically faithful improvements or useful bounded negative findings.
3. Audit all owned files, compile changed files at `/W3`, and check the diff
   against the thirteen allowed WIP bodies and notes. Record measured results
   and remaining limits here. No integration-wide reports or tools are edited.

## Environment

The Documents checkout and Python path in the shared contract are stale on
this host. Reuse the equivalents already verified by scope G; no installation:

- Python: `/opt/homebrew/opt/python@3.14/bin/python3.14` (capstone 5.0.7).
- `LEGOLAND_CL=/Users/systemadmin/Downloads/alpha team/alphateam/tools/wibo-msvc/cl`.
- Local `toolchain` symlink points to the existing Alpha Team toolchain.
- Local `original` symlink points to `../legoland/original` (never stage it).
- Original executable SHA-256 verified as
  `c50865b60bfcb26c0a7329a75fb772b10ae234af324f669901906e5abb0e2bd9`.
- Explicit iteration objects use `/tmp/sh_` prefixes; audit uses its built-in
  per-process object paths, allowing concurrent scope G compiles.

## Baseline

All nine files pass the authoritative gate. There are 168 existing exact
functions and thirteen owned partials. The per-file exact counts are:

| File | Exact functions |
| --- | ---: |
| logflume.c | 82 |
| logflume2.c | 55 |
| logflume3.c | 2 |
| logflume4.c | 2 |
| lfentrance.c | 6 |
| junglecruise.c | 13 |
| ridecb2.c | 4 |
| jcroute.c | 1 |
| anim2.c | 3 |

Full baseline output: `scratchpad/scope-h/baseline-audit.txt` (local scratch).
Percentages below use strict matching instruction positions, not the
difflib-aligned percentage reported by `matchfull.py`.

## First checkpoint per-function measurements

These are the auditor's sampled extents. The padding/table/truncation caveats
below distinguish sampled counts from complete executable bodies. Only the
corner is promoted; every other target retains `WIP-FUNCTION`.

| Address and function | Insns ours/original | Bytes ours/original | Strict mismatches | First index | Strict match | Audit / marker |
| --- | ---: | ---: | ---: | ---: | ---: | --- |
| 0x0040ca60 LFTrack_DrawAlt | 144/144 | 401/401 | 2 | 84 | 98.6% | not exact, WIP |
| 0x0040f050 LFTunnel_Place | 217/217 | 680/680 | 5 | 16 | 97.7% | not exact, WIP |
| 0x0040dc00 LFCorner_Place | 574/574 | 1825/1825 | 0 | — | 100.0% | [OK], FUNCTION |
| 0x0040bf70 LFEntrance_Activate | 222/222 | 672/673 | 10 | 14 | 95.5% | not exact, WIP |
| 0x0040abf0 LFEntrance_Remove | 102/102 | 343/343 | 11 | 39 | 89.2% | not exact, WIP |
| 0x00410180 LFDrop_Place | 111/111 | 344/341 | 44 | 1 | 60.4% | not exact, WIP |
| 0x0040a600 LFEntrance_Add | 256/256 | 809/812 | 206 | 2 | 19.5% | not exact, WIP |
| 0x00436dc0 JungleCruise_UpdateRiverTile | 116/116 | 367/367 | 27 | 10 | 76.7% | not exact, WIP |
| 0x00432d00 JungleCruise_UpdateRiverAnim | 422/422 | 1457/1466 | 112 | 111 | 73.5% | not exact, WIP |
| 0x00435750 JungleCruise_Tick | 354/354 | 1109/1114 | 208 | 110 | 41.2% | not exact, WIP |
| 0x00437260 JungleCruise_TraceRoute | 130/130 | 339/339 | 105 | 0 | 19.2% | not exact, WIP |
| 0x00418fe0 BoatingSchool_DrawBoats | 212/212 | 744/744 | 30 | 82 | 85.8% | not exact, WIP |
| 0x00442040 AnimApplyPart | 331/331 | 1152/1174 | 182 | 22 | 45.0% | not exact, WIP |

## Initial investigation

- **0x0040ca60 `LFTrack_DrawAlt`: at its existing floor under the six new
  probes.** 144/144 instructions, 401/401 bytes; 142 matching positions
  (98.6%), strict mismatch 2, register-blind 0, first divergence 84 (also 86).
  WIP retained; not audit `[OK]`. The original's `mov ecx,[esp+1ch]` and
  `push ecx` remain `eax` in the reconstruction. The original body was
  reviewed throughout, including the branch-local `mode` inconsistency.
  A pointer to `mode` inside the sprite guard, the same pointer before the
  preceding draw call, a pointer to `fwd->sq.b`, named far/near list pointers,
  a pointer to the final sprite-array element, and a named `b->piece` value
  all produce exactly the baseline relocated function bytes. Two exact
  neighbours and clean `/W3` preserved. No implementation change retained.
- **0x0040abf0 `LFEntrance_Remove`: the four new probes leave the existing
  scheduling limit unchanged.** 102/102 instructions, 343/343 bytes;
  91 matching positions (89.2%), strict 11, register-blind 10, first 39.
  WIP retained; not audit `[OK]`. The entire original body was reviewed;
  the residual is the same permutation of the origin/copy block with
  identical frame homes (no changed push depth in that block). Naming
  pointers to the global `v[2]`, the cursor, or the later by-value square
  argument immediately before the x statement is inert. Naming the nested
  `GetObjCost` result is inert too. All four relocated function byte streams
  equal baseline; 82 exact neighbours and clean `/W3` preserved.

Reproduce these ten probes with the existing Python:
`scratchpad/scope-h/draw_remove/variants.py`; the helper imports the existing
matching tools without changing them. Source snapshots, side-by-side original
listings, compiler output, whole-file audits and `results.json` are alongside
it. These are local experiment artifacts, not files to stage. The concise
negative findings are also recorded above the two WIP markers.

No new callees, globals, extern-type divergences or gameplay changes were
introduced by these probes. The original `mode` asymmetry and use of the
right footprint edge for the removal cursor's y offset remain reproduced.

### Corner closure and other placement findings

- **Corner:** expand only case 0's first `LF_MAKE_SUB(3, 0)` call and write
  `sub->def` immediately after kind/dir, before `sub->run`. The original
  stores def at `0x0040dc94` before reading run at `0x0040dc97`. This fixes
  all ten mismatches (formerly first index 38) and restores the missing byte
  through the EDX global load. It preserves the original read/store order,
  even if the parent and subpiece alias. The other nineteen constructors and
  shared macro are untouched. An early volatile read exposed the order, but
  removing volatile stayed exact; plain C is sufficient. Expansion alone and
  named pointer controls remain at ten. This corrects the old claim that all
  out-of-macro first-block orders necessarily break tail merging.
- **Tunnel:** nine new targeted probes; pointer and y-sum naming is inert,
  a named footprint pointer gives 13 mismatches, and free byte reads or a
  temporary footprint aggregate give 178–188. The 5/5/5 strict/register-blind/
  offset-blind baseline and frame homes are unchanged. No body edit retained.
- **Drop:** six probes; named pointers are inert, free byte reads give 64,
  a volatile class read 49, and the derived y-step in the for increment 84.
  No body edit retained. Its full compiled body is 112i/345B, versus the
  original 111i/341B: audit's 111i/344B window excludes the final ret. The
  full-body strict distance is 45 (59.8%); the table above reports audit's 44.

The placement group ran 24 candidates including controls. Exact-corner
original/compiled listings, controls, audits and detailed mechanics are in
`scratchpad/scope-h/placement/report.md` and `metrics.jsonl`.

### Entrance and animation findings

- **Activate:** four named pointers (ride-id field, tile bytes, next-link
  slot, current rider) all emit the baseline bytes. The qx/tx coalescing
  remains; no previously recorded volatile shim was adopted.
- **Add / DrawBoats:** re-read the complete original bodies without repeating
  the recorded scalar-escape or sum-order sweeps. No new reconstruction
  error found. Add's executable body is 250i/803B plus six NOPs sampled by
  audit; DrawBoats still groups its x sum into a lea where the original uses
  three adds, while the y chain already matches.
- **AnimApplyPart:** five x87 probes preserve the known conversion order.
  A distinct volatile input copy advances the input load, but its subtraction
  still follows the Y conversions: it fails to hold the remapped result at
  the required depth. Inline/six-input variants reduce strict mismatches to
  155/179 while worsening structural alignment and frame layout, so neither
  is retained. Compound-assignment chains are byte-identical. The baseline
  executable body is 321i/1142B plus ten NOPs sampled by audit.

Entrance/animation measurements, nine candidates, frame analysis and
original/compiled listings: `scratchpad/scope-h/animation/report.md` and
`bounded-results.json`. All four executable bodies remain unchanged.

### Jungle findings

- **RiverTile / RiverAnim:** grouping coordinate pairs in one Pos is
  byte-identical. The tile's free y read worsens its count to 89; named map
  pointer plus separate animation partial sums worsens its count to 308.
- **Tick:** naming the rider array-element pointer is byte-identical;
  grouping the seat/index pair gives 260 mismatches. Raw COFF confirms that
  its executable body is 353i/1108B. The following six relocated dwords are
  switch-label addresses; audit samples their first sentinel byte as `cdq`,
  producing the apparent 354i/1109B count.
- **TraceRoute:** removing both existing volatile macros, then appending
  `x=x` after the west arm, changes allocation and creates a real fourth
  recursive call, escaping the original extent. This confirms a regime
  change without proving the complete original allocation unreachable.
  Raw COFF also shows a 129i/338B body followed by NOP padding; audit samples
  one NOP. Its old equal-size argument is therefore insufficient.

The jungle group tested eight candidates. See
`scratchpad/scope-h/jungle/report.md`, `triage.json`, and `extent-evidence.txt`
for metrics with frame homes resolved by control-flow/push depth and the raw
COFF evidence. No executable body was changed. These probes support deferring
specific residuals, not a universal proof that no C spelling can match them.

## Verification and continuation

- `scratchpad/scope-h/verify_checkpoint.py` is the runnable checkpoint check.
  It compares source against `f22f7cc`, requires all executable text outside
  `LFCorner_Place` to be unchanged, runs the existing audit on all nine files,
  and requires 169 exact / 12 WIP with every other baseline metric unchanged.
- Final audits are in `scratchpad/scope-h/final-audit.txt`: PASS, zero rejected
  exact markers. `logflume3.c` rises from two to three exact functions; all
  other per-file counts stay at baseline.
- All nine files compile cleanly with `/W3 /O2 /Gy /Gd`; `git diff --check`
  passes. Results are in `scratchpad/scope-h/checkpoint-checks.txt`.
- The only implementation change is the first corner constructor's store
  order. Other tracked edits are owned WIP notes and this lane log. No new
  symbols or extern-type differences were introduced. Original corner quirks
  (pre-null-check parent reads and overwritten orientation) are preserved.
- Shared tools/reports and scope G were not edited. The original binary,
  toolchain links and experiments remain local inputs, excluded from staging.

The matching checks validate reconstructed machine code, not live gameplay.
This was a bounded first pass across all thirteen targets; twelve remain WIP.
For continuation, read their updated notes and the local reports before trying
new variants. The corner result is a concrete counterexample to treating a
finite store-order search as proof of a compiler floor. The next useful
hypothesis should identify a specific missing construct or scheduling regime;
repeating the eliminated pointer, sum-order or volatile sweeps adds no evidence.

## Completion run — continued from 3fdd47a

The user requested continued work through completion, rather than stopping at
another initial checkpoint. The remaining twelve WIPs are being investigated
in parallel, with existing exact bodies frozen. New scratch evidence lives in
`root2/`, `flume2/`, `jungle2/` and `animation2/` under `scratchpad/scope-h/`.

1. Re-read the checkpoint and source notes; baseline is 169 exact / 12 WIP.
2. Test new source-shape hypotheses, especially aggregate parameter identity
   and independent escaped versus arithmetic object lifetimes. Audit every
   retained source edit immediately. Do not treat a finite failed sweep as
   proof that the original's code generation cannot be reached.
3. Consolidate each verified closure, confirm the complete executable extent,
   compile at /W3, and preserve the original exact functions. Keep remaining
   limitations explicit; completion requires exact matches, not changed labels.

Verified new closures so far:

- `LFEntrance_Add`: 256i/812B exact, previously 206 audit mismatches and six
  sampled NOPs. Independent call-output Pos temporaries preserve arithmetic
  aggregates without making them escape; two p3 ordering corrections finish
  the body. Whole-file audit now seven exact functions.
- `JungleCruise_TraceRoute`: 130i/339B exact, previously 105 mismatches and one
  sampled NOP. Source and target pairs are by-value aggregates and recursive
  step coordinates use one aggregate local. The loop and original register
  allocation coexist, disproving the old phase-ordering retirement diagnosis.
  Both volatile macros are removed. Whole-file audit now two exact functions.

Work remains active. The measurements below supersede the historical first
checkpoint table above.


### Continued checkpoint: five closures, eight remaining

Whole-file gates now pass with **173 exact / 8 WIP** across the nine owned
files. All 168 original exact functions and the first checkpoint's corner
remain unchanged. This checkpoint preserves verified progress while work
continues; it is not a claim that the remaining eight bodies match.

| Address / function | Insns ours/orig | Bytes ours/orig | Mismatches | First index | Strict match | Audit / marker |
| --- | ---: | ---: | ---: | ---: | ---: | --- |
| 0x0040ca60 LFTrack_DrawAlt | 144/144 | 401/401 | 2 | 84 | 98.6% | WIP |
| 0x0040f050 LFTunnel_Place | 217/217 | 680/680 | 0 | — | 100.0% | [OK] / FUNCTION |
| 0x0040dc00 LFCorner_Place | 574/574 | 1825/1825 | 0 | — | 100.0% | [OK] / FUNCTION |
| 0x0040bf70 LFEntrance_Activate | 222/222 | 672/673 | 10 | 14 | 95.5% | WIP |
| 0x0040abf0 LFEntrance_Remove | 102/102 | 343/343 | 11 | 39 | 89.2% | WIP |
| 0x00410180 LFDrop_Place | 111/111 | 341/341 | 0 | — | 100.0% | [OK] / FUNCTION |
| 0x0040a600 LFEntrance_Add | 256/256 | 812/812 | 0 | — | 100.0% | [OK] / FUNCTION |
| 0x00436dc0 JungleCruise_UpdateRiverTile | 116/116 | 367/367 | 27 | 10 | 76.7% | WIP |
| 0x00432d00 JungleCruise_UpdateRiverAnim | 422/422 | 1457/1466 | 88 | 106 | 79.1% | WIP |
| 0x00435750 JungleCruise_Tick | 354/354 | 1109/1114 | 208 | 110 | 41.2% | WIP |
| 0x00437260 JungleCruise_TraceRoute | 130/130 | 339/339 | 0 | — | 100.0% | [OK] / FUNCTION |
| 0x00418fe0 BoatingSchool_DrawBoats | 212/212 | 744/744 | 11 | 83 | 94.8% | WIP |
| 0x00442040 AnimApplyPart | 331/331 | 1174/1174 | 17 | 284 | 94.9% | WIP |

Tick still has the sampled jump-table caveat documented above. All five newly
exact bodies reproduce their complete executable extents without padding.

Additional mechanics recovered:

- **A two-field aggregate needs an initialization phase before adjustment.**
  For Tunnel, assign `c.x = footprint.x + px`, then
  `c.y = footprint.y + py`, then `c.x += 6`. The old folded `+6` kept the
  head's wrong five-position load schedule. Moving just the adjustment
  before the Y assignment restores the old five mismatches. The final full
  body is 217i/680B exact. Evidence: `animation3/report.md` and its control.
- **The same phase closes Drop's register choice and final update.** Use one
  nonescaping BPos, assign both footprint/input sums, then `c.x += 2`.
  Expand its three constructors locally with the original field-store order;
  the shared macro is untouched. Remove the old volatile X shim and the
  temporary Y pointer. Full 111i/341B exact. Moving only `c.x += 2` before
  the Y definition gives 80 mismatches; a scalar delayed adjustment gives
  50. Both aggregate identity and phase matter (`animation4/report.md`).
- **Taking a scalar's address can matter without adding memory operations.**
  ApplyPart's six scalar UV values retain their existing frame homes when
  their final copybacks read through named scalar pointers. This restores
  the original full x87 stack, u0 remap timing and 331i/1174B extent.
  Texture assignment immediately before final copyback reduces 60 to 17;
  indices 0–283 and 301–330 are exact. The residual is only the texture
  load/store and first three UV copies scheduled one remap later. No new
  volatile qualifier or external declaration is used (`animation2/report.md`).
- **Named Y views recover the draw routines' read/order regime.** Boats
  needs only `screeny = &scr.y` and an early `offsety = &ofs.y`, plus flat
  screen sums. Every Y instruction and the hull X reload now match;
  30 → 14 → 11 mismatches. The remaining two X sums put rocking X last.
  RiverAnim similarly needs screen-X, screen-Y and image-Y views declared
  before projection work: 112 → 91 → 88. Seven head positions and the old
  81-position overlay layout remain (`root2/`, `jungle2/report.md`).

Independent Draw/Remove reviews found new transitions, without a retained
body improvement. A shared forward-X local makes Draw's mode register ECX
but changes the allocation, frame and true extent (154i/435B or 162i/469B);
branch-local copies fold back to baseline. Remove's alternate coordinate
views, call argument identity and actual-access volatile controls do not
recover its origin/copy schedule (`flume3/report.md`). These finite controls
are recorded as limits of measured constructions, not universal proofs.

Verification for this checkpoint: `scratchpad/scope-h/verify_continuation.py`
checks executable-token confinement to the thirteen original WIP bodies,
freezes the closed corner, checks every original exact audit row, validates
all five closures, compiles all nine files at /W3, and runs `git diff --check`.
Outputs: `continuation-checks.txt`, `continuation-audit.txt` and per-file W3
logs. The only removed non-body definitions are W_MEM and OWNER_MEM, which
served the old WIP TraceRoute alone. No new external symbols, original bugs
changed, shared type edits or shared report/tool edits. Scope G remains in
its separate checkout.


### Further active work after 61065a1

The user explicitly requested **100%**, meaning all thirteen target functions
must become exact. Five are closed; the other eight remain active WIPs. No
finite negative search is being relabelled as full completion.

- `BoatingSchool_DrawBoats`: **11 → 5**, full 212i/744B, first index 88,
  97.6% strict. Add a mutable `screenx = &scr.x` view at function entry and
  use it for the initial projection write. The same view in the inner draw
  block gives 12 (it improves the late block but moves seven prefix slots).
  The retained placement preserves the complete prefix and fixes the first
  X-sum loads; only two register-add swaps and three rider-sum slots remain.
  An apparent 25-mismatch candidate has all indices 67–211 exact, but swaps
  the physical identities of rocking-X and screen-X in the prefix, so it is
  not a latent exact result. Evidence: `flume4/report.md` and full extents.
- `JungleCruise_UpdateRiverAnim`: **88 → 85**, full 422i/1457B, first111,
  79.9% strict. Replace scalar sx with scr.x and write its initial value
  through the existing named screen-X pointer. Direct Pos writes give91;
  scalar writes stay88. The original pointer can stay in draw scope here.
  Four head add-order positions (111/115/139/140) and 81 overlay-layout
  positions remain. OX/OY pointer writes are inert85. The original's three
  post-ret instructions are reachable cold code; the retained body ends at
  its own ret with no sampled NOPs. Evidence: `flume5/report.md`.

Fresh investigations with no retained executable change are preserved in
`animation5/` (RiverTile parameter/row lifetime), `animation6/` and `root3/`
(Activate source identities), `jungle3/` and `flume6/` (ApplyPart final
schedule), `jungle4/` (Tick target phases and station-count/queue grouping),
and `jungle5/` (Remove call-square snapshot/escaped-copy lifetimes). These
reports identify the precise construction tried, its whole-file gate and
any misleading sampled extent. For example, reassigning ApplyPart's idB
also changes the cached target index for later iterations; preserving that
initial index removes the apparent gain. No such semantic change was kept.

The continued source check now freezes all five exact closures against
61065a1, in addition to preserving all168 baseline exact functions. Tests
remain function-level compiler matching plus /W3, not live gameplay. The
next overlay investigation uses matched corpus witnesses to seek a
constructive branch-layout pattern instead of repeating old goto sweeps.


### Sixth closure and renewed tick progress

`BoatingSchool_DrawBoats` is now exact: **212 instructions / 744 bytes**,
including the final RET. This brings the owned files to **174 exact / 7 WIP**.
All six closures preserve their complete original executable extents.

The successful source groups values by axis, rather than by coordinate
vector: one local `Axis { int rock; int screen; }` for horizontal values and
another for vertical values. Keep the existing screen-Y read and screen-X
write views, then initialize rocking X through its own named pointer.
Both records and field order matter: direct rocking-X initialization gives7,
only the horizontal record gives30, reversed fields give11, while the
retained combination gives0. No runtime instruction or memory access is
added. The experiment and full-body checks are in `root5/report.md`.

This transfers to `JungleCruise_UpdateRiverAnim`: **85 → 81**, full
422i/1457B versus original422i/1466B. All first247 instructions now match;
only the overlay branch layout remains. The corresponding direct-write,
uncoupled-Y and reversed-field controls give88/100/95. Its existing
screen/rock views can remain in draw scope (`flume9/report.md`). Earlier
new predicate/ternary/call-argument/aggregate-phase controls did not fix the
cold overlay blocks (`root4/`, `flume7/`, `flume8/`).

`JungleCruise_Tick` improves **208 → 104 → 62**. A body-local, ABI-compatible
view passes the two coordinate pairs to CalcMoveLine by value, preserving
the original five stack words and direct call. This recovers the genuine
missing case-0 instruction. Grouping case0's X key coordinate and class
origin in one nonescaping two-field record restores the entire first-loop
prefix. The final body has354 real instructions /1111B (original354/1114B),
first difference110, and346/354 register+offset-blind aligned agreement.
It ends in RET; no jump-table sentinel is sampled. This supersedes the old
353-instruction extent caveat and all source-unreachability claims.
Evidence and semantic ABI checks: `animation7/report.md`.

The remaining seven WIPs are DrawAlt2, Activate10, Remove11, RiverTile27,
RiverAnim81, Tick62 and ApplyPart17. The numbers are strict instruction
position differences, not completion percentages. Work continues toward
all thirteen exact; these verified gains are preserved as a checkpoint.


### Seventh closure: river-tile source reconstruction

`JungleCruise_UpdateRiverTile` is now exact: **116 instructions /367 bytes**,
including RET. Use `g_jc_river_tiles[mask][i * 5 + j]` for both byte reads,
remove the explicit cursor and its increment, and write key Y before
FindAt. Direct nested indexing alone gives7; the key-store order closes
those7. Existing i initialization and global-update order can stay unchanged.
The compiler derives the original cursor itself, preserving incoming Y
through the first row and coalescing it into EBX. The newer
`BsWater_SetTile` source supplied this construction; old notes claiming it
was undecompiled were stale. Evidence: `flume12/report.md` and full COFF
checks. The previous six closures remain frozen.

Further verified improvements since a9e38fe:

- Remove **11 → 8 → 3**, complete102i/343B. A Y-only coord/origin record
  initialized through one named field pointer restores the original sum
  destination and full prefix through50. Only the packed square's word load
  remains after the footprint copy instead of before it. All82 exact
  neighbours pass (`root6/report.md`).
- Tick **62 → 59**, complete354i/1111B. Scope the escaped world output Pos
  to a braced case3. That restores the paired loads before the flag update
  and position stores, making all227..353 instructions exact. Scope of
  screen alone and declaration order are inert. All four exact neighbours
  pass (`animation7/report.md`).

This leaves six active WIPs: DrawAlt2, Activate10, Remove3, RiverAnim81,
Tick59 and ApplyPart17. Seven targets are exact; the task continues until
all thirteen match. The next gate requires175 exact /6 WIP, preserves all168
baseline exact functions and all six earlier closures, verifies the new
Tile extent, and compiles all nine files cleanly at /W3.


### Eighth closure: final texture-remap scheduling

`AnimApplyPart` is now exact: **331 instructions /1174 bytes**, including
RET. Explicit float conversions around the relative-coordinate division
and destination-scale multiplication restore the original texture reload,
texture store and UV-copy schedule. All six coordinates use the same formula.
The compiler emits no extra FP rounding operation, storage or instruction.
The six named scalar UV addresses from the prior reconstruction remain needed.

The conversion mechanism was recorded in DECOMP: internal float conversions
can survive as no-code scheduling tuples. Assignment-result casts were inert;
internal casts reduced17 to14, then2, then0. Twelve uniformly placed casts are
retained. A deletion control needs eight on the first four coordinates;
deleting any one of those eight leaves2. This is a measured deletion minimum,
not a claim about every possible source. Evidence: `animation8/report.md`,
`full-check.json` and complete raw COFF disassembly through RET.

The scope gate now requires **176 exact /5 WIP**, including all168 baseline
exact functions and all eight new closures. All nine owned files compile
cleanly at /W3. The five remaining targets are DrawAlt2, Activate10, Remove3,
RiverAnim81 and Tick59. These are strict instruction differences. Work
continues until all thirteen scope targets are exact. No gameplay test has
been performed; the validation is exact compiler output and source isolation.


### Ninth closure: edit-cursor lifetime

`LFEntrance_Remove` is now exact: **102 instructions /343 bytes**, including
RET, with the same0x1838 frame. Move the plain EditCursorRec declaration
into a block containing only the origin calculations, footprint copy and
StandardRemoveObject call. The block ends before cost handling and
LFRun_RemovePiece. The prior Y-axis coord/origin record stays unchanged.

A loop-entry cursor declaration remains3; adding calculation/call braces
while leaving the cursor at function entry also remains3. The declaration's
narrow lifetime is necessary. It permits the packed square word load at51
before the footprint LEA and REP MOVSD, recovering the last three positions
without extra operations or altered ABI. Named footprint views, a same-layout
cursor type, intrinsic20B memcpy and copy-in-argument forms were inert on3.
Evidence: `jungle18/`, `jungle19/` and `jungle20/report.md`. Full COFF ends
in RET with only9 literal NOPs afterward; all82 prior exact neighbors pass.

Nine of the thirteen targets are exact. The gate requires177 exact across
the nine files, including all168 baseline matches and nine scope closures,
with /W3 clean. Four targets remain: DrawAlt, Activate, RiverAnim and Tick.
RiverAnim has a scratch branch-layout improvement under separate verification;
the retained source metric is updated only after that verification completes.


### Requested push checkpoint

`JungleCruise_UpdateRiverAnim` improves **81 →24**, complete422i/1464B
versus422i/1466B. Within each selected-seat guard, a zero-first conditional
chooses the entire frame-plus-offset expression. Both cold arms now occupy
the original positions:267..269 between loops and419..421 after RET418.
A nonzero-first conditional restores81. Every reachable instruction is
covered by the complete CFG check; the post-RET branch is not padding.
The remaining24 differences are codeEAX versus originalECX and the two
overlay call schedules. Register-blind differences improve76→16 and aligned
register/offset-blind LCS404→410 of422. All14 exact neighbors and /W3 pass.
Evidence: `animation9/report.md`, `full-extents.json`, `retained-check.json`.

Current scope H status, before the requested branch push:

| Address | Function | Instructions /bytes | Strict differences | Status |
| --- | --- | --- | ---: | --- |
| 0x40ca60 | LFTrack_DrawAlt |144/401 |2 |WIP |
| 0x40f050 | LFTunnel_Place |217/680 |0 |exact |
| 0x40dc00 | LFCorner_Place |574/1825 |0 |exact |
| 0x40bf70 | LFEntrance_Activate |222/672 |10 |WIP |
| 0x40abf0 | LFEntrance_Remove |102/343 |0 |exact |
| 0x410180 | LFDrop_Place |111/341 |0 |exact |
| 0x40a600 | LFEntrance_Add |256/812 |0 |exact |
| 0x436dc0 | JungleCruise_UpdateRiverTile |116/367 |0 |exact |
| 0x432d00 | JungleCruise_UpdateRiverAnim |422/1464 |24 |WIP |
| 0x435750 | JungleCruise_Tick |354/1111 |59 |WIP |
| 0x437260 | JungleCruise_TraceRoute |130/339 |0 |exact |
| 0x418fe0 | BoatingSchool_DrawBoats |212/744 |0 |exact |
| 0x442040 | AnimApplyPart |331/1174 |0 |exact |

The checkpoint has **nine of thirteen scope targets exact**, with177 exact
functions across the nine owned files. All168 baseline matches and all nine
new closures are preserved. The scope gate compiles all nine files cleanly
at /W3 and checks source isolation. This is a checkpoint; four targets remain
active and the task continues toward all thirteen exact. Scratch candidates
and original binaries are excluded from the commit and push.


## Fable continuation from f26cf09 (2026-09-05, evening)

Continued in a separate worktree on `scope/H-fable` (branched from Codex's
checkpoint; Codex's worktree and scratch were read, never written). Method: per
remaining partial, three Opus attempts with different lenses (witness scan over
the exact corpus, fresh reconstruction, lever search) on private copies, then a
second round aimed at the single decision the first round pinned down, each
claim verified independently (target `[OK]`, every other row unchanged, diff
confined to the one body, semantics against the disassembly, `/W3`).

**Tenth closure: `JungleCruise_UpdateRiverAnim` 0x00432d00 is exact**
(422/422, 1466/1466). The overlay sprite is chosen by ONE conditional over the
WHOLE lookup, `spr = i == 0 ? ilf->sprites[(f + 0x10) & 0xff] :
ilf->sprites[(f + 0x20) & 0xff]`, not by a conditional over the frame-plus-
offset value looked up once. With the conditional over the value the lookup
belongs to the argument list and is emitted after `push 0; push 0`, so the
coordinate loads are hoisted ahead and the merged value takes EAX (`and
eax,0xff`, 5 bytes). With the conditional over the lookup VC6 sinks the arms'
common tail (the image-list load, the mask, both indexed loads) into the join
ahead of the argument setup, and the frame value keeps ECX (`and ecx,0xff`, 6
bytes) with `b->sy` reusing it once it dies: the whole two-byte difference,
twice. Zero-first polarity is still required (inverting costs 79). Both
second-round lenses found this independently. Source-level web merging (one
variable for two values) is inert: VC6 re-derives webs from dataflow and
ignores symbol identity, so a register decision is moved by changing WHERE a
computation sits relative to a join, not by naming.

**`JungleCruise_Tick` 0x00435750: 59 → 17**, first index 126 (was 110),
354/354, 1111/1114. The first-round diagnosis (37 of the 59 a pure EAX↔ECX
rename, the three missing bytes exactly that rename: a 6-byte `mov
ecx,[g_jc_stations]` vs the 5-byte A1 form and two 6-byte `and ecx,0xff`) is
flipped by RANK, not order: a named pointer view `bl = st->blokes` taken once
per station gives the station web the references that put it in ECX. The
remaining 17 sit in case 1; the second-round order lens measured 108 further
head spellings and localised the residual there. Retained as WIP with the
improvement.

**`LFTrack_DrawAlt` 0x0040ca60 stays at 2**, but the decision is now a stated
rule, measured three ways in this one body: the reload of a stack-homed
argument takes the register of the MOST RECENTLY FREED TEMPORARY, i.e. the
right operand of the last `cmp`, provided that operand is an anonymous
temporary and not an enregistered variable. `cmp al,cl` with `cl` anonymous
gives the original's `mov ecx,[esp+1Ch]`; with `cl` the named `px` it gives
`mov eax`. Naming the forward byte instead flips the carrier to ECX but costs
`fwd` its home-slot spill and the byte lands in BL (154i/435B); leaving both
bytes anonymous drops the hoisted three-load block and the second `test`
(146i/409B). Two reachable bodies, neither the original: a floor of this frame
until a construct keeps `px` enregistered yet anonymous at the compare. Corpus
census (78 `mov reg,[esp+N] / push imm / push reg` sites): a preceding call
does not bias the carrier, nor does parameter-home vs spill.

**`LFEntrance_Activate` 0x0040bf70 stays at 10.** Whole-function census: only
indices 14–24 differ; the two preambles are the same nine values in the same
five load slots and seven arithmetic slots, only the value-to-slot assignment
differs. The original packs the two dead temps crosswise ({next, tx} share a
web, {qx, ty} the other); ours packs {qx, tx} and {next, ty}. Symbol reuse,
declaration order, initializer forms, pointer views and the sum's operand
order are all inert (five lenses, ~150 spellings). Floor pending a construct
that changes the dataflow of `qx` relative to `next` at the loop head.

Gate after this continuation: junglecruise.c 15 exact (was 14), ridecb2.c 4
exact + Tick at 17; all other owned files untouched. Ten of thirteen scope
targets exact.

### Continuation, 2026-09-06: eleven of thirteen exact

- **LFTrack_DrawAlt 0x0040ca60 is exact** (144/144, 401/401). Eight lenses and
  roughly 250 spellings had priced its two mismatches as a register floor, all
  of them working at the reload site. The error was ONE STATEMENT ABOVE it.
  The sprite select was written as a default plus a conditional override
  (`spr = fc1; if (p->dir) spr = fc3;`), which leaves the direction split's
  join block empty; VC6 folds that block into the sprite test and each arm
  then loads its own three operands. Written as a two-way choice
  (`if (p->dir == 0) spr = fc1; else spr = fc3;`, or the `?:` with the same
  polarity) the select if-converts to the identical instructions but the join
  survives, the loads that head both arms are hoisted into it as ANONYMOUS
  per-arm temporaries, `cmp al,cl` frees cl last, and the `mode` reload takes
  ecx as the original does.
- **JungleCruise_Tick 0x00435750 — 11** (was 208 at the checkpoint). A named
  pointer view of the station's bloke table flipped the station/key register
  decision (59 → 17); then two case-0 schedule sites closed on the fact that
  VC6's list scheduler disambiguates memory by IL BASE SYMBOL, not by address:
  a pointer view with a non-zero offset folds back into the same `[esi+...]`
  operand yet remains a distinct symbol, so `from = (Pos*)&b->x` keeps the y
  load after the ty store and `p = &b->dir8` keeps the action store after the
  direction store (17 → 11). Both views must die before the call.
  Re-classified: the remaining 11 are not one rank decision but TWO basins,
  each with one blocking defect, separated by one weighted reference; the
  view's only effect is that a web is live across the dispatch.
- **LFEntrance_Activate 0x0040bf70 — 10**, confined to indices 14–24. The
  preambles hold the same nine values in the same five load slots and seven
  arithmetic slots; only the value-to-slot assignment differs, and the
  original packs the two dead temps crosswise. Unlike the four closed cases,
  the re-derivation confirmed the earlier lenses' model rather than falsifying
  it: the frame is two homes and both are reproduced. Roughly 150 spellings
  are inert.

## Codex continuation in a new worktree (2026-09-06)

Started from the fetched `origin/scope/H-fable` at `e1d7dc35`, on
`codex/scope-h-fable` in `.worktrees/scope-h-fable`. The original executable
has the expected SHA-256
`c50865b60bfcb26c0a7329a75fb772b10ae234af324f669901906e5abb0e2bd9`.
Used the existing Documents-checkout VC6 wrapper and
`/Users/systemadmin/.venvs/legoland/bin/python`; the worktree's local
`original` and `toolchain` links are not committed.

**Result: still eleven of thirteen targets exact.** No implementation or
marker was changed. The nine owned files pass the audit with **179 exact /
2 WIP**, and all nine compile cleanly at `/W3 /O2 /Gy /Gd`. Their source
bytes are unchanged from `e1d7dc35`, so all eleven prior closures and the
168 original exact functions are preserved.

| Address / function | Instructions ours/original | Bytes ours/original | Strict differences | First index | Strict match | Audit / marker |
| --- | ---: | ---: | ---: | ---: | ---: | --- |
| `0x0040bf70` `LFEntrance_Activate` | 222/222 | 672/673 | 10 | 14 | 95.5% | WIP / WIP-FUNCTION |
| `0x00435750` `JungleCruise_Tick` | 354/354 | 1111/1114 | 11 | 126 | 96.9% | WIP / WIP-FUNCTION |

The complete original instructions and compiled bodies were inspected again
before the probes. Neither target escapes its original extent. The two
remaining differences are not new gameplay functionality to implement: they
are still source-shape/code-generation gaps. The measurements below bound
the tested forms; they do not prove every equivalent C spelling impossible.

- **A by-value coordinate call view does not separate the entrance's x sum
  from its offset.** `LFPath_StepBloke` called through a local function
  pointer with `(void*, Pos, Bloke*)` has the same four-word ABI as its
  existing declaration. Function-scope `Pos`, case-local `Pos`, and one
  `Pos` replacing both coordinate locals all leave 10 differences / 672B.
  No extern declaration was edited and no call-view change was retained.
- **Pairing an entrance coordinate with its origin is inert here.** The
  two-field x record, both initializer orders, scalar field assignments and
  a named pointer to its origin field all leave 10. The y record is 10 in
  the existing operand order and 11 reversed. This bounds transfer of the
  axis-record lever that helped other scope H bodies.
- **Pairing the dead x offset with `next` changes the frame, not the desired
  packing.** An int/pointer struct, both at function scope and inside the
  loop, gives 18 differences / 676B. An overlapping union leaves the
  baseline 10 / 672B. A copied rider record adds loads and escapes; copying
  the two signed offset bytes adds instructions. None is retained.
- **The entrance's loop and widening alternatives do not close the head.**
  Guarded do/while, an explicit early return before do/while, and an early
  busy-rider continue reproduce the baseline. An explicit latch goto is
  worse (145). Widening through short, long or unsigned int, subtracting a
  negated offset, a signed-byte union, and a 32-bit signed bit-field all
  leave 10. A volatile store through the existing `next` home reproduces
  the known 9-difference / 672B control, not a new closure, and is rejected.
- **Splitting the two station traversals does not change Tick's allocation.**
  A distinct station local and enclosing block for the first traversal
  leave the committed queue-view body at 11 and the prior no-view control
  at 51. Wrapping `seat`, `i`, or `st` alone in a one-field record is also
  inert. Putting both scan indices in one record instead changes the
  saved-register assignment and gives 263 differences.
- **A view spanning the station count, queue and rider slots does not solve
  the no-view case.** Correctly initialized nonzero-offset views confined
  to case 1 give 51 differences, with two additional schedule differences
  for some subsets of fields. A riders-only view is worse (186). A named
  current/previous queue slot in the shuffle arm gives 83/217. None improves
  the committed 11-difference body.
- **Queue-shift definitions and scan shape remain bounded negatives.**
  Moving the decrement before the stores while adjusting their indices
  leaves 51 in the closest no-view form; moving it before the loads gives
  217. A separate previous-slot index is worse. An explicit byte
  displacement for the seat-offset table is inert (51); negating the index
  as a separate assignment adds an instruction (211). A while-form scan
  or success-continue spelling is inert (51), a pointer scan is 52, and a
  post-scan conditional default, do-loop scan, or reusing `seat` for the
  movement direction worsens the full body.

Evidence stays local in `scratchpad/scope-h-codex/`: `probe.py`,
`round1.py` through `round6.py`, candidate C files and full side-by-side
listings, `baseline-audit.txt`, and `final-checks.json`. Compilation failures
and accidentally self-referential field-view probes are discarded; only the
corrected `tick_fix_group_*` probes support the field-view finding above.
No new symbols, callees, global addresses, gameplay changes, or extern-type
divergences are committed. Existing missing-station and save-pointer quirks
are preserved. Validation is against the compiler output, not live gameplay.
