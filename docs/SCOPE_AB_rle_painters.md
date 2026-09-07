# Scope AB — SoftBlitRLEPlain specialised painters (2026-09-07)

> **Status: DONE (2026-09-07).** Branch `scope/AB`. 8/8 exact.
> Notes: `docs/lanes/scope-ab.md`. Object prefix `/tmp/sab_`. Cut from
> inventory groups 18–19 (`0x00466d80..0x00467f00`).

**Read `docs/PARALLEL_CONTRACT.md` first; it carries everything not written
here** — including the relocation step of the gate
(`$PY tools/relocs.py LEGOLAND/<file>.c`, zero `MISMATCH` lines).

NEW-FUNCTION scope, **8 functions, ≈1,578 instructions**, one new file.
Caller `SoftBlitRLEPlain` (0x00466770) is already exact in `softblit2.c` and
already declares all eight painters with provisional names — **keep those
names** unless the body proves a clearer one (record renames in notes).

## What this tier is

`softblit2.c` documents the dispatch: eight specialised RLE painters chosen
by clip geometry × mouse hit-test. Read the `PAINT_RLE_FRAME` macro and the
extern block at ~line 709 before writing a body. All are cdecl.

- Four **Hit** painters take four extra args (`left`, `w`, spare `0`, mouse
  pixel) because they run the cursor hit test.
- Four **non-Hit** painters run when the cursor is outside the sprite box;
  the unclipped member (`RLEPaintFast`) does not need `left`/`w`.

Expect one family shape with per-clip variants — disassemble all eight,
diff, build the unclipped / single-edge members first, then transfer.

**Out of scope:** `SoftBlitRLE` (0x00468040) and the SoftPrint painter at
0x00468410 — leave for a later brief. Do not edit `softblit2.c`.

## `LEGOLAND/rlepaint.c` — the eight SoftBlitRLEPlain painters

| address | name (from softblit2.c) | insns | notes |
| --- | --- | ---: | --- |
| 0x00466d80 | `RLEPaintHitClipLR` | 325 | hit + left+right clip |
| 0x00467180 | `RLEPaintHitClipL` | 202 | hit + left clip |
| 0x004673f0 | `RLEPaintHitClipR` | 197 | hit + right clip |
| 0x00467640 | `RLEPaintHit` | 122 | hit, unclipped |
| 0x004677b0 | `RLEPaintClipLR` | 271 | no hit, both edges |
| 0x00467b00 | `RLEPaintClipL` | 180 | no hit, left |
| 0x00467d10 | `RLEPaintClipR` | 167 | no hit, right |
| 0x00467f00 | `RLEPaintFast` | 114 | no hit, unclipped; fewer params |

**Order:** `RLEPaintFast` → single-edge non-hit → `RLEPaintClipLR` →
`RLEPaintHit` → single-edge hit → `RLEPaintHitClipLR` last.

Reuse `RleFrame`, `LLSRec`, `WinRect`, `Pos`, and `g_mouse_point` layouts
from `softblit2.c` — declare matching types locally or via the same field
offsets; do not change the caller's declarations.

## Owned elsewhere — do not create or edit

`softblit2.c`, `render3.c`, `bigrender.c`, scopes F/G/H, V, AA, AC,
Codex-F's files, every existing `.c`. Declare only what you call.
