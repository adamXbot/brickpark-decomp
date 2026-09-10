# FGH-100c — the tie-break rule fitted from the exact corpus, applied to the six remaining WIPs

Branch: `cursor/fgh-100c`  
Worktree: `.worktrees/fgh-100c`  
Base: `cursor/fgh-100b` @ `14b064c3`  
Object prefix: `/tmp/fgh100c_`  
Scratch: `scratchpad/fgh100c/` (the dataset/rule scripts are tracked; probe
outputs under `scratchpad/fgh100c/probes/` are not)  
Environment as fgh-100b (`PY=/Users/systemadmin/.venvs/legoland/bin/python`,
`LEGOLAND_CL=.../alphateam/tools/wibo-msvc/cl`, `PYTHONPATH=/private/tmp/legoland-fgh-deps`).
Baseline `validation/fgh/check.py` → `assigned_exact_gate: 42`, `assigned_open: 6`,
`existing_normalized_regressions: 0` (`/tmp/fgh100c_baseline.json`). **Still 42/48
at the end of the round; no production body was changed.**

## Checkpoint

The brief asked for a different kind of round: instead of probing spellings,
derive VC6's register tie-break empirically from the ~2,700 exact bodies and
use the rule to name the feature each residual must move. That was done
(§1–§3), and the honest result is in two parts:

1. **The rule that the corpus supports is a loop-weighted use count, weight
   ≈2–4 per nesting level, uses only (definitions do not count), ties to the
   earlier definition** — with a fixed register order esi → edi → ebx → ebp
   (byte-addressed webs pulled to ebx). It predicts *which web gets ESI* at
   ~92–96% and the order among the remaining registers at ~85%, but the
   **ebx-vs-ebp choice only at ~75%**: a quarter of the ebx/ebp assignments in
   the exact corpus are decided by something no asm-visible feature of the two
   webs explains (§2.4). Interference degree looked like the dominant feature
   on the first cut (96%) and turned out to be mostly a selection artefact of
   the pair filter (§2.3); on the bias-free subset it adds nothing.
2. **Applied to the six WIPs (§3), the two callee-saved residuals are not
   near-ties that a count could tip — the ORIGINAL violates the corpus rule at
   both sites.** Temple's original gives ebx to `ty` (4 uses, degree 4) over
   `sq` (8 uses, degree 5); Raster's original gives ESI to `&v[0]` (weighted
   13) and EBX to the edge-loop cursor (weighted 20, the top-ranked web).
   Both sit in the corpus' unexplained ebx/ebp cell. JungleCruise's "outer
   allocation" turned out to be the same cell: the de-shim swaps EBX/EBP
   between the zero constant (29 uses) and `inst` (7 uses) with *identical*
   feature rows (§3.3). The other three residuals (SpaceTower head order, Mesh
   copy-before-store, StepSchoolCar rotation) are scratch-register schedule
   decisions with identical callee-saved allocations on both sides; the rule
   has no purchase on them.

Part B (§4) was therefore bounded to the one pair the rule and RA09 jointly
motivate — JungleCruise's de-shim × instance-loop head order — which restores
the original's structure at every site the WIP note names (case 1 through
`st`, the `bl` lea inside case 0, zero→EBX / `inst`→EBP) at 56 mismatches /
1111 B, i.e. worse than the incumbent's 11 because it moves the `b` load and
leaves a `st`↔`seat` EAX/ECX rotation. Not promoted.

## §1 Dataset — `scratchpad/fgh100c/webs.py`, `fnload.py`, `dataset.py`

**Unit of analysis.** The register allocator ranks *webs*, so the dataset is
built from webs read off the ORIGINAL bytes of every exact `// FUNCTION:` body
(2,717 bodies, 145,404 instructions, `lib.py` corpus cache), not from C. For
each body `webs.py` builds the CFG (jump tables through `match._table_targets`),
natural loops (dominator back edges), per-register reaching definitions and
liveness, unions def/use chains into webs, and then chains *in-place
redefinitions* (`shl ebx,8` / `add edi,ebx` / `mov bl,[..]` whose only
predecessor use is the redefinition) into **extended webs** — what one
register holds continuously, the allocator's coalesced live range. Prologue
`push`es of the entry value and epilogue `pop`s are not references; `call`
clobbers eax/ecx/edx; an EBP frame (`mov ebp,esp`) removes ebp from the
analysis; string-instruction operands (`rep movsd` etc.) and byte-accessed
webs are flagged as *constrained* (esi/edi and ebx respectively are not
choices there).

