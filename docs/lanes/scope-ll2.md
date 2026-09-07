# Scope LL2 — logflume drop / track tick cluster

Branch `scope/LL2`. File `LEGOLAND/logflume9.c`. Object prefix `/tmp/sll2_`.
Brief: `docs/SCOPE_LL2_logflume_drop.md`.

## Status

| address | name | insns | % | audit | marker |
| --- | --- | ---: | ---: | --- | --- |
| 0x0040d420 | LFGeom_ApplyCursors | 73 | 100 | [OK] | FUNCTION |
| 0x0040d520 | LFTrack_CommitPlacement | 128 | 100 | [OK] | FUNCTION |
| 0x0040d6f0 | LFPiece_UpdateCommon | 151 | 100 | [OK] | FUNCTION |
| 0x0040d900 | LFPiece_AddCommon | 83 | 100 | [OK] | FUNCTION |
| 0x0040da10 | LFTrack_UnlinkNeighbours | 61 | 100 | [OK] | FUNCTION |
| 0x0040db00 | LFPiece_RemoveCommon | 61 | 100 | [OK] | FUNCTION |

**6 / 6 exact.** Relocs clean. `/W3` clean.
Neighbour-helper names taken from scope LL1. 0x00409a90 / 0x0040a080 still
unmatched in LL1; named here `LFTrack_ReshapeEnds` / `LFTrack_LinkEnds`.

## UpdateCommon people-rect SIB (closed)

151i/520B exact. People-rect after `pop edi / pop esi` is
`mov eax,[ox] / mov ecx,[v0] / lea edx,[eax+ecx]` (`8D 14 08`).

Closing hybrid: keep the unsigned-`left` / two-def `o.y` lea, but assign
v0 through a cdecl RTL helper so the **ox write is the second argument**
(evaluated first) and **o.y takes the return** (first argument / v0):

```
o.y = LFUpd_Fst(g_edit_cursor.footprint.v[0], o.x = g_mapref.x);
left = (unsigned)o.x + (unsigned)o.y;
o.y = g_mapref.y;
r.left = (int)left;
```

`LFUpd_Fst` is `return a`. RTL loads `o.x = g_mapref.x` before v0
(`a1` then `8B 0D`); returning v0 into `o.y` is the last addend def
that selects base=eax. A plain `o.x=ox; o.y=v0` pair is the other
150/151 attractor (`8D 14 01`). `o.y=v0; o.x=ox` is the F/N attractor
(correct SIB, v0-first moffs). Hoisted `ox=g_mapref.x` then F/N
rematerialises the ox moffs after v0. Pointer-typed ox, commute,
volatile, comma, and `left=ox+o.y` stay on one attractor or drop the
lea. Track's mem+mem `[eax+ecx]` needs saved ebx/esi/edi; here only
three scratches remain after the pops.

2026-09-08 earlier floors (still true of spellings that drop the two-def
`o.y` / unsigned `left` pair):

- **Index 121:** `Pos o` + `r.left = o.x + v[0]` + volatile v1. v0 in ecx;
  dest-coalesces (`add ecx,eax`) and stores immediately. 519/520B.
- **Index 120:** named sum / delayed store without the `o.y` two-def.
  y-before-store but v0 in edx (`add edx,eax`).

`LFTrack_Update` emits `lea edx,[eax+ecx]` from four plain assigns because
ebx/esi/edi are still saved; the same four assigns here dest-coalesce or
take index 120. The original people block sits after `pop edi / pop esi`,
so the 3-scratch lea is real — Track's extra pushes are not a missing
Common `push ebx`.

Also ruled out this pass: silent post-add v0 (DCE to index 120),
observable post-add v0 (extra store), zero-table index (extra load or
DCE), `*(int*)&ox + v0` (index 121 or 120), cost/people as dest (120),
RTL `FillR` / second live sum (esi, moved pops). `__asm` not used.

Levers that landed the rest: union `{packed, nb}` in the fp slot; `mode=0`
after ScreenToMapRef so packed cannot colour onto mode; separate
`FirstRun` / `KeepRun` (nested call splits leftover); `if (count==0)` so
the short fail is inline; `if (probe!=0)` so success is inline; people
`if (n != -1) { if (n==1) err 3; } else err 4`.

## Names

- `LFTrack_UnlinkNeighbours` / `LFTrack_CommitPlacement` / `LFPiece_UpdateCommon`
  / `LFPiece_AddCommon` / `LFPiece_RemoveCommon` already declared in logflume.c.
- `sub_40d420` → `LFGeom_ApplyCursors`: stamps LFGeom connection points onto
  the two preview cursors at 0x004c4468 / 0x004c5ca0.

## Globals first named here

- `g_lf_geom_cursor_a` 0x004c4468 / `g_lf_geom_cursor_b` 0x004c5ca0
- `g_lf_place_cursor_a` 0x004ca5b0 / `g_lf_place_cursor_b` 0x004c1260
- `g_lf_commit_a` 0x004cbde0 / `g_lf_commit_b` 0x004c2a90 (zeroed by CommitPlacement)
