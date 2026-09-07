# Scope LL6 — raster / map / track-piece callbacks (inventory group 6) (2026-09-07)

> **Status: IN PROGRESS (claimed 2026-09-08).** Branch `scope/LL6`. Notes: `docs/lanes/scope-ll6.md`. Object prefix
> `/tmp/sll6_`. Cut from inventory group 6 live members
> (`tools/inventory.py`, 2026-09-07 refresh). Naming: `LL*` replaces letter
> scopes after AK.

**Read `docs/PARALLEL_CONTRACT.md` first.** Relocation gate required.
**Do not add any Co-Authored-By / Co-authored-by trailer** (Cursor or Claude).

NEW-FUNCTION scope, **24 functions, ≈771 instructions**, one new
file preferred: `LEGOLAND/coaster12.c`. Neighbours: `coaster3d.c`, `renderview.c`, `castleobj.c`, `coastertiny.c`.

## `LEGOLAND/coaster12.c`

| address | provisional name | insns | evidence |
| --- | --- | ---: | --- |
| 0x00423200 | `sub_423200` | 61 | called by Raster_SubmitPoly (coaster3d.c) |
| 0x00423940 | `sub_423940` | 8 | table at 0x004b5b64 in .data |
| 0x00423970 | `sub_423970` | 6 | table at 0x004b5b68 in .data |
| 0x00423990 | `sub_423990` | 2 | table at 0x004b5b6c in .data |
| 0x00423f40 | `sub_423f40` | 86 | called by 0x00424050 [unmatched] |
| 0x00424050 | `sub_424050` | 87 | called by RenderFullMap (renderview.c) |
| 0x00425da0 | `sub_425da0` | 25 | called by 0x0042a110 [unmatched] |
| 0x00426190 | `sub_426190` | 21 | called by Mat3_TransposeToMat4 [declared in coaster9.c] |
| 0x004263a0 | `sub_4263a0` | 72 | called by 0x00426750 [unmatched] |
| 0x00426460 | `sub_426460` | 21 | called by 0x0042a680 [unmatched] |
| 0x004265d0 | `sub_4265d0` | 66 | called by ClipRect_SetBounds [declared in coaster9.c] (+1 more) |
| 0x00426750 | `sub_426750` | 24 | called by CoasterModel_GetClipRect [declared in coaster9.c] |
| 0x004275b0 | `sub_4275b0` | 1 | table at 0x004b5d4c in .data |
| 0x004275c0 | `sub_4275c0` | 1 | table at 0x004b5d48 in .data |
| 0x00427a40 | `sub_427a40` | 24 | table at 0x004b5d3c in .data |
| 0x00427a80 | `sub_427a80` | 6 | table at 0x004b5d40 in .data |
| 0x00427c30 | `sub_427c30` | 24 | table at 0x004b5d74 in .data |
| 0x00427c70 | `sub_427c70` | 7 | table at 0x004b5d78 in .data |
| 0x00427f70 | `sub_427f70` | 39 | table at 0x004b5df4 in .data |
| 0x00427ff0 | `sub_427ff0` | 39 | table at 0x004b5df0 in .data |
| 0x00428350 | `sub_428350` | 30 | called by Coaster3D_BuildPieceGeometry (coaster3d.c) |
| 0x004283c0 | `sub_4283c0` | 103 | called by 0x004286e0 [unmatched] |
| 0x004286e0 | `sub_4286e0` | 8 | table at 0x004b5d44 in .data |
| 0x00428840 | `sub_428840` | 10 | called by 0x00428860 [unmatched] |

**Order:** stubs / smallest first; largest body (`0x004283c0`, 103i) last. Rename from body evidence.

## Owned elsewhere — do not create or edit

scopes F/G/H (partials), V + X (event ticks), Codex-F (`coaster10.c`, `ridemachine2.c`, …), AG (certificate/WinMain), AC mantex WIPs, every existing `.c` except your new file(s).