**Features per extended web** (all asm-derived, so the same instrument reads
an original body and a compiled candidate identically): `n_uses`, `n_defs`,
per-depth def/use histograms (so any loop weight can be scanned), `first_def`,
`first_use`, `last_use`, `span`, `live_n` (instructions live), `live_blocks`,
`ref_blocks`, `calls` crossed, `addr` (uses as a memory-operand base), `iv`,
`loop_carried`, `degree` (webs ever live alongside), `deg_cs` (of which
callee-saved), `deg_max`, `kind` of the first def (param / local / load / copy
/ lea / const / arith), `byte`, `word`, `string`, and first-member /
max-member counts of the chain.

**Register preference order, measured.** Functions using exactly one
callee-saved register: esi 460, edi 28, ebx 9. Two: esi+edi 339, esi+ebx 21
(18 of them byte webs). Three: esi+edi+ebx 215 vs esi+edi+ebp 17 — and only
27 of 194 unconstrained esi+edi+ebx functions have a byte web in ebx, so EBX is
genuinely third, not "the byte register". Same order in EBP-frame functions.
The 17 esi+edi+ebp exceptions hold early-defined parameters/constants or
16-bit-accessed values (`mov word ptr [esi+4],bp`, `mov bp,word ptr [..]`) in
ebp — the first sign that the ebx/ebp choice is not purely rank.

**Rows.** One row per interfering pair (A, B) of extended callee-saved webs
with A in the more-preferred register: 10,176 pairs. A row is `clean` when B
could have taken A's register (no third web in A's register interferes with
B), so under a priority-first/first-free hypothesis the row is a pure rank
statement: 5,966 clean, 4,962 after removing byte/string-constrained pairs.
Site flags: `loop_head` (both live into a loop header: 2,515) and `join` (both
live into a block with ≥2 predecessors: 3,697). Held-out split: 20% of
functions by VA hash. `python dataset.py` rebuilds `/tmp/fgh100c_pairs.jsonl`
in 25 s.

## §2 The fitted rule — `fit.py`, `failures.py`, `disagree.py`, `logit.py`, `policy.py`, `fit_report.txt`

Comparator convention: "A wins iff key(A) > key(B)", abstaining on ties;
accuracy is on decided rows, coverage is the decided fraction.

### 2.1 Clean pairs (4,962)

| comparator | accuracy | coverage |
| --- | ---: | ---: |
| `deg_max` (max simultaneous live webs) | 0.980 | 0.23 |
| `deg_cs` (interfering callee-saved webs) | 0.962 | 0.63 |
| `uses + 2·deg_cs` | 0.925 | 0.91 |
| `n_uses` (static use count) | 0.901 | 0.85 |
| `W2u` (uses weighted 2^depth) | 0.901 | 0.87 |
| `refs` (defs + uses) | 0.903 | 0.86 |
| `w8` / `w10` (defs+uses, ×8 / ×10 per level) | 0.848 / 0.846 | 0.89 |
| `live_n`, `span` | 0.80 | 0.98 |
| `−first_def` (earlier definition wins) | 0.754 | 1.00 |
| `refs / live_blocks` (Chow priority) | 0.451 | 0.87 |

Greedy lexicographic fit (train 3,854 / held-out 1,108): `uses+2·deg_cs`
(0.927, decides 91%) then `−first_def` (0.60 on the ties) → **held-out
0.890**. A 20-feature logistic model on feature differences is 0.900 held-out,
so this is the floor of what asm-level features carry on the clean set.
Failures by class: the `ebx`/`ebp` pair is wrong 24% of the time under `refs`
against 5–12% for every pair involving esi or edi; `(load, const)` /
`(const, const)` kinds and `loop_carried = (0,1)` rows are the other
over-represented failure classes. Constants are *not* systematically
deprioritised (a const with more uses wins 321 : 60).

