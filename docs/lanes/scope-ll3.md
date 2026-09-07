# Scope LL3 — route / joint / span clip callees

Branch `scope/LL3`. File `LEGOLAND/coaster11.c`. Object prefix `/tmp/sll3_`.
Brief: `docs/SCOPE_LL3_route_joint_span.md`.

## Status

| address | name | insns | % | audit | marker |
| --- | --- | ---: | ---: | --- | --- |
| 0x0041f030 | Span_SetVertexBuf | 4 | 100 | [OK] | FUNCTION |
| 0x0041cd20 | JointSlot_TestSquare | 7 | 100 | [OK] | FUNCTION |
| 0x0041e7f0 | RouteCar_GetHeading | 10 | 100 | [OK] | FUNCTION |
| 0x0041f4c0 | Span_AllocPairs | 13 | 100 | [OK] | FUNCTION |
| 0x0041e8f0 | RouteCar_PlaceAndBind | 25 | 100 | [OK] | FUNCTION |
| 0x0041cd40 | JointSlot_Find | 27 | 100 | [OK] | FUNCTION |
| 0x0041cd80 | JointSlot_Set | 45 | 100 | [OK] | FUNCTION |
| 0x0041db20 | Route_CollectCarSample | 37 | 100 | [OK] | FUNCTION |
| 0x0041ede0 | MapCell_AllowTrack | 38 | 100 | [OK] | FUNCTION |
| 0x0041d210 | TrackFitFindPartners | 60 | 100 | [OK] | FUNCTION |
| 0x0041f2b0 | Raster_ClipAgainstPlanes | 55 | 100 | [OK] | FUNCTION |
| 0x0041d950 | Route_SetTrainAt | 66 | 100 | [OK] | FUNCTION |
| 0x0041f3e0 | Span_FillEvalTable | 85 | 100 | [OK] | FUNCTION |
| 0x0041ee40 | TrackPlace_TestSquare | 79 | 100 | [OK] | FUNCTION |
| 0x00411fa0 | LFQueue_StepRider | 74 | 100 | [OK] | FUNCTION |
| 0x0041ef60 | Raster_ClipPoly | 80 | 100 | [OK] | FUNCTION |
| 0x0041db90 | Route_GetMassAndPower | 77 | 84 | 42 mis | WIP |
| 0x0041c940 | BsRoute_Trace | 130 | FLOOR | 105 mis | WIP |
| 0x0041f050 | Span_ClipPlane | 179 | 9 | 178 mis ESCAPES | WIP |

**16 / 19 exact.** Relocs on FUNCTION bodies: 0 MISMATCH. `/W3` clean.

## Names

Caller-given names kept: `JointSlot_Set`, `TrackFitFindPartners`,
`Route_SetTrainAt`, `LFQueue_StepRider`, `BsRoute_Trace`,
`Route_GetMassAndPower`, `Raster_ClipPoly`. New: `Span_SetVertexBuf`,
`JointSlot_TestSquare`, `RouteCar_GetHeading`, `Span_AllocPairs`,
`RouteCar_PlaceAndBind`, `JointSlot_Find`, `Route_CollectCarSample`,
`MapCell_AllowTrack`, `Raster_ClipAgainstPlanes`, `TrackPlace_TestSquare`,
`Span_FillEvalTable`, `Span_ClipPlane`.

## Mechanics

- **Span_SetVertexBuf**: stores PolyVtx stride 0x1c at 0x004b5608 and the
  live vertex pointer / lerp dword bound at 0x004b560c. ClipPoly passes
  `n + 1` (an integer bound, not a pointer).
- **JointSlot_TestSquare**: forwards (square, probe-ctx) to 0x0041ee40.
- **RouteCar_GetHeading**: copies RouteNode +0xb8..+0xc0 (dx/dy/dz).
- **Span_AllocPairs**: allocator at +0x24 gets `(n+1)*(n+2)/2` slots.
- **RouteCar_PlaceAndBind**: `RouteCar_SetPosition` then re-bind both bogie
  cursors via 0x0042a5e0.
