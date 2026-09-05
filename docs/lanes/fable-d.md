# fable-d — parallel session notes (2026-09-05)

Findings from the four files docs/SCOPE_FABLE_D.md assigned to this session, for folding into docs/DECOMP.md at integration. Per-lane originals beside this file carry the evidence.

| file | exact | WIP | notes |
| --- | --- | --- | --- |
| `LEGOLAND/audio5.c` | 4 | 0 | `fable-d-audio5.md` |
| `LEGOLAND/render5.c` | 6 | 0 | `fable-d-render5.md` |
| `LEGOLAND/coaster7.c` | 10 | 0 | `fable-d-coaster7.md` |
| `LEGOLAND/uimisc2.c` | 14 | 0 | `fable-d-uimisc2.md` |

---

# audio5.c

## Lane `fable-d` / `LEGOLAND/audio5.c` — narration and sample sources

**Result: 4 of 4 exact, `audit.py` PASS, `/W3` clean.** Every function landed
byte-exact; three of the four matched on the first draft.

```
### audio5.c
  [OK   ] 0x00496660 ClearSampleSource            ours=  22i/  53B  orig=22i/53B  mismatch=0
  [OK   ] 0x004967b0 RefreshSampleVolumes         ours=  28i/  64B  orig=28i/64B  mismatch=0
  [OK   ] 0x0042fb00 Restaurant2_StartSound       ours=  24i/  83B  orig=24i/83B  mismatch=0
  [OK   ] 0x00498420 ReadNarrationWaveHeader      ours= 187i/ 522B  orig=187i/522B  mismatch=0

PASS: 0 function(s) failed the extent gate
```

| address | name | insns | pct | audit | promotable | marker committed |
| --- | --- | --- | --- | --- | --- | --- |
| 0x00496660 | `ClearSampleSource` | 22 | 100% | `[OK]` | yes | `// FUNCTION: LEGOLAND 0x00496660` |
| 0x004967b0 | `RefreshSampleVolumes` | 28 | 100% | `[OK]` | yes | `// FUNCTION: LEGOLAND 0x004967b0` |
| 0x0042fb00 | `Restaurant2_StartSound` | 24 | 100% | `[OK]` | yes | `// FUNCTION: LEGOLAND 0x0042fb00` |
| 0x00498420 | `ReadNarrationWaveHeader` | 187 | 100% | `[OK]` | yes | `// FUNCTION: LEGOLAND 0x00498420` |

No function was renamed; all four keep the names their callers' externs use.
All four end in a real `ret`; none is recursive; none hits the extent-walker's
rotated-loop defect.

---

## Levers, with evidence

- **A trailing `return 0` block at the END of a function is what makes VC6
  retarget every earlier guard's failure branch to it.** `ReadNarrationWaveHeader`
  has nine leading `if (_read(...) != 4) return 0;` guards. Written as
  `while (_read(&tag,4) == 4) { ... } return 0;` — the same CFG the original
  has — VC6 inverts all nine guards to `jne <the trailing block>` and the body
  comes out **151 instructions / 489 B** against the original's 187 / 522.
  Written as `for (;;) { if (_read(&tag,4) != 4) return 0; ... }`, with the
  loop's exit return living *inside* the loop so no trailing block exists, each
  guard keeps its own inline `xor eax,eax / pop esi / add esp,8 / ret` — the
  original's **eleven** identical copies — and the body is 187/187. VC6 rotates
  the `for (;;)` itself into exactly the `while` shape (peeled read at
  0x0049854d, tag test as the loop header at 0x00498567, latch `je` back to the
  header at 0x004985d5), so the two spellings differ *only* in whether that
  merge target exists. This sharpens the recorded "Nested ifs, success early,
  failures to ONE trailing `return 0`" entry from the other side: the trailing
  block is not free — it collects nine guards you may not want collected.
- **Which identical `return 0` block VC6 merges into is a `goto` decision, and
  it is worth four bytes.** With the two chunk-walk read failures written as two
  separate `return 0;` statements the body is instruction-for-instruction exact
  at 187/187 but **526 B, not 522**: VC6 merges the skip arm's copy BACKWARDS
  into the *first* guard's block 323 bytes earlier and needs a six-byte rel32
  `jne`, where the original emits a two-byte `jne` **forward** to the data arm's
  copy 121 bytes away. Routing both failures through one `goto chunkfail;` at
  the end of the loop body pins the target and closes the last four bytes. The
  strict/register-blind/offset-blind triage says nothing here — the residual was
  visible only in the byte length, which is why the gate checks it.
- **An if/else whose two arms both begin with a call sharing a constant
  argument gets that push HEAD-MERGED into the test block.** The chunk walk's
  loop header is `mov eax,[esp+8] / push 4 / cmp eax,'data' / je <data arm>`:
  the `push 4` is the first (right-most) argument of `_read(fd, X, 4)` in
  *both* arms, and VC6 hoists it above the branch. It falls out for free once
  the arms are written as a plain `if/else` — no source construct forces it —
  but seeing a lone `push` between a load and its `cmp` is the tell that the
  two successors start with the same call.
- **`if (a < K) g = f(K); else g = f(a);` — two textual calls — is what gives
  `push K / jmp / push eax / call`; the ternary `f(a < K ? K : a)` gives
  `mov eax,K / push eax / call`.** VC6 cross-jumps the two calls from the `call`
  instruction onward and leaves the two argument pushes in their own blocks.
  The ternary form also **merged the malloc's `add esp,4` into the following
  read's cleanup** (`add esp,0x10` where the original has `add esp,4` then
  `add esp,0xc`, shifting every `[esp+N]` after it); splitting the arms
  restored both. Two levers, one edit, worth 5 instructions on
  `ReadNarrationWaveHeader`.
- **A `goto` backwards over an `if` is normalised to the same loop VC6 builds
  from a `while`.** Rewriting the chunk walk as
  `label: if (tag != 'data') { ...; goto label; }` produced byte-for-byte the
  same rotated loop (and the same 151-instruction merge) as the `while`. Not a
  lever — recorded so it is not re-tried.
- **A packed `{u8,u8}` map square passed BY VALUE is a `BPosW` union, not the
  `unsigned short` its caller declares.** `Restaurant2_StartSound`'s
  `mov eax,[esp+0x14] / mov ecx,[esp+0x15] / and eax,0xff / and ecx,0xff` — a
  dword read plus a byte-offset read of the *same* slot — is the by-value union
  shape, exact first try. `ridecb3.c`'s `unsigned short` extern is a caller-side
  lever and was left alone.

## Mechanics recovered — the narration wave-header format (runtime spec)

`ReadNarrationWaveHeader` (0x00498420) parses the speech `.wav` opened by
`audio4.c`'s `PlayNarrationFile` and leaves the descriptor for
`RewindNarrationSource` (0x00498120). It seeks to offset 0 and reads, with
`_read` (0x0049f4ca) only — never `_lseek` past anything:

1. a four-byte tag, which **must** be `'RIFF'`;
2. a `u32` RIFF size, **read into the same local the chunk sizes use and then
   never looked at**;
3. a four-byte form type, which **must** be `'WAVE'`;
4. a four-byte chunk id which is **read and NEVER COMPARED** — whatever sits in
   that position is taken to be the `'fmt '` chunk;
5. that chunk's `u32` size, then `malloc(max(size, 18))` into
   `g_speech_source_format` and a read of exactly `size` bytes into it. If the
   chunk was **18 bytes or shorter** the `u16` at +0x10 (`cbSize`) is forced to
   0 — a 16-byte PCM `fmt ` chunk carries no `cbSize`, so the pad-to-18 plus
   this one store is what turns the raw chunk into a valid `WAVEFORMATEX` for
   `acmStreamOpen`;
6. a chunk walk: read a four-byte id; if it is not `'data'`, read its `u32`
   size, `malloc` the whole payload, read it, and free it — **the unwanted
   chunk is copied through the heap, not seeked over**; loop. When the id is
   `'data'`, its `u32` size goes to `g_speech_data_size` and `_tell` gives
   `g_speech_data_start`, and the function returns 1.

Every other outcome returns 0: a short read at any of the eleven read sites, a
first tag that is not `'RIFF'`, a form type that is not `'WAVE'`. A missing
`data` chunk is not detected directly — the walk reads off the end of the file
and fails on the short read. Odd-length chunks are **not** RIFF-padded, so a
file with one would desynchronise the walk.

`RewindNarrationSource` (0x00498120, read for context, not written here) is the
consumer: `_lseek(g_speech_fd, g_speech_data_start, SEEK_SET)` then
`g_speech_data_left = g_speech_data_size`.

## Mechanics recovered — sample sourcing

