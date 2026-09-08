# Codex-F — CLOSED, 26/26 exact (2026-09-09)

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
| `ridemachine2.c` | 5 | 0 | 5 |
| **lane** | **26** | **0** | **26** |

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

### `ridemachine2.c` — 5/5 exact

| Address | Function | Insns | Audit | Residual |
| --- | --- | ---: | --- | --- |
| `0x00404860` | `Copters_StepCar` | 23 | OK | — |
| `0x0043a940` | `SpaceTower_StepCar` | 37 | OK | — |
| `0x004049a0` | `Copters_StopRide` | 71 | OK | — |
| `0x0043b810` | `SpaceTower_UpdateRiders` | 116 | OK | — |
| `0x00404630` | `Copters_UpdateCarRider` | 168 | OK | — |

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

Second sweep (same body, ~400 more variants; nothing moved the tie without
also changing code):

- FP phase is a period-4 counter of dead-loop expressions: k distinct
  `while (0) { v++; }` bodies give the ORIGINAL's full FP block (all six
  products, 90/168) for k = 1 and 5 only; k = 2,3,4,6 and `while (0) {
  mode++; anim++; }` are back at 00. Dead assignments (`oa = 0`, `i = j =
  0`, `f = 0.0f`, `m = 0`, `anim = anim`, `mode = mode + 0`, `kf = kf1`,
  declaration initialisers) are killed before that counter and are inert.
  All 64 operand orders of the six products and all 64 parenthesisations
  `(a*b) - (c*d)` fail: parens on a product re-order only the LATER products
  (never product 2), operand order in source never changes the emitted
  order. Parens/casts anywhere in the pre-FP statements are inert.
- Allocation is invariant under every natural loop-body spelling: 100 random
  combinations of index expression (`(oa + frame*4 + 1)*3 + ord_b[j]` in six
  associations, `frame`/`idx`/`slot` temporaries), product order, `m[j*3]`
  / `m[3*j]` / `*(m + j*3)`, `*(int*)&f` vs an `int fi` copy, for/do-while/
  `!=`/`++j`, `for (...; i++, m++)` all give 86/168 or 83/168 (the latter is
  the `i++, m++` form). goto loops, `for (;;)` with `break`, `while (1)`,
  `3 > i`, `i = i + 1`, `sizeof` bounds, trailing `continue;` — identical.
  `unsigned`/`long` counters — identical. Block-scoped `oa`/`j`/`scale`/`f`,
  `const float scale`, `static float k` — identical. Wrapping the loop nest
  (or the inner loop) in `do { } while (0)` / `for (;;) { ... break; }` to
  fake a deeper nesting level — identical.
- Register preference is esi, edi, ebx (micro-probes). Original picture is
  chain (layer/sprite/person/oa) = esi, seat = edi + [ebp+8] home, anim =
  ebx, loop temps = edi, j = ecx; ours is seat = esi, anim = edi, chain +
  temps = ebx, j = eax — the ORDER of all four decisions is inverted (the
  original favours the short live ranges), not one tie.
- Everything that does flip it emits code: `m[j*3+1] = 0` (113/174); no
  `ofs.ox` store (`pos.ox = ... + (sprite->dim >> 1)`, 92/171, +3 instr);
  `<= 2` bounds (94/173, jle); `char`/`short` j (96/178); a second exit
  `if (i > 2) break;` (109/175); an extra `person` or `anim` reference
  pre-loop; two `rider->person` loads. So the tie is broken by loop
  weight / live-range length, and the original's loops are classified
  differently (e.g. not a counted loop) with no visible trace.
- `person->matrix[j*3 + i]` (typed `Person3D*`, no `m` variable) is the
  right cursor form: it puts `lea edx,[esi+0x58]` in the loop preheader
  exactly like the original (best1 hoists it above the FP block) and with
  the ofs.ox flipper reaches 98/171 with the whole loop exact except j in
  eax (original ecx) and the preheader `mov [ebp-4],eax` one slot early.
  On its own it is 83/168 because ip takes [ebp+8] and m [ebp-4].