- **JointSlot_Find**: walk the slot's four PackedSquares; only bits set in
  the mask are compared. Returns the index or -1.
- **JointSlot_Set**: four direction bits; each live bit writes
  `base + g_joint_delta[i]` and keeps the bit if the probe returns 1.
- **Route_CollectCarSample**: `PositionRouteCars` on `g_route_eval`, then
  every node of the +0x70 ring (including the head) contributes velocity
  into +0x04 and a heading Vec3f; count = 1 + 3 * nodes.
- **MapCell_AllowTrack**: NULL and flag 0x40 succeed; no 0x8f8 bits, the
  environment class, flag 0x800, or class flag 0x200000 on a 0x88 cell
  refuse.
- **TrackFitFindPartners**: refuse closed circuit (state 2); stamp owner
  `&g_castle`; head offset +0x18/+0x1a against head_slot, tail +0x10/+0x12
  against tail_slot; a hit writes `1<<index` into jout.dir / jin.dir.
  Fail only when both indices are -1.
- **Route_GetMassAndPower**: save `rt->pos` / `rt->f24`, set `g_route_eval`,
  copy pos to `g_route_eval_at`, call 0x0041f4e0 (`Span_EvalRange`) with
  `Route_CollectCarSample`, ops at 0x004d8270, saved `f24`, `0.1f`. `*power`
  is the sample energy; `*mass` is the sum over cars of
  `|heading|^2 * GetAcceleration * 2.52015616e-06f` (0x4ab400). Restore
  pos/f24; push `*mass` into the 64-slot ring at 0x004d829c.
- **Raster_ClipPoly**: mask 0xf is a no-op (`*count = 3`). Otherwise
  `SetVertexBuf(n+1)` and clip against the low/high nibble of the mask:
  flags 3 → 4 planes at ctx+4; 2 → 2 planes at ctx+0x1c; 1 → 2 planes at
  ctx+4. flags==0 (both nibbles full, only reachable if mask!=0xf were
  possible) returns `count`, not `v`.
- **Span_FillEvalTable**: alloc `(n+1)*(n+2)/2` via vtable +0x20, fill row
  pointers at 0x004d88cc, evaluate `eval` at n+1 samples centred on `a`
  with step `b`, then combine adjacent pairs down the rows via +0x04.
- **Span_ClipPlane**: close `in[n] = in[0]`; negative plane distance is
  inside. Both-inside emits prev; leave emits prev + lerp; enter emits
  lerp. Lerp walks dwords `0..g_span_vtx` through `__ftol` (0x00458930)
  into `*cursor` and advances by `g_span_vtx_stride`.
- **BsRoute_Trace**: twin of `JungleCruise_TraceRoute`. DFS over the
  school's water graph (step 5); west is a tail-call that VC6 turns into
  a loop. `*ok` is a success flag.

## Levers

- **RouteCar_GetHeading**: mutate the `n` parameter (`n = (RouteNode*)((char*)n + 0xb8)`)
  to emit `add eax, 0xb8`; a derived `Vec3f*` folds to `[eax+0xb8]`.
- **Span_AllocPairs**: `(m * (m + 1)) >> 1` with `m = n + 1` emits
  `inc / lea / imul / sar`. `/ 2` becomes `cdq`.
- **JointSlot_Set**: `for (i = 0; i <= 3; i++)` (not `i < 4`) so the
  strength-reduced byte cursor compares `esi, 0x10 / jle` rather than
  `0x14 / jl`. Probe args are `(square, &g_joint_probe)` — cdecl
  `push 0x4b5570 / push dst`.
- **JointSlot_Find**: `int i = 0; int bit = 1; int mask = *p` — **bit
  before mask** so TEST is `test ecx,esi`. Mask-first (any `&` spelling)
  emits `test esi,ecx` and is 26/27. First insn is `mov edx,[esp+8]`.
