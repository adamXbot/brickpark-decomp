# Scope G — coaster, school car and joust partials

Started 2026-09-05 from `origin/main` at `f22f7cc` on branch `scope/G`,
in the isolated `legoland-scope-g` worktree. Ownership and verification follow
`docs/SCOPE_G_partials_coaster.md` and `docs/PARALLEL_CONTRACT.md`.

## Stage plan and checks

Continuation requested 2026-09-05 after checkpoint `5b61ff4`: keep working
until complete. Completion now requires all nineteen assigned functions to
pass the full-body exact gate; another bounded negative pass is not completion.
The existing evidence remains the experiment ledger. Next stage: resolve the
earliest structural mismatches in topology and rasterization, then transfer
any new liveness, frame or scheduling mechanisms to the remaining families.
No source-only or percentage-only result will be labeled exact.

1. Establish the baseline for all twelve owned files. Save complete audit
   output under `scratchpad/scope-g/`; require PASS and record the exact count.
2. Read each original body and its existing residual note, classify mismatches,
   and try only evidence-directed changes not already ruled out. Work on disjoint
   files in parallel. After every source change audit the entire file; preserve
   all existing exact bodies and their count. Keep unsuccessful experiments in
   scratch only and retain an improvement only if semantics still agree.
3. Audit all owned files, compile changed files at `/W3`, check the diff against
   the allowed bodies, and publish verified results or bounded negative findings
   here. Commit/push only `scope/G`; integration owns global verification/reports.

## Environment

The contract's Documents checkout and `~/.venvs/legoland/bin/python` do not
exist on this host. Existing equivalents were found and tested; no toolchain or
environment was installed:

- Python: `/opt/homebrew/opt/python@3.14/bin/python3.14` (capstone 5.0.7 and
  pefile already installed).
- `LEGOLAND_CL=/Users/systemadmin/Downloads/alpha team/alphateam/tools/wibo-msvc/cl`.
- `toolchain` links to the existing Alpha Team VC6 SP3 installation;
  `original/legoland.exe` links to the existing binary in `../legoland/original`.
  Both inputs are covered by the existing ignore rules.
- First gate: `schoolcar2.c` PASS, two exact functions preserved; manoeuvre C
  has 3 strict mismatches and A has 10, each 207 instructions / 669 bytes.

## Initial-pass result

Completed the reconstruction review and bounded hypothesis pass across all nineteen
assigned WIPs. **One source improvement, no new exact functions:** reversing the
high clip-nibble condition in `Raster_SubmitPoly` puts the mode=0/decrement path
on fall-through as in the original and reduces strict mismatches **231 → 228**.
All nineteen remain `// WIP-FUNCTION:`. Several historical size, first-divergence
and floor claims were corrected; structural cases remain open.

Final whole-file audit: **PASS, all 88 pre-existing exact functions preserved**
across the twelve files. All twelve compile clean at `/W3 /O2 /Gy /Gd`. A
comment-stripped source comparison against the starting commit confirms that the
single Raster condition/arm reversal is the only executable-source change;
all other edits are assigned WIP notes/markers. No types, externs, exact bodies,
unassigned WIPs or shared tools were changed.

## Continuation checkpoint

**Eight of nineteen assigned functions are now exact.** `Coaster3D_DrawMesh`
passes the full-body `[OK]` gate at 173 instructions / 514 bytes. Its source
cursor now addresses the destination through one invariant unsigned integer
byte displacement, explicitly reproducing `sub esi,ecx` and `[esi+ecx]`.
Advancing the source after the shared store closes all eight mismatches.
Unsigned integer addresses preserve the target's 32-bit arithmetic without
subtracting pointers to separate arrays. No inline assembly was added.

`InitTrackDrawModes` also passes `[OK]` at 60 instructions / 228 bytes.
Copying its adjacent in/out fields as an eight-byte local record, then
extracting the two values, fixes the original cursor bias and load order
together. Copying the out/mode pair gives the same exact result. The whole
twelve-byte record and three independent scalar loads had both failed;
the size of the copied subrecord is the decisive source distinction.

Both `SchoolCarManoeuvreC` and `SchoolCarManoeuvreA` now pass `[OK]`, each
at 207 instructions / 669 bytes. Duplicating only their final `c->start = p`
copy into the two source arms closes the earlier second-arm scratch-register
phase. VC6 still merges the emitted tail, so this adds no machine code.
Duplicating only the count store fails; duplicating both final stores also
works. This disproves the earlier floor inference from local temporary and
volatile probes: source ownership of a common tail affects upstream allocation.

`Shade_BuildRamp` passes `[OK]` at 129 instructions / 401 bytes. Double
accumulators and steps preserve the distinct x87 values without volatile
copies and restore the integer setup's schedule. The saved components remain
float; static float zero/maximum constants and casts on the second-run
subtractions retain the original dword constant operands. All eleven floating
constant references were resolved through COFF relocations and checked against
the original bytes, in addition to the full-body instruction audit.

`Coaster3D_SetupView` passes `[OK]` at 129 instructions / 466 bytes.
Qualifying the aggregate copy's source as volatile preserves the copy barrier
while permitting the original m[1]/m[0] load order. An eight-byte bounds
right/bottom copy into `Pos` fixes the half-width sum's register. Explicit
float casts on both products in the earlier eye-X expression close the final
config/x87 scheduling window without adding any instructions.

`TrackCurve_GatherParams` passes `[OK]` at 70 instructions / 211 bytes after
correcting its return type from void to int and returning `g_tc_n`. The
previously unexplained EAX reload is the live return value, not dead code.
The zero-count and one-count paths retain the earlier count load; positive
inner passes reload after the x87 status/swap operations clobber EAX.
All branch targets and instruction widths match. The foreign function-pointer
extern remains untouched; only this assigned function's signature changed.

