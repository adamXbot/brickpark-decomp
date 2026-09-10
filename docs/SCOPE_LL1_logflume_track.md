# Scope LL1 — logflume track / entrance helpers (inventory group 1) (2026-09-07)

> **Status: DONE — 22 of 22 exact, merged into `main`.** `logflume8.c` carries no
> WIP markers; branch `scope/LL1` has been deleted. The original
> cut follows. Branch `scope/LL1`. Notes: `docs/lanes/scope-ll1.md`. Object prefix
> `/tmp/sll1_`. Cut from inventory group 1 live members
> (`tools/inventory.py`, 2026-09-07 refresh). Naming: `LL*` replaces letter
> scopes after AK.

**Read `docs/PARALLEL_CONTRACT.md` first.** Relocation gate required.
**Do not add any Co-Authored-By / Co-authored-by trailer** (Cursor or Claude).

NEW-FUNCTION scope, **22 functions, ≈861 instructions**, one new
file preferred: `LEGOLAND/logflume8.c`. Neighbours: `logflume.c`, `logflume2.c`, `posstep.c`.

## `LEGOLAND/logflume8.c`

| address | provisional name | insns | evidence |
| --- | --- | ---: | --- |
| 0x00409620 | `sub_409620` | 25 | called by 0x004097a0 [unmatched] (+1 more) |
| 0x00409680 | `sub_409680` | 25 | called by 0x004097a0 [unmatched] (+1 more) |
| 0x004096e0 | `sub_4096e0` | 26 | called by 0x004097a0 [unmatched] (+1 more) |
| 0x00409740 | `sub_409740` | 25 | called by 0x004097a0 [unmatched] (+1 more) |
| 0x004097a0 | `sub_4097a0` | 170 | called by LFTrack_Add (logflume.c) |
| 0x00409a50 | `sub_409a50` | 12 | called by 0x00409a90 [unmatched] |
| 0x00409a90 | `sub_409a90` | 49 | called by 0x0040d900 [unmatched] |
| 0x00409b10 | `sub_409b10` | 37 | called by 0x0040a010 [unmatched] |
| 0x0040a010 | `sub_40a010` | 46 | called by 0x0040a080 [unmatched] |
| 0x0040a080 | `sub_40a080` | 45 | called by 0x0040d900 [unmatched] |
| 0x0040a0f0 | `sub_40a0f0` | 38 | called by 0x0040a2a0 [unmatched] |
| 0x0040a160 | `sub_40a160` | 37 | called by 0x0040a2a0 [unmatched] |
| 0x0040a1d0 | `sub_40a1d0` | 37 | called by 0x0040a2a0 [unmatched] |
| 0x0040a230 | `sub_40a230` | 40 | called by 0x0040a2a0 [unmatched] |
| 0x0040a2a0 | `sub_40a2a0` | 23 | called by LFTrack_Remove (logflume.c) (+1 more) |
| 0x0040b290 | `sub_40b290` | 93 | called by LFHoldUp_Interact (logflume.c) (+2 more) |
| 0x0040ce20 | `sub_40ce20` | 72 | called by 0x0040cf10 [unmatched] |
| 0x0040cf10 | `sub_40cf10` | 7 | called by 0x0040db00 [unmatched] (+2 more) |
| 0x0040cf30 | `sub_40cf30` | 10 | called by 0x0040d6f0 [unmatched] |
| 0x0040cf50 | `sub_40cf50` | 15 | called by 0x0040d900 [unmatched] (+1 more) |
| 0x0040cf80 | `sub_40cf80` | 14 | called by 0x0040d900 [unmatched] (+1 more) |
| 0x0040cfa0 | `sub_40cfa0` | 15 | called by 0x0040d900 [unmatched] (+1 more) |

**Order:** stubs / smallest first; largest body (`0x004097a0`, 170i) last. Rename from body evidence.

## Owned elsewhere — do not create or edit

scopes F/G/H (partials), V + X (event ticks), Codex-F (`coaster10.c`, `ridemachine2.c`, …), AG (certificate/WinMain), AC mantex WIPs, every existing `.c` except your new file(s).
