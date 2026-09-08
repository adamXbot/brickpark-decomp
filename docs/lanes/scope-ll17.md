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
| 0x0045e960 | FindObjDoorTile | mapbuild2.c | 6/89 | **0/89** | PASS, 3 OK | **FUNCTION** |
| 0x0048f0f0 | InitExitCheckBox | screens2.c | 118/119 | 118/119 | PASS, 11 OK | WIP (floor, note extended) |
| 0x0048a3e0 | GetObjectUID | objmap2.c | 20/191 | 20/191 | PASS, 14 OK | WIP (floor, note extended) |
| 0x00459970 | TallyBuildFootprints | mapbuild2.c | 6/116 | 6/116 | PASS, 3 OK | WIP (floor, note extended) |
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

### 0x0045e960 FindObjDoorTile — CLOSED 89/89 (reconstruction error, 8 spellings)

The original loads the door offset into a callee-saved register in both
blocks (`d.y` ebx in the entrance block; `d.x`/`d.y` ebx/edi in the exit
block); ours took the free scratch (edx; ecx/esi). Not a count problem:

| spelling | result |
| --- | --- |
| baseline `Pos d = cls->door; x = pos->x + d.x` | 83/89 |
| two separate `Pos d1`, `Pos d2` | 83/89 |
| block-scope `int dx, dy` | 84/89 |
| `Pos* dp = &cls->door` | 80/89 |
| `y` computed before `x` | 73/87 |
| **direct `x = pos->x + cls->door.x; y = pos->y + cls->door.y;`** (either operand order, both blocks) | **89/89** |

RA02 in its plainest form: a named aggregate copy and a direct field
expression are different webs, and only the direct expression gets the
original's registers. The appearance-count model has nothing to say here.

### 0x0048f0f0 InitExitCheckBox — floor re-confirmed (6 spellings, resumed session)

The residual is `push ebx / xor ebx,ebx / pop ebx`: a three-store constant
zero the original keeps in a callee-saved register. Five earlier passes
measured the hoist threshold (four unweighted uses) and the byte-class rule
for ebx. The two LL17 levers were tried and are inert:

| spelling | result |
| --- | --- |
| `int z = 0` at the top + `z = 0` again in the else arm (two reaching defs) | baseline, byte-identical |
| `z = 0` in all three arms, no top definition | baseline |
| `char z = 0` carrier (PASS N+1's 119-instruction lead) | baseline (confirms PASS N+2) |
| char carrier + `if (z) ;` pin after the def | baseline |
| char carrier + pin before the shared-block store | baseline |
| char carrier + pins after the def and before the tail stores | baseline |

VC6 folds phi(0,0), so a same-constant redefinition is not a second reaching
definition; the empty-if pin is folded along with a constant-valued local.
LL14's ranking model does not apply — the zero is the only callee-saved
candidate, so there is nothing to rank it against. Prediction: none
(out of the model's domain). Retired.

### 0x0048a3e0 GetObjectUID — floor re-confirmed (6 spellings, resumed session)

Residual: the two horizontal probes load `g_map` at first use inside the
guard (edx) where the original re-materialises it at the region entry (esi),
the zero-extend scratch taking the other register. The LL10 block-split pin
was placed at every position the earlier passes' mechanism analysis names:

| spelling | result |
| --- | --- |
| caller `m = g_map; if (m) ;` before each horizontal probe, `CellM(m, y, x)` | 20, byte-identical |
| `if (def) ;` as the pin instead of `if (m) ;` | 20, identical |
| pin inside a horizontal-only helper after `Map* m = g_map;` | 20, identical |
| pin inside the guard at the row-table read (`rows = g_map_rows; if (rows) ;`) | 20, identical |
| `if (y) ;` before `g_map_rows[y][x]` inside the guard | 20, identical |
| one `m` shared by both horizontals, pinned once | 20, identical |

A global load is re-materialised at its use (PASS 4's rule), and a block
boundary does not stop that: the LL10 pin works on register-resident locals
(its witnesses were x87 floats). LL14: the contested esi is between the map
pointer (1–3 appearances already measured identical) and an unnamed
compiler scratch, so no reference count can move. Prediction: none.
Retired; PASS 6's volatile proof (register-blind zero) remains the sharpest
statement.

### 0x00459970 TallyBuildFootprints — two floors, mechanism recovered (20 spellings)

Residual: in both latches (and both entries) the original reads `sq->x1`
before it stores `pt.x`; ours after. `strict == rb == ob`.

| spelling | result |
| --- | --- |
| baseline `for (pt.x = sq->x0; pt.x <= sq->x1; pt.x++)` | 110/116 (6) |
| do/while with `++pt.x <= sq->x1` | identical |
| do/while with `pt.x++ < sq->x1` | 93/118 |
| **diagnostic: a global as the bound** | x loop EXACT — the pin is a pointer-load vs frame-store dependence |
| all `pt` accesses through `Pos* pp` | identical |
| `/Oa`, `/Ow`, `#pragma optimize("a", on)`, `(unsigned)` view, volatile view | identical — the dependence is not lifted by any aliasing switch |
| `end = sq->x1` before `pt.x++` (do/while); `for (...; end = sq->x1, pt.x++)`; `pt.x = pt.x + ((end = sq->x1), 1)` (3 operand orders) | **106/116, register-blind ZERO**: exact sequence, eax/ecx swapped in both latches |
| `v = pt.x; end = ...; v++; pt.x = v` and `v += 1`, `v = pt.x + 1` forms | 94–96: `lea eax,[ecx+1]`, two webs |
| LL10 pin `if (v) ;` between load and `++`, and after `++` | identical to the unpinned form |
| temp entry `v = sq->x0; end = sq->x1; pt.x = v; if (v <= end)` | entry block EXACT (natural entry is not) |

Mechanism: scratch registers follow definition order; VC6 hoists the
side-effecting subexpression (`end = ...`) to the front of any statement, so
the bound always defines first; the only construct that defines the counter
first and keeps `inc` in one web is the bundled memory increment, whose store
then precedes the bound read. The original's source therefore read the bound
before the store with the counter defined first — a shape no C statement
reproduces here. Committed body unchanged (strict best). LL14: not applicable
(scratch, definition-ordered). LL10: inert (forward substitution hoists the
consumer to the definition site rather than the reverse).
