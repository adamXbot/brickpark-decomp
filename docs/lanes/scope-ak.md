# Scope AK — narration / sample-volume / LoadStrings (inventory group 18)

Branch `scope/AK`. File `LEGOLAND/narration2.c`. Object prefix `/tmp/sak_`.
Brief: `docs/SCOPE_AK_narration_strings.md`.

## Status

| address | name | insns | % | audit | marker |
| --- | --- | ---: | ---: | --- | --- |
| 0x00496760 | UpdatePlayingSampleSources | 33 | 100 | [OK] | FUNCTION |
| 0x004967f0 | FadeSamples | 73 | 100 | [OK] | FUNCTION |
| 0x004968d0 | RunSampleCallbacks | 32 | 100 | [OK] | FUNCTION |
| 0x00496920 | AutoKillSamples | 64 | 100 | [OK] | FUNCTION |
| 0x004969d0 | TickSamples | 4 | 100 | [OK] | FUNCTION |
| 0x00496e60 | FadeOutAllSound | 53 | 100 | [OK] | FUNCTION |
| 0x004975b0 | UnlinkSprite | 38 | 100 | [OK] | FUNCTION |
| 0x00497f60 | NarrRingA_FreeSpace | 15 | 100 | [OK] | FUNCTION |
| 0x00497f90 | NarrRingA_Contiguous | 7 | 100 | [OK] | FUNCTION |
| 0x00497fb0 | NarrRingA_Available | 25 | 100 | [OK] | FUNCTION |
| 0x00498000 | FillNarrationSourceRing | 73 | 100 | [OK] | FUNCTION |
| 0x00498100 | RewindNarrationAndFlag | 5 | 100 | [OK] | FUNCTION |
| 0x00498150 | ReadNarrationSource | 49 | 100 | [OK] | FUNCTION |
| 0x004981e0 | NarrationFreeSpace | 15 | 100 | [OK] | FUNCTION |
| 0x00498210 | NarrationContiguous | 7 | 100 | [OK] | FUNCTION |
| 0x00498230 | NarrationBytesReady | 5 | 100 | [OK] | FUNCTION |
| 0x00498250 | RefillNarrationRing | 102 | 100 | [OK] | FUNCTION |
| 0x00498b40 | PumpNarration | 142 | 100 | [OK] | FUNCTION |
| 0x00498d00 | LoadStrings | 192 | 100 | [OK] | FUNCTION |
| 0x00498f80 | AddString | 46 | 100 | [OK] | FUNCTION |

**20 / 20 exact, 980 instructions.** `audit.py` PASS, `relocs.py` zero
MISMATCH (26 UNRESOLVED, all CRT symbols declared through `<stdio.h>` /
`<ctype.h>` / `<stdlib.h>` / `<direct.h>`: `_getcwd`, `fopen`, `fread`,
`fseek`, `fclose`, `_chdir`, `atoi`, `exit`, `malloc`, `free`, `_isctype`,
`_pctype`, `__mb_cur_max`, `__chkstk` — addresses listed in the file's CRT
comment block), `/W3` clean.

## Names

Every function was unnamed except `RefillNarrationRing` (movie3.c) and
`LoadStrings` (startup.c); `NarrationContiguous` / `NarrationBytesReady`
were already declared by movie3.c for 0x00498210 / 0x00498230 and are kept.
`UnlinkSprite` is spritemisc.c's declared name for 0x004975b0.
`sub_498b40` (gameframe.c / screens3.c) → `PumpNarration`; `sub_4969d0`
(gameframe.c) → `TickSamples`. New: `UpdatePlayingSampleSources`,
`FadeSamples`, `RunSampleCallbacks`, `AutoKillSamples`, `FadeOutAllSound`,
`NarrRingA_FreeSpace` / `_Contiguous` / `_Available`,
`FillNarrationSourceRing`, `RewindNarrationAndFlag`, `ReadNarrationSource`,
`NarrationFreeSpace`, `AddString`.

