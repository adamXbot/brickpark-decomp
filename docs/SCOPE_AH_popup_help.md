# Scope AH — popup / help / free-play icon helpers (inventory group 15) (2026-09-07)

> **Status: DONE — 12 of 12 exact, closed into `main` 2026-09-07.** Branch
> `scope/AH`. Notes: `docs/lanes/scope-ah.md`. Object prefix `/tmp/sah_`.
> Cut from inventory group 15 live members (`tools/inventory.py`).

**Read `docs/PARALLEL_CONTRACT.md` first.** Relocation gate required.
**Do not add any Co-Authored-By / Co-authored-by trailer** (Cursor or Claude).

NEW-FUNCTION scope, **12 functions, ≈605 instructions**, one new file.
Neighbours: `fpui.c`, `bighelp.c`, `saveprof.c`, `panelui.c`, `screens3.c`.

## `LEGOLAND/popupmisc.c`

| address | provisional name | insns | evidence |
| --- | --- | ---: | --- |
| 0x0046f100 | `sub_46f100` | 85 | called by InGameFrame (gameframe.c) |
| 0x0046f890 | `sub_46f890` | 37 | near AddFreePlayIcon (fpui.c) |
| 0x0046f920 | `sub_46f920` | 36 | near AddFreePlayIcon (fpui.c) |
| 0x00470b00 | `sub_470b00` | 53 | near InitPopUpInfo (bighelp.c) |
| 0x00471170 | `sub_471170` | 212 | tail-jumped from UnLoad_PopUpInfo (saveprof.c) |
| 0x00471c10 | `sub_471c10` | 39 | near ResetInfoSelection (tinystubs.c) |
| 0x00473640 | `sub_473640` | 6 | near ProcessHelpKeys (tinystubs.c) |
| 0x00474750 | `sub_474750` | 44 | near UnLoad_Interface_Icons (panelui.c) |
| 0x00474ed0 | `sub_474ed0` | 22 | near BriefIconInput (screens3.c) |
| 0x00475f40 | `sub_475f40` | 52 | near RestoreCurrentMenu (sysstubs.c) |
| 0x00476030 | `sub_476030` | 8 | near RenderIconsHook (tinystubs.c) |
| 0x00476050 | `sub_476050` | 11 | near FlashButton (eventgoalprim.c) |

**Order:** stubs → medium → `0x00471170` last. Rename from body evidence.

## Owned elsewhere — do not create or edit

`bighelp.c`, `fpui.c`, `saveprof.c`, `panelui.c`, scopes F/G/H, V, AC, AG,
Codex-F (`coaster10.c`, `ridemachine2.c`, `uistubs2.c`), every existing `.c`.
