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
| 0x00429560 | TrackRunSetSlope | 90 | 92 | 7 (eax/edx) **floor** | WIP |
| 0x00429f30 | Track_StepAlong | 70 | 89 | 11 (232/232B) | WIP |
| 0x00428860 | TrackShade_FillPoly | 254 | 38 | 243 **ZBuffer floor** | WIP |

**14 / 17 exact.** SetSlope and FillPoly at their floors; StepAlong size-exact at 11 (two scheduler attractors; see levers). `/W3` clean. `relocs.py` 0 MISMATCH on the 14 FUNCTION bodies.

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
- **TrackShade_FillPoly** 0x00428860: `SpanFiller(tag, grad, nkeys, keys, edges)` from Raster_SubmitPoly. Increments `edges[keys[n-1].idx].y1`, looks up `g_coaster_tab_c[tag]` (0x00420780), bit-scans the two header dwords (0x00428840), writes colour into `g_raster_bits` and depth into `g_zb_base`. Inner span is `__asm`. Best draft 254/254i, 748/771B, frame 0x6c vs 0x70, matchfull ~38%. A dummy `dead` local dropped the count to 252i. `ne` is a MASM reserved word (jne) — the parameter is `nkeys`. Same residual class as ZBuffer_FillPoly (homes + mixed asm).
- **g_step_lo2** is `(step-tol)²`, not `(t-tol)²`. hi2 is `(step+tol)²`.

## Levers

