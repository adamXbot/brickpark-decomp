# Parallel session scope — "codex-c" (2026-09-05)

The follow-on to `docs/SCOPE_CODEX_A.md`, which is complete and merged (63 of
64 exact, zero conflicts). Same contract, new files. Everything you need is
here; if you ran scope A, this is the same workflow.

## What this project is

A matching decompilation of LEGOLAND (Windows, 2000, VC6 SP3, `/O2 /Gy /Gd`).
Human-written C in `LEGOLAND/*.c` must compile to reproduce
`original/legoland.exe` function-by-function. No game binary or asset is ever
committed (`original/` and `gamedata/` are gitignored).

**This session writes NEW functions from the unmatched-callee frontier's
small-function tail** — about 95 functions of 1–28 instructions across sim,
paths, the log flume, save helpers, system stubs and the tiniest screen
callbacks. Breadth beats depth: most of these should close on the first or
second compile.

## Read these first (in this order)

1. `docs/LANE_BRIEF.md` — the method per function and **the verification gate**
   (its PROJECT/STATUS lines are stale; use ENVIRONMENT below).
2. `docs/DECOMP.md`, section "VC6 SP3 codegen levers" — the ~260 entries at the
   TOP are the newest; your scope A levers will be folded in there. The
   `fable-a`/`fable-b` blocks (routes, object rects, work orders, system) and
   the log-flume entries are your closest.
3. `docs/HANDOFF.md` §3 (rules not in the code) and §6B (residual triage), and
   `docs/RIDE_CALLBACKS.md` (callback slot meanings).

## Environment

```
cd /Users/systemadmin/Documents/Development/Github/legoland
git checkout main && git pull --ff-only origin main
PY=/Users/systemadmin/.venvs/legoland/bin/python
export LEGOLAND_CL=/Users/systemadmin/Documents/Development/Github/alphateam/tools/wibo-msvc/cl
```

If either path does not exist, **stop and report** — do not build a toolchain.

- Disassemble: `$PY tools/disasm.py original/legoland.exe 0x<RVA> <count>` —
  **RVA = VA − 0x400000**.
- Iterate: `$PY tools/matchfull.py LEGOLAND/<file>.c <Name> 0x<VA> --obj /tmp/cc_<Name>.obj`
- **Authoritative gate:** `$PY tools/audit.py LEGOLAND/<file>.c` — a function
  counts ONLY when it prints `[OK]`. Trust `audit.py` over `matchfull.py`.
- `/W3` check: `ALPHATEAM_VC6_ROOT="$PWD/toolchain" "$LEGOLAND_CL" /nologo /c /W3 /O2 /Gy /Gd /Fo/tmp/cc_w3.obj LEGOLAND/<file>.c`
- Objects always under `/tmp`; never `/Zi`.
- **Do NOT run `tools/verify.py`, `tools/progress.py` or `tools/coverage.py`**,
  and do not edit anything under `tools/`. `audit.py`/`matchfull.py` use
  per-process object paths and are safe alongside other sessions' compiles.

## Your files and functions (≈1,090 instructions, ~95 functions)

Create these NEW files. Do not edit any existing `.c` file — read them freely.
Anything above 0x0049e000 is the statically linked CRT and is not a target.
Functions of 1–5 instructions are usually stubs or tail-jump wrappers: a
`void` tail-`jmp` wrapper is marked
`// WIP-FUNCTION: LEGOLAND 0x<VA>  (100% by audit.py; tail-jmp)` only if
`audit.py` cannot bound it — otherwise they close like anything else.

### `LEGOLAND/simcore2.c` — routing and visitor mood (≈173 insns)

