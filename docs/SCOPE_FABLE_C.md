# Parallel session scope — "fable-c" (2026-09-05)

Third self-contained brief for a Claude Code session working **in parallel**
with the integrating session. Scopes A and B (`docs/SCOPE_FABLE_A.md`, `_B.md`)
were both completed and merged with zero conflicts — 12 of 14 and 13 of 18
exact. Same workflow, new files. Everything you need is here.

## What this project is

A matching decompilation of LEGOLAND (Windows, 2000, VC6 SP3, `/O2 /Gy /Gd`).
Human-written C in `LEGOLAND/*.c` must compile to reproduce
`original/legoland.exe` function-by-function. Status at hand-off: **1644 exact
functions, 60 partials, 49.9% of game code matched exactly**. No game binary or
asset is ever committed (`original/` and `gamedata/` are gitignored).

**This session writes NEW functions from the unmatched-callee frontier.** The
last three rounds of this closed 10, 38 and 39+13 functions; grinding existing
partials closed none in the four rounds before them.

## Read these first (in this order)

1. `docs/LANE_BRIEF.md` — the method per function and **the verification gate**
   (its PROJECT/STATUS lines are stale; use ENVIRONMENT below).
2. `docs/DECOMP.md`, section "VC6 SP3 codegen levers" — the ~150 entries at
   the TOP are the newest (your scope A and B levers are folded in as two
   blocks). Search the rest when stuck.
3. `docs/HANDOFF.md` §3 (rules not in the code), §6B (residual triage), §6C.
4. `docs/RIDE_CALLBACKS.md` — names ride callbacks by object-definition slot.

## Environment

```
cd /Users/systemadmin/Documents/Development/Github/legoland
git checkout main && git pull --ff-only origin main
PY=/Users/systemadmin/.venvs/legoland/bin/python
export LEGOLAND_CL=/Users/systemadmin/Documents/Development/Github/alphateam/tools/wibo-msvc/cl
```

- Disassemble: `$PY tools/disasm.py original/legoland.exe 0x<RVA> <count>` —
  **RVA = VA − 0x400000** (0x00450f30 → 0x50f30).
- Iterate: `$PY tools/matchfull.py LEGOLAND/<file>.c <Name> 0x<VA> --obj /tmp/fc_<Name>.obj`
- **Authoritative gate:** `$PY tools/audit.py LEGOLAND/<file>.c` — a function
  counts ONLY when it prints `[OK]`.
- `/W3` check: `ALPHATEAM_VC6_ROOT="$PWD/toolchain" "$LEGOLAND_CL" /nologo /c /W3 /O2 /Gy /Gd /Fo/tmp/fc_w3.obj LEGOLAND/<file>.c`
- Objects always under `/tmp`; never `/Zi`.
- **Do NOT run `tools/verify.py`, `tools/progress.py` or `tools/coverage.py`.**
  `audit.py` and `matchfull.py` use per-process object paths and are safe
  alongside other sessions' compiles.
- Known tooling defect: the extent walker stops at an unconditional `jmp` that
  no EARLIER branch crosses, so a function whose middle holds a rotated
  loop's entry `jmp` is under-bounded and reports ESCAPES. If yours matches
  the truncated extent exactly, say so in the WIP note and move on.

## Your files and functions (≈1,600 instructions)

Create these NEW files. Do not edit any existing `.c` file — read them freely.
Anything above 0x0049e000 is the statically linked CRT and is not a target.

### `LEGOLAND/sysmisc2.c` — resources, audio, theme (≈412 insns)

| address | name | insns | declared by |
| --- | --- | --- | --- |
| 0x004928a0 | `StartPlayableSample` | 48 | audio2.c, audio3.c |
| 0x00492ce0 | `SetTheme` | 50 | input.c |
| 0x004965a0 | `SetSampleScreenPos` | 84 | sysmisc.c |
| 0x004510e0 | `RES_FindVolumeOnResPath` | 92 | sysmisc.c |
| 0x00450f30 | `RES_FindVolumeOnAnyDrive` | 138 | sysmisc.c |

