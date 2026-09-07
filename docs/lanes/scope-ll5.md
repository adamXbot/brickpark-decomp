# Scope LL5 — castle entrance track helpers

Branch `scope/LL5`. File `LEGOLAND/castletrack2.c`. Object prefix `/tmp/sll5_`.
Brief: `docs/SCOPE_LL5_castle_entrance.md`.

## Status

| address | name | insns | % | audit | marker |
| --- | --- | ---: | ---: | --- | --- |
| 0x00421ce0 | TrackCurve_InitArc | 38 | 94.7 | no (mismatch 3) | WIP |
| 0x00422180 | TrackCurve_InitCubic | 43 | 100 | [OK] | FUNCTION |
| 0x00421ab0 | TrackCurve_InitLine | 49 | 100 | [OK] | FUNCTION |

**2 / 3 exact, 92 / 130 instructions.** `relocs.py` zero MISMATCH on the
two `FUNCTION` bodies (UNRESOLVED only the pooled `0.0f` literal). `/W3` clean.

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
  of the caller's floats) so the stores stay GPRs.
- **Cubic** (`TrackCurve_InitCubic`): saves `{to.z, from.z, 0, 0}`, zeros both
  endpoints' z (mutates the caller), calls InitLine for the XY run, then
  `cubic = H * v` with H at 0x004b5abc and installs hooks at 0x004dd5e0.
  H is the Hermite basis for `{z1, z0, s1, s0}`; the last two columns are
  unused here (slopes stay 0). The inner product must name the table element
  (`t = *row; acc += t * *col`) so `fld` lands on the table, not `v`.
  `while ((int)row <= (int)&H[3][0])` is the original signed `jle`.

## InitArc residual

38i/114B exact size. First divergence at the radius pair after the third
Vec3f copy:

```
orig: mov ecx,[esp+18] ; mov edx,[esp+14] ; mov [eax+28],ecx ; xor ecx,ecx ; mov [eax+24],edx
ours: mov edx,[esp+18] ; mov [eax+28],edx ; mov edx,[esp+14] ; xor ecx,ecx ; mov [eax+24],edx
```

Everything else is identical (including the esi pop between offset.y and
offset.z). Adjacent `r1,r0` stores pair-load but address-sort to +0x24 first
(34/38). Splitting with `t0=0` between them gives this 36/38: +0x28 first
and xor in the right place, but r1 is in edx (just-freed dest pointer) and
r0 is not hoisted. Address-taken `volatile` on the stack args hoists the
pair and puts r1 in ecx (38-instruction match of the radii) but delays
`pop esi` and shifts every `[esp+N]` by one slot. Dest-pointer spellings
flip store order and swap ecx/edx. At 36/38 pending a hoist of `r0` into
edx before the +0x28 store, with r1 in ecx.

## Levers

- Do not name a walking `float*` dest on a 3-element `to[i]-from[i]` loop
  (LP14): dest is `lea ecx,[curve+0xc]`, only `to` is differenced from `from`.
- Named table element in an `acc +=` product selects `fld` order (FP01
  accumulator exception).
- Signed `int` compare of a walked pointer against the last-row address
  gives `jle` (pointer `<=` is `jbe`).
- Separate symbols for vtable slices; array-index forms do not relocate.
- Arc radii as `int` parameters keep the original dword moves.

## Extern-type divergences

- `TrackCurve_InitArc` takes `int r0, int r1` here; coaster7.c /
  coaster3d.c declare `float`. Do not align those callers.
- `TrackCurve_InitCubic` takes `Vec3f* from, Vec3f* to` (mutated); no
  prior extern.