| address | name | insns | declared by |
| --- | --- | --- | --- |
| 0x004776c0 | `AddClosedNode` | 5 | simcore.c |
| 0x00477980 | `RouteTurnCost` | 8 | simcore.c |
| 0x004700c0 | `IsWatchedBloke` | 10 | bnvmove.c |
| 0x00482d30 | `GetBlokeMood` | 16 | popup.c |
| 0x0044eae0 | `UpdateBlokeStay` | 17 | bnvmove.c |
| 0x004779a0 | `RouteStepAxis` | 18 | simcore.c |
| 0x00477790 | `RemoveOpenNode` | 19 | simcore.c |
| 0x00477680 | `RouteInBounds` | 25 | simcore.c |
| 0x00482df0 | `AdjustMood` | 27 | bnvmove.c, simcore.c |
| 0x0044ea50 | `SpawnVisitor` | 28 | bnvmove.c |

`simcore.c` (`RequestRoute`, a retired 3-of-482 partial with an alias-kill
proof in its note — read it), `objrect.c` (`GetRouteNode`, exact) and
`bnvmove.c` are the context. `AddOpenNode` (0x004776e0) belongs to a running
lane — do not write it.

### `LEGOLAND/pathmisc.c` — path rects, tiles and work-order spans (≈220 insns)

| address | name | insns | declared by |
| --- | --- | --- | --- |
| 0x004821c0 | `ClearPTPVisited` | 7 | bnvmove.c, workorder2.c |
| 0x0045e690 | `ObjHasExit` | 11 | bigrender.c, objrect.c |
| 0x0045ce10 | `IsPathCell` | 12 | pathtile2.c |
| 0x004139c0 | `Road_TileClaim` | 12 | ridecb8.c |
| 0x00482300 | `AddPTPRouteNode` | 13 | workorder3.c |
| 0x00450c00 | `FreeBuildSlotAt` | 16 | mappath.c |
| 0x004821e0 | `FreePTPOpenList` | 16 | bnvmove.c, workorder2.c |
| 0x00413990 | `Road_TileCost` | 18 | ridecb8.c |
| 0x00489f00 | `MarkObjectTiles` | 18 | mapobj.c |
| 0x0045d730 | `AddObjRectSpan` | 21 | workorder2.c |
| 0x0045cd00 | `GrowPathRect` | 23 | pathsq.c |
| 0x0045eaf0 | `ClassNeedsPath` | 25 | objmap2.c, popup.c |
| 0x00499720 | `SettleOrderSpan` | 28 | bigsim.c, workorder2.c |

