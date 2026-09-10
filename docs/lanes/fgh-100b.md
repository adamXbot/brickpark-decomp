# FGH-100b — four non-spelling methods on the eight remaining WIPs

Branch: `cursor/fgh-100b`  
Worktree: `.worktrees/fgh-100b`  
Base: `cursor/fgh-100` @ `7d8847bd` (= `codex/fgh-integration` @ `804ec161` + two docs commits)  
Object prefix: `/tmp/fgh100b_`  
Scratch (untracked): `scratchpad/fgh100b/`  
Environment: `PY=/Users/systemadmin/.venvs/legoland/bin/python`,
`LEGOLAND_CL=.../alphateam/tools/wibo-msvc/cl`, `PYTHONPATH=/private/tmp/legoland-fgh-deps`
(Unicorn 2.1.4). Unicorn CPU init is blocked inside the Cursor sandbox; every
execution command below was run with the `all` permission, as the validation
README anticipates.

## Checkpoint

**Fifth session (§8): still 42/48.** Two further "counted but not emitted"
classes tested on the rank/interference residuals: LICM-hoisted loop
references (negative — the count is taken after hoisting; byte-identical
objects, unchanged colourings on Raster and JungleCruise) and the Joust
switch-slot sweep (Temple: empty-body slots inert, but tail-group membership
*does* flip the loop head to the original's family at a structural price;
JungleCruise: inert). Branch pushed with docs and instruments.

**Fourth session (§7): still 42/48.** The instrument-first method was applied
to all six remaining WIPs (Mesh, SpaceTower, JungleCruise, TempleSlide,
Raster, StepSchoolCar — ~1,000 hand-derived variants plus 2 rescored mutant
frontiers). No function closed; no body was changed. What the round produced
is, per target, the **intermediate decision named and instrumented**, the set
of levers that *do* move it (all of them at a code price) and the set that
do not — see §7. Nothing pushed this session (nothing closed).

**Third session (§6): 40 → 42/48.** `Joust_Update` (703/703, 2258/2258) and
`LFEntrance_Activate` (222/222, 673/673) audit `[OK]` with zero relocation
`MISMATCH`, warning-free `/W3` and 1200/1200 differential execution; commits
`053b707b` and `a812ec84`, both pushed to `cursor/fgh-100b`.
`validation/fgh/check.py` → `assigned_exact_gate: 42`, `assigned_open: 6`,
`existing_normalized_regressions: 0` (`/tmp/fgh100b_after2.json`). Both
closes were a **pair** of levers that are each inert alone — see §6.

