# Parallel session scope — "fable-a" (2026-09-05)

A self-contained brief for a second Claude Code session working **in parallel**
with the main integration session. Everything you need is here; you do not
need the other session's context.

## What this project is

A matching decompilation of LEGOLAND (Windows, 2000, VC6 SP3, `/O2 /Gy /Gd`).
Human-written C in `LEGOLAND/*.c` must compile to reproduce
`original/legoland.exe` function-by-function. Status at hand-off: **1554 exact
functions, 42 partials, 45.7% of game code matched exactly**. No game binary or
asset is ever committed (`original/` and `gamedata/` are gitignored).

**This session writes NEW functions from the unmatched-callee frontier.** That
is where coverage moves: the last wave that did this closed ten functions;
the four before it, grinding existing partials, closed none.

## Read these first (in this order)

1. `docs/LANE_BRIEF.md` — the method per function and **the verification gate**
   (its PROJECT/STATUS lines are stale; use ENVIRONMENT below).
2. `docs/DECOMP.md`, section "VC6 SP3 codegen levers" — the ~60 entries at the
   TOP are the newest and several correct older ones. The rest is ~400 more
   levers; search it when stuck rather than reading it end to end.
3. `docs/HANDOFF.md` §3 (rules not in the code), §6B (residual triage), §6C.
4. `docs/RIDE_CALLBACKS.md` — names most ride callbacks by object-definition
   slot; disassembling the relevant `*_GetInterfaces` provider first often
   arrives with a cluster pre-named.

## Environment

```
cd /Users/systemadmin/Documents/Development/Github/legoland
PY=/Users/systemadmin/.venvs/legoland/bin/python
export LEGOLAND_CL=/Users/systemadmin/Documents/Development/Github/alphateam/tools/wibo-msvc/cl
```

- Disassemble: `$PY tools/disasm.py original/legoland.exe 0x<RVA> <count>` —
  **RVA = VA − 0x400000** (0x0041c4c0 → 0x1c4c0).
- Iterate: `$PY tools/matchfull.py LEGOLAND/<file>.c <Name> 0x<VA> --obj /tmp/fa_<Name>.obj`
- **Authoritative gate:** `$PY tools/audit.py LEGOLAND/<file>.c` — a function
  counts ONLY when it prints `[OK]`.
- `/W3` check: `ALPHATEAM_VC6_ROOT="$PWD/toolchain" "$LEGOLAND_CL" /nologo /c /W3 /O2 /Gy /Gd /Fo/tmp/fa_w3.obj LEGOLAND/<file>.c`
- Objects always under `/tmp`; never `/Zi`.
- **Do NOT run `tools/verify.py`, `tools/progress.py` or `tools/coverage.py`.**
  The integrating session runs those alone on a quiet tree. `audit.py` and
  `matchfull.py` use per-process object paths and are safe to run while other
  sessions compile.

## Your files and functions

Create these NEW files. Do not edit any existing `.c` file — read them freely.

### `LEGOLAND/bswater.c` — boating-school water and launch (≈418 insns)

| address | name | insns | declared by |
| --- | --- | --- | --- |
| 0x0041c4c0 | `BsWater_SetTile` | 109 | ridecb5.c, ridecb6.c |
| 0x0041cb20 | `BsWater_WalkStep` | 108 | ridecb6.c |
| 0x00418e60 | `BoatingSchool_TryLaunch` | 95 | ridecb5.c |
| 0x0042c6d0 | `Carousel_TickInstance` | 106 | ridecb3.c |

`BsWater_Probe` (0x0041c690) was matched exact last wave in `LEGOLAND/joust2.c`
— read it first, it documents the water/school/owner model and its two levers
(arithmetic in a call argument runs before the inlined guard; initialiser order
places the spill store). `ridecb5.c` has the placement side of the same family.

### `LEGOLAND/objrect.c` — object rectangles, path tiles and work orders (≈630 insns)

