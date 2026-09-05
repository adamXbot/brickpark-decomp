# Parallel session scope — "codex-d" (2026-09-05)

The follow-on to `docs/SCOPE_CODEX_C.md` (88 of 88 exact, merged). Same
contract, new files. If you ran scope A or C, this is the same workflow.

## What this project is

A matching decompilation of LEGOLAND (Windows, 2000, VC6 SP3, `/O2 /Gy /Gd`).
Human-written C in `LEGOLAND/*.c` must compile to reproduce
`original/legoland.exe` function-by-function. Status at hand-off: **2020 exact
functions, 74 partials, 55.5% of game code matched exactly**; the frontier is
316 functions. No game binary or asset is ever committed.

**This session writes NEW functions from the frontier**: the ride-record
family (seat, add, find, new, free — one shape per verb across a dozen ride
classes) and the remaining path/work-order helpers. About 60 functions,
~1,250 instructions, almost all 16–28 instructions each.

## Read these first (in this order)

1. `docs/LANE_BRIEF.md` — the method per function and **the verification gate**
   (its PROJECT/STATUS lines are stale; use ENVIRONMENT below).
2. `docs/DECOMP.md`, section "VC6 SP3 codegen levers" — the ~320 entries at the
   TOP are the newest (your scope A and C levers folded in). The ride-record
   entries are your closest: the unlink is ONE source compiled eight times,
   the +0xbc save is one source across four classes, etc.
3. `docs/HANDOFF.md` §3 (rules not in the code) and §6B (residual triage), and
   `docs/RIDE_CALLBACKS.md`.

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
- Iterate: `$PY tools/matchfull.py LEGOLAND/<file>.c <Name> 0x<VA> --obj /tmp/cd_<Name>.obj`
- **Authoritative gate:** `$PY tools/audit.py LEGOLAND/<file>.c` — a function
  counts ONLY when it prints `[OK]`. Trust `audit.py` over `matchfull.py`.
- `/W3` check: `ALPHATEAM_VC6_ROOT="$PWD/toolchain" "$LEGOLAND_CL" /nologo /c /W3 /O2 /Gy /Gd /Fo/tmp/cd_w3.obj LEGOLAND/<file>.c`
- Objects always under `/tmp`; never `/Zi`.
- **Do NOT run `tools/verify.py`, `tools/progress.py` or `tools/coverage.py`**,
  and do not edit anything under `tools/`.

## Your files and functions (≈1,250 instructions)

Create these NEW files. Do not edit any existing `.c` file — read them freely.

### `LEGOLAND/ridemisc4.c` — the ride-record family (≈700 insns, ~35 functions)

Group these by VERB before writing anything — each verb is one shape across
its classes (the recorded unlink/remove family was one source compiled eight
times; expect the same here). Disassemble all of a verb's members, build one
body carefully, then diff each sibling against it.