Globals first named here: `g_narr_src_ring` (0x0079ac20, the 0x10000-byte
encoded ring), `g_narr_flags` (0x0079a83c). Renamed from savemisc2.c's
reset-only names, with the divergence recorded in the file: 0x007aac24
`g_speech_header_flag` → `g_speech_decoded_pos` (read position within the
decoded chunk) and 0x007caca4 `g_speech_chunk_pos` → `g_speech_decoded_len`
(bytes the last acmStreamConvert produced). savemisc2.c is not edited.

## Mechanics (spec for a runtime)

- **Sample housekeeping** (`TickSamples`, from GameFrame every frame, in
  this order): `RunSampleCallbacks` — for every live instance with a def,
  flag 0x10 and `due <= GetTicks()`, call `callback(s)`; non-zero re-arms
  `due = ret + now`, zero clears flag 0x10. `UpdatePlayingSampleSources` —
  every instance with a def and a non-zero source kind whose
  `GetStatus & DSBSTATUS_PLAYING` gets `UpdateSampleSource`.
  `FadeSamples` — every instance with a def, not stopped (flag 2) and a
  non-zero `fade` (+0x08): `vol = GetVolume + fade`; `>= 0` → 0 and fade
  ends; `<= -3000` → -10000, fade ends, and an auto-kill instance (flag 8)
  is `Stop`ped; then `SetVolume(vol)`. `AutoKillSamples` — every auto-kill
  instance not explicitly stopped whose DirectSound status is 0 is unlinked
  and `FreePlayableSample`d ("Killing Joust FX" trace when its def is
  `g_joust_fx[0].sample`). No `g_samples_ready` guard on AutoKill.
- **FadeOutAllSound(step, interval)**: while either slider > 0, subtract
  `step` from both (clamp 0), `UpdateSoundVols`, busy-wait `interval` ticks;
  then pause every sample, tear the speech stream down (`PauseCurrentTrack`),
  `g_6687b0 = 4`, restore both sliders and `UpdateSoundVols` again.
- **UnlinkSprite**: walk `g_sprites_head` for the record; unlink it (head or
  `prev->next`), or log "Couldn't unlink sprite" — and free it either way.
- **Ring A** (encoded source, 0x10000 B @ 0x0079ac20; read `g_narr_a`, write
  `g_narr_b`): `FreeSpace` = `b < a ? a-b-1 : (a == 0 ? 0xffff-b : 0x10000-b)`;
  `Contiguous` = `b < a ? 0x10000-a : b-a`; `Available` =
  `min((b-a) & 0xffff, queue[0])` after retiring a zero-count block (shift
  `g_narr_queue` down 19 ints, `queue[19] = 0`, `g_narr_c--` if non-zero).
  `FillNarrationSourceRing`: while free space and `g_speech_bytes_left`,
  `_read(min(space, left))` at the write cursor; a successful read debits
  `left`, credits `queue[g_narr_c]` and advances `b` mod 0x10000. A SHORT
  read (`got < space`) ends the block: with `g_narr_flags & 4` (loop) and
  the source exhausted it rewinds (`RewindNarrationAndFlag`: seek back and
  `flags |= 8`) and opens a new block; without the loop flag it opens a new
  block and returns. `ReadNarrationSource(dst, len)` is movie3.c's
  `ReadDecodedNarration` over ring A plus `queue[0] -= chunk`.
- **Ring B** (decoded PCM, 0x20000 B @ 0x007aaca0; read `g_narr_d`, write
  `g_narr_e`): the same three queries with 0x20000 / 0x1ffff.
  `RefillNarrationRing`: top ring A up; while ring B has space: copy what
  remains of the decoded chunk (`decoded_len - decoded_pos`), then if space
  remains decode another chunk — `n = min(queue[0], g_speech_chunk_size)`,
  `header.srcLength = n`, `ReadNarrationSource(g_speech_source, n)`,
  `acmStreamConvert(acm, &header, ACM_STREAMCONVERTF_BLOCKALIGN)`,
  `decoded_len = header.dstUsed`, `decoded_pos = 0`; a chunk SMALLER than
  the remaining space is copied whole and the function returns at once
  (write cursor advanced unmasked, no trailing ring-A top-up); otherwise
  `space` bytes are copied and the loop continues. Ends with a ring-A top-up.
