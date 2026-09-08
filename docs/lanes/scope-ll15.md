# Scope LL15 — unreferenced (dead) functions, 0x00476450..0x0049c110

Branch `scope/LL15`. File `LEGOLAND/unref7.c` (new). Object prefix `/tmp/sll15_`.
Brief: `docs/SCOPE_LL15_unref_sim_profiles_screens.md`.

## Status

**44 / 44 exact.** `audit.py` PASS, `relocs.py` zero MISMATCH (1 UNRESOLVED:
`__chkstk`, a CRT symbol that carries no address annotation), `/W3` clean.

| address | name | insns | % | audit | marker |
| --- | --- | ---: | ---: | --- | --- |
| 0x00476450 | Unref_00476450 | 3 | 100 | [OK] | FUNCTION |
| 0x00477440 | MemScratch_TakeNode | 105 | 100 | [OK] | FUNCTION |
| 0x00477600 | MemScratch_Totals | 39 | 100 | [OK] | FUNCTION |
| 0x0047b500 | LLIDB_RemoveElement | 51 | 100 | [OK] | FUNCTION |
| 0x00481d70 | RenderPathSquareCursors | 63 | 100 | [OK] | FUNCTION |
| 0x004826f0 | FreePTPOpenNodes | 13 | 100 | [OK] | FUNCTION |
| 0x004830e0 | UnInitialiseBlokeExtras | 1 | 100 | [OK] | FUNCTION |
| 0x00483100 | UnInitialiseAllBlokes | 2 | 100 | [OK] | FUNCTION |
| 0x00483110 | FindBlokeSeatSlot | 12 | 100 | [OK] | FUNCTION |
| 0x00484940 | Unref_00484940 | 2 | 100 | [OK] | FUNCTION |
| 0x004855d0 | GetLayeredSpriteBounds | 84 | 100 | [OK] | FUNCTION |
| 0x00486180 | Unref_00486180 | 1 | 100 | [OK] | FUNCTION |
| 0x00486490 | MakeShadedPalette | 30 | 100 | [OK] | FUNCTION |
| 0x00486520 | ShadeLookupFixed | 6 | 100 | [OK] | FUNCTION |
| 0x00488730 | SampleTexturePixel | 39 | 100 | [OK] | FUNCTION |
| 0x0048b690 | OpenIconListLog | 9 | 100 | [OK] | FUNCTION |
| 0x0048cc90 | ProfileAge7Input | 18 | 100 | [OK] | FUNCTION |
| 0x0048ccd0 | ProfileAge9Input | 18 | 100 | [OK] | FUNCTION |
| 0x0048cd10 | ProfileAge11Input | 18 | 100 | [OK] | FUNCTION |
| 0x0048ef10 | OptionsAcceptInput | 13 | 100 | [OK] | FUNCTION |
| 0x0048ef40 | OptionsCancelInput | 18 | 100 | [OK] | FUNCTION |
| 0x0048f4f0 | ExitPopupCloseInput | 17 | 100 | [OK] | FUNCTION |
| 0x0048f5d0 | ClickOnlyInput | 11 | 100 | [OK] | FUNCTION |
| 0x0048fbd0 | GotoOptionsInput | 12 | 100 | [OK] | FUNCTION |
| 0x0048fe70 | TitleNewGameInput | 16 | 100 | [OK] | FUNCTION |
| 0x004917c0 | SaveProfileSlotToDisk | 108 | 100 | [OK] | FUNCTION |
| 0x004919a0 | FindProfileNodeBySlot | 11 | 100 | [OK] | FUNCTION |
| 0x00491b80 | RescanProfiles | 16 | 100 | [OK] | FUNCTION |
| 0x00491f90 | ProfileAgeSpinInput | 28 | 100 | [OK] | FUNCTION |
| 0x00491fe0 | NewProfileOkInput | 57 | 100 | [OK] | FUNCTION |
| 0x00495f00 | LoadNamedMusicSegment | 73 | 100 | [OK] | FUNCTION |
| 0x00496010 | GetMusicChordMap | 39 | 100 | [OK] | FUNCTION |
| 0x004975a0 | Unref_004975a0 | 7 | 100 | [OK] | FUNCTION |
| 0x00497e40 | ShowAllLayers | 21 | 100 | [OK] | FUNCTION |
| 0x00499040 | SplitPathAndBase | 61 | 100 | [OK] | FUNCTION |
| 0x004990c0 | AppendExtensionIfNone | 45 | 100 | [OK] | FUNCTION |
| 0x00499120 | ReplaceExtension | 46 | 100 | [OK] | FUNCTION |
| 0x00499190 | PrefixPathIfBare | 76 | 100 | [OK] | FUNCTION |
| 0x00499240 | PrependPathPrefix | 80 | 100 | [OK] | FUNCTION |
| 0x00499340 | DowncaseString | 23 | 100 | [OK] | FUNCTION |
| 0x00499490 | FormatMilliseconds | 42 | 100 | [OK] | FUNCTION |
| 0x00499ca0 | FindFreeMechanic | 36 | 100 | [OK] | FUNCTION |
| 0x0049c0f0 | CountListLinks | 9 | 100 | [OK] | FUNCTION |
| 0x0049c110 | StepListNodes | 17 | 100 | [OK] | FUNCTION |

