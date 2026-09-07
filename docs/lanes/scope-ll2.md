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
orig: lea edx,[eax+ecx] / mov ecx,[y] / mov [esp],edx
ours: add ecx,eax       / mov [esp],ecx / mov ecx,[y]
```

One byte short (`add` 2B vs `lea` 3B). Floor: VC6 coalesces left with dead
v0 into ecx and stores immediately. Naming the sum (helper temps, `people` /
`cost` as dest, function-scope `left`) moves v0 into edx and loads y before
the add — the older index-120 residual, still 3 mismatches. Pos + volatile
v1 is what keeps v0 in ecx without hoisting v1.

Tried and inert for the lea: named sum as dest, v0-first, Pos aggregate,
volatile v0/v1/y, one-temp, pointer/`&((char*)x)[v0]`, `register`, switch
people, live `def`/`keep` through the rect, Track_Update operand order,
helper-local Rect. `ebx` is unused in the original people block; GetObjCost
already ran.

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
