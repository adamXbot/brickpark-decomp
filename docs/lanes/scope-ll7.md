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
| 0x00429560 | TrackRunSetSlope | 90 | 92 | 7 (eax/edx) | WIP |
| 0x00429cf0 | Track_StepObjective | 93 | — | 61 | WIP |
| 0x00429f30 | Track_StepAlong | 70 | — | 58 | WIP |
| 0x0042a1b0 | Track_MeasureDistance | 92 | — | — | not started |
| 0x0042a680 | — | 82 | — | — | not started |
| 0x00428860 | — | 254 | — | — | not started |

**11 / 17 exact.** `/W3` clean. `relocs.py` 0 MISMATCH on the 11 FUNCTION bodies.

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

## Mechanics

- **RoutePos** is `{node, geom, pos}`. **RouteGeom** is 0x58 with `eval` at +0x4c, `t0`/`t1` at +0x44/+0x48, `prev` at +0x54 (RetreatGeometry).
- **TrackCursorPair** is 0x38: `t0`, `at0`, `roll0`, `roll1`, `t1`, `at1`.
- **TrackNode** piece is 0xa4: RouteGeom at +0x4c, parameter range at +0x90/+0x94.
- **TrackRunSetSlope** builds one ramp geom from the span's world endpoints (square-to-world + `g_joint_world[dir]`, half-offset `g_joint_half[i0]`) and stamps it on every piece with `t` ranges `[i/n, (i+1)/n]`. z/dz only place the endpoints.
- **Track_Bisect**: same-sign endpoints return 0 (`xor` of the float bits, test `0x80000000`); else midpoint of the final 0.005-wide bracket. Stats at 0x00615fc4 / 0x00615fc8; max iterations at 0x00615fec.
- **0x00428860** has an EBP frame (`push ebp / mov ebp, esp / sub esp, 0x70`) and calls 0x00420780 / 0x00428840 (owned by LL4 / LL6).

## Levers

- **TrackFitSpanGeom**: named `head_opp` / `tail_opp`. Nested Opposite calls split `add esp` (70%).
- **Track_Bisect**: `*(unsigned*)&pa ^ *(unsigned*)&pb` (pa first in the xor) lands the original `edx=[esp+0x14], ecx=[esp+0x10]`. A `pm` local is required so the mid-sample does not reuse `pb`'s slot (reuse dropped to 85%).
- **TrackRunSetSlope**: body is instruction- and byte-identical except the loop prelude's eax/edx swap (`edx=steps, eax=0` vs the reverse). Ruled out: named `zed`/`k`, `zed=steps` then 0, `volatile` reload of steps, splitting `count` for the `1/steps` fild, chained `a=b=c=0` (reverses the +0x40/+0x48 stores). Still open: a free volatile or one extra IR temporary that gives the zero eax.

## Extern-type divergences

- `TrackCurve_EvaluateOffset` / `EvaluateDerivative` / `EvaluatePosition` / `EvaluateUp` take `float t` here; coaster9.c uses `int t` on some of these.
- `g_curve_offset` 0x00615fd4 is the rail offset AND the stepper tolerance (RouteCar_SetPosition passes 4.8 for both).
