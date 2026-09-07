# Scope LL7 — track join / curve / station callees

Branch `scope/LL7`. File `LEGOLAND/coaster13.c`. Object prefix `/tmp/sll7_`.
Brief: `docs/SCOPE_LL7_track_join_curve.md`.

## Status

| address | name | insns | % | audit | marker |
| --- | --- | ---: | ---: | --- | --- |
| 0x0042a670 | TrackCursorPair_GetRollDelta | 2 | 100 | [OK] | FUNCTION |
| 0x00429ac0 | TrackCurve_EvalVtable | 12 | 100 | [OK] | FUNCTION |
| 0x00429b60 | TrackCurve_EvaluateBasis | 16 | 100 | [OK] | FUNCTION |
| 0x0042a5e0 | TrackCursorPair_Init | 19 | 100 | [OK] | FUNCTION |
| 0x00429c10 | TrackCurve_SolverSample | 22 | 100 | [OK] | FUNCTION |
| 0x0042a110 | RoutePos_Equal | 28 | 100 | [OK] | FUNCTION |
| 0x0042a150 | Track_AbsDerivative | 28 | 100 | [OK] | FUNCTION |
| 0x00429690 | TrackRunSetLevel | 35 | 100 | [OK] | FUNCTION |
| 0x00429af0 | TrackCurve_MakeBasis | 39 | 100 | [OK] | FUNCTION |
| 0x004298a0 | TrackFitSpanGeom | 41 | 100 | [OK] | FUNCTION |
| 0x00429e20 | Track_Bisect | 76 | 100 | [OK] | FUNCTION |
| 0x0042a1b0 | Track_MeasureDistance | 92 | 100 | [OK] | FUNCTION |
| 0x0042a680 | TrackCursorPair_Draw | 82 | 100 | [OK] | FUNCTION |
| 0x00429cf0 | Track_StepObjective | 93 | 100 | [OK] | FUNCTION |
| 0x00429560 | TrackRunSetSlope | 90 | 92 | 7 (eax/edx) | WIP |
| 0x00429f30 | Track_StepAlong | 70 | 62 | 58 | WIP |
| 0x00428860 | TrackShade_FillPoly | 254 | — | — | not started |

**14 / 17 exact.** `/W3` clean. `relocs.py` 0 MISMATCH on the 14 FUNCTION bodies.

## Names

- **TrackCursorPair_GetRollDelta** 0x0042a670: 0x0042a680 adds the return to `pair->roll[i]`. Retail body is a pooled `0.0f`.
- **TrackCurve_EvalVtable** 0x00429ac0: `geom->eval[mode].dir`.
- **TrackCurve_MakeBasis** 0x00429af0: twin of MakeRotation (coaster6.c 0x00426560).
- **TrackCurve_EvaluateBasis** 0x00429b60: vtable slot 1 then MakeBasis.
- **TrackCursorPair_Init** 0x0042a5e0: copies one `(at, t)` into both 0x20-aligned cursor slots.
- **RoutePos_Equal** 0x0042a110: node, geom, then Vec3Equal (0x00425da0).
- **TrackRunSetLevel** 0x00429690 / **TrackFitSpanGeom** 0x004298a0 / **TrackRunSetSlope** 0x00429560: names from TrackJoinPieces / TrackFitCheckSpan (coaster5.c).
- **TrackCurve_SolverSample** 0x00429c10: pointer inside TrackCurve_EvaluateDerivative.
- **Track_AbsDerivative** 0x0042a150: pointer inside 0x0042a1b0.
- **Track_Bisect** 0x00429e20: default `[0x004b63fc]` hook.
- **Track_StepObjective** 0x00429cf0 / **Track_StepAlong** 0x00429f30: the 30-unit backward stepper RouteCar_SetPosition calls.
- **TrackShade_FillPoly** 0x00428860: shaded/textured sibling of ZBuffer_FillPoly (schoolcar6.c 0x00423350). Same proofs: EBP frame, `xchg ebx,eax`, `add ebx,1`.
- **Track_MeasureDistance** 0x0042a1b0: name from Coaster_StationDerivative (coaster7.c).
- **TrackCursorPair_Draw** 0x0042a680: wheels at the first cursor; then first→second copy.

