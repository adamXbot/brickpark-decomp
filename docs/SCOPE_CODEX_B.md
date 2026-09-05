# Parallel session scope — "codex-b" (2026-09-05)

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

**This session writes NEW functions from the unmatched-callee frontier**:
audio and music, the report and free-play screens' input handlers, help and
icon helpers, and the smallest screen callbacks — about 60 functions, most
under 40 instructions. Small bodies close at a very high rate here.

## Read these first (in this order)

1. `docs/LANE_BRIEF.md` — the method per function and **the verification gate**
   (its PROJECT/STATUS lines are stale; use ENVIRONMENT below).
2. `docs/DECOMP.md`, section "VC6 SP3 codegen levers" — the ~250 entries at the
   TOP are the newest. The screen-callback, side-panel, pop-up and `fable-b`
   (system/audio) blocks are your closest; search the rest when stuck.
3. `docs/HANDOFF.md` §3 (rules not in the code) and §6B (residual triage), and
   `docs/RIDE_CALLBACKS.md` (callback slot meanings).

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
  **RVA = VA − 0x400000** (0x00498630 → 0x98630).
- Iterate: `$PY tools/matchfull.py LEGOLAND/<file>.c <Name> 0x<VA> --obj /tmp/cb_<Name>.obj`
- **Authoritative gate:** `$PY tools/audit.py LEGOLAND/<file>.c` — a function
  counts ONLY when it prints `[OK]`. `matchfull.py` can over-report a body whose
  `ret` is followed by a `.rdata` jump table; trust `audit.py`.
- `/W3` check: `ALPHATEAM_VC6_ROOT="$PWD/toolchain" "$LEGOLAND_CL" /nologo /c /W3 /O2 /Gy /Gd /Fo/tmp/cb_w3.obj LEGOLAND/<file>.c`
- Objects always under `/tmp`; never `/Zi`.
- **Do NOT run `tools/verify.py`, `tools/progress.py` or `tools/coverage.py`**,
  and do not edit any file under `tools/`. `audit.py` and `matchfull.py` use
  per-process object paths and are safe alongside other sessions' compiles.

## Your files and functions (≈1,460 instructions)

Create these NEW files. Do not edit any existing `.c` file — read them freely.
Anything above 0x0049e000 is the statically linked CRT and is not a target.

### `LEGOLAND/audio4.c` — audio, narration, MIDI (≈320 insns)

| address | name | insns | declared by |
| --- | --- | --- | --- |
| 0x00480170 | `ReadBE16` | 15 | music.c |
| 0x00480150 | `ReadBE32` | 15 | music.c |
| 0x004927b0 | `StopPlayableSample` | 32 | audio2.c |
| 0x004801a0 | `ReadMidiTrack` | 34 | music.c |
| 0x00498920 | `PauseCurrentTrack` | 35 | fpui5.c, mapscreen.c, mapscreen2.c, screens3.c |
| 0x00492130 | `InitSoundSampleSystem` | 39 | lifecycle.c |
| 0x00498630 | `PlayNarrationFile` | 150 | fpui5.c |

`audio2.c`, `audio3.c`, `audiomisc.c`, `music.c` and `sysmisc.c`
(`UpdateSampleSource`, exact — with a "shared tail forces volatile stores"
lever) are the context. `fpui5.c`'s `KillObjectHelp` (exact) documents the
narration pause/resume pair around `PlayNarrationFile` from the caller's side.

### `LEGOLAND/uimisc.c` — report/free-play input, pop-up tools, help and icons (≈830 insns)

| address | name | insns | declared by |
| --- | --- | --- | --- |
| 0x004714a0 | `ResetInfoPopUp` | 11 | iconui.c |
| 0x00471d60 | `ResetToolIcons` | 12 | fpui3.c, popup2.c |
| 0x0046df30 | `ClipToIcon` | 12 | render2.c |
| 0x0046f330 | `IconHitTest` | 13 | fpui2.c |
| 0x0046b4f0 | `NewScriptStep` | 13 | savechunks.c |
| 0x00459820 | `EndLevel` | 14 | input.c |
| 0x0046fbc0 | `IndicatorInput` | 14 | iconui.c |
| 0x004730f0 | `PU_CloseInput` | 15 | bighelp.c |
| 0x0046d340 | `ShowObjectHelp` | 15 | iconui.c |
| 0x0046d230 | `ShowIdHelp` | 16 | iconui.c |
| 0x004731a0 | `PU_DeleteInput` | 16 | bighelp.c |
| 0x004733b0 | `PU_PrevInput` | 17 | bighelp.c |
| 0x0046de50 | `GetIconBounds` | 17 | fpui2.c |
| 0x004993c0 | `ThawGameClock` | 17 | input2.c |
| 0x0046b200 | `ScriptEventDue` | 17 | fpui3.c |
| 0x00473310 | `PU_ToolA` | 21 | bighelp.c |
| 0x0046d280 | `ShowHelpPopup` | 21 | workorder2.c |
| 0x0046d460 | `UnlinkIcon` | 22 | iconui.c |
| 0x0046d4a0 | `UnlinkIcon2` | 22 | iconui.c |
| 0x0046c5c0 | `KillHelpText` | 22 | iconui.c |
| 0x00473360 | `PU_NextInput` | 23 | bighelp.c |
| 0x0046d4e0 | `DeleteIcon` | 23 | iconui.c, screens3.c |
| 0x0046b520 | `FreeScriptStep` | 25 | fpui3.c |
| 0x00444200 | `SaveReport` | 28 | savegame.c |
| 0x00444260 | `LoadReport` | 30 | savegame.c |
| 0x00474130 | `GetTypedChar` | 30 | input.c |
| 0x0046b6b0 | `ShowScriptStepText` | 30 | fpui3.c |
| 0x004733f0 | `PU_GardenerInput` | 31 | bighelp.c |
| 0x00473460 | `PU_MechInput` | 31 | bighelp.c |
| 0x004585c0 | `KillCurrentScreen` | 32 | mapscreen.c |
| 0x00490b20 | `ReportNextPageInput` | 32 | mapscreen2.c |
| 0x0048a790 | `FreePlayInit_48a790` | 38 | fpui2.c |
| 0x0048ac60 | `FreePlayAcceptInput` | 38 | fpui2.c |
| 0x00490aa0 | `UpdateReportPageIcons` | 38 | mapscreen2.c |
| 0x00490970 | `ReportAcceptInput` | 39 | mapscreen2.c |