`TrackCurve_Refine` now passes `[OK]` at 110 instructions / 356 bytes.
An initialized local iterator `{ float* r; int i; } = {root, 2}` puts the
counter at its original early position; reversing those fields or replacing
the initializer with scalar assignments does not. The existing embedded
slope assignment and separate `c1` read preserve both x87 values. A free
volatile `c3` read during scoring closes the final operand-order pair.
Independent checks confirm every instruction width, all six branch targets,
and all four floating-point constant bytes. Only this assigned body changed,
and the full-file gate rises from three to four exact functions.

Three additional improvements are retained as WIPs:

- Topology: 53 → 3 mismatches, now exactly 174i/582B. An explicit second-field
  seam cursor, consumed pair-copy trip count, separate live six/eighteen
  multiples and one volatile read at the outer counter latch recover the
  original first 120 instructions. Reading the output pointer into its own
  variable before advancing it preserves the original copy/add; loading the
  source with `k++` fixes the inner schedule. Only the template-address load
  at indices 121–123 is still scheduled differently.
- Kneeling: 37 → 4, now exactly 58i/177B. The running Y coordinate survives
  across `__ftol`, including its original intermediate target store. A read
  of the 512.0f constant through volatile places the multiplication earlier.
  Only two adjacent scheduling swaps remain, at 15/16 and 20/21.

- Route bisection: 78 → 32, now the actual 84i/250B body. Copying the
  twelve-byte snapshot as one record preserves the missing shared source
  address. Two free reads of `hi` select the original midpoint operand
  order. Assigning the final midpoint back into `lo` also restores its original
  dead-argument home. Initial dt scheduling and scratch-register allocation
  still differ.

The full-file gates now report **96 exact functions**, preserving all 88
previous exact functions. All twelve owned files compile clean at `/W3`.
A comparison against `5b61ff4` confirms that executable changes are limited
to these fourteen assigned functions; no exact neighbor, shared declaration or
unassigned body changed. Eleven assigned functions remain open. All eight
new exact bodies also pass an independent comparison of every instruction
width and branch target index, beyond the normalized instruction gate.

## Continuation evidence through abb63b7

Source checkpoint `6385a29` is pushed to `origin/scope/G`. All seven new
exact bodies additionally pass per-instruction width and direct-branch-target
checks, recorded in `scratchpad/scope-g/exact-control-flow.txt`. The goal is
**not complete**: twelve assigned functions remain WIP. The desktop goal then
reported `usageLimited`; the three parallel workers stopped with account usage
limit errors. Local work continued after those errors, with these results:

- **Mesh building:** four-byte scalar/record `memcpy`, returned-copy and
  inline output-copy forms remove the required element home. `memmove`
  introduces a call. An element/position aggregate preserves the home and
  original byte count but changes the callback and floating-point schedule.
  Neither replaces the retained eight-mismatch body.
- **Topology:** volatile template/target reads or stores change the inner
  schedule and shorten the body. They do not fix the three remaining
  template-address scheduling differences. Earlier pointer, index and
  paired-record experiments remain negative evidence for those forms only.
- **Routes:** signed/unsigned 64-bit snapshot pairs and packed twelve-byte
  mixed-width copies either fold the shared address or introduce the extra
  source-register copy. Refine's double `best` with a float constant is
  instruction-identical to the retained sixteen-mismatch body; widening the
  evaluated curve/deviation changes the frame and arithmetic. None retained.
- **Kneeling:** twelve float/double intermediate and cast combinations are
  identical to the retained four-mismatch body. The two small scheduling
  windows remain open.
- **School-car driver:** explicit height/flag unions, including nested
  fields and the recovered fourteen-word aggregate layout, do not separate
  the flag's allocation from its address-taken home. Widening projection
  expressions to 64 bits and narrowing the result reproduces the same two
  known allocation/schedule regimes. No source improvement.
- **Joust:** expanding only case 0x18's walk, copying its world/target values,
  or naming the byte result leaves sixteen mismatches. Narrow phase-two gate
  types introduce conversions or reorder code; `unsigned int`/enum forms
  are unchanged. Repeating gate initializers inside the cycle/horse branches
  changes allocation without reproducing the original. No source improvement.

All experiments ran on full translation units with the exact-neighbor audit.
Rejected variants remain local in scratch; no helper, type or assembly change
from them is retained. Drivers and results are named `buildmesh-root-copy`,
`topology-root-finalwindow`, `route-root-widecopy`, `refine-root-best`,
`kneel-root-precision`, `car-root-sharedslot`, `car-root-wideproduct`, and
`joust-root-{tail,gatetype,gateheads}` under `scratchpad/scope-g/`. The root
and three lane continuation ledgers contain earlier discriminating evidence.
These bounded negative results do not establish that the residuals are
unreachable; further work must start from the saved evidence and the current
seven-exact baseline. The final repeat gate confirms 95 exact functions
across all twelve files and clean `/W3` compiles for all twelve; evidence is
`scratchpad/scope-g/checkpoint-final{,-audit}.txt`. Only this report changed
after the source checkpoint.

## Resumed continuation

The subsequent goal continuation is active and all three workers resumed
successfully. `Route_StepToPieceEnd` is retained at 32 mismatches. Independent
root verification confirms its actual last instruction is the 84th instruction
at byte 250, the full-file audit preserves three exact neighbors, `/W3` is
clean, and a comment-stripped source comparison shows only the assigned WIP
body changed. After the 34-mismatch snapshot checkpoint, assigning only the
final midpoint into `lo` reduces the residual to 32 and restores the final
original stack home. Evidence: `scratchpad/scope-g/checkpoint-resume-32.txt`
and `checkpoint32-schoolcar6.audit`. No new exact marker is claimed.

`Raster_SubmitPoly` now retains separate branch-local inverse copies after
each edge's metadata. The common divide stays in x87 until the chosen arm
stores it, recovering the original two branch-specific stores without a new
integer reload. This is a structural correction: actual 251i/744B versus the
original 252i/743B, and 229 positional mismatches versus the previous 228.
It remains WIP. Independent full-file audit preserves three exact neighbors,
`/W3` is clean, and only the assigned body changed; evidence is
`checkpoint-resume-32.txt` and `checkpoint32-coaster3d.audit`.