## Mechanics

- **RoutePos** is `{node, geom, pos}`. **RouteGeom** is 0x58 with `eval` at +0x4c, `t0`/`t1` at +0x44/+0x48, `prev` at +0x54 (RetreatGeometry).
- **TrackCursorPair** is 0x38: `t0`, `at0`, `roll0`, `roll1`, `t1`, `at1`.
- **TrackNode** piece is 0xa4: RouteGeom at +0x4c, parameter range at +0x90/+0x94.
- **TrackRunSetSlope** builds one ramp geom from the span's world endpoints (square-to-world + `g_joint_world[dir]`, half-offset `g_joint_half[i0]`) and stamps it on every piece with `t` ranges `[i/n, (i+1)/n]`. z/dz only place the endpoints.
- **Track_Bisect**: same-sign endpoints return 0 (`xor` of the float bits, test `0x80000000`); else midpoint of the final 0.005-wide bracket. Stats at 0x00615fc4 / 0x00615fc8; max iterations at 0x00615fec.
- **0x00428860** is a shaded z-buffer span filler: EBP frame `sub esp,0x70`, increments `edge[key[n-1].idx].ylast`, calls 0x00420780 (table at 0x004d89c8) and 0x00428840 (lowest-set-bit). Sibling of ZBuffer_FillPoly. Inner span is `__asm` (`xchg ebx,eax`, `add ebx,1`).
- **g_step_lo2** is `(step-tol)²`, not `(t-tol)²`. hi2 is `(step+tol)²`.

## Levers

- **TrackFitSpanGeom**: named `head_opp` / `tail_opp`. Nested Opposite calls split `add esp` (70%).
- **Track_Bisect**: `*(unsigned*)&pa ^ *(unsigned*)&pb` (pa first in the xor) lands the original `edx=[esp+0x14], ecx=[esp+0x10]`. A `pm` local is required so the mid-sample does not reuse `pb`'s slot (reuse dropped to 85%).
- **TrackRunSetSlope**: body is instruction- and byte-identical except the loop prelude's eax/edx swap (`edx=steps, eax=0` vs the reverse). Ruled out: named `zed`/`k`, `zed=steps` then 0, `volatile` reload of steps, splitting `count` for the `1/steps` fild, chained `a=b=c=0` (reverses the +0x40/+0x48 stores). Still open: a free volatile or one extra IR temporary that gives the zero eax.
- **Track_MeasureDistance**: extra `g_dist_at = &cur` before the loop integrate and the final `[t0, t]` integrate (equal path sets it to `from`). 0.01f is `0x3c23d70a`.
- **TrackCursorPair_Draw**: interleave `s=sin; m[0]=s; c=cos; m[2]=c; m[8]=-c; m[10]=s` so the leftover sin is `fst` then later `fstp`. Computing both trigs first emitted `fld st(1)`. Mat slots are 0/2/8/10 (not 1/2/8/10). `#pragma intrinsic(sin, cos)`.
- **Track_StepObjective**: `if ((dist2 = x*x+y*y+z*z) > hi2)` (assignment-in-condition) lands `fld st / fcomp hi2`. A named `dist2 = sum; if (dist2 > hi2)` emitted `fcom [home]`.
- **TrackRunSetSlope**: post-call `int zed=0; int left=steps; if (left>zed)`, named `st`/`clr` pointer, and a `from`-slot pun on StepAlong are inert or worse. The IV still wins eax.
- **Track_StepAlong**: best honest body is 70i/227B vs 232B, matchfull 61.8%, audit 58. `g_step_lo2=(step-tol)²`. Original pushes `out_t` early, dword-moves `geom->t0` over the dead `from` slot, and `fstp [esp]` overwrites a pushed `tol` with `t`. A `float t0` local is 5 bytes short; an `int` pun and overwriting `from` dropped to ~55%. Residual is the solver-arg push schedule.

## Extern-type divergences

- `TrackCurve_EvaluateOffset` / `EvaluateDerivative` / `EvaluatePosition` / `EvaluateUp` take `float t` here; coaster9.c uses `int t` on some of these.
- `g_curve_offset` 0x00615fd4 is the rail offset AND the stepper tolerance (RouteCar_SetPosition passes 4.8 for both).