`mappath.c` (its header documents `FreeBuildSlotAt` from the caller's side),
`objrect.c` (`SubtractObjRect`, `RemoveObjectPathTiles`), `workorder3.c`
(`BuildPTPRoute`, the PTP node layout), `pathsq.c`, `pathtile2.c`, `roads2.c`
(`NewRoadRecord` clears sixteen tiles through `Set_UserFlags`) and `goldrush4.c`
(`SchoolCarMayEnterSquare` — the consumer of `Road_TileClaim`'s counter) are
the context. `ObjHasExit` is the sibling of `ObjHasEntrance`/`GetObjExitDir`,
which a running lane owns in `objdoor.c` — read that file if it exists yet,
but do not write those.

### `LEGOLAND/lfmisc.c` — log flume, boats and rider helpers (≈238 insns)

| address | name | insns | declared by |
| --- | --- | --- | --- |
| 0x00412290 | `FreeWalkPath` | 7 | goldrush.c, logflume.c, mechrides.c |
| 0x004122d0 | `WalkPath_Board` | 9 | goldrush.c, mechrides.c |
| 0x00412470 | `LFAnim_FromId` | 11 | logflume.c, logflume5.c |
| 0x0042d540 | `NthRiderNode` | 11 | screencb.c |
| 0x004123a0 | `LFAnim_SaveId` | 12 | logflume.c, logflume6.c |
| 0x00411bd0 | `Pump_FreeAll` | 13 | screencb2.c |
| 0x0043f870 | `Free3DPerson` | 13 | workers.c |
| 0x004333b0 | `JcBoat_Depart` | 14 | junglecruise.c |
| 0x00411290 | `LFQuadTopLeft` | 15 | logflume4.c |
| 0x004117e0 | `LFBoat_DropStep` | 15 | logflume6.c |
| 0x00411650 | `Sub_411650` | 15 | logflume5.c |
| 0x00411e30 | `LFQueue_Append` | 16 | lfentrance.c |
| 0x00411220 | `LFQuadTopRight` | 17 | logflume4.c |
| 0x0040ca30 | `LFDrawBoatList` | 21 | logflume4.c |
| 0x00482c60 | `InitBlokeName` | 22 | rides.c |
| 0x00418f90 | `BsBoat_Destroy` | 27 | bswater2.c, ridecb8.c, screencb2.c |

The log flume is the best-documented subsystem in the tree: `logflume.c`
through `logflume7.c` (all recent, most exact) document every one of these
from the callers' side — `LFBoat_DropStep` and `Sub_411650` from `logflume6.c`
(`LFBoat_Fall`/`LFBoat_Advance`), the two `LFQuad*` and `LFDrawBoatList` from
`logflume4.c`, `LFQueue_Append` from `lfentrance.c`. `Sub_411650` is unnamed —
name it from what it does. `mappath.c`'s `BuildWalkPath` documents the
walk-path node layout for `FreeWalkPath`/`WalkPath_Board`.

### `LEGOLAND/sysstubs.c` — save helpers, system stubs, math (≈380 insns)

| address | name | insns | declared by |
| --- | --- | --- | --- |
| 0x00499450 | `GetTicks` | 1 | many |
| 0x0047f850 | `DebugFlush` | 1 | data2.c, sysmisc.c |
| 0x0047f870 | `DebugPrintf` | 1 | many |
| 0x004688e0 | `sub_4688e0` | 1 | savechunks.c |
| 0x00443710 | `GetModelContext` | 2 | data2.c |
| 0x00443250 | `Sin` | 3 | math3d.c |
| 0x00443260 | `Cos` | 3 | math3d.c |
| 0x00458bb0 | `sub_458bb0` | 3 | savegame.c |
| 0x00474880 | `sub_474880` | 3 | savegame.c |
| 0x00474070 | `IsLShiftDown` | 4 | input2.c |
| 0x00474080 | `IsRShiftDown` | 4 | input2.c |
| 0x00468840 | `sub_468840` | 5 | savechunks.c |
| 0x00492110 | `MakePlayable` | 5 | audio3.c, sweep4.c |
| 0x00492d80 | `StopMusic` | 5 | input.c |
| 0x0047fe70 | `WNDENV_Minimise` | 5 | sysmisc.c |
| 0x0047fe80 | `WNDENV_Restore` | 5 | sysmisc.c |
| 0x00499460 | `GetSimClock` | 7 | scrolltick.c |
| 0x00488820 | `GetZBufferPixel` | 8 | gpu.c |
| 0x00497580 | `NewSprite` | 9 | sprite2.c |
| 0x00453ce0 | `DBError` | 10 | savegame.c |
| 0x00475f10 | `sub_475f10` | 10 | savegame.c |
| 0x00498900 | `SetSpeechVolume` | 10 | audio3.c |
| 0x00442de0 | `DotProduct` | 11 | math3d.c |
| 0x00474190 | `sub_474190` | 11 | savegame.c |
| 0x004741c0 | `sub_4741c0` | 11 | savegame.c |
| 0x00473a60 | `KillMouseDevice` | 11 | lifecycle.c |
| 0x00499380 | `FreezeGameClock` | 12 | input2.c, mapscreen2.c |
| 0x0049a120 | `CanHireGardener` | 14 | fpui2.c |
| 0x0049a160 | `CanHireMechanic` | 14 | fpui2.c |
| 0x00457910 | `SaveCurrency` | 15 | savegame.c |
| 0x00457940 | `LoadCurrency` | 15 | savegame.c |
| 0x0046b760 | `sub_46b760` | 15 | savegame.c |
| 0x00468940 | `FreeScriptEvent` | 15 | fpui3.c, fpui5.c, savechunks2.c |
| 0x00468910 | `NewScriptEvent` | 13 | savechunks2.c |
| 0x004920e0 | `NewSampleRecord` | 16 | data2.c |
| 0x00495a10 | `InitMusicSystem` | 16 | lifecycle.c |
| 0x0046ce20 | `KillAdvisorHelp` | 17 | fpui3.c, iconui.c |
| 0x00442da0 | `CrossProduct` | 22 | math3d.c |
| 0x00456770 | `HalfPos` | 22 | renderview.c |
| 0x00468bb0 | `AddHelpMessage` | 24 | softblit.c |
| 0x004887a0 | `InitZBuffer` | 25 | gpu.c |
| 0x0048af40 | `FreePlayItemAdd` | 27 | fpui3.c |

Name the `sub_*` from what they do (one name per address). `savegame.c`,
`savechunks.c`/`savechunks2.c` (the `ScriptEvent` list and framing), `sysmisc.c`
(`KillMusicSystem`, `CreateMouseDevice`, `FlipPrimary`), `math3d.c`, `gpu.c`,
`fpui3.c`/`fpui5.c` are the context. `FreezeGameClock`/`ThawGameClock` and
`CanHireGardener`/`CanHireMechanic` are pairs; `ThawGameClock` belongs to the
codex-b session — read its `uimisc.c` if it exists, do not write it.

### `LEGOLAND/screencb7.c` — the tiniest screen callbacks (≈45 insns)

| address | name | insns |
| --- | --- | --- |
| 0x00436190 | `CB_436190` | 4 |
| 0x00433cd0 | `CB_433cd0` | 5 |
| 0x004340b0 | `CB_4340b0` | 5 |
| 0x00433fa0 | `CB_433fa0` | 7 |
| 0x00434650 | `CB_434650` | 7 |
| 0x00452ab0 | `PowerStation_AC` | 9 |
| 0x00452b70 | `Dino_InitSound` | 11 |

Name each `CB_*` from `screen.c`'s `SetCustomCallbacks` (0x00452c20): the
class arm that stores the pointer and the ObjDef slot it goes into (8c
select-for-placement, 90 update, 94 draw-selection, 98 add, 9c remove, a0
draw-desc, a4 create, a8 tick, ac destroy, b0 draw, b8 load, bc save), following
`screencb.c`..`screencb5.c`. The codex-b session owns `screencb6.c` with the
9–28-instruction callbacks; these are the ones below that.

**Order:** `screencb7.c` → `lfmisc.c` → `simcore2.c` → `pathmisc.c` →
`sysstubs.c`, smallest first within each file, finishing a file before
starting the next. Save after every function.

## Files you must NOT touch

- Any existing `LEGOLAND/*.c` (read only), and anything under `tools/`.
- New files belonging to lanes running right now: `render4.c`, `bswater3.c`,
  `coaster6.c`, `schoolcar7.c`, `screencb5.c`, `ridemisc3.c`, `workers3.c`,
  `objdoor.c`, `posstep.c`; and to other parallel sessions: `sysmisc2.c`,
  `workorder4.c`, `savegame2.c`, `data3.c`, `mapscreen4.c` (scope C),
  `screencb6.c`, `audio4.c`, `uimisc.c` (codex-b).
- `docs/DECOMP.md`, `docs/HANDOFF.md`, `README.md`,
  `docs/LEGOLANDPROGRESS.HTML`, `docs/LEGOLANDPROGRESS.SVG`. **Put your levers
  and findings in `docs/lanes/codex-c.md`** in DECOMP's bullet style, each
  with its evidence.

## Git

`git checkout -b codex/scope-c` from current `main` (which now contains your
scope A work, merged). Commit to that branch only; **never commit to `main`
and never push `main`.** Push the branch when a file is complete.

## Rules that are not in the code

- **Marker discipline.** `// WIP-FUNCTION: LEGOLAND 0x<VA>  (<pct>, <precise residual>)`
  until `audit.py` prints `[OK]`; then exactly `// FUNCTION: LEGOLAND 0x<VA>`.
  Marker on the line immediately above the signature. **Never commit a
  prologue-only or fabricated-tail body as `// FUNCTION:`.**
- Declare every callee `extern` with a trailing `/* 0x0044xxxx */` comment.
- **Extern prototype TYPES are caller-side codegen levers** — a callee may need
  different prototypes in different files; never "align" another file's
  declaration, note the divergence. Symbol NAMES are not levers.
- **One name per address.** Placeholder names that collide with a real symbol
  get a distinct name, recorded.
- Reproduce original bugs; comment them at the site; do not fix them.
- Report behaviour you cannot explain from the disassembly rather than
  inventing plausible C. An honest WIP beats a wrong body that compiles close.

## Levers that matter most on tiny functions

- **Layout first:** an else arm is exiled past the fall-through trace iff it
  ends in an unconditional jump; `if (a == 0 || b != c) return X;` exiles X
  (split the test and jump INTO the second `if`'s block to inline it; MERGE
  separate `goto` guards to exile a shared block); two `return K` sites merge
  at the FIRST; the fall-through copy of a shared tail survives; a push sinks
  past a leading guard only when the guarded block ends in ONE `return K`; a
  jump-table `switch`'s case order is its block order, a compare-chain switch
  is laid out by case VALUES, and cases sharing a body lower to a RANGE chain;
  VC6 always jump-threads a provably-NULL pointer into a following `if (p)`.
- **Lists:** a search's failure arm as the `else` of `if (p)` keeps ONE copy;
  a `while`'s condition is whichever test VC6 leaves in the LATCH; walk a
  list through its head GLOBAL for the 5-byte accumulator store; the order of
  `p->next = head;` against a neighbouring store decides where the head LOAD
  lands; the record-unlink shape is one source compiled eight times in this
  tree (`ridemisc3.c`, `bswater2.c`) — reuse it, null-head bug included.
- **Values:** a two-term sum's destination register follows the destination
  SYMBOL; a `Pos` by value is three levers in one; an 8-byte struct RETURN must
  be the accumulator of any sum added to it; `n - i - 1` and `n - 1 - i` differ;
  `tw <<= 1` and `tw = tw * 2` differ; `x += -t*8`, `x -= t*8` and
  `x += (-t)<<3` are byte-identical (do not sweep); an explicit `& 0xff` and a
  `(unsigned char)` cast are different objects; `test byte ptr [mem],1` on byte
  +3 of a word is `& 0x100` on the `unsigned short`; `if (param == arr[i])` vs
  `if (arr[i] == param)` decides `cmp reg,mem` vs `cmp mem,reg`.
- **Reads:** read a global directly at every use; a free `volatile` read goes
  at the DEFINITION site (it cannot cross the callee-saved pushes); naming the
  intermediate POINTER advances the eax→ecx→edx rotation; check the STORE
  width for u16 stashes; `movsx`/`movzx` say signed/unsigned byte.
- **Floats:** float source order is the reverse of `fld` order; float
  constants are exact single-precision literals; `Sin`/`Cos` at 3 instructions
  are almost certainly tail-jumps into the CRT — audit decides.
- **`__asm`:** operand names collide with MASM reserved words (`cr0`-`cr4`,
  `dr0`-`dr7`, `tr3`-`tr7`, `st`).

## When a function will not close

Run the §6B triage: strict, register-blind, offset-blind with frame homes by
push depth. `strict >> rb` is allocation — try one free `volatile` read, then
the one-temporary spellings; `strict >> ob` is the frame; `strict == rb == ob`
is a scheduling permutation — sweep the store order. Then write the honest WIP
note and move on; with ninety-odd functions, breadth beats depth.

## Report format (in `docs/lanes/codex-c.md`, and in your final message)

Per function: address, name (and why, for anything you named), instructions,
percentage, `audit [OK]` yes/no, marker as committed, first diverging index
and residual for anything not `[OK]`. Then mechanics recovered, original bugs
reproduced, extern-type divergences, and every lever with its evidence.