Further root probes are negative: sharing topology's source/target variables
between its two loops leaves the three scheduling differences; a triangle
stamping helper with the output update in the caller is identical; changing
output/counter record representation adds differences. Rider value-returning
lookup helpers and based-pointer forms do not preserve the required index
address construction. A free model read before the index partly repairs its
schedule but changes other registers. Callback argument records and inline
vector addition do not repair the mesh builder's required home/register copy.
These results are in the `topology-resume-*`, `rider-resume-*` and
`buildmesh-resume-*` drivers/logs; none of their helpers or type changes is
retained. The route, raster and ride continuation ledgers record their parallel
investigations. Further topology tests confirm that explicit signed source
address bounds, paired source/destination fields, and harmless wide-integer
conversion boundaries do not change its three-operation scheduling window.
A twelve-byte source/current/next descriptor adds one register difference;
a volatile whole-descriptor copy adds spills. Whole rider-patch inline
helpers preserve the existing twelve differences. These are bounded negative
results, not proofs of a floor. Eight targets are exact and eleven remain open.

## Eighth exact function and raster frame reconstruction

The initialized iterator closes `TrackCurve_Refine` after earlier scalar
scheduling probes had stalled at sixteen differences. Authoritative audit,
`/W3`, branch/width/constant verification and the ownership mask all pass in
`scratchpad/scope-g/checkpoint-refine-exact.{txt,audit}`. The combined audit
of all twelve files reports 96 exact functions and no rejected function
(`checkpoint-eight-all.audit`). No exact neighbor was edited.

Raster's next retained step is the eight-byte constant/first-difference pair
plus a twenty-byte second-difference/four-gradient record. They restore the
original lower frame offsets: constant -0x40, differences -0x3c/-0x38,
gradients -0x34, keys -0x80, and edges -0x200. Separate free reads for the
two attribute loop bounds recover the original entry jump. Initializing the
outer counter before its guard restores that store's relative order. The
actual body now has all 252 instructions and the original mnemonic histogram,
747 bytes versus 743, with 182 strict differences. Scalar homes and register
allocation remain open; this is not an exact claim. Full-file PASS3, clean
`/W3`, the body-only mask and an independent extent/structure check pass in
`checkpoint-raster-prefix.txt` and its audit.

The four-entry gradient capacity is supported by all four detected original
direct call sites: 0x4209c9, 0x420bea, 0x420e39 and 0x423687 pass 2, 3, 4 and
3 attributes respectively. The original file has no literal function-pointer
reference to this target. Evidence: `raster-caller-count.txt` and its script.
This check includes three callers not yet represented by the named C symbol.

Root tried transferring Refine's initialized-iterator mechanism to topology
and rider lookup. Neither closes: topology source/counter pairs either add
spills or retain three differences; rider search pairs change count setup and
registers. Initialized source/destination pairs also leave topology at three.
The successful iterator is a specific source-shape result, not a universal
scheduling fix. These probes remain in scratch.

## Requested checkpoint after ad25c9f

Two more assigned bodies now retain verified structural improvements; both
remain **WIP-FUNCTION**, and completion remains eight of nineteen exact.

- **SchoolCarBlockedAhead:** 27 → 25 strict differences, with the missing
  instruction recovered: actual 63i/179B versus 63i/180B. A local `CarPos`
  ahead point and a `CarPos*` view of the other car's adjacent world coordinates
  restore the conversion/subtraction order and the Y register copy. The
  remaining callee-saved register permutation changes the prologue and makes
  the list-next load one byte shorter. Six exact neighbors remain protected.
- **StepSchoolCar:** 319 → 104 strict differences; actual 351i/1146B versus
  351i/1147B. Grouping the saved frame state and computing the projection as
  a block-local `CarPos` restores the original frame homes and projection
  registers. A whole position assignment naturally preserves the temporary
  X store, replacing the former volatile-store workaround. The extra dead
  local Y store and missing Y register copy remain matching defects. The
  temporary target fields are overwritten before the next call or movement
  use. All 27 direct calls retain their order; 29 exact neighbors are preserved.

The checkpoint gate `scratchpad/scope-g/checkpoint-car-blocked.py` and its
`.txt` report independently confirm all twelve full-file audits (96 exact),
clean `/W3` builds, the two actual instruction extents, unchanged call-target
sequences and control-transfer graphs, and changes restricted to the two
assigned bodies and notes. All five school-car switch-table targets resolve
inside the measured body and preserve their original control-block destinations.
The eight completed targets additionally pass fresh instruction-width and
branch-target checks in `exact-control-flow.txt`. No helper or shared
source declaration was added.

Further bounded probes remain scratch evidence: alternate views of the mesh
builder's element/position aggregate leave its 44-difference structural
alternative unchanged; duplicating its common count stores does not close
its retained eight differences. Topology's template-record view leaves three
scheduling differences. Kneeling's mixed integer/float record and whole-word
coordinate reads worsen the retained four. Raster can recover the original
gradient register allocation or every stack home in separate candidates, but
combining those remains unresolved; none replaces the retained 182-difference
body. These findings do not constitute completion or a proof of exhaustion.

## Per-function checkpoint

The eight zero-mismatch rows have audit `[OK]` **yes** and marker **FUNCTION**;
all other rows remain **WIP-FUNCTION**. Percent means
strict normalized positions equal within the audit window; it is not the
sequence-aligned percentage in some existing markers. A displaced instruction
can make strict percentages very low without implying missing gameplay behavior.
First divergence is a zero-based instruction index.

