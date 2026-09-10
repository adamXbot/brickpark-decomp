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
