# Scope K — `LEGOLAND/movie.c` (movie player + front-end teardown)

**Result: 16 of 16 exact.** `python3 tools/audit.py LEGOLAND/movie.c` ends
`PASS: 0 function(s) failed the extent gate`; `/W3 /O2 /Gy /Gd` compiles with
no diagnostics. Object prefix `/tmp/sk_movie_`. Nothing outside
`LEGOLAND/movie.c` and this file was created or edited.

| address | name | insns | bytes | audit | marker committed |
| --- | --- | --- | --- | --- | --- |
| 0x00475fe0 | `SetMenuHelp` | 8 | 25 | [OK] | `// FUNCTION: LEGOLAND 0x00475fe0` |
| 0x0048ffb0 | `KillAdvertScreenSprites` | 11 | 37 | [OK] | `// FUNCTION: LEGOLAND 0x0048ffb0` |
| 0x00473130 | `CloseInfoPopUpIfOpen` | 13 | 34 | [OK] | `// FUNCTION: LEGOLAND 0x00473130` |
| 0x00458940 | `EnterParkPlayMode` (was `sub_458940`) | 16 | 81 | [OK] | `// FUNCTION: LEGOLAND 0x00458940` |
| 0x00473160 | `ClosePrimaryPopUp` | 18 | 50 | [OK] | `// FUNCTION: LEGOLAND 0x00473160` |
| 0x00490270 | `KillCertScreenSprites` | 18 | 65 | [OK] | `// FUNCTION: LEGOLAND 0x00490270` |
| 0x0047afb0 | `LoadLevelDatabase` | 20 | 58 | [OK] | `// FUNCTION: LEGOLAND 0x0047afb0` |
| 0x0048fa40 | `RestoreFrontEndState` | 20 | 83 | [OK] | `// FUNCTION: LEGOLAND 0x0048fa40` |
| 0x004907a0 | `LoadHelpTextFor` | 26 | 94 | [OK] | `// FUNCTION: LEGOLAND 0x004907a0` |
| 0x0048ab60 | `UnlockSidePanelObjects` (was `sub_48ab60`) | 27 | 71 | [OK] | `// FUNCTION: LEGOLAND 0x0048ab60` |
| 0x00476630 | `CloseMovie` | 27 | 73 | [OK] | `// FUNCTION: LEGOLAND 0x00476630` |
| 0x004911c0 | `SetInfoPanelText` | 32 | 119 | [OK] | `// FUNCTION: LEGOLAND 0x004911c0` |
| 0x00490fa0 | `PrintCursor` | 84 | 224 | [OK] | `// FUNCTION: LEGOLAND 0x00490fa0` |
| 0x004989b0 | `RewindNarrationBuffer` | 98 | 324 | [OK] | `// FUNCTION: LEGOLAND 0x004989b0` |
| 0x00476460 | `OpenMovie` | 142 | 454 | [OK] | `// FUNCTION: LEGOLAND 0x00476460` |
| 0x004766f0 | `RunMovie` | 170 | 531 | [OK] | `// FUNCTION: LEGOLAND 0x004766f0` |

All sixteen end in a real `ret` and none is recursive, so all sixteen are
promotable and are committed as `// FUNCTION:`. `CloseMovie` contains an
internal tail `jmp` to `AVIFileExit`, but the `jne` two instructions earlier
crosses it, so the extent walker bounds it correctly at the following `ret`.

## Names settled

- `sub_458940` -> **`EnterParkPlayMode`**: clears `g_edit_changed`, sets
  `g_game_mode = 3` (a running park), installs the in-game icon handlers,
  drops `g_edit_object`, resets the edit cursor and clears the UI's two
  "front-end screen" bits while setting 0x20. Callers `StartFreePlayPark`
  (uimisc2.c) and 0x00458acf; both spell it `sub_458940` — left alone.
- `sub_48ab60` -> **`UnlockSidePanelObjects`**: walks the side-panel icon list
  and, for every icon of kind 1 whose named object class loads, marks that
  class available in the build menu. `progress_tick()` per icon drives the
  loading bar.

## First-named callees and globals

