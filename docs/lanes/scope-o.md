# Scope O — the movie-player tier (`pathmask2.c`, `movie3.c`, `movie2.c`)

Branch `scope/O`, object prefix `/tmp/so_`, baseline `main` at `3fa59569`
(2026-09-05). Brief: `docs/SCOPE_O_movie_tier.md`. Gate per file:
`audit.py` ends `PASS` with every function `[OK]`, `/W3 /O2 /Gy /Gd` clean,
`relocs.py` zero mismatches and zero unresolved on every function.

## `LEGOLAND/pathmask2.c` — 3 of 3 exact

| address | name | insns | bytes | audit | relocs | marker committed |
| --- | --- | --- | --- | --- | --- | --- |
| 0x00441830 | `RiderCursorSeek` | 16 | 50 | [OK] | 3/3 matched | `// FUNCTION: LEGOLAND 0x00441830` |
| 0x004829c0 | `MarkPathSquareReachable` | 52 | 128 | [OK] | 6/6 matched | `// FUNCTION: LEGOLAND 0x004829c0` |
| 0x00460f50 | `DrawCursorPathTile` | 81 | 201 | [OK] | 14/14 matched | `// FUNCTION: LEGOLAND 0x00460f50` |

`RiderCursorSeek` and `DrawCursorPathTile` were exact on the first compile:
the first from LEVERS RC01 (the `lea edx,[eax+0xc]` before the word compare
is the intrinsic two-byte `memcmp`), the second by copying render5.c's exact
`DrawPathTileOverlay` and routing the three corner draws through the
five-argument `PrintSprite` (the base tile still goes through `PrintSpriteAt`).

### Names given for the first time

| address | name | what it is |
| --- | --- | --- |
| 0x004819a0 | `CollectPathSquareNeighboursCounted` | pathsq.c's `CollectPathSquareNeighbours` (0x00481810) twin: fills `g_path_square_neighbours` from the squares touching a rectangle and leaves the count in 0x00669254 (the 0x00481810 form terminates the table with 0 instead) |
| 0x00669254 | `g_path_square_neighbour_count` | that count |

### Mechanics recovered

- The entrance flood fill (`MarkPathSquareReachable`) snapshots the global
  neighbour table into a `malloc`'d copy before recursing, because the
  recursion overwrites the table; the flag it sets is bit 1 of the square's
  +0x20 word, the bit `ResolveEntrancePathSquare` (pathmask.c) clears on
  every square first. No neighbours, or a failed allocation, ends the walk
  silently.
- `RiderCursorSeek`'s `item` argument is dead (texture.c already said so);
  the match is a two-byte compare of the node's +0x0c key against the
  caller's key.

### Levers, with evidence

- **A global read at every use, with a separate local copy taken BEFORE the
  guard, is how the original gets `mov eax,[g] / test eax,eax / mov ebp,eax /
  je / lea esi,[eax*4]`** — one CSE'd load feeding the test and the size, plus
  an unconditional copy for the loop bound (needed because a recursive call
  overwrites the global). `MarkPathSquareReachable` (0x004829c0, 52i/128B):
  `n = g; if (n) { malloc(n*4); memcpy(.., n*4); for (i<n) }` fuses the webs
  (`mov ebp,[g] / test ebp,ebp / lea esi,[ebp*4]`) and — as a side effect —
  loses edi's prologue push: the flag RMW takes esi and the memcpy is
  bracketed by a local `push edi / pop edi`; 40 of 51 at the original's byte
  length, and the early-return, explicit-`size`, and pointer-walk loop
  spellings are the identical body. `n = g` INSIDE `if (g)` with
  `size = g * 4`: 51 of 52 (the copy lands after the `lea`). `n = g` BEFORE
  `if (g)` with `size = g * 4`: 52/52. `n = g` before `if (n)`: back to 40 of
  51. The RMW's spelling (`|=`, a named temporary before or after the OR) is
  inert in all of these.
- **Reuses**: RC01's intrinsic `memcmp(&p->key, inst, 2) == 0` for a word
  key compare preceded by `lea` (16/16 first try); render5.c's
  `DrawPathTileOverlay` shape (char edge mask in the dead arg0 slot, explicit
  `& 0xff`, reverse-order `dec/je` switch chain) transfers unchanged to its
  five-argument twin (81/81 first try).
