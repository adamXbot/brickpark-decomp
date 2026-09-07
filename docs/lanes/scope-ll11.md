# Scope LL11 — unreferenced (dead) coaster track / castleobj functions

Branch `scope/LL11` from `main` `7d756410`. New file `LEGOLAND/unref3.c`.
Object prefix `/tmp/sll11_`. Brief:
`docs/SCOPE_LL11_unref_coaster_track_c.md`.

## Status

**16 / 16 exact, 658 / 658 instructions, 2153 / 2153 bytes.** `audit.py` ends
PASS with sixteen `[OK]` lines; `relocs.py` zero MISMATCH (two UNRESOLVED,
both literals: the `"Track %2x, ..."` string and the pooled `1/30`); `/W3`
clean.

| address | name | insns | % | audit | marker |
| --- | --- | ---: | ---: | --- | --- |
| 0x004238a0 | Coaster3D_PlotPoints | 49 | 100 | [OK] | FUNCTION |
| 0x00423930 | Castle_IsBuilt | 2 | 100 | [OK] | FUNCTION |
| 0x004239a0 | CastleTrack_Noop | 1 | 100 | [OK] | FUNCTION |
| 0x00426000 | Coaster3D_SetupFlatView | 51 | 100 | [OK] | FUNCTION |
| 0x004260e0 | Coaster3D_GetViewZScale | 2 | 100 | [OK] | FUNCTION |
| 0x00426230 | Coaster3D_TransformPoints | 11 | 100 | [OK] | FUNCTION |
| 0x00426680 | CoasterRegions_UpdateClipCodes | 14 | 100 | [OK] | FUNCTION |
| 0x004267b0 | WireBox_Draw | 56 | 100 | [OK] | FUNCTION |
| 0x00426850 | WireBox_Init | 80 | 100 | [OK] | FUNCTION |
| 0x00426be0 | TrackBlob_Dump | 26 | 100 | [OK] | FUNCTION |
| 0x004272a0 | Coaster_WriteSaveBlob | 44 | 100 | [OK] | FUNCTION |
| 0x00427310 | Coaster_ReadSaveBlob | 78 | 100 | [OK] | FUNCTION |
| 0x004274f0 | Track_StampSquareTiles | 48 | 100 | [OK] | FUNCTION |
| 0x00427570 | Track_RestoreSquareBaseMap | 28 | 100 | [OK] | FUNCTION |
| 0x00428b80 | Coaster3D_PlotPieceRails | 98 | 100 | [OK] | FUNCTION |
| 0x0042a020 | Track_FindSphereCrossing | 70 | 100 | [OK] | FUNCTION |

Eleven of the sixteen were exact on the first reconstruction pass. The five
that needed iteration are the five levers below.

## What these are

They are one coherent DEBUG / TOOLING subsystem that the shipped game never
calls, kept by the linker because the build had no `/OPT:REF`:

* a 16-bit point plotter and the 30-step line rasteriser above it
  (0x004237f0, itself dead) — the only 2D primitives in the coaster module;
* a wire-box mesh (build + draw) and a per-piece rail plotter, both drawing
  through that plotter — the visual debug overlay for track geometry;
* a "flat" camera setup that is `Coaster3D_SetupView` with the map scroll
  taken out;
* the coaster's own `RollerCoaster\RollerCoaster.sav` blob reader/writer and
  a `DBPrintf` dump of its square table;
* two map-square helpers (stamp the track's 2x2 tiles / restore them from
  the base map);
* a sphere-crossing search along the track, driven by the bisection root
  finder at 0x00429e20 through the module's own hook slot at 0x004b63fc.

## Names chosen (none of these is exported)