| address | name given | what it is |
| --- | --- | --- |
| 0x00492980 | `SetSfxPaused` | 2i — `g_sfx_paused = 1` (audio3.c names the global) |
| 0x00492990 | `ClearSfxPaused` | 2i — `g_sfx_paused = 0` |
| 0x004784c0 | `ResetLevelGlobals` | ~60i — clears the per-level state block before a load |
| 0x004781f0 | `ParseKeywordFile` | ~30i — opens a resource text file and dispatches its `[SECTION]` keywords through a table |
| 0x00490740 | `SetHelpTextPrefix` | 13i — `_splitpath` the key, `sprintf(0x7cae80, "%s_", fname)` |
| 0x00490680 | `LoadTextFileLines` | ~90i — reads a resource text file into a `char*` table, returns the line count |
| 0x00490800 | `LoadHintTextFor` | 26i — `LoadHelpTextFor`'s twin for the hint table (0x00490770 / 0x00490880 / cap 0x20) |
| 0x004983a0 | `ReadDecodedNarration` | ~35i — drains up to n bytes of decoded PCM out of the 0x20000-byte ring at 0x007aaca0 |
| 0x00478b20 | `EnsureObjectClassLoaded` | ~18i — `ElemID` + `LoadObjectClass`, sets flag 4, returns 1 on success |
| 0x00469900 | `MarkElemAvailable` | ~31i — flips an element's +0x08 flags into the build menu |
| 0x00476910 | `StartMovieAudio` | large — opens a KLIBAUDIO buffer for `mv->audio`; non-zero when there is one |
| 0x00476bf0 | `PrimeMovieAudio` | ~40i — first fill, at the first displayed frame |
| 0x00476d20 | `UpdateMovieAudio` | large — per-frame-step top-up, takes (frame, prev) |
| 0x00476c90 | `StopMovieAudio` | ~35i — teardown |
| 0x00476680 | `MovieTicks` | 20i — ms clock: QueryPerformanceCounter where available, GetTickCount otherwise; the mode is latched in 0x00668fac |
| 0x00465850 | `BlitDIBToScreen` | large — blits a BITMAPINFOHEADER+bits frame |

Globals named here for the first time: `g_menu_help[4]` (0x004bb18c),
`g_avi_open_count` (0x00668f98), `g_movie_bmi` (0x004bb4e0, a
BITMAPINFOHEADER), `g_movie_audio_scale` (0x00668fa4),
`g_speech_fill_block` (0x0079a840), `g_speech_blocks_ready` (0x0079a844),
`g_level_db_sections` (0x004bb6f8, 93 {keyword, handler} pairs).
`g_key_state[0x9d] | g_key_state[0x1d]` is "either Ctrl" and `[0x10]` is
DIK_Q, which is how the attract-mode movie is aborted; `[0x39]` (DIK_SPACE)
aborts the in-game one.

## Original bugs reproduced (commented at the site)

- **`RestoreFrontEndState` (0x0048fa40).** `g_front.mode` is latched at entry
  and written back over `g_front.screen` AFTER the whole saved block has been
  copied in, so the SAVED screen index is discarded and the screen index
  becomes whatever sub-mode was current when the pop ran. The saved sub-mode
  is then restored correctly, so the pair ends up mismatched.
- **`CloseMovie` (0x00476630).** The two AVI streams and the GETFRAME handle
  are released but `mv->file` never is — one `PAVIFILE` leaks per movie
  played. `g_avi_open_count` is still decremented and `AVIFileExit()` called
  at zero, so the leak is only reclaimed by AVIFile's own teardown.
- **`RunMovie` (0x004766f0).** On the last pass of a movie the prefetch is
  skipped (`cur + 1 >= mv->frames`) but `frame` is still set to `cur + 1`,
  leaving a stale `next`. Harmless only because the loop exits on the same
  pass.
- **`OpenMovie` (0x00476460).** The stream walk keeps the LAST `vids` and the
  LAST `auds` stream it sees, overwriting (and leaking a reference on) any
  earlier one; and the result of `AVIFileInfoA` is never checked —
  `fi.dwStreams = 0;` before the call is the only guard.

## Extern-type divergences (noted, not "aligned")

- `LoadLevelDatabase` is defined here as `int` returning 0 or 2. `uimisc2.c`
  declares it `void* LoadLevelDatabase(const char*)` and stores the result in
  `g_freeplay_db` — so the "database pointer" is in fact a 0/2 status code.
  Left alone.
