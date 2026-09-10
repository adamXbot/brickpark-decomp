# Scope PORT-A — the wasm32 link closure and the startup spine under node

> **Status: deliverables 1–5 done, 2026-09-11.** Branch `scope/PORT-A` from
> `db032c0c`. The wasm32 whole-archive link closes, `legoland_linkcheck` runs
> under node, and `legoland_headless` runs the game's own `WinMain` until the
> first DirectDraw call — which is PORT-B's boundary, so it stops there.
> Nothing in `LEGOLAND/*.c` was touched (no guards were needed), so the VC6
> gate has nothing to check for this lane.

## What this lane changed

| file | what |
| --- | --- |
| `portable/tools/gen_link.py` | one consistent declaration per symbol; wasm-typed stubs, forwarders and data aliases; `TRAP` messages |
| `portable/tools/linkreport.py` | nm selection that can read emcc's objects; the wasm object parser; the prototype-conflict census |
| `portable/hostwin/include/ll_host.h` | **new** — the host ABI, KERNEL32/ADVAPI32/VERSION filled in, the rest left for PORT-B |
| `portable/src/hostwin/kernel32.c` | **new** — all 56 KERNEL32-family imports the game references, for real |
| `portable/src/headless/main.c` | **new** — `WinMain(NULL, NULL, "-nointro -nomusic WINDEBUG", 1)` under node, plus a `--resmount` probe |
| `portable/cmake/headless.cmake` | `kernel32.c` into `legoland_core`; the `legoland_headless` target |
| `.gitignore` | `portable/build/` → `portable/build*/` so every lane's build dir is ignored |

## 1. Why `--ilp32` did not compile, and what the fix really is

The reported symptom was 12 `redeclaration of 'X' with a different type:
'unsigned char[]' vs 'unsigned int[N]'` in `gen/globals.c`. The cause: a global
with a word that equals a known symbol address is emitted as `unsigned int[N]`
(so the word can be written `(unsigned)&Symbol`), but the emitter declared
every symbol it pointed AT as `extern unsigned char name[]`, inline, as it
went — and some of those symbols are themselves re-pointed globals emitted
earlier or later in the same file.

Emitting "a forward declaration with the same type" needs the types to be known
before anything is written, so the generator now **plans** all of globals.c
first (address, size, words-or-bytes per entry), fills the address→symbol map
completely, and only then emits: one `extern` prologue with the planned types,
then the definitions. Two things fell out of that:

* the map is complete when the first global is emitted, so **forward**
  references re-point too — 243 symbols are now pointed at, where the
  as-you-go map could only ever catch backward ones;
* all 381 data aliases move into globals.c, because clang's `alias` attribute
  needs the aliasee in the same translation unit and that is the only form of
  data alias wasm has (`__asm__(".set ...")` at module level crashes the wasm
  backend outright, and the attribute with a cross-TU target is rejected).

Then the link found three more things that only a typed target can object to.
None of them are ILP32 issues; they are "Mach-O does not care" issues:

1. **No cross-TU function alias either**, so each of the 228 stale extern names
   becomes a forwarder. A forwarder is only callable if its signature is the
   one the callers emitted, so gen_link now reads the signature each undefined
   symbol is imported with, and the signature each defined body really has,
   straight out of the wasm objects (type + import + function + `linking`
   symbol-table sections — `linkreport.wasm_object_sigs`). Where the two
   disagree the forwarder calls through a cast.
2. **The same applies to every trapping stub.** With a mismatched signature
   wasm-ld silently replaces the call with its own trapping stub, so
   `TRAP DDRAW DirectDrawCreate from gpu.c` would have arrived as
   `RuntimeError: unreachable` with no name — the whole point of the trap
   gone. Stubs are signature-matched now.
3. **Function and data are different kinds of symbol.** A re-pointed word that
   names a function has to be declared as a function (a function pointer is a
   table index, not an address), and an unclassified extern the sources use as
   data needs storage rather than a body: `wasm-ld: error: symbol type
   mismatch`. The kind comes from the objects too.

`nm` matters as well: the objects emcc 6.0.9 writes use a symbol flag the nm
Xcode ships rejects (`invalid symbol type: 16`) — and it rejects it only for
some objects, so it half-works. `linkreport.nm_candidates()` asks `em-config
LLVM_ROOT` for Emscripten's own llvm-nm first, `$LL_NM` overrides, and a
failure restarts the scan with the next candidate rather than quietly losing
symbols.

