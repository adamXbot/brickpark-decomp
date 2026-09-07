# Scope LL5 — castle entrance track helpers

Branch `scope/LL5`. File `LEGOLAND/castletrack2.c`. Object prefix `/tmp/sll5_`.
Brief: `docs/SCOPE_LL5_castle_entrance.md`.

## Status

| address | name | insns | % | audit | marker |
| --- | --- | ---: | ---: | --- | --- |
| 0x00421ce0 | TrackCurve_InitArc | 38 | 100 | [OK] | FUNCTION |
| 0x00422180 | TrackCurve_InitCubic | 43 | 100 | [OK] | FUNCTION |
| 0x00421ab0 | TrackCurve_InitLine | 49 | 100 | [OK] | FUNCTION |

**3 / 3 exact, 130 / 130 instructions.** `relocs.py` zero MISMATCH
(UNRESOLVED only the pooled `0.0f` literal on Line/Cubic). `/W3` clean.

## Names

Callers already named 0x00421ab0 `Curve_InitLine` (coaster7.c) and 0x00421ce0
`Curve_InitCorner` / `Piece_InitCurve` (coaster7.c / coaster3d.c). Bodies
install the line / arc / cubic slices of schoolcar.c's `g_car_class_vt`
(0x004dd5e0 + 8 / +16 / +0), so the definitions are `TrackCurve_InitLine`,
`TrackCurve_InitArc`, `TrackCurve_InitCubic`.

`0x004dd600` / `0x004dd620` are declared as `g_curve_line_hooks` /
`g_curve_arc_hooks` because `&g_car_class_vt[8]` does not relocate in the
object (matchfull sentinel). They are the same table schoolcar.c fills.

`0x004b5abc` is `g_cubic_hermite[4][4]`, first named here.

## Mechanics

- One 0x58-byte curve record (RouteGeom / Curve / PieceDesc).
- **Line** (`TrackCurve_InitLine`): `dir[i] = to[i] - from[i]` as a counted
  loop (do not name a walking dest pointer — that strength-reduces dest
  against `from` and shifts every register). `length = sqrt(sum squares)`,
  `pos = *from`, `offset` copied, `[t0,t1] = [0,1]`, hooks at 0x004dd600,
  links cleared. `from` stays in ebx because the pos copy after the loop
  keeps it live.
- **Arc** (`TrackCurve_InitArc`): copy three Vec3f, radii at +0x24/+0x28,
  `[0, pi/2]` with the same 1.57079506f as `TrackCurve_GetQuarterTurnSamples`,
  hooks at 0x004dd620, links cleared. Parameters `r0,r1` are `int` (bit copies
  of the caller's floats) so the stores stay GPRs. Write `r0` then `r1` in
  source; VC6 pair-loads `[esp+18]`/`[esp+14]` into ecx/edx and emits the
  +0x28 store first (RA06 pair permutation). Writing `r1` first address-sorts
  the other way or sinks the second load.
- **Cubic** (`TrackCurve_InitCubic`): saves `{to.z, from.z, 0, 0}`, zeros both
  endpoints' z (mutates the caller), calls InitLine for the XY run, then
  `cubic = H * v` with H at 0x004b5abc and installs hooks at 0x004dd5e0.
  H is the Hermite basis for `{z1, z0, s1, s0}`; the last two columns are
  unused here (slopes stay 0). The inner product must name the table element
  (`t = *row; acc += t * *col`) so `fld` lands on the table, not `v`.
  `while ((int)row <= (int)&H[3][0])` is the original signed `jle`.

## Levers

- Do not name a walking `float*` dest on a 3-element `to[i]-from[i]` loop
  (LP14): dest is `lea ecx,[curve+0xc]`, only `to` is differenced from `from`.
- Named table element in an `acc +=` product selects `fld` order (FP01
  accumulator exception).
- Signed `int` compare of a walked pointer against the last-row address
  gives `jle` (pointer `<=` is `jbe`).
- Separate symbols for vtable slices; array-index forms do not relocate.
- Arc radii as `int` parameters keep the original dword moves.
- Adjacent radius stores: source `r0` then `r1` emits original `r1`/`r0`
  pair-load and +0x28-first stores (RA06). Source `r1` then `r0` address-sorts
  to +0x24 first. Splitting with `t0=0` sinks the second load (36/38).

## Extern-type divergences

- `TrackCurve_InitArc` takes `int r0, int r1` here; coaster7.c /
  coaster3d.c declare `float`. Do not align those callers.
- `TrackCurve_InitCubic` takes `Vec3f* from, Vec3f* to` (mutated); no
  prior extern.