| verb | address | name | insns | declared by |
| --- | --- | --- | --- | --- |
| seat-of | 0x00415760 | `SafariRide_SeatOf` | 28 | mechrides.c |
| seat-of | 0x0043e050 | `PlaneRide_SeatOf` | 25 | mechrides.c |
| seat-of | 0x0043ce10 | `SpinningBarrels_SeatOf` | 25 | mechrides.c |
| seat-of | 0x00416830 | `SpiderRide_SeatOf` | 25 | mechrides.c |
| seat-of | 0x00404f20 | `Copters_CopterOf` | 23 | mechrides.c |
| free-rec | 0x0042fa00 | `Restaurant2_FreeRec` | 25 | ridecb8.c |
| free-rec | 0x0042ef70 | `Restaurant1_FreeRec` | 25 | screencb5.c |
| free-rec | 0x0042bc00 | `Carousel_FreeRec` | 25 | screencb5.c |
| free-rec | 0x0042a9b0 | `Balloonz_FreeRec` | 25 | ridecb8.c |
| add-rec | 0x0043bdb0 | `SpinningBarrels_AddRecord` | 23 | mechrides.c |
| add-rec | 0x0043d880 | `PlaneRide_AddRecord` | 22 | mechrides.c |
| add-rec | 0x0043ab70 | `SpaceTower_AddRecord` | 22 | mechrides.c |
| add-rec | 0x004158f0 | `SpiderRide_AddRecord` | 22 | mechrides.c |
| add-rec | 0x004149c0 | `SafariRide_AddRecord` | 22 | mechrides.c |
| add-rec | 0x00403c40 | `Copters_AddRecord` | 22 | mechrides.c |
| new-rec | 0x0042cd70 | `EarthSlide_NewRecord` | 28 | ridecb8.c |
| new-rec | 0x0042eec0 | `Restaurant1_NewRecord` | 22 | ridecb8.c |
| new-rec | 0x0042bbc0 | `Carousel_NewRecord` | 22 | ridecb8.c |
| new-rec | 0x00406920 | `GoldRush_NewRecord` | 19 | goldrush.c |
| find-rec | 0x0043be40 | `SpinningBarrels_FindRecord` | 18 | mechrides.c |
| find-rec | 0x0042f9d0 | `Restaurant2_FindRec` | 18 | goldrush.c, ridecb3.c, ridecb4.c, ridecb8.c |
| find-rec | 0x0042ef40 | `Restaurant1_FindRec` | 18 | ridecb1.c, screencb5.c |
| find-rec | 0x0043d960 | `PlaneRide_FindRecord` | 16 | mechrides.c |
| find-rec | 0x0043ac40 | `SpaceTower_FindRecord` | 16 | mechrides.c |
| set-full | 0x0043d990 | `PlaneRide_SetFull` | 27 | mechrides.c |
| set-full | 0x00414ab0 | `SafariRide_SetFull` | 27 | mechrides.c |
| misc | 0x0043be00 | `SpinningBarrels_RemoveRecord` | 25 | mechrides.c |
| misc | 0x0042ce50 | `EarthSlide_JoinQueue` | 24 | ridecb1.c |
| misc | 0x0043ac70 | `SpaceTower_TakeSeat` | 19 | mechrides.c |
| misc | 0x0042fb60 | `Restaurant2_StopSound` | 21 | ridecb3.c |
| misc | 0x00411ba0 | `Pump_RemoveAllForSchool` | 18 | screencb.c |
| misc | 0x00436130 | `JungleCruise_AddValue` | 17 | junglecruise.c, ridecb7.c, screencb5.c |
| misc | 0x00406ec0 | `GoldRush_ClaimPan` | 20 | goldrush.c |

Templates: `ridemisc3.c` (the eight-wide unlink, `Balloonz_NewRecord`,
`Carousel_StopRide`, `Restaurant2_SeatCustomer`), `bswater2.c`
(`Restaurant2_NewRecord`, `PlaneRide_RemoveRecord`), `roads2.c`
(`NewRoadRecord`), `bswater3.c` (`SpaceTower_RemoveRecord`,
`SpaceTower_CountSeated`, the `TowerRec` car slots), `mechrides.c`,
`ridemisc.c` (`Copters_SetFull`), `ridecb1.c`/`ridecb3.c`/`ridecb8.c`.

### `LEGOLAND/pathmisc2.c` — path rects, order spans and route nodes (≈554 insns)

| address | name | insns | declared by |
| --- | --- | --- | --- |
| 0x00482210 | `FreePTPRouteList` | 16 | bnvmove.c, workorder2.c |
| 0x00489f50 | `UnmarkObjectTiles` | 17 | objmap2.c |
| 0x004837a0 | `RndWalk_LeftTile` | 18 | tilehelp.c |
| 0x00477760 | `RemoveClosedNode` | 19 | simcore.c |
| 0x004995d0 | `NewMechanicOrder` | 19 | workorder2.c |
| 0x004777c0 | `FindOpenRouteNode` | 20 | objrect.c |
| 0x00477730 | `FindClosedRouteNode` | 20 | objrect.c |
| 0x00482b60 | `TileJoinsPathNetwork` | 22 | objrect.c |
| 0x004817d0 | `FindPathSquareAt` | 26 | bnvmove.c |
| 0x0049b270 | `EraseWorkOrdersAt` | 27 | objmap2.c |
| 0x00499570 | `NewGardenerOrder` | 28 | workorder2.c |
| 0x0045ce30 | `PathTileIsPatterned` | 47 | workorder4.c |
| 0x004996a0 | `IsOrderSpanClear` | 48 | pathmisc.c |
| 0x00499620 | `AdvanceOrderSpan` | 54 | pathmisc.c |
| 0x0045c9c0 | `ScanPathArea5x5` | 75 | workorder4.c |
| 0x0045cbc0 | `GrowPathRectSide` | 98 | pathmisc.c |

Your own `pathmisc.c` (`GrowPathRect`, `SettleOrderSpan`, `AddObjRectSpan`)
and `simcore2.c` (`RemoveOpenNode`, `AddClosedNode`) are the callers of most
of these; `workorder4.c` (`FindPathRect`, `RefreshPathArea` — the 3x3 plaza
rect, and the 192x192-bit PTP visited map), `workorder2.c`, `workorder3.c`,
`objrect.c`, `pathsq.c`, `mappath.c`, `tilehelp.c`. `NewGardenerOrder`/
`NewMechanicOrder` are a pair; `FindOpenRouteNode`/`FindClosedRouteNode` are a
pair; `RemoveClosedNode` is `RemoveOpenNode`'s twin.

