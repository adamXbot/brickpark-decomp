# Scope LL5 — castle entrance track helpers (inventory group 5) (2026-09-07)

> **Status: DONE — 3 of 3 exact, closed into `main` 2026-09-08.**
> `/tmp/sll5_`. Cut from inventory group 5 live members
> (`tools/inventory.py`, 2026-09-07 refresh). Naming: `LL*` replaces letter
> scopes after AK.

**Read `docs/PARALLEL_CONTRACT.md` first.** Relocation gate required.
**Do not add any Co-Authored-By / Co-authored-by trailer** (Cursor or Claude).

NEW-FUNCTION scope, **3 functions, ≈130 instructions**, one new
file preferred: `LEGOLAND/castletrack2.c`. Neighbours: `coaster7.c`, `coastertiny.c`, `schoolcar.c`.

## `LEGOLAND/castletrack2.c`

| address | provisional name | insns | evidence |
| --- | --- | ---: | --- |
| 0x00421ab0 | `sub_421ab0` | 49 | called by Castle_InitEntranceTrack (coaster7.c) (+2 more) |
| 0x00421ce0 | `sub_421ce0` | 38 | called by Castle_InitEntranceTrack (coaster7.c) (+1 more) |
| 0x00422180 | `sub_422180` | 43 | called by 0x00429560 [unmatched] |

**Order:** stubs / smallest first; largest body (`0x00421ab0`, 49i) last. Rename from body evidence.

## Owned elsewhere — do not create or edit

scopes F/G/H (partials), V + X (event ticks), Codex-F (`coaster10.c`, `ridemachine2.c`, …), AG (certificate/WinMain), AC mantex WIPs, every existing `.c` except your new file(s).