Result:

```
$ emcmake cmake -S portable -B portable/build-wasm -G Ninja -DLL_ILP32=ON -DPython3_EXECUTABLE=$PY
$ ninja -C portable/build-wasm legoland_linkcheck
$ node portable/build-wasm/legoland_linkcheck.js
legoland_linkcheck: every symbol resolved
```

and the native 64-bit build still links and runs (`ninja -C portable/build
legoland_linkcheck`).

## 2–3. The host ABI header and the KERNEL32 shim

`portable/hostwin/include/ll_host.h` declares every host entry point, grouped
by DLL, with the Win32 signatures; PORT-B adds its blocks. The note at the top
is the one thing to read before writing a shim: the game declares the same
import with different types in different files (`int CreateFileA(...)` in
data2.c, `void* CreateFileA(...)` in loaders.c), which is harmless because both
are i32 on wasm32 — but the *shape* of the signature must match exactly.

`portable/src/hostwin/kernel32.c` implements all 56 of the KERNEL32 / ADVAPI32
/ VERSION imports the game references. The decisions worth knowing:

| area | decision |
| --- | --- |
| mutex/event/wait | single-threaded: a mutex is always free, `WaitForSingleObject` returns `WAIT_OBJECT_0`. startup.c reads `WAIT_TIMEOUT` as "already running", so this is the "we are the first instance" answer. An auto-reset event is still consumed by a wait. |
| `CreateThread` | **refuses** (returns 0, `GetLastError` 120). The music and streaming threads would never be scheduled, so the game keeping that work on the main thread is the honest behaviour. |
| time | one monotonic clock (`emscripten_get_now`, else `clock_gettime(CLOCK_MONOTONIC)`) behind `GetTickCount` and `QueryPerformanceCounter`; QPF is 1 MHz so movie2.c's 32-bit arithmetic survives. |
| `Sleep` | **no-op plus the `ll_host_yield` hook** — see the note to PORT-B below. |
| files | POSIX `open`/`read`/`write`/`lseek`; handles are a small table (index+1, so never 0 and never -1, both of which the game tests). Backslashes normalised. |
| drives | `$LL_CD_DIR` makes the shim present drive D: as a CD-ROM labelled `LEGOLAND` on CDFS, mapping `D:\x` to that directory; unset, there is no CD. |
| `Global*`/`Local*` | malloc with an identity lock. |
| `LoadLibrary*` | refuses — the AVI and DirectMusic paths that ask are stubs anyway. |
| VERSION | a fixed version block. `ReadExeVersionString` asks for a size, reads the block, `VerQueryValue`s one string out of it, and the string only ever reaches the crash log's header. |
| `IsBadReadPtr` | null check only: a wasm linear-memory read cannot fault the way a Win32 one can. |

`FindFirstFileA` is NOT implemented and is not missing: it is not in the import
table, because the game enumerates directories with the CRT `_findfirst`
family, which `msvcrt.c` already covers.

`LL_HOST_TRACE=1` prints one line per interesting host call (not
`GetTickCount`, which would bury everything).

## 4. The headless run, and the ordered list of host calls

```
$ ninja -C portable/build-wasm legoland_headless
$ LL_HOST_TRACE=1 LL_DATA_DIR=$PWD/gamedata/main node portable/build-wasm/legoland_headless.js
legoland_headless: data directory /.../gamedata/main
legoland_headless: WinMain(NULL, NULL, "-nointro -nomusic WINDEBUG", 1)
HOST GetFileVersionInfoSizeA("Legoland.exe")
HOST GetFileVersionInfoA
HOST VerQueryValueA("\StringFileInfo\080904B0\ProductVersion")
HOST CreateMutexA("LegolandGameMutex")
HOST WaitForSingleObject(1, 0)
TRAP DDRAW.dll DirectDrawCreate from gpu.c          # exit 70
```

`$LL_DATA_DIR` is a `chdir` in the harness: the loaders build relative paths
(`.\volumes\%s.res`), and `gamedata/` is a symlink in a worktree, so cd-ing to
it leaves the checkout.

That trace IS the ordered list of host calls the spine makes, and every one of
them would have been a trap before this lane. The order, against the code:

