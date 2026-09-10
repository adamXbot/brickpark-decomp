# Scope LL8 — gameframe / icon-UI leftovers (inventory group 12+13+14) (2026-09-07)

> **Status: DONE — 13 of 13 exact, merged into `main`.** `gameframe2.c` carries no
> WIP markers (`AddScriptString` 0x004689f0 closed); branch `scope/LL8` has been deleted. The original
> cut follows. Branch `scope/LL8`. Notes: `docs/lanes/scope-ll8.md`. Object prefix
> `/tmp/sll8_`. Cut from inventory group 12+13+14 live members
> (`tools/inventory.py`, 2026-09-07 refresh). Naming: `LL*` replaces letter
> scopes after AK.

**Read `docs/PARALLEL_CONTRACT.md` first.** Relocation gate required.
**Do not add any Co-Authored-By / Co-authored-by trailer** (Cursor or Claude).

NEW-FUNCTION scope, **13 functions, ≈514 instructions**, one new
file preferred: `LEGOLAND/gameframe2.c`. Neighbours: `gameframe.c`, `fpui.c`, `fpui4.c`, `iconui.c`, `popupmisc.c`, `input.c`, `movie3.c`.

## `LEGOLAND/gameframe2.c`

| address | provisional name | insns | evidence |
| --- | --- | ---: | --- |
| 0x004689f0 | `NewScriptEvent` | 91 | called by ResetLevelGlobals (movie3.c) (+1 more) |
| 0x0046cb20 | `sub_46cb20` | 13 | called by BeginParkLoad (gameframe.c) |
| 0x0046ce00 | `sub_46ce00` | 5 | called by sub_46cb20 [declared in gameframe.c] |
| 0x0046cff0 | `sub_46cff0` | 44 | called by InGameFrame (gameframe.c) |
| 0x0046d2f0 | `sub_46d2f0` | 15 | called by sub_46cff0 [declared in gameframe.c] |
| 0x0046d590 | `RemoveIconGroupRange` | 56 | called by UnLoadInGameIcons (popupmisc.c) |
| 0x0046dac0 | `sub_46dac0` | 36 | tail-jumped from ScanMouse (input.c) |
| 0x0046db40 | `sub_46db40` | 36 | tail-jumped from ScanMouse (input.c) |
| 0x0046dd10 | `sub_46dd10` | 92 | called by ScrollIconPanel (fpui4.c) |
| 0x0046ee00 | `UpdateIconPage` | 74 | called by MapScreenFrame (gamemain.c) (+1 more) |
| 0x0049a4a0 | `sub_49a4a0` | 3 | table at 0x004b83c8 in .data |
| 0x0049a4d0 | `sub_49a4d0` | 3 | table at 0x004b83cc in .data |
| 0x0049cfc0 | `sub_49cfc0` | 46 | called by BeginParkLoad (gameframe.c) |

**Order:** stubs / smallest first; largest body (`0x0046dd10`, 92i) last. Rename from body evidence.

## Owned elsewhere — do not create or edit

scopes F/G/H (partials), V + X (event ticks), Codex-F (`coaster10.c`, `ridemachine2.c`, …), AG (certificate/WinMain), AC mantex WIPs, every existing `.c` except your new file(s).
