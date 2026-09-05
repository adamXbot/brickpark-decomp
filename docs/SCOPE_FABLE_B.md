# Parallel session scope — "fable-b" (2026-09-05)

Second self-contained brief for a Claude Code session working **in parallel**
with the integrating session. Scope A (`docs/SCOPE_FABLE_A.md`) was completed
and merged — 12 of 14 exact, zero conflicts — so this follows the same shape
exactly. Everything you need is here; you do not need the other session's
context. If you ran scope A, this is the same workflow on new files.

## What this project is

A matching decompilation of LEGOLAND (Windows, 2000, VC6 SP3, `/O2 /Gy /Gd`).
Human-written C in `LEGOLAND/*.c` must compile to reproduce
`original/legoland.exe` function-by-function. Status at hand-off: **1592 exact
functions, 50 partials, 47.9% of game code matched exactly**. No game binary or
asset is ever committed (`original/` and `gamedata/` are gitignored).

**This session writes NEW functions from the unmatched-callee frontier.** The
last two rounds of this closed 10 and 38 functions; the four rounds before
them, grinding existing partials, closed none.

## Read these first (in this order)

1. `docs/LANE_BRIEF.md` — the method per function and **the verification gate**
   (its PROJECT/STATUS lines are stale; use ENVIRONMENT below).
2. `docs/DECOMP.md`, section "VC6 SP3 codegen levers" — the ~90 entries at the
   TOP are the newest (the last two rounds' findings, including your scope A
   levers folded in) and several correct older ones. Search the rest when stuck.
3. `docs/HANDOFF.md` §3 (rules not in the code), §6B (residual triage), §6C.
4. `docs/RIDE_CALLBACKS.md` — names ride callbacks by object-definition slot.

## Environment

```
cd /Users/systemadmin/Documents/Development/Github/legoland
git pull --ff-only origin main        # start from current main
PY=/Users/systemadmin/.venvs/legoland/bin/python
export LEGOLAND_CL=/Users/systemadmin/Documents/Development/Github/alphateam/tools/wibo-msvc/cl
```

- Disassemble: `$PY tools/disasm.py original/legoland.exe 0x<RVA> <count>` —
  **RVA = VA − 0x400000** (0x0046c7e0 → 0x6c7e0).
- Iterate: `$PY tools/matchfull.py LEGOLAND/<file>.c <Name> 0x<VA> --obj /tmp/fb_<Name>.obj`
- **Authoritative gate:** `$PY tools/audit.py LEGOLAND/<file>.c` — a function
  counts ONLY when it prints `[OK]`.
- `/W3` check: `ALPHATEAM_VC6_ROOT="$PWD/toolchain" "$LEGOLAND_CL" /nologo /c /W3 /O2 /Gy /Gd /Fo/tmp/fb_w3.obj LEGOLAND/<file>.c`
- Objects always under `/tmp`; never `/Zi`.
- **Do NOT run `tools/verify.py`, `tools/progress.py` or `tools/coverage.py`.**
  The integrating session runs those alone on a quiet tree. `audit.py` and
  `matchfull.py` use per-process object paths and are safe alongside other
  sessions' compiles.

## Your files and functions (≈1,450 instructions)

Create these NEW files. Do not edit any existing `.c` file — read them freely.
CRT functions (`realloc`, `tolower`, `toupper`, `NameCompare` at 0x004aab90 —
anything above 0x0049e000) are statically linked C runtime and are NOT targets.

### `LEGOLAND/savechunks2.c` — script events (≈201 insns)

| address | name | insns | declared by |
| --- | --- | --- | --- |
| 0x0046c700 | `SaveScriptEvent` | 77 | savechunks.c |
| 0x0046c7e0 | `LoadScriptEvent` | 124 | savechunks.c |

`LEGOLAND/savechunks.c` and `savegame.c` hold the chunk framework and the
sibling save/load pairs — read a matched pair first; save/load twins share
their record layout and usually their loop shape.

### `LEGOLAND/ridemisc.c` — small ride helpers (≈426 insns)

| address | name | insns | declared by |
| --- | --- | --- | --- |
| 0x0043d7c0 | `MechanicsHut_EvictRiders` | 64 | ridecb9.c |
| 0x004333e0 | `JcBoat_Advance` | 69 | junglecruise.c |
| 0x0044e890 | `RandomFavouriteFood` | 70 | rides.c |
| 0x0044e790 | `RandomFavouriteRide` | 73 | rides.c |
| 0x004048b0 | `Copters_SetFull` | 75 | mechrides.c |
| 0x0042cf70 | `EarthSlide_LaunchCar` | 75 | ridecb1.c |

`RandomFavouriteRide`/`RandomFavouriteFood` are an obvious pair — do one, diff
the other's disassembly against it. `rides.c`, `mechrides.c`, `junglecruise.c`
and `ridecb1.c`/`ridecb9.c` are the context (your scope A `bswater.c` and
`jcroute.c` are the closest recent analogues).

### `LEGOLAND/workorder3.c` — work orders and print list (≈221 insns)

| address | name | insns | declared by |
| --- | --- | --- | --- |
| 0x00499d60 | `UnlinkGardenerOrder` | 68 | workorder2.c |
| 0x00482430 | `BuildPTPRoute` | 76 | bnvmove.c, workorder2.c |
| 0x00485bd0 | `InsertPrintItem` | 77 | blokeai.c, printlist.c |