| address | name | why |
| --- | --- | --- |
| 0x004238a0 | `Coaster3D_PlotPoints` | locks the screen sprite and writes one 16-bit pixel per in-window point |
| 0x00423930 | `Castle_IsBuilt` | returns castleobj.c's `g_610a04`, "the castle is down" |
| 0x004239a0 | `CastleTrack_Noop` | a one-byte `ret`; empty `void f(void)` |
| 0x00426000 | `Coaster3D_SetupFlatView` | `Coaster3D_SetupView` without the scroll |
| 0x004260e0 | `Coaster3D_GetViewZScale` | returns `g_view_xf.m[6]`, the y-from-z term |
| 0x00426230 | `Coaster3D_TransformPoints` | `TransformVerts` bound to the view matrix and stride 0x10 |
| 0x00426680 | `CoasterRegions_UpdateClipCodes` | re-runs 0x004265d0 over the whole clip ring |
| 0x004267b0 | `WireBox_Draw` | transforms and strokes the box's 12 edges |
| 0x00426850 | `WireBox_Init` | builds the 8-vertex / 12-edge prism |
| 0x00426be0 | `TrackBlob_Dump` | `DBPrintf` of the blob's square table |
| 0x004272a0 | `Coaster_WriteSaveBlob` | `CreateFileA`/`WriteFile` of the whole blob |
| 0x00427310 | `Coaster_ReadSaveBlob` | its `ReadFile` counterpart |
| 0x004274f0 | `Track_StampSquareTiles` | `SetMapTile` over the piece's 2x2 |
| 0x00427570 | `Track_RestoreSquareBaseMap` | `RestoreBaseMap` over the same 2x2 |
| 0x00428b80 | `Coaster3D_PlotPieceRails` | 30 x 3 samples of a piece's rail hooks, plotted |
| 0x0042a020 | `Track_FindSphereCrossing` | walks segments until the probe brackets a zero |

Globals and callees named for the first time here:

* `0x004b5cf4` `g_coaster_save_path[]` — the literal
  `"RollerCoaster\\RollerCoaster.sav"`, reached as an ARRAY (both save
  helpers emit `mov eax,OFFSET g_coaster_save_path / test eax,eax`, a null
  test VC6 does not fold away).
* `0x006122a0` `g_rail_pts[90]` and `0x006159c8` `g_rail_screen[90]` — the
  rail plotter's sample and projection buffers.
* `0x00615f84` `g_probe_pos`, `0x00615f8c` `g_probe_centre`,
  `0x00615fd0` `g_probe_radius`, `0x00615fd4` `g_probe_band`,
  `0x00615fd8` `g_probe_r2`, `0x00615fdc` `g_probe_outer2`,
  `0x00615fe0` `g_probe_inner2` — the parameter block 0x00429cf0 reads.
* `0x004b63fc` `g_find_bracket` — a function-pointer global holding
  0x00429e20, the bisection bracket finder (it calls its `float(*)(float)`
  argument at both ends and returns 0 when the signs agree).
* `0x00429cf0` `TrackProbe_Distance` — the distance function it bisects.
* `0x004265d0` `Rect_ClipCode(const int* rect, const int* ref)` — four
  strict edge tests, each failing to its own `xor eax,eax / ret`.
* `0x004237f0` `Coaster3D_DrawLine(const ScreenPt*, const ScreenPt*, short)`
  — 30 interpolated points (`1/30` at `0x004ab44c`) handed to
  `Coaster3D_PlotPoints`. Its colour parameter is 16-bit: every caller only
  loads `cx`.
* `0x00829980` `g_basic_tiles` retyped `unsigned short*` here (coaster.c
  declares it `void*`); `Track_StampSquareTiles` needs `*g_basic_tiles + 1`
  as a 16-bit value. **Extern-type divergence, deliberate — do not align
  coaster.c's declaration.**

## Mechanics recovered

- **The screen plot.** `Coaster3D_PlotPoints` locks the SCREEN surface
  (`GetSprite(&surf, 0)`), and for each point strictly inside
  `g_view_left..g_view_right` / `g_view_top..g_view_bottom` writes
  `((unsigned short*)surf.bits)[(surf.pitch >> 1) * y + x] = colour`. The
  bounds test is strict on ALL FOUR edges — a point exactly on the left or
  top edge is dropped. Stride is 0x10, the `TransformVerts` layout for this
  module, so the vertex is `{int x, y, z, clip}`.
