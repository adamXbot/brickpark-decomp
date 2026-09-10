# Scope PORT-A3 — one block per object, trap naming, and the node spine

> **Status: IN PROGRESS (claimed 2026-09-11 by PORT-A3).** Branch
> `scope/PORT-A3` from main `b28e35cb`. PORT-A2's follow-up on the generator and
> the node harness. Nothing in `LEGOLAND/*.c` is touched, so the VC6 gate has
> nothing to check for this lane.

## 1. ONE block per object: interior aliases

**Done.** `gen_link.py` sized every rebuilt global by the gap to the next NAMED
address, which is correct for unnamed data (it is what lets PORT-A2 re-point a
pointer into the middle of a block) and wrong for a named object that other
names reach into at an offset: the object came out as several separately
16-byte-aligned arrays.

The generator now reads an object's extent out of the game's own declaration —
array bounds times an element size that is a language fact — and emits ONE
block, with the other names as offset aliases of it.

### The measurement: 15 objects, 100 interior names

Every case in the image, from `gen/manifest.md`'s new section. The ones that
matter to a running game are marked.

| host | addr | block | declared | interior names |
| --- | --- | --- | --- | --- |
| `g_key_state` **(input)** | 0x007fdda0 | 260 | 256 | 4: `g_left_ctrl`+0x1d, `g_left_shift`+0x2a, `g_right_shift`+0x36, `g_right_ctrl`+0x9d |
| `g_ddraw1`/`g_gpu_state` **(graphics)** | 0x00667d70 | 984 | 984 | 34: `g_ddraw`+4, `g_drvcaps`+8, `g_helcaps`+0x184, `g_primary`/`g_screen_surface`+0x300, `g_surface_74`+0x304, `g_surface_78`+0x308, `g_draw_surface`+0x30c, `g_clipper`+0x310, `g_screen_palette`+0x314, `g_colour_mode`/`g_pixel_fmt`/`g_screen_depth`+0x318, the four font/`g_gdi_obj` slots +0x31c..+0x328, `g_ddsd`/`g_lock`+0x32c, `g_ddsd_height`+0x334, `g_ddsd_width`+0x338, `g_ddsd_pitch`+0x33c, `g_ddsd_bits`+0x350, `g_render_clip`+0x398, `g_target`+0x3a8, `g_video_locked`+0x3d4 |
| `g_cur_save_slot` **(profile)** | 0x0080ffe4 | 220 | 4 | 2: `g_save_type`+1, `g_profile_unlocked`+2 |
| `g_if_icon_pressed` | 0x007fdcc0 | 64 | 36 | 8 theme/cursor icons +4..+0x20 |
| `g_if_icon_normal` | 0x007fdd40 | 48 | 36 | 8 theme/cursor icons +4..+0x20 |
| `g_cafe_chair_mask` | 0x0081cda0 | 64 | 64 | 15 `g_oct_tab_*` +4..+0x3c |
| `g_cafe_sprites` | 0x0081cd60 | 36 | 36 | 8 `g_oct_tent_*`/`g_oct_kiosk` +4..+0x20 |
| `g_copters_path0` | 0x004c1124 | 24 | 24 | 5: paths 1-4 and `g_copters_layers` |
| `g_car_class_vt` | 0x004dd5e0 | 100 | 96 | 2: `g_curve_line_hooks`+0x20, `g_curve_arc_hooks`+0x40 |
| `g_rate_t0` | 0x00832928 | 20 | 20 | 5: `g_mood_low`+4, `g_rate_t1`+8, `g_mood_high`/`g_rate_t2`+0xc, `g_rate_t3`+0x10 |
| `g_spacetower_car_matte` | 0x0062fd64 | 16 | 16 | 3 |
| `g_bloke_age_limits` | 0x004b8334 | 16 | 16 | 2 |
| `g_appraisal_flags`/`g_report_state` | 0x00665ff8 | 160 | 160 | 1: `g_goal`+0x14 |
| `g_save_extra` | 0x00798728 | 12 | 12 | 2 |
| `g_level_done` | 0x0080ffd4 | 15 | 15 | 1: `g_have_profile`+5 |