Your scope B `sysmisc.c` (`RES_EnsureMounted`, `UpdateSampleSource`) is the
direct context; the two `RES_FindVolume*` are a pair — do one, diff the other.

### `LEGOLAND/workorder4.c` — point-to-point routing and path rects (≈447 insns)

| address | name | insns | declared by |
| --- | --- | --- | --- |
| 0x0045d560 | `IntersectRect4` | 47 | objrect.c |
| 0x0045ca90 | `FindPathRect` | 51 | pathsq.c |
| 0x0045cd70 | `RefreshPathArea` | 56 | pathtile2.c |
| 0x00482240 | `AddPTPOpenNode` | 58 | bnvmove.c |
| 0x00482620 | `PTPVisitTile` | 60 | workorder2.c |
| 0x0049cf00 | `MarkWorkersOnMap` | 62 | printlist.c |
| 0x00482330 | `PTPShortcutSteps` | 113 | workorder3.c |

Your scope B `workorder3.c` (`BuildPTPRoute`, the walk-back) documents the
PTP node layout; `bnvmove.c` and `workorder2.c` hold the rest of the search.
Your scope A `objrect.c` is the caller of `IntersectRect4`.

### `LEGOLAND/savegame2.c` — save/load helpers (≈268 insns)

| address | name | insns | declared by |
| --- | --- | --- | --- |
| 0x00450a80 | `SaveBlock11` | 46 | savegame.c |
| 0x0046c680 | `LoadScriptString` | 50 | savechunks.c, savechunks2.c |
| 0x00482860 | `SavePathRects` | 54 | savegame.c |
| 0x00482920 | `LoadPathRects` | 57 | savegame.c |
| 0x004424e0 | `RecolourModelParts` | 61 | savechunks.c |

`SavePathRects`/`LoadPathRects` are a pair. Your scope B `savechunks2.c` is
the caller of `LoadScriptString` and documents the string framing.

### `LEGOLAND/data3.c` — locale text and textures (≈214 insns)

| address | name | insns | declared by |
| --- | --- | --- | --- |
| 0x004402d0 | `LoadTextFile` | 47 | data2.c |
| 0x0043f990 | `LoadLocSet` | 49 | data2.c |
| 0x00443720 | `LoadLocTextures` | 58 | data2.c |
| 0x004428f0 | `LookupTextureName` | 60 | blokeai.c |

`data2.c` holds the matched siblings and the loc/texture tables.

### `LEGOLAND/mapscreen4.c` — map screens and input (≈264 insns)

| address | name | insns | declared by |
| --- | --- | --- | --- |
| 0x00490350 | `InitScreen8` | 45 | mapscreen.c |
| 0x00490be0 | `ReportHintInput` | 46 | mapscreen2.c |
| 0x00452390 | `BeginMapClick` | 53 | bighelp.c |
| 0x004910f0 | `PrintScreenMode7` | 59 | mapscreen.c |
| 0x00451f70 | `ScrollFromKeys` | 61 | bighelp.c |

`mapscreen.c`, `mapscreen2.c` (`InitScreen7`, `PrintScreenMode8`, exact) and
`mapscreen3.c` (`InitScreen9`, `PrintScreenMode6`, exact) are the templates —
these screens are one shape with per-screen data.

**Order:** `data3.c` → `savegame2.c` → `mapscreen4.c` → `sysmisc2.c` →
`workorder4.c`, smallest first within each file, finishing a file before
starting the next. Save after every function.

## Files you must NOT touch

- Any existing `LEGOLAND/*.c` (read only).
- These new files belong to the other session's lanes running right now:
  `logflume6.c`, `coaster4.c`, `schoolcar5.c`, `goldrush3.c`, `ridemisc2.c`,
  `screencb3.c`, `fpui4.c`.