### 2.2 What the weights say

* **Loop weighting is mild.** ×8 / ×10 per level (the folklore weights) are
  *worse* than the plain count (0.848 vs 0.903): a depth-0 parameter with 14
  uses beats a depth-1 web with 7 in `BsWater_Relink`; 4 uses at depth 0 beat 2
  at depth 1 in `SoftPrint_XBltFast`; 17 uses at depth 2 beat 4 at depth 4 in
  `ZBufferHelper`. On the bias-free subset (§2.3) the best weights are 3–4 per
  level and on esi/edi pairs W4u reaches 0.915. **Definitions do not count**
  (`W1` = defs+uses 0.888 < `W1u` = uses 0.901); `calls` crossed add nothing
  beyond uses.
* **Register class is the strongest binary fact**: a web that crosses no call
  and sits in a callee-saved register is there only because the scratch
  registers were exhausted, and those rows are the noisiest (32% wrong).

### 2.3 The degree signal is mostly a selection artefact

`deg_cs` at 96% looked like the §7 interference-set rule confirmed, but the
`clean` filter is biased: a long web parked in the *worst* register is
excluded whenever the better registers are reused by short webs (a third web
in A's register interferes with it), while a long web in the *best* register
is kept. On the bias-free subset — **functions where every callee-saved
register holds exactly one web** (829 pairs, no third web anywhere) —
`deg_cs` is nearly constant and the ranking is carried entirely by the
weighted use count: `W3u` 0.861 / `W4u` 0.859 (81% coverage), `refs` 0.844,
then `−first_def` 0.65 on the ties; held-out 0.79. Per pair on that subset:
esi/edi 0.915 (W4u), ebx/ebp 0.73 at best (`live_blocks`).

### 2.4 The rule, stated, and what it does not explain

**Rule (corpus-supported).** Callee-saved candidates are ranked by
loop-weighted use count (uses only; weight 2–4 per nesting level; ties to
the earlier definition) and take registers in the fixed order esi, edi, ebx,
ebp, a byte-accessed web being pulled to ebx. Accuracy: ESI choice 0.92–0.96,
edi-vs-ebx 0.87, edi-vs-ebp 0.88, **ebx-vs-ebp 0.75**; whole-function
simulation (`policy.py`: rank, then first free register not held by an
interfering placed web) reproduces the complete callee-saved assignment of
only **56% of 700 unconstrained functions** (63% of webs). A round-robin
register choice (scan starting after the last register given) is worse
(35%), so the "rotation" the earlier rounds describe is not a cyclic register
pick in the callee-saved class.

**Not explained.** (a) Which of two webs gets ebx vs ebp, a quarter of the
time; the wrong rows are the *shorter, later-defined, lower-degree* web taking
the better register (`first_use` 0.75 and `−live_blocks` 0.80 on the wrong
rows vs 0.26/0.35 on the right ones). (b) Register *reuse*: VC6 takes a
fresh callee-saved register for a web whose predecessor in a free register
is dead (`SpaceTower_CountSeated`: loop-2 temp → ebp while edi was free), so
its allocation is not interference-driven at the du-web level — a
symbol-lifetime or region allocation would behave this way. (c) 16-bit
accessed values prefer ebp (`NewSchoolCar`, `MoveIcons`).

## §3 The six WIPs under the rule — `wipcmp.py`

`wipcmp.py <Name> [--tu file.c]` compiles the TU, resolves the compiled jump
tables through the COFF relocations, runs the same web analysis on both bodies
and prints the callee-saved (or all) extended webs side by side. Uses
(`u`), loop-weighted uses W4, live instructions, callee-saved degree (`d`),
first-def index.

### 3.1 TempleSlide_Update — the head tie is a rule violation in the original

| web | original | reads-below FLIPPED base (327) | GOOD-tail form (238) | incumbent (18, shim) |
| --- | --- | --- | --- | --- |
| `b` | esi u79 W316 d15 | esi u80 d18 | esi u76 d15 | esi u79 d15 |
| `tx` | edi u6 W24 d4 | edi u6 d4 | edi u6 d4 | edi u6 d4 |
| `ty` | **ebx** u4 W16 d4 live22 | **ebp** u3 d4 | **ebx** u4 d4 | **ebx** u4 d4 |
| `sq` | **ebp** u8 W32 d5 live70 | **ebx** u8 d7 | **ebp** u8 d5 | **ebp** u8 d7 |

The rule ranks `sq` (8 uses, degree 5, live 70, 14 blocks) above `ty` (4
uses, degree 4, live 22, 6 blocks) by every feature, so it predicts the
FLIPPED family (`sq`→ebx, `ty`→ebp) — which is what the honest reads-below
source produces. The original and the GOOD-tail form give ebx to `ty` with
*identical* feature rows to the FLIPPED base except `sq`'s degree (5 vs 7).
There is therefore **no count that must change**: for `ty` to win under the
rule it would need more than 8 uses (or a degree above `sq`'s), which is
several real references. The head decision is not a rank tie-break; it is the
ebx/ebp choice the corpus cannot predict, moved here by the tail grouping
(§8 of fgh-100b) — carried allocator state, as that session read it. The
incumbent's shim buys the GOOD family by raising `sq`'s degree to 7 (two
interference edges from `wx`/`wy`), i.e. it moves a feature the original
does not move.

### 3.2 Raster_SubmitPoly — the 3-cycle is a rule violation in the original

| web | original | incumbent (108) |
| --- | --- | --- |
| `job` (param) | **edi** u7 (5@d0, 2@d1) W4=13 d2 live83 | **esi** u7 W4=13 d2 |
| `&job->v[0]` | **esi** u4 (1@d0, 3@d1) W4=13 d2 live79 | **ebx** u4 W4=13 d2 |
| ring cursor | **ebx** u5 (all @d1) W4=20 d18 live171 loop-carried | **edi** u6 W4=24 d16 |

By the rule the edge-loop cursor is the top web (W4 20, degree 18) and should
hold ESI; the original puts it in EBX behind two W4-13 webs, and among those
two gives ESI to the one with fewer uses. Neither the original nor the
incumbent is rule-consistent; the source note's own measurement ("3+
parameter references inside the edge loop give the original colouring" — more
weight on `job` moves `job` *out* of ESI) is the same inversion seen from the
other side. Frame-home order follows the same unknown.

### 3.3 JungleCruise_Tick — the "outer allocation" is an ebx/ebp swap with equal features

`st` (ecx, scratch): original 20 uses, incumbent 18 — the two missing uses are
case 1's `cmp esi,[ecx+18h]` (207) and `mov [ecx+18h],ebx` (224), read
through `bl`/edi in ours. `bl` (`lea edi,[st+18h]`): original defined at 132
inside case 0 (2 uses, live 6); incumbent at 126 at the dispatch (4 uses, live
29). The de-shimmed variants of fgh-100b (`bl_c0_after_seat`, 84) have the
original's structure at both sites and lose the "outer allocation" — which
`wipcmp.py` shows is exactly:

| web | original | de-shimmed (84) |
| --- | --- | --- |
| zero constant (`xor r,r`) | **ebx** u29 W128 live340 calls22 d7 | **ebp** u29 W128 live340 d7 |
| `inst` (instance cursor) | **ebp** u7 W25 live149 calls10 d5 | **ebx** u7 W25 live149 d5 |

Identical rows, opposite assignment: the unexplained ebx/ebp cell again. The
rule says nothing has to change in either web's count; what moves the pair is
statement order at the instance-loop head (RA09) — see §4.

### 3.4 StepSchoolCar, SpaceTower_Activate, Coaster3D_BuildTrackMesh

Callee-saved allocations are identical between incumbent and original in all
three (`wipcmp.py`: StepSchoolCar `esi` u76 / `ebx` lea u6 / `ebp` u4 / `edi`
u5 on both sides; SpaceTower `esi` u48 / `edi` u14 / `ebp` u4; Mesh `edi`
param u4 / `ebx` const u3). The residuals are scratch-register decisions —
the eax/ecx/edx rotation over StepSchoolCar's call block, the head load order
in SpaceTower (a scheduling cycle, §7-2 of fgh-100b), and which of two
equal-feature scratch webs owns Mesh's first push (`mov esi,ecx` copy of the
element in the original vs a `[ebp-14h]` reload in ours, both 2 uses across 2
calls). A callee-saved rank rule has no purchase on them and the dataset does
not cover scratch webs (their ranking is dominated by definition order, which
is what "rotation" means).

