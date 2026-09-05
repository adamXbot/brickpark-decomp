# Lane `fable-d` / `LEGOLAND/audio5.c` — narration and sample sources

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
