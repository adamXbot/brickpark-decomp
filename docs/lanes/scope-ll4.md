# Scope LL4 — coaster shade / blit table callees (inventory group 4)

Branch `scope/LL4`. File `LEGOLAND/coastershade2.c`. Object prefix `/tmp/sll4_`.
Brief: `docs/SCOPE_LL4_coaster_shades.md`.

## Status

| address | name | insns | % | audit | marker |
| --- | --- | ---: | ---: | --- | --- |
| 0x0041f4e0 | Romberg_Evaluate | 78 | 100 | [OK] | FUNCTION |
| 0x0041f880 | TrackCursor_RetreatGeometry | 29 | 100 | [OK] | FUNCTION |
| 0x0041f8d0 | Span_FillFlat | 106 | ~57 | WIP | WIP (106/106i, 309/307B, 88 mismatch) |
| 0x0041fba0 | *(open)* | 136 | — | — | — |
| 0x0041fd80 | *(open)* | 162 | — | — | — |
| 0x0041ff80 | *(open)* | 202 | — | — | — |
| 0x00420200 | IntegrateSimpson | 81 | ~68 | WIP | WIP (81/81i, 251/258B, 61 mismatch) |
| 0x00420780 | GetCoasterTexture | 3 | 100 | [OK] | FUNCTION |

**3 / 8 exact.** `audit.py` PASS; `relocs.py` zero MISMATCH (UNRESOLVED
float-pool literals on Romberg); `/W3` clean.

## Names

- `GetCoasterTexture` — indexed reader of `g_coaster_tab_c` (0x004d89c8).
- `TrackCursor_RetreatGeometry` — inverse of `TrackCursor_AdvanceGeometry`.
- `Romberg_Evaluate` — 4-level Romberg combine. Callees `Romberg_Build`
  (0x0041f3e0) and `Romberg_Release` (0x0041f4c0) are LL3's.
- `Span_FillFlat` — table slot 0x004b5648; ZBuffer_FillPoly sibling that
  paints `g_shade_tab[tag][grad[0]]` into `g_raster_bits`.
- `IntegrateSimpson` — adaptive Simpson; caller is Track_MeasureDistance
  (0x0042a1b0).

`g_one_sixth` (0x004b5610) first named here. `g_raster_bits` is coaster8.c's
name for 0x004b5b20.

## Mechanics

- **GetCoasterTexture(i)** returns `g_coaster_tab_c[i]`.
- **TrackCursor_RetreatGeometry**: live `geom->prev` is stored and the
  function returns; otherwise `node->prev`, `GetTrackNodeWorldPos`, then
  walk `+0x50` to the last object.
- **Romberg_Evaluate(fn, ops, t, h, out)**: `half = h*0.5`; build a 4-level
  tableau; `alloc(2)`; `add(tab[1][1], tab[1][2], a)`;
  `add(tab[3][0], tab[3][1], b)`; `scale(b, 1/6)`; `add(a, b, out)`;
  `scale(out, 0.5/half)` (= 1/h); free the pair and the tableau. 0 if the
  build returns null. Route_GetMassAndPower calls this with 0x0041db20,
  the 0x004d8270 PhysOps block, the live route parameter and dt=0.1.
- **Span_FillFlat**: ZBuffer_FillPoly shape — `y1++` sentinel, 0x14
  interpolants, hand-written fill (`xchg`, `add ebx,1`, `jns/jmp`). Dead
  store of `g_zb_base`. Colour from the shade ramp, dest is the colour
  buffer.
- **IntegrateSimpson**: trapezoid refinement that keeps
  `s = fa+fb+4*odds+2*evens`, `approx = s*h`, return `approx/3`. Seed
  approx is `s*h*0.5*3` (different scale, so the first refine always
  runs). Loop while `|approx-old| > tol*|old|`.

## Levers

- Retreat walk: named `GetTrackNodeWorldPos` result for the first
  `if (geom->next)`, then `cursor->geom = cursor->geom->next` in the
  loop. The two-local walk emitted `add eax,0x50` and stalled at 25/29.
- Romberg: `half = h*0.5` kept, and the last scale is `0.5f / half` (not
  `1.0f / h`) so the overwritten h-slot matches. `g_one_sixth` is a raw
  dword push from 0x004b5610.
- Empty `__asm {}` forces Simpson's EBP frame; without it VC6 uses ESP
  and enregisters n/i. A volatile `SimpsonFrame` plus the exe float-pool
  globals (`g_half` / `g_two` / `g_three` / `g_four` / `g_third`) puts n
  at `[ebp-0x18]` and stops `2.0` becoming `fadd st,st`. Residual: sibling
  `fn(a)`/`fn(b)` still merge to `add esp,8`; the counted loop latches
  with `jle`/`inc` instead of the original's `jmp`-to-`jg` / `add ecx,1`.
- Span_FillFlat 0x14 interpolant array (`ed[4]`) is load-bearing, same
  as ZBuffer_FillPoly. Frame is still 0x58 vs 0x60.
