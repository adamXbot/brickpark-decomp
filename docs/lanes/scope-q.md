# scope Q — the main loop, the map-screen frame, level state and cursor segments (2026-09-05)

Branch `scope/Q`, notes for folding into `docs/DECOMP.md` at integration.
Three new files, nothing else touched. **12 of 17 exact (472 instructions);
4 WIP (560 instructions, each with its residual named above the marker);
1 dead body of 291 instructions decoded and deliberately not attempted.**
`audit.py` ends PASS on all three files, `/W3` clean, `relocs.py` zero
MISMATCH.

## Per function

| address | name | insns | % | audit | marker | residual |
| --- | --- | ---: | ---: | --- | --- | --- |
| 0x004594e0 | `KillFrontEndScreenIfActive` | 5 | 100 | `[OK]` | `// FUNCTION:` | — (one-case `switch`, see levers) |
| 0x004594f0 | `ResetCurProfileDefaults` | 11 | 100 | `[OK]` | `// FUNCTION:` | first compile |
| 0x00462e50 | `SetLevelParamA` | 6 | 100 | `[OK]` | `// FUNCTION:` | first compile |
| 0x00462e70 | `SetLevelParamB` | 6 | 100 | `[OK]` | `// FUNCTION:` | first compile |
| 0x00462e90 | `ResetLevelParams` | 17 | 100 | `[OK]` | `// FUNCTION:` | first compile |
| 0x004597e0 | `SetLevelEndSequence` | 20 | 100 | `[OK]` | `// FUNCTION:` | first compile (`#pragma function(memcpy)`) |
| 0x0045ac20 | `UnloadSessionSprites` | 50 | 100 | `[OK]` | `// FUNCTION:` | first compile |
| 0x00459360 | `MapScreenFrame` | 102 | 100 | `[OK]` | `// FUNCTION:` | 63/98 → 101/102 → 102 (see levers) |
| 0x004629e0 | `ResetLevelObjects` | 113 | 100 | `[OK]` | `// FUNCTION:` | first compile |
| 0x00459520 | `RunGame` | 121 | 100 | `[OK]` | `// FUNCTION:` | 120/121 → 121 (`volatile` flag) |
| 0x0045fad0 | `DrawCursorSegmentB` | 160 | 94.2 | `[WIP]` | `// WIP-FUNCTION: … (94.2%, …)` | first diverging index 63: the switch's default-arm layout, 17 spellings, STRUCTURAL, lever not found |
| 0x0045fca0 | `DrawCursorSegmentA` | 195 | ~77 | `[WIP]` | `// WIP-FUNCTION: … (77%, …)` | the same layout plus a scratch rotation and a folded pitch load in the diagonal loop |
| 0x00459970 | `TallyBuildFootprints` | 116 | 94.8 | `[WIP]` | `// WIP-FUNCTION: … (94.8%, …)` | both latches: store/bound-load order, 8 spellings, strict == rb == ob |
| 0x004598b0 | `ClearBuildTally` (dead) | 6 | 100 | `[OK]` | `// FUNCTION:` | first compile |
| 0x0045e930 | `DoorTileStep` (dead) | 15 | 100 | `[OK]` | `// FUNCTION:` | first compile (`return (int)c`) |
| 0x0045e960 | `FindObjDoorTile` (dead) | 89 | 93.3 | `[WIP]` | `// WIP-FUNCTION: … (93.3%, …)` | callee-saved choice for two Pos copies; rb 0 |
| 0x0045ade0 | `DrawTileDebugOverlay` (dead) | 291 | — | — | none | decoded, not attempted; shape recorded at the end of mapbuild2.c |