**Order:** `pathmisc2.c` smallest first, then `ridemisc4.c` verb by verb
(find-rec, add-rec, new-rec, free-rec, seat-of, set-full, misc). Save after
every function.

## Files you must NOT touch

- Any existing `LEGOLAND/*.c` (read only), and anything under `tools/`.
- New files belonging to other sessions right now: `coaster7.c`, `uimisc2.c`,
  `audio5.c`, `render5.c` (the Fable session, scope D); `uimisc3.c`,
  `coaster8.c`, `savemisc2.c`, `lfmisc2.c`, `musicthread.c` (the integrating
  session's lanes).
- `docs/DECOMP.md`, `docs/HANDOFF.md`, `README.md`,
  `docs/LEGOLANDPROGRESS.HTML`, `docs/LEGOLANDPROGRESS.SVG`. **Put your levers
  and findings in `docs/lanes/codex-d.md`.**

## Git

`git checkout -b codex/scope-d` from current `main`. Commit to that branch
only; **never commit to `main` and never push `main`.** Push the branch when a
file is complete.

## Rules that are not in the code

- **Marker discipline.** `// WIP-FUNCTION: LEGOLAND 0x<VA>  (<pct>, <precise residual>)`
  until `audit.py` prints `[OK]`; then exactly `// FUNCTION: LEGOLAND 0x<VA>`.
  **Never commit a prologue-only or fabricated-tail body as `// FUNCTION:`.**
- Declare every callee `extern` with a trailing `/* 0x0044xxxx */` comment.
- **Extern prototype TYPES are caller-side codegen levers** — never "align"
  another file's declaration; note the divergence. Symbol NAMES are not levers.
- **One name per address.** A placeholder or a name that collides with an
  existing symbol gets a distinct name, recorded.
- Reproduce original bugs; comment them at the site; do not fix them. The
  unlink family dereferences its list head with no null test in every copy —
  keep that.

## Levers that matter most on these families

- **Records:** a search's failure arm as the `else` of `if (p)` keeps ONE copy;
  a `while`'s condition is whichever test VC6 leaves in the LATCH; walk a list
  through its head GLOBAL for the 5-byte accumulator store; the order of
  `p->next = head;` against a neighbouring field store decides where the head
  LOAD lands; `memset` the whole struct then store the one non-zero field;
  `rep stosd` is NOT proof of memset; two zero registers in one small function
  is a sub-object `memset`; an aggregate local blocks reuse of a dead
  parameter's home slot; VC6's tail-duplication threshold (one call + `add
  esp,4` is copied, two calls are jumped to); test the GLOBAL, not the local
  copy, to keep a list head's null-ness off the cursor; VC6 always
  jump-threads a provably-NULL pointer into a following `if (p)`.
- **Paths/orders:** the bounds-checked cell fetch is `static __inline Cell*
  CellAt(int x, int y)`; run a rect walk on plain ints and write the `Pos` only
  at the call sites; a store to ANY field of an address-taken struct kills CSE
  of an unrelated load; a loop-condition read of a global the body also writes
  — mirror it in a local, re-read in an `else`; `test byte ptr [mem],1` on byte
  +3 of a word is `& 0x100` on the u16; `x & 0xff` and `(unsigned char)x` are
  different objects.
- **Layout:** an else arm is exiled iff it ends in an unconditional jump; two
  `return K` sites merge at the FIRST; the fall-through copy of a shared tail
  survives; a push sinks past a leading guard only when the guarded block ends
  in ONE `return K`; a jump-table switch's case order is its block order, a
  compare-chain by VALUES, shared-body cases as a RANGE chain.
- **Reads:** read a global directly at every use (compute both derived sums
  BEFORE either store); a free `volatile` read at the DEFINITION site; naming
  the intermediate POINTER advances the rotation; `movsx`/`movzx`; the STORE
  width for u16 stashes.

## When a function will not close

Run the §6B triage: strict, register-blind, offset-blind with frame homes by
push depth; try one free `volatile` read, then the one-temporary spellings,
then sweep the store order. Then write the honest WIP note and move on.

## Report format (in `docs/lanes/codex-d.md`, and in your final message)

Per function: address, name, instructions, percentage, `audit [OK]` yes/no,
marker as committed, first diverging index and residual for anything not
`[OK]`. Then which verb-groups proved to be one source, mechanics recovered,
original bugs reproduced, extern-type divergences, and every lever with its
evidence.