`fpui3.c` (9 of 9 exact — `PU_ToolB`, `UpdateHelpTick`, the scroll buttons),
`fpui4.c`, `fpui5.c` (`PU_Delete2Input`, `KillObjectHelp`), `popup2.c`,
`mapscreen2.c` (`InitScreen7` — the report screen, whose page icons and hint
cursor `UpdateReportPageIcons`/`ReportNextPageInput` drive), `iconui.c`,
`bighelp.c`, `savechunks.c`/`savechunks2.c` (the `ScriptEvent`/step lists) and
`fpui2.c` are the context. `UnlinkIcon`/`UnlinkIcon2` and `SaveReport`/
`LoadReport` are pairs — do one, diff the other. `FreePlayInit_48a790` is a
placeholder name: recover what it does and name it (one name per address).

### `LEGOLAND/screencb6.c` — the smallest screen callbacks (≈309 insns)

| address | name | insns |
| --- | --- | --- |
| 0x00452ba0 | `Dino_AC` | 9 |
| 0x004529c0 | `Fountain_AC` | 9 |
| 0x00434f50 | `CB_434f50` | 11 |
| 0x00433ca0 | `CB_433ca0` | 12 |
| 0x00434040 | `CB_434040` | 12 |
| 0x00434080 | `CB_434080` | 12 |
| 0x00433ce0 | `CB_433ce0` | 14 |
| 0x004340c0 | `CB_4340c0` | 14 |
| 0x004314f0 | `CB_4314f0` | 15 |
| 0x004304a0 | `CB_4304a0` | 16 |
| 0x00431120 | `CB_431120` | 18 |
| 0x004312c0 | `CB_4312c0` | 19 |
| 0x00436160 | `CB_436160` | 22 |
| 0x004361a0 | `CB_4361a0` | 22 |
| 0x004529e0 | `Fountain_Add` | 23 |
| 0x0042b9d0 | `CB_42b9d0` | 26 |
| 0x0042c3f0 | `CB_42c3f0` | 27 |
| 0x00452bc0 | `Dino_Add` | 28 |

Name each `CB_*` from `screen.c`'s `SetCustomCallbacks` (0x00452c20): find
the class arm that stores the pointer and the ObjDef slot it goes into — 8c
select-for-placement, 90 update, 94 draw-selection, 98 add, 9c remove, a0
draw-desc, a4 create, a8 tick, ac destroy, b0 draw, b8 load, bc save — and
follow the naming of the matched siblings in `screencb.c`..`screencb4.c`
(41 of 43 of those closed exact by grouping by slot, building one body per
group, then diffing each sibling). Disassemble all eighteen first, group,
then write.

**Order:** `screencb6.c` → `audio4.c` → `uimisc.c`, smallest first within
each file, finishing a file before starting the next. Save after every
function.

## Files you must NOT touch

- Any existing `LEGOLAND/*.c` (read only), and anything under `tools/`.
- These new files belong to lanes running right now: `render4.c`,
  `bswater3.c`, `coaster6.c`, `schoolcar7.c`, `screencb5.c`, `ridemisc3.c`,
  `workers3.c`, `objdoor.c`, `posstep.c`; and to other parallel sessions:
  `sysmisc2.c`, `workorder4.c`, `savegame2.c`, `data3.c`, `mapscreen4.c`,
  `coastermath.c`, `schoolcar8.c`.
- `docs/DECOMP.md`, `docs/HANDOFF.md`, `README.md`,
  `docs/LEGOLANDPROGRESS.HTML`, `docs/LEGOLANDPROGRESS.SVG`. **Put your levers
  and findings in `docs/lanes/codex-b.md`** in DECOMP's bullet style, each with
  its evidence; the integrating session folds them in.