## Names

Three bodies gave no behavioural clue at all and keep `Unref_<VA>`:

- `Unref_00476450` — stores its int argument into **0x00668fa8**, a movie-TU
  global whose ONLY reference anywhere in the image is that store (the byte
  scan finds one hit). It sits between `g_movie_audio_scale` (0x00668fa4) and
  the movie clock-mode latch (0x00668fac).
- `Unref_00484940` — `return 0;` (bnvpath.c neighbourhood).
- `Unref_00486180` — an empty `void f(void)` (tri3d.c neighbourhood).
- `Unref_004975a0` — walks the whole sprite list (`g_sprites_head`) and does
  nothing with it. Named for that, not for a purpose.

Named from body evidence:

- `MemScratch_TakeNode` / `MemScratch_Totals` — the missing halves of the
  memory module coaster.c already holds (`MemScratchInit`, `FreeMemScratch`,
  `MemScratch_Noop`). The 0x00668fb8 block is `{count, freelist, used}`
  followed by 0x8c-byte nodes chained through each node's +0x00.
- `LLIDB_RemoveElement` — the delete half of `LLIDB_RegisterNewElement`.
- `RenderPathSquareCursors` — a build cursor per path square plus one over
  the suggest target.
- `FreePTPOpenNodes` — `FreePTPOpenList` (0x004821e0) minus the head clear.
- `UnInitialiseBlokeExtras` / `UnInitialiseAllBlokes` — an empty stub and the
  wrapper that calls it before tail-jumping into `UnInitialiseBlokes`.
- `FindBlokeSeatSlot` — the inverse of `RenderBlokesNotInSeats`' walk.
- `GetLayeredSpriteBounds` — bounding box over a layered sprite's layers.
- `MakeShadedPalette` / `ShadeLookupFixed` — a 256-entry ramp table over a
  palette, and the 16.16 twin of `ShadeLookup` (0x004864f0, which takes a
  float).
- `SampleTexturePixel` — see "hand-written assembly" below.
- `OpenIconListLog` — opens "IconList.txt"; both its globals and the literal
  are referenced from nowhere else.
- `ProfileAge7Input` / `ProfileAge9Input` / `ProfileAge11Input` /
  `ProfileAgeSpinInput` — the register screen's age buttons. `Profile.f20`
  (+0x20) is the AGE: three buttons stamp 7/9/11 into the named profile's
  record and write it back, and the spinner clamps `g_temp_profile.age` to
  1..100. (fpui-side files already call the live copy `g_profile_age`.)
- `SaveProfileSlotToDisk` / `FindProfileNodeBySlot` — write one LIST node's
  profile back to "profiles\Profile%d.txt"; the twin of `SaveProfileToDisk`
  (0x00491910), which writes `g_temp_profile` for the CURRENT slot.