- **The wire box** is a 0xcc-byte record: `short colour` at +0x00, vertex
  count 8 at +0x04, edge count 12 at +0x08, 12 index PAIRS at +0x0c, and 8
  `Vec3f` at +0x6c. `WireBox_Init(hw, hh, depth, b)` lays the rectangle
  `(+hw,-hh) (+hw,+hh) (-hw,+hh) (-hw,-hh)` at z = 0, copies it to
  z = -depth, writes the four bottom edges, derives the four top edges by
  `+4` in a loop, and writes the four verticals.
- **`WireBox_Draw(pos, rot, b)`** is `MakeTransform` -> `TransformVec3` of
  the 8 corners -> `Coaster3D_TransformPoints` -> one `Coaster3D_DrawLine`
  per edge pair, re-reading the edge count and the colour every iteration
  (the call may alias). Frame: `Mat4` lowest, then the 8 `ScreenPt`
  (0x80 bytes), then the 8 world `Vec3f` (0x60) — 0x120 total.
- **`Coaster3D_PlotPieceRails(o, origin)`** samples the drawable's slot-1,
  slot-0 and slot-2 POSITION hooks (`o->hooks[k].get_pos`, coaster3d.c's
  `PosHooks` pair table at +0x4c) at 30 evenly spaced parameters from
  `o->t0` with step `(o->t1 - o->t0) / 30`, offsets each sample by
  `origin`, and plots the 90 results in white (`-1`).
- **`Coaster3D_SetupFlatView(use_base)`** picks `g_view_base` or
  `g_view_tmpl` on the argument, zeroes the transform's `m[3]`/`m[7]`,
  publishes the config's view window as the four clip bounds, zeroes the
  eye, and composes the basis with a plain identity into `g_view_matrix`.
  Both cdecl clean-ups merge into one `add esp,0x10`.
- **The .sav blob** is self-describing: its first dword is its own byte
  length, and `Coaster_WriteSaveBlob` sends exactly that many bytes.
  `Coaster_ReadSaveBlob` is `CoasterModel_LoadFile` (coaster7.c) without the
  chdir pair and with the fixed path. The blob's square table is
  `{int type; short x; short y}` entries at +0x0c with the count at +0x08.
