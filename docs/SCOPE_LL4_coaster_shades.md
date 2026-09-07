# Scope LL4 — coaster shade / blit table callees (inventory group 4) (2026-09-07)

> **Status: IN PROGRESS (claimed 2026-09-07).** Branch `scope/LL4`. Notes: `docs/lanes/scope-ll4.md`. Object prefix
> `/tmp/sll4_`. Cut from inventory group 4 live members
> (`tools/inventory.py`, 2026-09-07 refresh). Naming: `LL*` replaces letter
> scopes after AK.

**Read `docs/PARALLEL_CONTRACT.md` first.** Relocation gate required.
**Do not add any Co-Authored-By / Co-authored-by trailer** (Cursor or Claude).

NEW-FUNCTION scope, **8 functions, ≈797 instructions**, one new
file preferred: `LEGOLAND/coastershade2.c`. Neighbours: `schoolcar8.c`, `coastertiny.c`, `coaster7.c`.

## `LEGOLAND/coastershade2.c`

| address | provisional name | insns | evidence |
| --- | --- | ---: | --- |
| 0x0041f4e0 | `sub_41f4e0` | 78 | called by 0x0041db90 [unmatched] (+2 more) |
| 0x0041f880 | `sub_41f880` | 29 | called by 0x00429f30 [unmatched] |
| 0x0041f8d0 | `sub_41f8d0` | 106 | table at 0x004b5648 in .data |
| 0x0041fba0 | `sub_41fba0` | 136 | table at 0x004b564c in .data |
| 0x0041fd80 | `sub_41fd80` | 162 | table at 0x004b5658 in .data |
| 0x0041ff80 | `sub_41ff80` | 202 | table at 0x004b565c in .data |
| 0x00420200 | `sub_420200` | 81 | called by 0x0042a1b0 [unmatched] |
| 0x00420780 | `sub_420780` | 3 | called by 0x00428860 [unmatched] (+1 more) |

**Order:** stubs / smallest first; largest body (`0x0041ff80`, 202i) last. Rename from body evidence.

## Owned elsewhere — do not create or edit

scopes F/G/H (partials), V + X (event ticks), Codex-F (`coaster10.c`, `ridemachine2.c`, …), AG (certificate/WinMain), AC mantex WIPs, every existing `.c` except your new file(s).
