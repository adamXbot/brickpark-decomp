# Codex-F — in progress, 25/26 (2026-09-07)

Branch: `codex/scope-f`.
Worktree: `.worktrees/codex-f`.
Owned files: `LEGOLAND/uistubs2.c`, `LEGOLAND/coaster10.c`,
`LEGOLAND/ridemachine2.c`, this note.
Object prefix: `/tmp/cf_`.

## Status

| File | Exact | WIP | Total |
| --- | ---: | ---: | ---: |
| `uistubs2.c` | 5 | 0 | 5 |
| `coaster10.c` | 16 | 0 | 16 |
| `ridemachine2.c` | 4 | 1 | 5 |
| **lane** | **25** | **1** | **26** |

`/W3` clean. Exact bodies: audit `[OK]`, relocs zero MISMATCH.

## Scoreboard (all 26)

### `uistubs2.c` — 5/5 exact

| Address | Function | Insns | Audit |
| --- | --- | ---: | --- |
| `0x0046d390` | `SetHelpFaceState5` | 2 | OK |
| `0x00492da0` | `RestartMusic` | 5 | OK |
| `0x00457870` | `SetBrickLimit` | 6 | OK |
| `0x00489ee0` | `ClearMarkedTiles` | 6 | OK |
| `0x00499410` | `ResetGameClock` | 7 | OK |

### `coaster10.c` — 16/16 exact

| Address | Function | Insns | Audit | Residual |
| --- | --- | ---: | --- | --- |
| `0x0041e640` | `RouteNode_SetPending` | 11 | OK | — |
| `0x00429b90` | `TrackCurve_EvaluateUp` | 11 | OK | — |
| `0x00420fb0` | `CoasterModel_GetClipRect` | 12 | OK | — |
| `0x0041e670` | `RouteNode_CanAdd` | 14 | OK | — |
| `0x00426510` | `Mat3_TransposeToMat4` | 18 | OK | — |
| `0x00426700` | `ClipRect_SetBounds` | 19 | OK | — |
| `0x00429a80` | `TrackCurve_EvaluatePosition` | 25 | OK | — |
| `0x0041dca0` | `Route_GetSpeed` | 29 | OK | — |
| `0x0041ea70` | `RouteNode_LinkPending` | 40 | OK | — |
| `0x004261c0` | `TransformVec3` | 42 | OK | — |
| `0x00429c60` | `TrackCurve_EvaluateDerivative` | 42 | OK | — |
| `0x00422400` | `ModelImage_FindName` | 43 | OK | — |
| `0x0041e9e0` | `RouteNode_GetTransform` | 45 | OK | — |
| `0x00420810` | `CoasterModel_DrawPass1` | 161 | OK | — |
| `0x00420a20` | `CoasterModel_DrawPass2` | 165 | OK | — |
| `0x00420c40` | `CoasterModel_DrawPass3` | 182 | OK | — |

### `ridemachine2.c` — 4/5 exact, 1 WIP

| Address | Function | Insns | Audit | Residual |
| --- | --- | ---: | --- | --- |
| `0x00404860` | `Copters_StepCar` | 23 | OK | — |
| `0x0043a940` | `SpaceTower_StepCar` | 37 | OK | — |
| `0x004049a0` | `Copters_StopRide` | 71 | OK | — |
| `0x0043b810` | `SpaceTower_UpdateRiders` | 116 | OK | — |
| `0x00404630` | `Copters_UpdateCarRider` | 168 | WIP | 64.9% full, 86/168 strict; seat spill tie |

## Closed this wave (levers)

### `TransformVec3` 0x004261c0 (78.6% -> exact)

MatMul house style (coastermath.c): both operands are direct subscripts
with the row index folded in, `m->m[k*4+j] * ((float*)s)[j]`, the output is
`((float*)d)[k]` with `Vec3f* d` and `d++` after the k loop. That gives
ebp=s / ebx=k-count, the sp-then-rp preheader order and `fld [rp]` first.
Every walked-pointer spelling (`*rp++`, `*sp++`, a `float* d` cursor)
permutes one of the three; the earlier `float* d` fix traded the ESCAPES
spill for the ebp/ebx swap.

### `TrackCurve_EvaluateDerivative` 0x00429c60 (33% -> exact)

The residual was a wrong reading of the globals, not a schedule. 0x615fd4
is the float `offset` handed to `TrackCurve_DerivSample` (a `mov` of h's
raw bits), not an out pointer, and 0x615f98 is the 3-dim PhysVec ops pool
(`PhysVec_InitOps(&, 3)`, schoolcar.c). `g_deriv_offset = h;` then
`FiniteDifference(TrackCurve_DerivSample, &g_deriv_physvec_ops, t, 0.01f,
&scratch)` with `PhysVec scratch` and `out->x = scratch.v[0]` ... gives the
early `mov edx,out` interleave and the `fld`/`fstp [eax]` copy for v[0]
(eax holds `out`, only ecx/edx remain for v[1]/v[2]) for free. The comma
`(g_deriv_out = out, fn)` trick was a dead end.

