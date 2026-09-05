# Parallel session scope — "codex-a" (2026-09-05)

A self-contained brief for a Codex agent working **in parallel** with the
integrating session and with other parallel sessions. Everything you need is
here. The same shape has been run three times by other sessions (scopes
`docs/SCOPE_FABLE_A.md`, `_B.md`) and merged with zero conflicts each time.

## What this project is

A matching decompilation of LEGOLAND (Windows, 2000, VC6 SP3, `/O2 /Gy /Gd`).
Human-written C in `LEGOLAND/*.c` must compile to reproduce
`original/legoland.exe` function-by-function. Status at hand-off: **1724 exact
functions, 69 partials, 52.2% of game code matched exactly**. No game binary or
asset is ever committed (`original/` and `gamedata/` are gitignored).

**This session writes NEW functions from the unmatched-callee frontier** —
specifically its small-function tail: about 65 functions of 8–29 instructions
each, almost all of them currently named `Sub_<address>`. Small bodies close at
a very high rate here (the last five rounds of frontier work closed 10, 38,
52, 40 and 40 functions), and **naming each one from what it does is part of
the deliverable.**

## Read these first (in this order)

1. `docs/LANE_BRIEF.md` — the method per function and **the verification gate**
   (its PROJECT/STATUS lines are stale; use ENVIRONMENT below).
2. `docs/DECOMP.md`, section "VC6 SP3 codegen levers" — the ~250 entries at the
   TOP are the newest. The coaster, x87 and school-car ones were written on
   exactly your family over the last four rounds; search the rest when stuck.
3. `docs/HANDOFF.md` §3 (rules not in the code) and §6B (residual triage).

## Environment

```
cd /Users/systemadmin/Documents/Development/Github/legoland
git checkout main && git pull --ff-only origin main
PY=/Users/systemadmin/.venvs/legoland/bin/python
export LEGOLAND_CL=/Users/systemadmin/Documents/Development/Github/alphateam/tools/wibo-msvc/cl
```

If either path does not exist on your machine, **stop and report** — do not
try to build a toolchain or a venv.

- Disassemble: `$PY tools/disasm.py original/legoland.exe 0x<RVA> <count>` —
  **RVA = VA − 0x400000** (0x00426120 → 0x26120).
- Iterate: `$PY tools/matchfull.py LEGOLAND/<file>.c <Name> 0x<VA> --obj /tmp/ca_<Name>.obj`
- **Authoritative gate:** `$PY tools/audit.py LEGOLAND/<file>.c` — a function
  counts ONLY when it prints `[OK]`. `matchfull.py` can over-report a body whose
  `ret` is followed by a `.rdata` jump table; trust `audit.py`.
- `/W3` check: `ALPHATEAM_VC6_ROOT="$PWD/toolchain" "$LEGOLAND_CL" /nologo /c /W3 /O2 /Gy /Gd /Fo/tmp/ca_w3.obj LEGOLAND/<file>.c`
- Objects always under `/tmp`; never `/Zi`.
- **Do NOT run `tools/verify.py`, `tools/progress.py` or `tools/coverage.py`**,
  and do not edit any file under `tools/`. `audit.py` and `matchfull.py` use
  per-process object paths and are safe alongside other sessions' compiles.

## Your files and functions

Create these NEW files. Do not edit any existing `.c` file — read them freely.

### `LEGOLAND/coastermath.c` — named 3D/track helpers (≈215 insns)

| address | name | insns | declared by |
| --- | --- | --- | --- |
| 0x00426120 | `MatMul` | 10 | coaster3d.c, schoolcar3.c |
| 0x00425d30 | `Vec3Dot` | 11 | coaster3d.c, coaster4.c |
| 0x004260f0 | `MatIdentity` | 15 | coaster3d.c |
| 0x0041ef20 | `SetSpanClip` | 16 | coaster3d.c |
| 0x004264e0 | `MakeTransform` | 16 | schoolcar3.c |
| 0x0041ebd0 | `PackTrackClass` | 16 | coaster4.c |
| 0x00426ab0 | `FastSqrt` | 21 | coaster5.c |
| 0x00426980 | `FastRSqrt` | 21 | coaster5.c |
| 0x00425de0 | `Invert2x2` | 21 | coaster3d.c |
| 0x0041d1d0 | `BuildJoint` | 21 | coaster.c |
| 0x00425d50 | `Sub_425d50` | 29 | coaster4.c, schoolcar4.c |
| 0x004299a3 | `Sub_429a30` | 19 | coaster.c — **see note** |