`workorder.c`, `workorder2.c`, `printlist.c` and your scope A `objrect.c`
(`ClearCellForPath`'s gardener/mechanic queue handling) are the context.
`InsertChildIntoList` (fpui.c) is a matched linked-list insert of the same
family as `InsertPrintItem`.

### `LEGOLAND/sysmisc.c` — display, resources, audio, input (≈600 insns)

| address | name | insns | declared by |
| --- | --- | --- | --- |
| 0x004966a0 | `UpdateSampleSource` | 66 | audio2.c, audio3.c |
| 0x00473970 | `CreateMouseDevice` | 66 | input2.c |
| 0x00463ef0 | `SetScreenDisplayMode` | 72 | screen.c |
| 0x004401b0 | `UpdatePersonPos` | 74 | blokeai.c, blokemisc.c |
| 0x004515e0 | `RES_EnsureMounted` | 92 | res.c |
| 0x004661d0 | `FlipPrimary` | 111 | screen.c |
| 0x0047c6a0 | `LLIDB_UnLoadLLSData` | 119 | fpui2.c, memdb.c |

Mixed subsystems, each self-contained: `audio2.c`/`audio3.c`, `input.c`/
`input2.c`, `screen.c`/`surface.c`/`wndenv.c` (DirectDraw), `blokemisc.c`,
`res.c`, `llidb*.c`/`memdb.c`. Expect COM/DirectDraw vtable calls in
`FlipPrimary`/`SetScreenDisplayMode`/`CreateMouseDevice` — `surface.c` and
`wndenv.c` show how matched code spells them.

**Order:** `savechunks2.c` → `workorder3.c` → `ridemisc.c` → `sysmisc.c`,
smallest first within each file, finishing a file before starting the next.
Save after every function.

## Files you must NOT touch

- Any existing `LEGOLAND/*.c` (read only).
- These new files belong to the other session's lanes running right now:
  `coaster3d.c`, `schoolcar3.c`, `screencb2.c`, `schoolcar4.c`, `logflume5.c`,
  `fpui3.c`, `mapscreen3.c`.
- `docs/DECOMP.md`, `docs/HANDOFF.md`, `README.md`,
  `docs/LEGOLANDPROGRESS.HTML`, `docs/LEGOLANDPROGRESS.SVG`. **Put your
  levers and findings in `docs/lanes/fable-b.md`** (plus per-file notes if you
  like, as scope A did) in DECOMP's bullet style, each with its evidence.

## Git

`git checkout -b fable/scope-b` from current `main`. Commit to that branch
only, as often as you like; **never commit to `main` and never push `main`.**
Push the branch when a file is complete. End every commit message with:

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
  two or three different prototypes in one file (`screencb.c` declares
  0x0045f220 three ways). Never "align" another file's declaration; note the
  divergence instead. Symbol NAMES are not levers.
- **One name per address.** If a placeholder name collides with a real symbol,
  give the non-exported one a distinct name and record it.
- Reproduce original bugs; comment them at the site; do not fix them.
- Report behaviour you cannot explain from the disassembly rather than
  inventing plausible C. An honest WIP beats a wrong body that compiles close.

## Levers that decide whether a first draft lands at 95% or 40%

The scope A list still applies (block exile as an iff, short-circuit return
exile and its inverse, first-site merge of same-constant returns, switch case
order, loop direction, store width for u16 stashes, `movsx`/`movzx`, the
shared two-byte aggregate, direct global reads, free volatile at the
DEFINITION site, canonicalised sums, x87 depth, MASM reserved words). New since
then, all measured on fresh code this round:

- **A by-value struct's field ASSIGNMENT order is its register rotation** —
  a `WinRect` written top/bottom/left/right, not left/top/right/bottom
  (118 → 149 of 149 from that alone).
- **A pending cdecl `add esp` cannot cross a branch join:** if one `add esp`
  cleans calls on both sides of a join, the source had the calls in BOTH arms
  — duplicate the tail and let VC6 cross-jump it.
- **Zero a struct payload with `memset`**, not field-by-field, or the zeros
  merge into a function-wide web that steals a callee-saved register.
- **A partial aggregate initialiser is a placement lever:** `Ctx c = { 1 };`
  sinks the `1` with the call and pins the zero-fill at the top.
- **Appending into a global array:** fill a scalarised local and assign
  `arr[n] = t;` — a pointer kills an address-taken local's CSEs, direct
  subscripts fold the base into each store; only the struct assignment does
  both the original's way.
- **Struct-copy forward-propagation reads the FIRST field back from the
  source**, so the field updates after `dst = kConst;` must be ordered.
- **`tw <<= 1` and `tw = tw * 2` are different objects** (`shl` in place vs
  `lea` into a fresh register). Two derived values are often emitted in
  REVERSE source order. Adjacent address stores come out reversed. Emitted
  store order is not evidence of source order — measure both.
- **A volatile STORE through a cast** (`*(T* volatile*)&x = e;`) forces the
  home store without making reads volatile. **Scope**, not declaration order,
  moves an address-taken aggregate down the frame.
- **Twins share their residual index for index** — when two functions have the
  same size, diff their disassemblies before writing the second.

## When a function will not close

Run the §6B triage: strict, register-blind, offset-blind with frame homes by
push depth. `strict >> rb` is allocation — **try one free `volatile` read
before calling it a floor** (that closed a strict-14 / rb-0 residual in your
scope A `SubtractObjRect`); `strict >> ob` is the frame; `strict == rb == ob`
is a pure scheduling permutation; rb-still-high is structural and reachable.
For a self-recursive function, run the four-line ranking probe from your
scope A `jcroute.c` notes first. Then write the honest WIP note and move on.

## Report format (in `docs/lanes/fable-b.md`, and in your final message)

Per function: instructions, percentage, `audit [OK]` yes/no, marker as
committed, first diverging index and residual for anything not `[OK]`. Then
mechanics recovered, callees named for the first time, original bugs
reproduced, extern-type divergences, and every lever with its evidence.
