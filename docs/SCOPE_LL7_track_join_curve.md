# Scope LL7 — track join / curve / station callees (inventory group 7) (2026-09-07)

> **Status 2026-09-09: LL7 is MERGED to main.** Its WIP leftovers — `TrackShade_FillPoly` —
> are now claimed by `docs/SCOPE_LL23_coaster_span_raster.md`. Do NOT assign
> anything from this brief; findings live in `docs/lanes/scope-ll7.md`.

> **Status: IN PROGRESS (claimed 2026-09-08).** Branch `scope/LL7`. Notes: `docs/lanes/scope-ll7.md`. Object prefix
> `/tmp/sll7_`. Cut from inventory group 7 live members
> (`tools/inventory.py`, 2026-09-07 refresh). Naming: `LL*` replaces letter
> scopes after AK.

**Read `docs/PARALLEL_CONTRACT.md` first.** Relocation gate required.
**Do not add any Co-Authored-By / Co-authored-by trailer** (Cursor or Claude).

NEW-FUNCTION scope, **17 functions, ≈999 instructions**, one new
file preferred: `LEGOLAND/coaster13.c`. Neighbours: `coaster5.c`, `coaster7.c`, `coaster9.c`, `schoolcar4.c`.

## `LEGOLAND/coaster13.c`

| address | provisional name | insns | evidence |
| --- | --- | ---: | --- |
| 0x00428860 | `sub_428860` | 254 | table at 0x004b5f50 in .data |
| 0x00429560 | `sub_429560` | 90 | called by TrackJoinPieces (coaster5.c) |
| 0x00429690 | `sub_429690` | 35 | called by TrackJoinPieces (coaster5.c) |
| 0x004298a0 | `sub_4298a0` | 41 | called by TrackFitCheckSpan (coaster5.c) |
| 0x00429ac0 | `sub_429ac0` | 12 | called by 0x00429b60 [unmatched] (+1 more) |
| 0x00429af0 | `sub_429af0` | 39 | called by RouteNode_GetTransform [declared in coaster9.c] (+1 more) |
| 0x00429b60 | `sub_429b60` | 16 | called by 0x0042a680 [unmatched] |
| 0x00429c10 | `sub_429c10` | 22 | pointer in TrackCurve_EvaluateDerivative [declared in coaster9.c] |
| 0x00429cf0 | `sub_429cf0` | 93 | pointer in 0x00429f30 [unmatched] (+1 more) |
| 0x00429e20 | `sub_429e20` | 76 | table at 0x004b63fc in .data |
| 0x00429f30 | `sub_429f30` | 70 | called by RouteCar_SetPosition (schoolcar4.c) (+2 more) |
| 0x0042a110 | `sub_42a110` | 28 | called by 0x0042a1b0 [unmatched] |
| 0x0042a150 | `sub_42a150` | 28 | pointer in 0x0042a1b0 [unmatched] |
| 0x0042a1b0 | `sub_42a1b0` | 92 | called by Coaster_StationDerivative (coaster7.c) |
| 0x0042a5e0 | `sub_42a5e0` | 19 | called by 0x0041e8f0 [unmatched] |
| 0x0042a670 | `sub_42a670` | 2 | called by 0x0042a680 [unmatched] |
| 0x0042a680 | `sub_42a680` | 82 | called by RouteNode_LinkPending [declared in coaster9.c] |

**Order:** stubs / smallest first; largest body (`0x00428860`, 254i) last. Rename from body evidence.

## Owned elsewhere — do not create or edit

scopes F/G/H (partials), V + X (event ticks), Codex-F (`coaster10.c`, `ridemachine2.c`, …), AG (certificate/WinMain), AC mantex WIPs, every existing `.c` except your new file(s).