| address | name | insns | declared by |
| --- | --- | --- | --- |
| 0x004777f0 | `GetRouteNode` | 122 | simcore.c |
| 0x0045d3d0 | `RemoveObjectPathTiles` | 122 | mappath.c |
| 0x0045d5d0 | `SubtractObjRect` | 118 | workorder2.c |
| 0x0045e4a0 | `RefreshObjectAtPos` | 110 | workers.c, workorder.c |
| 0x0045e850 | `ClearObjectUserFlags` | 80 | mappath.c, objmap2.c |
| 0x0045e770 | `SetObjectDoorFlags` | 78 | popup.c |

`LEGOLAND/mappath.c` (all four of its functions exact) and `LEGOLAND/objmap2.c`
declare the cell/map/object structs and model the bounds-checked cell fetch as
`static __inline Cell* CellAt(int x, int y)` — reuse that shape. `mappath.c`'s
header already describes `RemoveObjectPathTiles` from its caller's side.
`RequestRoute` in `simcore.c` is the caller of `GetRouteNode`.

### `LEGOLAND/jcroute.c` — jungle cruise routing (≈238 insns)

| address | name | insns | declared by |
| --- | --- | --- | --- |
| 0x00437260 | `JungleCruise_TraceRoute` | 130 | junglecruise.c |
| 0x00437440 | `JungleCruise_StepRoute` | 108 | junglecruise.c |

`LEGOLAND/junglecruise.c`, `ridecb2.c` and `ridecb9.c` hold the rest of the
ride and its structs.

### `LEGOLAND/goldrush2.c` — gold rush people (≈248 insns)

| address | name | insns | declared by |
| --- | --- | --- | --- |
| 0x004064d0 | `Fort_StepVisitor` | 126 | goldrush.c |
| 0x004025d0 | `SetPerson3DHeading` | 122 | goldrush.c |

`LEGOLAND/goldrush.c` (its caller) and `LEGOLAND/person3d.c` (the 3D bloke
renderer) are the context.

**Order:** smallest first within each file, and finish a file before starting
the next: `goldrush2.c` → `jcroute.c` → `bswater.c` → `objrect.c`. Save after
every function; the file on disk is what survives if you are cut off.

## Files you must NOT touch

- Any existing `LEGOLAND/*.c` (read only).
- These four new files belong to the other session's lanes running right now:
  `schoolcar3.c`, `roads2.c`, `screencb.c`, `logflume4.c`, `popup2.c`,
  `mapscreen2.c`, `render3.c`.
- `docs/DECOMP.md`, `docs/HANDOFF.md`, `README.md`,
  `docs/LEGOLANDPROGRESS.HTML`, `docs/LEGOLANDPROGRESS.SVG`. These are shared
  and regenerated at integration. **Put your levers and findings in
  `docs/lanes/fable-a.md`** (create it) in the same style as DECOMP's bullets —
  each with its evidence — and they will be folded into DECOMP at integration.

## Git

Work on a branch: `git checkout -b fable/objrect-rides` from current `main`.
Commit to that branch only, as often as you like; **never commit to `main` and
never push `main`.** Pushing the branch is fine. The integrating session will
merge, run `verify.py` alone, regenerate the progress report and push. End every
commit message with:

```
Co-Authored-By: Claude Fable 5.1 <noreply@anthropic.com>
```

## Rules that are not in the code (the ones that bite)

- **Marker discipline.** `// WIP-FUNCTION: LEGOLAND 0x<VA>  (<pct>, <precise residual>)`
  on a body until `audit.py` prints `[OK]`; then exactly
  `// FUNCTION: LEGOLAND 0x<VA>` with no parenthetical. The marker goes on the
  line immediately above the signature. **Never commit a prologue-only or
  fabricated-tail body as `// FUNCTION:`.**
- Declare every callee `extern` with a trailing `/* 0x0044xxxx */` comment —
  `tools/callees.py` parses it.
