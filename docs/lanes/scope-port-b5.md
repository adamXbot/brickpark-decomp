# Scope PORT-B5 — the remaining inline-asm bodies ported to C

Branch `scope/PORT-B5` off `feat/decomp-completion-next-steps-24a0d6`
`595aae91` (the PORT-B3 merge). Goal: the fifteen bodies
`portable/tools/linkreport.py` still files as "Unported inline-asm bodies"
after PORT-B3, in the order the brief set them.

Rules kept: the `#ifndef LEGOLAND_PORTABLE` arm is byte-for-byte the original
`__asm` **except the one `row:` label fix in §1**, all new C is inside the
`#else` arm, and no preprocessor line sits between a `// FUNCTION:` marker and
its signature.

## 0. What was ported

| body | file | test |
| --- | --- | --- |
| `SoftBlitAnim` 0x00465240 | softblit.c | `anim_recolour` |
| `DrawFlatTri` 0x004877b0 | tri3d.c | `tri_raster` |
| `DrawGouraudTri` 0x00486590 | tri3d.c | `tri_raster` |
| `DrawFlatTexTri` 0x00487d40 | tri3d.c | `tri_raster` |
| `DrawGouraudTexTri` 0x00486c70 | tri3d.c | `tri_raster` |
| `ZBufferHelper` 0x00464a90 | bigrender.c | `zbuf_blit` |
| `BltAdvisor` 0x004659a0 | blitmisc.c | `zbuf_blit` |
| `ShowCapacityOverlay` 0x004632b0 | blitmisc.c | reviewed, not testable — §6 |
| `RenderTransSprite` 0x00489190 | popup.c | reviewed, not testable — §7 |
| `FastSqrt` 0x00426ab0 | coastermath.c | unreachable, proved — §8 |
| `FastRSqrt` 0x00426980 | coastermath.c | unreachable, proved — §8 |
| `sub_458930` 0x00458930 | bnvpath.c | unreachable, proved — §8 |

Not in this lane's brief and still traps: `coaster13.c:767
TrackShade_FillPoly`, `coastershade2.c:445 Span_FillShade`,
`coastershade2.c:595 Span_FillShadeZ` — the Gouraud/textured span fillers of
the coaster-track renderer.

## 1. The `row:` label in softblit.c — the verdict

PORT-B3 (§3b FINDING 2) reported that `SoftBlitAnim`'s `row:` label sits one
line BELOW where the original's back edge lands. Confirmed against the binary
before touching anything:

```
$PY tools/disasm.py original/legoland.exe 0x6543d 10
  0x00465443: mov     edx, dword ptr [0x7febac]     ; g_sp_h
  0x00465449: and     edx, edx
  0x0046544b: je      0x46582e                      ; done
  0x00465451: mov     dword ptr [0x7fea10], edx     ; g_sp_rows_left = edx
  0x00465457: mov     edx, dword ptr [0x7fea50]     ; g_sp_left
$PY tools/disasm.py original/legoland.exe 0x65820 8
  0x00465828: jne     0x465451
  0x0046582e: popal
