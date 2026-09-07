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
| 0x00429560 | TrackRunSetSlope | 90 | — | — | not started |
| 0x00429f30 | — | 70 | — | — | not started |
| 0x00429e20 | — | 76 | — | — | not started |
| 0x0042a680 | — | 82 | — | — | not started |
| 0x0042a1b0 | Track_MeasureDistance | 92 | — | — | not started |
| 0x00429cf0 | — | 93 | — | — | not started |
| 0x00428860 | — | 254 | — | — | not started |

**10 / 17 exact.** `/W3` clean. `relocs.py` 0 MISMATCH (one UNRESOLVED: the pooled `0.0f` in GetRollDelta).

## Names

- **TrackCursorPair_GetRollDelta** 0x0042a670: 0x0042a680 adds the return to `pair->roll[i]`. Retail body is a pooled `0.0f`; both arguments are unused.
- **TrackCurve_EvalVtable** 0x00429ac0: `geom->eval[mode].dir` — the +4 slot of each 8-byte pair. EvaluatePosition (0x00429a80) uses the +0 slot and adds `RoutePos.pos`.
- **TrackCurve_MakeBasis** 0x00429af0: twin of MakeRotation (coaster6.c 0x00426560). Called by RouteNode_GetTransform.
- **TrackCurve_EvaluateBasis** 0x00429b60: vtable slot 1 then MakeBasis. Called by 0x0042a680.
- **TrackCursorPair_Init** 0x0042a5e0: copies one `(at, t)` into both 0x20-aligned cursor slots. Caller 0x0041e8f0 forks a RouteNode's front and rear TrackCursors.
- **RoutePos_Equal** 0x0042a110: node, geom, then Vec3Equal (0x00425da0) on `pos`.
- **TrackRunSetLevel** 0x00429690: name from TrackJoinPieces (coaster5.c).
- **TrackFitSpanGeom** 0x004298a0: name from TrackFitCheckSpan (coaster5.c).
- **TrackCurve_SolverSample** 0x00429c10: the function pointer TrackCurve_EvaluateDerivative (0x00429c60) passes to 0x0041f4e0. Fills a 3-wide PhysVec from the stashed cursor globals.
- **Track_AbsDerivative** 0x0042a150: function pointer inside 0x0042a1b0. `fsqrt` of the three-term sum of squares.

## Mechanics

- **RoutePos** is `{node, geom, pos}` (coaster7.c / coaster9.c), not coastertiny.c's `{node, pos, geom}`.
- **RouteGeom.eval** at +0x4c is an array of `{pos, dir}` pairs; mode indexes the pair.
- **TrackCursorPair** is 0x38: `t0`, `at0`, `roll0`, `roll1`, `t1`, `at1`. The rolls sit in the 8-byte gap between two 0x20 slots.
- **Solver stash** (written by EvaluateDerivative): `g_curve_at` 0x00615f84, `g_curve_mode` 0x00615f90, `g_curve_offset` 0x00615fd4. Distance-measure stash: `g_dist_at` 0x00615f80, `g_dist_mode` 0x00615ff0, `g_dist_offset` 0x00615ff4.
- **TrackFitSpanGeom** counts sloped joints on both partner runs plus the new piece against the two facing (opposite) direction bits; the span fits iff that count is positive.

## Levers

- **TrackFitSpanGeom**: named `head_opp` / `tail_opp` locals. Nested `TrackJointSloped(d, Opposite(...), Opposite(...))` split the `add esp` (70%, 111B vs 108B) and dropped the `push ebx`. The named first opposite stays in ebx across the second call; all five cdecls share one `add esp,0x24`.
- **Track_AbsDerivative**: `float sum = 0.0f` stored before the derivative call, then the three-term loop accumulates into it and `sqrt` is `#pragma intrinsic`.
- **TrackCurve_SolverSample**: store `v[0..2]` then `n = 3` (original `fstp` x, then integer copies of y/z, then the count).
- **GetRollDelta**: plain `return 0.0f` emits `fld [__real@0]` (pooled), not `fldz`.

## Extern-type divergences

- `TrackCurve_EvaluateOffset` is `float t` here; coaster9.c's definition takes `int t` (raw bits). Same bits on the wire.
- `TrackCurve_EvaluateDerivative` declared `float t, float offset` here; coaster9.c's extern is `int, int`.