Sessions one and two: started and ended at **40/48** assigned exact under `validation/fgh/check.py`
(`assigned_exact_gate: 40`, `assigned_open: 8`, zero normalized regressions;
`/tmp/fgh100b_baseline.json` before, `/tmp/fgh100b_after.json` after). **No
production WIP body was promoted.** Two WIP bodies were replaced by strictly
better, execution-verified ones found by the mechanical search (StepSchoolCar
83 → 72, Raster_SubmitPoly 142 → 108 with the original's 743 bytes); their
files still audit `PASS` with unchanged `[OK]` counts (29, 3), zero relocation
`MISMATCH`, warning-free `/W3`. **Second session (§5): Joust_Update went
1 → 0 normalized mismatches and 2262 → 2261 bytes** (index 673 closed by a
nested busy-arm form; only case 0x16's tail-copy target, +3 bytes, remains —
still WIP because the byte gate is the audit). The other five residuals are
unchanged:

| Function | Norm diffs | Bytes ours/orig | Exec test | Clean-room best | Mutation-search best |
| --- | ---: | --- | --- | ---: | ---: |
| Joust_Update | **1 → 0 → CLOSED** | **2258**/2258 | 1200/1200 pass | 14 rotation-phase probes fold to the incumbent object | (0, 3) §5 → (0, 0) §6 |
| Coaster3D_BuildTrackMesh | 6 | 442/441 | 600/600 pass | 123 (inlined-helper family) | (6, 1) unchanged |
| SpaceTower_Activate | 10 | 698/698 | 600/600 pass | 167 (no-`tile` variable) | (10, 0) unchanged |
| LFEntrance_Activate | **10 → CLOSED** | **673**/673 | 1200/1200 pass | 7 inlined-helper heads: 10 or 190–206 | (10, 1) → (0, 0) §6 |
| JungleCruise_Tick | 11 | 1111/1114 | 600/600 pass | 57 (de-shimmed case-0 join) | (11, 3) unchanged |
| TempleSlide_Update | 18 | 1122/1122 | 600/600 pass | — | (18, 0) unchanged |
| StepSchoolCar | **83 → 72** | 1147/1147 | 600/600 pass | 5 asymmetric projection stores: 114–333 | **(72, 0) committed** |
| Raster_SubmitPoly | **142 → 108** | **743/743** | 600/600 pass | — | **(108, 0) committed** |

What this pass establishes that the three spelling passes could not:

1. **None of the eight is a reconstruction error.** All eight incumbent
   bodies are behaviourally identical to the original bytes on randomized and
   boundary inputs under a stubbed-callee differential harness (§1). The
   residuals are pure code generation.
2. **The flag/pragma regime is settled at all eight sites** (§4b): `/G3 /G4
   /G5 /GB`, `/Ox`, `/Ob1`, `/Gf`, `/Gs`, `/Op` (integer sites) and per-function
   `#pragma optimize("t"|"a"|"w"|"p")` are byte-identical to `/O2 /Gy /Gd`;
   `/G6`, `/Oa`, `/Ow`, `("s")`, `("g",off)`, `("y",off)` are far worse everywhere.
3. **VC6 `/O2` is not name- or declaration-order-sensitive at these sites**
   (§4a): ~200 single-step renames and ~180 declaration permutations produced
   byte-identical objects every time. Statement order, `volatile` reads,
   split declaration/initialisation and `<<8`↔`*256` are the only textual
   operators that change the object at all.
4. **The incumbents' shims are load-bearing, not path-dependence** (§3):
   every clean-room re-derivation lands in a far basin (57–210) and the
   naive derivation reveals *which* original allocation feature the shims are
   standing in for (recorded per function below).
5. **Two exact analogs of the Joust cross-jump exist in the tree** (§2):
   `OpenMovie` and `ScanBlokeSurroundings` both carry a `cmp / jmp → jcc`
   merge, and both come from two *full source copies* of a tail ≥ 5
   instructions. Duplicating only `f20 = 1` in Joust does not reproduce it and
   flips the edi/ebp pair (10 mismatches).

## §1 Differential execution — `scratchpad/fgh100b/diffexec.py`, `configs.py`

A generic harness on top of `validation/fgh/emulate.py`: the original PE image
and the freshly compiled COFF body run on identical fabricated heap/global
memory. Every callee is stubbed **at its original address** (the COFF's
annotated externs relocate to the same addresses; in-TU callees such as
`LFRun_TickAll`, `LFQueue_*`, `TempleSlide_TakeLane/ReleaseLane`,
`JungleCruise_UpdateRiverAnim` are mapped by marker VA and stubbed identically
on both sides). Stubs log the callee and its arguments (byte-typed arguments
masked; pointers into *our* frame replaced by the pointed-to bytes, so frame
layout differences cannot cause false divergences), return deterministic
values, and apply configured side effects (`GetTileDimensions`, `ClipPoly`,
`ScreenToMapRef2`, `AdjustOffsetForViewMode`, `sprintf`, ...). `__ftol`
executes the real `0x458930`; `rdtsc` sites are hooked to a deterministic
counter; float returns (`GetUnitDepth`) go through an `fld` trampoline;
struct returns (`GetScreenCoordsForObject`) set eax:edx.

Compared per case: ordered call log with arguments, multiset of non-stack
writes, final heap+global image, return value, callee-saved registers, stack
balance, x87 control/status/tag. Indirect callees (hooks, shader table) are
planted at fake addresses and stubbed the same way.

| Function | Cases | Result | Coverage (every callee reached) | Negative controls detected |
| --- | ---: | --- | --- | --- |
| SpaceTower_Activate | 300 + 600 | pass | all 13 callees, cases 0–9 | `(base[1]+1)<<8`→`+2` (4/100), `car = seat>>1`→`>>2` (detected) |
| LFEntrance_Activate | 400 + 600 | pass | all 12 callees, cases 0–11 | case-3 `+0x80`→`+0x81` (9/100) |
| Coaster3D_BuildTrackMesh | 300 + 600 | pass | both hooks, MakeRotation/Transform, MatMul, TransformVerts, real `__ftol`, 4 rounding modes | `e1`→`e0` in the shade (53/60) |
| StepSchoolCar | 400 + 600 | pass | all 22 callees incl. both manoeuvre pickers, all 5 manoeuvre cases, both `return` shapes | `>>16`→`>>15` in `cur.x` (60/100) |
| Raster_SubmitPoly | 400 + 600 | pass | ClipPoly (cnt 0–7), AddSpanRecord, both shader slots, dy<0 / dy>0 / dy==0, sort ties, 4 rounding modes | `e->y1 = va->y`→`vb->y` (86/100); sort `>`→`>=` (69/100) |
| TempleSlide_Update | 400 + 600 | pass | all 17 callees incl. GetUnitDepth (float), NewBNVPath, HeapFree_w, all 7 cases, all 4 seats | — |
| JungleCruise_Tick | 400 + 600 | pass | all 13 callees, loop 1 launch path, cases 0 (found/not found/remove), 1, 3, 4, 5 | — |
| Joust_Update | 600 + 600 | pass | all 11 callees incl. `rand`, `sprintf` (buffer content compared), UnSource/PlayInstance (source struct compared), all 25 cases, both phase-2 arms | — |

Reports: `scratchpad/fgh100b/exec/<Name>.json` (the second column is a final pass with a fresh seed, 4,800 cases in all; every case of every function agrees). Passing execution does not
promote anything; it removes the "wrong reconstruction" hypothesis that
`JungleCruise_UpdateRiverAnim` made necessary. Two harness bugs found and fixed
on the way are worth recording for the next user of `emulate.py`: in-TU
callees are in their own COMDAT sections (compare `(section, value)`, not
`value`, when aliasing), and a stub that is hooked twice returns twice.

## §2 Analog mining — `scratchpad/fgh100b/mine.py` over 2,715 exact bodies

`lib.py` caches the true-extent disassembly of every `// FUNCTION:` body
(`/tmp/fgh100b_corpus.pkl`, 144,479 instructions). Signatures scanned:

| Signature (target) | Hits | Reading |
| --- | ---: | --- |
| `cmp r8,imm / jmp → jcc` (Joust 673) | 0 | the exact encoding is once-in-binary, as fgh-100 said |
| any `jmp → jcc` with flags carried (Joust) | **2** | `OpenMovie` 0x476460 (`cmp [g_avi_open_count],ebx / jmp → jne` of the LAST `if (g==0) AVIFileExit(); return 0;` copy) and `ScanBlokeSurroundings` 0x450530 (`cmp ecx,eax / mov ecx,[esp+3c] / jmp → jne` of case 4/5's tail from case 1's identical tail). Both sources are **two full textual copies** of the tail; the merged suffix is ≥ 5 instructions and starts at the jcc because the flag-setting instruction differs in form (`cmp mem,reg` vs `mov/test`; `dl` vs `dh`) |
| copy-before-store (`mov Rs,[m]; mov Rcs,Rs; mov [frame],Rs`) (Mesh 27–32) | 0 | confirms the WIP note: once-in-binary |
| three-register `lea rD,[rA+rB]` (LFEntrance 22) | 66 in 42 fns | none with a byte-load temp operand; all are int+int sums |
| same-slot double reload `mov R1,[esp+X]; mov R2,[esp+X]` (SpaceTower 22–23) | 6 in 5 fns | `PositionRouteCars` (a parameter read twice at entry), `LevelKw_REPORT`, `MakeAnimInstance`, `OpenMovie` (three reloads); `Copters_Activate`. The natural no-`tile` SpaceTower spelling (§3) also produces the double reload with **no volatile** — so the double reload is not the hard part of that head |
| `xor eax,eax / mov al,[r] / mov R,[R+d] / mov R,[R+d]` (SpaceTower 24–27) | 0 | — |
| rider-loop head (`mov R,[n]; mov R,[n+8]; lea R,[n+0xc]`) | 27 exact ride callbacks | **in every one the `next` load is the first instruction of the head.** LFEntrance's original is the only ride callback in the binary whose head loads `def->qx` before `r->next`; whatever produced it is structurally unlike the other 27 sources |
| `qx/qy` preamble (`movsx [+0x24] … movsx [+0x25]`) | 8 in 7 fns | `EarthSlide_Tick` (exact) interleaves the two sums (`kx; tx; ky; ty`) — its note is the source of that lever |

## §3 Clean-room re-derivation — `scratchpad/fgh100b/st_clean*.py`, `mesh_v*.py`, `jc_clean.py`

Fresh bodies written from the disassembly only, then scored with the
repository matcher (`probe.py`):

- **SpaceTower_Activate.** Straight derivation (def local, `tile` local,
  `tilex/tx/ty` head, `while`): **210, escapes, first index 0** — VC6 puts
  `def` in EBP with a frame slot, homes `r` in the parameter slot, and turns
  `tile` into `add ebx,0xc`. 12 orderings × `while/for` × three case-3
  spellings: all 207–219. Natural variants that might make `def`
  memory-resident (`elem->data` at every use, `def` assigned inside the loop,
  before/after `TickMachine`, `Pos` aggregate for tx/ty): 207–219. **Removing
  the `tile` variable** (`RIDE_TILE(r)` inline at all six uses): **167,
  no escape, 692/698 B**, and the head now has the original's shape *without
  any volatile*: def spilled to the parameter slot, both def reloads
  (indices 22 and 29), `tile` rematerialised — only `r`↔`tile/tx` have EBX/EBP
  swapped and `tilex` has no home (6 bytes short). So the incumbent's two
  volatile def reads stand in for "no `tile` variable", and the remaining
  head question is what ranks `r` above `tile/tx` for EBX. `tilex` early,
  `ty` before `tx`: 214/186.
- **Coaster3D_BuildTrackMesh.** Loop body moved into a `static __inline`
  helper (element as parameter, as `list[i]` inside, as a one-field struct by
  value, helper owning rot/xf): all **123, 438 B** — the `-0x14` element
  store is gone (inlined-helper parameters are not homed). Build P
  reproduced (42/441) and five record-shape variants around it (two reads of
  `list[i]`, `e` then record, comma, union view): 42 or 121 — exactly the two
  families the WIP note describes. No new family.
- **JungleCruise_Tick.** De-shimming case 0's join (plain `b->tx/b->ty` sums,
  `d` local, no `axis_x`, no `p`): **57, 1109 B, first 110** — the `axis_x`
  aggregate and the pointer store are load-bearing. The `move` function-pointer
  view is required by the file's five-int `CalcMoveLine` extern (an
  extern-type lever, not a shim).
- **LFEntrance_Activate.** The incumbent *is* the natural derivation (it has
  no shims), so §2's rider-head result is the clean-room finding: 27/27 exact
  ride heads load `next` first; the original loads `def->qx` first. The
  required source has a feature none of the other 27 callbacks has.
- Joust, TempleSlide, StepSchoolCar, Raster: not re-derived by hand this
  pass (Joust is 700 instructions of layout that already matches; the other
  three carry shim-heavy bodies whose clean forms the resume-lab already
  measured at 134–322).

## §4 Mechanical mutation search — `scratchpad/fgh100b/mutate.py`

Textual, semantics-preserving operators over the incumbent body: local
renames; declaration permutations/shuffles; split/fold of declaration
initialisers; adjacent-independent-statement swaps and multi-step moves
(call-free, no written-lvalue conflicts); named-temp introduction (`{ int t
= e; lhs = t; }`) and elimination; field-read naming; free `volatile` read
insertion/removal; `if/else`↔`?:`; `while`↔`for` rotation; `x++`↔`x += 1`↔`x =
x + 1`; commutative operand swaps; `<<8`↔`*256`; parenthesisation /
reassociation; `if` negation with arm swap; duplication of a statement into
both arms of an `if/else`; empty `if (v) { }` consumers. Every variant is
compiled as a full TU with `/W3 /O2 /Gy /Gd` (warnings reject), scored by the
repository matcher, and logged (`mutants/<Name>/<tag>/log.json`); a Pareto
frontier over (normalized mismatches, |byte delta|, escapes) is expanded for 10
rounds with 8 parents × 50 children × up to 3 steps, parents chosen among
**distinct compiled outputs**, operator sampling adaptively weighted by each
operator's measured rate of changing the object.

