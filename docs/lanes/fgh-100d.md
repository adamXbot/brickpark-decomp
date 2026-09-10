# FGH 100d — cancelled-pointer copy web on the six WIPs (2026-09-08)

Worktree `.worktrees/fgh-100d`, branch `cursor/fgh-100d` from `fgh-pickup`
`d2a7037a`. Baseline `validation/fgh/check.py --output /tmp/fgh100d_baseline.json`:

```
assigned_exact_gate: 42
assigned_open: 6
existing_normalized_regressions: 0
exact_markers: 410
```

No production body promoted. Objects `/tmp/fgh100d_`. Scratch
`scratchpad/fgh100d/` (score harness + Mesh variants). Pickup's uncommitted
`m5_slot` was recovered from the session `/tmp` tree and reconstructed exactly.

## Mesh (`Coaster3D_BuildTrackMesh` 0x00428cb0) — not closed

Incumbent (volatile union) still best: **6 mism, 151i / 442B, first 27,
ebx = verts, slot not hoisted**, window is store-then-reload (`mov esi,[ebp-14h]`).

### m5_slot (reconstructed, never committed on pickup)

Function-scope `struct { void* elem; Vec3f pos; } cur`, `void* e`,
`struct { int x, y; } tc`:

```c
cur.elem = list[i];
tc.y = (int)slot;
tc.x = (int)cur.elem;
tc.x += tc.y;
tc.x -= tc.y;
e = (void*)tc.x;
```

| metric | value |
| --- | --- |
| instructions / bytes | 151 / 446 |
| index-for-index / aligned | 127 / 38 |
| window | copy-before-store, both hook e-pushes from esi |
| hoist | yes (`mov ebx,[ebp+10h]`) |
| ebx = verts | no (immediate `mov [ebp+14h], verts`) |
| first | 19 |

Matches pickup's "window solved, one hoist blocks it" (they scored aligned
~30 with `h2.py`; we get 38 on the same shape because matchfull alignment
and the missing `jmp` / continue-reload differ by a few opcodes).

### Idea 1 — `out` outranks `slot` (new)

Cancel anchored on a **load through `out`** (`tc.y = out->x`, `out->shade`,
`g_track_verts[i*6].x`, `*(int*)out`) instead of `slot`.

| metric | `tc.y = out->x` (c_outx family) |
| --- | --- |
| instructions / bytes | 151 / 439 |
| index-for-index / aligned | 128 / 45 |
| window | copy-before-store + `push esi` both hooks, slot reloaded from `[ebp+10h]` |
| hoist | **no** |
| ebx = verts | **yes** |
| first | 19 |

The leftover is `mov ecx,[ebx]` in the preheader (LICM of `out->x` against
the constant verts base) immediately overwritten by `mov ecx,[edi+0x4c]`.
Because `out` stays a register IV, the only memory IV is the list cursor and
it lands in `[ebp+14h]` instead of `[ebp+8]`; the `jmp` / `mov ebx,[ebp+14h]`
continue-reload disappear.

Tried on this family and still the same leftover + homes:

- named `int ax = out->x`
- shade `out++` walk / `dest++` walk / `q[j]`
- `*(list + i)`, `g_track_verts[i*6].x`, `out[i].x`
- CSE `TransformVerts(..., g_track_verts + i*6, ...)`
- depth-2 `if (out)` / comma / recomputed shade dest
- `int sl = slot` outside the loop (`tc.y = sl`, hooks still use `slot`) —
  CSE folds `sl` back onto `slot` and hoists
- `tc.y = slot` before the loop
- user-IV `out = g_track_verts; ...; out += 6` plus `out->x` cancel (pickup
  only measured that cursor with a *slot* anchor)
- do/while latch, `out += 6` after shade
- hole member on `cur` (`cur.tag = out->x`) — 441B but first 2 (frame)

`tc.y = (int)(list + i)` / `void** ep = list + i` gives the window and no
hoist but puts **list** in ebx (aligned 46–48, first 17).

### Idea 2 / 3

Something reached through `cur` (`*(int*)cur.elem`, `cur.pos.x`, hole +
`o->hooks`) either self-folds, adds a leftover load, or changes `sub esp`
(first 2). Accepting the m5_slot hoist and repairing the preheader
(do/while, `for (++i)`) did not restore the jmp + verts home; aligned 38
is not closer than incumbent 6 in the audit metric.

## Other five

None of them is a "copy instead of propagate" residual, so the cancel lever
was not swept:

| target | mism | why not |
| --- | ---: | --- |
| SpaceTower_Activate | 10 | head scheduling cycle |
| JungleCruise_Tick | 11 | ebx/ebp swap; byte-need already closed |
| TempleSlide_Update | 18 | original violates the corpus rule |
| StepSchoolCar | 72 | scratch rotation |
| Raster_SubmitPoly | 108 | 3-cycle; original violates the rule |

## Gate

Incumbent bodies unchanged. Do not promote. Next Mesh attempt needs a
loop-variant **memory** load that is already live in the element window
(the list cursor at `[ebp+8]`, not `list+i` as its own IV, and not
`out->x` against a constant verts base) so the cancel does not emit the
dead `mov ecx,[ebx]` and does not collapse `out` to a register-only IV.