Names: every provisional name in the brief was kept; the disassembly
contradicted none. First named here (extern-only, defined nowhere yet):
`SuspendMusicThread` 0x00492c60 / `ResumeMusicThread` 0x00492c80
(`SuspendThread`/`ResumeThread` on `g_music_sys`'s thread handle at
0x0079a698), `LoadIconBarGFX` 0x0046f890 / `UnloadIconBarGFX` 0x0046f920
(GBarFrame.lls, IF_Side_*.lls), `InitAdvisorMovies` 0x00444090 /
`KillAdvisorMovies` 0x00444150 (AD_Blink/AD_LR/AD_Phone.avi),
`KillControllers` 0x00451f40, `UnloadBubbleHelpGFX` 0x00454a10 ("SPEECH
BUBBLE"), `UnInitialiseBlokes` 0x00482ec0, `UpdateIconPage` 0x0046ee00,
`GetPathSquareList` 0x00481720 (returns `g_path_squares`), the globals
`g_map_click_time/x/y` 0x00667c68/70/74, `g_tally_blocked/special/percent`
0x00667d00/04/08, and the CRT `memcpy` at 0x004a0110 and the import slots
`PeekMessageA` [0x4ab2bc], `Sleep` [0x4ab114], `LoadLibraryA` [0x4ab124],
`FreeLibrary` [0x4ab128], `PtInRect` [0x4ab2c0] (all confirmed from the
import table).

## Mechanics recovered

- **`RunGame` (the session), in order:** `g_hedge_def = ElemID("HEDGE")->def`;
  profile defaults (zero the 0x110-byte record, music 100, speech 75, sfx
  75); `InitSoundSystem`; `SetMusicGrooveLevel(1)`; suspend the music
  thread; `SetupControllers`; `LLIDB_ClearOnLevel`; `ResetController`;
  `SetPointer(0)`; `ProcessSystemEvents`; `PlayMovie("lmi.avi", 0, 1)`;
  `ShowWaitSprite`; resume the music thread; `SetWaitSpriteRect(0, 0)`;
  then **spin until `g_music_disabled` is nonzero** (`PeekMessageA` /
  `Sleep(100)` / `progress_tick` per turn — the wait for the music thread's
  start-up); `LoadIconBarGFX`; `LoadWorkerInterfaceGFX`; `LoadBubbleHelpGFX`;
  `InitialiseBlokes`; `InitGameMap`; `SetPointer(0)`;
  `PlayMovie("Intro.avi", 1, 0)`; `SetThemeInTransition(0)`; `SetPointer(5)`;
  `g_detail = 0`; control icons; tick; theme icons; `FreeTileSpace(0, 0x800)`;
  **`g_game_mode = 3`**; `LoadMapTiles`; tick; `InitMan`; tick;
  `CreateObjectClasses`; tick; `EnterFrontEnd` (mode 2, scope P); tick;
  `LoadLibraryA("Ir50_32.dll")` (the Indeo 5 codec, kept for the session);
  tick; the advisor movies; tick; `ClearWaitSprite`; resume the music
  thread again; **`r = GameFrame(); while (r) r = GameFrame();`** (two call
  sites, exactly as the brief predicted); then teardown in this order:
  `KillFrontEndScreenIfActive`, `PauseCurrentTrack`, free `g_backdrop`,
  `KillControllers`, icon bar, bubble help, blokes, `KillGameMap`, control
  icons, theme icons, `UnloadSessionSprites`, `UnInitMan`, advisor movies,
  `FreeLibrary`, `FreeBlokeCounters`, `KillHelp`, `KillSoundSystem`,
  `KillInputSystem`.
- **`MapScreenFrame` draw order:** `ResetHitInfo`; lock the surface;
  `DrawMapScreen`; `SetPointer(5)`; `PrintSprite(g_ci_interface_bg, 0, 0, 0,
  &ctx)` with `ctx = {1, 0, 0}`; `UpdateIconPage`; `RenderIcons`;
  `CheckFocussedIcon`; then on the hit type: 2 → `SetPointer(6)` and finish;
  bit 0x100 (over the map) → clip to `{0, 32, 640, 372}`, `RenderMouseBounds`,
  restore the clip; left button held → `MapScreenSetScrollPos(&g_gfx_point)`;
  right button → **double-click detection**: within 500 ms of the last right
  click and within 5 px in x and y, `g_80ff70 = 1`, `g_game_mode =
  g_game_mode_saved`, `g_game_mode_saved = 1` (leave the map back to the
  mode the map button stashed); the click's position and time are recorded
  every time. Finish: `UpdateFocussedIconPtr`, `PopRenderingStatus`,
  `RenderingComplete`.
- **The 0x00832824 table** is six 0x2c-byte records (bigsim.c sees the same
  memory as `g_map_ai.cat[k]` from +0x14); only the first two dwords are
  proven here: `a` (+0) and `b` (+4). `ResetLevelParams` sets records 0..5
  to (0x32,0x14), (0x21,0x32), (0,0), (0,0), (0x21,0x28), (0x21,0x28); the
  two setters write one field of record `i`.
- **`SetLevelEndSequence(which, s)`** copies 0x100 bytes into
  `g_level_end_sequence1` (which != 0) or `…2` and NUL-terminates at
  [0xff]; a NULL `s` empties the buffer.
- **`ResetLevelObjects`** returns 0 when no map is loaded; otherwise
  `ResetBuildStats`, clears the `u16` at +0x0c of every map cell, unloads
  the TSM mapping and terrain elements (and the second terrain element when
  present), unloads every LLIDB element whose flags have both 0x10 and 1,
  frees the two level arrays (clearing `flags & ~0x3000e` on each element
  of the second before unloading it), `ClearOverlays`, `sub_4828f0`, and
  clears `g_map_loaded`.
- **`UnloadSessionSprites`** frees the map block, unloads "MAPPING 1" and
  kills the four arrow sprites; returns 1.
- **The cursor segment fills** (bigrender.c's `RenderCursor` callees):
  16-bit surfaces only (`vs->format == 2`). B is a solid marching-ants
  pattern of 16 entries (12 of the first colour, 4 of the second), A a
  transparent one of 32 (4 of each colour, the rest untouched — A only
  paints nonzero entries); the phase is `g_cursor_phase` (the frame
  counter) and advances per pixel. kind 0 walks up one column painting the
  pixel and its left neighbour (x > 0); kinds 1/2 walk a diagonal with
  `dir = -1/+1`, painting the pixel and the one above (y > 0), stepping x
  every pixel and y every second pixel; both loops PtInRect every pixel
  against `g_clip`. kind 0 paints `h` pixels, the diagonals `h + 1`.
- **`TallyBuildFootprints`** runs at most every 10 s (`GetGameTimer`):
  for every path square it adds the square's perimeter (`2*(w+h)+4`) to
  `blocked`, then calls `TallyFootprintCell` on every cell of the ring one
  outside the rect (top and bottom rows, then left and right columns),
  which decrements `blocked` for passable cells and increments `special`
  for type 2/3 objects; stores `g_tally_blocked`, `g_tally_special` and
  `g_tally_percent = special * 100 / blocked` (0 when nothing is blocked).
- **`FindObjDoorTile`/`DoorTileStep`** (dead): 0 when the object's entrance
  tile (`pos + cls->door`) is on the map and holds a door-blocking object
  (`flags & 0x8a8`) of a class other than the environment class; the exit
  check is nested inside the entrance check and reads the SAME +0x0c/+0x10
  offset pair (an original quirk, reproduced).

## Extern-type divergences (deliberate; do not "align")

- `g_music_disabled` is `volatile int` here (as in musicthread.c); the
  plain `int` of the other files loses RunGame's load placement (see levers).
- `col` in the cursor fills is `const unsigned char*` (bigrender.c declares
  `const char*`): the seven byte loads are zero-extended (`xor eax,eax /
  mov al`), so the elements are unsigned.
- `PrintSprite`'s fifth parameter is `void*` here (bigrender.c also uses
  `void*`, money.c `BlitCtx*`); `PtInRect(const ClipRect*, int, int)` takes
  the POINT as two ints.
- `g_map` is a `MapHdr` with only `w`/`h` (unsigned shorts at +0x14/+0x16);
  `g_cur_profile` a 0x110-byte blob (bigscreens.c has the layout);
  `g_level_params` is this file's view of 0x00832824 (bigsim.c: `g_map_ai`).
- `g_hit_type` is read as an `int`, `g_mouse_buttons` as an
  `unsigned char` (`test byte ptr`), `g_gfx_point` as a `Pos`.

## Levers, with evidence

- **A one-case `switch` compiles to `sub eax,K / jne`; `if (x == K)` to
  `cmp`.** `KillFrontEndScreenIfActive`: `if (g_game_mode == 2)
  KillCurrentScreen();` 2/4; `switch (g_game_mode) { case 2:
  KillCurrentScreen(); break; }` 5/5 with the tail `jmp`.
- **`#pragma function(memcpy)` around the one body that CALLS memcpy**
  (fable-d's lever, third confirmation): `SetLevelEndSequence` 20/20 first
  compile, `call 0x4a0110` with the three pushes and `add esp,0xc`.
- **A `volatile` flag polled by a wait loop keeps its first load below the
  merged argument cleanup.** `RunGame` with plain `int g_music_disabled`
  hoists `mov eax,[g_music_disabled]` above `add esp,0x20` (120/121); the
  `volatile` declaration musicthread.c already uses puts it after (121/121).
  Evidence that the original header declared the flag volatile.
- **Read a frame offset with the pending pushes counted.** MapScreenFrame's
  `lea ecx,[esp+8]` looked like `&ctx.sub` until the `push 5` of the
  preceding `SetPointer(5)` (cleaned up only at the merged `add esp,0x18`)
  was counted: it is `&ctx`, the whole {1, 0, 0} record. A probe with
  distinct constants (`kind = 1, p = 2, n = 3`) settled it in one compile.
- **Two zero stores plus three zero pushes across calls form a callee-saved
  zero web (an extra `push esi`); `memset(&ctx.sub, 0, 8)` does not.**
  MapScreenFrame 63/98 with `ctx.sub.p = 0; ctx.sub.n = 0;` (esi = 0 for
  the stores, `push esi` ×3 for the arguments, `clip.left = 0` from esi);
  the memset spells the two zeros through scratch eax and the pushes and
  the clip store become immediates: 101/102.
- **A three-call tail duplicated into the `== 2` arm is kept as a copy**
  (`SetPointer(6); UpdateFocussedIconPtr(); PopRenderingStatus();
  RenderingComplete(); return;` written out, then the same three calls after
  the if): VC6 does not cross-jump it (BL05's two-call threshold read from
  above). With the tail written once the arm jumps to the shared copy.
- **A block-scoped aggregate initialiser stores at the use site with
  immediates:** `ClipRect clip = { 0, 0x20, 0x280, 0x174 };` declared inside
  the `& 0x100` branch gives the original's four immediate stores
  interleaved with the `GetClipping(&saved)` setup; the same as four
  assignments at function scope does not (FR06 from the branch side).
- **`i = n; while (i--)` is the trip-count idiom that keeps `i` and derives
  the counter as `i + 1`:** the cursor fills' `mov eax,[h] / inc / mov ecx,eax
  / dec / test ecx / je / inc / mov [h],eax` (diagonals, `i = h + 1`) and
  `mov ecx,eax / dec eax / test ecx / lea edi,[eax+1]` (vertical, `i = h`,
  where the parameter itself is decremented: `while (h--)`). `for (i = h;
  i != 0; i--)` folds the dance to `test eax,eax`; `for (i = h; i >= 0; i--)`
  gives a `jl` guard.
- **VC6 SP3 idiom-recognises a constant-trip fill loop into `rep stosd`**
  (12 `unsigned short` stores of one value become `mov si,ax / shl esi,0x10
  / mov si,ax / mov ecx,6 / rep stosd`), even though it never unrolls. The
  original's 16 and 32 explicit `mov word ptr` stores need 16 and 32
  explicit assignments in the source (or an initialiser).
- **Nesting shows in the failure edges.** `FindObjDoorTile`'s "no entrance"
  branch jumps to the final `return 1`, not to the exit check, so the exit
  check is INSIDE the entrance block; written sequentially the shared `pos`
  load is hoisted above the first test (`mov edi,[esp+0x18]` before the
  `add esp,4`), nested it stays in the block (`[esp+0x14]` after).
- **`if (!c) return (int)c;` reproduces `test eax,eax / jne / ret` with no
  `xor`** (`DoorTileStep` 15/15): the tested register is the return value.
  `return 0;` would materialise the zero.
- **A `Pos` struct copy loads both fields before either use** (`mov ecx,
  [esi+0xc] / mov ebx,[esi+0x10]` then the two adds); separate `int`
  temporaries interleave the loads (79 vs 82/89 on `FindObjDoorTile`).
- **Two `LLElem*` out-pointer locals: the first declared takes the lowest
  slot** — `LLElem* d; LLElem* e;` puts `d` (FindElementFromDataPtr) at
  S+0 and `e` (GetElement) at S+4, the original's homes (`ResetLevelObjects`
  113/113 first compile).
- **The map loop `for (y = 0; y < g_map->h; y++) for (x = 0; x < g_map->w;
  x++) g_map_rows[y][x].f0c = 0;`** with the dimensions as `unsigned short`
  gives the `jbe` zero-trip guard, the `movzx`-then-`jl` latch, the per-pass
  reloads of both globals (the word store may alias) and the
  `[edx+ecx-8]` addressing with ecx pre-incremented by 0x14 — first compile.
- **Measured negative — the switch default arm (cursor fills).** The
  original's kind chain falls through into a CLONED exit epilogue for the
  default and keeps case 1 (`jmp`) before case 2 (adjacent to the shared
  loop); ours inverts the last test to `jne end` and places case 2 first.
  Seventeen spellings are identical (both notes list them); the shape `dec
  eax / je / four pops / add esp / ret` occurs in no other function of the
  binary, so no template exists. Whoever finds it: it is one block
  decision, and DrawCursorSegmentA's remaining scratch rotation almost
  certainly follows from it.
- **Measured negative — a latch store sunk below the compare
  (`TallyBuildFootprints`).** The original reloads the counter, loads the
  bound, cleans up, increments, compares and stores after the compare; ours
  stores before the bound load in eight spellings (the note lists them). A
  separate register counter costs 50.

## Dead code left

`DrawTileDebugOverlay` 0x0045ade0 (291i) — decoded shape recorded in
mapbuild2.c; nothing live names it, so it was not attempted. `ClearBuildTally`,
`DoorTileStep`, `FindObjDoorTile` are the other three dead bodies; two are
exact.