Three of those are load-bearing right now:

* **`g_key_state`.** `extern unsigned char g_key_state[256]` (input.c:53), and
  the four alias offsets ARE the DIK codes: 0x1d LCONTROL, 0x2a LSHIFT,
  0x36 RSHIFT, 0x9d RCONTROL. `ScanKeyboard` (input.c:107) does one
  `GetDeviceState(kbd, 256, g_key_state)`; split up, that wrote 256 bytes into a
  29-byte object, and `IsLShiftDown`/`IsRShiftDown` (sysstubs.c:181/184,
  `g_left_shift >> 7`) plus `g_left_ctrl`/`g_right_ctrl` (unref6.c:158) read a
  byte of the wrong key, so shifted text entry and movie.c's Ctrl+Q skip were
  dead. This is PORT-B2 finding 2, closed.
* **`g_gpu_state`.** `extern char g_gpu_state[0x3d8]` (util.c:79) is the host
  GPU block gpu.c:10 maps field by field, and `CheckHostSystemGPU`
  (util.c 0x004637c0) clears the WHOLE of it with one `memset` before calling
  `InitHostSystemGPU`. Split up, that memset cleared nothing but the DirectDraw
  pointer: both surfaces, the clipper, the palette, the four GDI fonts, the
  `DDSURFACEDESC` the software renderer locks through (`g_ddsd` +0x32c, with
  `g_ddsd_height`/`width`/`pitch`/`bits` its own fields) and `g_video_locked`
  all kept their stale values across a display-mode change.
* **`g_cur_save_slot`.** gameframe.c:169 reads `CurProfile+0x44` as a dword
  (`g_cur_save_slot_wide`) that spans the save slot, the save type (+1) and the
  first two bytes of the 200-byte unlocked block (+2).

### Before and after, measured

`git show b28e35cb:portable/tools/gen_link.py` run over the same objects emits

```c
__attribute__((aligned(16))) unsigned char g_key_state[29];   /* 0x007fdda0 */
__attribute__((aligned(16))) unsigned char g_left_ctrl[13];
__attribute__((aligned(16))) unsigned char g_left_shift[12];
__attribute__((aligned(16))) unsigned char g_right_shift[103];
__attribute__((aligned(16))) unsigned char g_right_ctrl[103];
```

and a probe linked against that globals.c reports the offsets the game would
have seen:

| symbol | image offset | before | after |
| --- | --- | --- | --- |
| `g_left_ctrl` | +0x1d (29) | +256 | +29 |
| `g_left_shift` | +0x2a (42) | +272 | +42 |
| `g_right_shift` | +0x36 (54) | +749392 | +54 |
| `g_right_ctrl` | +0x9d (157) | +749280 | +157 |
| `g_primary` | +0x300 (768) | +570288 | +768 |
| `g_ddsd_bits` | +0x350 (848) | +32 | +848 |

Note `g_ddsd_bits` at +32: the "direct indexing still works" half of PORT-B2's
finding was true only for `g_key_state`. In the GPU block the fragments were
*close enough to overlap*, so `g_ddsd_bits` aliased another field of the same
lock descriptor — the renderer's `lpSurface` and the surface pitch were two
names for nearby bytes of the wrong object.

### How the interior alias is emitted, and why that form

There is no offset form of `__attribute__((alias))`, and no way in C at all to
place a symbol at an interior offset of another. The assembler has one:

```c
#if defined(__APPLE__)
__asm__(".globl _g_left_shift\n.set _g_left_shift, _g_key_state+42\n");
#else
__asm__(".globl g_left_shift\n.set g_left_shift, g_key_state+42\n");
#endif
```

`portable/README.md`'s PORT-A section records that module-level asm does not
survive the wasm backend — that is true of a `.text` *function* alias, which is
why `fn_alias` became a forwarder. A DATA symbol is different: in the wasm
object format a data symbol is a (segment, offset, size) triple, so an interior
label is exactly representable and `.set` lowers to it. Verified on both
backends before writing any of this (two TUs, the alias read from the other
one): wasm `interior == big+42`, Mach-O likewise.

