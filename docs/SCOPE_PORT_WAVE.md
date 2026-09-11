# Scope PORT-A / PORT-B / PORT-C — the port wave: first running code (2026-09-11)

> **PORT-A — Status: MERGED (2026-09-11) — wasm32 link closes (gen_link plans globals.c, signature-matched forwarders/stubs), ll_host.h, kernel32.c (56 KERNEL32-family imports real), `legoland_headless` runs the spine to DirectDrawCreate under node; census host 149 -> 95, game-fn 12 -> 0 (CRT thunks forwarded). OPEN follow-up (PORT-A2): pointers to UNNAMED .rdata are not re-pointed under --ilp32, so `g_volume_names` yields "D:\\.res" and the loader fails — the first thing the browser page hits after the CD check. Notes `docs/lanes/scope-port-a.md`**
> **PORT-B — Status: MERGED (2026-09-11) — DDRAW/USER32/GDI32/DINPUT/WINMM/DSOUND shim complete, 0 traps left in those six DLLs, ASYNCIFY main loop decided; `legoland_shimtest` runs at ~82 fps in the browser; `legoland_browser` itself waits on PORT-A's wasm32 closure. GDI text, AVI, sound, MIDI, printing are documented non-trapping stubs. Notes `docs/lanes/scope-port-b.md`**
> **PORT-C — Status: MERGED at 5 tests / 193 checks green natively (2026-09-11, d19d3fd7) — res_archive, llidb_icm and loadpos are ILP32-only and run once PORT-A's wasm32 closure links; 8 findings in `docs/lanes/scope-port-c.md` (resfile.py drops alias members; RES_EnsureMounted needs GetVolumeInformationA to report CDFS/LEGOLAND; LEGOLAND.ICM case)**
> Branches `scope/PORT-A`, `scope/PORT-B`,
> `scope/PORT-C` from `origin/main` `6de9cab0`+scaffold. Notes:
> `docs/lanes/scope-port-a.md` / `-b.md` / `-c.md`. No VC6 object prefix: these
> lanes do not match; they compile with clang/emcc only.
> **PORT-B2 — Status: MERGED (2026-09-11) — GDI text real (ll_font.c, DrawTextA measures), MessageBoxA answers IDCANCEL, SPI_GETMOUSE, node-safe shim (shimtest --frames 400 PASS under node), input mapping verified against input.c/input2.c; page shows frames, last MessageBox, TRAP banner** — the
> follow-up to PORT-B: everything the first front-end frame and the first click
> need once PORT-A2's loader fix lands. GDI text made visible (a real bitmap
> font into the 16-bpp surface), `MessageBoxA` auto-answering so the modal loops
> terminate, the JS library made node-safe for `legoland_tests`/`legoland_headless`,
> the DirectInput shapes checked against `input.c`/`input2.c`, and the page
> instrumented. Notes `docs/lanes/scope-port-b2.md`.
> **PORT-A2 — Status: MERGED (2026-09-11) — pointer words resolve to symbol / interior-of-block / game object / gap (272 exact + 441 interior), all three RES volumes open; fopen wrapper + install-path resolution; tests link the shim, llidb_icm 47/47 (oracle was wrong); spine now reaches LoadSprite -> unreachable in __BMPLoader, caused by live prototype conflicts (RES_CloseFile, RES_CloseVolume, DBPrintf) — a matching lane's work under LEGOLAND_PORTABLE guards**
> **PORT-M1 — Status: MERGED (2026-09-11) — 105 prototype conflicts fixed in 126 game files under LEGOLAND_PORTABLE guards (wasm-ld signature warnings 130 -> 19 symbols, census 542 -> 437); audit PASS 2168 [OK], relocs 0 MISMATCH, 3281/42 unchanged; the game now runs InitSession end to end and ShowTitleScreen reaches RLEPaintHit, the first unported asm painter (rlepaint.c). 19 call-site mismatches with recipes and the NewScriptEvent duplicate-name finding in `docs/lanes/scope-port-m1.md`**
> **PORT-A3 — Status: MERGED (2026-09-11) — gen_link emits ONE block per object with interior aliases (15 objects / 100 names; g_key_state and g_gpu_state were live and wrong), `name_trap.py` + `legoland_headless_debug` name a poisoned call site, `headless_spine` and `install_paths` ctests, MEMFS case-insensitive path resolution. Notes `docs/lanes/scope-port-a3.md`**
> **PORT-M2 — Status: MERGED (2026-09-12) — the 19 call-site signature mismatches closed (wasm-ld 19 -> 1, the survivor is gen_link's printf alias declaration, integrator-side); two REAL name fixes for both builds: levelkw.c/movie3.c called `NewScriptEvent` where 0x004689f0 is `AddScriptString`, coaster4.c declared 0x00420e90 as `DrawSupportModel` where it is `Coaster3D_DrawModel` — bytes identical, audit PASS 366 [OK], relocs 0 MISMATCH, 3281/42; ctest 8/8; a clean wasm-ld run is not a valid module (binaryen directize) — see `docs/lanes/scope-port-m2.md`**
> **PORT-B3 — Status: MERGED (2026-09-12) — 11 asm painters ported to C in the #else arms (rlepaint.c x8, rlepaint2.c x2, softblit2.c SoftBlitAnimPlain), asm arms byte-identical, audit [OK] mismatch=0, relocs 0; tests test_rle_paint (43, incl. a real shipped sprite bit-exact) and test_anim_paint (10); census asm stubs 26 -> 15. THE TITLE SCREEN RENDERS in the browser. Next blocker: RunGame's music wait spins because DirectSoundCreate returns DSERR_NODRIVER so InitMusicSystem never runs — dsound.c needs a no-op IDirectSound (PORT-B's file). Finding: softblit.c:740 SoftBlitAnim's `row:` label is one line too low (source-level, invisible to the byte gates). Notes `docs/lanes/scope-port-b3.md`**
> **PORT-A4 — Status: MERGED (2026-09-12) — gen_link declares aliased CRT names from a libc prototype table (wasm-ld signature mismatches 0, validator 0 on all 7 modules); name_trap.py names call_indirect type mismatches by module byte offset and callback table; CI job `portable-wasm` (emsdk 6.0.9, asset-free ctest 5/5, fails on any signature mismatch); `headless_spine` pins the title-screen first present (98% non-black, checksum 0x4a092b01). Notes `docs/lanes/scope-port-a4.md`**
> **PORT-B4 — Status: MERGED (2026-09-12) — THE FRONT END COMES UP: a silent IDirectSound with a wall-clock play cursor (+ ole32 failure path) lets RunGame leave the music wait; PLAYER DETAILS renders and runs at 33.5 fps (the 28 ms flip floor); `?args=` on the page. Open: input arrives from the host but the front end ignores it (suspect a live global resolving to two objects); CreateThread refuses so music-ON hangs (kernel32.c); `_findclose(-1)` traps (msvcrt.c); ~41 externs with no address comment trap when reached (WindowProc, ODFError, ObjDefFinalize, SelectProfileSlot ... — matching-side name/address fixes). Notes `docs/lanes/scope-port-b4.md`**
> **PORT-M3 — Status: MERGED (2026-09-12) — typed-callback pass: 30 slots/tables inventoried from the wasm objects + -O0 IR (475 indirect call sites), 26 now type-consistent (Icon input slot, level keyword table, ObjDef +0xa8/+0xac/+0xb8/+0xbc, coaster10 casts, TrackDesc build hook); 15 files, audit PASS 363 [OK], relocs 0, portable-undefined reduction byte-identical; `portable/tests/test_callback_types.c` type-checks 454 (slot, body) pairs at compile time. Open (§5): DrawBasicPath in +0xa0, schoolcar3.c hooks[3], g_fast_sqrt/rsqrt casts, coaster8.c attach/detach. Notes `docs/lanes/scope-port-m3.md`**
> **PORT-B5 — Status: MERGED (2026-09-12) — 9 more asm bodies ported (SoftBlitAnim with the `row:` label fix, tri3d.c x4, ZBufferHelper, BltAdvisor, ShowCapacityOverlay, RenderTransSprite); the 3 ST(0) helpers proved unreachable in the portable build; `LL_FISTP` no longer lowers to a libm call (every fistp site was a latent wasm trap); census asm stubs 15 -> 6; tests anim_recolour/tri_raster/zbuf_blit (70 checks); findings: tri3d/texture texel formula transposed in the headers, DrawGouraudTexTri masks crossed, ZBufferHelper 2-byte step in a 4-byte buffer, sub_458930 is (int)<float> rounding to nearest at 110 sites. Notes `docs/lanes/scope-port-b5.md`**
> **PORT-M4 — Status: MERGED (2026-09-12) — externs without a readable address comment 81 -> 2 (52 lacked one, 29 had one the scanner could not read: continuation lines, function-pointer types); 7 live globals had existed TWICE as zeroed placeholders (g_present, g_active_input_cb, g_lt_action_handlers, ...); 30 files, audit rows unchanged, relocs 0 MISMATCH, 3281/42; WindowProc -> LegoLandWindowProc. With the integrator's libm/WINMM classification fixes the page reaches PLAYER DETAILS with zero GAME traps (six AVIFIL32 host stubs remain). Four caller/definition name disagreements recorded, not resolved. Notes `docs/lanes/scope-port-m4.md`**
> **PORT-A5 — Status: MERGED (2026-09-12) — LL_TRAP_CONTINUE=1 / name_trap.py --continue (every blocker in one run); CreateThread runs MusicThread inline with a setjmp escape and unsignalled events poll as WAIT_TIMEOUT (the front end is reached WITHOUT -nomusic); _findclose(-1) and the CRT -1-handle family; THE INPUT BUG: the 164-byte GameInput record (and PopUpUI, Profile) was emitted as nine separate objects — STRUCT_EXTENTS in gen_link.py, ctest probe_input. Notes `docs/lanes/scope-port-a5.md`**
> **PORT-B6 — Status: MERGED (2026-09-12) — AVIFIL32/MSACM32/WINSPOOL shims: generated host traps 96 -> 0, the page runs the front end trap-free at 33 fps; press latch, live-surface registry and keyCode fallback fixed; window.llFrameHash/llNonBlack/llSnapshot/llTrace/llStats hooks. Integrator added B6's two split-record rows (BlitCtx/HitInfo 0x004bdd00, CurProfile 0x0080ffa0) to gen_link's STRUCT_EXTENTS: A CLICK ON SLOT 1 NOW OPENS THE NAME EDITOR (hash 0x9d9b7c50 -> 0xdd2ac534). Notes `docs/lanes/scope-port-b6.md`**
> **PORT-A6 — Status: MERGED (2026-09-12) — `tools/cdecl.py` computes object extents from the game's own struct definitions (MSVC x86 layout, #pragma pack, ILP32): 27 more split objects merged beyond the hand table (g_front, EditMode, g_map_ai with 45 names, g_castle with 16, ...), every initialised byte and pointer target identical, ctest cdecl_extents; M4's three linkreport scanner fixes; 372 overlap hazards catalogued with a recipe. Notes `docs/lanes/scope-port-a6.md`**
> **PORT-B7 — Status: MERGED (2026-09-12) — typing a profile name works end to end (read back live from g_temp_profile.name); llMove/llClick/llKey/llType page driver in game pixels, TRAP banner for the RuntimeError ASYNCIFY used to swallow, ll_dbg_addr live globals, legoland_browser_named; main menu, tutorial select and park-advert screens reached on a throwaway build. Blocker B1 (integrator applied at merge): spritemisc.c declares the owner vtable's +0x08 Release slot `void` where it is `long` — the first expiring cached-text sprite trapped the module. B2: two clicks past the main menu the loop stops yielding (next B lane). Park not reached. Notes `docs/lanes/scope-port-b7.md`**
> **generated host traps 96 -> 0**, and `legoland_headless` runs the front end
> with no `LL_TRAP_CONTINUE`. Three host input defects fixed and measured in a
> tab: dinput.c's **press latch** (a click whose down and up landed in one drain
> collapsed to "up" — `buttons=000` on every poll), ddraw.c's **live-surface
> registry** (`ll_host_surface_pixels` dereferenced a gdi32 cookie from
> `CreateCompatibleDC(0)`, killing the module in `HTBubbleHelp`), and
> ll_canvas.js's **`dikOf`** (keydown with `code == ""` — every virtual/IME/
> automation keystroke was dropped). Page tooling: `llFrameHash/llSnapshot/
> llStats/llTrace` + a collapsible host-call panel. Screens reached: PLAYER
> DETAILS `0x9d9b7c50`, its bubble help, the NEW PROFILE popup `0x5ab7ca10` from
> one real click. Gates wasm ctest 14/14, native 8/8. **OPEN, PORT-A:** two more
> split records, both proved — the 12-byte hit record at 0x004bdd00 is THREE
> objects (so NO front-end icon can ever be focussed) and the 270-byte
> `CurProfile` at 0x0080ffa0 is EIGHT (so the name editor never runs); both rows
> twice-cited in the notes. Notes `docs/lanes/scope-port-b6.md`.
> **PORT-M5 — Status: MERGED (2026-09-12) — five caller/definition names decided from the disassembly (ShowStepHint, ResetFreePlayTable, SaveFrontEndState, CloseActiveThemeButton, SetVidAnim); M3's three open slots closed (DrawBasicPath is an original mis-registration that cannot fire; coaster8.c detach was a wrong callee NAME — RouteSeat_ReleaseCar); sub_458930 is a bare fistp that ROUNDS: 95 of 111 float-to-int sites now LL_FISTP in portable arms, g_recip[30]=2185 corrected; the last three span fillers ported (asm stubs 6 -> 3, all unreachable); 37 files, audit 425 [OK] mismatch=0, relocs 0 tree-wide, 3281/42. Notes `docs/lanes/scope-port-m5.md`**
> **PORT-B8 — Status: MERGED (2026-09-12) — B2 diagnosed with a host-call heartbeat (`?beat=`): both doors into the park spin in `KillLowMarkerSprites` because `g_low_markers`/`g_level_markers` carry RAW x86 addresses of strings swallowed inside A6's widened objects (1211 raw-VA words in 177 objects — PORT-A7's). Tutorial select and advert screens replayable; save/load of Profile1.txt works in-session (MEMFS, lost on reload). Notes `docs/lanes/scope-port-b8.md`**
> **PORT-M6 — Status: MERGED (2026-09-12) — pathmisc2.c declared three pointer PAIRS on single extern lines (two addresses, one comment) that every scanner read as one: in the portable build g_route_open WAS g_route_closed and the gardener/mechanic work-order tails were their heads — split for both builds (no bytes move); overlap hazards: the mechanism is a CSE'd LOAD (volatile goes on the read; same-address pairs are the worst case), six genuine pairs folded in portable arms, 165 recorded; SetVidAnim(NULL) is a shipped game bug (guarded portable-only), OpenMovie's pfile is not a bug; RES_LowRead/RES_LowSeek renamed to ReadFile/SetFilePointer; void-slot class closed; cast forwarders 82 -> 72 (the rest need declaration+slot moved together — PORT-M7). 19 files, audit 244 [OK], relocs 0, 3281/42. Notes `docs/lanes/scope-port-m6.md`**
> **PORT-M7 — Status: READY FOR MERGE (claimed 2026-09-12 by PORT-M7) — cast forwarders 72 -> 0**, declaration and slot type moved together (continues PORT-M3). 71 rows were a stale registration name typed with an EMPTY parameter list against a body that takes arguments, and for those the cast was the only wrong thing in the chain: interfaces.c/ridesave.c declare every ObjDef slot `void*`, so the `call_indirect` type comes from the CALLING file and already agreed with the body. Two needed their slot moved too (`mapscreen.c`'s `typedef void (*IconHandler)(void)` against fpui.c's four-argument call -- the first click on the map screen OK icon; `castleobj.c`'s `Track_Update90` into +0x90). One is a different ABI, not arity: `sub_457970` is `FootprintClearanceTest(Pos)` by value, identical on x86, indirect on wasm32 -- the alias is gone from the closure (263 -> 262) and the drag-build path no longer faults. Adjacent, same sweep: interfaces.c's 33 `+0xac` and 2 `+0xa8` registrations got PORT-M3 section 4b's adapter, so NO `def->cb_* =` store in the tree now disagrees with its slot. 5 files, audit 103 `[OK]` 0 REJECT, relocs 0 MISMATCH, 3281/42, marker set identical; test_callback_types.c +142 compile-checked pairs; wasm ctest 16/16, native 10/10; a PRE-LANE build gives a byte-identical 2639-line headless trace. Notes `docs/lanes/scope-port-m7.md`**

This wave is NOT matching work. The matching phase is at its practical end
(3281 exact / 42 WIP, 81.9% exact, every game function has a C body). The
portable build (`portable/`, see `portable/README.md`) compiles all 258 game
sources with clang and emcc, and the native 64-bit whole-archive link closes.
What does not exist yet is a program that runs the recovered game. The user's
decision (2026-09-11): **browser canvas via Emscripten is the first run
target**; wasm32 is also the only ILP32 target this Mac can execute.

## Hard rules for every lane

1. **`LEGOLAND/*.c` is read-only for PORT-B and PORT-C.** PORT-A may add
   `#ifdef LEGOLAND_PORTABLE` guards only (never between a `// FUNCTION:`
   marker and its signature), and must run the VC6 gate on every file it
   touches: `$PY tools/audit.py LEGOLAND/<file>.c` (all `[OK]`, no REJECT),
   `$PY tools/relocs.py LEGOLAND/<file>.c | grep MISMATCH` (empty). Anything
   the game needs from the host goes in `portable/`, not in the game.
2. **The generated closure is the truth about what is missing.** Do not hand-
   write a stub for a symbol `gen_link.py` already generates; make the
   generator or the host shim own it. `portable/tools/linkreport.py` is the
   census; re-run it in your notes before and after.
3. **File ownership** (a lane edits only its files; shared files are listed):

| lane | owns | shared (append-only, small) |
| --- | --- | --- |
| PORT-A | `portable/tools/gen_link.py`, `portable/tools/linkreport.py`, `portable/cmake/headless.cmake`, `portable/src/headless/**`, `portable/src/hostwin/kernel32.c` (+ `advapi32`/`version` in the same file), `portable/hostwin/include/ll_host.h` (the host ABI header, see below) | `portable/README.md` (your section), `LEGOLAND/*.c` guards only |
| PORT-B | `portable/cmake/browser.cmake`, `portable/src/browser/**` (C + JS library + `index.html`), `portable/src/hostwin/ddraw.c`, `user32.c`, `dinput.c`, `winmm.c`, `gdi32.c`, `dsound.c` (stub) | `portable/README.md` (your section), `portable/hostwin/include/ll_host.h` (add declarations; PORT-A creates it in its first commit — if it does not exist yet, create it with just your declarations and note it) |
| PORT-C | `portable/cmake/tests.cmake`, `portable/tests/**`, `tools/oracle_*.py` | `portable/README.md` (your section) |

Nobody edits `portable/CMakeLists.txt` (it already includes the three lane
files) or `docs/HANDOFF.md` (the integrator does).

4. **Environment.** Your worktree is a fresh checkout: the gitignored assets
   are missing. First thing:
   ```
   ln -s /Users/systemadmin/Documents/Development/Github/legoland/original original
   ln -s /Users/systemadmin/Documents/Development/Github/legoland/gamedata gamedata
   ln -s /Users/systemadmin/Documents/Development/Github/legoland/toolchain toolchain
   PY=/Users/systemadmin/.venvs/legoland/bin/python
   export LEGOLAND_CL=/Users/systemadmin/Documents/Development/Github/alphateam/tools/wibo-msvc/cl
   ```
   cmake 4.4, ninja 1.13 and emsdk 6.0.9 are on PATH (Homebrew). Build dirs:
   `portable/build` (native clang, `cmake -S portable -B portable/build -G Ninja
   -DPython3_EXECUTABLE=$PY`) and `portable/build-wasm` (`emcmake cmake -S
   portable -B portable/build-wasm -G Ninja -DLL_ILP32=ON
   -DPython3_EXECUTABLE=$PY`). Both are gitignored. Put shell loops in a script
   file under your scratchpad, not inline (the worktree guard refuses inline
   loops).
5. **Commit on your branch only, small commits, notes in `docs/lanes/`.** The
   integrator merges. Never push to `main`. Update this brief's status line
   to `IN PROGRESS (claimed <date> by <lane>)` in your first commit.
6. **Never run `tools/verify.py`.** It is the integrator's whole-tree gate and
   must run alone.

## What the game needs from a host (read before designing)

From the import table (`portable/tools/win32_imports.txt`): KERNEL32 102,
USER32 34, GDI32 28, AVIFIL32 16 (Indeo 5 intros — stub), MSACM32 6 (ADPCM —
stub), WINMM 5 (`timeSetEvent`/`timeKillEvent`/`timeGetTime`, `midiOut*` —
timers real, MIDI stub), VERSION 3, ole32 2 (DirectMusic — stub), DSOUND 1
(`DirectSoundCreate` — stub returning failure first), DINPUT 1, DDRAW 1
(`DirectDrawCreate`), WINSPOOL 1 (printing — stub).

The startup spine is `WinMain` (winmain.c) -> `GameMain` (startup.c):
`CreateMutexA`, `WaitForSingleObject`, command switches (`-nointro`,
`-nomusic`, `WINDEBUG` = windowed + `FlipPrimary`), `CheckHostSystemGPU`
(gpu.c: `DirectDrawCreate`, `QueryInterface(IID_IDirectDraw2)`,
`SetCooperativeLevel`, `SetDisplayMode`, `CreateSurface` primary+back,
`CreateClipper`), then `InitSession` -> `RunGame`. The DirectDraw vtable slots
the game actually calls are documented at the top of `LEGOLAND/gpu.c` (IDirectDraw:
QueryInterface/Release/...; IDirectDrawSurface: Release 0x08, Blt 0x14, Lock
0x64, Restore 0x6c, SetPalette 0x7c, Unlock 0x80) and `LEGOLAND/surface.c`
(`g_ddsd` DDSURFACEDESC 0x6c bytes, `lpSurface`, pitch; the game renders in
software into the locked 16-bpp back surface and the `g_present` callback
flips or blits it). Input is DirectInput-shaped state in `input.c`/`input2.c`;
the message pump uses the `windows.h` slice in `portable/hostwin/include/`.
`docs/RUNTIME_SPEC.md` and `docs/runtime/*.md` are the recovered contracts for
every subsystem; `docs/runtime/assets.md` covers the loaders and host lifecycle.

## PORT-A — close the wasm32 link, run the startup spine under node

Deliverables, in order; commit each:

1. **Fix `gen_link.py --ilp32`.** `emcmake` + `-DLL_ILP32=ON` today fails in
   `gen/globals.c` with 12 `redeclaration with a different type` errors: a
   global emitted as `unsigned int[N]` (because a word in it is re-pointed to
   `(unsigned)&Symbol`) is later declared `extern unsigned char name[]` by the
   alias/re-pointing pass. Emit one consistent declaration per symbol (or a
   forward declaration with the same type). `ninja -C portable/build-wasm
   legoland_linkcheck` must link; `node portable/build-wasm/legoland_linkcheck.js`
   must print its line. Keep the native 64-bit build working (CI runs it).
2. **Host ABI header** `portable/hostwin/include/ll_host.h`: the C declarations
   of every host entry point the shims implement, grouped by DLL, with the
   exact Win32 signatures (stdcall is ignored off x86). PORT-B adds to it.
3. **KERNEL32/ADVAPI32/VERSION shim** in `portable/src/hostwin/kernel32.c`:
   real implementations, not traps, for what the spine and the loaders need:
   mutex/event/wait (single-threaded: mutex always acquired, `WaitForSingleObject`
   returns 0), `GetTickCount`/`QueryPerformanceCounter` (emscripten_get_now),
   `Sleep` (no-op or `emscripten_sleep` under ASYNCIFY — coordinate with PORT-B,
   who decides the main-loop strategy), `GetCommandLineA`, `GetModuleFileNameA`,
   `GetCurrentDirectoryA`/`SetCurrentDirectoryA`, `CreateFileA`/`ReadFile`/
   `WriteFile`/`SetFilePointer`/`GetFileSize`/`CloseHandle`/`FindFirstFileA`
   family (over the POSIX layer `msvcrt.c` already uses; under node use
   `-sNODERAWFS=1` so `gamedata/` is read straight from disk), `GetFileVersionInfo*`
   + `VerQueryValueA` (return a fixed version block), `GlobalAlloc`/`HeapAlloc`
   family over malloc, `OutputDebugStringA` to stderr. `gen_link.py` must stop
   generating a trap for anything kernel32.c defines (it already skips symbols
   the objects define — confirm).
4. **Headless harness** `portable/src/headless/main.c` + `cmake/headless.cmake`
   target `legoland_headless` (wasm32 only; `EXCLUDE_FROM_ALL` is fine): calls
   `WinMain(NULL, NULL, "<switches>", 1)` with `-nointro -nomusic` and
   `WINDEBUG`, with `gen_link`'s trap helper extended so every trap prints
   `TRAP <dll> <symbol> from <caller if known>` and exits non-zero. Run it under
   node with `gamedata/` as cwd. Deliverable is the **ordered list of traps
   hit**, one per commit as you turn each into a real call, in
   `docs/lanes/scope-port-a.md`, until the spine reaches the first host call
   that is PORT-B's (`DirectDrawCreate` or a USER32 window call). Stop there
   and record exactly what state the game is in.
5. Re-run `linkreport.py` on both build dirs; put the before/after census in
   your notes and the README section.

## PORT-B — the Emscripten canvas host shim

Goal: a page that loads the wasm, creates the game's 16-bpp primary surface
as a canvas, feeds keyboard/mouse, and runs the game's own loop. First
milestone is **one frame of the title/front-end screen**; the second is
responding to a click. Deliverables:

1. **Decide and document the main-loop strategy** in `docs/lanes/scope-port-b.md`
   within your first hour, because PORT-A's `Sleep`/`WaitMessage` depend on it.
   Recommended: `-sASYNCIFY` with `emscripten_sleep(0)` inside `PeekMessageA`/
   `WaitMessage`/`Sleep`/`Flip` so the game's synchronous loop yields to the
   browser; switch to `emscripten_set_main_loop` later if needed. Record the
   ASYNCIFY import list you need.
2. **DDRAW shim** `portable/src/hostwin/ddraw.c`: `DirectDrawCreate` returning
   a C object whose vtable matches the slot layout `gpu.c`/`surface.c` call
   (QueryInterface -> IDirectDraw2 with the same layout, SetCooperativeLevel,
   SetDisplayMode, CreateSurface primary/back/offscreen, CreateClipper,
   GetCaps/GetDisplayMode as the game reads them; surface Lock/Unlock/Restore/
   Blt/Flip/SetPalette/GetSurfaceDesc/Release). Pixel format: whatever
   `SetDisplayMode` asks for (expect 640x480x16; check `gpu.c`), stored in a
   malloc'd buffer with the pitch `g_ddsd` expects. `Flip`/primary `Blt`
   presents: convert 16-bpp to RGBA into a canvas `ImageData` via a small JS
   library (`--js-library portable/src/browser/ll_canvas.js`).
3. **USER32/GDI32 shim**: window creation returns a handle, `PeekMessageA`/
   `GetMessageA`/`DispatchMessageA` drive a queue fed from canvas events,
   `SetCursor`/`ShowCursor`, `GetSystemMetrics`, `MessageBoxA` -> console;
   GDI text (`CreateFontA`, `TextOutA`, `GetTextExtent*`) may stub to a fixed
   bitmap font or no-op first, but must not trap.
4. **DINPUT shim**: `DirectInputCreateA` + device objects with the methods
   `input.c`/`input2.c` call (GetDeviceState for keyboard 256 bytes and the
   mouse state struct the game declares). **WINMM**: `timeGetTime`,
   `timeSetEvent` (drive callbacks from the main-loop tick), `timeKillEvent`;
   `midiOut*` return MMSYSERR_NOTSUPPORTED. **DSOUND**: `DirectSoundCreate`
   returns failure so the game runs silent (check that the game tolerates it —
   `docs/runtime/assets.md` sound state).
5. `cmake/browser.cmake`: target `legoland_browser` (wasm32 only) producing
   `legoland.html/.js/.wasm` in the build dir with `index.html` from
   `portable/src/browser/`, assets via `--preload-file ../gamedata@/gamedata`
   for now (a fetch backend later). Document how to serve (`python3 -m
   http.server` in the build dir) and test with the in-app browser.

Until PORT-A lands, build against the current closure: you can link your
targets with `-sERROR_ON_UNDEFINED_SYMBOLS=0` to iterate on the shim; merge
will use PORT-A's closure.

## PORT-C — headless subsystem tests against gamedata/

Goal: the first tests of the recovered code's *behaviour*, independent of
graphics. Oracles come from the clean-room Python decoders already in
`tools/` (`resfile.py`, `leveldata.py`, `tilemap.py`, `comp.py`, `geom.py`,
`audioinfo.py`, `iscab.py`) and from `docs/RUNTIME_SPEC.md` / `docs/runtime/*.md`.
Deliverables:

1. **Pick 5–8 pure entry points** that take a file or buffer and produce a
   checkable result, e.g. the RES archive reader (`res.c` / `memdb.c`), the
   LLIDB loaders (`llidb*.c`), map/level loading (`loadmap.c`, `levelkw*.c`),
   tile-map decode (`map.c`, `tilehelp.c`), path-cost/route (`pathsq.c`,
   `mappath.c`), save-chunk framing (`savechunks*.c`). Read the C and the
   runtime spec page for each; list the globals they need initialised (the
   closure initialises `.data` from the exe, so many are already right).
2. **`tools/oracle_<subsystem>.py`**: emit expected values as JSON/C headers
   from the Python decoders over the real `gamedata/` (committed outputs must
   contain no game assets — hashes, counts, offsets and small derived numbers
   only).
3. **`portable/tests/*.c`** + `cmake/tests.cmake`: one wasm32 executable per
   test (or one driver with subcommands), run under node with `-sNODERAWFS=1`
   and cwd `gamedata/`, linked against `legoland_core` + the generated closure
   (`legoland_gen`). Until PORT-A's closure links on wasm32, develop on the
   native 64-bit build ONLY for tests whose structs have no `long`/pointer
   fields in serialised layouts, and say so per test; everything else waits
   for the wasm32 closure.
4. A `ctest` wiring and a `docs/lanes/scope-port-c.md` table: test, entry
   point, oracle, result, and every divergence found (a divergence between the
   C and the Python decoder is a finding, not a failure to hide — it may be
   the decoder that is wrong; say which and why).

## Integration (integrator only)

Per lane: merge onto main, `ninja -C portable/build` and `ninja -C
portable/build-wasm` both build, `legoland_linkcheck` links on both, audit +
relocs on any `LEGOLAND/*.c` a lane touched, marker-set superset check,
`verify.py` alone, then push. Update this status line at merge.