```

The back edge targets **0x465451, the store**. The label was below it, so
`jne row` assembled to 0x465457 and the store ran once: `g_sp_rows_left` would
stay at `g_sp_h` for ever and the loop would never end for a sprite taller
than one row. Moved up one line, and nothing else in the arm changed.

### The byte-gate evidence

The encodings are the same length — the target is 0x3d9 bytes back either way,
too far for `rel8`, so both are the 6-byte `jne rel32` at 0x00465828 — and
`match.py`'s `norm()` rewrites every direct branch target to `<t>`, so the gate
cannot tell the two apart. It reported **`[OK] 0x00465240 SoftBlitAnim
ours=415i/1544B orig=415i/1544B mismatch=0` before AND after**, which is
exactly the point: the gate was never the thing that could decide it.

What decides it is the disassembly above plus the sibling cross-check PORT-B3
named: `SoftBlitAnimPlain`'s own back edge `0x00464a5e jne 0x4646a1` also
targets its store, and `softblit2.c` has always had its `row:` label above it.
A third witness was found in this lane: `bigrender.c`'s `ZBufferHelper`, a
third member of the same family, also places `row_loop:` above
`mov g_sp_rows_left, edx`.

So the fix changes the **relocation-free displacement** of one `jne` from
-0x3d7 to -0x3dd. No `[OK]` row moved, `relocs.py` is unaffected (a relative
branch inside the body carries no relocation), and the behaviour of the
assembled function goes from "hangs on any sprite over one row tall" to
correct. The evidence is written into the file above the marker so the next
reader does not have to rediscover it.

## 2. SoftBlitAnim 0x00465240 — the recolouring type-2 painter

PORT-B3's §3b table listed nine differences from `SoftBlitAnimPlain`. Two of
the nine turn out to be textual only, and the port proves it by reusing
`ll_anim_code` / `ll_anim_count` for both painters:

* the single-code reader is `shr ebx,2 / and g_zb_bits,0fh / jne / mov 10h /
  dec` here against the plain one's `dec eax / jns / mov eax,0Fh`. Both refill
  when the count is 0 and both leave 15 behind, so they are the same function.
* the length splice is `shrd ecx,ebx,8 / shr ecx,18h` against `movzx ecx,bl`,
  and the refill test is `cmp g_zb_bits,4 / jae` against `sub eax,4 / jns` —
  same condition, same post-refill count of 0x0c.

The remaining **eight are real and all reproduced**:

1. every store is `and ax, word ptr g_sp_recolour` — the LOW WORD of the mask
   only, so a high half in the `int` global is ignored;
2. the left-clip subtraction is `sub edx,ecx / ja`, not `jns`: a run that ends
   exactly on the left edge LEAVES the skip pass here and continues it in the
   plain painter (where `lc_step` then drives the skip to -1 and the rest of
   the row is eaten);
3. a left-clip skip run that exactly fills the row leaves via `jbe endrow`,
   not `js endrow`;
4. a repeat run that finds a zero budget writes NOTHING (`or ecx,ecx / je
   endrow`); the plain one's doubled-dword `rep stosd` path emits one word
   before it looks at the count;
5. the repeat run is a software loop, not `rep stosd` — it cannot be, every
   word is masked;
6. the run hit test is `seta`, strictly `dp > mouse`, where the plain
   painter's `sbb ecx,-1` gives `dp >= mouse`;
7. the transparent single is `dec edx / je endrow / add edi,2`, the correct
   order — the plain painter's `add` between the `dec` and the `je` is
   PORT-B3 FINDING 1, and this is that branch working;
8. the hit flag is OR'ed as a DWORD (`or g_blit_hit,eax`) where the plain
   painter ORs the low byte. The value is 0 or 1 either way, so this one is
   not observable.

### Test `anim_recolour` — 25 checks, 0 failed

`portable/tests/test_anim_recolour.c`. Its own encoder (written again from the
reader's arithmetic, not shared with `test_anim_paint.c`), a six-row op-list
sprite drawn into a reference image for the unclipped / top-skipped /
right-clipped / left-clipped / both-edges cases, plus the recolour mask and
the low-word-only check.

Differences 2, 3, 4, 6 and 7 are each driven **over an identical stream through
both painters** on a row built to isolate it, so the divergence is observed
rather than asserted:

| difference | the row | SoftBlitAnim | SoftBlitAnimPlain |
| --- | --- | --- | --- |
| 6 (`seta`) | a 3-pixel copy run, width 3, mouse one pixel past it | no hit | hits |
| 2 (`ja`) | copy 3 + two singles, left clip 3 | paints source column 3 | paints nothing: `jns` keeps it in the skip pass and `lc_step` drives the skip to -1 |
| 4 (zero budget) | skip 12 then repeat 3, width 12 | writes nothing | one stray pixel at column 12, and that is the ONLY pixel the two disagree on |
| 7 (the dead `je`) | SEVEN transparent singles then a pixel, width 6 | the sixth ends the row | writes one column past the right clip |
| 3 (`jbe`) | skip 12, a transparent single, a pixel; left clip 3, width 9 | ends the row | writes one pixel past the right clip |

Difference 7 needed seven singles, not six: at a budget of exactly zero
`copy_run`'s own unsigned test still refuses the pixel, so it takes a
**negative** budget to turn the comparison round. Six singles look identical
in both painters and would have hidden the finding.

## 3. tri3d.c's four rasterisers

All four are the same skeleton — sort by y, two halves, per-edge 16.16
stepping, a vertical fast-forward, a per-scanline span with its own
reciprocal, a horizontal clip, then an unsigned Z test per pixel — with more
interpolants each time (z; z+shade; z+u+v and one flat shade; z+u+v+shade).
The C keeps the asm's own label names (`flat_L12`, `gt_L13`, `ft_L14`,
`gtt_L5` ...) so the two read side by side, and the header's FRAME SLOT MAP
becomes the local names.

Arithmetic that is load-bearing and is spelled out at each site:

* `imul ecx / shrd eax,edx,0x10` is `LL_FMUL16` — a SIGNED 32x32->64 product
  shifted right 16;
* `mul ecx` is an UNSIGNED 32x32 product of which only the low 32 bits are
  kept, so a negative per-scanline delta times a positive clip count still
  lands right: written `(int)((unsigned)d * (unsigned)n)`;
* `shr ecx,0x10` on a span width is LOGICAL, `sar edi,0x10` on an edge x is
  ARITHMETIC;
* the Z test is `cmp edx,[zb] / jb`, so a pixel is drawn on UNSIGNED `>=` and
  equal keys overwrite;
* the Z address is `g_zbuf + (y << 9) + x*4` — 0x200-byte rows whatever the
  render target's width.

Per filler:

* **DrawFlatTri** is the only one that writes back into the caller: `shl dword
  ptr [edx+0x14], 6` shifts vertex **a**'s shade in place, BEFORE the sort.
  The other three copy the shade out into frame slots first.
* **DrawGouraudTri** re-reads `g_flat_colour->table` at EVERY pixel and
  indexes it with `movzx edx, word ptr [ebp-0x22]`, the high word of the span
  shade accumulator. Nothing clamps.
* **DrawFlatTexTri** converts u and v once per vertex with
  `fld [v+0xc] / fmul 65536.0f / fistp` then `(& 0xffff) << shift`, so the
  integer part is dropped (u == 1.0 wraps to 0) and the result is a 16.16
  TEXEL coordinate. `Vertex2D` declares u/v as `int` and the asm loads them as
  floats, hence `LL_ASFLT`. It does NOT mask.
* **DrawGouraudTexTri** masks both halves, so it tiles — see FINDING 2.

### FINDING 1 — the texel address is transposed in the documentation

Both textured fillers compute

```
texel = texels[((v >> 16) << ushift) + (u >> 16)]
```

v scaled by the ROW shift at descriptor +0x00 and u the fast axis. In
`DrawFlatTexTri` the instruction that settles it is
`movzx edx, word ptr [ebp-0x26]` — `[ebp-0x28]` is the V accumulator, so
`[ebp-0x26]` is its high word, and it is what `shl edx, cl` shifts — and in
`DrawGouraudTexTri` it is `movzx edx, word ptr [ebp-0x22]` over the V slot at
`[ebp-0x24]`.

**The TEXTURE ADDRESSING paragraph at the top of `LEGOLAND/tri3d.c`, and the
identical one above `Texture` in `LEGOLAND/texture.c`, have u and v the other
way round** (`texels[(high16(u) << ushift) + high16(v)]`). Two independent
witnesses say the asm is right and the prose is wrong:

* `texture.c`'s `BuildTextureRecord` (0x004437d0) fills the texels row-major,
  `tex->texels[img->w * y + x] = c`, with +0x00 = log2(w). Row-major means the
  ROW is what gets multiplied by the width.
* `unref7.c`'s `SampleTexturePixel` (0x00488730), a separate hand-written
  sampler whose C port already landed, computes `(cv << shift) + cu`.

Documentation only — no shipped behaviour changes — but it would have sent the
next reader of this file the wrong way, and it is the one thing a port of these
two functions cannot get wrong silently. The correct formula is now recorded in
the `LLTexDesc` comment in tri3d.c; `texture.c` was not touched (not this
lane's file, and the same sentence needs the same fix there).

### FINDING 2 — DrawGouraudTexTri's two masks are crossed

```
movzx edx, word ptr [ebp-0x22]    ; the V accumulator's high word
and   edx, dword ptr [ebx+0x10]   ; ... masked with UMASK  (w - 1)
shl   edx, cl                     ; ... shifted by ushift = log2 w
add   edx, dword ptr [ebx+8]
movzx ecx, word ptr [ebp-0x2a]    ; the U accumulator's high word
and   ecx, dword ptr [ebx+0x14]   ; ... masked with VMASK  (h - 1)
```

`BuildTextureRecord` sets +0x10 = `w - 1` and +0x14 = `h - 1`. So the ROW
index is masked with the WIDTH mask and the COLUMN index with the HEIGHT mask:
the pair is swapped. On a square texture the two masks are equal and the defect
is invisible, and every texture the game builds looks square (the switches in
`BuildTextureRecord` accept only powers of two up to 256 for each dimension,
and the blokes' textures are square), which is presumably why it shipped.
REPRODUCED, not fixed.

### Test `tri_raster` — 33 checks, 0 failed

`portable/tests/test_tri_raster.c`. **The oracle is deliberately not a second
rasteriser** — re-deriving the 16.16 edge walk in the test would share every
bug with the thing under test. What is checked instead is the set of properties
the file's own contract states, each of which fails differently:

* `g_recip[n] == 65536/n` at n = 1, 30, 99 (the table the whole thing divides
  with, and the reason nothing may span more than 99);
* coverage is inside the triangle's bounding box, non-empty, and of a
  plausible size; the first pixel of the top row is painted and the column left
  of it is not;
* the clip rectangle bounds the span on all four sides (x 15..30, y 10..20
  against a triangle that spans 10..39 x 5..34);
* the Z test: a smaller key loses, an EQUAL key overwrites, a larger key wins —
  three draws of the same triangle with three different ramps;
* the mouse pick fires on a painted pixel and not on one above the triangle;
* only `DrawFlatTri` shifts the caller's vertex **a** shade left by 6, and it
  leaves b and c alone; the other three leave a alone too;
* `DrawGouraudTri` with three equal shades paints exactly ONE ramp entry, and
  the index is `(shade << 6) >> 16`; with a gradient the top row takes vertex
  a's index, the index grows down the triangle and never goes back down;
* **FINDING 1** is driven directly: the texture's value is its ROW and the
  triangle's v is CONSTANT while u sweeps 0..0.9, so a correct address gives
  ONE value for the whole triangle and a transposed one would vary across
  every span. Repeated at v = 9/16 to show the value follows v;
* **FINDING 2** is read straight out of a pixel: constant u = 5/16 and
  v = 6/16 (so the span deltas are zero and the address is decidable by hand)
  over a deliberately non-square mask pair, umask 15 / vmask 3. The crossed
  code gives `(6 & 15) << 4 | (5 & 3)` = 0x61; masking the way the field NAMES
  imply would give `(6 & 3) << 4 | (5 & 15)` = 0x25. The painted pixel is 0x61.
  The texel array is the full 16x16 either way, so the over-read stays inside
  the test's own buffer.

The ramp tables are built so `table[k] = 0x8000 | k`, which is what makes a
painted pixel report the index that reached it; the per-texel ramps are flat
per ramp, which is what makes the textured pair report the texel instead.

## 4. ZBufferHelper 0x00464a90

A fourth member of the type-2 family: the same grammar and the same
`ll_anim_code` / `ll_anim_count` reader as the animation painters, writing
DWORDS into the Z buffer. A pixel is `(unsigned)index << 24` — the index byte
becomes the TOP byte of the Z key, which is what `docs/runtime/presentation.md`
and tri3d.c's header mean by "the Z value in the top byte". No palette, no
recolour, no mouse test; the row pitch is `g_sp_rowlen`, which the existing C
prologue sets to 0x200; the destination is `zbuf + dst->x*4 + dst->y*0x200`.

Two of SoftBlitAnimPlain's defects are absent here: `d_skip1` is
`dec edx / je tail_skip / add edi,4`, the correct order, and a repeat run that
exactly fills the row ends with `mov ecx,edx / rep stosd`, where `rep` with a
zero count writes nothing.

### FINDING 3 — a two-byte step in a four-byte-per-pixel buffer

The left-clip TRANSPARENT-run path is

```
        sub     edx, ecx
        jns     c_loop
        neg     edx
        lea     edi, [edi+edx*2]      <-- TWO bytes per pixel
        mov     ecx, g_sp_w