One trap, which cost a link: **a tentative definition is a COMMON symbol under
`-fcommon`, and the assembler cannot resolve `.set alias, common+off` at all** —
it does not error, the alias silently stays undefined, and the link fails on the
alias name with no hint about why. A zero-filled host block therefore gets an
explicit `= {0}`, which makes it a real `.bss` definition; the object is no
bigger. Only host blocks get it, so nothing else in globals.c changes.

### The rules the pass follows

1. The extent comes from the game's own `extern` declaration: array bounds times
   `elem_size(type)`, which knows the primitive types and "every pointer is 4
   bytes" and **nothing else**. A `Pos`, an `FXEntry[]` or a `RideDef*` table has
   no size until somebody writes the struct, so such a declaration hosts nothing
   and keeps the gap tiling. That is the "where known, else the gap tiling" rule
   of the brief, and it is why the pass finds 15 objects and not 200.
2. The scan stops at any address inside the extent that is NOT an extern-only
   data name — a function, or data a game object defines — and clamps the extent
   there rather than claiming storage someone else owns. No case in this image
   needs the clamp; it is there so the next one cannot go wrong silently.
3. The merged block's size is `max(declared extent, the tiled end of the last
   absorbed address)`, so the total coverage of `.rdata`/`.data` is exactly what
   it was: PORT-A2's pointer resolution sees the same tiling, and the census
   numbers below are unchanged.
4. An absorbed address no longer gets a `symbol_at` entry, so a pointer word
   whose value equals it resolves through `containing()` to `(host, offset)`
   rather than to the alias name. One less symbol the prologue has to declare,
   and it cannot depend on an alias that the `missing_data` filter might not
   emit.

### Census: unchanged, which is the point

`gen/manifest.md`, wasm32 `-DLL_ILP32=ON`, before → after:

```
- globals defined: 2374 -> 2259 (3703048 bytes both times), data aliases: 238
- objects merged from interior-aliased names: 0 -> 15 (100 offset aliases)
- words re-pointed at a symbol address (ilp32): 272 -> 272
- pointer words re-pointed INTO a block (ilp32): 441 -> 441
- synthesised ll_gap_ blocks for unnamed data: 0 -> 0
- pointer words left raw: 12 -> 12
- host API stubs: 95 -> 95
```

115 fewer definitions, the same bytes, the same pointer resolution.

### The proof: `portable/tests/test_keystate.c`

A new `legoland_tests keystate`, 29 checks, registered on BOTH toolchains
(`ll_add_test(keystate ... FALSE)`): every check is a byte offset inside a byte
array, which is the same on LP64 and ILP32, it needs no `gamedata/` and no host
shim — `IsLShiftDown`/`IsRShiftDown` are real recovered game code that touches
nothing but these globals. It is the one test in `portable/tests/` with no
oracle, because its expectation is the original's own layout.

What it checks: the four DIK offsets; that a 256-byte `ScanKeyboard`-shaped
write into `g_key_state` does not reach `g_info_icon_g` at 0x007fdea4 (the first
named address past the array); that each alias reads back the byte written at
its own index, with an `i*7+0x11` pattern so no two indices share a value;
`IsLShiftDown`/`IsRShiftDown` through all four shift combinations; eight offsets
of the GPU block including the four `DDSURFACEDESC` fields; that
`CheckHostSystemGPU`'s `memset(g_gpu_state, 0, 0x3d8)` really clears
`g_ddraw1`, `g_primary`, `g_ddsd_bits` and `g_video_locked`; and the
slot/type/unlocked dword overlay.

Against the pre-fix generator every offset check fails with the numbers in the
table above.

**For the integrator:** this test needed three small append-only edits to
PORT-C's files (`portable/tests/ll_tests.h` one prototype,
`portable/tests/ll_tests.c` one table row, `portable/cmake/tests.cmake` one
source and one `ll_add_test`). The brief for this lane authorises adding a test
file under `portable/tests/`; PORT-C is closed.