- **Extern prototype TYPES are caller-side codegen levers.** If you need a
  different type for a callee another file already declares, say so in your
  notes; do not "align" the other file. Symbol NAMES are not levers.
- **One name per address.** If a callee's placeholder name collides with a real
  export (it has happened: `AddObjectToMap` is the export at 0x0045dd80),
  give the non-exported one a distinct name and record it.
- Reproduce original bugs; do not fix them. Comment them at the site.
- Report behaviour you cannot explain from the disassembly rather than
  inventing plausible C. An honest WIP beats a wrong body that compiles close.

## Levers that decide whether a first draft lands at 95% or 40%

- **Block layout beats everything on fresh code.** The block-exile rule is an
  iff: an else arm is exiled past the fall-through trace iff the arm itself ends
  in an unconditional jump. `if (a == 0 || b != c) return X;` exiles X past the
  whole function; splitting the test and jumping *into* the second `if`'s
  compound statement puts it back inline. Two `return K` sites with the same
  constant merge at the FIRST site — a `goto fail;` to a trailing `fail:
  return 0;` pins the merged block at the end. A `switch`'s case ORDER is its
  block order, and a label sharing a jump-table entry may still be a separate
  case body in the source (test per label).
- **Loop form.** `while (n-- != 0)` on an unsigned counter rotates into a
  do/while with dead `dec`/`inc`. An up-counting `for (i = 0; i < N - 2; i++)`
  emits `add eax,-2`; the down-counting form emits `sub eax,2`.
- **Types.** A stashed `u16` field is often an `int` local — check the width of
  the STORE. `movsx` vs `movzx` says signed vs unsigned byte. Map coordinates
  are frequently signed bytes.
- **Aggregates.** VC6 flattens an aggregate whose address is never taken, so a
  struct wrapper changes nothing about field ACCESS — but it still decides
  PLACEMENT: two ints as one `Pos` land at the top of the frame, and two
  byte-wide values that must survive calls get the original's EBX winner only
  as one shared two-byte aggregate. Two one-member structs behave like scalars.
- **Reads.** Read a class/def global directly at every use rather than caching
  it in a local. A free `volatile` read (at a site the original loads anyway)
  goes at the DEFINITION site, not the use — VC6 will not hoist a volatile
  access across a store. `(void)&x;` does NOT make a local address-taken.
- **Sums.** A flat three- or four-term sum is canonicalised — do not spend
  variants reordering one. A two-term sum's destination register is decided by
  the destination symbol alone (`py += e` is the only spelling that keeps `py`).
- **x87.** Conversion order can be read off the original's operand
  displacements. Stack depth is a lever: a value pinned at `st(7)` forces its
  siblings to memory operands.
- **`__asm`.** Operand names collide with MASM reserved words (`cr0`-`cr4`,
  `dr0`-`dr7`, `tr3`-`tr7`, `st`, segment names). A local named `cr2` in this
  project became the control register and emitted a privileged instruction.

## When a function will not close

Run the §6B triage before spending hours: measure strict, register-blind and
offset-blind with frame homes resolved by push depth (never raw `[esp+N]`).
`strict >> rb` is allocation, `strict >> ob` is the frame, `strict == rb == ob`
is a pure scheduling permutation, and rb-still-high is a structural difference
— the only kind reliably reachable from C. Then insert one free `volatile` read;
if it moves nothing, the residual is a global web rank and no ordering construct
will reach it. At that point write the honest WIP note and move on — a 95% body
with a precise note is a good outcome.

## Report format (in `docs/lanes/fable-a.md`, and in your final message)

Per function: instructions, percentage, `audit [OK]` yes/no, marker as
committed, first diverging index and residual for anything not `[OK]`. Then
mechanics recovered (these headers are the spec for an eventual browser
runtime), any callee named for the first time, any original bug reproduced,
and every lever with its evidence.