| # | call | from | resolved by |
| --- | --- | --- | --- |
| 1 | `GetFileVersionInfoSizeA` | `WinMain` → `ReadExeVersionString` (gameframe.c) | fixed version block |
| 2 | `GetFileVersionInfoA` | same | copies the block |
| 3 | `VerQueryValueA` | same | returns the one string in it |
| 4 | `CreateMutexA("LegolandGameMutex")` | `GameMain` (startup.c) | handle table |
| 5 | `WaitForSingleObject(mutex, 0)` | same, the one-instance test | `WAIT_OBJECT_0` |
| 6 | `DirectDrawCreate` | `CheckHostSystemGPU` (util.c) → `InitHostSystemGPU` (gpu.c) | **PORT-B** |

The file name in call 1, `Legoland.exe`, comes from `g_str_exe_name` in the
rebuilt `.data`, which is a nice incidental check that the globals and the
ILP32 re-pointing are doing their job.

**State of the game at the stop:** `g_exe_version` filled; the instance mutex
held; `g_present = FlipPrimary` and `g_windowed = 1` (WINDEBUG);
`g_map->no_intro = 1`; `g_music_sys = 0` (-nomusic); `g_hinstance = NULL`,
`g_ncmdshow = 1`; `g_gpu_state` (0x00667d70, 0x3d8 bytes) freshly zeroed by
`CheckHostSystemGPU`. Nothing has been loaded and no file has been opened.

### `--resmount`: the loaders, behind the DirectDraw wall

`InitSession` mounts the resource volumes *after* `CheckHostSystemGPU`, so a
whole-game run cannot reach them until PORT-B lands. The harness calls them
directly to test the file and drive half of the shim:

```
$ LL_HOST_TRACE=1 LL_CD_DIR=$PWD/gamedata/disc LL_DATA_DIR=$PWD/gamedata/main \
  node portable/build-wasm/legoland_headless.js --resmount
HOST GetLogicalDrives
HOST GetDriveTypeA("C:\") = 3
HOST GetDriveTypeA("D:\") = 5
HOST GetVolumeInformationA("D:\"): LEGOLAND on CDFS
legoland_headless: volumes mounted, g_res_path="D:\"
TRAP GAME 0x0049e4ff MemAlloc from blokeai.c, bnvmove.c, data2.c...
```

`MemAlloc` is now forwarded rather than trapped (see below), so the same probe
gets one step further — and straight into the next thing:

```
legoland_headless: volumes mounted, g_res_path="D:\"
HOST CreateFileA(".\volumes\.res", access=0x80000000, disp=3)
HOST CreateFileA("D:\.res", access=0x80000000, disp=3)
legoland_headless: RES_OpenVolume("") = 0
```

Both paths are right except for the volume NAME, which is empty.
`g_volume_names` (0x004bcba4) is a table of three `const char*` pointing at
string literals in `.rdata` — and those literals are not named by any extern,
so `--ilp32` has no symbol to re-point the words at and leaves the original
0x004b… values, which are not addresses in wasm linear memory. **Pointers to
unnamed data are the next limit of the re-pointing pass**, and the fix belongs
in gen_link: it already computes the gaps between known addresses, so a word
pointing into a gap can get a synthetic `ll_gap_<start>` block (initialised
from the exe like any other global) and be re-pointed at
`&ll_gap_<start>[X - start]`. Nothing in the startup spine depends on it;
everything in the loaders does. Left open (see below).

So `RES_EnsureMounted` is satisfied and `g_res_path` is set. Without
`LL_CD_DIR` it fails instead — and on the real startup path it would loop on
`MessageBoxA("Insert CD")` forever, because sysmisc.c's loop has no other exit
(`while (!RES_FindVolumeOnAnyDrive(kVolLegoland))`), so **PORT-B's `MessageBoxA`
must be able to return `IDCANCEL`**.

The three volumes live in `gamedata/disc` (`Legoland.res`, `Graphics1.res`,
`Graphics2.res`), not in `gamedata/main`, which is why the CD emulation is
there at all. `gamedata/main` has no `volumes/` directory, so the first path the
game tries (`.\volumes\%s.res`) always fails and the `%s%s.res` + `g_res_path`
fallback is the one that matters.

## 5. Census, before and after

`linkreport.py` over the clang objects (`portable/build`, native 64-bit) and
the emcc objects (`portable/build-wasm`, wasm32 `-DLL_ILP32=ON`):