- **`ClearSampleSource` (0x00496660) resets the VOLUME only.** It is the
  `kind == 0` arm of `sysmisc.c`'s `UpdateSampleSource` and the "no source" arm
  of `audio3.c`'s `PlayInstanceOfSample`; it pushes `g_sfx_master_db`
  (0x007988a0) into the instance's buffer through `SetVolume` (vtable +0x3c)
  and does **not** touch the pan the last positional update wrote. Same
  three-guard prologue as `audio4.c`'s `StopPlayableSample`
  (`g_samples_ready`, the pointer, then `->def`), and the `if (!s) return 0;`
  on the value just loaded into eax gives the bare `ret` at 0x00496674.
- **`RefreshSampleVolumes` (0x004967b0) walks `g_playable_list` and re-sources
  every live instance**, and its `GetStatus` call (vtable +0x24) is a pure
  liveness probe: the `DWORD` it writes into a stack local is never read, and
  only its `HRESULT == 0` gates the `UpdateSampleSource` call. `UpdateSoundVols`
  (audio3.c) calls it immediately after storing the new `g_sfx_master_db`, so
  the master-attenuation change reaches playing instances through the positional
  path rather than through a second `SetVolume`.
- **`Restaurant2_StartSound` (0x0042fb00) starts TWO effects from one source
  record**, `g_rest2_fx[0]` with flags `(0, 1)` and `g_rest2_fx[1]` with
  `(1, 1)`, both sourced at the placement's map square (`SoundSource::kind = 2`).
  `SoundSource::obj` (+0x04) is left **uninitialised** — for kind 2
  `UpdateSampleSource` never reads it. VC6 emits the three stores as
  `pos.x`, `kind`, `pos.y` (adjacent-store reordering); the source order is
  `kind`, `pos.x`, `pos.y`.

## Callees named for the first time

- `g_speech_data_start` — 0x007cacb4, the file offset of the `data` chunk's
  payload. Named from `RewindNarrationSource`.
- `g_speech_data_size` — 0x0079ac04, the `data` chunk's byte count. Adjacent to
  `audio4.c`'s `g_speech_decoded` (0x0079ac08) and `g_speech_source`
  (0x0079ac0c); `RewindNarrationSource` copies it into 0x007cacac
  (`g_speech_data_left`, not written here).
- `_lseek` at 0x004a56c3 and `_tell` at 0x004aacbd confirmed from
  `profiles.c`'s header; `_read` at 0x0049f4ca from `saveprof.c`'s.

## Original bugs reproduced (commented at the site)

- The RIFF size is read into the chunk-size local and discarded.
- The `'fmt '` chunk id is read and never compared.
- Unwanted chunks are `malloc`/`read`/`free`d whole instead of being seeked
  past — a large `LIST`/`INFO` chunk in a speech file costs a full heap round
  trip.
- Both `malloc` results are used unchecked, and the `fmt` block is written to
  at +0x10 before any check.
- `ClearSampleSource` leaves the pan where the last positional update put it.
- `Restaurant2_StartSound` leaves `SoundSource::obj` uninitialised.

## Extern-type divergences (do NOT align)

- **`ClearSampleSource` (0x00496660)** is defined here returning `int`
  (`sysmisc.c`'s declaration; `UpdateSampleSource` does
  `return ClearSampleSource(s);`). `audio3.c` declares it `void`. Both are left
  as they are.
- **`Restaurant2_StartSound` (0x0042fb00)** is defined taking a `BPosW` union
  **by value**; `ridecb3.c` declares `unsigned short square`. ABI-identical
  under `__cdecl`; the union spelling is what produces the callee's
  `mov eax,[esp+0x14] / mov ecx,[esp+0x15]` byte pair.
- **`HeapFree_w` (0x0049e4d0)** is declared `void` here (as in `audio4.c`);
  `audio3.c` declares it `int`.
- **`FXEntry`** is `{ char* name; int pad4; void* sample; }` here — the layout
  `audiomisc.c`, `loaders.c`, `money.c`, `joust2.c` and `lifecycle.c` all use,
  and the one the disassembly needs (`g_rest2_fx[0].sample` is 0x004b6970 =
  0x004b6968 + 8). **`screencb.c` and `screencb6.c` name +0x04 `sample` and
  +0x08 `flags`** for the same table; that is the wrong way round for the
  play sites. Not touched — flagged here.

---

# render5.c

## Lane `fable-d` / `LEGOLAND/render5.c` — cursor tiles, path overlay, text cache

**6 of 6 exact.** `python3 tools/audit.py LEGOLAND/render5.c` → `PASS`,
`/W3` clean. Object prefix `/tmp/fd_render5_`.

| address | name | insns | bytes | audit | marker committed |
| --- | --- | --- | --- | --- | --- |
| 0x0045a3e0 | `TakeRenderNodeInColumn` | 28/28 (100%) | 77/77 | `[OK]` | `// FUNCTION: LEGOLAND 0x0045a3e0` |
| 0x00461020 | `FlushCursorSpriteList` | 29/29 (100%) | 94/94 | `[OK]` | `// FUNCTION: LEGOLAND 0x00461020` |
| 0x00455f70 | `ExpireCachedText` | 29/29 (100%) | 71/71 | `[OK]` | `// FUNCTION: LEGOLAND 0x00455f70` |
| 0x00451e20 | `SaveCertificateBitmap` | 29/29 (100%) | 74/74 | `[OK]` | `// FUNCTION: LEGOLAND 0x00451e20` |
| 0x00460e90 | `DrawPathTileOverlay` | 72/72 (100%) | 181/181 | `[OK]` | `// FUNCTION: LEGOLAND 0x00460e90` |
| 0x004610f0 | `PaintCursorTiles` | 106/106 (100%) | 304/304 | `[OK]` | `// FUNCTION: LEGOLAND 0x004610f0` |

No renames — every name is the one the scope assigned. All six end in a real
`ret`, none is recursive, so all six are promotable and promoted. Nothing is
left `WIP`, so there is no residual triage to report.

## Mechanics recovered

- **The build cursor path.** `render4.c`'s `DrawEditCursor` (0x00461220)
  calls `PaintCursorTiles`, which walks the diamond grid under the cursor
  rectangle and hands every visited map cell to 0x00461080; that routine
  bounds-checks the cell, keeps only path cells whose tile is non-zero, and
  passes the survivor to 0x00460f50 — the four-argument twin of this file's
  `DrawPathTileOverlay`, which paints STRAIGHT to the locked surface.
- **`FlushCursorSpriteList`'s deferred sprite queue is VESTIGIAL — nothing
  ever appends to it.** The queue is a flat 0x10-stride array of
  `{Sprite*, x, y, mode}` at **0x00801420**, an append cursor at
  **0x004b95f0** (initialised in `.data` to 0x00801420) and a count at
  **0x00667d44**; the call replays every record through `PrintSprite` with
  `ctx = 0` and resets both. A scan of the whole `.text` for the three
  4-byte constants finds references only at 0x00461022..0x00461078, i.e.
  inside this one function: the count is only ever READ and ZEROED. It lives
  in zero-filled data, so it is 0 for the life of the process, the loop never
  runs and both resets are no-ops. `renderview.c`'s `RenderView` still calls
  it between `DrawEditCursor` and `PopRenderingStatus`. The producer was
  presumably compiled out of the shipped build; the body is reproduced as it
  stands. **Anyone hunting the "cursor sprite list" producer should stop —
  there is not one in this executable.**
