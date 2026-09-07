# Codex-F — in progress (2026-09-07)

Branch: `codex/scope-f`, based on `4671f8fd` (`main`).
Worktree: `.worktrees/codex-f`.
Owned files: `LEGOLAND/uistubs2.c`, `LEGOLAND/coaster10.c`,
`LEGOLAND/ridemachine2.c`, this note.
Object prefix: `/tmp/cf_`.

## Status

- `uistubs2.c`: **5/5 exact**, audit PASS, relocs clean, `/W3` clean.
- `coaster10.c`: not started.
- `ridemachine2.c`: not started.

## `uistubs2.c` — 5 exact, 26 instructions

All rows: **100%, audit `[OK]`, committed marker `// FUNCTION:`**.

| Address | Function | Insns | Notes |
| --- | --- | ---: | --- |
| `0x0046d390` | `SetHelpFaceState5` | 2 | twin of `SetHelpFaceTalking` (stores 4) |
| `0x00492da0` | `RestartMusic` | 5 | `SetTheme(g_imt_theme)` |
| `0x00457870` | `SetBrickLimit` | 6 | former `sub_457870` |
| `0x00489ee0` | `ClearMarkedTiles` | 6 | former `sub_489ee0` |
| `0x00499410` | `ResetGameClock` | 7 | freezes main + aux clock bases |

### Naming

- **0x00457870 → `SetBrickLimit`**: stores `(limited == 0)` at `g_brick_lock`
  (0x004b90fc). Matches `BricksAreLimited` (`tinystubs.c`): lock==0 means
  limited. Free-play passes 0 (unlimited bricks).
- **0x00489ee0 → `ClearMarkedTiles`**: writes u16 `0xffff` into each
  `g_marked_tiles[i].key` (0x007cb3e0, 128 entries, stride 4). Same sentinel
  `MarkObjectTiles` (`pathmisc.c`) treats as empty. Count half-words left
  untouched.

### Levers

- **Signed end-pointer compare for `ClearMarkedTiles`.** A typed
  `MarkedTile*` walk emits `jb`; the original latch is `jl`. Casting the
  cursor and end to `int` and walking with `p += 4` / `*(unsigned short*)p`
  recovers the signed compare. Evidence: matchfull first residual was
  `jl` vs `jb` at index 5; after the int walk, audit `[OK]`.

### Mechanics recovered

- Help-face state 5 is the talking/report twin of state 4.
- Music "restart" re-dispatches the current IMT theme through `SetTheme`
  (0x00492ce0); it does not touch the streaming track directly.
- Brick lock is an inverted flag relative to the caller's "limited" argument.
- Marked-tile clear only invalidates keys; residual counts survive.
- `ResetGameClock` snapshots `GetTicks()` into held/base and copies
  `g_detail` (0x008119a4) into the auxiliary held/base pair.

## Next

`coaster10.c` smallest-first, then the three `CoasterModel_DrawPass*` as one
family; then `ridemachine2.c` smallest-first.