- **Route_CollectCarSample**: `count = 1` before `n = &rt->head` so
  `mov ebp,1 / lea esi,[eax+0x70]` sit between the
  `PositionRouteCars` pushes and the call (esi/ebp are callee-saved).
  Loop latch reloads `&g_route_eval->head`, not a cached pointer.
- **TrackFitFindPartners**: one trailing `return 0` (`if (state != 2) {
  ...; if (a != -1 || b != -1) return 1; } return 0;`). Local
  `PackedSquare at` reuses the dead `d` argument slot.
- **Route_SetTrainAt**: same walk as PositionRouteCars but
  `RouteCar_PlaceAndBind`; CoasterRoute +0x20 is the unused `dimension`
  word so `f24` stays at +0x24 and `head` at +0x70.
- **Raster_ClipAgainstPlanes**: dest-relative dword copy
  (`edx = src - dest; [edx+ecx]`), then ping-pong `g_clip_ping[i&1]` /
  `[(i-1)&1]`. Latch is `i++; planes += 0xc` (not a for-increment).
  Reuses `n` as the clip result so the next plane sees the survivor count.
- **TrackPlace_TestSquare**: do-while over the PlaceRect list (first
  node is dereferenced with no NULL test — original bug). `int x =
  sq->x; int lim = sq->x + ctx->x1; x += ctx->x0` (textual `sq->x`
  repeat) emits `mov esi,eax / add esi,edx`. A named `sx` fuses to
  `add esi,eax` (191B, 96%).
- **Raster_ClipPoly**: `if (1) { switch (flags) { return Clip(); } }
  return count`. The constant-true wrapper is the layout hammer:
  `dec/jne` + case-3-inline, no tail-merge, case 1 colors g in ecx.
  Bare switch puts default first (`je case3`); `if (flags)` is the
  same layout plus a `test edi / je`. `r = count; break` hoists
  count into eax and `dec ecx`. Named `p = g` in case 1 alone
  un-merges but colors g in eax (193B vs 194B).
- **LFQueue_StepRider**: field stores `b->target.x/y = to.x/y` then
  `CalcMoveLine(b->world, b->target, pathp)` emit both stores before
  the pushes, `shl eax,8 / shl ecx,8`, then `mov eax,ecx /
  mov ecx,[target.x]`. Aggregate `b->target = to` plus
  `CalcMoveLine(..., to)` interleaved stores with the pushes.
- **BsRoute_Trace** (**FLOOR**): 130i/339B byte-exact, 105 mismatches.
  Identical NG22 phase-order ALLOCATION as `JungleCruise_TraceRoute`
  (jcroute.c / LEVERS NG22). Original: esi=x, edi=y, ebp=x1, ebx=y1, `w`
  in the dead arg3 slot `[esp+0x1c]`. Ours converts the west tail-call
  to a loop *before* allocation and ranks the dereferenced pointers
  first. Dead trailing statements restore the int ranking but emit a
  real call instead of the loop. No source spelling has both. Retired.
- **Span_FillEvalTable**: `int count = n` first puts n in ebx.
  `*(volatile int*)&a = count` homes the rows counter on the dead `a`
  slot. Latch is a **block-local** `left = *(volatile int*)&a` then
  `count--; row++; left--; store` so the load is `mov eax,[slot]`
  interleaved with `dec ebx / add edi,4`. Function-scope `left` steals
  ebx (65%). `--*(volatile int*)&a` after the pair uses ecx (5 mis).
