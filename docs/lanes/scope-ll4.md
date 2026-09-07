# Scope LL4 — coaster shade / blit table callees (inventory group 4)

Branch `scope/LL4`. File `LEGOLAND/coastershade2.c`. Object prefix `/tmp/sll4_`.
Brief: `docs/SCOPE_LL4_coaster_shades.md`.

## Status

| address | name | insns | % | audit | marker |
| --- | --- | ---: | ---: | --- | --- |
| 0x0041f4e0 | Romberg_Evaluate | 78 | 100 | [OK] | FUNCTION |
| 0x0041f880 | TrackCursor_RetreatGeometry | 29 | 100 | [OK] | FUNCTION |
| 0x0041f8d0 | Span_FillFlat | 106 | ~57 | WIP | 106/106i, 309/307B, 88 mismatch |
| 0x0041fba0 | Span_FillFlatZ | 136 | ~63 | WIP | 136/136i, 398/399B, 62 mismatch |
| 0x0041fd80 | Span_FillShade | 162 | ~74 | WIP | 162/162i, 507/505B, 106 mismatch |
| 0x0041ff80 | Span_FillShadeZ | 202 | ~71 | WIP | 202/202i, 633/633B, 62 mismatch |
| 0x00420200 | IntegrateSimpson | 81 | ~68 | WIP | 81/81i, 251/258B, 61 mismatch |
| 0x00420780 | GetCoasterTexture | 3 | 100 | [OK] | FUNCTION |

**3 / 8 exact.** All five WIPs are at instruction-count equality. `audit.py`
PASS; `relocs.py` zero MISMATCH on the three FUNCTION bodies (UNRESOLVED
float-pool literals on Romberg); `/W3` clean.

## Names

- `GetCoasterTexture` — indexed reader of `g_coaster_tab_c` (0x004d89c8).
- `TrackCursor_RetreatGeometry` — inverse of `TrackCursor_AdvanceGeometry`.
- `Romberg_Evaluate` — 4-level Romberg combine. Callees `Romberg_Build`
  (0x0041f3e0) and `Romberg_Release` (0x0041f4c0) are LL3's.
- `Span_FillFlat` / `Span_FillFlatZ` — table 0x004b5648 / 0x004b564c.
- `Span_FillShade` / `Span_FillShadeZ` — `g_span_fillers[0]` / `[1]`
  (0x004b5658 / 0x004b565c).
- `IntegrateSimpson` — adaptive Simpson; caller is Track_MeasureDistance
  (0x0042a1b0).

Globals first named here: `g_one_sixth` (0x004b5610), `g_span_ramp`
(0x004d89c0), `g_span_dshade_hi` (0x004d89b4), `g_span_dshade_lo`
(0x004d89bc). `g_raster_bits` is coaster8.c's name for 0x004b5b20;
`g_shade_clamp_mid` is schoolcar8.c's.

## Mechanics

- **GetCoasterTexture(i)** returns `g_coaster_tab_c[i]`.
- **TrackCursor_RetreatGeometry**: live `geom->prev` is stored and the
  function returns; otherwise `node->prev`, `GetTrackNodeWorldPos`, then
  walk `+0x50` to the last object.
- **Romberg_Evaluate(fn, ops, t, h, out)**: `half = h*0.5`; build a 4-level
  tableau; `alloc(2)`; `add(tab[1][1], tab[1][2], a)`;
  `add(tab[3][0], tab[3][1], b)`; `scale(b, 1/6)`; `add(a, b, out)`;
  `scale(out, 0.5/half)` (= 1/h); free the pair and the tableau.
  Route_GetMassAndPower calls this with 0x0041db20, the 0x004d8270
  PhysOps block, the live route parameter and dt=0.1.
- **Span_FillFlat**: ZBuffer_FillPoly sibling — `y1++` sentinel, 0x14
  interpolants, hand-written fill (`xchg`, `add ebx,1`, `jns/jmp`). Dead
  store of `g_zb_base`. Pixel is `g_shade_tab[tag][grad[0]]` into
  `g_raster_bits`.
- **Span_FillFlatZ**: same, plus left-edge z and a flat `grad[1]` across
  the span; write only when interpolated z is not behind the z-buffer
  (unsigned compare).
- **Span_FillShade**: Gouraud. Negative `grad[1]` is negated and a flip
  flag selects `adc` vs `sbb`. Shade is 16.16, `ror 16`, clamped through
  `g_shade_clamp_mid`, then `ramp[clamped]`.
- **Span_FillShadeZ**: Gouraud+Z. Left edge carries x, shade and z.
  `dshade_lo` packs the shade fraction with `(grad[2]>>8)` so one add
  advances both. Inner loop steals EBP for the ramp pointer (`push ebp`).
- **IntegrateSimpson**: `s = fa+fb+4*odds+2*evens`, `approx = s*h`,
  return `approx/3`. Seed is `s*h*0.5*3`. Loop while
  `|approx-old| > tol*|old|`.

## Levers

- Retreat walk: named `GetTrackNodeWorldPos` result for the first
  `if (geom->next)`, then `cursor->geom = cursor->geom->next` in the
  loop. The two-local walk emitted `add eax,0x50` and stalled at 25/29.
- Romberg: `half = h*0.5` kept, last scale is `0.5f / half` (not
  `1.0f / h`). `g_one_sixth` is a raw dword push from 0x004b5610.
- Empty `__asm {}` forces Simpson's EBP frame. A volatile `SimpsonFrame`
  plus the exe float-pool globals (`g_half` / `g_two` / `g_three` /
  `g_four` / `g_third`) puts n at `[ebp-0x18]` and stops `2.0` becoming
  `fadd st,st`. Residual: sibling `fn(a)`/`fn(b)` merge to `add esp,8`;
  the counted loop latches with `jle`/`inc` instead of `jmp`-to-`jg` /
  `add ecx,1`.
- Span interpolant array `ed[4]` of 0x14 is load-bearing (ZBuffer_FillPoly
  rule). FlatZ parks z at +8; Shade parks shade at +4. Residuals are the
  same class as ZBuffer_FillPoly: row/ylast homes swapped with argument
  slots. Wrapping the scalars in a struct grew the frame and cost bytes.

## Ruled out (do not rerun)

- Span_FillFlat: sl-struct of {ylast,pitch,dead,row} → 313B / 87 mismatch
  (worse). `n = color` with `__asm` using `n` → 36.7%.
- IntegrateSimpson: ESP frame without `__asm` → 5.6%. Volatile pfn → 78
  mismatch. Separate `int i` plus `f.i = i` → 64 mismatch.