- **PumpNarration** (every frame): state 0/1 → 0; state 2 →
  `RefillNarrationRing`, 0; state 3: while the play cursor is NOT inside
  block `g_speech_fill_block` (0x1000-byte blocks of the 0xa000 buffer):
  `Lock` that block, `ReadDecodedNarration(block, lockedBytes)`, copy
  (zero-pad a short read), `g_speech_blocks_ready++` if any data, `Unlock`,
  `fill_block = (fill_block + 1) % 10`; then `--g_speech_blocks_ready == 0`
  → `StopNarrationPlayback`, return 0. Returns 1 when caught up.
- **LoadStrings**: `_getcwd`; `fopen(".\\strings\\stab.str", "r")` (missing
  → `exit(1)`); size the file with a feof/ferror `fread` loop into `str`;
  `malloc(size+1)`, `fseek 0`, `fread` whole, `fclose`; `_chdir(cwd)`
  (failure → return, buffer leaked). Tokeniser: a digit run → `atoi` →
  current id (the terminator is un-read: `if (pos) pos--`); a token starting
  with `isalpha | ispunct` → copy chars into `str` until the SECOND unpaired
  `"` (a doubled `""` yields one literal quote) and `AddString(str, id)`.
  Buffers: `str[0xf0]`, `num[4]`, `cwd[0x100]`. `AddString(text, id)`:
  12-byte `StrNode {id, text, next}` malloc'd, pushed on
  `g_string_buckets[id % 10]`, text `strlen+1` malloc'd and `strcpy`d.

## Original bugs / quirks reproduced (commented at the site)

- `NarrRingA_Available` reads `queue[0]` once, before the retire shift, so
  the frame a block retires it returns `min(avail, 0) = 0`.
- `FillNarrationSourceRing`: a `_read` error (-1) still counts as a short
  read and `space -= -1` grows the space.
- `RefillNarrationRing`'s small-chunk arm returns without the trailing
  ring-A top-up and without the 0x1ffff mask (cannot wrap there).
- `LoadStrings`: `num[4]` overflows past three digits; a number ending at
  EOF backs `pos` onto its own last digit and the tokeniser never
  terminates; an unquoted string can never see a second quote and runs off
  the end of `str` (NEXT_CHAR yields 0 forever); `id` is used before any
  number is seen; `_chdir` failure leaks `buf`; missing file is `exit(1)`.
- `AutoKillSamples` has no `g_samples_ready` guard (its three siblings do).

## Extern-type divergences (caller-side levers; nothing shared edited)

- `g_speech_bytes_left` (0x007cacac) is `int` here; tinystubs.c declares it
  `unsigned int`. The `cmp edi,ecx / jle` in FillNarrationSourceRing is
  signed.
- `GetTicks` (0x00499450) declared `int`: `cmp edi,[esi+0x20] / jl` and
  `cmp esi,eax / jle` are signed. `Sample.due` is `int` here (audio5.c:
  `unsigned int`).
- `IDSBufferVtbl::GetCurrentPosition` / `GetVolume` / `Lock` out-params
  declared `int*` (play cursor compared signed against `fill << 12`).
- `PauseCurrentTrack` / `InitOptionSamples` are the defining files' names
  (audio4.c / tinystubs.c) for 0x00498920 / 0x00492830; goalstate.c calls
  them `PauseCurrentTrack` / `PauseAllSamples`.

## Levers (each measured on the named body)

- **A `while` whose condition carries a CALL is not loop-inverted; the
  duplicated latch copy must be in the source** (`PumpNarration`,
  0x00498b40). `while (GetCurrentPosition(..) != 0 || play < lo || play >= hi)
  { body }` compiles with ONE copy of the condition and the `return 1`
  exiled (122i vs 142i). The original is `if (caught_up) return 1;
  do { body } while (!caught_up);` — the 3-test condition written twice,
  which also settled the play/bytes frame slots. A `for(;;)`/`if … return 1`
  form and a `static __inline` predicate gave the same 20-short body.
