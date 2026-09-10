# Scope LL2 — logflume drop / track tick cluster (inventory group 2) (2026-09-07)

> **Status: DONE — 6 of 6 exact, merged into `main`.** `logflume9.c` carries no
> WIP markers; branch `scope/LL2` has been deleted. The original
> cut follows. Branch `scope/LL2`. Notes: `docs/lanes/scope-ll2.md`. Object prefix
> `/tmp/sll2_`. Cut from inventory group 2 live members
> (`tools/inventory.py`, 2026-09-07 refresh). Naming: `LL*` replaces letter
> scopes after AK.

**Read `docs/PARALLEL_CONTRACT.md` first.** Relocation gate required.
**Do not add any Co-Authored-By / Co-authored-by trailer** (Cursor or Claude).

NEW-FUNCTION scope, **6 functions, ≈602 instructions**, one new
file preferred: `LEGOLAND/logflume9.c`. Neighbours: `logflume.c`, `logflume2.c`, `logflume4.c`.

## `LEGOLAND/logflume9.c`

| address | provisional name | insns | evidence |
| --- | --- | ---: | --- |
| 0x0040d420 | `sub_40d420` | 73 | called by 0x0040d6f0 [unmatched] |
| 0x0040d520 | `sub_40d520` | 128 | called by LFTrack_Update (logflume.c) (+1 more) |
| 0x0040d6f0 | `sub_40d6f0` | 151 | called by LFDrop_Update (logflume.c) (+7 more) |
| 0x0040d900 | `sub_40d900` | 83 | called by LFDrop_Add (logflume.c) (+7 more) |
| 0x0040da10 | `sub_40da10` | 106 | called by LFTrack_Remove (logflume.c) (+1 more) |
| 0x0040db00 | `sub_40db00` | 61 | called by LFDrop_Remove (logflume.c) (+7 more) |

**Order:** stubs / smallest first; largest body (`0x0040d6f0`, 151i) last. Rename from body evidence.

## Owned elsewhere — do not create or edit

scopes F/G/H (partials), V + X (event ticks), Codex-F (`coaster10.c`, `ridemachine2.c`, …), AG (certificate/WinMain), AC mantex WIPs, every existing `.c` except your new file(s).
