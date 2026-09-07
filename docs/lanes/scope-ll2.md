# Scope LL2 — logflume drop / track tick cluster

Branch `scope/LL2`. File `LEGOLAND/logflume9.c`. Object prefix `/tmp/sll2_`.
Brief: `docs/SCOPE_LL2_logflume_drop.md`.

## Status

| address | name | insns | % | audit | marker |
| --- | --- | ---: | ---: | --- | --- |
| 0x0040d420 | LFGeom_ApplyCursors | 73 | 100 | [OK] | FUNCTION |
| 0x0040d520 | LFTrack_CommitPlacement | 128 | 100 | [OK] | FUNCTION |
| 0x0040d6f0 | LFPiece_UpdateCommon | 151 | 98.7 | 3 | WIP |
| 0x0040d900 | LFPiece_AddCommon | 83 | 100 | [OK] | FUNCTION |
| 0x0040da10 | LFTrack_UnlinkNeighbours | 61 | 100 | [OK] | FUNCTION |
| 0x0040db00 | LFPiece_RemoveCommon | 61 | 100 | [OK] | FUNCTION |

**5 / 6 exact.** Relocs clean on the five FUNCTION bodies. `/W3` clean.
Neighbour-helper names taken from scope LL1. 0x00409a90 / 0x0040a080 still
unmatched in LL1; named here `LFTrack_ReshapeEnds` / `LFTrack_LinkEnds`.

## UpdateCommon residual (index 121)

151i/519B vs 151i/520B. First **120** instructions match (v0 now in ecx),
including packed BPos in the dead `fp` argument slot (`[esp+0x64]` /
`[esp+0x4c]`), leftover `add esp,0x28` / `0x18` / `0x14`, inverted count
fails, probe `je` fail, and the people `!= -1` / `== 1` tail.

The only remaining delta is dest-coalesced people-rect **left**:

```
orig: lea edx,[eax+ecx] / mov ecx,[g_mapref.y] / mov [esp],edx
ours: add ecx,eax       / mov [esp],ecx        / mov ecx,[g_mapref.y]
```

One byte short (`add` 2B vs `lea` 3B). The ecx load between lea and the left
store is **oy** (`g_mapref.y` at 0x7fffc8), not v1; v1 is loaded into edx
*after* the store (`add edx,ecx` for top). ox stays in eax through `r.right`.

Two 149/151 floors, neither is lea-with-v0-in-ecx:

- **Index 121 (kept):** `Pos o` + `r.left = o.x + v[0]` + volatile v1 on top.
  v0 stays in ecx; dest-coalesces (`add ecx,eax`) and stores immediately.
- **Index 120:** any named sum / delayed `r.left=left` / sequential helper.
  Correct *schedule* (y load before store) but v0 is loaded into edx
  (`add edx,eax`). Same 3 mism.

`LFTrack_Update` emits the exact `lea edx,[eax+ecx]` sequence from four
plain assigns, but that function still has ebx/esi/edi saved. After this
body's `pop edi / pop esi` only eax/ecx/edx are free; the same four assigns
here either dest-coalesce or take the index-120 coloring.

Tried and inert for the lea (this pass + earlier): named sum as dest
(block/function/`cost`/`people`), v0-first, Pos aggregate, volatile v0/v1/y
and volatile-y-then-store, one-temp, pointer/`&((char*)ox)[v0]` /
`(char*)ox+v0` on the baseline, `register`, switch people, live `def`/`keep`,
Track_Update / Roads operand orders (with and without Pos/volatile),
y-first top, preload named v0, comma `o.y=(left=..., y)`, helper-local Rect,
RTL `FillR` (four live arg temps pull esi and move the pops), sequential
`PeopleRect` helper (ox kept live for right — still index 120), `__inline`
store-after-y / two-arg lea helpers, Pos-sum then call-arg store, cost/people
as the y web. `ebx` is unused in the original people block; GetObjCost
already ran.

Need a spelling that keeps the index-121 v0-in-ecx load *and* the index-120
y-before-store schedule, so dest cannot coalesce with v0 and must be
`lea edx,[eax+ecx]`. v0 has to die after the add (interfere with dest) but
before the y load (so y can reuse ecx). No dummy use found that creates
that window without an extra insn.

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