- **The isometric grid, read off `PaintCursorTiles`.** A tile is `w = 2h`
  pixels wide and `h` tall, `h` being the DEFAULT tile sprite's `+0x16` height
  (the same short arithmetic `pathbuild.c`'s `GetTileBounds` does). From the
  cell under `(x, y)`: the cell under `(x - w/2, y + h/2)` is `+1` in map y;
  the cell under `(x + w, y)` is `(+1, -1)`. So the inner loop paints **two**
  cells per full tile width and steps `(map.x + 1, map.y - 1)`; the outer loop
  restarts the row at `(row.x + 1, row.y + 1)` and steps screen y by one tile
  height. The inner bound is `clip->right + (short)(w+1)/2`, i.e. half a
  diamond past the clip, so the half-step column is never clipped away.
- **`PaintCursorTiles` MODIFIES THE CALLER'S `Pos`.** `p->x -= w` is written
  back through the pointer before `PlayfieldToMap`, so the caller's cursor
  origin moves one full tile width left. Reproduced as written and commented
  at the site; it is the caller's contract, not a bug, but it is invisible
  from the caller's declaration.
- **The path layer is base tile + corner filler.** 0x0045ceb0 answers a
  four-neighbour connection mask (bits `0x1`/`0x2` one diagonal pair,
  `0x4`/`0x8` the other) and that mask indexes the sixteen path tiles at
  `g_tile_sprites[code + 0 .. code + 15]`, where `code` is the FIRST DWORD of
  the loaded "path" tile record at 0x00832bf0 and the run base is `code + 3`.
  0x0045d080 then answers a 2-bit corner code — bit 0 when the `0xc` pair is
  complete but the cell diagonally beyond it is not a path, bit 1 for the
  `0x3` pair — and codes 1/2/3 select `g_tile_sprites[code + 19]`,
  `[code + 20]`, `[code + 21]`. Three corner sprites, never more than one
  drawn.
- **The render-node table's occupancy protocol.** `TakeRenderNodeInColumn`
  completes the model `render3.c` sketched: a node in the 0x1000-entry ring at
  0x00807f60 says "when the column scan next reaches column `x`, resume at row
  `y`". The lookup hands the row back in `p->y`, then clears `live` — the
  store order (`p->y` first, `live = 0` second) is visible in the original —
  and a column with no node parked restarts the scan at row 0.
- **The rendered-text cache expires on DETAIL GENERATIONS, not on a clock.**
  `ExpireCachedText(all)` (called once a frame by `render2.c`'s
  `RenderingComplete` with `all = 0`) tests
  `(unsigned)(g_detail - entry->sprite->detail) > 10`, and `sprite2.c`'s
  `NewSprite` stamps `SpriteRec::detail` (+0x0c) with `g_detail - 1` at
  creation. So an entry survives while the global detail counter has not moved
  more than ten steps past the value the text was rasterised at. **The compare
  is UNSIGNED**, so a counter that moves BACKWARDS expires the entire cache on
  the next sweep. 0x00455ee0 removes one entry by index and COMPACTS the
  array, which is why the index is not advanced on the free arm and why the
  count is re-read every iteration.
- **The certificate "print" (runtime-spec material).**
  `SaveCertificateBitmap` is `mapscreen2.c`'s screen-8 saver. It builds the
  CRT timestamp `asctime(localtime(&t))` — with `time(&t)` called first — and
  strips asctime's trailing `"\n"` in place with `s[strlen(s) - 1] = 0`, then
  calls the screen grabber 0x00451740 with **three** arguments:
  `("EGC.bmp", g_cert_message, stamp)`. 0x00451740 opens the file with
  `_open(path, 0x8000, 0x100)`, writes a 14-byte `BITMAPFILEHEADER` and a
  0x28-byte `BITMAPINFOHEADER` (`_write` calls of 0xe and 0x28), blits through
  `BitBlt` with `SRCCOPY` (0xcc0020) from a DC it creates, and formats the
  saved path into the message buffer. Answer is truthy on success;
  `SaveCertificateBitmap` narrows it with `!= 0`.

## Callees named for the first time

| address | name given | contract |
| --- | --- | --- |
| 0x0045ceb0 | `PathEdgeMask(Pos* at)` → `char` | four-neighbour path connection mask |
| 0x0045d080 | `PathCornerMask(char edges, Pos* at)` → `char` | 2-bit corner-filler code |
| 0x00485f00 | `PrintSpriteAt(Sprite*, int x, int y)` | thin wrapper: `PrintSprite(s, x, y, 0, 0)` |
| 0x00461080 | `DrawCursorTileAt(Pos* at, int x, int y, int mode)` | bounds-check + path/tile filter, then 0x00460f50 |
| 0x00460f50 | (twin of `DrawPathTileOverlay`, 4 args) | same body; corner draws go through the five-argument `PrintSprite` so the caller's blit mode is honoured, where 0x00460e90 uses `PrintSpriteAt` (mode 0) |
| 0x00451740 | `SaveScreenshotBmp(const char* path, char* msg, const char* stamp)` → `int` | screen grab to a 24-bpp .BMP, formats the saved path into `msg` |
| 0x00455ee0 | `FreeCachedTextEntry(int i)` | drop text-cache entry `i` and compact |
| 0x0049fbc6 / 0x0049fa66 / 0x0049f990 | `time` / `localtime` / `asctime` | CRT (above 0x0049e000, never targets) |

New globals: `g_cursor_sprites[]` (0x00801420, stride 0x10),
`g_cursor_sprite_put` (0x004b95f0), `g_cursor_sprite_count` (0x00667d44).

## Extern-type divergences (deliberate, do not "align")

- **`PlayfieldToMap` (0x0045a970) is declared `Pos PlayfieldToMap(int, int)`
  here**, where `objmap.c` defines it returning its own 8-byte `Offset`. The
  pair is a map coordinate in this file and is handed straight to
  `GetTileBounds(Pos*, …)`; both are `{int,int}` so the ABI is identical.
- **`g_path_tile` (0x00832bf0) is a `PathTileRec*` whose leading tile code is
  read as a DWORD** (`mov eax,[0x832bf0] / mov ecx,[eax]` + a 32-bit `add`).
  `maprestore.c` and `pathbuild.c` read the same word 16 bits wide,
  `sysmisc3.c` declares `unsigned short*` and `pathsq.c` `int*`. Four
  spellings of one global across the tree; this file needs the dword one.
- **`PathCornerMask`'s first parameter is `char`, not `int`.** That is what
  makes the caller push the raw dword out of the byte local's home slot
  instead of masking first — see the lever below.

## Original bugs reproduced

None found in these six. The `p->x -= w` write-back in `PaintCursorTiles` is
a deliberate side effect on the caller's `Pos`, not a bug, and is commented at
the site. `SaveCertificateBitmap`'s `s[strlen(s) - 1] = 0` would underflow on
an empty string, but `asctime` never returns one.

## Levers, with evidence

- **`i++` written as a STATEMENT before a cursor advance, versus in the `for`
  increment, swaps two ALU ops and moves a store.** `FlushCursorSpriteList`
  as `for (i = 0; i < n; i++) { call; put++; }` emits
  `add eax,0x10 / inc esi / mov [put],eax / cmp esi,ecx`; as
  `while (i < n) { call; i++; put++; }` it emits the original's
  `inc esi / add eax,0x10 / cmp esi,ecx / mov [put],eax` — the store sinks
  BELOW the compare on its own once the two increments are in source order.
  2 of 29 at identical byte length; nothing else about the body changed. The
  recorded "a decrement in the for-increment vs the body reorders two ALU ops"
  rule, measured from the increment side, and with the store placement as a
  second, free consequence.
- **The free-volatile guard-load lever transfers between the two halves of one
  data structure.** `ExpireCachedText`'s
  `for (i = 0; i < *(volatile int*)&g_text_cache_count; )` is worth 3 of 29
  and is free (the original loads the count in both the guard and the latch),
  exactly as `fpui3.c`'s `FindCachedText` records for the same global. Without
  it VC6 (a) hoists the guard's load ABOVE `push esi` — a volatile access
  cannot cross the push's store — and (b) folds the latch's reload into
  `cmp esi,dword ptr [mem]` where the original materialises
  `mov eax,[mem] / cmp esi,eax`. **One volatile fixed both**: the second
  effect is not a separate lever, it is the same barrier forcing the load to
  be its own instruction. When a global is read by two functions that share a
  cache, expect the same lever on both.
- **A `char` local homed in a DEAD PARAMETER's slot is passed on as a RAW
  DWORD, and that is readable off the listing.** `DrawPathTileOverlay` stores
  `PathEdgeMask`'s answer with `mov byte ptr [esp+0x18],al` into the incoming
  `at` slot (once `at` has been root-copied into esi), then loads the WHOLE
  dword back with `mov esi,[esp+0x1c]` and pushes it as the next call's byte
  argument — the callee reads only `dl`, so the stale high bytes of the
  original pointer are harmless. The `& 0xff` needed for the tile index is
  then a SEPARATE `and esi,0xff` after the call. Declaring the local `int`
  gives a dword store and no mask; declaring the callee's parameter `int`
  masks before the push. `char edges;` plus `extern char PathCornerMask(char,
  Pos*)` plus an explicit `(edges & 0xff)` in the index is the only
  combination that reproduces all three instructions. First measurement of the
  dead-parameter-slot rule where the SLOT'S STALE CONTENTS are visible in the
  argument.
- **A three-case `dec/je` chain lays its blocks in REVERSE source order, and
  the LAST case shares the function's epilogue.** `DrawPathTileOverlay`'s
  `switch (corner) { case 1: … case 2: … case 3: … }` emits case 3 inline
  after the chain, then case 2, then case 1 — and case 1's block falls
  through into the `pop/pop/pop/pop/ret` that the `default` arm also jumps to,
  while cases 3 and 2 each get their own tail-duplicated epilogue. Confirms
  the recorded case-order layout rule at a third case count, and adds the
  detail that the case laid LAST is the one that merges with the default exit.
- **An `int` local with the `(short)` cast at its USE site is how a dword
  store and a `movsx word ptr` load coexist on one slot.** `PaintCursorTiles`
  stores `(h+1)>>1` with `mov dword ptr [esp+0xc],eax` in the prologue and
  reads it back with `movsx edx, word ptr [esp+0x10]` inside the loop. A
  `short` local would store `mov word ptr`; an `int` local with no cast gives
  a plain `mov`. The same value kept in a register (`(w+1)>>1`, in esi) shows
  the cast as `movsx edi,si` in the loop preheader. This is `pathbuild.c`'s
  `GetTileBounds` spelling — `short h`, `short w = (short)(h + h)`,
  `(short)((w + 1) >> 1)` at the use — carried across to a second function
  that computes the same two half-steps, and it landed 106/106 first try. **A
  sibling that does the same geometric arithmetic is worth copying spelling
  for spelling before trying anything else.**
- **`mov ax,[mem] / lea ecx,[eax+eax] / movsx ecx,cx / movsx eax,ax` is a
  `short` pair, not an int one.** The doubling is done 32-bit on a register
  whose high half is stale and then narrowed, which only happens when the
  RESULT is a `short`. Read `short w = (short)(h + h);` off that sequence;
  `int w = h * 2;` gives a single `movsx` plus an `add`.
- **A twin found by callee overlap, not by size.** 0x00460f50 and
  `DrawPathTileOverlay` (0x00460e90) are adjacent and call the same three
  helpers in the same order; the only difference is that 0x00460f50 takes a
  fourth `mode` argument and spends it on the corner draws. The base-tile draw
  still goes through the mode-0 wrapper `PrintSpriteAt` in BOTH. Worth
  recording against the "size is not evidence of twinning" entry as the
  converse: a shared callee SEQUENCE is good evidence, and the diff was two
  arguments deep.
- Small negative worth recording: **`&arr[i]` subscript form was enough for
  both strength-reduced cursors in this file** (`g_render_nodes[i].x` anchors
  at +6 by the one-reference-each tie-break, `g_text_cache[i].sprite` anchors
  at +0x1c as the only field read). Neither needed a named or biased pointer,
  which the recorded const-table entry warns about — the biased-pointer form
  is for READ-ONLY tables walked by more than one field.

## Method notes

- Every function in this file landed at 100% within at most one iteration.
  The two that took a second build (`FlushCursorSpriteList`,
  `ExpireCachedText`) were both fixed by a lever already in `DECOMP.md`; the
  two largest (`DrawPathTileOverlay`, `PaintCursorTiles`) were exact on the
  first compile, because the context files named every callee and
  `pathbuild.c` supplied the exact short-arithmetic spelling.
- The side-by-side lister used is `scratchpad/sbs.py` (aligned by `difflib`
  over `match.py`'s `norm`); `matchfull.py`'s diff view elides matched lines
  and made a pure two-instruction SHIFT look like an insert/delete pair.

---

# coaster7.c

## Lane `fable-d` — `LEGOLAND/coaster7.c`

Coaster entrance track, model loader and physics callbacks (SCOPE_FABLE_D
§`coaster7.c`). **10 of 10 exact**, `tools/audit.py LEGOLAND/coaster7.c`
ends `PASS`, `/W3 /O2 /Gy /Gd` compiles clean, no duplicate addresses across
the tree.

## Result table

| address | name | insns | bytes | audit | marker committed |
| --- | --- | --- | --- | --- | --- |
| 0x0041f380 | `Span_SetClip` | 23 | 84 | `[OK]` | `// FUNCTION: LEGOLAND 0x0041f380` |
| 0x0041e4c0 | `Route_SetDeadline` | 23 | 47 | `[OK]` | `// FUNCTION: LEGOLAND 0x0041e4c0` |
| 0x004223c0 | `CountModelRecords` | 27 | 53 | `[OK]` | `// FUNCTION: LEGOLAND 0x004223c0` |
| 0x00426490 | `Mat3_ToMat4` | 30 | 73 | `[OK]` | `// FUNCTION: LEGOLAND 0x00426490` |
| 0x00424bc0 | `Coaster_GetLongestWait` | 30 | 66 | `[OK]` | `// FUNCTION: LEGOLAND 0x00424bc0` |
| 0x00429940 | `TrackRunStepsBack` | 33 | 71 | `[OK]` | `// FUNCTION: LEGOLAND 0x00429940` |
| 0x0041de10 | `RoutePhys_EvaluateDerivative` | 55 | 169 | `[OK]` | `// FUNCTION: LEGOLAND 0x0041de10` |
| 0x004248b0 | `Coaster_StationDerivative` | 57 | 168 | `[OK]` | `// FUNCTION: LEGOLAND 0x004248b0` |
| 0x00420550 | `CoasterModel_LoadFile` | 92 | 235 | `[OK]` | `// FUNCTION: LEGOLAND 0x00420550` |
| 0x00423a10 | `Castle_InitEntranceTrack` | 231 | 811 | `[OK]` | `// FUNCTION: LEGOLAND 0x00423a10` |

All ten end in a real `ret`, none is recursive, all are promotable. Names are
the scope's; nothing was renamed.

## Mechanics recovered

- **The route physics is literal energy conservation.** The solver's state
  vector is `{track parameter, total energy}` and `RoutePhys_EvaluateDerivative`
  returns `{speed, power}`:
  `v = sqrt(2 * (E - PE) / m)`, clamped at zero rather than rooting a
  negative, with `PE` a sum over the route's node list and `m` the train's
  mass. The energy rate is zero unless the lift/launch motor is armed
  (`power > FLT_MIN`) **and** the train is still travelling **less** than 0.05
  units per tick — a launch assist that cuts out once the train is up to
  speed. Only the mass/power getter is a single call; everything else is
  summed per node.
- **`Coaster_StationDerivative` is a constant-deceleration STATION BRAKE laid
  over that derivative.** It runs the ordinary derivative first and then
  overwrites `out->v[1]` with `-m * a * v`, where `a = v*v / (2*d)`, `v` is
  the train's travel this tick and `d` the remaining track distance from the
  train's live position record (`rt+0x0c`) to the station square. That is
  exactly the deceleration that brings the train to rest at the station.
  The distance is scaled by `0.0015875f` (0x004ab404) on the way out of
  0x0041dd00 / 0x0042a1b0.
- **The castle ENTRANCE TRACK is an L of three curve segments plus a piece
  table** (`Castle_InitEntranceTrack`):
  - three 0x58-byte `RouteGeom`/`Curve` records at 0x006102f8, 0x00610350 and
    0x006103a8 (`g_station_geometry`), chained `A <-> B <-> C` through
    `+0x50` (next) and `+0x54` (prev);
  - A and C are straight runs built by 0x00421ab0 with sideways offsets
    `{-10,0,0}` and `{0,-10,0}`; B is the corner, built by 0x00421ce0 from
    three world points and the shape constants 1.5f and 0.5f;
  - a table of 0x24-byte piece templates at 0x0060f928, count at 0x00610a08:
    `{ Pos16 sq; int geom; float t0, t1; FootPart part; }`. `geom` is 0/1/2
    (which segment) and `[t0,t1)` is that square's equal slice of the
    segment's `[0,1]` parameter range — so `step = (t1 - t0) / n` with
    `n = (c2.y - c1.y) >> 1` for the y run and `(c1.x - c2.x) >> 1` for the x
    run, i.e. one piece every TWO map squares.
  - the last 0x14 bytes of every template are a `FootPart`
    (`x0,y0,x1,y1,next`), filled in a final pass as the 1x1 square of that
    piece and threaded into one list, terminated on the last entry.
- **`RouteGeom` and schoolcar8.c's `Curve` are one record.** 0x00421ab0 fills
  `pos = from`, `dir = to - from`, `offset`, `length = |dir|` at +0x40, sets
  the parameter range to `[0.0f, 1.0f]` at +0x44/+0x48, an evaluator pointer
  0x004dd600 at +0x4c, and clears the two links. Size 0x58.
- **The entrance hangs off the castle's own footprint.**
  `Castle_GetFirstCorner` (0x004239b0) and `Castle_GetSecondCorner`
  (0x004239e0) both read the `FootPart` rect at +0x3c of `g_castle_def`
  (0x00829bf8) — four ints, of which only the low words are used — and add the
  caller's square: first = `(x1 + sq.x, y0 + sq.y - 2)`, second =
  `(x0 + sq.x - 2, y1 + sq.y)`. `Castle_InitEntranceTrack` calls both with a
  `{0,0}` square.
- **`CoasterRec` embeds its STATION piece as a whole `TrackNode` at +0x04.**
  That is why schoolcar8.c's `Coaster_GetStationStart` takes the record as a
  `const short*` and reads its map square at short index 4 —
  `TrackNode.sx` sits at `TrackNode+0x04`, i.e. `rec+0x08`.
- **`Span_SetClip` writes a four-plane half-plane clip set**:
  `{ int count; struct { float a, b, c; } p[4]; }`, tested as
  `a*x + b*y >= c`, so the right and bottom edges are stored negated
  (`-x >= -right`). Built from coastermath.c's `SpanRect {top,left,bottom,right}`.
- **Coaster model images are CRLF-terminated text.** `CountModelRecords`
  walks the `{image, byte length}` pair one byte at a time over the CRLF
  predicate 0x004222f0, stepping two at a break; the bound is one byte short
  of the image so the 16-bit CR/LF test cannot read past the end.
- **`CoasterModel_LoadFile` is schoolcar5.c's `LoadWholeFile` wrapped in a
  chdir pair.** It enters `RollerCoaster\RollerCoaster\CreatedData`, does the
  CreateFileA / GetFileSize / allocate / ReadFile / CloseHandle sequence, and
  restores `..\..\..` on **all four** exits — so the restore call is written
  out four times and none of the copies merge.

## Callees named for the first time

| address | name here | evidence |
| --- | --- | --- |
| 0x004222f0 | `ModelImage_IsEOL` | `cmp word ptr [ecx], 0xa0d / sete al` |
| 0x0041dae0 | `Route_SumPotentialEnergy` | sums 0x0041e810 over the route's +0x70 list; consumed as the `E - PE` term |
| 0x0041db90 | `Route_GetMassAndPower` | two float out-params; the first divides `2*(E-PE)`, the second is the energy rate |
| 0x0041dd00 | `Route_TravelPerTick` | `|tangent| * speed * 0.0015875f` |
| 0x0041dd50 | `Route_TravelThisTick` | 0x0041dd00 applied to 0x0041dca0's current speed |
| 0x0041dd70 | `Route_SumMass` | sums 0x0041e7e0 over the same node list; the `m` of `-m*a*v` |
| 0x0042a1b0 | `Track_MeasureDistance` | distance between two position records; result is the brake's `d` |
| 0x00426a90 | `VecMath_Sqrt` | `fld [ebp+8] / call [0x00829a58]`, the module's fast-sqrt hook |
| 0x00420530 | `SetWorkingDirectory` | null-checked `SetCurrentDirectoryA` (schoolcar.c calls it `Sub_420530`) |
| 0x00421ab0 | `Curve_InitLine` | builds a straight segment; see above |
| 0x00421ce0 | `Curve_InitCorner` | the corner counterpart, `(a, b, c, curve, 1.5f, 0.5f)` |
| 0x0060f928 / 0x00610a08 | `g_entrance` / `g_entrance_count` | the piece-template table and its count |
| 0x006102f8 / 0x00610350 | `g_castle_curve_a` / `_b` | the first two entrance segments |

Also confirmed from the caller side: 0x00422300 (just above `ModelImage_IsEOL`)
copies one CRLF-terminated line out of a model image, and 0x0041e810 /
0x0041e7e0 are the per-node potential-energy and mass contributions.

## Original bugs reproduced

- **`Castle_InitEntranceTrack` terminates the footprint list off the LOOP
  variable, not the count.** `g_entrance[k - 1].part.next = 0;` with `k` the
  final-pass index: when the table is empty (`i == 0`) `k` is 0 and the store
  lands four bytes in FRONT of `g_entrance`. Reproduced, commented at the
  site. (For any non-empty table `k == i` and the store is correct.)

## Extern-type divergences (do NOT align)

- `coastermath.c` declares `Span_SetClip(const SpanRect*, void* context)`;
  the definition needs the real four-plane record as the second parameter.
- `schoolcar8.c` declares `CoasterModel_LoadFile(const char* name, int mode)`.
  The second parameter is really an **optional `unsigned int*` byte-length
  out-pointer** — the body ends `if (len) *len = n;`, which is what makes
  `CoasterModel_LoadLTX`'s literal `0` legal. schoolcar8.c's extern is left
  as it is.
- `schoolcar8.c` declares `g_station_geometry` as `unsigned char[]`; it is the
  THIRD castle curve segment and is typed `RouteGeom[]` here. The **array**
  spelling is load-bearing in both files (`push OFFSET`, not a load) —
  recorded lever "two tables pushed as `push OFFSET` are ARRAYS".
- `schoolcar8.c`'s `PhysObj.route` is a `CoasterRoute*` whose +0x20 view it
  calls `RoutePhysics`; they are one object and this file types it as one
  `CoasterRoute` with `here` at +0x0c and `{dimension, energy[2]}` at +0x20.

## Levers, with evidence

- **The `- 1` of an end bound belongs in the LOOP CONDITION, not the end
  pointer's initialiser.** `end = data + len; while (p < end - 1)` gives the
  original's `add eax,esi` + `lea ebx,[eax-1]` (27/27); folded into the
  initialiser (`end = p + len - 1`) VC6 emits one `lea ebx,[eax+esi-1]` and
  the function is 24 of 26. Four other spellings (a second statement, `end--`,
  `end = end - 1`, a named `len` local) are all the folded form.
  (`CountModelRecords`.)
- **Three adjacent constant stores to one array are emitted in SOURCE order,
  not reversed.** `Mat3_ToMat4`'s trailing `m[11] = m[7] = m[3] = 0.0f` was
  swept over all six permutations: `11, 7, 3` is exact, `3, 7, 11` is 28 of
  30 and every other order 29 of 30. The recorded "adjacent address stores
  come out REVERSED" rule is for a PAIR; with three, only the descending
  order matched here — sweep, do not assume.
- **A float value that must survive a call needs its own IR temporary, and a
  block-scoped SECOND local is what supplies it.** In
  `Coaster_StationDerivative` the negated quotient written as a re-assignment
  of the `v*v` local keeps the whole chain on the x87 stack, sinks `v*v` past
  the distance call and flushes the merged `add esp,0x28` **before** the first
  `fmul` — 49 of 57. Writing it as its own `{ float brake = -(vv / (d+d)); }`
  forces `vv`'s memory home and restores the original's
  `call / fmul [vv] / add esp,0x28 / fmul [v]` order: **57/57**. Free
  volatile reads of `v` and `vv` at the use, at the definition, and all six
  operand orders of the final triple product floor at 56. Same family as the
  wave-fourteen caveat "one extra IR temporary advances what a volatile read
  cannot".
- **`x * 2` on a float in x87 is `fadd st(0),st(0)` only when the operand is
  ALREADY on the stack.** `2.0f * (E - pe) / m` gives the original's
  `fsub / fadd st,st / fdiv`; and `d + d` on a named call result gives
  `fmul K / fadd st,st / fdivr`. The integer rule ("`x*2` must be `x+x`")
  carries over unchanged.
- **A `double`-typed threshold survives as `fcomp qword`.** `< 0.05` (no `f`
  suffix) is the original's `fcomp qword ptr [0x004ab408]`, where the two
  float thresholds in the same function are dword-pooled. And `fcomp` +
  `test ah,1` + `je` is `<`, not `>=`: the `je` is taken when C0 is CLEAR,
  i.e. when the comparison is **not** less-than, so the guarded store belongs
  to the `<` arm. (One mismatch in `RoutePhys_EvaluateDerivative` until the
  direction was flipped.)
- **`1.175494351e-38f` is FLT_MIN (0x00800000) and reproduces exactly** — a
  "is this armed at all" guard on a float, not a magic number.
- **An explicit `i = 0;` statement at the TOP of a function is what gives VC6
  a SECOND zero register.** `Castle_InitEntranceTrack` runs on two: `ebx` for
  the `push 0.0f` arguments and the byte stores, and `esi` — which is the
  running table index, still zero — for the two `off.y`/`off.z` float-zero
  stores. Written as `for (i = 0; ...)` the index is not live that early, VC6
  keeps one zero register and the body comes out an instruction SHORT: 170 of
  230 against 187 of 231. Declaring `i = 0;` before the first call and writing
  the loop `for (; i < n; i++)` is the whole difference. Sharpens the recorded
  zero-web entries: the threshold decides *whether* a zero register appears,
  but an already-zero long-lived local is what supplies a *second* one.
- **A struct copy placed between the count and the divide keeps the `fidiv`
  spill out of the copied-to local's frame slot.** `n = ...; sq = c2;
  t = ...; step = (...)/n;` is 201 of 231; with `sq = c2;` written after
  `step`, VC6 reuses `sq`'s (escaped!) slot for the integer spill `fidiv`
  needs, instead of sharing it with `step`, and the frame shifts — 197 of 231.
  The store has to be live across the divide for the slot to be denied.
- **Where a shared register is freed decides which register carries a
  function-wide zero.** The six curve-chain stores written BEFORE `sq = corner;`
  keep `corner` in ebx across them, so VC6 takes a scratch `edx` for the zero
  it needs from there to the end; written after, ebx frees early, VC6 takes
  ebx as the zero and **every** register in the last two loops and the
  epilogue is one position off. 201 of 231 -> **231/231**, with no other
  change. Within the six stores, `prev` before `next` in each pair is also
  load-bearing (227 the other way round).
- **A `short` local keeps its sign extension at the USE; an `int` one moves it
  to the definition and loses an instruction.** `cy2` (`c1.y + 2`, needed
  again 80 instructions and three calls later) as an `int` makes VC6 emit
  `movsx edi,ax` at the definition and drop the original's `movsx ecx,di`
  entirely — 187 of **230** for a 231-instruction function. As a `short` it is
  `mov di,bp` at the definition and `movsx` at the use: 197 of 231. When your
  body is exactly one instruction short and the original sign-extends late,
  narrow the local's type.
- **`>> 1` versus `/ 2` on a signed count** — the original's bare `sar eax,1`
  is the shift; `/2` would add `cdq/sub`. Third instance in the tree.
- **A dword load feeding only 16-bit stores is normal VC6 output, not a union
  in the source.** `mov eax,[esp+0x30] / mov word [..],ax / inc eax /
  mov word [..],ax` is what plain `Pos16` field arithmetic compiles to (the
  dword form is a byte shorter than `mov ax,[..]`); no packed-union spelling
  is needed. Worth recording because the recorded `{u8,u8}` dword-plus-mask
  idiom reads like it would be.
- **A four-times-repeated cleanup call does not merge.** `CoasterModel_LoadFile`
  has the chdir restore in all four exits and VC6 cross-jumps none of them,
  because each arm's tail differs (the `xor eax,eax` placement and, on the
  success path, the out-param store). Written out four times: 92/92 first try.
- **The push-sinking rule at its cleanest.** `Coaster_GetLongestWait`'s
  `push ebx` sinks below the empty-list test because the whole walk lives in
  the guarded block; and `TrackRunStepsBack`'s two exits (`*endOut = node;
  return steps;`) are written once and duplicated by VC6, with different
  schedules in each copy — no source construct required. 30/30 and 33/33
  first try.

## Tooling

- No instance of the recorded extent-walker defect (the rotated-loop entry
  `jmp`) in this file — the two candidates, `Castle_InitEntranceTrack`'s three
  loops and `TrackRunStepsBack`'s rotated walk, all bound correctly and audit
  reports the exact original extents.
- A side-by-side lister was kept at
  `scratchpad/fd_coaster7/sbs.py <file> <func> <addr>`; it prints
  original-vs-ours index for index, which is what made the one-instruction
  deficits above readable.

---

# uimisc2.c

## Lane `fable-d` / `uimisc2.c` — movie, help bar, report and level-end screens

**Result: 14 of 14 exact (802 instructions, 2,526 bytes), `audit.py` PASS,
`/W3` clean.** Every body ends in a real `ret`, none is recursive, none needed
the truncated-extent (ESCAPES) caveat, and none needed a `volatile` barrier or
any other artificial construct. Ten of the fourteen matched on the FIRST draft;
the four that did not are the four levers written up below.

| address | name | insns | bytes | pct | audit | marker committed |
| --- | --- | --- | --- | --- | --- | --- |
| 0x00490b90 | `ReportPrevPageInput` | 22 | 71 | 100% | `[OK]` | `// FUNCTION: LEGOLAND 0x00490b90` |
| 0x00468b00 | `EnqueueObjectHelp` | 23 | 57 | 100% | `[OK]` | `// FUNCTION: LEGOLAND 0x00468b00` |
| 0x0046d3c0 | `FreeIcon` | 34 | 121 | 100% | `[OK]` | `// FUNCTION: LEGOLAND 0x0046d3c0` |
| 0x0048abb0 | `StartFreePlayPark` | 39 | 172 | 100% | `[OK]` | `// FUNCTION: LEGOLAND 0x0048abb0` |
| 0x00490610 | `SetReportMovie` | 42 | 108 | 100% | `[OK]` | `// FUNCTION: LEGOLAND 0x00490610` |
| 0x00491080 | `PrintReportLine` | 43 | 103 | 100% | `[OK]` | `// FUNCTION: LEGOLAND 0x00491080` |
| 0x00468b40 | `SetScriptEventText` | 46 | 108 | 100% | `[OK]` | `// FUNCTION: LEGOLAND 0x00468b40` |
| 0x0048d230 | `RestoreCurrentProfileFromList` | 56 | 194 | 100% | `[OK]` | `// FUNCTION: LEGOLAND 0x0048d230` |
| 0x004908b0 | `KillReportScreenSprites` | 56 | 183 | 100% | `[OK]` | `// FUNCTION: LEGOLAND 0x004908b0` |
| 0x0046de90 | `GetIconHitBounds` | 57 | 160 | 100% | `[OK]` | `// FUNCTION: LEGOLAND 0x0046de90` |
| 0x00459710 | `RunLevelEndSequence` | 67 | 204 | 100% | `[OK]` | `// FUNCTION: LEGOLAND 0x00459710` |
| 0x00490ea0 | `BlinkReportPageIcons` | 80 | 242 | 100% | `[OK]` | `// FUNCTION: LEGOLAND 0x00490ea0` |
| 0x0046d110 | `UpdateHelpBar` | 81 | 280 | 100% | `[OK]` | `// FUNCTION: LEGOLAND 0x0046d110` |
| 0x004771f0 | `PlayMovie` | 156 | 523 | 100% | `[OK]` | `// FUNCTION: LEGOLAND 0x004771f0` |

Nothing is left `[WIP]`, so there is no residual to triage.

---

## Levers (evidence attached)

- **A by-value `WinRect`'s field-assignment order is its register rotation —
  confirmed on LIVE values, not just constants.** `mapscreen2.c` records the
  rule for four literals in `PrintScreenMode8`; `PrintReportLine` is the same
  rule where the four values are two parameter loads and two derived sums. The
  natural `left / top / right / bottom` gives every value the ring position one
  step off (x→eax, y→ecx, right→edx, bottom→esi) — **13 of 43 at IDENTICAL byte
  length**, register-blind 0. Writing `top / bottom / left / right` claims the
  eax→ecx→edx→esi ring in the original's order (y→eax, bottom→ecx, x→edx,
  right→esi) and closes the function with no other change. So the rule is about
  the ASSIGNMENT ORDER of the aggregate's fields, not about the kind of value
  being assigned; when a by-value aggregate's operands are one ring position
  out, permute the field assignments and nothing else.
- **A 15-byte packed struct copy must be a STRUCT ASSIGNMENT — that is what
  materialises the source address in a register.** In
  `RestoreCurrentProfileFromList` the five `ProfileStats` fields written one at
  a time (`g_cur_profile.stats.a = n->p.stats.a; ...`) fold each read into
  `[edx+0x38] … [edx+0x46]`; `g_cur_profile.stats = n->p.stats;` emits
  `lea ecx,[edx+0x38]` and reads `[ecx] / [ecx+4] / [ecx+8] / [ecx+0xc] /
  [ecx+0xe]` — the original, dword/dword/dword/word/byte. **A named
  `ProfileStats* st = &n->p.stats;` pointer does NOT reach it** (VC6 folds the
  pointer straight back into the displacements), so this is not the recorded
  "name the intermediate pointer" lever; the aggregate assignment is a
  different lowering. `profiles.c`'s `AddNodeToProfileList` writes the same
  copy the same way, which is the corroboration.
- **A list search whose success body lives INSIDE the loop threads away the
  post-loop null test.** Written `while (n) { if (hit) break; n = n->next; }
  if (n) { body }`, VC6 emits a second `test edx,edx / je` after the loop;
  written `while (n) { if (hit) { body; return; } n = n->next; }` the test
  disappears and the loop-exhausted edge becomes its own bare `ret`. VC6
  DUPLICATES that two-byte epilogue rather than jumping to the shared one,
  because the callee-saved pushes live inside the found path and the exhausted
  edge therefore needs no pops. Read a lone mid-function `ret` with no pops in
  front of it as exactly this shape.
- **VC6's inline `strcpy` copies a STRING LITERAL as one dword plus one byte;
  a named `extern const char[]` gets the full scan-and-copy expansion.**
  `PlayMovie` builds `"FMV\\" + name`. With the prefix as an extern array the
  copy is 20 instructions of `repne scasb` / `rep movsd` / `rep movsb`; written
  as the literal `strcpy(path, "FMV\\")` it is
  `mov ecx,[k] / mov [buf],ecx / mov dl,[k+4] / mov [buf+4],dl` — the original,
  and note it still LOADS from the pooled literal rather than using immediates.
  The same function's retry prefix is the real global `g_res_path` and keeps
  the long form, so both expansions sit in one body: **a short constant string
  prefix in the original was a literal in the source, and only its length has
  to be known at the call site.**
- **`#pragma function(memcpy)` around a single body is how one file mixes a
  CALLED and an INLINED `memcpy`.** `SetReportMovie`'s constant 0x100-byte copy
  is `call 0x004a0110` in the original; `RestoreCurrentProfileFromList`'s
  constant 200-byte copy is `rep movsd`. Declaring `memcpy` with
  `#pragma intrinsic(memcpy)` at the top, toggling to `#pragma function(memcpy)`
  immediately before `SetReportMovie` and back to `intrinsic` after it gives
  both. Under `/O2` alone the intrinsic wins everywhere, so the pragma is
  load-bearing, not decoration.
- **To EXILE a `return K` block past the function, nest the rest of the body
  inside the guard — a `goto` to a trailing label does not do it.**
  `PlayMovie` has three exits laid out `[main] [return 1] [return 0]`.
  `if (!mv) return 1;` puts the block inline at the test (`jne` over it);
  `if (mv) { …; return played; } return 1;` exiles it and gives the original's
  `test esi,esi / je <far>`. The same nesting one level up
  (`if (!g_game->no_movies) { … } return 0;`) merges BOTH `return 0` sites into
  one block laid last. Two `goto suppressed;` statements with
  `suppressed: return 0;` as the final statement were tried first and left the
  block inline at the SECOND goto — so the recorded "write it `goto fail;` to
  pin the merged block at the END" prescription is not general: the reachable
  handle here is the NESTING, and the `goto` form is only equivalent when the
  label has a single predecessor.
- **Naming the derived limit is what hoists a `lea` above a two-arm clamp.**
  `GetIconHitBounds` ends with two clamps against `base` and `base + 0x28`.
  Spelled `if (r->bottom < base + 0x28) r->bottom = base + 0x28;` VC6
  recomputes with `add eax,0x28` inside the second test and loads `r->bottom`
  into a register first — 7 of 57. `int lim = base + 0x28;` written before the
  pair gives the original's `lea ecx,[eax+0x28]` scheduled into the FIRST
  clamp's compare/branch gap AND the memory-operand compare
  `cmp dword ptr [esi+0xc], ecx`. 0 of 57. Same family as "a named sum blocks
  VC6's algebra", from the scheduling side: the temporary buys the hoist.
- **Seven literal zeros hoist the zero register with no loop in the function.**
  `KillReportScreenSprites` is seven written-out `if (s) { KillSprite(s);
  s = 0; }` blocks plus one call; VC6 hoists `xor esi,esi` and every guard
  becomes `cmp eax,esi` rather than `test eax,eax`. Consistent with the
  recorded three-to-five threshold, and worth knowing as a READ: a run of
  `cmp reg,<callee-saved>` guards in a teardown function means the source had
  that many literal zeros, not a loop.
- **Nine cdecl argument pushes across a whole straight-line function merge into
  ONE `add esp`.** `StartFreePlayPark` cleans nothing until the epilogue's
  `add esp,0x58` (0x34 of frame plus 0x24 of arguments over nine calls). The
  consequence is that its single 0x34-byte stack buffer is addressed as `[esp]`
  at the `sprintf` and as `[esp+8]` at the loader — the same local, at two
  displacements, because two argument dwords are still pending. Resolve stack
  displacements by push depth before deciding two `lea`s name different locals.
- **A four-term short-circuit hit test written out inline three times.**
  `BlinkReportPageIcons` was exact on the first draft with
  `if (mx < p->x || mx > p->w + p->x || my < p->y || my > p->h + p->y)` spelled
  in full at each of three sites and each icon global read DIRECTLY at every
  use. Two details carry: the right and bottom edges are computed only after
  the earlier terms pass (so they must be inside the `||`, not in a
  pre-built rect), and the sums are `w + x` / `h + y` — width first — which is
  what puts the sum's destination in the w/h register. `uimisc.c`'s
  `GetIconBounds` writes `r->right = p->w + p->x;` the same way round.
- **`ReportPrevPageInput`'s two-parameter prototype is a codegen lever.** The
  twin `ReportNextPageInput` (0x00490b20, `uimisc.c`) forwards `ev` to
  `ReportAcceptInput` and therefore loads it as a dword; the Prev handler only
  tests it, and `char ReportPrevPageInput(Icon*, int)` — the two-argument form
  `mapscreen2.c` declares — is what gives the original's
  `mov al, byte ptr [esp+0x10] / test al,2`.

---

## Mechanics recovered (runtime-spec material)

### The movie player — `PlayMovie` (0x004771f0)

`int PlayMovie(const char* name, int flags, int force)`. **It is not
DirectShow at this level**: no COM vtable call appears anywhere in the body.
Three plain cdecl entry points do the work, and the handle is an ordinary
pointer whose +0x14 and +0x1c blocks `CloseMovie` releases:

| address | name (ours) | signature |
| --- | --- | --- |
| 0x00476460 | `OpenMovie` | `void* OpenMovie(const char* path)` |
| 0x004766f0 | `RunMovie` | `int RunMovie(void* mv, WinRect* dst, int flags)` |
| 0x00476630 | `CloseMovie` | `void CloseMovie(void* mv)` |

- **Destination is the fixed rectangle `{0, 0, 0x140, 0xf0}`** — 320x240, built
  as a full aggregate initialiser at the top of the frame. The movie is never
  scaled to the window.
- **Two search prefixes.** `"FMV\\" + name` first; if `OpenMovie` fails,
  `g_res_path` (0x00813b04 — the alternate-volume prefix `data2.c` and
  `sysmisc2.c` use for the CD) + name. Failing both, the function returns 1.
- **Suppression.** `g_movie_shown` (0x00668fb0) is a one-shot latch: `force`
  clears it, and with `force == 0` a set latch returns 0 immediately. The game
  record's +0x40 (0x004bcbf4) is a global "no movies" flag; set, every call
  returns 0.
- **The audio duck is three separate layers**, and they nest differently:
  `PauseCurrentTrack` (0x00498920) is called BEFORE the file is even opened,
  while `PauseAllSamples` (0x00492830) and `StopMusic` (0x00492d80) bracket
  only the playback and are undone by `ResumePausedSamples` (0x00492850) and
  `RestartMusic` (0x00492da0) after it. The streaming track is never resumed
  here — `g_6687b0` is set to 4 instead (see below).
- **Video.** `PushRenderingStatusAndUnlockVideoSurface` (0x00464080) /
  `PopRenderingStatus` (0x004641f0) wrap `RunMovie`.
- **The button drain.** After `PopRenderingStatus` the function spins
  `do { ProcessSystemEvents(); ReadGameButtons(); } while (g_mouse_btn_a & 7);`
  so the click that skipped the movie is not delivered to the screen
  underneath. The mask 7 is hoisted into `bl` for the loop.
- **Return value:** 0 = suppressed, 1 = could not open at either prefix,
  otherwise `RunMovie`'s own result. `uimisc.c` declares the function `void`.
- Six debug strings survive in `.rdata` even though both logging stubs
  (0x0047f870 varargs, 0x0047f850 flush) are a bare `ret` in the shipped build:
  `"Attempting to open Movie %s"`, `"Movie openned OK (%s)"` *(sic)*,
  `"Attempting to play movie.."`, `"Starting Movie\n"`, `"Stopping Movie\n"`,
  `"Stopping movie"`. The last three go out through `DBPrintf` (0x00453a20)
  and the debug stub in an interleaved order that is part of the match.

### The help bar and narration — `UpdateHelpBar` (0x0046d110)

- **`g_help_target` (0x004b9f8c) is POLYMORPHIC, and the face state is the
  discriminator.** Face state 0 formats it as a decimal string id
  (`"Text%04d.wav"`), states 1 and 2 as a `char*` base name (`"%s.wav"`,
  `"%sz.wav"`). That is exactly why `uimisc.c`'s `ShowIdHelp` stores an id and
  sets face state 0 while `ShowObjectHelp` stores an object pointer and sets
  face state 2 — one int, two types, selected by a second global.
- Face state >= 3 (`fpui5.c`: 4 = "the advisor is talking") suppresses the
  whole update.
- **`g_6687b0` (0x006687b0) is a frame HOLD-OFF counter**, not a mode. While it
  is non-zero and `g_help_force` is clear, `UpdateHelpBar` decrements it and
  does nothing else. `PlayMovie`, `ReportAcceptInput` and
  `FreePlayAcceptInput` all set it to 4 — i.e. "swallow help for four frames
  after this transition".
- **The hover threshold is 0x1f4 ms (500 ms)** from `g_help_hover_start`
  (0x007fe920, a `GetTickCount` stamp). `g_help_force` short-circuits the wait.
  A hover that has not matured sets `g_help_deferred` (0x006687b4) to 1 and
  returns; a matured one clears it, clears `g_help_changed` and `g_help_force`,
  re-stamps the hover time, and plays the wave through `PlayNarrationFile`
  (0x00498630) between `PauseCurrentTrack` and `ResumeCurrentTrack`.
- `g_help_target` is reset to -1 on any frame where nothing requested help.

### The report screen

- `ReportPrevPageInput` steps `g_rep_page` back by **14** — the page size
  `mapscreen2.c` documents — and calls `UpdateReportPageIcons`. Unlike its Next
  twin it has no "icon greyed → forward to Accept" arm.
- `PrintReportLine(text, x, y, h, big)` centres one line in
  `(x, y)-(x + 0x1cc, y + h)`. **The box is always 460 px wide**, whatever `x`
  is. `big` picks font 3 through `NewPrintCent` (0x00491d60) and font 2 through
  its twin 0x00490fa0.
- `BlinkReportPageIcons` refreshes three icons per frame — next (0x007cb2e4),
  previous (0x007cb2e0), hint (0x007cb1c0) — drawing each in its resting
  sprite while the mouse is OFF it. Only the Next icon blinks (plain vs lit on
  `GetBlink()`); the other two just reset.
- `KillReportScreenSprites` frees seven sprites (backdrop, next, next-lit,
  prev, prev-lit, hint1, hint2) and removes icon group 7.
- `SetReportMovie` fills the 256-byte `g_report_movie` (0x00798778) that
  `uimisc.c`'s `ReportAcceptInput` plays on the way out of the screen.

### Level end — `RunLevelEndSequence` (0x00459710)

- **The argument's format is `"<helpkey>;<movie>"`.** `uimisc.c`'s `EndLevel`
  passes one of the two strings at 0x00832998 / 0x00832a98 according to the
  result code.
- The `';'` is overwritten with NUL, the left half `strcpy`'d into an 0x80-byte
  stack buffer, and **the `';'` is then written BACK** (`*p++ = ';'`), so the
  caller's string survives intact for the next call.
- A non-empty right half is copied into `g_level_end_movie` (0x008100c0) and
  `g_level_end_has_movie` (0x00832bac) set. The consumer is 0x00458dc0, which
  does `SetPointer(0) / PlayMovie(g_level_end_movie, 1, 1) / SetPointer(5)` and
  clears the flag.
- On a successful `LoadHelpTextFor` the game switches to icons2 mode 1, game
  mode 2, current screen -1 and screen mode 7 — i.e. the front end takes over.

### Free play — `StartFreePlayPark` (0x0048abb0)

- The level database is the **fixed file `"FreePlayTest.txt"`** (0x004beb4c),
  produced with `sprintf` and no conversions at all.
- Start-up order: clear the selected class → format the name → pause the game
  timer → reset the game clock (0x00499410) and the save timer → clear "castle
  built" (0x0079a8d0) → reset the map AI → load the database into 0x00667c4c →
  0x00457870(0) → 0x0048ab60() → `AllocBlokeCounters(g_game->+0x1a)` →
  0x00458940() (enters game mode 3) → clear `g_pending_state` → 0x00489ee0()
  → `UpdateMenu` → `ClearWaitSprite` → `ShowInfoPanel(1)` →
  `SetInfoPanelText(g_script_text1, 0)` → 0x00458bb0(1) → `ThawGameClock` →
  `UpdateSoundVols`.
- 0x00489ee0 fills 0x007cb3e0..0x007cb5e0 with the word 0xffff at a **4-byte
  stride** (128 entries, upper half untouched) — a free-play goal/slot table.
- The record at 0x004bcbf4 has an `unsigned short` at **+0x1a** that is
  `AllocBlokeCounters`' per-class counter size.

### The icon record (additions to `iconui.c`'s map)

- **+0x22 is a signed short**: an extra hit-box height used only under icon
  flag 0x200, where `GetIconHitBounds` makes the box span
  `[y + f22, y + f22 + 0x28]`.
- **Flag 0x80 means the +0x30 widget back-pointer is a heap block the icon
  OWNS** — `FreeIcon` frees it before freeing the record.
- The three hit-box growth modes: **0x20** inflates by 3 on all four sides;
  **0x40** raises the top by 0x16 (the pop-up title bar) and then consults the
  widget; **0x200** applies the +0x22 span.
- The widget's own flag byte at **+0x22**: bit 3 grows the box DOWN by
  `g_icon_hit_dy` (0x00668840), bit 1 grows it RIGHT by `g_icon_hit_dx`
  (0x0066884c). Both are `short`. **Neither address is referenced anywhere else
  in `.text`** (four-byte little-endian scan of the section: one hit each, both
  inside `GetIconHitBounds`), and both sit past the raw `.data`, so they are
  zero at start-up and can only be non-zero if something writes them through a
  base pointer. Recorded as found rather than explained.
- `FreeIcon` clears `g_focussed_icon` on the REMOVED record; `uimisc.c`'s
  `UnlinkIcon` clears it on the removed record's SUCCESSOR. Between them the
  two cases are covered — the asymmetry in `UnlinkIcon` is not a lone bug.

### The script-event record

- **+0x38 is a PRIORITY (`int`)**, and the object-help queue at 0x00668724 is
  kept in DESCENDING priority order. `uimisc.c`'s view of the record leaves
  +0x38 inside the pad, so this is new.
- Flag **0x20** at +0x10 means "the event owns its text": `SetScriptEventText`
  sets it when it copies the string onto the heap and clears it when it adopts
  the caller's pointer. `uimisc.c`'s `ShowScriptStepText` is the caller that
  hands over ownership by clearing the step's own pointer and setting 0x20 by
  hand.

### Profiles

- The live `CurProfile` (0x0080ffa0) and the on-disk / list `Profile` hold the
  same fields in DIFFERENT layouts (the three slot bytes sit between the stats
  and the 200-byte block in one and after the block in the other), which is
  why `RestoreCurrentProfileFromList` is a field-by-field copy and not a struct
  assignment. Only the 15-byte `ProfileStats` group is shared verbatim.
- The restore does **not** restore the current save slot (+0x44) or the +0x45
  flag; it zeroes both.

---

## Original bugs reproduced (commented at the site)

- **`EnqueueObjectHelp` (0x00468b00) DISCARDS the queue on a head insert.** The
  "no predecessor" arm is `g_object_help = e; e->next = 0;`, shared with the
  empty-list case where it is correct. It is reached whenever the new event's
  priority is >= the current head's, and then the entire existing queue is
  leaked and dropped.
- **`RunLevelEndSequence` (0x00459710) can pass an uninitialised buffer.** With
  no `';'` in the string, `key[0x80]` is never written and is handed to
  `LoadHelpTextFor` anyway. Both shipped strings contain one, so it never
  fires in the retail game.
- **`SetReportMovie` (0x00490610) NUL-terminates twice and truncates
  silently.** The short path is `strcpy`'d and then written again at
  `movie[strlen(name)]`, costing a third inline `strlen`; the over-long path is
  copied at 0x100 bytes and byte 255 forced to NUL rather than being rejected.
- **`FreeIcon` (0x0046d3c0) compares against a dangling pointer.** The record
  is freed BEFORE the `g_focussed_icon == p` and `g_hit_info.obj == p` checks.
  Harmless (pointer identity only), and it is the original's order.

---

## Extern-type / naming divergences (deliberately NOT aligned)

- **0x004771f0 `PlayMovie` returns `int`** (0 suppressed / 1 open failed /
  `RunMovie`'s result). `uimisc.c` declares it `void`. Both callers ignore it.
- **0x00490fa0** — `screens2.c` names it `PrintCursor` (it draws the blinking
  name-entry cursor); it is `NewPrintCent`'s twin and is used here as the
  SMALL-font report-line printer. Same 4-argument, `WinRect`-by-value
  signature; the name is kept for resolution.
- **0x00498920** — `fpui5.c` and `uimisc.c` call it `PauseCurrentTrack`,
  `mapscreen.c` and `mapscreen2.c` call it `ResetFrontEnd`. One address, two
  names already in the tree; `PauseCurrentTrack` is used here because the two
  call sites both duck audio.
- **0x00492830** — `mapscreen2.c` declares it `InitOptionSamples`, but the body
  walks the sample list calling `PauseSingleSample` (0x00492800), so it is
  `PauseAllSamples`; that is the name used here and the divergence is left.
- **0x00490b90 `ReportPrevPageInput(Icon*, int)`** — two parameters, as
  `mapscreen2.c` declares, where the twin `ReportNextPageInput` needs four.
  See the lever above: it is what produces the byte-wide `ev` read.

## Callees named for the first time

`OpenMovie` (0x00476460), `RunMovie` (0x004766f0), `CloseMovie` (0x00476630),
`RestartMusic` (0x00492da0), `PauseAllSamples` (0x00492830),
`LoadLevelDatabase` (0x0047afb0), `ResetGameClock` (0x00499410).

Globals: `g_movie_shown` (0x00668fb0), `g_level_end_movie` (0x008100c0),
`g_level_end_has_movie` (0x00832bac), `g_help_deferred` (0x006687b4),
`g_icon_hit_dy` / `g_icon_hit_dx` (0x00668840 / 0x0066884c),
`g_freeplay_db` (0x00667c4c), `g_castle_built` (0x0079a8d0, confirming
`castleobj.c`'s reading), and the string constants 0x004beb4c
(`"FreePlayTest.txt"`), 0x004ba858/0x004ba868/0x004ba870 (the three narration
formats) and 0x004bb508..0x004bb588 (the movie strings and the `"FMV\\"`
prefix).

Still unnamed, kept as `sub_*` with their addresses: 0x00457870 (sets
0x004b90fc to `arg == 0`), 0x0048ab60 (walks the icon list on free-play
start-up), 0x00458940 (enters game mode 3 and rebuilds the cursor), 0x00489ee0
(fills the 0x007cb3e0 table with 0xffff), 0x00458bb0.