```

while every other output step in the function uses four: `add edi,4`,
`lea edi,[edi+ecx*4]`, `stosd`. It is the 16-bpp sibling's instruction left in
place. A transparent run that crosses `src->left` therefore lands the rest of
that row at HALF the offset it should — `overhang/2` dwords in instead of
`overhang` — so a Z sprite drawn with a left clip whose clip edge falls inside
a transparent run seeds the wrong columns for that row. REPRODUCED, with `*2`
spelled out at the site and the reason in a comment.

### Test `zbuf_blit` (ZBufferHelper half) — 8 checks

Its own encoder again, a five-row op-list sprite, and an independent reference
image: unclipped, top-skipped, right-clipped, left-clipped, and a destination
offset of (3,2) checked against the 0x200-byte row stride. The `*2` path is
exercised (a left clip of 4 into the wholly transparent row) and asserted to
paint nothing and return; making its consequence VISIBLE needs a painted run
after the transparent one in the same row, which this sprite has not got — the
arithmetic is recorded rather than pinned, and the check's value is that the
path runs at all.

## 5. BltAdvisor 0x004659a0

A bottom-up 16-bpp DIB into the locked surface. The source starts at
`dib + 0x28 + (h-1)*w*2` and walks UP by `w*2` per row while the destination
walks DOWN by `g_ddsd_pitch`; `mov ax, word ptr [edx+ecx]` with `edx = src -
dst` is just `src[i]`. When `g_screen_depth == 2` each word is widened from
RGB555 to RGB565 by

```
ebx = eax / and eax,1fh / and ebx,0ffffffe0h / shl ebx,1 / or eax,ebx
```

— the low 5 bits (blue) stay and everything above them moves up one, and
because only AX is written back the bit that shifts out of bit 15 never lands.
`test ecx,ecx / je` makes a zero-height DIB a no-op.

Tested in `zbuf_blit`: the bottom-up row order at an offset of (2,1) against a
direct construction, nothing touched outside the destination rectangle, the
555->565 widening recomputed independently, and the zero-height no-op.

## 6. ShowCapacityOverlay 0x004632b0 — ported, reviewed, not testable

Six `AICat` rows then a total. `esi` walks `&g_ai_cat[i] + 0x18`, so the four
fields it reads are `[esi-0x18]` objects, `[esi-0x10]` cap, `[esi-0x04]` pct,
`[esi]` scale, and `lea edx,[eax+eax*4] / lea edx,[edx+edx*4] / shl edx,2` is
`scale * 100`.

The argument order is the push order read backwards, and it interleaves a
double between two ints:

```
sprintf(buf, kCapRowFmt, *names, objects, cap, pct,
        product * 0.01, scale, min(product, scale*100) * 0.01)