| Address | Function | Audit ours / original (i, B) | Mismatches | Strict % | First |
| --- | --- | --- | ---: | ---: | ---: |
| 0x00401080 | `SchoolCarManoeuvreC` | 207i/669B / 207i/669B | 0 | 100% | — |
| 0x00401320 | `SchoolCarManoeuvreA` | 207i/669B / 207i/669B | 0 | 100% | — |
| 0x00428750 | `InitTrackDrawModes` | 60i/228B / 60i/228B | 0 | 100% | — |
| 0x00422000 | `TrackCurve_GatherParams` | 70i/211B / 70i/211B | 0 | 100% | — |
| 0x00422e40 | `Shade_BuildRamp` | 129i/401B / 129i/401B | 0 | 100% | — |
| 0x004234e0 | `Coaster3D_DrawMesh` | 173i/514B / 173i/514B | 0 | 100% | — |
| 0x00428cb0 | `Coaster3D_BuildTrackMesh` | 151i/442B / 151i/441B | 8 | 94.7% | 21 |
| 0x00425e20 | `Coaster3D_SetupView` | 129i/466B / 129i/466B | 0 | 100% | — |
| 0x00421660 | `CoasterCar_BuildRider` | 191i/549B / 191i/549B | 12 | 93.7% | 103 |
| 0x00402490 | `SchoolCarBlockedAhead` | 63i/179B / 63i/180B | 25 | 60.3% | 1 |
| 0x00428f00 | `Coaster3D_InitTrackTopology` | 174i/582B / 174i/582B | 3 | 98.3% | 121 |
| 0x0041e000 | `Route_StepFree` | 76i/217B / 76i/218B | 53 | 30.3% | 23 |
| 0x0041df00 | `Route_StepToPieceEnd` | 84i/250B / 84i/250B | 32 | 61.9% | 1 |
| 0x00421e90 | `TrackCurve_Refine` | 110i/356B / 110i/356B | 0 | 100% | — |
| 0x0042a2f0 | `Raster_SubmitPoly` | 252i/747B / 252i/743B | 182 | 27.8% | 4 |
| 0x00402780 | `StepSchoolCar` | 351i/1146B / 351i/1147B | 104 | 70.4% | 19 |
| 0x004070b0 | `GoldRush_KneelAtPan` | 58i/177B / 58i/177B | 4 | 93.1% | 15 |
| 0x00417430 | `TempleSlide_Update` | 347i/1122B / 347i/1122B | 22 | 93.7% | 116 |
| 0x00407c30 | `Joust_Update` | 703i/2261B / 703i/2258B | 16 | 97.7% | 500 |

The audit can include trailing NOPs when a draft is shorter than the original.
The remaining short StepFree draft has an actual unpadded body of 75i/216B.
BlockedAhead now has all 63 instructions in 179 bytes. StepSchoolCar has all
351 instructions in 1146 bytes; its trailing alignment and five-entry switch
table are excluded from that measurement. Raster has all 252 instructions
in 747 bytes, and StepToPieceEnd has its full 84i/250B body.
GatherParams now has its exact 70i/211B body; retained Refine and Kneel
improvements have the full 110i/356B and 58i/177B bodies respectively. Consequently the former Raster
claim of matching every instruction/byte except a uniform frame shift was
incorrect. No marker was promoted on the strength of equal audit totals.

## Measurements and findings

### Manoeuvre twins, draw table, rider model

- **Manoeuvre C/A:** strict/register-blind remains 3/0 and 10/0; complete
  side-by-sides show identical stack operands and only the second-arm scratch
  rotation. Sixteen new paired variants: separately naming each of nine waypoint
  rows, integer heading locals and three first-arm heading-update forms are
  inert. A byte heading regresses C to 86 (A stays 10); naming the final CarPos
  destination adds two tail-schedule mismatches to both. Prior volatile sweeps
  were not repeated. These levers leave the same measured floor, not proof of
  global unreachability.
- **InitTrackDrawModes:** named row, pointer-to-array row, byte cursor, named
  first field, one-int aggregate copy, for-clause cursor update and three free
  volatile field reads are inert. A postincremented row reaches the original
  zero bias but scores 48. A diagnostic cancelling first-field expression reaches
  zero bias with in/out/mode load order intact, yet changes cursor/out allocation
  and constant initialization (27); its sum/subtraction/unsigned forms agree.
  Reassigning in after the three reads scores 10. None retained. Resolving the
  baseline cursor as table+8 accounts for all four address differences.
- **CoasterCar_BuildRider:** separate block-local trip symbols are inert at 12.
  Loop-local template pointers with volatile reads in a/b/c score 103/63/37;
  a+c or all three score 104. Explicit lockstep record/trip cursors in a+c score
  135 in both increment orders and shorten the body to 538B. Baseline windows
  start 103/131/162; strict/register-blind 12/12, all stack operands agree.

### Mesh, view and polygon rasterization

- **BuildTrackMesh:** moving volatility to the source element scores 124 and
  removes its required home; keeping a scalar for both hooks plus the existing
  volatile home store also scores 124. Naming the first hook-table read is
  inert at 8. Residual is still reversed preheader stores plus a reload where
  the original copies the register; strict/register-blind/offset-blind 8/8/8.
- **DrawMesh:** moving s++ after k++ in the for clause regresses 8→18 at the
  same length. Strict/register-blind/offset-blind 8/4/8. Original computes
  `tri-source` and uses one stepping cursor; the absent preheader subtraction
  is an addressing-form difference, not just register naming.
- **InitTrackTopology:** corrected first divergence to **13**, not 97. The
  original compares scalar a and uses known row-5 stores; the draft compares a
  derived cursor and uses indexed stores. Moving a++ into the second store
  regresses 53→56. Strict/register-blind/offset-blind 53/49/52: structural work
  remains. Ring tail from 144 is exact; the first-96-exact claim was stale.
- **SetupView:** right-first half accumulator is inert at 10; mirrored form
  scores 12; naming config before the final eye.y store scores 31. All retain
  129i/466B. Strict/register-blind/offset-blind 10/4/10, correcting the old
  approximately-zero register-blind claim.