- **Declaration order IS a register tie-break between two equal-weight
  webs** (`LoadStrings`, 0x00498d00). `buf` (loop pointer) and `size` (loop
  bound) both want the one free callee-saved register in the parse loop
  (ebx is forced to `c` for `bl`, ebp = pos, esi = digit index). Declared
  `char cwd[]; char str[]; char num[]; int size; int pos; …; char* buf;`
  VC6 gives `size` edi and reloads `buf` (34-line residual, `mov edx,[buf]`
  at every read); declared `char* buf; int size; int pos;` FIRST, `buf`
  gets edi and `size` is compared from its slot — exact. `register`,
  `= 0` initialisers on other locals, `unsigned char*`, a `len` copy,
  pointer-arithmetic spellings and while/for shapes of the main loop all
  left the swap in place. Refines the brief's "DECLARATION ORDER is
  irrelevant": true for slots, not for this tie.
- **Same tie-break, two definitions** (`NarrRingA_Available`, 0x00497fb0):
  `int cur = queue[0]; int avail = (b - a) & 0xffff;` puts the read cursor
  in edi (exact); `avail` first puts it in esi (6-line register residual).
- **A no-op re-assignment in the then-arm is a layout lever**
  (`LoadStrings`): `c = NEXT(); if (c != '"') quotes++;` exiles the
  ternary's `c = 0` arm (`xor bl,bl / jmp inc` after the epilogue, 3 extra
  lines); `if (c == '"') c = '"'; else quotes++;` — the dead store is
  folded by value propagation — lays `xor bl,bl` inline falling into
  `inc ecx`, with `jne inc / jmp skip` out of the load arm. Writing the
  two `quotes++` out in full (`if (pos < size) { c = buf[pos++]; if (c != '"')
  quotes++; } else { c = 0; quotes++; }`) gets the inline `xor` but the
  wrong polarity (`je skip / jmp inc`).
- **`char str[0xf0] = { 0 }`** gives `mov byte ptr [esp+0x20],0` plus a
  `rep stosd / stosw / stosb` fill of the remaining 239 bytes (explicit
  element as an immediate, remainder zero-filled); `= ""` loads the byte
  from a pooled literal (`mov al,[lit] / mov [esp+0x20],al`, +1 line).
- **Two full calls, cross-jumped** (`FillNarrationSourceRing`, 0x00498000):
  `_read(fd, p, space > left ? left : space)` emits one push set; the
  original's two complete push sequences joined at one `call` need
  `if (space > left) got = _read(.., left); else got = _read(.., space);`.
  The `mov bl,4 / je` quirk (byte immediate hoisted into bl for
  `test byte ptr [g_narr_flags],bl`, with a dead `je` re-using the outer
  test's flags) falls out of `space = FreeSpace(); while (space) { while
  (space) {…} space = FreeSpace(); }` — the inner `while (space)` entry test
  is the dead `je`.
- **Inner `while (space) { n = …; if (n == 0) break; … }` followed by
  `if (space)`** (`RefillNarrationRing`, 0x00498250): the break exit lands on
  `test ebx,ebx / je latch`, and the natural exit is threaded straight to
  the outer latch (`jmp`). `do { … } while (space)` and `for (;;)` with a
  second `if (space == 0) break;` both duplicated the `n` test into the
  latch and sank the `g_narr_e` load into the body (+2 lines).
- **Exiled `b < a` arm** (`NarrRingA_FreeSpace` / `NarrationFreeSpace`):
  the original tests `if (b >= a) { if (a == 0) …; return …; } return
  a-b-1;` — writing the `b < a` return first puts it inline (12-line layout
  residual).
- **Bucket index as a local** (`AddString`, 0x00498f80): `g_string_buckets
  [id % 10]` used twice CSEs the ADDRESS into `lea eax,[edx*4+buckets]`
  (+1 line); `int h = id % 10;` keeps two scaled-index operands.
- `((status == 0) & 1)` for `sete al / test al,1` (AutoKillSamples), as
  DECOMP records; no `xor eax,eax` needed — eax is the GetStatus zero.
- Argument order read off the prologue: `FadeOutAllSound(step, interval)` —
  `mov ebp,[esp+0x18]` after two pushes + `sub esp,8` is arg 2.