```

`push eax` carries **scale** — eax still holds `[esi]` when that push happens,
and only then is eax reloaded with `objects`. The running total sums the
**clamped** value, not the product: `[esp+0x14]` is the clamped one and it is
what is added into the accumulator at `[esp+0x18]`. The totals line is
`sprintf(buf, kCapTotFmt, acc*0.01, g_visitor_cap_extra, g_visitor_cap,
g_visitor_limit)`, and both lines are drawn with
`Print(g_clip_rect.left + 8, g_clip_rect.top + <y>, buf, 2)` at y = 0x14 per
row and 0x96 for the total. `fild <int> / fmul kHundredth / fstp qword` is an
int widened through the FLOAT 0.01f and stored as a double, hence
`(double)x * (double)kHundredth`. The 0x3c and 0x2c stack adjustments after the
two calls confirm the argument counts (0x2c + 0x10 and 0x1c + 0x10).

No test: it formats with `sprintf` and draws with text.c's `Print`
(0x00454ba0), which needs a loaded font and a locked surface, and nothing it
does is decidable from a pixel buffer headlessly. The port was checked against
the asm's push order and stack adjustments instead, as above.

## 7. RenderTransSprite 0x00489190 — ported, reviewed, not testable

The depth-2 (RGB565) arm. The clip first: `drect` is the sprite rectangle
`(x, y, x+w-1, y+h-1)` trimmed to `g_clip_rect` and `srect` is how much each
edge lost, so `srect` is in sprite-local coordinates. The blit works on DWORDS
— two pixels at a time — with the source pointer aligned DOWN to a dword
boundary; each half is masked with `0xf7def7de` (the low bit of every 5/6/5
field cleared) and halved, and the two are added, which is a 50% blend with no
carry between channels. A source dword that is zero after the mask is
transparent. Every result is stored TWICE, to this row and to the row one
`dpitch` below, and both pointers advance two rows per pass — the effect is
drawn at half vertical resolution.

All three faults the file header lists are reproduced, each with the
instruction at the site:

1. `add eax,4` runs BEFORE `mov edx,[eax]` while the stores use `[eax-4]`, so
   the destination pair read for the blend is the one AFTER the pair written;
2. the opaque path's `jns pixel` falls THROUGH into the transparent-skip
   block, so one extra `add edi,4 / sub ecx,2` runs at the end of every row
   (harmless — both are reloaded per row);
3. the row and column counts come from `srect`
   (`srect.bottom - srect.top - 1`, `srect.right - srect.left - 2`), and the
   value left in eax — hence returned — is the advanced destination ROW
   POINTER, not a status. The early exits return 0.

No test: both surfaces are opened with `GetSprite` (printlist.c 0x00497c30),
which locks real DirectDraw surfaces — a generated trap in the test build,
where PORT-B's `legoland_hostwin` is not linked. Driving it would mean linking
the browser closure into `legoland_tests`, which `portable/cmake/tests.cmake`
documents as a PORT-A2 decision this lane must not take.

## 8. The three ST(0)-ABI stubs — unreachable, and why

None of these three can be ported: the argument arrives in the x87 stack, so
there is no C signature to give them. What this lane owed was a proof that
nothing in the portable build reaches them. An `.text` scan for `e8`/`e9`
relative branches and for absolute dwords equal to each address
(`port-b5-findcallers.py`, kept in the scratchpad) settles all three.

### FastSqrt 0x00426ab0 and FastRSqrt 0x00426980

* **No direct call anywhere in the image.** The only two references to their
  addresses are the two `lea eax, FastSqrt` / `lea eax, FastRSqrt` stores
  inside `FastSqrt_InitTables` (0x00426b10) and `FastRSqrt_InitTables`
  (0x004269e0) — at `.text+0x25b87` and `.text+0x25a77`.
* Both of those stores are inside `#ifndef LEGOLAND_PORTABLE` arms in
  `coaster5.c`, whose `#else` arms install `ll_FastSqrt` / `ll_FastRSqrt`
  (coastermath.c, same tables, same arithmetic, a C signature) instead.