- **Raster_SubmitPoly:** retained condition reversal, **231→228**. Original
  fall-through mode=0/n-- is now reproduced. The draft actually has 250i/741B,
  whereas original has 252i/743B. Frame 0x1fc versus 0x200 is only part of the
  residual: homes coalesce/permutate, original preserves job through gradients,
  integer dy and inverse residency differ, and loop/sort operations differ.
  Separate integer dy scores 215 (253i/749B), separate inverse locals 209
  (252i/749B), their combination with reversal 227 (254i/752B); each worsens
  size/structure and was rejected. Volatile n 240, early flags 231, full
  PolyVtx-shaped gradient scratch 241 (244i/733B), also rejected.
  Retained strict/register-blind/offset-blind is 228/204/218.

### Routes, curve refinement and shading

- **BlockedAhead:** applying the coordinate-pair/sum-of-squares aggregate to
  the second distance and then both distances is inert at 27, first 36.
  Original finishes the second conversion before loading the other car's x;
  current scheduling crosses that boundary. Strict/register-blind/offset-blind
  27/22/27. Plain statement interleavings were already eliminated.
- **GatherParams:** explicit pointer/counter latch orders both regress 6→10,
  first 49, without recovering the dead g_tc_n reload after the inner loop.
  Current subscript form remains best; triage 6/5/6, first 64.
- **StepFree:** re-derived first mismatch 23, the shared snapshot address and
  argument/load schedule. Existing pointer, aggregate-copy and read-order
  experiments already cover the obvious spellings; no duplicate sweep run.
  Triage 53/43/53. A matched sibling with this exact copy lowering would be
  better evidence than another unmotivated variant.
- **Refine:** four new interactions combine a free b2 square-operand read with
  embedded slope assignment. Free reads alone are inert; combination extends
  exact prefix 11→19 and lowers strict 99→89 but shortens farther to audit
  352B (original 356B). Rejected: b2 residency remains wrong. Triage 99/96/99.
- **StepToPieceEnd:** first divergence is **1**, not just a shift after 25:
  current float compare precedes the saved-register pushes. Volatile dt-to-hi
  copy scores 76 but reads the wrong home at compare; volatile hi compare 80;
  reversed midpoint sum inert at 78. Prelude and missing snapshot LEA are
  separate open constraints. Triage 78/68/78.
- **Shade:** two counter/dst latch orders are inert at 6. Three early volatile
  green-width reads score 27/28/27, moving the frame home. Same 129i/401B
  baseline retained; first 74, triage 6/6/5. Setup must cross x87 cleanup and
  accumulator reloads without changing allocation.

### Gold Rush, school-car driver and joust callbacks

- **TempleSlide:** positive busy guard with current reads scores 258, first
  61; crossing it with post-call reads scores 327 and ESCAPES. Y accumulator
  form changes neither result. The tested guard/read matrix does not uncouple
  projection and loop-head allocation; retain 22.
- **Joust:** opposite goto busy guard scores 169, first 412, 2273B; baseline
  16/2258-original-B remains best. Confirms the callbacks require opposite
  guard forms. Seat-multiply and phase-two gate allocation remain open.
- **KneelAtPan:** earlier y capture still sinks to instruction 23 and returns
  to the previously recorded 26-mismatch volatile-x/carrier family, not a new
  optimum. Capturing y in the initial table store scores 50, or 46 with
  volatile x. Rejected; baseline retained.
- **StepSchoolCar:** free reads at wx/wy/both definitions regress 319 to
  329/329/330 and first divergence 19→5. Seven straight-line push-depth
  assertions reconfirm displaced homes: swx/swy current E+08/+0c versus
  original +0c/+10; tx/ty +04/+10 versus +24/+28; tw2/th2 +14/+18 versus
  +04/+08; th/flag E+0 in both. Equal frame size is not equal layout.
  The ride lane used aligned register/immediate-blind LCS as an extra
  diagnostic, not full CFG frame-aware triage; those scores are not promoted
  to offset-blind evidence here.

Mesh/route triage resolves ESP/EBP homes by push depth before blinding. These
classifiers and bounded negative probes do not establish that all possible
source forms are exhausted. All unsuccessful implementation variants remain in
scratch; none were retained for an improved score alone.

## Mechanics, names and original behavior

No new symbol names, globals, callee declarations, extern-type divergences or
original bugs were introduced. Existing behavior is preserved, including the
raster gradient store-before-clamp bug and the kneel/stand offset asymmetry.
The original topology emits five sequential seam rows then row 5 closes to zero;
DrawMesh eliminates the destination cursor. These are useful constraints for
further reconstruction. Route stepping, bisection and curve refinement behavior
was reconfirmed; no gameplay functionality was added in this checkpoint.

## Initial-pass follow-up hypotheses (historical)

1. **InitTrackTopology:** isolate a row cursor and scalar a specifically in the
   first seam loop, preserving both the scalar-bound comparison and known row-5
   stores. Previous pointer-walk experiments concern the later replication loop.
   Fix the earliest structural difference before the later allocation cascade.
2. **Raster:** investigate arm-local inverse definitions from the common divide,
   allowing arithmetic hoisting while preserving separate homes. The original
   stores to -0x24/-0x38 in opposite arms after a shared fdiv; this pass tested
   copies of an already-homed inverse. Preserve the integer dy web without
   adding the extra operations introduced by the naive copy.
3. Remaining route/shading work needs a new liveness or copy-lowering construct,
   not another repeat of the recorded read/store-order sweeps.

No exact function closed. This is a verified initial checkpoint, not completion
of every partial. Global verify/progress/coverage are deliberately reserved for
the integrating session, as required by the parallel contract.

## Reproduction and local evidence

```sh
export LEGOLAND_CL='/Users/systemadmin/Downloads/alpha team/alphateam/tools/wibo-msvc/cl'
python3 tools/audit.py LEGOLAND/schoolcar2.c LEGOLAND/schoolcar3.c LEGOLAND/schoolcar4.c LEGOLAND/schoolcar5.c LEGOLAND/schoolcar6.c LEGOLAND/coaster3d.c LEGOLAND/coaster4.c LEGOLAND/coaster6.c LEGOLAND/goldrush.c LEGOLAND/goldrush3.c LEGOLAND/joust.c LEGOLAND/joust2.c
```