Note on 0x004299a3: that address is not 16-aligned and is probably the middle
of a function, i.e. `coaster.c`'s extern is mis-declared. Disassemble around it
first; if it is not a function entry, do not write a body — report the finding
with the real entry address instead.

`FastSqrt`/`FastRSqrt` are the routines whose tables `coaster5.c`'s
`FastSqrt_InitTables`/`FastRSqrt_InitTables` (exact) publish at
0x00829a58/0x00829a5c — per-bucket tangent-line approximations over the top six
mantissa bits times an exponent table; read that header first. `coaster3d.c`
and `schoolcar3.c` document the 3D basis conventions (`MakeRotation`, the
view matrix at 0x008299fc). `coaster5.c`'s `TrackFitCheck*`/`TrackPieceSetJoints`
are `BuildJoint`'s callers.

### `LEGOLAND/schoolcar8.c` — the coaster/school-car micro-subs (≈830 insns, ~53 functions)

Every `Sub_*` of 8–29 instructions declared by `schoolcar.c` or `schoolcar4.c`
(addresses 0x0041cca0..0x00429270; list them with
`$PY tools/callees.py | awk '$1 < 30 && $4 ~ /schoolcar/'`). They are the
coaster's vector, matrix, physics, route and z-buffer micro-helpers — the
pipeline `schoolcar3.c`..`schoolcar6.c` and `coaster3d.c`..`coaster5.c` (all
from the last four rounds) documents from the callers' side: an RK4 solver
descriptor with a ten-entry vector-op table filled by `schoolcar.c`'s
`CarPoolInit` at 0x004dcbd0 (scale, max-abs, zero, add-scaled, alloc, free…),
route nodes with two track cursors at +0x08/+0x40, a cubic vertical profile at
+0x24..+0x30, track pieces of 0xa4 bytes, and the `{int n; short ylast; short;
{short x; short y; int step;} v[n]}` z-buffer command. **Most of your
functions are the slots of that vector-op table** — `schoolcar5.c`'s
`PhysVec_ScaleAdd2` (slot 5, exact) is your template for the family.

Excluded (owned elsewhere, do NOT write): 0x00421be0, 0x00421c30, 0x00426ec0,
0x00423480, 0x00420640, 0x0041e500, 0x00401000 (a running lane's
`schoolcar7.c`), and 0x00429990, 0x004296f0, 0x00429840, 0x00426560,
0x00421660 (`coaster6.c`).

**Order:** `coastermath.c` first (the named ones teach the conventions), then
`schoolcar8.c` smallest first. Save after every function. Name each `Sub_*`
from what it does (`PhysVec_Scale`, `Route_…`, `TrackCurve_…`, `ZBuffer_…`
are the existing prefixes); one name per address; if a name collides with an
existing symbol, pick another.

## Files you must NOT touch

- Any existing `LEGOLAND/*.c` (read only), and anything under `tools/`.
- These new files belong to lanes running right now: `render4.c`,
  `bswater3.c`, `coaster6.c`, `schoolcar7.c`, `screencb5.c`, `ridemisc3.c`,
  `workers3.c`, `objdoor.c`, `posstep.c`; and to other parallel sessions:
  `sysmisc2.c`, `workorder4.c`, `savegame2.c`, `data3.c`, `mapscreen4.c`,
  `audio4.c`, `uimisc.c`, `screencb6.c`.
- `docs/DECOMP.md`, `docs/HANDOFF.md`, `README.md`,
  `docs/LEGOLANDPROGRESS.HTML`, `docs/LEGOLANDPROGRESS.SVG`. **Put your levers
  and findings in `docs/lanes/codex-a.md`** in DECOMP's bullet style, each with
  its evidence; the integrating session folds them in.

## Git

`git checkout -b codex/scope-a` from current `main`. Commit to that branch
only; **never commit to `main` and never push `main`.** Push the branch when a
file is complete. Commit messages: one line of what closed, then the counts.