* Every consumer goes through the hook: `VecMath_Sqrt` (coaster9.c 0x00426a90)
  calls `[g_fast_sqrt]` and `VecMath_ReciprocalSqrt` (coastertiny.c
  0x00426960) calls `[g_fast_rsqrt]`, and both already have portable arms that
  call the hook through a `float (*)(float)` cast.

So the traps are dead code. `tri_raster` keeps that honest: it runs both init
functions and asserts `g_fast_sqrt == ll_FastSqrt`, `g_fast_rsqrt ==
ll_FastRSqrt`, and that the two twins agree with libm to the tables'
precision over 40 inputs. The reasoning is written above the two functions in
`coastermath.c`.

### sub_458930 0x00458930 — and FINDING 4

This one is **not a BNV routine at all**. The scan finds **110 direct call
sites and one tail `jmp`**, spread right across the image — 0x00401a17, the
0x00419xxx person/vector block, 0x00422exx, tri3d.c's 0x004861xx / 0x004863xx
/ 0x0048650x, 0x0049bxxx. Every one sits immediately after an x87 computation
and reads the result out of eax, e.g. at `BuildChannelTables` 0x00486135:

```
  0x00486123: fild  dword ptr [esp + 0xc]
  0x00486127: fmul  dword ptr [0x4ab550]
  0x0048612d: fld   st(0)
  0x0048612f: fmul  dword ptr [0x4ab444]
  0x00486135: call  0x458930
  0x0048613a: mov   di, ax
```