### `Copters_StopRide` 0x004049a0 (85.9% -> exact)

Plain `rec->seat[n].flags &= ~1u;` statements in the copter enumeration
order 1,0,2,3,4. VC6 pairs them (edx,ebx) and emits the SECOND of each pair
first, so the emitted 0/1, 3/2 picture IS source order 1,0,2,3,4; explicit
edx/ebx temporaries written in emitted order come out mirrored. The
`xor ebx,ebx` sits right after the seat[4] flags store only when the rider
stores PRECEDE the frame stores in source.

## WIP notes (latest)

### `Copters_UpdateCarRider` 0x00404630 — 86/168 strict, 64.9% full

Body is now the natural form (no volatile, no punned parameter): `seat =
&rec->seat[index]`, switch on `index` with `anim = 1` default, `kf1`/`kf`
frame pointers (`slots[anim] + frame*0x30`), cross product `b x a` as three
statements, `for (i) { oa = ord_a[i]; for (j) { f = slots[anim][(oa +
seat->frame*4 + 1)*3 + ord_b[j]]; FSCALEF(f, scale); m[j*3] = sign_a[j] *
sign_b[oa] * *(int*)&f; } m++; }`. Jump table, frame 0x28, all call
sequences, both loop shapes and the switch layout match.

Residual = ONE allocation decision. The original does not enregister
`seat`: it spills at the def into the dead `rec` slot (`lea edi,
[ecx+eax+0x18] / mov [ebp+8],edi` before the first call), keeps edi through
the pre-loop code, and reloads `mov edi,[ebp+8]` at the head of every inner
iteration; that frees esi for `oa`, ebx for `anim`, and lets `scale` share
screen.oy's slot ([ebp-0xc]) while ip/m spill to [ebp-4]/[ebp-8]. This
build keeps seat in esi, gives anim edi, spills `oa` to [ebp-0xc] and m to
[ebp+8], and puts scale at [ebp-8]. Products 2/3 of the cross product also
emit `fld [eax+0x10] / fmul [eax+0x20]` where the original has 0x20 first.

Measured on this body (all inert unless noted):

- Adding one inner-loop store `m[j*3+1] = 0` (or `= *(int*)&f`) FLIPS the
  whole picture to the original's (seat edi + [ebp+8] spill and per-iteration
  reload, anim ebx, chain esi, scale at [ebp-0xc]) at 113/174 — the decision
  is a near tie the original's source breaks with something that emits no
  code. `m[j*3+1] = oa` also flips (esi+spill); `+ oa` in the product,
  `g_copter_sign_a[j] = oa`, and `= j` do not.
- `*(CopterSeat* volatile*)&rec = &rec->seat[index]` with a punned `SEAT`
  homes seat in [ebp+8] and gives the exact loop body (102/176), but the
  address is computed after the call and reloaded once (8 extra
  instructions). Plain `void* rec` / `CoptersRec* rec` reassignment (before
  or after the call, with or without an `inst` copy) allocates seat like a
  local (62–85). `(*(CopterSeat* volatile*)&seat)->frame` at the loop use
  only: 88, seat still esi.
- Outer loop as a walked pointer `for (ip = ord_a; ip < ord_a+3; ip++)`
  swaps seat/anim to edi/ebx (the original's registers) at the same 86 but
  reloads `*ip` inside the inner loop and rewrites m as m-ip.
- Inert: `register` on oa/anim/layer/i/j; unsigned anim/mode/oa; volatile
  scale; declaration order of scale (slot stays [ebp-8]); dead
  `dummy = seat->frame` / `dummy = oa`; explicit inner `mp` cursor (`*mp =
  ...; mp += 3`), `m[j*3+i]`, 2-D `[j][i]`, `*m = ...; m += 3; m -= 8`;
  `g_copter_ord_a[i]` written twice instead of `oa` (50); product order
  `f*sa*sb`, `sb*sa*f`, `sa*(sb*f)`; Vec3 struct / `V3 v[3]` spellings for
  kf1/kf/loop (identical IR); mode default-initialised before the switch
  (44–86, all worse); a `default:` arm; single-block asm, `dword ptr`
  operands, union `{float f; int i;}` for f, separate int for fistp (61).
- FP phase: `while (0) { mode++; }` anywhere before the FP block shifts
  products 2/3 one step (fp 00 -> 10, 90/168) with no emitted code; two of
  them do not shift further. `do {} while (0)`, `if (0) {...}`, `for (;0;)`
  and dead arithmetic are inert. The original's source has one more
  surviving IR expression before the cross product than this body.

Next: look for a value the original keeps in memory or an expression that
survives to value numbering but folds before emission (e.g. a helper that
reads `m` back, a `frame` temporary the loop re-derives, or the lift
computed through a struct field the front end does not flatten).

## Remaining

Close `Copters_UpdateCarRider`. No merge until 26/26 or explicit ask.
