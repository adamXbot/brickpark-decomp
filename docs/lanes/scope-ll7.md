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
| 0x00429f30 | Track_StepAlong | 70 | 100 | [OK] | FUNCTION |
| 0x00429560 | TrackRunSetSlope | 90 | 100 | [OK] | FUNCTION |
| 0x00428860 | TrackShade_FillPoly | 254 | 69 | 254/254i, 765/771B, 176/254=69.3%, **0x70, exact through imul** | WIP |

**16 / 17 exact.** FillPoly is size-exact (254i) with the original 0x70 frame; matchfull 176/254 = 69.3%. Residual is the swapped crow/zrow loads after the product, then nshade-in-eax before the crow store. `/W3` clean. `relocs.py` 0 MISMATCH on the 16 FUNCTION bodies.

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
- **TrackShade_FillPoly** 0x00428860: `SpanFiller(tag, grad, nkeys, keys, edges)` from Raster_SubmitPoly. Increments `edges[keys[n-1].idx].y1`, looks up `g_coaster_tab_c[tag]` (0x00420780), bit-scans the two header dwords (0x00428840), writes colour into `g_raster_bits` and depth into `g_zb_base`. Inner span is `__asm`. `last` is `&keys[n-1].idx` (`lea [eax+edx*8-4]`). `y` is a bare local (`[ebp-4]`); the other seven setup dwords sit in reversed-layout `ShadeSetup` (`crow` → `[ebp-0x20]`). Size-exact **254/254i**, matchfull **176/254 = 69.3%**, frame **0x70**. Exact through the `imul eax,[ebp-4]` product (insn 85). Residual: original is `mov edx,[crow]; mov ecx,[zrow]; shl; add; add; store crow; nshade-in-edx; test; store zrow`. Ours swaps the two base loads then hoists nshade into eax before the crow store. `ne` is a MASM reserved word — the parameter is `nkeys`.
- **g_step_lo2** is `(step-tol)²`, not `(t-tol)²`. hi2 is `(step+tol)²`.

## Levers

- **TrackFitSpanGeom**: named `head_opp` / `tail_opp`. Nested Opposite calls split `add esp` (70%).
- **Track_Bisect**: `*(unsigned*)&pa ^ *(unsigned*)&pb` (pa first in the xor) lands the original `edx=[esp+0x14], ecx=[esp+0x10]`. A `pm` local is required so the mid-sample does not reuse `pb`'s slot (reuse dropped to 85%).
- **TrackRunSetSlope**: **exact** (90i/302B). The 7 eax↔edx residual was the latch spelling, not a volatile/remain floor. `if (steps > 0) { do { … } while (--steps); }` colours the `--steps` IV into eax and the loop zero into edx. `for (; steps > 0; steps--)` emits the same 90 instructions / 302 bytes but flips the pair (`edx=steps`, `eax=0`, `dec edx`). Operand swap (`0 < steps`), goto-skip, `return 0` coalescing (+1 xor), entry `remain`, and a post-ramp `remain = steps` did not. The earlier one-temp/volatile sweep still stands as negatives for that do-while shape.
- **Track_MeasureDistance**: extra `g_dist_at = &cur` before the loop integrate and the final `[t0, t]` integrate (equal path sets it to `from`). 0.01f is `0x3c23d70a`.
- **TrackCursorPair_Draw**: interleave `s=sin; m[0]=s; c=cos; m[2]=c; m[8]=-c; m[10]=s` so the leftover sin is `fst` then later `fstp`. Computing both trigs first emitted `fld st(1)`. Mat slots are 0/2/8/10 (not 1/2/8/10). `#pragma intrinsic(sin, cos)`.
- **Track_StepObjective**: `if ((dist2 = x*x+y*y+z*z) > hi2)` (assignment-in-condition) lands `fld st / fcomp hi2`. A named `dist2 = sum; if (dist2 > hi2)` emitted `fcom [home]`.
- **Track_StepAlong**: **exact** (70i/232B). `org = origin` live-across + late `g_step_origin = org` puts origin in edx during `rep movsd`. `hi = tol; … hi = t` + `t0 = cur.geom->t0` before `g_step_len2 = step * step` lands `push esi` immediately after the geom/out_t loads, `push tol` as the hi placeholder, dword `t0` into the dead `from` slot, and `fstp [esp]`. Loop is exact (`hi = cur.geom->t1`). The named `float step2 = step * step` CSE was the 11-mismatch wall: it kept step² as a live local, finished t0+fstp as pre-call statements, and batched `push esi` with `push tol` (attractor A). Writing the square only at the store (`g_step_len2 = step * step`) after t0 keeps prologue `fld/fmul` on ST and lets the solver call start between geom/esi and the t0 load. Attractor B (`g`; `len2`; `t0`) still hoists fstp/fadd before geom. Older negatives (helpers/comma/union/Fst RTL) left in git history (`7c7640a4` and earlier); they do not apply once the named temp is gone.
- **TrackShade_FillPoly**: 0x70 frame, 254/254i, matchfull 176/254 = 69.3%. The `g_zb_polys++` ebx-vs-eax / hoisted-y residual closed by splitting `y` out of `ShadeSetup` and address-taking it with a late `y = *(int volatile*)&y + 1` (ZBuffer's `*(int volatile*)&y` on a struct field grew the frame to 0x74 and made `y` an imul temporary). `keys[*(int volatile*)&nkeys].y` plus `keys = *(SortKey* volatile*)&keys` before the outer loop put keys in edx (not ebx) so the sentinel is `mov [edx+ecx*8],eax`. Named `py = pitch * y; py <<= 1; crow = (char*)c + py` lands `imul eax,[ebp-4]` and `shl eax,1`. Volatile zrow *after* the crow store was the 68.9% attractor (exact through insn 86, shl before zrow). `py = *(int volatile*)&s.pitch * y` then volatile zrow is the 69.3% pin: exact through imul, then `shl` + both `add`s match, but the two base loads are swapped (`zrow` then `crow`) and nshade still takes eax before the crow store. ZBuffer's volatile-y multiply flipped the imul operands (`imul eax,[pitch]`). Ordinary named `c`/`z` together hoist crow into edi before the sentinel (and with volatile pitch grow a 0x74-like extra slot). Comma nshade onto the crow store swaps the add registers (66.9%). Volatile crow (`*(short* volatile*)&s.crow`) delays `push edi` (firstX=8). `py & 0` / `py * 0` dependencies, dummy `dead`, `&s.y` / `volatile int y` field, and `int slot[8]` stay ruled out. Residual: crow-before-zrow load order, then nshade-in-edx after the crow store.

## Extern-type divergences

- `TrackCurve_EvaluateOffset` / `EvaluateDerivative` / `EvaluatePosition` / `EvaluateUp` take `float t` here; coaster9.c uses `int t` on some of these.
- `g_curve_offset` 0x00615fd4 is the rail offset AND the stepper tolerance (RouteCar_SetPosition passes 4.8 for both).
- `CoasterTex_Get` 0x00420780 / `BitLowestSet` 0x00428840 are owned by LL4 / LL6; declared here for the shaded filler only.