It is the compiler's `(int)<float>` lowering: a `__ftol`-shaped helper that
takes the value in ST(0), `fistp`s it into the fixed scratch dword at
0x00667c3c and returns it in eax. It differs from the stock VC6 `_ftol` in one
way that matters — **it does not save and reload the control word to select
truncation**, so with the game's own nearest/masked CW it ROUNDS TO NEAREST.

Unreachable in the portable build for a structural reason: the call sites are
all source-level casts, and clang lowers a float->int cast to a native
instruction rather than to a call, so no portable caller exists and none can be
written.

**FINDING 4, which is a MATCHING question and not a porting one.** A source
cast that lowers to this helper rounds, while C requires truncation. Any site
whose reconstructed C spells `(int)f` and whose original bytes are
`call 0x458930` is therefore describing the wrong arithmetic, and the portable
build silently differs from the original by up to one unit at that site.
`LL_FISTP` in `ll_portable.h` is the round-to-nearest spelling the ported arms
use where the asm was explicit about it; the 110 implicit sites have not been
audited and are worth a pass by a matching lane. Recorded in `bnvpath.c` above
the function as well.

## 9. FINDING 5 — every LL_FISTP site in the wasm build was a latent trap

`ll_portable.h`'s `LL_FISTP` / `LL_FISTPD` were `__builtin_lrintf` /
`__builtin_lrint`. Those honour the CURRENT rounding mode, so clang cannot fold
them and emits a call to libm — and `gen_link.py` does not see emcc's libm in
its defined set, so the wasm32 closure generates **trapping stubs for both**:

