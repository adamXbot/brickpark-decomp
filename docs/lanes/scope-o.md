# Scope O — the movie-player tier (`pathmask2.c`, `movie3.c`, `movie2.c`)

**Result: 21 of 21 exact — all 19 functions of the brief plus two
undeclared siblings (`SetHintTextPrefix` 0x00490770, `SetMovieVolume`
0x004771e0), ≈1,290 instructions, in three new files.** Every function
prints `[OK]`, all three files are `/W3` clean, and `relocs.py` matches
every resolvable relocation (349 of 350; the one unresolved position is
the 1000.0f literal in `MovieTicks`, a floating-point constant the tool
cannot bind). Two identity errors the instruction gate passed were caught
by the relocation gate and fixed (`StartMovieAudio`'s dwSampleSize field,
`PrimeMovieAudio`'s swapped stream calls).

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

## `LEGOLAND/movie3.c` — 10 of 10 exact (+1 undeclared twin)

| address | name | insns | bytes | audit | relocs | marker committed |
| --- | --- | --- | --- | --- | --- | --- |
| 0x00490740 | `SetHelpTextPrefix` | 16 | 48 | [OK] | 4/4 | `// FUNCTION: LEGOLAND 0x00490740` |
| 0x00490770 | `SetHintTextPrefix` (undeclared twin, added) | 16 | 48 | [OK] | 4/4 | `// FUNCTION: LEGOLAND 0x00490770` |
| 0x00490800 | `LoadHintTextFor` | 21 | 74 | [OK] | 7/7 | `// FUNCTION: LEGOLAND 0x00490800` |
| 0x00478b20 | `EnsureObjectClassLoaded` | 27 | 68 | [OK] | 3/3 | `// FUNCTION: LEGOLAND 0x00478b20` |
| 0x00469900 | `MarkElemAvailable` | 43 | 121 | [OK] | 7/7 | `// FUNCTION: LEGOLAND 0x00469900` |
| 0x004781f0 | `ParseKeywordFile` | 43 | 134 | [OK] | 8/8 | `// FUNCTION: LEGOLAND 0x004781f0` |
| 0x004983a0 | `ReadDecodedNarration` | 46 | 128 | [OK] | 9/9 | `// FUNCTION: LEGOLAND 0x004983a0` |
| 0x004784c0 | `ResetLevelGlobals` | 60 | 265 | [OK] | 40/40 | `// FUNCTION: LEGOLAND 0x004784c0` |
| 0x00490680 | `LoadTextFileLines` | 86 | 184 | [OK] | 7/7 | `// FUNCTION: LEGOLAND 0x00490680` |
| 0x00465850 | `BlitDIBToScreen` | 113 | 331 | [OK] | 9/9 matched, 0 mismatches, 0 unresolved | `// FUNCTION: LEGOLAND 0x00465850` |

`BlitDIBToScreen` closed last, from 35 of 116 to 113/113 in five steps,
each one lever (the note above its marker has the numbers): the surface
toggle written before the DIB reads gave the prologue its esi=row/edi=dib
roles (56 -> 61); an up-counting column loop made the column count a
compiler temporary (61 -> 66); ALL SUBSCRIPTS ON ONE INDEX in the inner
loop (`src[x]`, `d0[2*x]`, `d1[2*x+1]`, ...) — the lockstep-cursor lever —
reproduced the original's mixed anchoring (66 -> 100); reading `h` and `w`
BEFORE the toggle's store let the two loads precede it (100 -> 105); and an
explicit countdown `cnt = y + 1` from the `y = h - 1` that positions the
last DIB row gave the `inc` from `h - 1` and the memory countdown
(-> 113/113). Inert or worse along the way: four row-loop forms, four
store orders in subscript and `*d++` forms, int/unsigned pixel temps.

### Names given for the first time (movie3.c)

| address | name | what it is |
| --- | --- | --- |
| 0x00478280 | `ParseKeywordSections` | the section parser ParseKeywordFile hands the opened file to |
| 0x00469ab0 | `MarkElemUnavailable` | clears element flags 2 and 0x10000 (if flag 1) and dirties the menu |
| 0x0048a6e0 | `UnlockFreePlayEntry` | finds the element's `g_fp_table` entry by `_stricmp` name and unlocks it |
| 0x00471c10 | `AddNewObjectMarker` | loads `NewObjIcons\\<class>.bmp` and appends a new-object marker (cap 20) |
| 0x00498230 | `NarrationBytesReady` | `(g_narr_e - g_narr_d) & 0x1ffff` |
| 0x00498210 | `NarrationContiguous` | bytes readable from the read cursor without wrapping |
| 0x00498250 | `RefillNarrationRing` | the decoder top-up |
| 0x004689a0 | `FreeScriptStrings` | frees `g_script_strings[0..count)` |
| 0x004689f0 | `NewScriptEvent` | three-argument allocator; `ResetLevelGlobals` stores `NewScriptEvent(0,0,0)` in 0x007fdca4 (`g_script_root`) |
| 0x004441f0 | `ClearReportState` | `g_report_state[0] = 0` |
| 0x0044db20 | `ClearSim832b9c` | zeroes 0x00832b9c (meaning unknown; 0x0044db30 reads it back) |
| 0x0044db80 | `ClearAppraisalState` | zeroes 0x00832978 and `g_instant_appraisal` |
| 0x0044dc70 | `SetLevelGoalState` | `0x0083297c = a; ClearSim832b9c(); 0x004597e0(0, b)` |
| 0x00468830 | `ClearScriptTexts` | zero byte 0 of both script text buffers |
| 0x00482d70 | `ResetMoodAdjustments` | the `g_mood_adjustments` constant block |
| 0x00462e90 | `ResetSimTuning` | the 0x00832824 constant block |
| 0x00476050 | `ClearIconHelp` | `0x00476030(i, 0)` for i < 9 (ClearMenuHelp's icon twin) |
| 0x00463560 | `ResetDamageClock` | 0x00667d54 = GetGameTimer(), `g_dmg_clock` = GetSimClock() |
| 0x00482b10 | `ResetEntranceTileTime` | `g_entrance_tile_time` = GetGameTimer() |
| 0x006681ec | `g_last_blit_bits` | the surface the last movie frame went to, toggled to 0 when it repeats |
| 0x00668fd0 | `g_keyword_file_name` | 0x80-byte copy of the keyword file's name |
| 0x007aaca0 | `g_narr_ring` | the 0x20000-byte decoded-PCM ring (`g_narr_d` is its read cursor) |
| 0x00669054, 0x004bb5ac, 0x00669050, 0x00669098, 0x00832924, 0x00832920, 0x007fdca4, 0x004bb5b0 | `g_level_number`, `g_level_db_flag_ac`, `g_level_byte_669050`, `g_level_int_669098`, `g_visitor_cap_extra`, `g_visitor_cap`, `g_script_root`, `g_level_db_active` | per-level state ResetLevelGlobals writes; named from what it does to them, meanings unconfirmed |

Extern-type divergence: `LoadObjectClass` is defined in saveprof.c as
`void* LoadObjectClass(ClassElem*)`; declared `int LoadObjectClass(DBElem*)`
here (only the test matters). `legoland.h`'s `Elem` is a different record from
the level-database element; the local type is `DBElem`.

### Mechanics recovered (movie3.c)

- The movie blit doubles a 16-bit DIB two-by-two onto the locked surface,
  bottom-up (the last DIB row is the top screen row), converting RGB555 to
  RGB565 when `g_screen_depth == 2`, and clears `gap/2` rows above and the
  rest below, each cleared row 640 pixels wide regardless of the surface.
  With a zero-height DIB the bottom fill starts from an uninitialised row
  pointer (original behaviour).
- `LoadTextFileLines`: whole RES file into a `size + 2` heap buffer, lines
  cut at '\r' (NUL written over it, the '\n' skipped); returns the line
  count; a missing/empty file or failed allocation returns 0.
- `LoadHintTextFor` is `LoadHelpTextFor` for the hint table
  ("Intervals\\<key>", cap 0x20) without the header-line drop.
- `ParseKeywordFile`: "Scripts\\<name>" opened through RES, the name copied
  into a 0x80 global, the file handed to the section parser, closed;
  returns the parser's result or 0 if the file did not open.
- Level-database element flags at +0x08: bit 0 exists, bit 1 available in
  the build menu, bit 2 object class loaded, bit 16 taken away.
  `EnsureObjectClassLoaded` loads the class on first use and marks the
  element unavailable; `MarkElemAvailable(e, marker, unused)` grants it
  unless already granted and not taken away (only logged), unlocks the
  free-play entry and, with `marker`, adds a new-object icon; the menu is
  dirtied on every path, including a null element.
- `ReadDecodedNarration(dst, len)`: clamps to what the decoder has after one
  refill, copies in contiguous runs, advances the read cursor modulo
  0x20000, refills again, returns the count copied.
- `ResetLevelGlobals`: the full store/call list is the body; `lpConfig`'s
  +0x30/+0x34/+0x38 are set 0/1/1 and +0x1a to 200 (copied to 0x00832920).
- ORIGINAL BUG: the "Not giving %d.. Already got it" trace formats the
  element's name pointer with %d.

### Levers, with evidence (movie3.c)

- **One textual `return K` per constant.** `EnsureObjectClassLoaded`
  (0x00478b20): `if (flags & 4) return 1; ... return 1;` duplicates the
  `mov eax,1 / pop / ret` block inline (26 of 30); `if (!(flags & 4)) {
  load } return 1;` shares it and is 27/27.
- **The arm that must fall through is the one WITHOUT the exile-able tail.**
  `MarkElemAvailable` (0x00469900): `if ((f & 0x10002) == 2) log; else
  grant;` puts the log inline (31 of 43); inverting to `if (... != 2) grant;
  else log;` exiles the log past the main path, 43/43.
- **Adjacent store order**: `ResetLevelGlobals`, `g_832924 = 0` written
  before the u16 copy is emitted after it (59 of 60); writing the copy
  first is 60/60.
- **A byte temp must be a compiler temporary to stay out of the scratch
  pool.** `LoadTextFileLines` (0x00490680): `do { c = buf[i++]; } while (c
  != '\r' && i < size);` names the byte, VC6 gives it ecx, `lines` moves to
  edx and `max` is pushed into ebx, which frees edi for the file handle:
  62 of 86 with every register role shifted. `while (buf[i++] != '\r' && i
  < size) ;` peels the first test (60 of 88). `do ; while (buf[i++] != '\r'
  && i < size);` keeps the latch-only test and a compiler temp: the temp
  lands in ebx (the file handle's register, restored each iteration as the
  original does), `lines` in ecx, `max` in edx, 86/86. The outer loop as
  `while (n < max && i < size)` and as `for (n < max) { if (i >= size)
  break; }` are both exact.
- `if (f) { ...; return rc; } return 0;` (ParseKeywordFile) and `if (!f)
  return 0;` on the value just loaded (LoadTextFileLines' bare `ret`) are
  both first-try exact — the choice follows whether the original's fail
  block carries its own `xor eax,eax`.

## `LEGOLAND/movie2.c` — 8 of 8 exact (+1 undeclared setter)

| address | name | insns | bytes | audit | relocs | marker committed |
| --- | --- | --- | --- | --- | --- | --- |
| 0x00492980 | `SetSfxPaused` | 2 | 11 | [OK] | 1/1 | `// FUNCTION: LEGOLAND 0x00492980` |
| 0x00492990 | `ClearSfxPaused` | 2 | 11 | [OK] | 1/1 | `// FUNCTION: LEGOLAND 0x00492990` |
| 0x004771e0 | `SetMovieVolume` (undeclared, added) | 4 | 15 | [OK] | 1/1 | `// FUNCTION: LEGOLAND 0x004771e0` |
| 0x00476680 | `MovieTicks` | 30 | 110 | [OK] | 9/10 (+1 float literal, unresolvable) | `// FUNCTION: LEGOLAND 0x00476680` |
| 0x00476c90 | `StopMovieAudio` | 36 | 131 | [OK] | 15/15 | `// FUNCTION: LEGOLAND 0x00476c90` |
| 0x00476bf0 | `PrimeMovieAudio` | 47 | 148 | [OK] | 15/15 | `// FUNCTION: LEGOLAND 0x00476bf0` |
| 0x00476910 | `StartMovieAudio` | 197 | 731 | [OK] | 71/71 | `// FUNCTION: LEGOLAND 0x00476910` |
| 0x00476d20 | `UpdateMovieAudio` | 344 | 1201 | [OK] | 114/114 | `// FUNCTION: LEGOLAND 0x00476d20` |

### Names given for the first time (movie2.c)

The movie-audio state block 0x00668ee0..0x00668fb4 (`g_mva_*`: acm_used,
hdr [ACMSTREAMHEADER 0x54], bytes_per_block, acm_open_rc, buffer, bits,
blocks, dst_offset, start, acm, pos, sample_size, wf [WAVEFORMATEX],
stream, end, dst_buf, samples_per_block, playing, block, leftover),
`g_movie_timer_mode` (0x00668fac), `g_movie_perf_scale` (0x007fdca8),
`g_movie_volume` (0x004bb4dc, 1.0f); the AVIFile thunks 0x0049e41e
`AVIStreamReadFormat`, 0x0049e424 `AVIStreamLength`, 0x0049e42a
`AVIStreamStart`, 0x0049e430 `AVIStreamRead` and MSACM's 0x0049e3ca
`acmStreamConvert` — all confirmed against the import table with pefile,
not inferred from usage.

Extern-type divergence: `KLIBAUDIO_SetAVIVolume` is defined with one
parameter (audiomisc.c); both callers here push two (buffer, volume).
`StopMovieAudio` is declared `void` in movie.c but returns an int (0 or the
destroy result). `UpdateMovieAudio` uses the dead `prev` parameter's slot as
`AVIStreamRead`'s samples-read out-pointer (`(long*)&prev`); a separate local
grows the frame to 0x14.

### Mechanics recovered (movie2.c)

- Buffer = `g_movie_audio_scale` blocks of `bytesPerSecond / fps` bytes;
  PCM streams read `samplesPerSecond / fps` samples per block straight into
  the locked block; ADPCM (tags 2, 0x11) goes through an ACM stream to 16-bit
  PCM of the same rate/channels (`ACM_STREAMOPENF_NONREALTIME`), source
  reads of 256 samples / a quarter block of bytes, converted with
  `ACM_STREAMCONVERTF_BLOCKALIGN` until a block of output accumulates, the
  surplus carried as `g_mva_leftover`. The decoded buffer is three blocks.
- `UpdateMovieAudio(frame, prev)`: (0,0) primes eleven blocks from position
  0; `frame - prev >= blocks` stops, re-seeks to `spb * frame`, refills
  eleven blocks from block `frame % blocks` and restarts at that block's
  offset with the volume reapplied; otherwise fills `frame - prev` blocks.
  Past the stream's end a block is silence (0x80 for 8-bit, else 0) and the
  position still advances by one when ACM is in use.
- `MovieTicks`: mode 0 -> probe QueryPerformanceFrequency once; mode 2 is
  `(int)((float)counter * (1000 / frequency))`, anything else GetTickCount.
- `StartMovieAudio` stores the stream's `dwSampleSize` (+0x30), NOT its
  dwLength — the relocation check caught the reconstruction error.

### Levers, with evidence (movie2.c)

- **A commutative sum of two calls is evaluated later-declared-callee
  first.** `PrimeMovieAudio` (0x00476bf0): `end = AVIStreamLength(a) +
  AVIStreamStart(a)` AND `AVIStreamStart(a) + AVIStreamLength(a)` both call
  Start first when Length is declared before Start; declaring Start first
  makes both call Length first. The instruction gate is blind to this
  (47/47 either way); `relocs.py` reports the swapped callee targets.
  Splitting the sum into two statements fixes the order but swaps the
  edi/esi roles of `start`/`end` (42 of 47); a named temporary is worse
  (34). So: DECLARATION ORDER of externs is a lever in this one situation.
- **`switch` on a global keeps a stored constant register-backed through the
  join.** `MovieTicks` (0x00476680): `if (g != 2) return GetTickCount();`
  after the two arms store 1 and 2 is jump-threaded (immediate stores, 20 of
  30), whether the test is on the global or on a local copy; `switch (g) {
  case 2: ...; default: ... }` keeps `mov eax,1 / mov [g],eax` + `sub eax,2
  / je` and is 30/30.
- **The guarded body with `return 0` falling through** exiles the early
  return: `StopMovieAudio` `if (!playing) return 0;` inline (33 of 36) ->
  `if (playing) { ...; return Destroy(); } return 0;` 36/36.
- **A local copy of a global read once is what keeps it in a register
  across the call that follows**: `StartMovieAudio`'s `blocks =
  g_movie_audio_scale; g_mva_blocks = blocks; ... Create(fmt, bpb *
  blocks)` gives the original's `mov ebx,[scale] / imul ecx,ebx`; reading
  the global at the multiply reloads it (187 of 197).
- **Frame pinning with one struct**: `struct { long fmtsize; AviStreamInfo
  si; } f;` puts the scalar below the 0x8c aggregate as the original has
  it; as two locals VC6 put the aggregate at the bottom regardless of
  declaration order (196 of 197 by the one displacement).
- **A count that must take a callee-saved register is assigned BEFORE the
  call it does not otherwise cross.** `UpdateMovieAudio` (0x00476d20):
  `count = 11` after the stop call gives count eax and `frame` esi, with
  edi's push sunk to the loop (324 of 344); `count = 11` anywhere before
  the `frame % blocks` division (which needs eax) gives count esi and frame
  edi with both pushes at the guards (342-343); the exact placement is
  `pos = ...; restart = 1; count = 11; block = frame % blocks; restart_pos
  = ...` (344/344), which puts the rematerialised `mov esi,0xb` after the
  restart store where the original has it. The count-as-loop-counter
  (`while (--count)`) forms home it in memory from the start (315).
- The `count == 0` early return as `else if (count == 0) return 1;` gives
  two returns inside the guarded region and pins `push esi` in the prologue;
  `if (count != 0) { loop; restart }` then one `return 1` sinks both pushes
  and lets VC6 thread the constant-11 arms straight to the loop entry as the
  original does.
- The partial-block ACM arm keeps only `need = bpb - leftover` (re-reading
  both globals after the `memcpy`, which VC6 must treat as aliasing) and
  addresses the tail as `lock + bpb - need`; keeping `have` costs a spill
  (264 of 350). The silence fill is two `memset`s under `if (bits == 8)`;
  a ternary fill value emits byte-replication code.
- The silence arm is the THEN of `if (pos >= end)` (falls through) with the
  decode arm exiled; `off += bpb` is the then-arm of `if (leftover != 0)`.