- `docs/DECOMP.md`, `docs/HANDOFF.md`, `README.md`,
  `docs/LEGOLANDPROGRESS.HTML`, `docs/LEGOLANDPROGRESS.SVG`. **Put your
  levers and findings in `docs/lanes/fable-c.md`** (plus per-file notes if you
  like) in DECOMP's bullet style, each with its evidence.

## Git

`git checkout -b fable/scope-c` from current `main`. Commit to that branch
only; **never commit to `main` and never push `main`.** Push the branch when a
file is complete. End every commit message with:

```
Co-Authored-By: Claude Fable 5.1 <noreply@anthropic.com>
```

## Rules that are not in the code

- **Marker discipline.** `// WIP-FUNCTION: LEGOLAND 0x<VA>  (<pct>, <precise residual>)`
  until `audit.py` prints `[OK]`; then exactly `// FUNCTION: LEGOLAND 0x<VA>`.
  Marker on the line immediately above the signature. **Never commit a
  prologue-only or fabricated-tail body as `// FUNCTION:`.**
- Declare every callee `extern` with a trailing `/* 0x0044xxxx */` comment.
- **Extern prototype TYPES are caller-side codegen levers** — a callee may need
  two or three different prototypes in one file. Never "align" another file's
  declaration; note the divergence. Symbol NAMES are not levers.
- **One name per address.** Placeholder names that collide with a real symbol
  get a distinct name, recorded.
- Reproduce original bugs; comment them at the site; do not fix them.
- Report behaviour you cannot explain from the disassembly rather than
  inventing plausible C.

## Levers that decide whether a first draft lands at 95% or 40%

Your scope A and B lists still apply in full. New this round, all measured on
fresh code:

- **A shared tail that RELOADS locals from the frame** means every arm left
  them in memory: a volatile STORE through a cast in every arm makes the arms
  identical and VC6 cross-jumps them into one tail (85 → 66/66).
- **`xor <callee-saved>,<same> / mov eax,<it>`** proves a two-predecessor
  join; a straight-line `v = 0; return v;` is deleted by global constant
  propagation, every disguise included.
- **The fall-through copy of a shared tail survives; the copy in a jump arm is
  merged away** — regardless of textual position. In a plain if/else chain
  that is the last arm.
- **A `while` loop's condition is whichever test VC6 leaves in the LATCH**; the
  peeled copy doubles as the enclosing `if`. Read the latch off the original.
- **A jump-table `switch`'s case order is its block order; a compare-chain
  switch is laid out by case VALUES** and source order is inert.
- **An address-taken counter turns strength reduction off** (90 of 93 from one
  `&n`). **Two lockstep cursors spelled the same way** let VC6 eliminate an
  induction variable; spelled differently keep both.
- **A push sinks past a leading guard only when the guarded block ends in its
  own `return K`.** A state-copy local must re-test the GLOBAL, not the copy.
- **Float source order is the reverse of `fld` order**; a float swap needs no
  temporary; `((float)a - b) * K` emits `fild/fisub`; a `volatile float` local
  is how an FP constant gets a stack home.
- **VC6 biases a strength-reduced array IV to the MIDDLE of the accessed
  offsets** — read record framing accordingly.
- **The free-volatile floor test is a signal, not a proof**: try naming an
  array element, naming a call result, or splitting a nested call before
  retiring anything on it.

## When a function will not close

Run the §6B triage: strict, register-blind, offset-blind with frame homes by
push depth. `strict >> rb` is allocation — try one free `volatile` read first;
`strict >> ob` is the frame; `strict == rb == ob` is a scheduling permutation;
rb-still-high is structural and reachable. For a self-recursive function run
the four-line ranking probe from your scope A `jcroute.c` notes. Then write the
honest WIP note and move on.

## Report format (in `docs/lanes/fable-c.md`, and in your final message)

Per function: instructions, percentage, `audit [OK]` yes/no, marker as
committed, first diverging index and residual for anything not `[OK]`. Then
mechanics recovered, callees named for the first time, original bugs
reproduced, extern-type divergences, and every lever with its evidence.
