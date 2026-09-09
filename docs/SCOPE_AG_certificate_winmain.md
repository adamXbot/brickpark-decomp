# Scope AG — certificate bitmap + WinMain shell (group 14 leftovers) (2026-09-07)

> **Status: DONE — merged 2026-09-08, 3 of 3 exact.** Branch
> `scope/AG`. Notes: `docs/lanes/scope-ag.md`. Object prefix `/tmp/sag_`.
> The rest of inventory group 14 is already exact in `exceptlog.c` — do not
> recreate those.

**Read `docs/PARALLEL_CONTRACT.md` first.** Relocation gate required.

NEW-FUNCTION scope, **3 functions, ≈677 instructions**, one or two new
files. Small count, large middle body — budget time for `0x00451740`.

## Already owned in group 14 — do NOT recreate

`exceptlog.c` owns 0x00453da0..0x004548f0 (exception report dump). DEAD:
0x00453c20.

## `LEGOLAND/certificate.c` — save-certificate blit path

| address | provisional name | insns | evidence |
| --- | --- | ---: | --- |
| 0x00451740 | `SaveCertificateBitmap` | 620 | called by render5.c's SaveCertificateBitmap declaration — confirm name from body |
| 0x00451f40 | `sub_451f40` | 9 | called by gamemain 0x00459520 |

## `LEGOLAND/winmain.c` — CRT → GameMain wrapper

| address | provisional name | insns | evidence |
| --- | --- | ---: | --- |
| 0x00453d10 | `WinMain` | 48 | SEH: `__try/__except` around GameMain; exceptlog.c / startup.c document the shape. Scope U taught match.py to resolve CRT `__except_list` — use that. |

**Order:** `sub_451f40` → `WinMain` → `SaveCertificateBitmap` last.

`startup.c` (scope T) and `gamemain.c` (scope Q) are the callers/callees —
declare what you need `extern`, do not edit them.

## Owned elsewhere

`exceptlog.c`, `startup.c`, `gamemain.c`, `render5.c`, F/G/H, V, AA–AF,
AC, Codex-F, every existing `.c`.
