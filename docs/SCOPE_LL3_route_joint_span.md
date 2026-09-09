# Scope LL3 — route / joint / span clip callees (inventory group 3) (2026-09-07)

> **Status 2026-09-09: LL3 is MERGED to main.** Its WIP leftovers — `BsRoute_Trace`, `Route_GetMassAndPower`, `Span_ClipPlane` —
> are now claimed by `docs/SCOPE_LL23_coaster_span_raster.md`. Do NOT assign
> anything from this brief; findings live in `docs/lanes/scope-ll3.md`.

> **Status: IN PROGRESS (claimed 2026-09-07).** Branch `scope/LL3`. Notes: `docs/lanes/scope-ll3.md`. Object prefix
> `/tmp/sll3_`. Cut from inventory group 3 live members
> (`tools/inventory.py`, 2026-09-07 refresh). Naming: `LL*` replaces letter
> scopes after AK.

**Read `docs/PARALLEL_CONTRACT.md` first.** Relocation gate required.
**Do not add any Co-Authored-By / Co-authored-by trailer** (Cursor or Claude).

NEW-FUNCTION scope, **19 functions, ≈1091 instructions**, one new
file preferred: `LEGOLAND/coaster11.c`. Neighbours: `coastermath.c`, `coaster5.c`, `coaster3d.c`, `bswater2.c`, `lfmisc2.c`.

## `LEGOLAND/coaster11.c`

| address | provisional name | insns | evidence |
| --- | --- | ---: | --- |
| 0x00411fa0 | `sub_411fa0` | 74 | called by LFQueue_StepFront (lfmisc2.c) |
| 0x0041c940 | `sub_41c940` | 130 | called by BoatingSchool_BuildRoute (bswater2.c) (+1 more) |
| 0x0041cd20 | `sub_41cd20` | 7 | called by 0x0041cd80 [unmatched] |
| 0x0041cd40 | `sub_41cd40` | 27 | called by 0x0041d210 [unmatched] |
| 0x0041cd80 | `sub_41cd80` | 45 | called by BuildJoint (coastermath.c) |
| 0x0041d210 | `sub_41d210` | 60 | called by TrackFitCheckSpan (coaster5.c) (+1 more) |
| 0x0041d950 | `sub_41d950` | 66 | called by Route_Reset (schoolcar7.c) |
| 0x0041db20 | `sub_41db20` | 37 | pointer in 0x0041db90 [unmatched] |
| 0x0041db90 | `sub_41db90` | 77 | called by RoutePhys_EvaluateDerivative (coaster7.c) (+1 more) |
| 0x0041e7f0 | `sub_41e7f0` | 10 | called by 0x0041db20 [unmatched] |
| 0x0041e8f0 | `sub_41e8f0` | 25 | called by 0x0041d950 [unmatched] |
| 0x0041ede0 | `sub_41ede0` | 38 | called by 0x0041ee40 [unmatched] |
| 0x0041ee40 | `sub_41ee40` | 79 | called by 0x0041cd20 [unmatched] |
| 0x0041ef60 | `sub_41ef60` | 80 | called by Raster_SubmitPoly (coaster3d.c) |
| 0x0041f030 | `sub_41f030` | 4 | called by 0x0041ef60 [unmatched] |
| 0x0041f050 | `sub_41f050` | 179 | called by 0x0041f2b0 [unmatched] |
| 0x0041f2b0 | `sub_41f2b0` | 55 | called by 0x0041ef60 [unmatched] (+1 more) |
| 0x0041f3e0 | `sub_41f3e0` | 85 | called by 0x0041f4e0 [unmatched] |
| 0x0041f4c0 | `sub_41f4c0` | 13 | called by 0x0041f4e0 [unmatched] |

**Order:** stubs / smallest first; largest body (`0x0041f050`, 179i) last. Rename from body evidence.

## Owned elsewhere — do not create or edit

scopes F/G/H (partials), V + X (event ticks), Codex-F (`coaster10.c`, `ridemachine2.c`, …), AG (certificate/WinMain), AC mantex WIPs, every existing `.c` except your new file(s).