## §4 Part B — the one pair the rule motivates — `jc_v5.py`

Because §3.3 identified JungleCruise's outer allocation as the zero/`inst`
ebx/ebp pair, and RA09 records that this very pair answers to the instance-
loop head order (`b = inst->bloke` before vs after the station search), the
bounded experiment was the 2-D sweep: **case 1 through `st` + `bl` defined
inside case 0 (the original's structure) × every dependency-valid order of the
five head statements** (`st = g_jc_stations`, `next = inst->next`, `key =
inst->key`, the search `while`, `b = inst->bloke`), 30 variants plus four
with `bl` first in case 0 (`scratchpad/fgh100c/probes/JungleCruise_Tick/v5/`).

| form | mismatches | bytes | first diff | reading |
| --- | ---: | --- | ---: | --- |
| incumbent | 11 | 1111 | 126 | hoisted `bl`, case 1 via `bl[0]` |
| de-shim, `b` after the search (base order, 7 variants) | 84–86 | 1109–1110 | 3 | zero/`inst` swapped ebx/ebp |
| **de-shim, `b` before the search, `next` before `key` (15 variants)** | **56** | **1111** | 110 | outer allocation restored; `bl` lea inside case 0 at 131; case 1 reads `[st+18h]` at 207/224 |
| de-shim, `b` before the search, `key` before `next` (12) | 58 | 1110 | 110 | as above, one head byte |

The pair does what §3.3 predicts — the zero constant is back in EBX and `inst`
in EBP with `b` loaded before the search, in every one of the 15 orders — and
the body now has the original's structure at every site the WIP note names.
What remains (56): the `b` load itself is now scheduled at 112 where the
original loads it at 122 (after the search, as the base order has it), and the
whole case-0/case-1 region is one EAX/ECX rotation (`st` in eax, `seat`/`i` in
ecx; original the reverse) — the same scratch rotation the incumbent already
carries at the join (§7-3 of fgh-100b). So the original has `b` loaded *after*
the search AND the zero in EBX AND case 1 through `st`; our source can have any
two. The third lever — what keeps the zero in EBX with `b` after the search in
a body without the `bl` shim — is the ebx/ebp unknown of §2.4 and was not found.
Not promoted (56 > 11); no body changed.

Temple's named pair (GOOD-tail form + arm A as the merge host) was not run:
§3.1 shows the head is not a count decision, §8 of fgh-100b already measured
every `goto` pairing of the three tail copies, and the arm-A merge needs the
reload form of `b->target` that only a volatile view produced.

## §5 What this round changes for the next one

- **Stop looking for a count.** At the two callee-saved residuals the original
  is on the wrong side of the corpus rule; no reference-count construct
  ("counted but not emitted" or otherwise) can move `ty` above `sq` or the
  edge cursor below `&v[0]`. The lever class is allocation *order* carried
  from a neighbouring decision (Joust's tail groups, Temple's tail grouping,
  JungleCruise's head order), and the fgh-100b instruments read it.
- **The ebx/ebp choice is the shared unknown** of Temple, Raster and
  JungleCruise. The corpus has ~125 clean ebx/ebp pairs that violate the
  rank rule; a targeted study of those (what the ebx web and the ebp web look
  like in the C — parameter vs local, 16-bit width, first-statement position,
  whether one is a hoisted CSE temporary) is the next cheapest experiment and
  the dataset already isolates them (`fit.py --pair ebx,ebp`, then the `wrong`
  list in `failures.py`).
- **Register reuse is not interference-driven** at the du-web level (§2.4 b).
  Any model of the rotation residuals (StepSchoolCar, Raster's homes) should
  start from lifetime-of-symbol allocation, not graph colouring.

## §C Part C — ablating the rule-violating ebx/ebp pairs — `ebxebp.py`, `ablate.py`, `factor.py`, `srcorder.py`, `quad.py`

### C.1 The violating set and its mapping to C locals (`ebxebp.py`, `ebxebp_cases.json`)

Of the 500 clean, non-byte ebx/ebp pairs, **98 violate** the fitted rule
(`W2u(ebp web) > W2u(ebx web)`; 93 under the plain use count), 115 are ties
and 287 conform. Webs were mapped to source locals automatically: the `/Fa`
listing of the production TU is aligned instruction-for-instruction with the
original (exact bodies: same count and mnemonic sequence; 88 of the 98 align,
the rest are switch-table functions the listing splits differently), each
def/use of a web is taken to its `; Line N`, and the identifiers on those
lines are voted with a `_name$` stack-slot symbol on a def or store weighing 4,
the assignment target of a def's line 2, any other identifier 1, the two webs
of a pair voted *contrastively* so a shared name (`def`, `sq`) cannot win both.
84 pairs map to two distinct names; 70 are pairs of locals/parameters (the
rest name a global or a constant web), and every one of the 70 was confirmed
by the compile in C.2 (the recorded ebx/ebp defs are found in the compiled
body at the same indices).

### C.2 Single-variable interventions (`ablate.py`, `ablate_results.{json,md}`)

For each of the **70 cases** the TU is copied to `/tmp/fgh100c_abl/<fn>/`,
one semantics-preserving edit is applied to the function body, the variant is
compiled with `/Fa`, and the two locals are re-located in the compiled body by
the overlap of their ref source lines (mapped through a line diff) with the
base compile's, so a variant is read even when the code changes. 315 variants:

| intervention | applied | inert (byte-identical) | same code-changed, pair unchanged | pair member left ebx/ebp | **flip** | compile fail |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| rename A / rename B | 140 | 140 | 0 | 0 | 0 | 0 |
| declaration order swap (own lines or inside one comma list) | 39 | 30 | 4 | 0 | 0 | 5 |
| first-assignment swap (adjacent, independent) | 3 | 2 | 1 | 0 | 0 | 0 |
| move a first assignment one statement up/down (independent) | 29 | 15 | 10 | 4 | 0 | 0 |
| one-reference temp `{ T x_t = rhs; x = x_t; }` | 66 | 63 | 3 | 0 | 0 | 0 |
| initialiser split `T x = e;` → `T x; … x = e;` / fold | 38 | 26 | 5 | 2 | 0 | 5 |
| **total** | **315** | **276** | **23** | **6** | **0** | **10** |

**No single-variable source intervention flips ebx/ebp in any of the 70
cases.** 88% are byte-identical objects (names, declaration order, temps,
initialiser placement and adjacent statement order are inert, as fgh-100b
found on the WIPs); the 23 that change code (a moved load or a different
spill slot order) keep the same two registers on the same two locals; the 6
that move a web move it *out* of the callee-saved class (a spill/scratch
decision), never across the ebx/ebp line. The block-shape interventions
(if/else↔goto, case slot, merge host) were not automatable across 70 bodies
and were not run generically; §D.3 runs them on JungleCruise.

### C.3 Looking for the common factor (`factor.py`, `srcorder.py`, `quad.py`)

Every hypothesis the brief names was computed on the original bytes for all
500 pairs (`factor_rows.json`):

| factor | violating (98) | conforming (287) | reading |
| --- | --- | --- | --- |
| ebx web defined first in layout (`a_first`) | 52 | 233 | ordinary 74% correlate, no separation |
| first block where both are live, who enters first | the metric is degenerate as computed (498/500 identical) | | not testable at web level |
| register the first def is computed from (`a_src`/`b_src`) | `imm`/`eax`/`mem` in the same proportions | | no |
| defined by a call return (`mov r,eax` after `call`) | 9 / 6 | 43 / 19 | no |
| losing (ebp) web loop-carried (`b_lc`) | 64 | 89 | the one strong skew (see below) |
| both live at a join | 70 | 109 | correlated with the above |
| ebp web's range contains the ebx web's (`b_nested`) | 34 | 21 | the "short web takes the better register" shape of §2.4 |
| kinds | `const`→ebx over `load`→ebp 31:12; **`arith`/`lea`/`local`-defined webs sit in ebp 37:6** | | computed temporaries prefer EBP |
| source order (aligned listing): first def / first use / declaration line | 0.52 / 0.48 / 0.52 accuracy | | none |
| parameter order (param/param pairs) | the **later** parameter takes ebx 22:9 (16:4 on one-web-per-register functions) | | small n, but the wrong way for a symbol-table walk |

As a *predictor* on the ebx/ebp cell (`fit.py --pair ebx,ebp`): `loop_carried`
is right 0.902 where exactly one web is loop-carried (132 rows), `deg_cs` 0.887
on 311, `uses` 0.752 on 375. On the 165 cleanest pairs — functions where ebx
and ebp each hold exactly one web — the use count is 70 : 43, i.e. **the
fourth-register choice is close to a coin flip for whole-function variables
with any asm-level or source-order feature** we can compute. The four-web
permutation test (`quad.py`, 66 functions with one web per callee-saved
register) shows the same thing from the other side: by use count esi/ebp is
right 0.98, esi/edi 0.89, esi/ebx 0.84, edi/ebp 0.87 but **edi/ebx 0.64 and
ebx/ebp 0.69** — the rank is right coarsely and wrong between *adjacent*
ranks, so the residual is a secondary key we do not have, not a different
primary one.

Two corpus facts bound what that key can be. (i) `ebxstats.py`: 194 functions
use exactly {esi, edi, ebx} and only 17 use {esi, edi, ebp} — EBX is the
third register — yet in those 17 EBP is taken with EBX *free*
(`SpaceTower_CountSeated`: loop-1 RMW temp → edi, loop-2 RMW temp → ebp;
`MoveIcons`: 16-bit expression temp → bp with dx/dy in esi/edi;
`__DEBUG_TAG`: `tag` → ebp with `n` in esi and edi constrained by `scasb`).
The pick is therefore *not* "first free register in a fixed order"; each
web carries a preference, and computed temporaries (C.3 kinds row) lean to
EBP. (ii) C.2: that preference is not carried by anything a single-variable
source edit touches. The ≥90% rule asked for was **not found**; the honest
result is that the ebx/ebp key is an IR-level property (which webs the
optimizer created and in what order, or a per-web preference set when the
temp is formed) that the asm and the C text both under-determine.

## §D Part D — the three WIPs

### D.1 TempleSlide_Update (`ty`/`sq`)

Factor readings (original / incumbent, `wipcmp.py`): `sq` ebp u8 deg_cs 5→7
live 70 calls 2 loop-carried, kind `lea`; `ty` ebx u4 deg_cs 4 live 22 calls 0,
kind `arith` (computed). Every C.3 factor points the *wrong* way for the
original — `sq` is loop-carried and has the higher degree and use count, and
`ty` is the computed temporary that C.3 says leans to EBP — so the original's
`ty`→ebx is a violation under the new factors as under the old rule. C.2 says
no declaration/temp/order construct on `ty` or `sq` will move it; what moved it
in fgh-100b §8 is the tail grouping (the GOOD family), i.e. block structure,
consistent with the brief's block-group hypothesis but not reducible to a
per-web construct. Not re-run this round (every `goto` pairing of the three
tail copies is already measured in fgh-100b §8; no new lever to pair it with).

### D.2 Raster_SubmitPoly

Readings: edge cursor ebx u5 (all @d1) live 171 loop-carried, kind `lea`;
`&job->v[0]` esi u4 kind `lea`; `job` edi param u7. The loop-carried factor
(0.90 on the cell) puts the cursor in ebx over a non-loop-carried web, which
is what the original does — the cursor/`&v[0]` half of the 3-cycle is
*consistent* with the strongest ebx/ebp predictor, and the violation is
the esi/edi half (`job` u7 in edi behind `&v[0]` u4 in esi), which is an
adjacent-rank (esi/edi) swap of the C.3 kind, not an ebx/ebp one. No construct to
try: the incumbent already has the cursor as the loop-carried `lea` web; the
frame-home order follows the same unknown.

### D.3 JungleCruise_Tick — the 56 basin (`jc_v6.py`, probes/JungleCruise_Tick/v6/)

| form | mism. | bytes | pair (zero, `inst`) | `b` load |
| --- | ---: | --- | --- | --- |
| 84 basin (de-shim, `b` after the search) | 84 | 1110 | ebp, ebx (wrong) | 122 (right) |
| 56 basin (`b` before the search) | 56 | 1111 | **ebx, ebp** | 112 (wrong) |
| `pb = &inst->bloke` before, `b = *pb` after (one-reference address temp) | 84 | 1110 | wrong | 122 |
| `pb` first in the head; `ic = inst` copy before, `b = ic->bloke` after; `b` folded into `if ((b = inst->bloke)->action == 0)`; `pb` + fold | 84 | 1110 | wrong | 122 |
| `st, key, b, next, loop` | 58 | 1110 | right | before |
| **`st, key, b, loop, next`** (`b` before, `next` after) | **87** | 1110 | **wrong** | before |
| 56 basin + `key.w == st->pos.w` / `for` search / both | 56 | 1111 | right | 112 |
| 56 basin + `b == bl[i]` in the seat loop | 57 | 1111 | right | 112 |

Two results. **The pair follows `inst`'s liveness across the search, not
`b`'s position**: with both `next = inst->next` and `b = inst->bloke` before
the search `inst` is dead across it and the zero takes EBX (56/58); with
either after it `inst` is live across the search and takes EBX (84/87). The
one-reference temps and folds are inert exactly as in C.2 (the address temp,
the copy and the fold all compile to the 84 basin byte for byte). The
original loads `b` *after* the search (`mov esi,[ebp+8]` at 122, `inst` live
across it) *and* has the zero in EBX — so the original's source has a third
property that offsets `inst`'s liveness which none of the forms here has.
The scratch-rank levers on the 56 basin (compare operand order, `for` form,
seat-compare order) are inert on the head rotation (`st` in eax not ecx,
`next`/`key` in ecx/cx not edx/ax); the seat-loop swap costs one. **Not
closed**; incumbent (11) unchanged; no body edited.

## Commands

```
cd .worktrees/fgh-100c/scratchpad/fgh100c
$PY ebxebp.py --show             # violating ebx/ebp pairs mapped to C locals -> ebxebp_cases.json
$PY ablate.py --workers 6        # single-variable interventions -> ablate_results.{json,md}  (/tmp/fgh100c_abl/)
$PY factor.py                    # structural factors for all clean ebx/ebp pairs -> factor_rows.json
$PY srcorder.py --subset single  # source-order features (listing-aligned)
$PY quad.py                      # one-web-per-register functions: pairwise rank accuracy
$PY probe.py JungleCruise_Tick jc_v6.py 4   # the §D.3 sweep
$PY lib.py                       # exact-corpus cache (/tmp/fgh100c_corpus.pkl)
$PY fa_all.py --workers 6        # /Fa listings of every TU -> /tmp/fgh100c_fa/  (not needed by the fit)
$PY dataset.py                   # -> /tmp/fgh100c_pairs.jsonl (25 s), preference histograms
$PY fit.py [--one-per-reg] [--pair ebx,ebp] [--loop-head] [--first deg_cs,uses]
$PY failures.py refs [filters]   # what the wrong rows look like
$PY disagree.py refs W2u         # where two comparators disagree, who is right
$PY logit.py                     # pure-python logistic fit on feature differences
$PY policy.py --rank W           # whole-function fixed vs round-robin simulation
$PY fnload.py 0xVA [--vv]        # webs of one original body
$PY wipcmp.py <Name> [--tu f.c] [--all] [--range a-b]   # incumbent vs original web features
$PY probe.py JungleCruise_Tick jc_v5.py 4               # the §4 sweep
```