Full initial/final output: `scratchpad/scope-g/baseline.txt`, `final-audit.txt`.
Individual reconstruction listings, complete scratch variants, their audits,
probe drivers, frame assertions, triage and lane reports live under
`scratchpad/scope-g/`. These local artifacts are not committed; the durable
results and tested hypotheses are recorded above and at each assigned WIP.
Neither original data nor generated objects are staged. Explicit experiment
objects use `/tmp/sg_*`; audit uses its own per-process `/tmp/_audit_*` path.


## Fable/Opus continuation from 341d8d9 (2026-09-05 evening — 2026-09-06)

Continued in a separate worktree on `scope/G-fable` (Codex's worktree and
scratch were read, never written). Method: per function, three Opus lenses
(witness scan over the exact corpus, fresh reconstruction from the
disassembly, lever search) on private copies, then Fable or Opus passes aimed
at whatever single decision those lenses pinned down; every claim verified
independently (target `[OK]`, all other rows unchanged, diff confined to the
one body, semantics against the disassembly, `/W3`).

**Thirteen of nineteen exact** (Codex closed eight, this continuation five).

### Closures and their levers

- **CoasterCar_BuildRider 0x00421660** — a record pointer needs a SECOND USE
  in the source (`rp->idx` as the template lookup and again as the store's
  subscript). VC6 CSEs the two loads back to one, but address selection has
  already seen `rp` consumed twice, so it materialises `list + i*16` instead
  of folding it into the load's base+index mode. A corpus scan of 2,028 exact
  bodies found no compiler-generated unfolded `add r32,r32` feeding a memory
  base with a single self-consuming use; every instance has a second consumer.
  A second use of a SUBSCRIPT does not work, only of an ADDRESS.
- **Coaster3D_InitTrackTopology 0x00428f00** — the three "scheduling
  differences" were the register RANK of three loop-carried values, decided in
  the OTHER loop. Written with `base` as the pair table's own row index bumped
  per pair inside a counted `for`, it outranks `out` and `s`, wins ebx, and
  VC6 eliminates it by final-value replacement (`add ebx,esi` in the
  preheader). Companion rule: the seam reads are indexed from a local pointer
  anchored at the SECOND field. Both volatile shims removed.
- **GoldRush_KneelAtPan 0x004070b0** — capture the compound assignment's
  RESULT (`y = (t->y += key->by << 8)`) so the field's own load heads the web
  and takes edi across `__ftol`. Every separate capture fails: adjacent to the
  table store VC6 forwards it; after the x statement it sinks the load into
  the y statement. Volatile removed.
- **Route_StepFree 0x0041e000** and **Route_StepToPieceEnd 0x0041df00** — one
  shared lever, a FRAME misreading. Phys_Step's second argument is a lone
  float it accumulates into (its own exact body only reads and increments
  `ctx->t`), so the dword each routine stores below it is the twelve-byte
  snapshot RECORD'S OWN FIRST FIELD landing in its stack home. VC6 forwards
  the two fields that are read and drops their stores; a block-copy field
  never loaded keeps its store. Spelling it as `ctx.f04 = saved.f` into a
  16-byte context made that field a forwarded value with a competing use,
  which split the record's base out of eax. For PieceEnd a second error
  remained: with the frame right, only the record copy placed FIRST (so the
  `rt` load heads the block) puts `fnstsw` after the copy instead of inside
  it; all 24 prologue permutations were measured.

### Open, with the residual re-characterised

- **Joust_Update 0x00407c30 — 1 mismatch** (703/703, 2262/2258 B, index 673).
  Closed 16 → 1 on two levers: phase two has only ONE `f20 = 1` store, reached
  by `goto` from the other busy arm (a reference-count lever: the gate web
  drops from five references to four and the callee-saved pair flips); and
  case 0x19 is written BEFORE case 0x16 (a code-free cross-jumped case still
  rotates the scratch-register cursor for every case after it). The last
  mismatch is the original's exiled arm ending `cmp dl,3 / jmp` INTO a
  conditional jump — a cross-jump shape that occurs exactly ONCE in the whole
  executable. A synthetic translation unit holding only phase two reproduces
  this toolchain byte-for-byte and pins the cross-jumper's rules: a merge into
  the join's fall-through predecessor always stops at the jcc; a merge of two
  jumping tails carries it only from five instructions up (the original's
  region is three, and its free pair, also three, is unmerged in the binary).
  Only `/O1`, `/Os` and `#pragma optimize("s")` produce the shape, and they
  rewrite the function. Treat as a floor unless the flag story changes.
- **Coaster3D_BuildTrackMesh 0x00428cb0 — 6** (was 8). The preheader pair
  closed by deriving the vertex cursor from the loop counter at the top of the
  body (`out = g_track_verts + i * 6`), so both induction variables form in
  body order and the homing stores follow. The last six are the window where
  ours reloads the element from its frame home and the original copies the
  register it just loaded. The note's old claim that this is the whole
  one-byte deficit is wrong; the frame is exactly 0xc8 with nothing taking the
  address of -0x14.
- **SchoolCarBlockedAhead 0x00402490 — 25**, a pure three-cycle of the four
  callee-saved registers over an otherwise identical body. RETRACTION: the
  earlier rule ("registers go out esi, edi, ebx, ebp by descending web rank;
  p must rank second") is wrong. `p` ranks LAST in the original, rank is not a
  reference count, adding p references does not move p monotonically, and
  adding c references never moves c. esi/edi are not loop-invariant here: they
  are reloaded from `c` every iteration. Three independent rules, not one.
- **TempleSlide_Update 0x00417430 — 18** (was 22). The two axes are spelled
  differently: X's add destination is the ox-minus-scroll temporary (two-def
  `sx2 = delta + px`), Y accumulates in place. FALSIFIED: the standing
  explanation that the two world reads sit above the GetScreenCoordsForObject
  call is impossible — the original's loads are at indices 119/120, inside the
  post-call block, and VC6 never schedules a load across a call, so the
  original's source reads them BELOW it. The busy-guard form is byte-free.
