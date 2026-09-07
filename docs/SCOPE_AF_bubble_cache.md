# Scope AF — bubble-help / text-cache helpers (inventory group 15 live) (2026-09-07)

> **Status: DONE — 8 of 8 exact, closed into `main` 2026-09-07.** Branch
> `scope/AF`. Notes: `docs/lanes/scope-af.md`. Object prefix `/tmp/saf_`.
> Cut from inventory group 15 live members.

**Read `docs/PARALLEL_CONTRACT.md` first.** Relocation gate required.

NEW-FUNCTION scope, **8 functions, ≈797 instructions**, one new file.
Neighbours: `fpui2.c` / `fpui3.c` / `popup2.c` / `text.c`.

## Already owned — do NOT recreate

| address | owner |
| --- | --- |
| 0x00457870 | Codex-F `uistubs2.c` (`SetBrickLimit`) |
| 0x00457900 | `eventgoalprim.c` (`SetCurrency`) |
| 0x004551a0, 0x00455220, 0x00455de0 | DEAD |

## `LEGOLAND/bubblecache.c`

| address | provisional name | insns | evidence |
| --- | --- | ---: | --- |
| 0x00454a10 | `sub_454a10` | 92 | called by gamemain 0x00459520 |
| 0x00455a10 | `sub_455a10` | 25 | called by 0x00455a50 |
| 0x00455a50 | `sub_455a50` | 122 | pointer in 0x00455bb0 |
| 0x00455bb0 | `sub_455bb0` | 74 | `HTBubbleHelp` (fpui2.c) |
| 0x00455c80 | `sub_455c80` | 75 | called by 0x00455e50 |
| 0x00455e50 | `sub_455e50` | 49 | `DrawPopUpExtra` (popup2.c) |
| 0x00455fc0 | `sub_455fc0` | 275 | called by gameframe 0x00458ee0 — last |
| 0x00457970 | `FootprintClearanceTest` | 85 | gameframe.c `sub_457970` |

**Order:** smallest → `0x00455fc0` last. Rename from bodies / callers.

## Owned elsewhere

Codex-F `uistubs2.c`, `eventgoalprim.c`, `fpui2.c`, `fpui3.c`, `popup2.c`,
F/G/H, V, AA–AE, AC, every existing `.c`.