```
portable/build-wasm/gen-browser/stubs.c:101
  unsigned int lrint(double a0)  { ll_gen_trap("GAME", "lrint",  "coaster3d.c"); ... }
  unsigned int lrintf(float a0)  { ll_gen_trap("GAME", "lrintf", "bnvpath.c, coaster10.c, coaster12.c..."); ... }
```

So every ported `LL_FISTP` arm in the tree — person3d.c's and math3d.c's
included, which is every bloke the 3D renderer draws — would have died in
`TRAP GAME lrintf` the first time it ran under node or in the browser. It had
never fired because nothing had called one: the title screen reaches only the
RLE painters. `tri_raster`, driving `DrawFlatTexTri`'s u/v conversion, is the
first thing that did, and it failed under node while passing natively (SSE's
`cvtss2si` lowers `lrintf` inline with MXCSR already at nearest-even).

Fixed in `ll_portable.h` by spelling the conversion out — `ll_fistp_f` /
`ll_fistp_d`, plain arithmetic, round-half-to-EVEN hard-coded, which is the
mode the game's own control word selects and the only one the macro's comment
ever claimed. No libcall, so the two generated traps are now unreachable from
the game's C as well as being dead in the native build. `stubs.c` still defines
them (the generator reads the referenced set, and nothing references them any
more once the tree is rebuilt from clean) — worth a line in PORT-A's census
when it next runs.

## 10. The VC6 gate

Per touched file, after the integrator's quiet window.

## 11. Builds and the census

`portable/tools/linkreport.py portable/build`: **asm stubs 15 -> 6**, and the
six are exactly the bodies no one owes a C body for:

* `bnvpath.c:129 sub_458930`, `coastermath.c:162 FastSqrt`,
  `coastermath.c:194 FastRSqrt` — the ST(0) ABI, proved unreachable in §8;
* `coaster13.c:767 TrackShade_FillPoly`, `coastershade2.c:445 Span_FillShade`,
  `coastershade2.c:595 Span_FillShadeZ` — the coaster-track Gouraud and
  textured span fillers, outside this lane's brief and the obvious next lane.

Everything else in the report is unchanged: game-fn 0, host 0, alias 0,
duplicates 0, prototype conflicts 0.
