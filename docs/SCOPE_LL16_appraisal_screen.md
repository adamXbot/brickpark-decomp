# Scope LL16 — the park-appraisal report screen `RunAppraisalScreen` 0x004453a0

Branch `scope/LL16` from `0767e1a1` (= `main` 7d756410 + the `tools/match.py`
window change that makes this body boundable). **New file
`LEGOLAND/appraisalscreen.c`** — create only this file (plus
`docs/lanes/scope-ll16.md`); read everything else. Object prefix
`/tmp/sll16_`. Contract: `docs/PARALLEL_CONTRACT.md`. Method and gate:
`docs/LANE_BRIEF.md`. Levers: `docs/LEVERS.md` (symptom index first).

## The function

| | |
| --- | --- |
| address | `0x004453a0` (RVA `0x453a0`) |
| size | **8,085 instructions, 34,662 bytes**, ends `mov eax,1 / pop ebx / add esp,0x23d4 / ret` |
| frame | `mov eax,0x23d4 / call __chkstk` (0x0049e600) — 9,172 bytes of locals, then push ebx/ebp/esi/edi |
| declared | `int RunAppraisalScreen(void)` in `goalstate.c` (called by `AppraisalDueTick`) |
| shape | no x87, no `rep movs`; one jump table `jmp [eax*4+0x0044db08]` at 0x0044ae0e (4 cases, indexed by `rand() & 3`) |
| helpers | all of `appraisal.c` (scope Y) — read it first: the five attraction counters, cell count, sprite loaders, per-line renderers, page buttons |

Callees (47 distinct):

| calls | address | name (file) |
| ---: | --- | --- |
| 129 | 0x0049e4b2 | `rand` (CRT) |
| 112 | 0x00498f50 | `GetString` (text.c) |
| 2 | 0x0049e573 | `sprintf` (CRT) |
| 2 | 0x00498cf0 | tinystubs.c body |
| 2 | 0x00498920 | `PauseCurrentTrack` (audio4.c) |
| 2 | 0x004641f0 | `PopRenderingStatu` (surface.c) |
| 2 | 0x00463850 | `SetPointer` (sweep2.c) |
| 2 | 0x00452460 | `ReadGameButton` (bighelp.c) |
| 1 | 0x0049e600 | `__chkstk` (CRT, implicit) |
| 1 each | 0x004442c0 0x004442f0 0x00444320 0x00444350 0x00444380 0x004449b0 0x00444a70 0x00444b70 0x00444bf0 0x00444c70 0x00444cd0 0x00444d20 0x00444d70 0x00444df0 0x00445000 0x00445100 0x00445190 0x004636c0 | appraisal.c bodies |
| 1 each | 0x0044f360 `IsObjectRunning`, 0x00454d80 `NewPrintColoured`, 0x0045a850 `GetFirstRenderObject`, 0x0045a8b0 `GetNextRenderObject`, 0x00463fc0 `PushRenderingStatusAndLockVideoSurface`, 0x00464080 `PushRenderingStatusAndUnlockVideoSurface`, 0x00466500 `RenderingComplete`, 0x0046b280 `ScriptRunning`, 0x0046d080 `ProcessFrontEndHelp`, 0x0046d110 `UpdateHelpBar`, 0x0046f010 `RenderIcons2`, 0x0046f4c0 `CheckFocussedIcon`, 0x004700a0 `UpdateFocussedIconPtr`, 0x00474880 `SetInGameIconHandler`, 0x004853a0 `PrintSprite`, 0x00485ef0 `ResetHitInfo`, 0x00491d60 `NewPrintCent`, 0x00498630 `PlayNarrationFile`, 0x00498b00 `ResumeCurrentTrack`, 0x00498b40 `sub_498b40` | |

The 129 `rand()` calls and 112 `GetString` calls say what this is: a report
generator that picks phrasing at random from string-table ranges per
statistic, formats lines into stack buffers, and drives a paged screen with
its own input loop. Expect long straight-line runs of the same statement
shape; find the shape once and it repeats.

## How to work a body this size

- **Do not try to match it in one pass.** Get the frame, the prologue and
  the first screen-setup block exact first (matchfull shows the first
  diverging index); then extend section by section. Commit each time the
  first diverging index moves forward. Keep `// WIP-FUNCTION: LEGOLAND
  0x004453a0  (<pct>, first diverging index N, <residual>)` on the body
  until `audit.py` prints `[OK]`.
- The tooling now bounds it: `audit.py`, `matchfull.py` and `relocs.py` all
  use the widened walker in this worktree's `tools/match.py`. Do NOT edit
  `tools/` further. If a tool is slow on 8k instructions, that is expected.
- `matchfull.py` diff output will be very long; pipe it to a file under
  `/tmp/sll16_` and read around the first diverging index.
- Repeated shapes: the `rand()`-driven phrase picks are almost certainly a
  small `static` helper or macro in the original; VC6 inlines `static
  __inline` and small `static` functions at `/O2`, and whether a helper is
  inlined decides block layout — measure both spellings on the first
  occurrence before writing 100 of them.
- The 0x23d4 frame is mostly `char` buffers; their order and sizes are frame
  levers (an aggregate local sits at the top of the frame; contending
  locals in one struct pin their relative slots). Recover sizes from the
  `lea` offsets passed to `sprintf` / `GetString` / the print helpers.
- Reproduce original bugs; comment them at the site; do not fix them.

## Rules

- Declare every callee `extern` with a trailing `/* 0x0044xxxx */` comment;
  never alter another file's declaration; note type divergences in your
  lane doc. `appraisal.c` already declares many of the helpers — copy the
  types it uses.
- Gates before every commit: `audit.py LEGOLAND/appraisalscreen.c` ends
  PASS (a WIP body is fine, a false `// FUNCTION:` is not), `relocs.py`
  zero `MISMATCH`, `/W3` clean.
- Do not run `verify.py`, `progress.py`, `coverage.py`; do not edit
  `tools/`, `docs/DECOMP.md`, `docs/HANDOFF.md`, `README.md`, or any
  existing `LEGOLAND/*.c`.
- Commit to `scope/LL16` only, no push, no merge, **no Co-Authored-By
  trailer of any kind** on commits.