| Function | Variants scored | Distinct objects | Best key (mismatch, \|Δbytes\|, esc) | Better than incumbent? |
| --- | ---: | ---: | --- | --- |
| LFEntrance_Activate | 3,304 | 648 | (10, 1, 0) | no |
| SpaceTower_Activate | 3,105 | 278 | (10, 0, 0) | no |
| Coaster3D_BuildTrackMesh | 2,456 | 165 | (6, 1, 0) | no |
| Joust_Update | 2,891 | 1,254 | (1, 4, 0) | no (no mutant reaches 2258–2261 B either) |
| JungleCruise_Tick | 2,234 | 863 | (11, 3, 0) | no |
| TempleSlide_Update | 2,361 | 911 | (18, 0, 0) | no |
| StepSchoolCar | 2,481 | 930 | **(72, 0, 0)** | **yes: 83 → 72**, 1147/1147, exec 600/600 (`mutants/StepSchoolCar/m2/front_r4_01288.c`) |
| Raster_SubmitPoly | 2,456 | 1,455 | **(108, 0, 0)** | **yes: 142 → 108 and 742 → 743 B (the original's size)**, exec 600/600 (`mutants/Raster_SubmitPoly/m2/front_r6_01933.c`) |

The two improved bodies are NOT promoted (they are not exact), but after a
cosmetic pass that kept only hash-inert edits (`cleanup.py`: `!(!(x))`,
named index temporaries, `0 == cnt`, `if (kp) { }` all measured inert and
removed; `if (sx) { }`, the named vertex-`y` temporaries and `jt = job`
measured load-bearing and kept) they were spliced into `goldrush.c` and
`coaster3d.c` as the new WIP bodies, with notes above the markers. What
moved them, read from the lineage in `log.json`:

- **StepSchoolCar 83 → 72** (`base → swap_adjacent (75, 1146) → negate_if×2 +
  volatile_read (75, 1146) → temp_field + swap_adjacent (74, 1147) →
  volatile_remove + empty_consumer (72, 1147)`): the 3D-person stores
  reordered `sy, depth, sx`; `frame.off.y`'s index through a named `int`
  temporary; a free `if (sx) { }` consumer after `sx -= (tw2 + 1) >> 1`; the
  blocked-ahead test written `!(!(...))`. Indices 19–44 (the projection) are
  unchanged; the gain is in 103–199, the scratch rotation the note calls
  downstream of 19.
- **Raster_SubmitPoly 142 → 108, 743 B** (`base → move_stmt + compound (144,
  746) → volatile_remove + empty_consumer + commute (109, 746) → move_stmt
  (108, 746) → temp_field×3 (108, 744) → move_stmt (108, 744) → swap_adjacent +
  move_stmt + temp_field (108, 743)`): in the downward edge arm `kp->y` and
  `e->y0` are written before `ne++; ecur++` and `kp++` before `e->y1`; the
  vertex `y` reads go through named `int` temporaries; `grad.a[0] = jb->f10`
  through a temporary; `jt = job` without the volatile view; `if (kp) { }`
  after `kp += 1` in the upward arm. The entry block and the EDI/ESI/EBX
  3-cycle are unchanged; the gain is the edge-arm and sort schedule.

Continuation (tag `m3`, 10 rounds each from the improved bodies, 8 from the
no-`tile` SpaceTower derivation): Raster 2,715 variants / 1,240 distinct
objects, no point below (108, 0); StepSchoolCar 2,881 / 977, none below
(72, 0); SpaceTower from the 167 derivation 2,382 / 720, best (149, 3) — a
worse basin than the incumbent's 10. Both improved bodies are local optima of
this operator set.

### §4a Which operators move the object (single-step attribution, LFEntrance)

| Operator | changed / applied | Operator | changed / applied |
| --- | --- | --- | --- |
| swap_adjacent | 68 / 76 | rename | **0 / 99** |
| volatile_read | 71 / 75 | decl_permute | **0 / 80** |
| split_decl_init | 19 / 38 | decl_shuffle | **0 / 98** |
| shift_mul | 7 / 85 | temp_intro | **0 / 118** |
| move_stmt | 6 / 7 | commute | **0 / 28** |
| loop_rotate | 1 / 27 | compound | **0 / 64** |
| temp_field | 2 / 88 | | |

Across SpaceTower, Mesh and Joust as well: `rename` 0/81, 0/24, 0/29;
`decl_permute`/`decl_shuffle` 0/142, 0/28, 0/51; `compound` 0/70, 0/22,
0/30; `commute` 0/22, 6/32, 0/21. Whole-RHS temporaries and named field
reads are inert at LFEntrance and Joust (0/118, 0/28; 0/16) but do change
SpaceTower (21/91, 26/66) — where the RHS contains the incumbent's shimmed
reads — so they are not universally inert. So at these sites VC6's
frame-home and register decisions are **not** a function of local names or
declaration order (the LEVERS.md line-971 regime does not apply here), and
canonicalisation swallows operand order and `x++` spellings. Statement
*position* (`swap_adjacent`, `move_stmt`), `volatile`, `if` negation, and
where a declaration's initialiser lives are what change the object. That is also
why three spelling passes plateaued: the reachable neighbourhood of the
incumbent under spelling changes is a few hundred objects, and it has been
enumerated.

### §4b Flags and pragmas — `scratchpad/fgh100b/flags.py`, `flags.json`

All eight production TUs compiled under 12 flag sets and the WIP body wrapped
in 8 `#pragma optimize` regimes. Byte-identical to `/O2 /Gy /Gd` at every
site: `/G3`, `/G4`, `/G5`, `/GB`, `/Ox`, `/Ob1`, `/Gf`, `/Gs`, `("t")`,
`("p")`; `/Op` and `("a")`/`("ay")` identical at the integer sites and worse
at the two x87 sites (Mesh 65, Raster 210). Much worse everywhere: `/G6`
(44–604), `/Oa` (107–662), `/Ow` (190–497), `("s")` (125–698), `("g",off)`,
`("y",off)`. No flag or pragma moves any residual toward zero.

## Per-target notes (what changed in the understanding; nothing committed to the bodies)

### Joust_Update (1 / 2262↔2258)
The +4 bytes are two facts, both tail-merge choices, both branch-target
issues the normalized score cannot see: index 673 (`jmp → jne` vs our `je`,
+1) and index 493 (case 0x16's `jmp` goes to the ebp-forwarding tail copy at
449 instead of the reload copy at 522, +3). Execution is identical on 600
cases. §2's two analogs say the `jne`-carrying merge comes from **two full
copies of a ≥ 5-instruction tail**; here the shared suffix after the `jne` is
`mov ebp,1` + the `stop` join — one instruction — and duplicating `f20 = 1`
(or a `switch (cycle)` with both arms complete) yields `cmp dl,3 / je set20`
+ fall-through to `run`, with the edi/ebp pair flipped (10). The
fgh-100 hypothesis (a ≥ 5-instruction shared suffix is required for the
merge to carry the jcc) is consistent with both analogs. For 493: the three
tail copies A (449, y forwarded from ebp), B (522, y reloaded — its
predecessors include 0x18 with y in edx) and C (401, reload, other register
assignment) are register-allocation *variants* of one IR tail, so the group a
case joins is decided by which variant its own post-RA tail equals; 0x16
(y in ebp) equalling the reload form in the original means VC6 did not
forward ebp there, while it did for the identical-source 0x1b (→C) and
0x17/0x1a (→A). Fourteen rotation-phase probes in cases 0x15/0x16/0x1b
(`scratchpad/fgh100b/joust_v2.py`: named `x`/`y` temporaries, a named
`Pos*` to the target, a second `Bloke*`, `ty * 256`, the walk written
longhand) all fold to the **identical** object — VC6 canonicalises every
one — while storing `target.y` before `target.x` in case 0x16 alone moves
0x16 onto copy A *and* turns A into a reload copy (218, 2261 B): store order
inside a case body changes tail-group membership; folded temporaries do not.
`volatile` on `tx`/`ty` in one case rewrites the whole function (643–655).

### Coaster3D_BuildTrackMesh (6 / 442↔441)
Exec 300/300 (real `__ftol`, all rounding modes). Corpus: zero copy-before-store
witnesses. Clean-room: helper forms drop the `-0x14` store (123). Build P
(record + `e = cur.elem`) and its neighbours are 42/121, the two known
families. Mutation search: 2,456 variants, 165 distinct objects, none below
(6, 1). Still needed: a non-volatile reason for the element's copy to esi to
precede the record store.

### SpaceTower_Activate (10 / 698↔698)
Exec 300/300. Clean-room result above is the new fact: the no-`tile`
derivation gets the double def reload and the rematerialised `tile` for free
(167/692, no escape); the incumbent's volatile def reads and `base[2]` are
substitutes for that structure. Open: what ranks `r` above `tile/tx` for
EBX, and where the homed `tilex` and the dead 0x18 slot come from in a
no-`tile` source. Mutation search on the incumbent: 3,105 variants, none
below (10, 0).

### LFEntrance_Activate (10 / 672↔673)
Exec 1000/1000. §2: the only ride head in the binary that loads `def->qx`
before `r->next`. 3,304 mutants / 648 distinct objects, none below (10, 1);
no mutant that keeps the prologue emits `movsx edx,[ebx+24h]` at index 14.
Seven `static __inline` head helpers (`lfe_v1.py`: `tx`/`ty` through
`LF_TX(def, tile)`, a `Pos`-returning `LF_Base`, helpers taking `r`, `tx`
before `b`): 10 (byte-identical to the incumbent) when the helper takes
`tile`, 94 for the `Pos` return, 190–206 with a broken prologue when the
helper takes `r`. The source feature that demotes the `next` load is not a
spelling of the current statements and not an inlined helper.

### JungleCruise_Tick (11 / 1111↔1114)
Exec 400/400 including the seat-found / not-found / remove paths of case 0
and the `blokes[-1]` aliasing write. De-shimmed case 0 is 57 (the `axis_x`
aggregate and the pointer store are load-bearing).

### TempleSlide_Update (18), StepSchoolCar (83), Raster_SubmitPoly (142)
Exec 1000/1000 each (TempleSlide with the float-returning `GetUnitDepth`;
StepSchoolCar with both `GetTileDimensions` calls, both manoeuvre pickers, all
five manoeuvre cases and both early returns; Raster with clipped and
unclipped polygons, empty edge lists, both edge directions, sort ties and
both shader slots). The Raster frame is already `0x200` in this build
(the marker's `0x1fc` is stale); the residual is the EDI/ESI/EBX 3-cycle and
the homes/reloads that follow it. Semantics of all three are confirmed
correct; what remains in each is allocation. StepSchoolCar's one dead
projection store was re-probed with the asymmetric shapes the note did not
list (`ssc_v1.py`: x through the address-taken aggregate with read-back and
y a plain local, in both sum orders, with a volatile read-back, and with the
y store killed by an adjacent `target.y = c->ty`): 322/322/333/114 — a plain
`sy` always sinks past the two calls (first index 5) and killing y kills x
too (114, first 8), exactly the two families the note names.

## §5 Second session — the five leads, worked in priority order

### Joust_Update: index 673 CLOSED (1 → 0, 2262 → 2261 B). Commit `e25432e3`.

**What moved it.** Not tail duplication. Both analogs' merges cross a `jcc`
whose *skip labels correspond* (`OpenMovie` 113: `cmp [g],ebx / jmp →
jne L / call / L: pops / ret`), so the merger matches suffixes across
branch tuples as long as the tuples have the same shape. Our flat busy arms
`if (h != 3) goto run; f20 = 1;` are a bare `jne run` when the merger runs
and it folds only the trailing `mov ebp,1`, then the branch simplifier turns
`jne run / jmp mov` into `je mov` + fall-through. The nested form

```c
if (horses.h[1] != 1) { if (horses.h[1] == 3) f20 = 1; else goto run; }
...
if (horses.h[0] != 1) { if (horses.h[0] == 3) f20 = 1; else goto run; } else goto stop;
```

makes the front end emit `cmp 3 / je setN / jmp run / setN: mov ebp,1` on
BOTH arms; the merger sees an identical four-tuple suffix and folds horse 0's
arm into one `jmp` that lands on horse 1's `jne run` (after simplification)
— the original's encoding, byte for byte at 673. Sweep evidence
(`joust_v7.py` 65 variants, `joust_v8.py` 121 variants,
`probes/Joust_Update/v7,v8`): the merge appears only when the horse-1 arm
has *no* trailing `goto stop` in the flat form (`S`/`N6`) and the horse-0
arm has the nested/explicit-skip form (`E1`/`N2`); every other pairing is
1 (the old `je`), 3 (2258 B but compare order 3-before-1 at 670–673), 10
(edi/ebp flip) or 41–73 (layout). Full tail copies (`joust_v3.py`: both
arms, either arm, all four arms, with/without `stop:`) are 80–85: two
`Joust_FadeSample` argument aggregates change the frame. Audit
`Joust_Update ours=703i/2261B orig=703i/2258B mismatch=0`, relocs 0
MISMATCH, `/W3` clean, diff-exec 1200/1200 (fade 141 ×, start-sample 517 ×).

**What remains: case 0x16's `jmp` at 493** goes to walk-tail copy A (449,
`lea edx / ecx=x / eax=ebp`) where the original goes to copy B (522,
`ecx=y / edx=x / lea eax`); normalized diff is target-blind so it scores 0
but costs +3 bytes (rel8 → rel32). Measured this session
(`joust_v9.py`): instruction-identical respellings of 0x16 (`*256`,
`((ty-15)<<8)-0x46`, named `x16/y16`) and of the block before it (0x19's
duplicate body: named timer/seat temporaries) are inert; `y` before `x`
in 0x16, `Pos` aggregate, `case 0x19: goto sit;`, sharing 0x19 with 0x0d,
moving 0x19/0x0d around 0x16 are 41–299. The three tail copies are one IR
tail under three temp-register rotations (A: edx,ecx,eax; B: ecx,edx,eax;
C: edx,eax,ecx — each a cyclic eax→ecx→edx pick order), and which copy a
case joins is which rotation *its own* tail was emitted in; that is carried
allocator state, not a property of the case body (0x16 and 0x1b are
textually identical and land on B and C in the original). Positional sweeps
this session: 0x16 at every one of the 25 slots (`joust_v10.py`: only its
current slot keeps the layout; after 0x18/0x17/0x1b it is 19–20 at 2260 B
with a fourth tail form), `case 0x1a` given its own 0x17-identical body at
every slot as a no-code state advancer (`joust_v11.py`: 3 at 2264 B when
placed before 0x15, otherwise the duplicate fails to merge), and
instruction-identical IR-history respellings of 0x16 (`joust_v12.py`:
`++b->action`, `ty * 0x100`, `-(0xf00+0x46)`, copies of `tx`/`ty` through
block locals — all fold to the same object; recomputing `tx`/`ty` from the
tile re-allocates the whole function). Byte-targeted mutation search from
the new body (`mutants/Joust_Update/m5`, 9 rounds, 3,435 variants, 1,511
distinct objects): no point below (0, 3).

### LFEntrance_Activate (10, unchanged) — the `for` form is the first lever that moves the head

`lfe_v2..v6` (CSE spellings of case 10, all 120 `while`-head orders, named
`qx/qy` locals, two-step sums, inlined helpers with `qx/qy` by value): 10
every time, `next` first every time. **`lfe_v7.py`: with `for (; r; r =
next)` instead of `while (r) { ... r = next; }` the head order in the source
is honoured** (the `while` form emits `next, b, qx, qy, tile` for all 120
orders; the `for` form emits the source order, so `for0_btxyn` puts `next`
LAST, into ebx after def's last use). It still does not reproduce the
original because the original's `next` sits in ecx for six instructions
before its spill and qx therefore takes a separate temp edx (`lea ecx,
[edx+eax]`); in every `for` variant the spill follows the load immediately
and qx coalesces into tx's home. Read against the round-robin pick order
above, the original's head temps are `next=ecx, qx=edx, x=eax, tx=ecx,
y=edx` — a clean cycle in IR order `next, qx, x, tx, y` — and ours are
`qx=ecx(=tx), next=edx, x=eax, y=edx`. So the source has to (a) create
`next`'s temp before `qx`'s and (b) not store it until the `lea`. Best
`for` variants: `for0_txnyb` (22 / shape 4: ebx/esi swapped for def/b) and
`for0_ntbxy` (95: head right up to the spill, but the different head
rotation shifts the temp registers of every case body — the same carried
state seen in Joust). Iterated search from the `for` base with watch on
14/15/22 (`mutants/LFEntrance_Activate/m5for`, 11 rounds, 3,933 variants,
1,704 distinct objects): no point below (10, 1); 14/15/22 wrong at every
frontier point. The corpus' only other early-`qx` heads (`SafariRide_Activate`
27–36, `EarthSlide_Tick` 34–40) load one operand of each sum straight into
the sum's home register — none has LFEntrance's both-operands-in-temps
`lea`, which is what a busy home register (ecx held by `next` until the
store) produces. The source therefore has to hold `next`'s value in a
register across the `qx` load and store it only at `tx`; no plain
assignment order does that in either loop form.

### SpaceTower_Activate (10, unchanged) — the no-`tile` basin does not reach

Iterated search from the no-`tile` derivation with the register-agnostic
`shape` metric (`lib.shape_norm`, `mutate.py --metric shape --watch
25,26,27`; `mutants/SpaceTower_Activate/m4shape`, 13 rounds, 5,300
variants, 2,298 frontier improvements): best (132 mismatch, **84 shape**,
704 B) vs the incumbent's (10, 8, 698). Head 25–27 (`mov al,[ebp]` before
the `base_y` load) is wrong at every frontier point. The no-`tile` structure
is the right skeleton for the def reloads but the search cannot recover the
incumbent's allocation from it; the incumbent stays.

### StepSchoolCar (72) and Raster_SubmitPoly (108) — second generation exhausted

`m3` (10 iterated rounds, 3 steps, seeded from the committed bodies): 2,881 /
2,715 variants, 977 / 1,240 distinct objects, no point below (72, 0) /
(108, 0). Raster's 108 clustered by kind (this session): 17 pure
register-allocation diffs, 16 schedule-only, **75 frame-home permutations**
— the six scalar slots `cnt, m-bound, grad-ptr, ne, vp` sit at `-4, -8,
-0xc, -0x14, -0x18` in the original and `-8, -4, -0x18, -0xc, -0x14` in
ours, with the two compiler temporaries (loop bound `n-1`, `&grad.a[m]`
cursor) interleaved *between* named locals in the original. Declaration
permutations are inert (§4a), so the slot order is not declaration order;
what places a compiler temporary between two named homes is the open
question, and it is the same question as the 3-cycle.

### Coaster3D_BuildTrackMesh (6, unchanged)

The original's 27–32 is `t = list[i]` → `esi = t` (register home, used by
both pushes) and `[ebp-0x14] = t` (a dead aggregate store), with the hooks
load *first*. `mesh_v3.py` (by-value record copy, by-value inlined helper
taking the record, `t0` feeding both `e` and `cur.elem` in both orders,
`cur.elem` read back for the second call, aggregate initialiser): 120–125
— every one loses the early store (it sinks to the escape point before the
second call, or the register copy is forwarded from the temp so the first
push uses the temp). The corpus still has no witness for
copy-then-dead-store; the `volatile` union remains the only spelling that
pins the store before the first call.

### Commits this session

- `e25432e3` Joust_Update: close index 673 (1→0 mismatch, +4→+3 bytes) via
  nested busy-arm form. Diff-exec 1200/1200. Not pushed: no function closed
  under the audit byte gate.

## §6 Third session — the bounded lever list (A–D)

Rules as before; no generic mutation search was rerun. Per target: best
(normalized diffs, byte delta, branch-target status), closed or not, what
moved it, evidence.

### A. Joust_Update — CLOSED (0 diffs, 2258/2258, case 0x16 → copy B). Commit `053b707b`, pushed.

Metric for every probe here: bytes (2258 is the target) plus
`scratchpad/fgh100b/joust_tails.py` (which walk-tail copy each `0x16a/0xf46`
case block jumps to) and `joust_forms.py` (compiles a variant with `/Fa`,
parses the jump table and reports, per case block, the register-pick
signature of its tail and the tail labels it reaches). Results in lever
order:

1. **Explicit binding** (`joust_v13.py`): a `goto walk_1c` at the end of
   0x16 landing before `Joust_Walk` in 0x1c, the same through a shared `wy`
   local so the label sits on the exact suffix, every copy-A/C predecessor
   `goto`-ing its own host, and the mirror (one source tail, eleven `goto`s):
   297 / 297 / 209 / 363 mismatches. VC6 does not let source `goto`s decide
   the pairing: the cross-jumper still runs on the resulting blocks and the
   extra labels only break the merges that were right.
2. **Fallthrough** 0x16 → 0x1c: 207 mismatches, 2255 B, and it re-executes
   0x1c's target stores (semantics change) — rejected on both counts.
3. **Host body position**: 0x1c, 0x0e and 0x0a each swept through every
   slot with 0x16 fixed: no slot better than the base (best 13 / 2264 B);
   most break the layout because the hosts are real (non-merged) blocks.
4. **`default:`** at every position and an empty `default: break;`:
   byte-identical to the base in all nine positions (inert, as the source
   note already said for 8/9/0x11–0x13).
5. **Analogs**: `OpenMovie` / `ScanBlokeSurroundings` have exactly two
   textual tail copies and one merged suffix; neither contains a
   three-copy tail, so there is no "farther copy" witness in the exact
   corpus for this decision.

What closed it came from instrumenting instead of guessing. The per-block
signatures show that the three walk-tail copies are **groups formed before
register allocation** (all members of a group share one form; a group that
contains a seat case — 0x0c or 0x18 — gets the reload form, one that does
not gets the forwarded form), and that a block's group depends on **which
case bodies sit between which in the source, including bodies VC6 later
merges away whole**. In the original the groups are A = {0x0e, 0x14,
0x17/0x1a}, B = {0x0b, 0x16, 0x18, 0x1c}, C = {0x0a, 0x0f, 0x0c, 0x15,
0x1b}; ours had 0x16 in A. The jump table (read from the binary at
0x408504 and from our `/Fa` listing) confirms 0x0f merges into 0x0a and
0x19 into 0x0d in both.

- `joust_v14.py` (diagnostic, spare label 0x1f): a duplicate of any body
  placed right before 0x16 moves 0x16 to C or D, never B; a duplicate of
  0x16's own body placed *before* the 0x19 duplicate comes out **B** —
  proof that the B slot is reachable from that region.
- `joust_v15.py` (2-D: 0x0f-dup slot × 0x19-dup slot, 271 variants): no
  2258. The 0x0f duplicate is not free — anywhere but after 0x0e it stops
  merging into 0x0a.
- `joust_v16.py`: the natural `case 0x0d: case 0x19:` spelling with no
  duplicate is 263 (0x15 ≡ 0x1c and 0x0a ≡ 0x14 then merge whole); moving
  the seat cases or 0x14 from there does not recover.
- **`joust_v17.py` (2-D: 0x19-dup slot × a split 0x1a duplicate of 0x17's
  body, 340 variants): five slot pairs give 0 mismatches at 2258 B with
  `0xf46-blocks -> ['B', 'C']`.** Committed: the 0x19 duplicate directly
  after 0x0d, and `case 0x1a` as its own copy of 0x17's body between 0x16
  and 0x18 (it still merges into 0x17's block; the jump table is
  unchanged). Also 2258: 0x19-dup after 0x0c / 0x10 / 0x16 / 0x18 with the
  same 0x1a slot. The earlier note "a separate 0x1a body costs 23" was
  true only for the slot next to 0x17. Empty `case 8/9/0x11–0x13: break;`
  blocks are inert at every slot (also in v17).

Audit `[OK]` 703i/2258B; `relocs.py` 0 MISMATCH (8 pre-existing
unresolved: `k_manbox`, the jump table, a string literal); `/W3` clean;
`diffexec.py Joust_Update --cases 1200` → 1200/1200.

### B. LFEntrance_Activate — CLOSED (0 diffs, 673/673). Commit `a812ec84`, pushed.

Instrument: `scratchpad/fgh100b/lfe_range.py` (register and live range of
`next` from its `[ebp]` load to its `[esp+0x10]` spill, register of `qx`).
Target: `next@15→ecx, spill@21 (range 6), qx→edx`. `lfe_v8.py` (491
variants): all 120 head orders under `while`, `do/while`, a `goto` loop and
`for (; r; r = next)` (B4); `next` as `void*`, `char*`, `unsigned long`, a
`RiderNode**` and a first-field `*(void**)r` view (B2); four `static
__inline` helpers consuming `r`/`next` (B3); an alias-pointer read (B1).
**No variant put `qx` in edx**: with `n` before `x` in the head `next` does
take ecx for 5 instructions but `qx` then loads into ecx *after* the spill
(92–95); with `x` first the whole web rotates (`def` leaves ebx, 190–213).
The first probe that extends `next`'s ecx range past the qx load is
`c10x_recomp_xnty` in `lfe_v9.py` (177, but `def` in esi) — and the one
that does it with everything else intact is the close below.

Reading the original as an allocation *order*: `next` is allocated first
(ecx), `tx` second (also ecx, because its range starts at the `lea` after
the spill), and the `qx` temp cannot coalesce into `tx`'s register while
`next` holds it, so it goes to edx and the sum is a 3-operand `lea`. In
ours `tx` (with `qx` coalesced) is allocated first. So the lever is
`tx`'s rank, not the scheduler. `lfe_v9.py`: **case 10 spelled as
`((def->qx + tile->b.x) << 8) + 0x80`** — VC6 CSEs it with the head's sum
(no code change) but `tx` becomes a one-reference local — **plus `next =
r->next` before `tx` in the head** gives 0 mismatches at 673/673.
Either alone is 10 (the earlier "rank probe" had measured the case-10
change with the old head order). Rejected in the same file: `ty` given the
matching treatment (207, escapes to ebp), `tx` as `long`/`unsigned`, a
named `qx`, a second `def` pointer, a folded second use of `next`.

Audit `[OK]` 222i/673B; `relocs.py` 0 MISMATCH / 0 unresolved; `/W3`
clean; `diffexec.py LFEntrance_Activate --cases 1200` → 1200/1200.

### C. Raster_SubmitPoly — not closed (108, 743/743, unchanged)

Instrument: `scratchpad/fgh100b/homes.py` (splits mismatches into
frame-offset-only, register-only and other, and infers the original→ours
home map from the offset-only lines). Base: 27 offset-only, 17
register-only, 64 mixed; map `-4↔-8` (cnt / gradient-loop bound), `-0xc→
-0x18` (gradient cursor), `-0x14→-0xc` (ne), `-0x18→-0x14` (vp), plus
`-0x1c/-0x24/-0x38` reuse. The two temporaries between named homes are the
strength-reduced gradient-loop bound (`n-1`, at `-8`) and the `&grad.a[m]`
cursor (at `-0xc`); the `-0x10` slot is the `fistp` scratch reused as the
ring cursor in both.

`raster_v1.py` (144 variants): all 120 entry-block statement orders,
`ne`/`i`/`cnt`/`vp`/`kp` initialised in their declarations, `ne`/`i`/`m`/
`grad_count` moved into block scopes, `ne = 0` / `i = 0` / `kp = keys` /
`first.k` moved to the earliest and latest valid points, six declaration
permutations incl. aggregates first and `cnt` last, the gradient bound as a
named local and as `n` directly. **The home map is identical in every
variant that keeps the code (108–111)**; the variants that change it
(`ne_late` 239, `kp_late` 233, `i_early` 242, `k_late` 164) do so by
changing the code. So under `/O2` the home order here is not
first-definition order, declaration order or block scope — it follows the
web ranking that also produces the EDI/ESI/EBX 3-cycle, which the source
note already ties to the parameter's weight against the edge loop.
`raster_v2.py`: LFEntrance-style "counted but deleted" parameter references
inside the edge loop (`job - job`, `job != job`, `(int)job * 0`, `job->v -
job->v`, `job->area == job->area`, `job->kind & 0`, `n - n`): all
byte-identical to the base — the front end folds these before anything
counts them; the conditional forms (`job ? p[0] : p[0]`) generate code
(187–236). The reference that VC6 deletes *after* counting must be a real
memory expression that CSE removes, as in LFEntrance; none of the
edge-loop values is job-derived with no intervening store or call, so no
such expression was found.

### D. StepSchoolCar — not closed (72, 1147/1147, unchanged)

`homes.py`: **0** offset-only, 31 register-only, 41 mixed. Clusters: the
entry (19–30: `lea eax,[ebx+ebp]; imul` scheduled ahead of the copy in the
original, and the `[esi+0x10]` / `[esi+0x34]` copies homed at `0x24`/`0x48`
vs our `0x40`/`0x2c` — coupled to the registers, not offset-only), the
sum block 83–94 (schedule), the call-sequence block 103–164 (a pure
eax/ecx/edx rotation, the largest cluster, ~45 lines), and the ecx/edx swap
at 237–259. `ssc_v1.py`: `flag`/`wx`/`wy`/`tw` first-definition moves and
declaration inits, seven declaration permutations incl. the `frame`
aggregate last: nothing below 72; moving `flag = 0` away from its slot is
109–336. The entry statement order was already the unique best of 720 (see
the source note). The rotation cluster at 103–164 is the same kind of
carried allocator state as Joust's tail groups; its lever would be a
layout-neutral block placed before it, and this function has no
merged-away duplicate to move.

### Commits this session

- `053b707b` joust2: close Joust_Update (703/703, 2258/2258, audit OK). Pushed.
- `a812ec84` lfentrance: close LFEntrance_Activate (222/222, 673/673, audit OK). Pushed.
- this docs commit.

### Transferable lesson

Both closes were **two individually-inert levers applied together**, and
both were found by first building an instrument that reads the compiler's
intermediate decision (tail-group membership; `next`'s register and range)
rather than the final mismatch count. For the six that remain, the single-
lever sweeps in §4a/§5 are exhausted; pairwise sweeps of the levers each
target's note lists as "inert" are the next cheapest experiment, scored by
the intermediate decision rather than by the diff.

## §7 Fourth session — instrument first, then levers, then pairs (42/48 unchanged)

Method, applied in the same order to each target: name the intermediate
compiler decision behind the residual, build/reuse an instrument that reads it
from the compiled listing, search for levers that move the instrument (not the
diff count), then pair levers. Generic instrument added this round:
`scratchpad/fgh100b/win.py <dir|sbs>... --r a-b [--r c-d] [--orig]` prints
our side of the listing over the given index windows, abbreviated, for every
variant in a probe directory next to its mismatch/byte count, so a window of
the object is read as a value rather than the diff count. `rescore.py <dir>`
re-scores a mutant frontier keeping listings so `win.py` can read them.

### 1. Coaster3D_BuildTrackMesh — not closed (6, 442/441)

- **Decision:** which web owns the first `push` in the per-element window
  (original: element copied to ESI *before* the store, ESI pushed; ours: the
  store first, then the copy, EAX pushed) — copy-before-store vs scratch.
- **Instrument:** `mesh_win.py` — classifies the element window's
  instructions into loads (H hooks / E element / A address / T temp), store
  S, copy C, reload R, push P and prints one string per variant
  (original `HEACST esi`).
- **Levers measured** (`mesh_v4.py` 38 rank levers incl. one-reference
  locals, `hooks`/`list` first-read order, `&dir` placement, `pos`
  accumulation shapes, cursor/pointer forms; `mesh_v5.py` 12 second
  definitions of the element temp; `mesh_v6.py` 9 address-taken/aggregate
  forms of `e`; `mesh_v7.py` 15 forms designed to give two loads IR-level CSE
  does not merge; 146 rescored `m1` mutants read through the instrument):
  **the window string is identical in every variant that keeps the loop
  body's code.** The only strings that differ come from variants that lose
  the `-0x14` store (the inlined-helper family, 123+). The window is rigid
  over ~220 objects; no inert lever moved the instrument, so there was no pair
  to try.

### 2. SpaceTower_Activate — not closed (10, 698/698)

- **Decision:** head 25–27 order — `mov al,[ebp]` (the second `def` reload's
  byte read) must precede the `base_y` load.
- **Instrument:** `win.py --r 22-36` on the head, read as the scheduler's
  order of the three loads.
- **Levers** (`st_v1.py`: all dependency-valid orders of the seven head
  statements plus 7 hybrids that transplant the no-`tile` derivation's head
  into the incumbent; `st_v2.py`: every volatile/plain combination of the two
  `def` reads and a volatile-typed `def`; `st_v3.py`: 14 non-volatile head
  arrangements with `by` before the `tilex` store): the head order is a
  **cycle, not a tie**: the second `def` reload hoists above `base_y` only
  when the `tile->b.y` byte cannot hoist, and it cannot hoist only when its
  register is already taken — by the reload. Every variant lands on one side
  of that cycle (10) or breaks the double reload (19–210). The hybrid seeds
  score 10–167 and none reorders 25–27.

### 3. JungleCruise_Tick — not closed (11, 1111/1114)

- **Decision:** case-0 join allocation — original `st` (queue pointer) in
  ECX and `seat` in EAX, i.e. `seat` outranks `st`; ours the reverse.
- **Instrument:** `win.py --r 111-113 --r 122-127 --r 152-154` (dispatch,
  join, tail) read as the register pair.
- **Levers** (`jc_v1.py`: folded views of `st`'s case-1 members — `rd`,
  `cnt`, `q` locals — to lower `st`'s weighted count; `jc_v2.py`: the
  LFEntrance recipe literally — one-reference locals for the join's sums
  `sx`/`sy`/`-seat`, an earlier read of `inst->bloke`, `seat` through a
  pointer, 12 probes): the pair never flips without a code change; every
  variant that flips it does so by adding an instruction (57–330). The
  de-shimmed base confirms the rank scalar (57) and no inert lever touched it.

### 4. TempleSlide_Update — not closed (18, 1122/1122)

- **Decision:** the loop-head allocation of `{ty, sq}` over `{ebx, ebp}`
  (GOOD = original: `b→esi tx→edi ty→ebx sq→ebp`; FLIPPED: `sq→ebx
  ty→ebp`). The incumbent buys GOOD with the world reads placed *above* the
  `GetScreenCoordsForObject` call (an interference edge), which costs 116–120
  (the two loads must be below the call). The honest reads-below base is 327
  (FLIPPED, whole extent 1200 B).
- **Instrument:** `win.py --r 16-19 --r 23-27 --r 116-120` on the
  reads-below base: the family is read off the head, 116–120 off the call.
- **What moves the decision** (`ts_v1.py`, 80 variants over `v1`..`v7`,
  all on the reads-below base):
  - GOOD + 116–120 exact, at a price: `yb` as an `int` local for the tile's
    y byte shared by the head and case 2 (127, but `yb` is spilled to
    `[esp+14]`, frame +4); a real `ty` use in case 2 (`c2_ty_diag`, 247);
    a real `ty` use in case 3 *before* the call (226); removing `sq` from
    the TakeLane call (14), from the case-3 call (14), from case 5's x
    (62) or y (60) read — these four are semantic changes and only calibrate.
  - Two-lever pairs that flip when neither does alone: `ty` used in case 4
    **and** case 5 (153); `ty` used in case 6 **and** `sq` removed from
    case 6 (33); `ty` in cases 1+4+5+6 (239).
  - **Inert in the head (all 321–334):** empty `if (ty) {}` consumers
    anywhere (deleted before ranking); `ty` used in case 1, case 3 after the
    call, case 4, case 5 or case 6 alone; an extra `ty` use inside case 0's
    arms (where `ty` is already live); `sq` removed from case 6 alone; `sq`
    added in case 1 or 4; `unsigned char` byte locals (spill, 325–332); byte
    pointer views; `xb` named (x is already CSE'd in both); definition order
    of `next`/`sq`/`b` (3 orders); the `if (b->state == 0) {}` block guard
    with and without `b`-first; `dir` as four block-scoped webs; `lane` as a
    byte; `dir`/`lane` merged into one web.
- **Reading:** the tie is *not* a plain weighted reference count — a `ty`
  reference in an already-live block does nothing, a reference in case 1
  does nothing, but one in case 2 or before the case-3 call flips it, and
  two "nothing" references (cases 4+5) flip it together. That is the
  signature of an **interference/degree** rule, not a frequency one: what
  counts is the set of short temporaries `ty` becomes live alongside (case 2
  and the pre-call block both reload `def`), not how often it is read. Every
  lever that changes `ty`'s or `sq`'s interference set here changes the
  code, so the pairing step had nothing inert to pair. The sibling exact
  bodies (`Copters_Activate`, `SpinningBarrels_Activate`, `Joust_Update`,
  `LFEntrance_Activate`) all write `b` before the tile pointer and `if
  (b->state == 0) {`; both were measured inert on Temple.

### 5. Raster_SubmitPoly — not closed (108, 743/743)

- **Decision / instrument:** the callee-saved 3-cycle at indices 6/10/16
  (`win.py --r 6-6 --r 10-10 --r 16-16`: original `job→EDI &v[0]→ESI
  ring→EBX`, ours `job→ESI &v[0]→EBX ring→EDI`). The source note already
  measured the rule (reference count, edge-loop ×4, ties to the earlier
  candidate) and that 3+ *parameter* references inside the edge loop give
  the original colouring — every such reference so far generating code.
- **Levers this round** (`raster_v3.py`, 12): the LFEntrance class of
  "counted, then deleted" references — dead-store loads of `jb->f10` /
  `area` / `dy1` / `v` (1×, 3×, summed, spread over both arms), pointer
  copies `jcopy = jb` ×3, the same through `job` and through a non-volatile
  view reload. **All twelve are byte-identical to the base.** So dead
  assignments are removed *before* the count is taken; the only class that
  survives the count (LFEntrance's) is a CSE-able recompute with a real
  consumer, and the edge loop has no `job`-derived value with a consumer and
  no intervening store to host one. `raster_v2.py`'s folded forms (§6-C)
  are the other half of the same finding.

### 6. StepSchoolCar — not closed (72, 1147/1147)

- Not probed this round beyond reading the instrument: the entry cluster
  19–30 is the sum-first projection (`lea eax,[ebx+ebp]` with both operands
  live, then `mov ebp,eax`) versus our diff-first in-place `add`, and the
  source note's DSE analysis (one dead store of the projected X, none of Y;
  the 8-byte `frame.target` copy keeps both, field stores lose both) is the
  same single fact seen from the frame. The rank levers the brief lists
  (one-reference locals, splitting at the call, aggregates) were measured in
  §6-D and in the source note's 720-order and helper sweeps; nothing new was
  added because no inert lever on this body has ever moved the instrument.

### Commits this session

- this docs commit, plus the instruments and probe scripts
  (`win.py`, `mesh_win.py`, `rescore.py`, `mesh_v4..v7.py`, `st_v1..v3.py`,
  `jc_v1..v2.py`, `ts_v1.py`, `raster_v3.py`). Nothing pushed (no close).

### Transferable lesson (added)

The LFEntrance "counted but deleted" reference is specifically a **CSE'd
recompute with a real consumer**; dead assignments and front-end-foldable
expressions never reach the counter. And the Temple tie shows a second rank
rule beside the reference count: an **interference-set** rule, where a use
that puts a variable alongside new short temporaries moves it and a use in
an already-live block does not.

## §8 Fifth session — two more "counted but not emitted" classes (42/48 unchanged)

Brief: test (1) LICM-hoisted loop-weighted references and their mirror
(sinking), (2) the Joust switch-slot sweep on Temple and JungleCruise, and
(3, only if 1–2 are exhausted) a corpus fit of the tie-break rule. Same
instruments as §7 (`win.py` windows). Nothing closed; no body changed.
Scripts: `scratchpad/fgh100b/raster_v4.py`, `jc_v3.py`, `jc_v4.py`,
`ts_v2.py`; listings under `scratchpad/fgh100b/probes/<Fn>/{v4,v3,v8}/`.

### Class 1 — LICM-hoisted references: negative everywhere it applies

- **Raster_SubmitPoly** (`raster_v4.py`, 20). A recompute of `jb->v` inside
  the gradient loop with the three vertex reads as its consumer is
  **byte-identical** (108/743) — whether `jv` is kept outside as well
  (`g_jv_inside`), removed so the hoisted recompute *is* the web
  (`g_jv_inside_nojv`), or only one vertex goes through `jb->v[0]`
  (`g_jv0_inside`). Also byte-identical: `jvl = jb->v; if (jvl == p) {}` and
  a dead `kind = jb->kind` in the edge loop. **The 6/10/16 colouring does
  not move in any of them** (`win.py --r 6-17`: `job→ESI &v[0]→EBX
  ring→EDI` throughout). By the source note's own thresholds (`&v[0]` flips
  past the parameter at +2 depth-0 references), one hoisted loop reference
  worth ×4 on either web would have shown — so **the reference count is
  taken after LICM/CSE**, at the hoisted depth, and hoisted references carry
  no loop weight. Two or three separate recomputes (`g_jv_x2/x3`) are not
  merged and change the object (236/732), as does any recompute of `inv`
  (219/740, the note's case), `dy1/dy2` hoisted by hand (220/762), `f10`
  stored in the loop (130/747), and the depth-2 bound through the parameter
  (181/747).
- **JungleCruise_Tick** (`jc_v3.py`, 14). `seat = 4` moved inside the
  seat-search loop (hoisted store): 1111 B, mismatch 11 → 10 — but the
  listing shows it is a coincidence of the shift (`xor edx,edx` lands on
  the original's slot while `mov eax,4` moves off it); the join allocation
  is unchanged. The station search through a separate cursor `q` with `st =
  q` after (drops `st`'s two loop-weighted references): byte-identical.
  `bl = st->blokes` inside the loop as a hoisted invariant: needs case 1
  de-shimmed to `st->blokes[0]`, and that de-shim flips the **outer**
  allocation (`st`→EBX, 83–84 mismatches) whatever the definition site
  (loop-hoisted, first statement, after `seat = 4`, with `seat = 4` in the
  loop). The residual at 126/132/207/224 is exactly this: the original's
  `lea edi,[st+18h]` sits inside case 0 after `seat = 4; i = 0;` and case 1
  reads `[ecx+18h]` through `st`; the incumbent's `bl[0]` in case 1 is what
  holds the outer allocation and costs those four indices. Loop-invariant
  folds (`i < 5 + (st->count & 0)`, `seat = 4 - (4 - i)`) are front-end
  folded, byte-identical.
- **StepSchoolCar**: the body has **no loop**, so class 1 (and its mirror)
  has no host; not probed.
- **TempleSlide_Update**: the only loop is the rider `while`, and every
  candidate invariant (`def->base_x`, `def->base_y`) is re-read per
  iteration in the original because calls intervene — nothing to hoist.
  Covered by class 2 instead.
- Mirror (sinking): a definition placed outside a loop with its only
  consumer inside (`m_sink_jv`, Raster) is not sunk — it becomes an extra
  web and changes the object (236/732).

### Class 2 — switch-slot sweep

- **TempleSlide_Update** (`ts_v2.py`, 42, on the reads-below base):
  - Empty `case 1: break;` at each of the 7 slots and an empty `default:
    break;` at each of the 8 slots: **byte-identical** (327/1113). Unlike
    Joust, Temple has no *merged-away body with code*; empty bodies carry no
    slot.
  - The shared CalcMoveLine tail (case 0 arm A, case 2, case 5 — three
    source copies today) rewritten as one host + `goto`s: **grouping moves
    the head allocation.** With case 2 hosting and case 5 `goto`ing it while
    arm A keeps its own copy (`tail_2host_5goto`, and the mirror
    `tail_5host_2goto`, one object), the loop head is the **original's
    GOOD family** (`ebx=[edx+c] … ebp=&[eax+c] … xor ebx / mov bl,[ebp+1]`),
    reads-below and all — 238 mismatches, 1117 B. Every form in which arm A
    is merged with another copy is FLIPPED: the base (327), A hosting with 2
    or 5 `goto`ing (324/1119), A `goto`ing into 2 or 5 (324/1119, 324/1122),
    all three sharing A's label (327/1143 — VC6 moves the labelled join out
    of line). The GOOD form's price is structural: arm A's tail is no longer
    cross-jumped (it passes `b->target` from registers, `mov eax,ebx / mov
    ecx,edi`), whereas the original's arm A tail *is* the merge host
    (reloads `[esi+28h]/[esi+24h]` at 51–52, case 2 enters at 0x4174d7,
    case 5 at 0x4174ee).
  - 2-D: the GOOD tail form × empty-body slots (14): all byte-identical to
    it (238/1117); × the `if (b->state == 0) {}` block guard: 292/1116,
    head still GOOD.
  - Reading: the head decision follows **whether arm A's tail is merged**
    — when it is, `ty`'s shifted value dies at the store and `sq` wins
    EBX; when arm A stands alone, `ty`'s value flows to the `push` and `ty`
    wins. That is the same interference-set rule §7 inferred, now with a
    grouping lever as its witness. The original has arm A merged *and* the
    GOOD head, so its source carries one more `ty`/`sq`-interference
    feature that none of the 122 Temple variants across §7–§8 reproduces
    without code.
- **JungleCruise_Tick** (`jc_v4.py`, 16): empty `case 2` and `default`
  bodies at every slot: byte-identical (11/1111). Case 3/4 tails as one host
  + `goto`: 1035 B (the original's two copies are source copies). Case
  order changes move code (89, 224). Join allocation unchanged in all.

### Class 3 — corpus fit of the tie-break: not started

Classes 1 and 2 were exhausted at ~90 variants; the corpus fit (feature
extraction from C and `/Fa` for 30–50 exact loop-head ties) was not started
within the round's budget. Inputs for it are now concrete: the Temple
evidence table in §7 (which single references flip the head and which do
not) plus §8's grouping witness, i.e. the feature to fit is the
interference set of the two webs at the loop head, not their reference
counts.

### Commits this session

- this docs commit plus `raster_v4.py`, `jc_v3.py`, `jc_v4.py`, `ts_v2.py`;
  `cursor/fgh-100b` pushed at the end of the round as requested.

## What not to repeat

- Joust busy arms: any pairing other than (horse-1 flat-no-goto-stop,
  horse-0 nested-with-else-goto-stop) — 186 pairings measured in v7/v8.
- Joust full tail copies (frame changes), `y`-before-`x` in 0x16, `Pos`
  aggregates, shared/`goto` 0x19.
- LFEntrance `while`-head orders (120, all identical) and the seven
  helper/CSE families in `lfe_v1..v6`.
- Hand-spelling sweeps of the eight incumbents: §4a shows the reachable
  neighbourhood is a few hundred objects and it has been enumerated three
  times over. Renames, declaration order, operand order, `x++` forms and
  whole-RHS temporaries are provably inert at these sites.
- Flag/pragma regimes (§4b): settled at all eight sites.
- Duplicating `f20 = 1` in Joust (10, flips edi/ebp).
- Inlined-helper forms of the BuildTrackMesh loop body (lose the `-0x14` store).
- Straight clean-room SpaceTower with a `tile` local (210, wrong basin).
- (§7) Mesh element-window rank levers that keep the loop body's code — the
  window string is invariant over ~220 objects.
- (§7) SpaceTower head statement orders and volatile/plain `def` read
  combinations — the 25–27 order is a cycle with the `tile->b.y` hoist.
- (§7) Temple reads-below levers that do not change `ty`'s or `sq`'s
  interference set (empty consumers, uses in already-live blocks, byte
  locals, definition order, block guard, `dir`/`lane` web structure).
- (§7) Raster dead-store loads / pointer copies of the parameter in the edge
  loop — removed before the count is taken, byte-identical.

## Next levers, with the evidence that motivates them

1. **SpaceTower:** start from the no-`tile` derivation (167, right head
   shape, no volatile) and look for the natural feature that (a) homes
   `tilex` (b) ranks `r` over `tile/tx` for EBX (c) leaves the 0x18 slot dead
   — e.g. a `Pos` for the two head sums whose `.x` is dead.
2. ~~LFEntrance~~ — closed in §6 (it was `tx`'s allocation rank, not the
   schedule of the `qx` read).
3. ~~Joust~~ — closed in §6 (tail-group membership set by merged-away
   duplicate bodies' slots).
4. **Mesh:** a second address-taken object whose store cannot sink below
   the pushes without a volatile — e.g. the record also passed to the FIRST
   hook.

## Commands

```
cd .worktrees/fgh-100b
PY=/Users/systemadmin/.venvs/legoland/bin/python
export LEGOLAND_CL=/Users/systemadmin/Documents/Development/Github/alphateam/tools/wibo-msvc/cl
export PYTHONPATH=/private/tmp/legoland-fgh-deps
$PY validation/fgh/check.py --output /tmp/fgh100b_baseline.json
cd scratchpad/fgh100b
$PY baseline.py                              # eight incumbents, side-by-side listings
$PY diffexec.py <Name> --cases 400           # differential execution (needs `all` permission)
$PY diffexec.py <Name> --mutate 'OLD=>NEW'   # negative control
$PY mine.py <signature>                      # analog mining over the exact corpus
$PY probe.py <Name> <variants.py>            # score hand-written bodies
$PY mutate.py <Name> --rounds 10 --beam 8 --per-parent 50 --steps 3 --workers 6 --tag m1
$PY flags.py                                 # flag/pragma regimes
```