- `uimisc2.c` declares `int RunMovie(void*, WinRect*, int)` and
  `void* OpenMovie(const char*)` on an opaque handle; this file gives them the
  real `Movie*` (0x28 bytes). `PrintCursor` is declared `void` in both
  screens2.c and uimisc2.c and defined `void` here — no divergence.
- `PU_CloseInput` / `PU_NextInput` are declared here with FOUR parameters
  (`Icon*, int, short, short`); misc3.c has a two-parameter spelling of
  `PU_NextInput`. Both are cdecl, so the divergence is invisible at the call.
- `sub_458940` / `sub_48ab60` keep their placeholder names in `uimisc2.c`; the
  real names live only here.

## Levers, with evidence

- **Two globals that are copied as a unit must be ONE struct, and a 12-byte
  struct assignment is what STOPS dead-store elimination.**
  `RestoreFrontEndState` writes `g_cur_screen` twice, the second write
  clobbering the first. Written as six scalar assignments VC6 deletes the
  first store (17 instructions for the original's 20) — a plain global store
  followed by another to the same global is dead. Written as
  `g_edit = *game; g_front = *screen; g_front.screen = mode;` — two 12-byte
  struct assignments over the contiguous triples at 0x008119b0 and 0x0080ff80
  — the copy is lowered too late for DSE to see through it, all three stores
  survive, the bug store schedules into the middle of the copy, and the latch
  of `g_front.mode` is hoisted to the top of the function because it must
  precede the copy that overwrites it. 20/20 exact, first try in that form.
  (Nine other statement orders of the scalar spelling floored at 20 mismatch.)
- **`cond ? 0 : K` is `setcc / dec / and K`; the polarity of the condition is
  the whole residual.** `LoadLevelDatabase`: `(rc >= 0) ? 0 : 2` gives
  `setl / and al,0xfe / add eax,2` (3 wrong, 59B for 58B);
  `(rc < 0) ? 2 : 0`, `((rc >= 0) - 1) & 2` and
  `if (rc < 0) return 2; return 0;` are all byte-identical to the original
  `setge / dec / and eax,2`. `(rc >> 31) & 2` is a different lowering
  (`sar/and`, 4 wrong).
- **Two leading guards that return the same constant merge into ONE trailing
  block only when they are ONE `if`.** `RewindNarrationBuffer`: two separate
  `if (...) return 0;` give two inline epilogues (`add esp,0x1008 / ret`
  twice, 8 wrong); `goto` on both merges only ONE of them (the second still
  gets an inline copy). All four of
  `if (a == 2 || a == 0) return 0;`, `if (a != 2 && a != 0) { ...; return 1; }
  return 0;`, `if (a == 2 || !a) goto idle;` and the nested-`if` form give the
  original's single trailing `xor eax,eax / add esp,0x1008 / ret`. The `||`
  spelling was kept.
- **A shared failure tail is cross-jumped only when there are THREE textual
  copies, not one shared `goto` and not two.** `OpenMovie` has three failure
  arms (file will not open / no video stream / allocation failed), all ending
  `AVIFileRelease; if (!g_avi_open_count) AVIFileExit(); return 0;`. Written
  as one copy plus two `goto`s, the early arm is exiled with no compare of its
  own (125 mismatch); written as two copies, the early arm keeps a full
  10-instruction inline epilogue (115). Written out THREE times, VC6 merges
  the suffixes exactly as the original does — the early arm keeps only its own
  `cmp [g_avi_open_count], ebx` and jumps INTO the later copy's `jne`,
  borrowing its own flags, and the two release arms merge at the shared
  `call AVIFileRelease` with the `push` duplicated in each. That single change
  took `OpenMovie` from 115 mismatches to 74.
- **A store to a global kills the CSE of a load through a pointer.** In both
  `OpenMovie` and `RunMovie` the width/height are read once into locals and
  the product formed from the locals; reading `mv->width`/`mv->height` again
  inside the product costs two extra loads, because the intervening stores to
  `g_movie_bmi` may alias `*mv`. Same rule as DECOMP's "a store to ANY global
  kills CSE of an unrelated global load", seen from the pointer side.
- **The assignment ORDER inside a chained assignment decides everything.**
  `RunMovie`'s frame fetch is `frame = (int)(next = AVIStreamGetFrame(...))`
  (next assigned first). Spelled the other way round,
  `next = (void*)(frame = (int)AVIStreamGetFrame(...))`, VC6 rotates the loop,
  reloads `frame` in a new preheader and spills it — 123 mismatch against 6.
  Two spellings of the same C, 117 instructions apart.
- **An overloaded sentinel variable is what wins the fourth callee-saved
  register.** `RunMovie` has four loop-carried values and four callee-saved
  registers. With one `void* frame` variable, `shown` takes ebx and `frame`
  goes to the frame (123 mismatch, every one of them an allocation
  consequence; the instruction sequence was otherwise identical). Splitting it
  into `int frame` (a -1 sentinel that also carries the prefetched frame's
  INDEX) plus `void* next` (the pointer, homed in the dead `mv` argument slot)
  puts `frame` in ebx and `shown` in the frame — the original's allocation.
  Making `shown` `volatile` also moves `frame` into ebx (108) but costs an
  extra load at every read; the split is free.
- **DECLARATION ORDER decides the callee-saved RANKING when several locals are
  initialised at entry** — a real exception to "declaration order is
  irrelevant", which was measured on spill SLOTS. All 24 orders of
  `{frame, cur, start, shown}` in `RunMovie` were compiled: six give the
  original exactly (every order that puts `frame` first and `start` not
  second, plus `shown, frame, cur, start`), six cost 2, five cost 5 and seven
  cost 6. The cost is always the same thing: which zero-valued initialiser
  becomes the shared zero register the two entry guards compare against
  (`cmp edi, ebp` against `test edi, edi`).
- **The statement order of four independent assignments decides which one is
  spilled.** `OpenMovie`'s `vids` arm assigns `frames`, `fps`, `w`, `h`; the
  original keeps `frames`, `w` and `h` in ebp/esi/edi and spills `fps`. All 24
  orders were compiled: `frames` first gives 2 mismatch, `fps` first gives 74.
  All four uninitialised locals share ONE frame home (the loop preheader loads
  ebp, esi and edi from the same slot), which is the documented
  "uninitialised locals coalesce into one home" pattern seen three ways at
  once.
- **`mv->file = pfile;` must be written BEFORE `mv->getframe = 0;`.** The
  emitted store order is the same either way (VC6 reorders adjacent stores)
  but the LOAD of `pfile` moves: written after, the load lands between the two
  stores instead of before them (2 mismatch). Eight fill orders measured.
- **A `for` loop over a GLOBAL counter keeps its zero-trip guard** because the
  intervening COM call may write the global: `RewindNarrationBuffer`'s
  `g_speech_fill_block = 0; Lock(...); for (; g_speech_fill_block < 10; ...)`
  emits `cmp dword ptr [mem], 0xa / jge` before the loop and reloads the
  counter three times per pass. The two callee-saved pushes for the inlined
  `memcpy`/`memset` sink past that guard on their own.
- **`PrintCursor` is `NewPrintCent` (text.c, 0x00491d60) with the DrawText
  flags changed from 0x25 to 0x24** — index for index the same body, 84/84 on
  the first compile. The twin is real: DT_CENTER dropped, DT_VCENTER |
  DT_SINGLELINE kept, so the text is left-aligned in the caller's box. VC6
  hoists `__imp__SelectObject` into esi for its three calls with no construct,
  and homes `oldfont` in the dead `white` argument slot.
- **AVIFile and ACM entry points are declared WITHOUT `__declspec(dllimport)`**
  so the compiler emits `call <thunk>`; `__declspec(dllimport)` would give
  `call dword ptr [__imp__...]`, which is what the GDI calls in `PrintCursor`
  need. Both spellings appear in this one file, three lines apart.

## Mechanics recovered (runtime spec)

- **The movie player is Video for Windows' AVIFile API**, not MCI or
  DirectShow. `OpenMovie` = `AVIFileInit` (once, refcounted in
  `g_avi_open_count`) + `AVIFileOpenA` + `AVIFileInfoA` + a linear walk of
  `dwStreams` calling `AVIFileGetStream` / `AVIStreamInfoA`. The frame rate is
  the INTEGER quotient `dwRate / dwScale`, so a 29.97 Hz file plays at 29; the
  pixel size is `rcFrame` right-left / bottom-top, and the length is the video
  stream's `dwLength` in frames. Both kept streams are `AVIStreamAddRef`'d on
  top of the reference `AVIFileGetStream` already returned, which is what lets
  `CloseMovie` release them once each.
- **Decompression is to 16-bit RGB at the movie's native size**: the
  `BITMAPINFOHEADER` at 0x004bb4e0 gets `biBitCount = 16`,
  `biWidth`/`biHeight` from the movie and `biSizeImage = h * w * 2`, and that
  is handed to `AVIStreamGetFrameOpen`. `RunMovie`'s `dst` rectangle is never
  read, so uimisc2.c's fixed {0,0,320,240} is decoration — the frame lands
  wherever `BlitDIBToScreen` puts it.
- **Playback is clock-driven and drops frames.** Every pass recomputes
  `frame = (now - start) * fps / 1000` from `MovieTicks()` (QPC where the
  machine has one, GetTickCount otherwise) and jumps straight to that frame;
  the clock starts at the FIRST displayed frame, not at open, and the audio is
  primed at the same moment. When the clock has not moved on, the next frame
  is decompressed ahead of time; when it has, the prefetch is discarded. The
  catch-up wait is a spin on the clock, not a sleep, so the player pegs the
  CPU while ahead of schedule.
- **Two abort regimes.** With `flags` non-zero (the in-game path) the movie
  stops on a lost message pump, mouse-button bit 0, any of the three mouse
  buttons — which also latches `g_movie_shown` so the movie is not offered
  again — or DIK_SPACE. With `flags` zero (the attract path) only Ctrl+Q
  stops it. Either way the player then spins on
  `ProcessSystemEvents`/`ReadGameButtons` until the mouse-button bits 1 and 2
  are clear, so the aborting click is not delivered to the screen underneath.
- **The narration ring is ten 0x1000-byte blocks (0xa000 bytes) in ONE
  DirectSound buffer.** `RewindNarrationBuffer` stops the buffer, rewinds to
  0, locks the whole thing as a single region and fills it block by block
  through a 0x1000-byte stack staging buffer, zero-padding the short final
  block to silence; `g_speech_blocks_ready` counts the blocks that carried any
  data. State 2 means "wound back and ready", 3 means playing; the function is
  a no-op in state 2 and in state 0 (nothing loaded).
- **The report/help text pipeline.** `LoadHelpTextFor(key)` sets a string-id
  prefix from the key's file name (`"<fname>_"`), frees the previous buffer,
  and loads `"Intervals\\<key>"` into the 0x64-entry table at 0x007cafa0. The
  stored line count is one LESS than the number of lines read whenever any
  were read — the first line of the file is a header — and that is also the
  result. `SetInfoPanelText(a, b)` loads the briefing text and, when there is
  one, the hint text; on success it puts the game in screen mode 7 with
  `g_cur_screen = -1` and un-greys the briefing icon, and on failure it greys
  it (flag 0x400) and changes nothing else.
- **Front-end state is pushed and popped as three blocks** at 0x007cb30c
  (the icon mode), 0x007cb300 (`{popup, screen, screen_mode}` = the triple at
  0x0080ff80) and 0x007cb2f0 (`{edit_changed, game_mode, edit_object}` = the
  triple at 0x008119b0). `PlayTitleMovie` (0x0048f9f0) takes the same three
  and is the push side.

## What is left in this file's neighbourhood

Nothing from the assigned table. The first-named callees above are all still
unmatched and are the natural next tier: `StartMovieAudio` (0x00476910),
`UpdateMovieAudio` (0x00476d20), `StopMovieAudio` (0x00476c90) and
`PrimeMovieAudio` (0x00476bf0) form one cluster (the movie's own audio
streaming, ~500 instructions, sharing the globals 0x00668ee0..0x00668fa4);
`LoadTextFileLines` (0x00490680), `LoadHintTextFor` (0x00490800),
`SetHelpTextPrefix` (0x00490740) and 0x00490770 form the report-text cluster;
`MovieTicks` (0x00476680) is 20 instructions and trivially reachable.