- `RescanProfiles` — `ScanForProfiles` + `LoadProfilesFormDisk`, complaining
  on screen when the profile directory is unusable.
- `NewProfileOkInput` — the new-profile popup's OK button.
- `LoadNamedMusicSegment` / `GetMusicChordMap` — two more of music.c's
  DirectMusic template wrappers (`IDirectMusicStyle::GetChordMap` is the
  vtable slot at +0x28).
- `SplitPathAndBase`, `AppendExtensionIfNone`, `ReplaceExtension`,
  `PrefixPathIfBare`, `PrependPathPrefix` — five path-string helpers.
- `DowncaseString` — instruction-for-instruction `UpcaseString` (0x00499300)
  with `tolower`.
- `FormatMilliseconds` — "%02d:%02d:%02d.%03d" into the shared buffer.
- `FindFreeMechanic` — the mechanic twin of `FindFreeGardener` (0x00499c40);
  workorder2.c's header already predicted the address and the 0x11 plan.
- `CountListLinks` / `StepListNodes` — savechunks.c list arithmetic.
- `ShowAllLayers` — `ShowLayer` (0x00497e10) across every layer.

## Globals named for the first time

| address | name | evidence |
| --- | --- | --- |
| 0x00668fa8 | `g_movie_fa8` | written by `Unref_00476450`; no reader in the image |
| 0x00798654 | `g_iconlist_log` | the FILE* "IconList.txt" is opened into |
| 0x00798658 | `g_iconlist_log_open` | the flag guarding that open; never set |
| 0x004beb60 | `g_name_iconlist` | "IconList.txt" |
| 0x0079a878 | `g_time_text` | `FormatMilliseconds`' sprintf destination (it starts immediately after the string table `DeleteStrings` clears up to 0x0079a878) |
| 0x004bff0c | `g_fmt_hmsms` | "%02d:%02d:%02d.%03d" |
| 0x007cad80 | `g_temp_profile.age` | clamped 1..100 by the spinner (Profile +0x20) |

## Mechanics recovered (spec for a runtime)

- **The 0x00668fb8 scratch pool.** Header `{count, freelist, used}` then
  `count` nodes of 0x8c bytes, each chained through its own +0x00.
  `MemScratch_TakeNode` pops the free list onto the live list; when the free
  list is empty it `realloc`s to `(count + 0x400) * 0x8c + 0xc`, adds the
  pointer delta to every link of the live chain (the block may have moved),
  zeroes the 0x400 new nodes and threads them into a fresh free list — the
  LAST of the 0x400 keeps its zeroed link, so the chain terminates.
  `MemScratch_Totals` sums each live node's +0x08 word and counts them.
  Neither ever updates `count`, so a second grow re-adds from the same base:
  an original bug, reproduced.
- **The LLIDB table is paged.** 256 `LLElem` (20 bytes each) per page,
  `g_llidb_pages[i >> 8][i & 0xff]`. `LLIDB_RemoveElement` closes the gap by
  shifting each page's tail down one slot and carrying the NEXT page's first
  element up into the previous page's slot 255. `slot == -1` is the marker the
  outer loop leaves to mean "shift this page from 0"; `slot >= 0xfe` means
  there is nothing left to move inside it.