- **StepSchoolCar 0x00402780 — 83** (was 319 at the checkpoint), now
  351i/1147B, the original's length to the byte, with the volatile reads gone.
  The saved map square is an eight-byte struct copy (`frame.saved = c->cur`),
  which VC6 lowers after CSE so the original's double loads survive where
  plain field assignments fold them. Register-blind is down to 20 and the
  aligned register/offset-blind LCS is 342 of 351.
- **Raster_SubmitPoly 0x0042a2f0 — 145** (was 231 at the checkpoint). The
  frame (derived by push depth) and the control flow are the original's; the
  residual is one register-colouring decision — the original colours the three
  interfering pointer webs jv=esi, job=edi, kp=ebx, ours swaps job and kp —
  with everything else downstream. Reading the PolyJob parameter as TWO webs
  (the original loads `[ebp+0xc]` at 0x0042a2fc and again at 0x0042a578) moved
  every scalar home and six frame offsets. NEW: a volatile-read local for a
  parameter is unconditionally RANK 1 and takes esi, so the shim an earlier
  lens added to split the parameter is itself what pins the colouring; the
  next attempt needs a non-volatile way to get two webs.

Not targets, unchanged: `ZBuffer_FillPoly` (partly hand-written `__asm`),
`Coaster3D_BuildPieceGeometry` (exact in its window; the extent walker cannot
bound it), `MatMul`.

## Codex continuation from 7a9eb866 (2026-09-06)

Requested continuation of `scope/G-fable` in a new worktree. Fetched that
branch and created `codex/scope-g-fable` at `7a9eb866`, in
`.worktrees/scope-g-fable`. The existing main checkout and other worktrees
were left alone. This checkout uses the existing Python environment at
`/Users/systemadmin/.venvs/legoland/bin/python` and compiler wrapper at
`/Users/systemadmin/Documents/Development/Github/alphateam/tools/wibo-msvc/cl`.
The ignored `original/legoland.exe` and `toolchain` link to the existing inputs.

**Result: still 13/19 assigned functions exact. No new exact match or source
improvement was retained.** The six open bodies are byte-for-byte unchanged
from the fetched branch. This is a record of a bounded unsuccessful attempt,
not completion or proof that these functions cannot be matched.

### Verified checkpoint

The authoritative twelve-file `audit.py` run ends PASS with **101 existing
exact functions**. All twelve files also compile without warnings at
`/W3 /O2 /Gy /Gd`. An additional check of the 101 exact bodies found no
per-instruction width, internal direct-branch destination, or switch-table
destination differences. No exact markers were added or removed.

The six assigned WIPs have these unchanged measurements. The percentage is
normalized instruction-position agreement; it does not excuse a byte-length
or branch-destination difference. Each still has a `WIP-FUNCTION` marker and
does **not** pass as an exact `[OK]` body.

| Address | Function | Instructions | Compiled/original bytes | Mismatches | Agreement | First index |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| 0x00428cb0 | Coaster3D_BuildTrackMesh | 151/151 | 442/441 | 6 | 96.0% | 27 |
| 0x00402490 | SchoolCarBlockedAhead | 63/63 | 179/180 | 25 | 60.3% | 1 |
| 0x0042a2f0 | Raster_SubmitPoly | 252/252 | 742/743 | 142 | 43.7% | 6 |
| 0x00402780 | StepSchoolCar | 351/351 | 1147/1147 | 83 | 76.4% | 19 |
| 0x00417430 | TempleSlide_Update | 347/347 | 1122/1122 | 18 | 94.8% | 116 |
| 0x00407c30 | Joust_Update | 703/703 | 2262/2258 | 1 | 99.9% | 673 |

### Experiments and remaining constraints

176 candidate translation units compiled and received a whole-file comparison
against every exact neighbor; none regressed an exact neighbor. Three other
candidates failed to compile and were discarded. One compiled boolean-switch
candidate emitted C4145 and was also rejected. All experiments remain in
local scratch, including negative results; none changed a tracked C file.

- **BuildTrackMesh:** crossed one-pointer aggregate copies with source,
  destination and extraction volatility. A volatile aggregate source still
  gives the nonvolatile family's 441-byte/42-mismatch body. A volatile
  aggregate extraction reproduces the existing 442-byte/6-mismatch body.
  Intermediate copies add homes or remove the required home. Neither copy
  qualification nor conditional extraction supplies the required register
  copy before the element's frame store. The unresolved window remains
  indices 27–32; the complementary plain record is not an exact result.
- **BlockedAhead:** explicit loop guards, `for`, goto guards, separate
  second-stage scalar/coordinate symbols, parameter copies, head/latch
  volatile reads, and an initialized ahead-point record do not fix the
  saved-register permutation. The best candidates reproduce 25 mismatches;
  moving the parameter read or making the result volatile worsens the body.
  These results only eliminate the tested forms, not all ways to change the
  register allocation in the existing reload regime.
- **Raster:** ordinary parameter copies, a record copy of the parameter,
  marking its address taken, and splitting its source lifetime do not recover
  the original allocation. Moving key initialization later worsens the body.
  Empty pointer/field tests, cancelling pointer subtraction, and identity
  assignments inside the edge loop are eliminated without changing the
  142-mismatch result, including repeated copies of those references.
  Re-tested separate integer-sign/conversion locals against this branch's
  corrected edge-loop entry, crossing conversion placement with plain versus
  volatile parameter/count reads. The best of those candidates is 153
  mismatches and still has the wrong frame/control schedule. An explicit
  integer/float union reproduces the separate-conversion-local result; it
  does not recover the original integer dy residency.
