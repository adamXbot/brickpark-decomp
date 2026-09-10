# FGH handoff — for the next session (2026-09-08)

Written by the integrator session that closed scope V. Read this first, then
`docs/lanes/fgh-pickup.md` for the measurements behind it. The long history is
in `fgh-integration.md`, `fgh-100.md`, `fgh-100b.md`, `fgh-100c.md` and the
per-scope `scope-f/g/h.md`; do not re-read those to get oriented, they are the
evidence trail.

## Status: 42 of 48, unchanged by this session

Re-measured on `fgh-pickup` with the lane's own gate, not inherited:

```
validation/fgh/check.py  ->  assigned_exact_gate: 42
                             assigned_open: 6
                             existing_normalized_regressions: 0
                             exact_markers: 410 / 419 annotated
```

| target | file | mism | class |
| --- | --- | ---: | --- |
| `Coaster3D_BuildTrackMesh` 0x00428cb0 | schoolcar3.c | 6 | **element window SOLVED, one hoist blocks it** |
| `SpaceTower_Activate` 0x0043bac0 | mechrides.c | 10 | head scheduling cycle |
| `JungleCruise_Tick` 0x00435750 | ridecb2.c | 11 | ebx/ebp swap, equal feature rows |
| `TempleSlide_Update` 0x00417430 | joust.c | 18 | same cell; original violates the corpus rule |
| `StepSchoolCar` 0x00402780 | goldrush.c | 72 | scratch eax/ecx/edx rotation |
| `Raster_SubmitPoly` 0x0042a2f0 | coaster3d.c | 108 | callee-saved 3-cycle; original violates the rule |

No production body was changed this session. Two commits on `fgh-pickup`
(branched from `cursor/fgh-100c` @ `41bd26f4`) add lane notes only:

```
d4b24106  second Mesh round + the JungleCruise byte-need negative
6e487579  Mesh element window reproduced with the scope-V cancel lever
```

To pick them up: `git fetch origin && git merge origin/fgh-pickup` (ask the
integrator to push it if the ref is missing).

## The two levers this session brings, and where they came from

Scope V closed `EventTick_Clear` (0x00469c80) on 2026-09-08 after nine passes.
Both levers are now at the top of DECOMP's "VC6 SP3 codegen levers" section
under **SCOPE V**; read that entry, it is short. In one line each:

1. **Sibling-copy forwarding kill.** A `rep movsd` whose destination is a
   member of a *local aggregate* kills the cached value of **every** member of
   that aggregate, so a plain (non-`volatile`) narrowing read of a sibling
   reloads from its home. Hole members reproduce a `Rect`'s never-stored slots
   without memory-homing the ones that are stored. Write member sums as one
   statement each (`L.top = f.top + by`), never `=` then `+=`.
2. **Cancelled-pointer copy web.** `t.x = v; t.x += (int)p; t.x -= (int)p;`
   through a **two-member struct** keeps `t.x` a web distinct from `v`, so a
   later use of `t.x` costs a bare `mov r32, r32` from the original register.
   Instruction selection cancels the add/sub; copy propagation cannot see
   through it. Every identity expression written directly on `v` folds.

Lever 2 is the one that matters here: it is the general answer to "the original
copies a value into a callee-saved register instead of propagating it", which
is exactly Mesh's residual and was previously only reachable with `volatile`.

## Mesh — the one worth picking up first

**The residual.** Original window (indices 27-32):

```
mov ecx,[eax]          load the element
mov esi,ecx            COPY to the callee-saved register   <- 2 bytes
mov [ebp-14h],ecx      dead store into the record
mov ecx,[ebp+10h]      slot, reloaded
```

The incumbent body gets the store first and then *reloads* esi from
`[ebp-14h]` (3 bytes) because the only spelling that pinned the order was a
`volatile` read. Same instruction count, one byte over: 442 vs 441.

**What is now solved.** With the record local `struct { void* elem; Vec3f pos; }
cur;` plus lever 2 anchored on `slot`:

```c
cur.elem = list[i];
tc.y = slot;              /* struct { int x, y; } tc; */
tc.x = (int)cur.elem;
tc.x += tc.y;
tc.x -= tc.y;
e = (void*)tc.x;
o->hooks[slot].get_dir(o, e, &dir);
```

the window compiles to the original's shape — copy before the dead store, both
pushes from esi — with **no `volatile`, no extra instruction, 151 i**. That is
the first time this window has been reproduced; every previous family either
sank the store or reloaded.

**What blocks it.** One callee-saved decision upstream. `slot` is *not*
hoisted in the original: it is reloaded from `[ebp+10h]` at both hook calls,
which leaves ebx free for the `out` cursor (the original's preheader is
`mov edi,[ebp+8]` = o, `mov ebx,6139C8h` = `g_track_verts`, then it reuses the
three dead parameter homes for the list cursor, the out cursor and the
down-counter). Giving `slot` a third use crosses VC6's LICM threshold, `slot`
is hoisted into ebx in the preheader, and `out` is pushed into `[ebp+10h]` and
reloaded inside the inner loop. Net 30 strict, first divergence 19.

**So the whole problem is now:** *get the cancel without giving any
loop-invariant a third use.* Reopen from the `m5_slot` shape, not from the
`volatile` union.

Bounded negatives from this session (do not repeat):

