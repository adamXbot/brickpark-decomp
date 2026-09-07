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
| 0x0041f050 | Span_ClipPlane | 179 | 33.9 | latch jne; ebx=n; and ebx abs; frame 0x2c; 606B | WIP |

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
  **2026-09-08 interleave hypothesis (still 65/77, lea ebx did not land).**
  Original slot is `lea edi,[esp+0x14] / lea ebx,[eax+0x70] / rep movsd`
  with eax still p for `[eax+0x24]` and `g_route_eval=eax`. Ours fills that
  slot with `mov ebx,eax` and sinks `add ebx,0x70` to the last push before
  `Span_EvalRange`. Reorder `fr.pos=p->pos; n=&p->head; fr.f24=p->f24`
  (and named `src=&p->pos`, comma, two-step `node` then aggregate, rt
  without `p`) all CSE back to the attractor. 3-arg / return-keep
  `MassSnap` helpers DCE the unused keep and dest-coalesce. Short-lived
  `p->head.next` DCE; used nxt steals ebp (59/80). `g_route_eval=p` then
  `n=&g_route_eval->head` either `add eax,0x70` (54%) or still mov/add
  (56–72%). `n` only in the EvalRange comma mutates eax (54%). Extra
  PlaceAndBind on this 77i body still mov/add (56/83) — the mini-morph
  lea does not survive. Volatile `fr.f24` makes mov/add adjacent (64/77)
  but does not fuse to lea; pos-then-head does not change that.
  q-before-`add esp,4`: comma / `while (n!=&(q=reload)->head)` /
  `if ((q=reload)!=0)` leave the load after cleanup (or add a test).
  Hist `edx=*mass; ecx=i&0x3f`: named `m`/`i`/`slot` become `fld`/`fstp`
  (64/77); int-bitcast, post-inc, and SetSlope do-while keep the ecx/edx
  swap. Best remains 65/77. Trace / ClipPlane not touched.
  **2026-09-08 ClipPlane imm8 store (reverted):**
  `*(volatile unsigned char*)&p->head = 0` after pos/f24 forces delay-slot
  `lea ebx,[eax+0x70]` (66/77) but always emits `mov [ebx],0`. Dropping the
  store dest-coalesces again. Coupled on this body — cannot exact with the
  store, cannot lea without it. Body restored to 65/77.