- `static __inline int FScaleI(float x, float k)` with the asm inside IS
  inlined by VC6 (76/168), but the 65536 store then lands in the loop
  preheader; the original's store sits right after `add esp,0x28`, so the
  constant is a user local written before the cross product.
- SEAT as a macro (`&rec->seat[index]` at every use) recomputes after the
  call (66/169). `index = 0xeb` reusing the parameter as mode: 44/172.

Next: the two invisible differences are (1) loop classification/weight and
(2) one dead expression surviving to VN. A helper macro that expands to
`while (0) {...}` or an inline function VC6 inlines and then deletes could
supply both; so could the original loops being driven by a table length
VC6 cannot fold (e.g. `extern const int` bound, `for (ip = ord_a; *ip >= 0;
ip++)` sentinel — untested because the emitted compares are constants).

Third sweep (2026-09-08, ~150 variants, measured with a register-picture
extractor rather than the score alone; scratch scripts under the session
scratchpad, not committed):

- The whole residual is ONE global decision that many things flip. Every
  flipper found so far emits code: `s = sign_a[j]*sign_b[oa]` or
  `sb = sign_b[oa]` computed BEFORE the asm block (147/175 matchfull:
  chain=esi, seat=edi+[ebp+8] home and per-iteration reload, anim=ebx,
  oa=esi, ip/m at [ebp-4]/[ebp-8], scale at [ebp-0xc] -- the original's
  entire picture except that the sign product is emitted pre-asm);
  ord_b+sign_a declared as ONE `int tab[2][3]` (147/174: same picture, but
  the inner IV becomes a pointer over the table, `cmp eax,&tab[2][3]`,
  where the original keeps `ecx=j*4 / cmp ecx,0xc`, so the original's
  tables are separate symbols); `person` taken from `g_copters_def`
  instead of `seat->rider` (124/173); one extra pre-loop reference to
  `anim` (swaps seat/anim between esi/edi; a reference-count tie).
- The SAME lever closes the mantex.c sibling `PutOne3DBlokeOnRide`
  0x00441980 to 78/81 (only the pre-asm product placement left), so it is
  one mechanism shared by both originals, not a per-function accident.
- In both originals the inner loop's expression temp takes EAX and the
  j*4 IV takes ECX (cursor EDX); every build of ours gives the IV EAX and
  the temp ECX/EBX. Whatever pushes `oa` into the callee-saved pool also
  ranks that temp above the IV; j-before-oa statement orders, do/while,
  comma inits and byte-offset loops are all normalised away (inert).