- **Route_GetMassAndPower** (WIP, 84%): `{f24, pos, sample[21]}` is the
  0x6c frame. Pre-call `p = rt` puts rt in eax; `acc = 0.0f` after
  `*mass = 0` homes the heading sum in the dead power slot
  (`fstp [esp+0x88]`). `q = *(CoasterRoute* volatile*)&rt` after
  `GetAcceleration * acc` stops p/rt coalescing so mass stays ebp.
  Plain `q = rt` coalesces back to ebp (70%). Residual: `mov ebx,eax
  / add ebx,0x70` vs `lea ebx,[eax+0x70]`; q load after `add esp,4`
  not before; eax vs edx for the reload; hist ecx/edx swap.
  **LL2 `LFUpd_Fst` RTL does not move this body.** The 65/77 attractor
  dest-coalesces n onto a callee-saved copy of p (`mov ebx,eax` in the
  first `rep movsd` setup, `add ebx,0x70` sunk before `Span_EvalRange`).
  `lea ebx,[eax+0x70]` can only be selected while eax still holds p
  (before `mov eax,[f24]` for the call). Transparent `return a` helpers
  (unused 2nd arg, by-value `RoutePos`, int `base+0x70` in the body,
  `Mass_End` in the latch, `Mass_FstQ`/`Mass_FstF` around the call or
  the K-mul, two-web `t`/`n`, delayed `n = &p->head`) all fold back to
  65/77. Used 2nd-arg side effects (`dst=&fr.pos`, `fr.pos=p->pos`)
  likewise. Volatile `head` / `n` spill (frame 0x70, 50/79). `head-0x64`
  as the copy source mutates eax (`add eax,0x70` / `lea esi,[eax-0x64]`,
  52/78). Volatile `fr.f24` makes `mov`/`add` adjacent but does not fuse
  to lea and breaks the 2nd `rep movsd` interleave (64/77). `Mass_FstF`
  with `q=` as the 2nd arg of the *call* pulls q into ebp before
  GetAcceleration (61/78). q-in-edx + hist-ecx still only with extra
  volatiles (80i). LL2's helper closes a 3-scratch `lea edx,[eax+ecx]`;
  this residual is dest-coalesce of `reg+disp8` plus a post-call
  edx/eax coloring, not a last-def SIB.
  Further negatives (still 65/77 unless noted): decl-init `n=&rt->head`
  without a `p` copy; named `&p->pos` plus comma `(src=&pos, &head)`;
  typed `sizeof==0x70` prefix increment `(MassOff*)p+1`; `Mass_Head`
  helper that `return &r->head` after a cdecl `&pos` second arg;
  `register RouteNode* n`. `n=&rt->head` *after* `Span_EvalRange` keeps
  rt in ebx from the first load (56/76). Named `end` plus hist
  `i` then `float m=*mass` uses `fld`/`fstp` for the ring store (64/77).
  FindFreeSeat's `lea edi,[eax+0x70]` has a dead eax after the lea;
  this body's eax must stay live for `[eax+0x24]` / `g_route_eval=eax`,
  which is exactly the dest-coalesce shape.
  **Live-eax sibling is SetTrainAt / PositionRouteCars, not FindFreeSeat.**
  Both already exact in this file / schoolcar.c: `mov ebx,[eax+0x158]`
  (`n = rt->head.next`) then `lea ebp,[eax+0x70]` / `mov [eax+0x24],ecx`.
  Mini-morphs emit the wanted `lea ebx,[eax+0x70]` while eax stays p only
  when n is *used before* Span_EvalRange (extra PlaceAndBind) or when a
  simple body keeps nxt live in a callee-saved. On this 77i body the same
  decls dest-coalesce again, or spill nxt (frame 0x70, 51/80). Pipelined
  `n=nxt; nxt=n->next` loads `[eax+0x158]` into ebp but still
  `mov ebx,eax / add ebx,0x70` (59/80); mass moves to edi. Immediate-use
  is the real lea lever; Mass has no original call/push of n before
  `mov eax,[f24]`. q-before-`add esp,4` still only with extra volatiles
  or a pre-call q (ebp, 61/78). CollectCarSample's no-use lea cannot host
  `lea ebx,[eax+0x70]` here: Mass kills eax for f24 before the call.
  Trace / ClipPlane not touched.
- **Span_ClipPlane** (WIP): 179i, ESCAPES. Need the original's 0x2c frame,
  `in++` cursor in the latch, and the three-way sign classify
  (`(prev_sign>>1)|next_sign` against 0x80000000 / 0xC0000000 / 0x40000000).
