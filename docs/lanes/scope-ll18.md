# Scope LL18 — partials wave B: scheduling / cold-block residuals

Branch `scope/LL18` from `origin/main` `b39f261b`. PARTIAL scope over four
documented-floor WIP bodies (brief: `docs/SCOPE_LL18_partials_scheduling.md`).
Object prefix `/tmp/sll18_`. Every measurement below is `tools/audit.py` on
the whole file plus a difflib side-by-side of the compiled body against the
original (`/tmp/sll18_sbs.py`, a `matchfull.py` derivative that prints the
full aligned listing with indices).

## Status

| address | function | file | before | after | audit `[OK]` | marker |
| --- | --- | --- | --- | --- | --- | --- |
| 0x00492db0 | `MusicThread` | musicthread.c | 3161/3161i, 11325/11335B, mismatch 7, first 2917 | unchanged — retired with the mechanism modelled | no | WIP-FUNCTION |
| 0x004724a0 | `DrawPopUpInfo` | popup.c | 962/962i, 3141/3141B, 13 strict, first 590 | unchanged — floor confirmed | no | WIP-FUNCTION |
| 0x0046c7e0 | `LoadScriptEvent` | savechunks2.c | 124/124i, 323/319B, 26 strict, first 95 | unchanged — floor confirmed | no | WIP-FUNCTION |
| 0x00470620 | `CheckWorkerOnMouseStatus` | workers2.c | 184/184i, 672/672B, 82 strict (difflib 3), first 102 | unchanged — floor confirmed | no | WIP-FUNCTION |

Nothing closed; nothing regressed. Per file: `audit.py` PASS with the `[OK]`
count unchanged (musicthread 0, popup 2, savechunks2 1, workers2 9),
`relocs.py` zero MISMATCH, `/W3` clean. No `// FUNCTION:` body, struct or
extern type was touched; only the four WIP bodies' notes changed.

## MusicThread 0x00492db0 — what the residual IS (new)

The 7 are one construct: the original loads `__imp__WaitForSingleObject`
(esi) and `__imp__ResetEvent` (edi) in the message loop's PREHEADER, before
the DBPrintf's `add esp,4`, and reloads both on the notification pump's exit
edge (order edi/esi, together with the `beat` store-back); ours loads them
once at the loop head. Two instructions and ten bytes.

A stand-alone model (`/tmp/sll18_t/t1.c`..`t4.c`: two imports called twice
each, the `while (Get(&m) == 0)` pump with the `memcmp` intrinsic, a `beat`
local) reproduces both behaviours and shows what decides them:

* Loop SYNTAX is fully canonicalised: `for (;;)`, `while (1)`,
  `do .. while (1)`, a `goto` loop, a guarded `do-while` pump, a
  `for (;;) { if (Get()) break; }` pump and a named `hr` all compile to the
  same 96i/310B object. This closes the loop-form axis the earlier notes
  had only partly swept.
* The hoist is an ALLOCATION decision. With `beat` live in ebx across the
  outer loop, `Wait` (two call sites) is defined at the loop head (our
  shape); drop `beat` and it hoists into ebx; give it a third call site and
  it hoists into ebp; remove the intrinsic or the inner loop and it hoists.
  A jump-table `switch` in the pump adds the const-4 web, which VC6 hoists to
  the OUTER preheader when a register is free. So VC6 hoists an `__imp__`
  web to the preheader when it can give it a callee-saved register that is
  free across the whole outer loop, and otherwise leaves the definition at
  the head. In the real body every candidate is taken inside the pump
  (esi/edi by `repe cmpsd`, ebx by the const-4 web, ebp by the promoted
  `beat`), which is exactly the no-hoist case — yet the original hoisted
  into esi/edi and split the webs around the pump. That is an allocation
  order or cost tie the instruction stream does not determine.
* Eighteen real-body spellings are byte-identical to the committed object
  (block splits `if (beat) ;` before the loop, at the head and inside the
  pump; `beat = beat;` twice; dead `if (0) { WaitForSingleObject(..);
  ResetEvent(..); }` before the loop and inside the pump; `if
  (WaitForSingleObject && ResetEvent) ;`; a named `guid` pointer for the
  memcmp; `beat` declared first; the two dllimport declarations moved ahead
  of memcmp's; all five other orders of the compare-chain `switch
  (g_imt_cmd)`). Any other order of the jump-table notify switch is worse
  (16 / 18 — its source order IS its block order) and swapping case 4's arms
  is 33.

Retired at its floor; the note above the marker carries the summary.

## DrawPopUpInfo 0x004724a0

Residual unchanged at 13 (indices 590..603, the argument block of the first
kind-0x306 `PrintCachedText`). The LL10 block split is inert in every
position with and without the volatile shim: `if (w) ;` / `if (halfw) ;`
between the `halfw` definition and the `ty` statement, and `if (halfw) ;`
after it, are byte-identical to their baselines (13 with the shim; 166 /
960i / frame 0x444 for plain `halfw`, which keeps `w/2` in ecx to its push
and spills only for the second call). The split moves forward substitution;
the residual is residency (the original spills at the definition and reloads
for both calls), and there is no callee-saved contention in the block for the
LL14 ranking to act on. Floor stands.

## LoadScriptEvent 0x0046c7e0

Residual unchanged at 26 (three cold blocks permuted). Reading the layout:
the original's cold blocks sit in the order of the branch that reaches each
(terminator from 17, err-after-name 34, err-after-text 52, then `head = 0`
from 87 inside the terminator block) — the head-zero block is the only cold
block not placed adjacent to its single predecessor. Six new forms confirm
the two documented walls: `goto empty` / `goto done` with `empty: head = 0;
done: return head;` textually last is pulled back next to the terminator
(15 with the branch inverted, 14 with the arm order swapped); a `for (;;)`
with the read test at the top and `break` to a post-loop `head = 0; return
head;` (either polarity, or `return head;` alone) is const-folded into the
last error handler's `return 0` tail (117 instructions); the same loop with
`if (prev) prev->next = 0; else head = 0; break;` is the committed object.
Floor stands.

## CheckWorkerOnMouseStatus 0x00470620

Residual unchanged (82 strict = the dead `mov ebp,1` at 102 shifting every
later index, plus `test eax,eax` at 166). Five zero-cost probes are
byte-identical: `if (found) ;` and `if (cell.y) ;` after the out-of-range
arm's store, `if (found) ;` before the `if (g_drag_lock)` join, and
`if (g_drag_lock) ;` / `if (g_icon_clicked) ;` inside the arm. All are folded
before the threading that makes the arm function-ending, so the const-1 web
still dies before the store and the store still folds to an immediate. Floor
stands.

## Levers recorded

* **Loop syntax is not a lever for VC6 loop optimisation.** Six loop
  spellings of the same message loop, including a `goto` loop and a
  for/break pump, are byte-identical (model, above).
* **An `__imp__` load is hoisted to a loop preheader only when the web can
  take a callee-saved register that is free across the whole loop, or when
  its call-site count is high enough to pay for a split; otherwise the CSE'd
  load sits at the loop head.** The choice flips on the presence of ONE
  other register-resident local across the loop (model, above).
* **A cold block with a single predecessor is laid out adjacent to that
  predecessor regardless of its source position; VC6 inverts the branch if
  necessary to do so** (LoadScriptEvent l2 vs l4).
* **The LL10 empty-`if` block split is zero-cost in every position tried
  (nine sites across three bodies) and inert on residency, rematerialisation
  and hoisting residuals** — it is a forward-substitution lever only.