- **`Track_FindSphereCrossing`** copies the caller's `RoutePos` into a local,
  publishes the probe block (radius, band, r^2, (r+b)^2, (r-b)^2, centre and
  the cursor's address), then asks `g_find_bracket` for a sign change of
  `TrackProbe_Distance` — first over `[from, geom->t1]`, then over each
  following segment's `[t0, t1]` after
  `TrackCursor_AdvanceGeometry` — and copies the cursor out.

## Original bugs reproduced (commented at the site)

1. `WireBox_Init` never writes `verts[2].z`. The other three corners get
   their `0.0f`; that one keeps whatever was in the buffer. (The copy loop
   overwrites `verts[6].z` with `-depth` anyway, so only `verts[2]` is
   undefined.)
2. `WireBox_Init`'s third VERTICAL edge is `(1,5)` again instead of `(2,6)`,
   so the box is drawn with one edge doubled and one missing.
3. `Coaster_WriteSaveBlob` reads the length out of the blob BEFORE its own
   null test on the blob pointer, so the guard cannot save a null argument.

## Levers learned (with evidence)

- **A struct copy chosen by a condition must be an if/else over two whole
  struct assignments.** `Coaster3D_SetupFlatView`: both
  `g_view_xf = flag ? g_view_base : g_view_tmpl` and a
  `const Mat4* src = flag ? &a : &b; g_view_xf = *src;` schedule the
  rep-movsd count (`mov ecx,0x10`) AFTER the `lpConfig` load and hoist the
  zero-extension `xor edx,edx` above the copy — 48/51 for either. Written as
  `if (flag) g_view_xf = a; else g_view_xf = b;` the same code is 51/51.
  Twelve other spellings (config in a local, four bound-store orders, an
  `int`-local staging pass, `!flag` inverted, `x + w` vs `w + x`, eye first,
  the transform stores after the bounds) were measured; none reaches it and
  the `int`-local staging is the worst at 30/48.
- **Two count fields must be written FIRST when the rest of the initialiser
  is float stores.** `WireBox_Init`: with `b->nverts = 8; b->nedges = 12;`
  after the eight corner stores, VC6 sinks them BELOW the three-zero web and
  lowers `verts[0].x = hw` as `fld/fstp` instead of the original's GPR copy
  — 66/80. First, everything schedules: 80/80. Nine statement orders
  measured; the next best (the three `.z` stores hoisted to the top) is
  78/80, `y` before `x` per vertex is 65/81, and moving the copy loop before
  the edge table is 53/80.
- **A named float local for a float PARAMETER argument is free and changes
  the argument lowering.** `Track_FindSphereCrossing`: `f0 = from;` before
  the global stores, then `g_find_bracket(fn, f0, tb, hit)`. `f0` is
  coalesced onto the parameter's own slot so it costs no instruction, but
  the argument becomes x87-delivered — the original's `push ecx` reserve
  plus `fld from / fstp [esp]`. Passing `from` directly is `mov ecx / push
  ecx`, one instruction SHORT, and stays 69 of 70 across all 24 global-store
  permutations, `(float)from`, `*(float*)&from` and both `band`/`centre`
  volatile probes.
- **`*(volatile float*)&x` buys the same third instruction but pins the
  schedule** (RA11's "a volatile access cannot cross any store"): 60/70,
  with the two argument pushes and the `g_probe_centre` store left on the
  wrong side of the barrier. Prefer the named local. Recorded because the
  volatile is the obvious first try and it is a local maximum.
- **VC6 swaps an adjacent pointer/float pair of global stores.**
  `Track_FindSphereCrossing` emits `g_probe_radius` before `g_probe_centre`;
  the SOURCE must say centre first. All 24 orders of
  {radius, centre, band, pos} were measured: centre-then-radius is 2 ahead
  of every alternative, and writing them in emission order costs 2.
- **Which of two same-typed locals a repeated assignment targets decides the
  dead-argument-slot homes.** `Track_FindSphereCrossing` has two float
  locals homed in the dead `radius` (+8) and `start` (+0x0c) argument slots.
  The pre-loop `= cur.geom->t1` must be assigned to the SAME local the loop
  assigns `cur.geom->t1` to; giving it the loop's `t0` local swaps both
  homes and nothing else (68/70 -> 70/70). Declaration order is irrelevant,
  as recorded.
- **A global array indexed by a running int, not walked by a pointer.**
  `Coaster3D_PlotPieceRails` shows three strength-reduced cursors —
  0x006122a0, +4 and +8, each stepping 0x0c — one per FIELD of the `Vec3f`
  array. A walking `Vec3f* p` gives ONE register and the wrong code; the
  index form gives an induction variable per (base, offset) pair, and the
  index itself is eliminated.
- **A `short` argument read from memory is `mov cx,[mem] / push ecx`.**
  Both `WireBox_Draw` (the box's colour) and `Track_StampSquareTiles` (the
  tile id) depend on the callee's parameter being 16-bit; declaring
  `SetMapTile`'s tile `unsigned short` and `Coaster3D_DrawLine`'s colour
  `short` is what keeps the un-zeroed high half.
- **A string literal reached as an `extern char[]` keeps its null test.**
  Both .sav helpers begin `mov eax,OFFSET g_coaster_save_path / test eax,eax`
  — VC6 does not fold `if (array)` even though the address is a constant.
  Declaring the path as a `char*` variable would emit a load instead.