- Measured inert on the typed `person->matrix[j*3+i]` base (which puts
  the preheader `lea` right but scores 161 vs 146 audit mismatches, so the
  committed body stays): all flag sets incl. /O1 /Ox /G5 /G6 /Oa /Ob0-2
  /Oy- /Gf /Op /Za /QIfist; declaration order (12 random permutations);
  1-8 unused locals; `while (0) {v++;}` dead loops k=1..8 (no FP-order
  movement on this base -- the notes' k=1/5 counter is base-specific);
  `rec`/`index` reused as seat/mode/f (a modified parameter gets a
  register, so [ebp+8]/[ebp+0xc] are ordinary spill homes); inlined
  helpers for the bake, the loop or the cross product; static/const/2-D
  table declarations other than the ord_b+sign_a pairing; every index
  spelling incl. `float(*)[4][3]`, PosFrame structs and
  `frame*12+(oa+1)*3+ord_b[j]` (VC6 factors them all to the same IR);
  product association/temps after the asm; pointers to f, the
  destination, the row or the sign entries (all fold before allocation);
  merged layer/sprite/person temporaries (web-split); block-scoped f
  (only reshuffles the frame slots); an asm spelled as one block, bare
  `__asm fld f` lines, or with `dword ptr [f]`.
- 0x00458930 is the CRT `_ftol` (used for `(int)kf1[1]`), so the loop's
  bare fistp is inline asm, not /QIfist.

Fourth sweep (2026-09-08, sibling-first, ~300 variants on
`PutOne3DBlokeOnRide` 0x00441980 in mantex.c, then applied back):

- DECISIVE: adding any EAX write to the asm block (`xor eax,eax`,
  `inc eax`, `mov eax,0` after the fistp) makes the mantex sibling 81/82
  (matchfull) -- every instruction of the original including the
  post-asm `sign_a[j]` load, the `add ecx,4` placement, esi/ebx/edi for
  si/hoisted/track and ecx/edx for the j and cursor IVs; the only
  difference is the clobber instruction itself. A read of EAX (`test`,
  `cmp`) does nothing; a write to ECX or EDX gives a different, wrong
  picture. On Copters the same clobber plus the pre-asm
  `sb = sign_b[oa]` gives 163/175. So both originals were compiled by a
  VC6 that believed this fld/fmul/fistp block wrote EAX.
- That belief is NOT a compiler build: the decomp.me packages msvc6.0
  (12.00.8168), 6.4, 6.5 and 6.6 (12.00.8804, distinct C2.DLL checksums)
  produce byte-identical output to our SP3 8447 for the base bodies and
  for the parameter-reassignment variants. Not the front end either
  (`/TP` with `extern "C"` is identical), nor any flag (/Oa /Ow /Op /Za
  /Ge /Gh /G3-/G6 /Ob0-2 /Oy- /Os /Oi- /Gf /GX /GR /Zp /J /MT /MD /ML /Gs
  /QIfist /Zi /Z7 all measured on the sibling).
- Not the asm text: `fld frame`/`dword ptr [frame]`/`frame[0]`/
  `[ebp+frame]`/`ss:`/upper case/`(frame)`/raw `[ebp+16]` (which
  additionally drops the 65536 store as dead), one block vs bare lines,
  a label, an empty block, a comment, `even`, and a C statement placed
  between the asm lines (VC6 keeps C code exactly where it sits between
  asm statements; the scheduler hoists loads above C stores but never
  across an asm line).
- Not the variables: `float f` / `float scale` locals land in the dead
  `frame`/`screen_x` parameter slots by themselves (the pointer puns are
  unnecessary but harmless); a union, `register`, a float-typed
  parameter, an int fistp target other than the fld source, an inlined
  FixMul helper (return value or out-param), a real call in place of
  the asm (everything then goes callee-saved and ebp is freed) -- none
  reproduce the picture without bytes. The clamp is not written back to
  [ebp+0x10] in the original, so `frame` itself is not the asm's
  variable there; a float local homed in its slot is.
- `sa = sign_a[j]` before the asm (77/81) and a `_ftol` call before the
  loop (63/82) flip the same core (j=ecx, cursor=edx, si=esi, track=edi)
  by other routes; 90 random spellings of every other axis leave the
  base picture untouched, so the base is a knife-edge that only an
  extra EAX occupant across the asm resolves the original's way.
- 0x00458930 is the game's own three-instruction `_ftol` replacement
  (`fistp [scratch]; mov eax,[scratch]; ret`), so `(int)` casts call it
  and the loop's bare fistp is inline asm, not /QIfist.

Fifth sweep (2026-09-08, after the V completion agent pointed here at
DECOMP.md's SCOPE V entry): the scope-V cancelled-pair lever IS byte-free
IR. `t.x = <expr>; t.x += t.y; t.x -= t.y;` on a struct member with a
pointer-valued anchor (`t.y = (int)track` / `(int)person`) survives to the
allocator as real defs and uses and cancels at instruction selection.
Applied to the float index temp it raises that web above the j IV, so the
temp takes EAX and j/cursor fall to ECX/EDX with the sign product after
the asm -- the original's loop without any clobber:

- mantex `PutOne3DBlokeOnRide`: 34 -> 8 audit mismatches, 81i/221B
  size-exact, relocs 0, /W3 clean (committed). Residual is only the
  `si`/`track` esi<->edi swap. That order is not a weight tie: a real
  extra use of `si`, the cancel on `si` itself (before or after the
  inner loop), and a dying copy web of `index` at the track def all
  leave `track` in esi or cost registers; only the EAX write in the asm
  classifies `si` early enough to take esi before `track`.
- `Copters_UpdateCarRider`: 146 -> 50 audit mismatches (matchfull
  154/176; committed) with the lever on the index and on `oa` (as
  `u.ox`). Everything outside the loop is now the original's; `u.ox` is
  memory-homed at [ebp-0x30] (frame 0x30 vs 0x28) instead of esi, and
  the kf[8] product order is unchanged. Plain-int identities fold in the
  front end (as the V notes say); the member cancel adds defs, so a
  chain of webs, not weight on one web.
- Anchors matter: `(int)track` coalesces with track at no cost;
  `(int)person` / `(int)anim` make the anchor its own callee-saved web
  and spill something else.

Sixth sweep (2026-09-08): the cancelled-pair lever characterised, and the
sibling residual reduced to a three-web rotation.

- **The cancel survives only when BOTH operands are struct members.** With a
  plain-scalar anchor (`t.x += si; t.x -= si;`, and the same with `i`, `j`,
  `frame`, `(int)dest`, `(int)person`, `anim->frame_count`) the front end
  folds it and the body returns to the pre-lever baseline exactly. This is
  the V note's "every identity written on the value itself folds", stated
  from the other side.
- **The lever DEMOTES the carrier; it does not promote the anchor.** A
  carrier member takes a scratch register and a frame home. The anchor only
  keeps its host's rank, and only because `t.y = (int)track` coalesces with
  an existing plain variable. Making the value of interest the anchor member
  instead (`t.y = g_ride_mtx_chan[i]`, with or without a third member for
  the cancel) DEMOTES it to edx and lifts the cursor to esi, because a
  member carries a frame home. So the lever cannot be used to raise a web.
- **Sibling residual is a pure rotation** of {si, track, cursor} over
  {esi, edi, edx}. The original ranks si > track > cursor; the committed
  body ranks track > si > cursor (8 mismatches, all of them that swap);
  every si-as-member form ranks cursor > track > si. A second carrier on
  si, under eight different anchors, always pushes si to edx and the
  cursor to edi.
- **Reusing the dead `index` parameter as the channel variable is inert**
  (the original's prologue keeps `index` in esi and deliberately loads the
  track base into edi, which is what suggested it). A modified parameter is
  an ordinary local to VC6, as the third sweep already found.
- **Frame cost.** On mantex the carrier is free: 81i/221B and frame 0x10,
  both size-exact. On Copters it costs a slot (frame 0x30 against the
  original's 0x28, 534B against 528B). Rehousing both carriers in the
  members of the four dead `Offset` locals (`screen`, `layer_ofs`, `ofs`,
  `pos`) restores frame 0x28 — all 48 assignments measured — but the best
  non-escaping one is 70 mismatches against the committed 50, and the four
  49-mismatch ones all report ESCAPES at 536B. The committed body is kept.

Because the original's frame has no spare slot, the cancelled pair is
probably a proxy for whatever the original wrote, not the original
construct itself, on Copters at least. The seventh sweep supports that:
the lever fixes the loop shape but cannot set the register ranking.

Seventh sweep (2026-09-08) — **the EAX hypothesis is FALSIFIED. Do not
spend more time on it.**

- All 34 files containing `__asm` pass the gate. Only two EXACT functions in
  the tree share the target shape (fld/fmul/fistp on a frame local):
  `ApplyObjectOrientationToPerson` 0x00484950 (bnvpath.c, 95i/273B, **0
  mismatches**) and `ComputeVertexBounds` 0x00440980 (person3d.c).
- **`ApplyObjectOrientationToPerson` keeps `person` in EAX across NINE
  consecutive fld/fmul/fistp blocks and is byte-exact.** So VC6 does not
  model this asm as writing EAX, and neither did the original compiler.
  The fourth sweep's "the original was allocated as if the asm wrote EAX"
  reading is wrong.
- Confirmed directly: on the current (cancelled-pair) base an added
  `xor eax,eax` no longer flips anything — same rotation, one extra
  instruction. The old 81/82 result was an artefact of a pre-lever base
  sitting on a knife edge, not evidence about the original.
- The game's house style for this conversion, read off the exact bnvpath
  body (`float scale = 65536.0f;` + `float value;` + bare `__asm fld value`
  lines + `*(int*)&value`), leaves the sibling's register picture
  completely unchanged when applied on top of the lever. Float locals home
  themselves in the dead parameter slots either way.
- Anchor sweep completed: an anchor that is a **constant** (an array
  address, `&arr[0]`, a literal, 1) always folds back to the pre-lever
  baseline, so the anchor must hold a runtime value and therefore must
  occupy one of the six registers the loop already needs. Anchor
  **position** is inert (before the call, after it, in either loop). The
  anchor's HOST is the only thing that selects the rotation, and only
  `(int)track` gives the correct cursor and product order.

Eighth sweep (2026-09-08) — **the /FAc listing route, which took Copters
from 50 audit mismatches to 15 and made it instruction-, byte- and
frame-exact (168i/528B, frame 0x28).**

`/FAcs /Fa<path>` works through the wibo wrapper and is now the tool of
choice for this class of residual: the listing prints VC6's own frame
symbol table and attributes every instruction to a source line.

- **VC6 overlaps locals onto dead PARAMETER slots.** The exact bnvpath body
  equates `_value$ = 8` — the same offset as `_person$ = 8`. That is why the
  original's asm operands sit at `[ebp+0x10]` and `[ebp+0x1c]`: they are the
  dead `frame` and `screen_y` parameters holding `value` and `scale`. Our
  pointer puns and a plain `float` local produce identical bytes, so that
  choice was never the issue.
- **The equates list tells you which locals cost frame slots.** mantex lists
  only `_out$`/`_base$`, so its carrier is free — hence 221B size-exact.
  The old Copters body listed `_u$ = -48`, exactly the 8 bytes that took the
  frame from the original's 0x28 to 0x30.
- **A struct member that must live ACROSS a loop is memory-homed and
  reloaded; one confined to a straight-line region stays in a register.**
  So the cancelled-pair lever is byte-free only for a value that does not
  cross a loop. The `oa` carrier violated that and cost the slot plus a
  reload every inner iteration.
- **THE ANCHOR IS LIVE WHEREVER THE CANCEL IS — pick it accordingly.** This
  is the general rule the listing exposed and the one to carry to other
  scopes. The old body anchored on `(int)person`, so `person` stayed live
  through the inner loop and occupied esi; the original frees esi at
  `lea edx,[esi+0x58]` and hands it to `oa`. Anchoring on `oa` itself, taken
  where `oa` is defined, leaves every other web exactly where the original
  has it. Anchors on `anim` (45) and on the dead-at-that-point `layer`,
  `mode`, `seat->frame`, `(int)kf` (77-164, several ESCAPES) confirm the
  rule from the other side, as does mantex: `(int)track` stays its best
  anchor (8) because the original really does keep the track pointer live
  through the loop, while `si`/`chan[i]` anchors give 11/14.
- Inert on the new base: cross-product operand order and statement order
  (all swaps identical), the dead-loop phase counter k=1..6, doubling or
  tripling the cancel, an explicit `m` cursor in four placements, a cursor
  carrier, `register`, and every spelling of how the anchor takes its value
  (`= oa`, `= chan[i]`, chained, two-step).

Residual on Copters is now exactly two items: `oa` and the store cursor are
swapped (original `oa`=esi with `lea edx,[esi+0x58]`, ours `oa`=edx with
`add esi,0x58`), and the second factor of two cross products loads in the
other order. Both are rank/phase decisions, and 15 mismatches is a plateau
across ~40 further variants.

Next: both bodies are honest WIPs with everything but a rank ordering
matching. The EAX story is dead. If a ninth pass happens, the /FAc route
is the one that works — read the listing, not the score. Nothing in the C or asm syntax
tried so far does it; the two remaining ideas are (a) an instruction
the compiler emits for C code inside the loop that we have attributed
to the asm and that VC6 models as an EAX def, and (b) an inline-asm
construct outside the loop body (e.g. an asm block earlier in the
function, or in a macro used by both files) that changes how the asm
blocks in the loop are modelled. Both siblings must be closed by the
same construct.

## Closed — 2026-09-09

`Copters_UpdateCarRider` is exact: **168 instructions / 528 bytes, frame 0x28,
0 mismatches**, `audit.py` `[OK]`, `relocs.py` 0 MISMATCH, `/W3` clean. The
lane is 26 of 26 with no WIP bodies. Two zero-cost levers closed it, found by
a 15-agent parallel sweep over ~3,000 measured variants:

1. **An alias pointer defeats VC6's commutative-fmul canonicalisation.**
   `kg = kf;` with four of the twelve cross-product operands read through the
   alias. VC6 canonicalises a commutative `fmul` whose two operands are
   constant offsets off ONE pointer, which is what reversed the fld/fmul order
   of the two products containing `kf[8]` (offset 0x20); it cannot order
   operands off two different pointers, so source order survives. Source
   operand order is itself inert — all 64 permutations measured, twice, on two
   different bases. Interchangeable spellings that also work: a `volatile`
   alias, a char* cast on the 0x20 operand, and a dead-store seed.
2. **A cancelled-pair anchor must be a value whose ranking does not matter.**
   `t.oy = (int)g_copter_ord_a;` — a link-time address constant, hoisted out
   of the loop. The anchor receives an allocator priority bump; anchoring on
   `oa` ranked oa above the store cursor (the 11-line register swap, 15
   mismatches) and anchoring on `anim` fixed that but rotated the callee-saved
   trio instead (45). An address constant is already materialised, so it bumps
   nothing. `t.oy = j * 4` works equally well for the same reason — ecx
   already holds j*4. **The constant must be assigned to the struct MEMBER
   first**; used directly in the cancel it folds in the front end.

Both levers are independent: a full 2x3 grid of {address-constant anchor,
`j*4` anchor} x {alias re-read, volatile alias, dead-store seed} all reach 0.
The match is a basin, not a knife edge.

### Reusable rules for other lanes

- **The anchor of a scope-V cancelled pair is live wherever the cancel is and
  receives a priority bump.** Pick a value that is already register-resident
  and already ranked correctly — an address constant, or an existing induction
  value. Never anchor on a variable whose register assignment you still need.
- **A struct member that must live across a loop gets a real frame slot and is
  reloaded; one confined to straight-line code is free.** Check the `/FAcs`
  equates list: a `_name$ = -N` line for your carrier means it cost a slot.
- **`/FAcs /Fa<path>` is the tool for this class of residual** — it prints
  VC6's frame symbol table and attributes every instruction to a source line.
  It also shows that VC6 overlaps locals onto dead PARAMETER slots
  (`_value$ = 8` sharing `_person$ = 8` in bnvpath.c), which is why the
  original's asm operands live in dead parameter homes.
- **Two constant offsets off one pointer are canonicalised in a commutative
  float multiply; two different pointers are not.** Reach for an alias when an
  fld/fmul pair comes out reversed and source order does nothing.

## Remaining

Nothing. The lane is closed at 26/26.