| category | native before | native after | wasm32 after |
| --- | --- | --- | --- |
| game-fn (all 12 were CRT thunks; now forwarded, not trapped) | 12 | 12 | 12 |
| game-data | 2613 | 2613 | 2613 |
| alias | 228 | 228 | 228 |
| **host** | **149** | **95** | **95** |
| crt | 57 | 57 | 46 |
| unknown | 83 | 83 | 88 |
| duplicates | 0 | 0 | 0 |
| asm stubs | 26 | 26 | 26 |
| **prototype conflicts** | n/a | 0 | **542** |

(The wasm column had no "before": the build did not compile. `crt` and
`unknown` differ between the two because emcc's libc provides a few names
Apple's does not and vice versa.)

`gen/manifest.md` on the wasm build:

```
- object format: wasm (signature-matched stubs and forwarders)
- globals defined: 2232 (3705348 bytes), data aliases: 381
- symbols re-pointed into (ilp32): 243
- function aliases (stale extern names): 228
- unwritten game function stubs: 0
- CRT thunks forwarded instead of trapped: 12
- unresolved-name stubs: 44 (+44 placeholder data blocks)
- host API stubs: 95
- symbols imported with more than one signature (most common wins): 54
```

### Finding: 542 prototype conflicts inside the game

`linkreport.py` has a new row and section for a defect class that only a typed
target exposes. One source declares `extern void SetupControllers(void)` and
calls it; another defines `int SetupControllers(void)`. In x86 cdecl the caller
ignores EAX and nobody notices. On wasm the call is type-checked, so wasm-ld
routes it through a stub that traps — `RuntimeError: unreachable`, no symbol
name, at whatever frame the game happens to be in.

542 (symbol, disagreeing declaration) pairs; wasm-ld warns about 130, the ones
that survive its dead-code pass. Many are only a return type, but plenty are
arity: `AddBasicObject` is defined with three parameters in objmap2.c and
called with two from 21 files; `AddBasicPath` is defined with two and called
with none from screen.c. **Each of these is a latent runtime trap for PORT-B**,
and each is either a one-line `#ifdef LEGOLAND_PORTABLE` prototype in the
caller or — more often, I suspect — a matching-lane correction, because one of
the two prototypes is simply wrong about the recovered function. The full table
is the "Prototype conflicts" section of `linkreport.md`.

## For PORT-B

1. **`Sleep` is a no-op with a hook, because your notes did not exist yet.**
   `docs/lanes/scope-port-b.md` was not in `origin/main` when this lane ran, so
   `kernel32.c` does the safe thing: `Sleep(ms)` calls `ll_host_yield(ms)` if
   that function pointer is set and otherwise does nothing. Set it from your
   shim's init:

   ```c
   static void ll_browser_yield(unsigned int ms) { emscripten_sleep(ms); }
   ll_host_yield = ll_browser_yield;    /* declared in ll_host.h */
   ```

   Nothing in the startup spine sleeps, so the no-op cannot hurt before you
   arrive. The loop that will need it is in `gamemain.c` around line 330:
   `while (g_music_disabled == 0) { PeekMessageA(&msg, 0, 0, 0, 0); Sleep(100);
   progress_tick(); }` — a synchronous spin with a `PeekMessageA` and a
   `Sleep(100)` in it, which is exactly the ASYNCIFY shape your brief
   recommends. `WaitMessage` is declared in `hostwin/include/windows.h` and is
   yours; if you would rather yield there than in `Sleep`, leave the hook unset.
2. **Put your shim sources in `legoland_core`, from `browser.cmake`:**
   `target_sources(legoland_core PRIVATE .../src/hostwin/ddraw.c)`. gen_link
   reads `legoland_core`'s objects to decide what still needs a trap, so a shim
   in a library of its own gets a generated trap AND your definition — a
   duplicate symbol. `headless.cmake` does this for `kernel32.c`; copy it.
   (Nobody may edit `portable/CMakeLists.txt`.)
3. **Your signatures must match the call sites**, not just the Win32 docs: wasm
   calls are type-checked and a mismatch becomes a silent trapping stub. Check
   what the game declares — e.g. `DirectDrawCreate` is declared in gpu.c, and
   the vtable slots it calls are listed at the top of that file. If you get a
   `RuntimeError: unreachable` with no `TRAP` line, suspect this first and look
   for `wasm-ld: warning: function signature mismatch` in the build log.
4. **`MessageBoxA` must be able to return `IDCANCEL`** (2), or the missing-CD
   loop in sysmisc.c never ends. And `WNDENV_Minimise` / `WNDENV_Restore` /
   `WNDENV_Gethwnd` are game functions that will reach your USER32 shim.
5. **Declare your entry points in `ll_host.h`** (its last section is a
   placeholder for exactly that) and keep the "same import, two types" note in
   mind.
6. `LL_HOST_TRACE=1` traces the KERNEL32 side; a matching trace in your shim
   would make the two logs interleave usefully.

## For PORT-C

* The wasm32 closure links now, so the "develop on the native build only"
  caveat in your brief is lifted: link `legoland_core` + `legoland_gen` the way
  `headless.cmake` does.
* Run with `LL_DATA_DIR`-style `chdir` (or your own) into `gamedata/main`, and
  `LL_CD_DIR=$PWD/gamedata/disc` if the test needs the `.res` volumes.
* `MemAlloc` and the other eleven CRT thunks are forwarded now, so allocation
  works. The open limit is pointer tables of unnamed data (item 1 under "Left
  open"): a test that reads a string table out of `.data` will see rubbish,
  while one handed a buffer or a file path of its own will not.

## The 12 "unwritten game functions" were CRT thunks: 12 → 0

`MemAlloc`/`HeapAlloc_w` at 0x0049e4ff, `MemFree`/`HeapFree_w`/`RES_FreeFile`/
`ReleaseAnimInstance` at 0x0049e4d0, `Format`/`sprintf_w` at 0x0049e573,
`CRT_calloc` (0x004a020e), `rand_w` (0x0049e4b2), `NameCompare` (0x004aab90),
`DebugPrint` (0x0049e5c5) — every one of them is a second name for a CRT
function the sources ALSO declare at that same address (startup.c has `malloc`
at 0x0049e4ff, `free` at 0x0049e4d0, `sprintf` at 0x0049e573; anim2.c has
`rand` at 0x0049e4b2; audio3.c has `_stricmp` at 0x004aab90; bigrender.c has
`printf` at 0x0049e5c5). So they are exactly the "stale extern name" case, and
gen_link now routes them through the alias machinery:

```
- unwritten game function stubs: 0
- CRT thunks forwarded instead of trapped: 12 (CRT_calloc -> calloc,
  DebugPrint -> printf, Format -> sprintf, HeapAlloc_w -> malloc,
  HeapFree_w -> free, MemAlloc -> malloc, MemFree -> free,
  NameCompare -> _stricmp, RES_FreeFile -> free,
  ReleaseAnimInstance -> free, rand_w -> rand, sprintf_w -> sprintf)
```

The printf-shaped ones work because a variadic callee has a fixed wasm
signature (its arguments arrive through one pointer), so forwarding what the
caller passed is exactly right. `linkreport.py` still files them under
"Unwritten game functions", which is now a misnomer for all 12 — the report
lists them, the generator no longer traps them. That is the row the old
`portable/README.md` census called "CRT-range wrappers filed as game-fn: 12".

## Left open

1. **Pointers to unnamed data are not re-pointed** (`--ilp32`): the word in
   `g_volume_names` that should point at the string `"Legoland.res"` keeps the
   original 0x004b… value, because no extern names that literal. That is why
   `--resmount` opens `"D:\.res"` instead of `"D:\Legoland.res"`. The fix is
   the one sketched in the `--resmount` section: synthetic `ll_gap_<start>`
   blocks for the gaps between known addresses, re-pointing into them with an
   offset. Until then, anything that reads a pointer table of unnamed data
   (string tables above all) sees rubbish — this is the single biggest thing
   between the port and a loader that works.
2. **44 unclassified externs used as data get a 256-byte placeholder block**
   (`UNKNOWN_DATA_SIZE`) rather than their real storage, because nothing knows
   their address or size. They keep the link closed; a write through one is
   isolated from the rest of `.data`, so a subsystem that depends on one will
   misbehave quietly. They are in `linkreport.md`'s "Unclassified" table.
3. **The 542 prototype conflicts** above are untouched: they are edits to
   `LEGOLAND/*.c` prototypes, which needs a matching lane's judgement about
   which of the two prototypes is right.
4. **130 `wasm-ld: warning: function signature mismatch` lines** in every wasm
   link are the subset of (3) that survives dead-code elimination. They are
   warnings, not errors, and the link closes; do not add
   `-Werror`-equivalents until (3) is done.
5. `GetDriveTypeA` reports DRIVE_FIXED for everything but the emulated CD, and
   `GetVolumeInformationA` fails for it. Nothing the spine does cares.
