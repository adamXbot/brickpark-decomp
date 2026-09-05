# Scope O — the movie-player tier: audio stream, DIB blit, keyword files (2026-09-05)

> **Status: DONE — 21 of 21 exact (the 19 below plus two undeclared siblings), merged into `main` 2026-09-05.** Branch `scope/O`. Notes: `docs/lanes/scope-o.md`.
> Object prefix `/tmp/so_`. Any agent. Unrelated to `SCOPE_CODEX_F.md`.

**Read `docs/PARALLEL_CONTRACT.md` first; it carries everything not written
here** — in particular the new relocation step of the gate
(`$PY tools/relocs.py LEGOLAND/<file>.c`, zero `MISMATCH` lines).

NEW-FUNCTION scope, **19 functions, ≈1,260 instructions**: the tier of
callees scope K's `movie.c`, `pathmask.c` and `texture.c` (all merged,
28 of 28 exact) declared and did not define. Everything here is reached from
those three files, which document each call from the caller's side; read
their headers and `docs/lanes/scope-k-*.md` before the disassembly.

## `LEGOLAND/movie2.c` — the movie's audio stream (≈660 insns)

`RunMovie` (movie.c) is the caller and records the frame/tick protocol; the
AVIFile/ACM entry points are declared WITHOUT `__declspec(dllimport)` there
(`call <thunk>`) while GDI calls need it — check which each callee wants from
the original's `call` form. `UpdateMovieAudio` is the largest body in the
frontier; build the two small ones and `StopMovieAudio` first, then
`StartMovieAudio`, then `UpdateMovieAudio` with the others as the template.

| address | name | insns | declared by |
| --- | --- | --- | --- |
| 0x00492980 | `SetSfxPaused` | 2 | movie.c |
| 0x00492990 | `ClearSfxPaused` | 2 | movie.c |
| 0x00476680 | `MovieTicks` | 30 | movie.c |
| 0x00476c90 | `StopMovieAudio` | 36 | movie.c |
| 0x00476bf0 | `PrimeMovieAudio` | 47 | movie.c |
| 0x00476910 | `StartMovieAudio` | 197 | movie.c |
| 0x00476d20 | `UpdateMovieAudio` | 344 | movie.c |

## `LEGOLAND/movie3.c` — DIB blit, text files, level reset (≈455 insns)

`BlitDIBToScreen` is the DirectDraw path `RunMovie` hands each frame to;
`softblit.c`/`screen.c` hold the surface-lock idiom (`g_ddsd`, `g_lock`) and
`text.c` the `NewPrintCent` family `LoadHintTextFor`/`SetHelpTextPrefix`
feed. `LoadTextFileLines`/`ParseKeywordFile` are RES-file readers — `rin.c`
and `data2.c` show the `RES_OpenFile`/`RES_ReadFile` spellings that match.

| address | name | insns | declared by |
| --- | --- | --- | --- |
| 0x00490740 | `SetHelpTextPrefix` | 16 | movie.c |
| 0x00490800 | `LoadHintTextFor` | 21 | movie.c |
| 0x00478b20 | `EnsureObjectClassLoaded` | 27 | movie.c |
| 0x00469900 | `MarkElemAvailable` | 43 | movie.c |
| 0x004781f0 | `ParseKeywordFile` | 43 | movie.c |
| 0x004983a0 | `ReadDecodedNarration` | 46 | movie.c |
| 0x004784c0 | `ResetLevelGlobals` | 60 | movie.c |
| 0x00490680 | `LoadTextFileLines` | 86 | movie.c |
| 0x00465850 | `BlitDIBToScreen` | 113 | movie.c |

## `LEGOLAND/pathmask2.c` — the cursor path tile and the rider seek (≈150 insns)

`pathmask.c`'s `DrawCursorTileAt` and its probe macros are the template for
`DrawCursorPathTile`; `MarkPathSquareReachable` is the write side of
`ClearPathSquareVisited` (tinystubs.c). `RiderCursorSeek` is the shared seek
helper `ObjFirstRider`/`ObjNextRider` (texture.c) tail into — texture.c's
header states its contract (walk forward to the first node whose `u16` at
+0x0c equals `*inst`, park the global cursor there).

| address | name | insns | declared by |
| --- | --- | --- | --- |
| 0x00441830 | `RiderCursorSeek` | 16 | texture.c |
| 0x004829c0 | `MarkPathSquareReachable` | 52 | pathmask.c |
| 0x00460f50 | `DrawCursorPathTile` | 81 | pathmask.c |

**Order:** `pathmask2.c` → `movie3.c` smallest first → `movie2.c` smallest
first. Name nothing new unless the disassembly contradicts the declared name;
record every rename in the notes.

## Owned elsewhere — do not create or edit

`coaster10.c`, `ridemachine2.c`, `uistubs2.c` (Codex-F); every file in
scopes F, G, H; every existing `.c` (movie.c, pathmask.c and texture.c
included — you read them, you do not edit them).