- **Anchor-free struct cancels fold back to Build P** (151 i / 441 B / first
  27): bare member copy, two-member chain `tc.y = v; tc.x = tc.y`, one-member
  pointer struct, self-doubling `tc.x += tc.x; tc.x -= v`. The mirrored
  self-anchor does not fold but does not cancel either (154 i). **The cancel
  needs two genuinely different values**, as in CLEAR.
- **Frame addresses never cancel.** `&dir`, `&rot`, `&cur.pos`, `&xf`,
  `&o->hooks[slot]`, `g_track_verts`, `&g_track_mesh` all leave the add/sub
  in place (153-157 i). **Only a value loaded from memory into a register
  cancels.**
- **Capping the anchor at two uses just moves the hoist.** Indexing one or
  both calls through `tc.y`, through `h = o->hooks`, or through
  `hs = &o->hooks[slot]` hoists the new name instead (31-61).
- **Loop-variant anchors are worse.** `i`, `n`, `last` change the loop shape
  (58-90, first 2). Explicit `void** lp` and `out += 6` cursors, anchored or
  not, give 44-53.
- **Escaped-pointer reads do not stop the hoist.** `int* sp = &slot` outside
  the loop with `tc.y = *sp` is 40, first 19. A `volatile` read of the
  parameter changes the frame (first 2).

Untried ideas, in the order I would try them:

1. A source shape where **`out` outranks `slot`** rather than one where `slot`
   is not hoisted — give the out cursor a use VC6 counts at loop depth 2
   without emitting an instruction. Adding a `TrackVtx* p = out` or
   `int* sh = &out->shade` walk did *not* work (both are the same derived IV,
   151 i / 30), so it needs to be a use of `out` itself.
2. An anchor loaded from a **non-hoistable location that is already read in
   the window** — the only real candidate left is something reached through
   `cur`, whose address escapes at `get_pos`; the cost is the extra store, so
   it only pays if that store replaces one already there.
3. Accept the hoist and check whether the resulting 30-strict body is
   *closer in the metric that matters* after the preheader is repaired some
   other way. It is not, today, so this is a last resort.

## JungleCruise — one hypothesis closed

fgh-100c §3.3 leaves the ebx/ebp assignment as "the unexplained cell": the
zero constant (29 uses) and `inst` (7 uses) have identical feature rows and
opposite assignments between the original and the de-shimmed body.

Scope V measured a rule that would have explained it — **a web with a register
byte use takes the byte-capable register absolutely, displacing even `next`
from ebx** — so the natural guess is that the original's zero constant is
stored to a byte field somewhere and therefore needs `bl`.

**It is not.** A scan of the original body (0x00435750, 360 instructions)
finds no byte reference to ebx at all, and `inst` in ebp is used only as a
base register (`[ebp]`, `[ebp+8]`, `[ebp+0Ch]`), paying the mandatory disp8
byte each time. The original chose the *worse* encoding for `inst`. The cell
stays unexplained and the scope-V byte rule does not reach it. Do not spend
another round looking for a byte need here.

## The other four

`SpaceTower_Activate` (head 25-27 is a scheduling cycle, fgh-100b §7-2),
`TempleSlide_Update`, `StepSchoolCar` and `Raster_SubmitPoly` were read but
not attacked. fgh-100c's corpus fit is the key fact for the last two: **the
original violates the rule fitted from 2,717 exact bodies at both sites**, so
they are not spelling problems and 48/48 may not be reachable by spelling at
all. If a round has to be spent on them, spend it on a new mechanism, not on
another sweep.

## Checkpoint plan (integrator's recommendation)

Merging `cursor/fgh-100c` to main at 42/48 is worth doing and merges clean —
no file overlap with main since the merge base `6a95613e`, no binary or game
asset committed anywhere on the branch, gate green with zero regressions. It
carries two closes main does not have (`Joust_Update`, `LFEntrance_Activate`)
and a real reconstruction fix: `JungleCruise_UpdateRiverAnim` 0x00432d00 had
zero normalized differences but branched to the wrong arithmetic tails, fixed
by `soA.y -= 0x10` / `soB.y -= 0x10` in the two seat-1 cases.

**Merge the 48 source, docs and validation files; leave `scratchpad/` out.**
It is 292,182 of the 301,153 added lines, main tracks no scratchpad file
today, and the probe output in it is already summarized in the lane notes.
Separately, `scratchpad/fgh/backup-review/` holds resource manifests derived
from Dutch, Czech and Japanese-demo copies of the game — JSON listings rather
than assets, so not a rule break, but a further reason to keep that tree
local rather than push it to a public remote.

## Environment

```
PY=/Users/systemadmin/.venvs/legoland/bin/python
export LEGOLAND_CL=/Users/systemadmin/Documents/Development/Github/alphateam/tools/wibo-msvc/cl
export ALPHATEAM_VC6_ROOT="$PWD/toolchain"
PYTHONPATH=/private/tmp/legoland-fgh-deps   # unicorn, for validation/fgh
```

Per-function iteration: `$PY tools/matchfull.py LEGOLAND/<f>.c <Name> 0x<VA>
--obj /tmp/<prefix>_<Name>.obj`. Authoritative per-file gate:
`$PY tools/audit.py LEGOLAND/<f>.c`. Lane gate:
`cd validation/fgh && $PY check.py --output /tmp/fgh.json`. Do not run
`tools/verify.py` alongside other compiles.