- **TrackFitSpanGeom**: named `head_opp` / `tail_opp`. Nested Opposite calls split `add esp` (70%).
- **Track_Bisect**: `*(unsigned*)&pa ^ *(unsigned*)&pb` (pa first in the xor) lands the original `edx=[esp+0x14], ecx=[esp+0x10]`. A `pm` local is required so the mid-sample does not reuse `pb`'s slot (reuse dropped to 85%).
- **TrackRunSetSlope**: **at its floor** (90i/302B, 7 eax↔edx). The `--steps` IV always wins eax; the loop zero always lands in edx. This session also ruled out: named `w0`/`w1`/`half` pointers (59%, 70 mismatch); `volatile int zed` (57%, +16B); `zed` live across the call via `steps+zed` (folded, same 7); named `dir0`/`jw0` values (ESCAPES, 34%); `if (*(volatile*)&steps > 0) { remain = steps; }` (0 *does* win eax but a second load of steps, 308B, 27 mismatch); volatile `remain` definition (same 7); `if ((remain = steps) > zed)` (same 7). No remaining one-temp or volatile spelling flipped the IV/zero pair at identical bytes.
- **Track_MeasureDistance**: extra `g_dist_at = &cur` before the loop integrate and the final `[t0, t]` integrate (equal path sets it to `from`). 0.01f is `0x3c23d70a`.
- **TrackCursorPair_Draw**: interleave `s=sin; m[0]=s; c=cos; m[2]=c; m[8]=-c; m[10]=s` so the leftover sin is `fst` then later `fstp`. Computing both trigs first emitted `fld st(1)`. Mat slots are 0/2/8/10 (not 1/2/8/10). `#pragma intrinsic(sin, cos)`.
- **Track_StepObjective**: `if ((dist2 = x*x+y*y+z*z) > hi2)` (assignment-in-condition) lands `fld st / fcomp hi2`. A named `dist2 = sum; if (dist2 > hi2)` emitted `fcom [home]`.
- **Track_StepAlong**: size-exact **70i/232B**, matchfull 88.6%, audit **11**. `org = origin` live-across + late `g_step_origin = org` puts origin in edx during `rep movsd`. `hi = tol; … hi = t` + `t0 = cur.geom->t0` before `g_step_len2 = step2` lands `push tol` as the hi placeholder, dword `t0` into the dead `from` slot, and `fstp [esp]`. Loop is exact (`hi = cur.geom->t1`). Residual is one scheduler permutation of two independent blocks after `rep movsd`: original is `[geom, esi=out_t, push esi, t0, eax=step, fstp len2, fld/fadd, store-from, push tol, g_step_len, fld/fmul]`. Two attractors, no tested spelling emits that order:
  - **A (current, 11 mism)**: `t0` before `len2` (with or without named `g`). Correct fstp/fadd; `push esi` batched 7 late with `push tol`; `g_step_len` 2 late (after `fld st/fmul`).
  - **B (21 mism, 90%)**: `g = cur.geom; g_step_len2 = step2; t0 = g->t0`. Early `push esi` + `g_step_len` right after `push tol`; fstp/fadd hoisted *before* geom/push/t0.
  - **C (80%, 71i)**: union/`unsigned` t0bits then `t0 = u.f`. `push esi` lands *between* t0 and fstp (the wanted window) but adds an insn and breaks the later `push t0` into `fld/push/fstp`.
  Coupling: making `t0` a finished pre-call statement (needed to hold fstp after the load) also batches the out_t push with `push tol`. Starting the call before `t0` (needed for early `push esi`) only happens once `len2`/`fadd` have already spilled. Comma-as-hi-arg and `__inline` helper-as-hi flatten or evaluate the complex hi arg *before* the simple `out_t` push (65% or worse) unless hi2 is also a prior statement, which hoists fadd to the prologue. Empty `__asm {}` forces an EBP frame (56%). Also inert/worse: Joust `H(&ot,out_t)`, `float* ot` two-def/volatile, `sol = g_track_solver`, `g->t0` as the call arg (kills CSE, 62%), `g_step_len` comma-into-hi2, hi2 via `g_step_len`, `plen = &g_step_len2`, `step2 + t0*0`, helper `put(t0,step2)` / `t0l()`.
  Side-by-side residual is only indices 12–23: A already has `eax=geom; esi=out_t` then does t0/fstp/fadd; original inserts `push esi` at 12 so the mid-window esp offsets are +4 and `mov [g_step_len],eax` sits between `push ecx` and `fld st/fmul`. 2026-09-08 wave (six requested ideas + helpers/goto) did not leave A:
  - **1** `g=cur.geom` then comma-lo `(t0=g->t0, g_step_len2=step2, t0)`: either flattens to A (C8/C9, 88.6%) or hoists `fadd` over the prologue `fmul` (I1/I1d, 65.7%). Putting every store in that comma still flattens once `step2` is used before the call (needed to keep `fld/fmul [esp+0x1c]`).
  - **2** Named `step2`/`sl`/`keep` held off `g_step_len2` until after t0: same A (I2/C9) or 65.7% if the store is a last-arg comma (E2).
  - **3** `volatile float *out_tp = out_t` (61.6%, prologue wrecked). Plain `float *ot` / `StepAlong_Ot(out_t)` helper: still A. esi is already early; the missing instruction is the push.
  - **4** Split `lo`/`hi2` after the conceptual push: 55.1%.
  - **5** Homes are **not** swapped. After `push tol`, `mov [615fd0],eax` is `g_step_len=step`; later `mov [615f8c],edx` is `g_step_origin=org`; `mov [615fd4],ecx` is `g_curve_offset=tol`. Reordering those three stores: 87.1% (I5/I5b) or the same 11.
  - **6** `Track_MeasureDistance` pushes last-arg **constant** `0.01f` before `rep movsd` because `cur.geom->t1` depends on the copy. StepAlong's last arg is a parameter already in esi; `cur.geom->t0` as the lo arg is B-class (I6/I6b, 61%). `Track_Bisect` has no push/fstp interleave to copy.
  Also worse/inert this wave: whole-call `static __inline` wrappers (H1–H6, 48–88.6%, best = A); `goto solve` merge with the loop (G1 = A, G2 71.8%); inner-scope `g`/`ot` initialisers (S1 = A); `from->geom->t0` (78.9%); `g_step_len=step` before t0 (82.9%).
  Follow-up wave (last-arg / hi-comma / pin-helper / volatile-g) still on A:
  - Last-arg `(g=cur.geom, out_t)` or `PinOt(out_t,g)` + comma-lo: 52.9% (hi2-as-statement steals ST — prologue `fadd` not `fmul`) or 8.6%/2.9% once hi2 moves into the comma (fmul delayed past memcpy).
  - Both-complex (hi-comma t0/len2/hi2 + last-arg geom/out_t): same 52.9% or 2.9%. Work between the two pushes *must* live in the hi arg, but a complex hi still evaluates before a simple `out_t`, and making `out_t` complex starts the call too early.
  - Joust `Put(&t0,g,step2)` as a statement: still A. As hi/last-arg side effect: 2.9–52.9%.
  - `volatile RouteGeom *vg` / `*(volatile float*)&g_step_len2` / `g->t0` as lo: 1.4–52.9% (prologue or B-class hoist). `&*out_t` and `plen=&g_step_len2`: still A.
  - Union/`*(unsigned*)&t0` C-shape: push sits *after* t0 (wanted window, one slot late) at 18.6% / extra insn; single-store pun wrecks the frame (4.3%).
  LL2 `LFUpd_Fst` RTL-helper wave (2026-09-08, tip 637f8231) still on A. `static __inline int Fst(int a, int b) { return a; }` (b evals first) and pointer twins `Ot`/`OtG`/`G`/`T0` / float `FstF`/`Keep`:
  - Statement pin (`t0 = T0(geom,out_t)`, `t0 = G(geom,out_t)->t0`, `org = Fst(origin,out_t)`, last-arg `Ot(out_t,0)` / `OtG(out_t,geom)` / `(float*)Fst(out_t,0)`): **still A** (88.6%). Inlined helper args become temps, not a `push`; esi was already early.
  - Float helpers / `*(int*)&t0` pun: steal ST or hoist `fld/fmul` above `sub esp` (I1 69.1%, frame wrecked). Keep(len2=step2, t0) as a statement: 68.1%.
  - Wanted window is push-then-t0-then-fstp-then-fadd-then-push-tol. That work must live in the **hi** arg *and* last-arg `out_t` must eval first. Complex hi still wins over last-arg helper (t0/fstp before esi/push). Heavier last-arg only moves FP work *before* the push (K3 71%, K5/K6 43%).
  - J11 (all stores in hi-comma + `OtG(out_t, g=cur.geom)`): best non-A at **81.4%/70i**, prologue `fld/fmul` kept, but origin load before `push esi` and both pushes still batched after fadd. Pulling origin back to a statement (K4) drops to 65.2% (fmul delayed past memcpy).
  - Balanced `FstF(hi, t0/len2)` + `OtG` last: 65.2–65.7% (fadd-over-fmul). hi2-in-helper vs hi2-as-statement is the same ST/prologue pair as the prior last-arg wave.
  Coupling unchanged: helper RTL can pin `out_t` before `t0` but cannot emit the missing `push esi` without starting the solver call, and starting the call early still hoists fstp/fadd (B) or flattens (A). Slope/Shade not reopened.
- **TrackShade_FillPoly**: **ZBuffer floor**, same class as schoolcar6.c `ZBuffer_FillPoly` (EBP frame, `xchg ebx,eax`, `add ebx,1`, mixed `__asm`). 254/254i, 748/771B, frame 0x6c vs 0x70. Not ground further.

## Extern-type divergences

- `TrackCurve_EvaluateOffset` / `EvaluateDerivative` / `EvaluatePosition` / `EvaluateUp` take `float t` here; coaster9.c uses `int t` on some of these.
- `g_curve_offset` 0x00615fd4 is the rail offset AND the stepper tolerance (RouteCar_SetPosition passes 4.8 for both).
- `CoasterTex_Get` 0x00420780 / `BitLowestSet` 0x00428840 are owned by LL4 / LL6; declared here for the shaded filler only.