- **Profile ages.** `Profile +0x20` is the player's age: the register screen
  offers 7 / 9 / 11 as direct buttons (each writes the LIST node's copy and
  saves that profile's file, not the live one) and a spinner that clamps the
  TEMP profile's age to 1..100. `NewProfileOkInput` fills an empty name with
  `sprintf("%s%d", GetString(0x8d), slot)` and parks the length in
  `name[0x1e]` — the last usable byte of the 0x20-byte name field doubles as
  the on-screen keyboard's caret.
- **`SaveProfileSlotToDisk` rebuilds the record field by field** over a zeroed
  local rather than assigning it whole, so `Profile.f24` (+0x24) and the three
  pad bytes after it always go to disk as zero.
- **`GetLayeredSpriteBounds`** reads `obj->flags` BEFORE its own `obj != 0`
  test — an original bug, reproduced — and halves each layer's render offset
  TOWARD ZERO with an explicit `-((-v) >> 1)` for negatives, not an arithmetic
  shift.
- **`SampleTexturePixel`** reads the texture descriptor's +0x00/+0x04 as
  extents (multiplied by the 16.16 fraction) where the rasterisers read them
  as log2 shifts; +0x08 is the row shift either way. shade >> 10 selects the
  64-level ramp entry, with shade == 1.0 exactly (bit 16) folded back onto
  level 63 by a borrow.
- **`FreePTPOpenNodes`** is `FreePTPOpenList` without the trailing
  `g_ptp_open_head = 0`, so it leaves the head dangling. Reproduced.

## Levers (with evidence)

- **A char result keeps its `movsx` only under TWO independent guards.**
  `RescanProfiles`: `if (rc == -1) print; if (rc != -1) { load; }` compiles to
  `movsx eax,al / cmp eax,-1 / jne`; VC6 threads the second test away but the
  widening survives. Every single-guard spelling — `if (F() == -1)`,
  `int rc = F(); if (rc == -1)`, `(unsigned)F() == 0xffffffff`, a one-case
  `switch`, an if/else — narrows to `cmp al,-1`, and the switch also flips the
  block order. (`if/else` on the same value narrows too: it is the SECOND
  independent reference that keeps the widened value alive.)
- **Two lockstep cursors must both be INDEXED for VC6 to eliminate one.**
  `MakeShadedPalette`: `tab[i] = f(levels, &pal[i])` gives the original's
  `sub edi,eax` / `lea eax,[edi+esi]` single-IV form. `*d++ = f(levels, pal++)`
  (and the `unsigned char*` + `pal += 4` variant) keeps both cursors live and
  emits `push edi` / `add edi,4` (86.2%).
- **Reuse the SCAN variable as the copy loops' counter.** `SplitPathAndBase`
  went 43%→100% when `i` (the backwards scan index) became the counter of both
  copy loops with a separate saved `cut`: that is what forces the callee-saved
  save of the scan result (`mov ebp,eax`) and the fourth push. A distinct `j`
  leaves `i` in eax and loses the push entirely.
- **`dot` must take the strlen register.** `int dot = strlen(s); int i = dot;`
  gives `mov eax,ecx` (i copied out of the strlen register); the reverse
  spelling copies `dot` instead and rotates the whole body.
- **Two plain `return 2` guards fold to `test/setne/inc`.**
  `ProfileAgeSpinInput`: `if (b & 1) return 2; if (b & 4) return 2; return 1;`
  emits `test al,4 / setne al / inc eax`. Every arithmetic spelling of the same
  value — `((b & 4) != 0) + 1`, a `char` temp, a ternary — lowers to
  `shr eax,2 / and eax,1 / inc eax` or `neg/sbb/neg` instead.
- **Source-order of two independent extractions decides which is done in
  place.** `LLIDB_RemoveElement`: `page = idx >> 8; slot = idx & 0xff;` gives
  the original's `mov edx,eax / and eax,0xff` (slot in the loaded register);
  the reverse order loads into edx and copies to eax (92.2%).
- **Exile a leading `return K` by making it the function's LAST statement.**
  `if (idx < count) { body; return 0; } return -3;` puts the -3 block after the
  body, as the original does; `if (idx >= count) return -3;` inlines it.
- **The non-layered arm has to come FIRST in the source.**
  `GetLayeredSpriteBounds`: `if (!(flags & 0x8000)) { small arm; return; }
  big arm;` — writing the big arm as the `if` body puts the wrong block
  inline (76.2%).
- **A sub-object `memset` is what gives a function its SECOND zero register.**
  `RenderPathSquareCursors`: five explicit `cur.rect.<field> = 0` stores reuse
  the loop's ebp zero; `memset(&cur.rect, 0, sizeof(cur.rect))` unrolls from a
  fresh `xor eax,eax` exactly as the original does.
- **Read a list head BEFORE the call that precedes the loop.**
  Same function: `sq = g_path_squares; DefaultCursor(&cur);` puts the load in
  the prologue; `for (sq = g_path_squares; ...)` after the call does not.
- **`#pragma optimize("", off)` is still the memory module's signature.**
  0x00477440 and 0x00477600 join `MemScratchInit` (schoolcar.c) and the three
  coaster.c wrappers: full ebp frame, every local in memory, no CSE of the
  global. The C reconstructs instruction-for-instruction under the pragma.
- **NEW: at /Od, VC6 assigns ebp slots by a hash of the local's NAME.**
  Neither declaration order nor first-use order moves them — all six
  permutations of `MemScratch_Totals`' three declarations give byte-identical
  offsets, and swapping the two initialiser statements only reorders the
  stores. Renaming `total`/`n` to `aaa`/`zzz` swapped the slots. The lever is
  therefore: probe the name set with a throw-away `#pragma optimize("",off)`
  function compiled `/FAcs`, read `mov DWORD PTR _name$[ebp]`'s modrm byte, and
  pick names whose slots match. `MemScratch_Totals` needed
  `amount / node / nodes`; `MemScratch_TakeNode` needed
  `idx / front / was / larger / march / skew` (found by searching ~1700 name
  sets against the probe — the plain `i / base / t / p / q / delta` and
  `index / nodes / prev / grown / chain / adjust` sets are both wrong).
- **`shrd` is proof of hand-written assembly in this tree.** VC6 lowers EVERY C
  64-bit shift by a constant through `__aullshr` / `__allshr` — inline `shrd`
  is not reachable from C. 0x00488730 has two of them, plus an EBP frame in an
  /O2 file, a `mul` (not `imul`) 32x32->64, bit 16 tested straight into the
  carry flag (`shr ecx,11h / sbb eax,0` — the only such site in the image
  outside the CRT; ten C spellings of `x - ((x >> 16) & 1)` all give
  `shr/and/sub`), and a result parked in a dword stack slot the epilogue reads
  back as a WORD. Written as one `__asm` block storing into an
  `unsigned int` local with `return (unsigned short)px;`, it matches exactly —
  and VC6 saves only `ebx` for it, so the "asm forces a full ebx/esi/edi save"
  note is about asm that *writes* all three.
- **`#pragma optimize("y", off)` reproduces an /O2 ebp frame** and was the
  first hypothesis for 0x00488730 (it does give `push ebp / mov ebp,esp` with
  otherwise full optimisation); it was not needed once the body became `__asm`.
- **A dword store read back as a word needs `volatile`, not a union.**
  Measured while triaging 0x00488730: `union { unsigned int i; unsigned short w; }`,
  a two-element `unsigned short` array, a `struct` type-pun and
  `*(unsigned short*)&px` are ALL folded away at /O2; only
  `volatile unsigned int px` keeps `mov DWORD PTR px,ecx / mov ax,WORD PTR px`.
  (The final body uses `__asm` instead, which forces the same slot.)
- **A one-instruction tail-call wrapper still audits.** `UnInitialiseAllBlokes`
  (`call` + `jmp`) is bounded by `audit.py` and needs no WIP marker.

## Extern-type notes

- `ScanForProfiles` is declared `char ScanForProfiles(void)` here (profiles.c
  defines it that way). screens3.c declares it `void` because that file only
  tests its ADDRESS.
- `UpDateCurrentProfile` is `void` here, as in screens3.c.
- `MakeShadedColour` is `Shade*` here (texture.c's spelling); person3d.c
  declares it `int`.
- `MemFree` is used for the PTP node free (0x0049e4d0) — pathmisc.c calls the
  same address `HeapFree_w`.
- `PrintCentColref` matches text.c exactly.
- `IDMStyle`'s vtable is declared out to +0x28 here (GetChordMap); music.c
  stops at +0x0c.
- The texture descriptor is declared with `unsigned int` extents at +0x00/+0x04
  (`TexDesc`), which is NOT how tri3d.c reads the same global (`ushift`/
  `vshift`). Only 0x00488730 uses it this way; noted, not "aligned".
