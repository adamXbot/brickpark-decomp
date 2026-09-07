# Scope LL17 — partials wave A: allocation-class residuals (2026-09-08)

Branch `scope/LL17` from `origin/main` `b39f261b`. PARTIAL scope: seven
documented "allocation floor" WIP bodies reopened with the LL14 lever
(`docs/lanes/scope-ll14.md`, "Closing 0x0046f9a0": with the callee-saved
registers oversubscribed, VC6 ranked candidates by static name-appearance
count). Object prefix `/tmp/sll17_`. Brief:
`docs/SCOPE_LL17_partials_allocation.md`.

## Status

| address | name | file | before (strict) | after (strict) | audit | marker |
| --- | --- | --- | --- | --- | --- | --- |
| 0x0046d850 | ScrollIconPanel | fpui4.c | 35/121 | 35/121 | PASS, 6 OK | WIP (floor, note extended) |
| 0x00482430 | BuildPTPRoute | workorder3.c | 10/76 | **0/76** | PASS, 2 OK | **FUNCTION** |
| 0x0045e960 | FindObjDoorTile | mapbuild2.c | | | | |
| 0x0048f0f0 | InitExitCheckBox | screens2.c | | | | |
| 0x0048a3e0 | GetObjectUID | objmap2.c | | | | |
| 0x00459970 | TallyBuildFootprints | mapbuild2.c | | | | |
| 0x00499d60 | UnlinkGardenerOrder | workorder3.c | | | | |

(`[OK]` counts are the whole-file `audit.py` totals; baselines were fpui4.c 6,
workorder3.c 1, mapbuild2.c 2, objmap2.c 14, screens2.c 11.)

## Per-body analysis

### 0x0046d850 ScrollIconPanel — floor confirmed (13 spellings)

Post-call the body has four candidate webs: the two loads `list_x0`,
`list_y0` (CSE'd across the arms to the write-back) and the two sums `nx`,
`ny`. Each has three surviving refs (def + two uses) in every spelling.

| spelling | result | allocation (ours) |
| --- | --- | --- |
| baseline (sums named, loads as field CSE) | 89/121 | list_x0 ebp, list_y0 ebx, nx edx, ny edi |
| `ny` defined before `nx` | 86/124 | list_y0 ebp, list_x0 ebx, ny edx, nx edi |
| loads named `x0`/`y0`, 3 refs each | 89/121 | identical to baseline |
| `x0`/`y0` named, compare via `x0 + dx` (4 pre-CSE refs, sums 2) | 86/121 | x0 ebx, y0 ebp, nx edx, ny edi; `ny` lea sinks into its arm |
| sums first via fields, then named loads for write-back | 89/121 | identical |
| interleaved `x0, nx, y0, ny` | 89/121 | identical |
| compare via field expression, sums named | 61/118 | different regime |
| subtract via `(y0 + step)` expression | 93/125 | leas sink into arms |
| 4th algebraic use of `x0`/`y0` in the list_x1/list_y1 write-backs (named or field) | 89/121 | folded before ranking, identical |
| `x0`/`y0` defined above the early exits (used pre- and post-call) | 50/109 | held in ebx/edi across the call |
| the same with a post-call redefinition | 56/121, 51/121 | head breaks |

Model that fits every row: equal-rank webs are coloured LATEST-DEFINED FIRST
with register preference edi, edx, ebx, ebp (esi is `w`, eax/ecx are the
call result and `dx`). The original's `list_y0` edi, `list_x0` edx, `ny`
ebx, `nx` ebp is that model with the two loads ranked ABOVE the two sums.
The lever therefore predicts the fix (a fourth surviving ref on each load),
but no C spelling supplies one: a compare through `x0 + dx` is CSE'd with
`nx` before ranking, an algebraic 4th use folds before ranking, a copy
propagates. Every original instruction that touches the load registers is
already in our source. The model is right; the spelling is unreachable.

### 0x00482430 BuildPTPRoute — CLOSED 76/76

Ours had `b` edi / `c` esi, the original `b` esi / `c` edi. Appearance
counts (b 7, c 6, d 8) were the earlier lanes' explanation and predicted a
ref must move; in fact no count move was needed and none was possible
without an instruction:

| spelling | result |
| --- | --- |
| `n = b; AddPTPRouteNode(n->x, n->y)` (also for c, d; also `n = b; if (n)`) | 10, identical — pointer copies propagate before ranking |
| `x = b->x; y = b->y;` load-carrying temporaries | 10, identical |
| zero chains `d = c`, `b = c`, `c = d` | 10 (chains that keep the init order) or 12 (xor order only) |
| all six declaration/initialisation orders | xor order follows INITIALISATION order; register choice unchanged |
| guard as `if (!b) break;` | 10, identical |
| **one `return d == 0;` after the switch, every case ending in `break`** | **1 (xor order)** |
| the same with initialisation order b, c, d | **0, 76/76, 152 B** |

Diagnosis: the three `return d == 0;` copies in the original are LATE tail
duplication of one source return (LP05: small ret-ending blocks clone after
allocation, same registers in each copy). Spelling them as three source
returns builds three pre-allocation blocks, and that graph — not any
reference count — ranks `b` above `c`. This is a new negative for the
appearance-count model: a pure register swap between two equally-shaped
webs was a block-graph effect, and every count-moving spelling was inert.