## Git

`git checkout -b codex/scope-b` from current `main`. Commit to that branch
only; **never commit to `main` and never push `main`.** Push the branch when a
file is complete. Commit messages: one line of what closed, then the counts.

## Rules that are not in the code

- **Marker discipline.** `// WIP-FUNCTION: LEGOLAND 0x<VA>  (<pct>, <precise residual>)`
  until `audit.py` prints `[OK]`; then exactly `// FUNCTION: LEGOLAND 0x<VA>`.
  Marker on the line immediately above the signature. **Never commit a
  prologue-only or fabricated-tail body as `// FUNCTION:`.**
- Declare every callee `extern` with a trailing `/* 0x0044xxxx */` comment.
- **Extern prototype TYPES are caller-side codegen levers.** A callee may need
  different prototypes in different files (`screencb.c` declares one callee
  three ways); never "align" another file's declaration, note the divergence.
  Symbol NAMES are not levers.
- **One name per address.** Placeholder names that collide with a real symbol
  get a distinct name, recorded.
- Reproduce original bugs; comment them at the site; do not fix them.
- Report behaviour you cannot explain from the disassembly rather than
  inventing plausible C. An honest WIP beats a wrong body that compiles close.

## Levers that decide whether a first draft lands at 95% or 40% (these families)

- **Screen callbacks / UI:** struct-copy forward-propagation picks the FIRST
  field read (order the field updates after `dst = kConst;`); a by-value
  `WinRect`'s field ASSIGNMENT order is its register rotation — top, bottom,
  left, right; the four-corner `struct { int left, top, right, bottom; } box;`
  idiom with `box.right - box.left` as arguments; a `||` hit-test's hoisted
  operand must be an aggregate member; zero a struct payload with `memset`
  (three literal zeros stay immediates, five get a hoisted register); a partial
  initialiser `Ctx c = { 1 };` is a placement lever; a push sinks past a
  leading guard only when the guarded block ends in ONE `return K`; a
  state-copy local must re-test the GLOBAL; a named sum can BLOCK VC6's
  algebra; a three-way colour choice is THREE textual calls; a parameter's
  type can be the whole function (`char` where the caller declares `int`).
- **Lists / script steps:** a search's failure arm as the `else` of `if (p)`
  keeps ONE copy; a `while`'s condition is whichever test VC6 leaves in the
  LATCH (read the latch off the original); walk a list through its head
  GLOBAL for the 5-byte accumulator store; two identical arms merge into the
  fall-through copy; `xor <callee-saved>,<same> / mov eax,<it>` proves a
  two-predecessor join; "restore, then test" is a named call result.
- **Audio / system:** a shared tail that RELOADS locals from the frame means
  every arm left them in memory — a volatile STORE through a cast in every
  arm makes the arms identical so VC6 cross-jumps them into one tail; a
  `sete al / test al,1` guard is `((x == 0) & 1) == 0` and the `== 0` half is a
  LAYOUT lever; `mov esi,[__imp__X] / call esi` in a loop needs no construct;
  two near-identical if/else arms were written out IN FULL when a shared
  block sits between them; `memset` the whole struct then store the one
  non-zero field; `rep stosd` is NOT proof of `memset`.
- **Layout:** an else arm is exiled past the fall-through trace iff it ends in
  an unconditional jump; `if (a == 0 || b != c) return X;` exiles X — split
  the test and jump INTO the second `if`'s block to inline it, MERGE separate
  `goto` guards to exile a shared block; two `return K` sites merge at the
  FIRST; a jump-table `switch`'s case order is its block order (a
  compare-chain switch is laid out by case VALUES).
- **Reads / types:** read a global directly at every use; a free `volatile`
  read goes at the DEFINITION site (it cannot cross the callee-saved pushes);
  naming the intermediate POINTER advances the eax→ecx→edx rotation; check the
  STORE width for u16 stashes; `movsx`/`movzx` say signed/unsigned byte; an
  `unsigned short` parameter kept in `cx` can be the whole function.
- **`__asm`:** operand names collide with MASM reserved words (`cr0`-`cr4`,
  `dr0`-`dr7`, `tr3`-`tr7`, `st`).

## When a function will not close

Run the §6B triage: strict, register-blind, offset-blind with frame homes by
push depth. `strict >> rb` is allocation — try one free `volatile` read first,
then the one-temporary spellings; `strict >> ob` is the frame;
`strict == rb == ob` is a scheduling permutation — sweep the store order. Then
write the honest WIP note and move on; breadth beats depth here.

## Report format (in `docs/lanes/codex-b.md`, and in your final message)

Per function: address, name (and why, for anything you renamed), instructions,
percentage, `audit [OK]` yes/no, marker as committed, first diverging index
and residual for anything not `[OK]`. Then mechanics recovered, original bugs
reproduced, extern-type divergences, and every lever with its evidence.