- **StepSchoolCar:** four-byte copies eliminate the needed dead projection
  store and give 321 mismatches/1141 bytes; eight-byte `memcpy` reproduces
  the existing two-store, 83-mismatch body. Volatile whole-record source
  copies add a frame home; volatile destination copies give 90 mismatches
  at the original length. Source/destination read-back variants and scalar
  writes adjacent to the record copy leave 83. The single dead X store and
  the sum-first projection are still unresolved together.
- **TempleSlide:** ordinary projection record copies remove the required
  store/home. Grouping the spill with the escaping path-position record
  preserves a plain store but changes frame layout (48 mismatches), while
  a single explicit frame record changes rider-cursor loads and allocation.
  Reusing case-local scalar symbols, scoping the tile pointer, and splitting
  the call's pointer expression do not restore the loop-head allocation with
  the world reads after the call. Crossed three Y-expression forms with five
  screen-Y capture positions and plain/volatile reads; none improves 18.
  The pre-call world-read shim remains, with its five-position discrepancy
  explicitly unresolved.
- **Joust:** sharing a boolean readiness result between the horse arms changes
  the phase-two layout (82–83 mismatches); equivalent nested/boolean-switch
  spellings either reproduce the baseline or shorten the wrong layout.
  Index 673 remains conditional versus unconditional branch, and the
  separate case-0x16 walk-tail destination still accounts for the other
  three bytes. A headline of one normalized mismatch is not one byte away
  from a verified full-body match.

No new mechanics, names, extern types, globals or callees were introduced.
Existing original-behavior reconstructions and documented bugs are unchanged.
In particular, no assembly replacement or compiler-flag relaxation was used
to claim completion. Shared tools and global progress reports were untouched.

### Local reproduction evidence

`scratchpad/scope-g-codex/baseline.txt` contains the authoritative audit.
The `*_base.json` records contain the twelve warning-free compiler results;
`baseline-control-flow.json` contains the 101 width/branch/table checks.
The seven experiment drivers, their result text files, and each
candidate's `.c`, `.sbs` and `.json` retain the complete experiments.
Objects are under `/tmp/sgc_*`. Scratch and generated objects are not committed;
this appendix is the durable result of the attempt.

## Focused reconstruction follow-up (2026-09-06)

The requested second attempt focused on `Raster_SubmitPoly` and
`StepSchoolCar`, starting from the original instruction stream and its
temporary lifetimes. **No improvement was retained: 13/19 assigned functions
remain exact.** All six WIP bodies, markers, and checkpoint measurements in
the table above are unchanged. This bounds the experiments below; it does
not establish that 100% is impossible.

### Reconstruction evidence and tests

- **Raster, 0x0042a2f0:** the original keeps the signed integer height in a
  register across `fild/fstp` into one stack slot, then tests the integer
  register. Its downward and upward reciprocals finish in different homes.
  Tested distinct ordinary/volatile float values, integer/float conversion
  helpers, saved predicates, and separate flat/outer/inner loop counters.
  Ordinary scalar and record float conversions score 141 rather than 142,
  but produce the wrong conversion/division sequence and 744 rather than
  743 bytes. That one-position change is not a retained improvement.
- **Raster control structure:** duplicated conversion/division in the two
  direction arms, a shared increment after the arms for each combination of
  edge count/edge cursor/key cursor, and a separate current-key pointer all
  fail to recover the original allocation and loop structure. Duplicating
  only the reciprocal is inert on the current volatile-read baseline.
  Whole-key record assignment also regresses. None supplies the original
  job/vertex-base/key-cursor register assignment together with the original
  stores, loop count and frame.
- **StepSchoolCar, 0x00402780:** the original computes the sum first using a
  separate scratch register, preserves only the dead projected-X store,
  and places its final depth addition after the screen-Y store. Tested
  mixed volatile/plain projected-field stores, a locally qualified X
  member, world/target snapshot record copies, and scalar/record projection
  helpers with an output parameter or an eight-byte return. These do not
  recover those three requirements together. The ordinary eight-byte
  return assigned to `frame.target` reproduces the existing 83-mismatch
  result; the local return without that copy drops the required store and
  gives 321 mismatches/1141 bytes. A qualified X member keeps the original
  byte length but regresses to 245 mismatches. No helper or local-type
  change was added to tracked source.
- **Processor tuning, diagnostic only:** compiled both unchanged translation
  units with each of `/G3`, `/G4`, `/G5`, and `/G6` in addition to the required
  flags. The first three leave their strict scores and sizes unchanged.
  `/G6` worsens Raster to 187 mismatches and StepSchoolCar to 276, and also
  changes 3 and 15 previously exact neighbors respectively. The options
  were accepted without warnings. These tests provide no basis for a
  compiler-setting workaround; the required build flags remain unchanged.

98 distinct candidate translation units compiled under the required flags
and were compared against every exact neighbor in their file; no candidate
changed an exact neighbor. Seven exploratory candidates warned about an
unused local and were rejected. Two initial current-key forms failed due
to a declaration after statements; corrected forms compiled and also
regressed. The eight processor-tuning builds are separate from the 98
source candidates. No generated source, objects, or helpers are committed.

The final twelve-file authoritative audit again reports PASS with **101
existing exact functions**, and all twelve files compile cleanly at
`/W3 /O2 /Gy /Gd`. Tracked C files are identical to the previous checkpoint;
its additional exact-body width/branch/table checks therefore still apply.
There are no new names, callees, globals, mechanics, extern-type changes,
or changes to the reproduced original bugs.

Local reproduction evidence is in `scratchpad/scope-g-reconstruction/`:
six source-experiment drivers, their shared probe, and candidate
`.c`/`.sbs`/`.json` records,
`processor_tuning.py` and its JSON results, `final-audit.txt`, and
`final-warnings.json`. Objects use `/tmp/sgr_*`. As before, scratch remains
local and this note records the unsuccessful result durably.