## Rules that are not in the code

- **Marker discipline.** `// WIP-FUNCTION: LEGOLAND 0x<VA>  (<pct>, <precise residual>)`
  until `audit.py` prints `[OK]`; then exactly `// FUNCTION: LEGOLAND 0x<VA>`.
  Marker on the line immediately above the signature. **Never commit a
  prologue-only or fabricated-tail body as `// FUNCTION:`.**
- Declare every callee `extern` with a trailing `/* 0x0044xxxx */` comment.
- **Extern prototype TYPES are caller-side codegen levers.** A callee may need
  different prototypes in different files; never "align" another file's
  declaration, note the divergence. Symbol NAMES are not levers.
- **One name per address.** Placeholder names that collide with a real symbol
  get a distinct name, recorded.
- Reproduce original bugs; comment them at the site; do not fix them.
- Report behaviour you cannot explain from the disassembly rather than
  inventing plausible C. An honest WIP beats a wrong body that compiles close.

## Levers that decide whether a first draft lands at 95% or 40% (this family)

- **x87/floats:** float source order is the REVERSE of `fld` order (except at
  an accumulator site, where `acc += step` is the only spelling); a float swap
  needs NO temporary; `((float)a - b) * K` emits `fild/fisub`; three in-place
  idioms — `fild x / fstp x`, `fild / fmul k / fistp x`, `fld f / fistp i` —
  are not C casts; a `volatile float` local is how an FP constant gets a stack
  home; explicit PARENTHESES stop VC6 reassociating FP constants; a
  double-typed intermediate pools the constant as a double; an embedded
  assignment `c = pa - (m = e) * a;` produces `fst`; float constants are exact
  single-precision literals (`%.9g` of the bit pattern), never `(float)M_PI`;
  a value used in both arms must be READ IN BOTH ARMS for one head-merged
  load; VC6 spills by furthest next use and `st(7)` depth forces siblings to
  memory operands.
- **Vectors/matrices:** subscripted access vs a named pointer decides whether
  a subtraction folds (declare pointers AFTER); two lockstep cursors spelled
  the SAME way let VC6 eliminate an induction variable; a strength-reduced
  cursor anchors at the field with the MOST references (ties to the LAST;
  offset 0 never wins from `->`); `i <= N-1` vs `i < N` is `jle` vs `jl`; a
  snapshot handed to a float callee must be a RAW DWORD with an `int`
  prototype; a Pos/8-byte struct RETURN must be the ACCUMULATOR of any sum
  added to it; two tables pushed as `push OFFSET` are ARRAYS.
- **Layout:** an else arm is exiled past the fall-through trace iff it ends in
  an unconditional jump; two `return K` sites merge at the FIRST; the
  fall-through copy of a shared tail survives; a jump-table `switch`'s case
  order is its block order (a compare-chain switch is laid out by case
  VALUES); a degenerate `cmp/jne +0` comes only from a self-assignment arm
  (`if (i == 0x1d) i = 0x1d;` is a breakpoint hook the developers left in —
  reproduce it).
- **`__asm`:** operand names collide with MASM reserved words (`cr0`-`cr4`,
  `dr0`-`dr7`, `tr3`-`tr7`, `st`). Hand-written asm shows as `xchg` against
  memory, callee-saved pushes INSIDE the stream, or an EBP frame in an `/O2`
  file — `ZBuffer_FillPoly` (0x00423350) is one; say so early if you see it.

## When a function will not close

Run the §6B triage: strict, register-blind, offset-blind with frame homes by
push depth. `strict >> rb` is allocation — try one free `volatile` read first,
then the one-temporary spellings (name an array element or a call result,
split a nested call); `strict >> ob` is the frame; `strict == rb == ob` is a
scheduling permutation — sweep the store order. Then write the honest WIP note
and move on; with sixty-odd functions, breadth beats depth.

## Report format (in `docs/lanes/codex-a.md`, and in your final message)

Per function: address, the name you gave it and why, instructions,
percentage, `audit [OK]` yes/no, marker as committed, first diverging index
and residual for anything not `[OK]`. Then mechanics recovered (the vector-op
table's slot semantics are runtime-spec material), original bugs reproduced,
extern-type divergences, and every lever with its evidence.