- **Span_ClipPlane** (WIP): 64/189 (33.9%), latch jne-to-header, frame **0x2c**,
  ebx=n held, loop abs `and ebx,0x7fffffff`. Reconstruct
  notes (2026-09-08): trailing early-out after `in[n]=in[0]`; `in++` then
  `left=n` with latch `in+=4; dec left; jne`; signed classify
  `sar1/and 0x40000000/or` vs 0x80000000 / 0xC0000000 / 0x40000000; divide
  is `fld` of bit-abs (`and 0x7fffffff`), not fild/abs. Prologue must load
  cursor, then **ebx=n**, then `*cursor`→edx — deref-first steals ebx and
  blocks 0x2c homes.
  **2026-09-08 ebx=n wave (did not land).** Decl order, `int left/nn = n`
  live into `--n`, comma `*(n, cursor)`, `in+n`, `&out` after n, and
  reading n before `*out`/`*cursor` all still colour `mov ebx,[cursor]`.
  n goes to esi/edi. Homing dest (volatile / `*cursor` only) moves the
  deref to eax/ecx but then **prev_abs or out takes ebx**; n stays esi.
  `--n` live across `__ftol` overlaps next_abs, so VC6 will not reuse
  ebx as original does (n prologue → spill left → ebx=next_abs).
  Need n's live range to **die at the left home** before abs is computed,
  without a long-lived dest pointer competing for ebx. Still 18/195,
  frame 0x24. Mass/Trace not touched.
  **Left-home wave:** left spill lands (`[esp+0x1c]` after cmp/in++); dest
  off ebx still leaves ebx to plane → prev_abs → xor-zero, never n (esi).
  Best transient 30/191; tip restored.
  **n/next_abs ebx wave:** `mov ebx,n` / `cmp ebx,1` / `[esi+ebx*4]` landed.
  dest+plane+out homed, prev_abs and in edx, out_n is `mov [esp+0x10],0`.
  Cursor still wants ebx for the early-out `*cursor` store; a byte store of
  n (`*(volatile unsigned char*)&left = (unsigned char)n`) forces ebx
  because only ebx is byte-addressable among callee-saves. next_abs and
  stays edx (not the original `and ebx,0x7fffffff`). Frame still 0x24.
  25/189 (13.2%). Mass/Trace not touched.
  **2026-09-08 next_abs→ebx wave.** `and ebx, 0x7fffffff` landed. After
  `left = n`, reuse `n` for abs (`n = bits.i; next_sign = n; n &= 0x7fffffff`)
  and keep that web live across `__ftol` by storing `prev_abs = n` *after*
  the classify/lerp (lerp dword is a separate temp so `n` is not overwritten).
  Short-lived abs stays edx; callee-save abs wants edi; a byte store of the
  abs value (`*(volatile unsigned char*)&abs_b = (unsigned char)n`) forces
  ebx (edi is not byte-addressable). Prologue `mov ebx,n` kept. Extra
  dest/dlt homes steal ebx back. 8-byte spill / k-up reach 0x28 not 0x2c.
  20/195 (10.3%), 588/593B, frame 0x24. Mass/Trace not touched.
  **2026-09-08 0x2c frame wave.** Original 0x2c locals after 4 pushes:
  `[esp+0x10]` out_n, `+0x14` dest, `+0x18` k/delta, `+0x1c` left, `+0x20` cls,
  `+0x24`/`+0x28` dlt↔dest-rel, `+0x2c` bits/next_abs, `+0x34` prev_abs;
  **unused `+0x30` and `+0x38`**. Arg reuse: n-slot→in, in-slot→t, out updated
  in place. Extra dest/dlt homes steal ebx. Volatile plane floats used in
  the fmul open 0x2c but rewrite `[ecx]`/`[ecx+4]` operands (score stays 20,
  more insns). `bits` as `double` union is 0x28 / 27/195. The unused pair
  is an 8-byte prev_abs `{int i; int pad;}` — pad is ebx-neutral and does
  not replace dest/dlt. `sub esp,0x2c`, 20/195 (10.3%), 195i, ebx-n and
  `and ebx,0x7fffffff` held. Mass/Trace not touched.
  **2026-09-08 trailing-jl + count-up lerp.** `if (n >= 1) { body; return }
  *cursor=dst; return` exiles the early-out (`cmp ebx,1 / jl` past the
  fall-through ret). Both lerp arms are `k=0; k++; while (k <= g_span_vtx)`:
  ENTER keeps `k` outside the vtx guard so it spills (`mov [slot],0`);
  LEAVE inits `k` inside the guard (ebp / inc). Goto continue-header lost
  `and ebx` (abs fell to edx) and was dropped. 36/199 (18.1%), frame 0x2c,
  ebx-n and `and ebx,0x7fffffff` held. Mass/Trace not touched.
  **2026-09-08 continue-header latch.** Seed `cls = prev_sign` and start the
  do-while at the reload (`prev_sign = cls; abs_r = bits.i; prev = nxt;
  prev_abs = abs_r; nxt = *in`) so the first iter `jmp`s over the two
  loads and the latch is `load in; load left; add 4; dec; store; store;
  jne header` — original shape. ebx=n and `cmp ebx,1 / jl` held (volatile
  dest + volatile plane + byte-n). Loop abs is `and ecx,0x7fffffff`: the
  header-seed sign web sits in edi and the bits reload reuses plane's ecx;
  keeping n live across `__ftol` or rebirthing n as abs at the header
  knocks ebx=n and/or inverts the latch to `je / mov / jmp`. Frame **0x28**
  (prev_abs pad unused). 35/193 (18.1%), 640/593B. Mass/Trace not touched.
  **2026-09-08 cls/esi + 0x2c wave.** Seed `cls=prev_sign` after `left=n`
  was a `mov ebx,eax` (sign started in a scratch), so the bits load could
  not be ebx. Assigning sign before `nxt = v0` puts the sign web in a
  callee-save from the first classify (ebp here, not the wanted esi).
  Named long-lived `plane` hoists to edi and knocks ebx=n. Volatile plane
  plus a post-bits writeback to the plane arg keeps ecx=plane, but dest
  is then free to take the leftover ebx (`void* d = dst` + volatile
  restore) and bits falls to edx (`and edx,0x7fffffff`). That dest copy
  plus the writeback is what restores **frame 0x2c** (pad lives) without
  extending n across `__ftol` or inverting the latch. ebx=n, `cmp ebx,1
  / jl`, count-up lerp, and `in+=4; dec left; jne reload` all held.
  35/203 (17.2%), 672/593B. Mass/Trace not touched.
  **2026-09-08 destrel-before-fild → and ebx.** Homing dest off ebx after
  `left=` frees ebx but then bits takes edx (edx is free). Occupying dest
  during the fild — `destrel = dst - nxt` plus a volatile store to the
  unused prev_abs pad — keeps dest/ecx busy through `fstp`, so the abs
  copy is leftover ebx: `mov ebx,edx / and ebx,0x7fffffff`. Seed `cls`
  *before* `left=n` so the sign copy cannot steal ebx. `in[n]=in[0]`
  before `dst=*cursor` keeps `mov ebx,n` / `cmp ebx,1 / jl`. n reused
  for abs after `left=` but dies at `na.i` (not across `__ftol`). Latch
  still `jne` to the continue-header. Sign still ebp, not esi. Frame
  **0x2c**. 48/191 (25.1%), 629/593B. Mass/Trace not touched.
  **2026-09-08 drop plane writeback.** `*(ClipPlane* volatile*)&plane_v = plane`
  after the loop fild did not buy ecx=plane or dest/edx; destrel-pad already
  holds frame 0x2c. Dropping it keeps ebx=n, `cmp ebx,1 / jl`, latch, and
  `and ebx,0x7fffffff`. 48/190 (25.3%), 609/593B (audit window), still
  ESCAPES. Sign still ebp (nxt is esi). Mass/Trace not touched.
  **2026-09-08 nxt→edi wave (did not land).** dest-in-edi is the destrel-before-fild
  occupant that keeps loop abs on ebx. Homing dest (dest_mem / volatile /
  *cursor / assign-after-classify) frees edi, but nxt stays esi — loop abs
  takes the delayed edi (`and edi,0x7fffffff`). nxt does not migrate; VC6
  assigns esi at the 3-save prologue and only pushes edi when the loop abs
  web needs a 4th. Assignment-order swap of nxt/dest is inert. Named
  function-scope plane knocks ebx=n (plane→ebx or edi). Block-local
  non-volatile plane coalesces across __ftol → edi, gives and-ebx, loses
  ebx=n (n→edx). Split plane (first non-vol, loop vol) puts first plane in
  ecx but bits still copy-abs in ecx; edi delayed. Plane writeback after
  the abs/sign split restores ebx=n, not nxt=edi (plane is eax). An in_v
  writeback makes **in** win edi (`mov edi,eax` / walk in in edi) and
  still loads nxt into esi. A dummy `keep` live to `return` spills
  (frame 0x30), never a 4th callee-save. Original nxt=edi because at the
  first bits load **all three scratches are busy** (eax=in, ecx=plane,
  edx=dest still live after the [esp+0x14] home), so abs is ebp, sign is
  esi, nxt is leftover edi; dest stays edx until lerp destrel and is
  spilled before __ftol. destrel-before-fild plus dest increment after
  __ftol forces dest into edi — that conflicts with dest-as-edx. Cursor-first
  / dest-smash / fild-order not reused. Tip restored. Mass/Trace not touched.
  **2026-09-08 dest-edx / nxt-edi rebuild (did not land).** Drop destrel-before-fild
  and rebuild toward orig scratches (dest edx through seed, nxt edi, sign esi,
  loop and-ebx by another lever). Best transient **43/193 (22.3%)** / 41/192
  (21.4%) — below tip 25.3%. nxt never edi with ebx=n + latch + 0x2c.
  Cursor-first / `int nn=n` / dest-before-in either steal ebx=n (dest→ebx,
  n→edx) or still colour dest **edi** because `in` wins edx as the hot pointer.
  dest_mem + dest=0 after seed fild CSE back: dest stays edi through lerp
  destrel / stride (`mov ecx,edi` at ENTER). `void* volatile dest_mem` does
  put seed dest in ecx (a scratch) and homes it, but nxt stays esi and sign
  becomes edi; loop abs is edx. `prev_sign=0` / `prev_sign=nn` before nxt
  DCE. A volatile sign seed (`pad=nn; prev_sign=*pad`) knocks ebx=n
  (cursor→ebx, n→eax). Named `out` spills (`mov [esp+0x3c],eax`) and does
  not take esi. Byte-abs in the loop uses cl when bits is already a scratch,
  so it does not force ebx. Later destrel-only-at-lerp without an edx occupant
  loses `and ebx`. Tip C restored (destrel-before-fild still the and-ebx
  occupant; dest edi / nxt esi / sign ebp). Mass/Trace not touched.
  **2026-09-08 je-arm + prev_abs fld.** Nested
  `cls != ENTER { cls != BOTH { LEAVE } else BOTH } else ENTER` emits
  orig `je ENTER / je BOTH / jne-cont / LEAVE` fallthrough. LEAVE `k=0`
  outside the vtx guard (still spills, not `xor ebp`). fld first operand
  from `prev_abs.i` (`[esp+0x34]`) while pa/na copies stay for colouring.
  Byte-n store dropped: `mov ebx,n / cmp ebx,1 / jl` and `and ebx` still
  hold. 62/187 (33.2%), 611/593B, still ESCAPES. dest edi / nxt esi /
  sign ebp. Mass/Trace not touched.
  **2026-09-08 shared cursor store.** `if (n>=1){body;} *cursor=dst;
  return` so the early-out stores dest (no `xor eax,eax` skip). 64/189
  (33.9%), 606/593B, still ESCAPES (jl target 0x262). All KEEP held
  (frame 0x2c, ebx=n, latch, and-ebx, je-arm, fld prev_abs.i). Lerp is
  still `lea edi` dest-walk. destrel-pin / pad+delta / bits-volatile all
  steal dest=edi → n leaves ebx and `and ebx` dies; destrel-pin alone is
  593B no ESCAPES at 22.8% and was not landed. pa/na still colouring.
  bits still `fstp [esp+0x40]` (n-slot). Mass/Trace not touched.
