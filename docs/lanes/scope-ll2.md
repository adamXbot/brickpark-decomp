# Scope LL2 — logflume drop / track tick cluster

Branch `scope/LL2`. File `LEGOLAND/logflume9.c`. Object prefix `/tmp/sll2_`.
Brief: `docs/SCOPE_LL2_logflume_drop.md`.

## Status

| address | name | insns | % | audit | marker |
| --- | --- | ---: | ---: | --- | --- |
| 0x0040d420 | LFGeom_ApplyCursors | 73 | 100 | [OK] | FUNCTION |
| 0x0040d520 | LFTrack_CommitPlacement | 128 |  |  | WIP |
| 0x0040d6f0 | LFPiece_UpdateCommon | 151 |  |  | WIP (ESCAPES) |
| 0x0040d900 | LFPiece_AddCommon | 83 |  |  | WIP (23 mismatch, frame 0x40 vs 0x38) |
| 0x0040da10 | LFTrack_UnlinkNeighbours | 61 | 100 | [OK] | FUNCTION |
| 0x0040db00 | LFPiece_RemoveCommon | 61 | 100 | [OK] | FUNCTION |

**3 / 6 exact.** Relocs clean on the three FUNCTION bodies. Neighbour-helper
names taken from scope LL1. 0x00409a90 / 0x0040a080 still unmatched in LL1;
named here `LFTrack_ReshapeEnds` / `LFTrack_LinkEnds`.

## Names

- `LFTrack_UnlinkNeighbours` / `LFTrack_CommitPlacement` / `LFPiece_UpdateCommon`
  / `LFPiece_AddCommon` / `LFPiece_RemoveCommon` already declared in logflume.c.
- `sub_40d420` → `LFGeom_ApplyCursors`: stamps LFGeom connection points onto
  the two preview cursors at 0x004c4468 / 0x004c5ca0.

## Globals first named here

- `g_lf_geom_cursor_a` 0x004c4468 / `g_lf_geom_cursor_b` 0x004c5ca0
- `g_lf_place_cursor_a` 0x004ca5b0 / `g_lf_place_cursor_b` 0x004c1260
- `g_lf_commit_a` 0x004cbde0 / `g_lf_commit_b` 0x004c2a90 (zeroed by CommitPlacement)
