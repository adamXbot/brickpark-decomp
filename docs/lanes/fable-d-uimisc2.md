# Lane `fable-d` / `uimisc2.c` — movie, help bar, report and level-end screens

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
