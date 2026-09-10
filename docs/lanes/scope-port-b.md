# Lane PORT-B — the Emscripten browser-canvas host shim

Brief: `docs/SCOPE_PORT_WAVE.md` "## PORT-B". Branch `scope/PORT-B`.
Owned files: `portable/cmake/browser.cmake`, `portable/src/browser/**`,
`portable/src/hostwin/{ddraw,user32,gdi32,dinput,winmm,dsound}.c`, additions to
`portable/hostwin/include/ll_host.h`, this note, and the PORT-B section of
`portable/README.md`. `LEGOLAND/*.c` is read-only for this lane; anything the
game itself would have to change is in [§8 For the integrator](#8-for-the-integrator).

---

## 1. Main-loop decision: ASYNCIFY, with the yields in the host shim

**Decision: `-sASYNCIFY`, and every blocking construct the game already has
yields to the browser from inside the host shim. The game's `while` loops are
not touched and `emscripten_set_main_loop` is not used.**

PORT-A's `Sleep` and `WaitMessage` must follow this contract; it is why this
section is the lane's first commit.

### Why the game cannot be driven by `emscripten_set_main_loop`

`emscripten_set_main_loop` needs a function that renders exactly one frame and
returns. The recovered game has no such function. Its frame is produced inside
nested synchronous loops that only exit on a time or state condition:

| site | loop | exit condition |
| --- | --- | --- |
| `LEGOLAND/sysmisc.c:572` `FlipPrimary` | `while (timeGetTime() - g_flip_time < 0x1c) ;` | 28 ms of wall clock has passed |
| `LEGOLAND/blitmisc.c:325` `PresentFlip` | same 28 ms spin | same |
| `LEGOLAND/blitmisc.c:342` `PresentFlip` | `while (GetFlipStatus(...) != 0)` | the flip has retired |
| `LEGOLAND/input.c:312` `ProcessSystemEvents` | `while (PeekMessageA(..., PM_NOREMOVE))` | the message queue is empty |
| `LEGOLAND/input.c:318` `ProcessSystemEvents` | `do { ... WaitMessage(); } while (g_game->flags & 1)` | the window regains focus |
| `LEGOLAND/input.c:306` `ScanKeyboard` | `while (1) { GetDeviceState... }` | `GetDeviceState` returns `DI_OK` |
| `LEGOLAND/gamemain.c:330` `RunGame` | `while (g_music_disabled == 0) { PeekMessageA; Sleep(100); }` | the music thread published itself |

Restructuring any of those means editing `LEGOLAND/*.c`, which this lane may
not do, and which the matching build would reject anyway. ASYNCIFY is the only
strategy that runs this control flow unchanged: it rewrites the wasm so a call
to `emscripten_sleep` unwinds the whole C stack into a side buffer, returns to
the browser event loop, and rewinds the stack on the next tick. From the game's
point of view the spin simply took 28 ms.

### Where the shim yields

Every yield goes through `emscripten_sleep()`, which is already in Emscripten's
default `ASYNCIFY_IMPORTS`, so **no custom `ASYNCIFY_IMPORTS` entry is
needed**. The single helper is `ll_host_yield(unsigned ms)` in
`portable/src/hostwin/user32.c`, declared in `ll_host.h`; under a non-wasm
build it is a no-op so the native build still compiles.

| host entry | yield | reason |
| --- | --- | --- |
| `PeekMessageA` | yield once when it is about to return 0 (queue empty) | this is exactly once per `ProcessSystemEvents`, i.e. once per frame: the inner `while` drains the queue without yielding, then exits through the one yield |
| `GetMessageA` | yield while the queue is empty (it is specified to block) | the game only calls it after a successful `PeekMessageA`, so this normally does not fire |
| `WaitMessage` | `ll_host_yield(16)` and return | the game is deliberately idling for focus; one animation frame per spin |
| `Sleep(ms)` (PORT-A, kernel32.c) | `emscripten_sleep(ms)` — **never a no-op** | `RunGame`'s music wait is `PeekMessageA; Sleep(100);` with no other yield. A no-op `Sleep` makes that a hard browser hang whenever `g_music_disabled` is still 0 |
| `timeGetTime` | yield if ≥ 4 ms of wall clock has passed since the last yield | this is what makes the two 28 ms present spins cheap; it bounds "time with the event loop blocked" to ~4 ms everywhere in the program, including any spin not listed above |
| `GetFlipStatus` | yield, then report the flip done | turns `PresentFlip`'s status spin into one yield |
| `Flip` / primary `Blt` | push pixels to the canvas, then yield | the browser only composites the canvas once control returns to it, so the frame is not visible without this |

`timeGetTime` yielding is the load-bearing one: it means *any* wall-clock spin
in the game — including ones not yet found — cannot lock the page for more than
about 4 ms. The cost is that ASYNCIFY must instrument essentially the whole
program (no `ASYNCIFY_ONLY` whitelist), because a yield can happen at any
depth.

### Consequences the whole wave has to respect

1. **Re-entrancy is forbidden while unwound.** Between the unwind and the
   rewind, JS runs but wasm must not be called. So:
   - canvas event handlers (`keydown`, `mousemove`, `mousedown`, `wheel`) only
     push records into a plain JS array. They never call into wasm.
   - `timeSetEvent` callbacks are **not** driven from a JS `setInterval` —
     that would re-enter wasm mid-unwind. They are dispatched synchronously
     from the message pump (see §5), which is what the brief means by "the
     main-loop tick".
2. **`-sASYNCIFY_STACK_SIZE` must be raised.** The default 4096 bytes is the
   side buffer for the unwound stack; the game's render path is deep (front
   end → print list → sprite blitter) and a yield inside `timeGetTime` can
   happen at any of those depths. `browser.cmake` sets 131072 and the page
   reports an ASYNCIFY overflow clearly rather than corrupting memory.
3. **Build cost.** ASYNCIFY instrumentation roughly doubles code size and costs
   ~20–40% speed. That is the price of not editing the game. If it becomes the
   bottleneck, the successor is **JSPI** (`-sJSPI`), which does the same job
   with stack switching in the engine instead of a rewrite; it needs a
   recent Chrome. The shim's yield points do not change, only the flag — that
   is why all of them funnel through `ll_host_yield`.
4. **`EXIT_RUNTIME=0`** and the game entered from `main()`: with ASYNCIFY,
   `main` itself is allowed to unwind, and the runtime stays alive.

### Rejected alternatives

- `emscripten_set_main_loop` + restructuring the game loop: needs
  `LEGOLAND/*.c` edits. Rejected by the lane's rules, and the 28 ms spin plus
  the focus-wait `do/while` cannot be expressed as a one-frame callback without
  a state machine in the game.
- Pthreads + `SharedArrayBuffer`, game on a worker, synchronous blocking real:
  the cleanest long-term answer and it needs no ASYNCIFY, but it needs COOP/COEP
  headers on the host, a proxied canvas, and the game's `CreateThread` calls
  (music thread) to be real. Too much for a first running page; revisit after
  the first frame renders.
- No-op `Sleep` + hoping the spins are short: `RunGame`'s music wait is an
  unbounded `while`, so this hangs the tab. Rejected on evidence.

---

## 2. Status

| deliverable | state |
| --- | --- |
| 1. main-loop decision | **done** — §1, committed first (`f36d0d34`) |
| 2. DDRAW shim + canvas present | **done** — §3; verified running in the browser |
| 3. USER32 / GDI32 shim | **done** — §4 |
| 4. DINPUT / WINMM / DSOUND | **done** — §5; verified running in the browser |
| 5. `browser.cmake` + page + serve/test | **done for `legoland_shimtest`; wired but blocked for `legoland_browser`** — §6 |

Nothing the shim owns traps. `closure_filter.py` gives the census: of the 149
Win32/DirectX traps `gen_link.py` generates, **70 are now provided by the shim
and 0 remain in PORT-B's six DLLs**; the 79 left are KERNEL32 50, AVIFIL32 16,
MSACM32 6, VERSION 3, ole32 2, ADVAPI32 1, WINSPOOL 1 — PORT-A's half plus the
movie / ADPCM / DirectMusic / printing stubs.

---

## 3. DDRAW — `portable/src/hostwin/ddraw.c`

### Vtables, and the line that pins each slot

The game never names a method; it indexes a vtable at a byte offset recovered
from the disassembly. A slot at the wrong offset is a call into the wrong
function with the wrong arguments, so each one is justified:

| slot | method | called from |
| --- | --- | --- |
| **IDirectDrawSurface** | | |
| 0x08 | Release | `gpu.c:118`, `screen.c:1231`, `sprite2.c:54`, `printlist.c:101` |
| 0x14 | Blt | `gpu.c:120`, `bigrender.c:180`, `sysmisc.c:513` |
| 0x1c | BltFast | not called; implemented over Blt |
| 0x2c | Flip | `blitmisc.c:51` |
| 0x44 | GetDC | `text.c:44`, `movie.c:82`, `frontend2.c:87`, `bubblecache.c:34`, `fpui4.c:522`, `renderview.c:1619`, `unref5.c:101` |
| 0x48 | GetFlipStatus | `blitmisc.c:53` |
| 0x58 | GetSurfaceDesc | `bigrender.c:183` |
| 0x60 | IsLost | `bigrender.c:185` |
| 0x64 | Lock | `surface.c:106`, `bigrender.c:186`, `printlist.c:103` |
| 0x68 | ReleaseDC | `text.c:46`, `movie.c:84`, `bubblecache.c:36`, `fpui4.c:524` |
| 0x6c | Restore | `surface.c:112`, `gpu.c:123`, `sysmisc.c:518`, `savechunks.c:192` |
| 0x70 | SetClipper | `gpu.c:124`, `screen.c:1233` |
| 0x74 | SetColorKey | `sprite2.c:56`, `bigrender.c:191`, `bubblecache.c:38` |
| 0x7c | SetPalette | `spritemisc.c:37`, `rin.c:202` |
| 0x80 | Unlock | `surface.c:114`, `gpu.c:126`, `blitmisc.c:60` |
| **IDirectDraw / IDirectDraw2** | | |
| 0x00 | QueryInterface | `gpu.c:139` (asks for `IID_IDirectDraw2`) |
| 0x08 | Release | `gpu.c:141`, `gpu.c:148` |
| 0x0c | Compact | `sprite2.c:84` |
| 0x10 | CreateClipper | `screen.c:1249` |
| 0x14 | CreatePalette | `rin.c:194` |
| 0x18 | CreateSurface | `screen.c:1251`, `bigrender.c:200`, `sprite2.c:86` |
| 0x2c | GetCaps | `gpu.c:150` |
| 0x30 | GetDisplayMode | `sysmisc.c:280` |
| 0x50 | SetCooperativeLevel | `screen.c:1253` |
| 0x54 | SetDisplayMode | `sysmisc.c:281` |
| **IDirectDrawClipper** | | |
| 0x1c | SetClipList | `gpu.c:133`, `screen.c:1240` |
| 0x20 | SetHWnd | `screen.c:1241` |

Those offsets are exactly the DirectX 1 ABI, so the shim emits the **whole**
standard vtable as a 4-byte-per-slot array (`long (*)(void)`), not only the
slots in the table. Every slot the game never calls points at a function
returning `DDERR_UNSUPPORTED`, so a mis-recovered offset shows up as an
unsupported call rather than an arbitrary jump. On wasm32 a function pointer is
4 bytes, which is what makes the array match the game's structs; on the 64-bit
native build it does not, and that build is a census, not a run target
(`portable/README.md`, "ILP32 is the real target").

### Mode and pixel format

`SetScreenDisplayMode` (`sysmisc.c` 0x00463ef0) asks for
`g_screencfg->w x g_screencfg->h x 16`, then *reads the format back*:

```c
case 16: g_screen_depth = (desc.ddpf.dwGBitMask == 0x7e0) + 1;
```

`InitScreen`'s fullscreen arm independently writes `g_screen_depth = 2`
(`screen.c:1375`). The two agree only if the host reports **RGB565**, so
`GetDisplayMode` reports R `0xf800` / G `0x07e0` / B `0x001f` at 16 bpp, and
`SetDisplayMode` **refuses 8 bpp** (the game asks for 16 first and 8 only as a
fallback). Reporting 565 also keeps the paletted path out of the picture
entirely: `LoadColourTable`, `ResendPalette` (`spritemisc.c:171`) and `rin.c`'s
`CreatePalette` are all guarded by `g_screen_depth == 0`.

Surfaces are `calloc`'d linear 16-bpp buffers with **pitch = width × 2**. That is
what `Lock` publishes in `DDSURFACEDESC::lPitch` (+0x10) and `::lpSurface`
(+0x24) — the two fields `surface.c` reads back as the separate globals
`g_ddsd_pitch` (0x006680ac) and `g_ddsd_bits` (0x006680c0) — along with
`dwWidth`/`dwHeight`, which `text.c` reads as `g_ddsd_width`/`g_ddsd_height`.
`dwSize` is left alone: the caller presets it to 0x6c.

### What presents the frame

The fullscreen path is **not** a flip chain. `InitScreen` creates the primary
with caps `0x4200` (PRIMARYSURFACE|VIDEOMEMORY), no `DDSCAPS_FLIP`, no
`dwBackBufferCount`, and sets `g_present = FlipPrimary` (`screen.c:1394`), which
*Blts* `g_surface_78` onto the primary (`sysmisc.c:577`). So the rule in the
shim is:

> **a `Blt` whose destination is the primary surface pushes the primary's pixels
> to the canvas.**

`ll_host_present16` → `ll_js_present16` converts RGB565 to RGBA into a reused
`ImageData` and `putImageData`s it, then yields so the browser composites.
Channel expansion replicates the top bits into the low ones
(`(r << 3) | (r >> 2)`), which is what keeps white white instead of `0xf8`.
`Flip` (blitmisc.c's `PresentFlip`, the `.data` default `InitScreen` overwrites)
is implemented too and presents the back buffer; `GetFlipStatus` yields once and
reports the flip retired, turning that spin into one browser turn.

`Blt` itself does colour fill (`DDBLT_COLORFILL`, `gpu.c`'s `RenderBlock`), plain
copy, source-colour-keyed copy (`DDBLT_KEYSRC`, `gpu.c`'s `RenderSprite` for
sprites with flag 0x40) and a nearest-neighbour stretch when the rects differ in
size (`bigrender.c:554`'s scaled blit is the only caller that needs it). The
clipper is recorded but not honoured: every caller has already intersected its
rect with `g_clip_rect`, and `Blt` clamps to the surface anyway.

---

## 4. USER32 / GDI32 — `user32.c`, `gdi32.c`

**Real, not stubbed** (the renderer is built on them): `IntersectRect`,
`OffsetRect`, `PtInRect`, `wsprintfA`/`wvsprintfA`, `GetClientRect`,
`GetKeyState`, the message queue and the pump. Win32 rect semantics are
half-open; the game's own rects are *inclusive*, which is why `surface.c` builds
`{0,0,w-1,h-1}` before intersecting — that asymmetry lives in the game and is not
"fixed" here.

Load-bearing details:

- `GetWindowLongA(hwnd, GWL_HINSTANCE)` must be non-null: `InitInputSystem`
  (`input2.c:266`) passes it straight to `DirectInputCreateA` unchecked.
- `GetClientRect` reports origin (0,0) and `ClientToScreen` adds nothing, so
  `FlipPrimary`'s `{0,0,640,480}` destination stays over the canvas.
- `RegisterClassExA` remembers `lpfnWndProc` (+0x08 of WNDCLASSEXA) so
  `DispatchMessageA` can call `LegoLandWindowProc` (`input2.c` 0x0047fe90). Focus
  events reach it and drive `g_game->flags` bit 0, which is the flag the pump
  idles on in `WaitMessage`.
- `TranslateMessage` is a no-op: the only `WM_CHAR` the game acts on is backspace
  (`if ((char)wp == 8)`), and the event translation synthesises that directly.
  Typed text reaches the game through the DirectInput key array (`GetTypedChar`,
  `input2.c`), not through `WM_CHAR`.
- `MessageBoxA` prints to the console and returns IDOK. `InitSession` reports
  every fatal startup failure through it, so those stay visible.

GDI32 is three kinds of thing, none of which traps: **handle factories** (a
distinct non-null cookie per object, because the game stores the four fonts and
`DeleteObject`s them at shutdown), **no-op drawing** that reports success, and
**printing that reports failure** so the print path aborts at its first check.

---

## 5. DINPUT / WINMM / DSOUND

**DINPUT has to work, not degrade.** `InitSession` (`startup.c:156`) aborts the
whole session when `InitInputSystem` returns 0, and that needs both
`CreateKeyboardDevice` and `CreateMouseDevice` to succeed. Three things the shim
has to get right:

- `GetCapabilities` must report `dwFlags != 0`. `CreateMouseDevice`'s test is
  spelled `((caps.dwFlags == 0) & 1) == 0` (`sysmisc.c:245`) — "flags are
  non-zero". `DIDC_ATTACHED` (1) is set.
- `GetDeviceState` must eventually return `DI_OK`: both `ScanKeyboard`
  (`input.c:306`) and `ScanMouse` (`input.c:123`) loop on it forever otherwise.
- `DIPROP_GRANULARITY` of `DIMOFS_Z` is kept in `g_wheel_granularity` and
  `ScanMouse` compares `lZ` against ± it. Reporting **120** and emitting ±120 per
  wheel notch from JS is what makes one notch one icon-bar scroll.

The mouse axes are **relative**: the shim accumulates deltas and zeroes them on
read, which is what DirectInput's relative mode does. The browser only reports
absolute pointer positions, so `ll_canvas.js` differences them (see §7 for the
divergence that introduces).

**WINMM.** `timeGetTime` is the port's second yield point (§1). `timeSetEvent`
callbacks are dispatched from `ll_host_pump_timers`, called by the message pump —
not from a JS timer, which would re-enter wasm mid-unwind. A timer's period is
therefore a lower bound: the pump runs about once per presented frame (the 28 ms
floor gives ~35 Hz), so `InitMIDIManager`'s 20 ms `TIME_PERIODIC` timer
(`lifecycle.c` 0x00480630) actually ticks every ~28 ms. A backgrounded tab does
not build a backlog: one tick per pass, next due measured from now. `midiOut*`
return `MMSYSERR_NOTSUPPORTED` (8), which `InitMIDIManager` ignores — it reports
success regardless of those results.

**DSOUND** returns `DSERR_NODRIVER` and the game runs silent. The tolerance chain
was checked in the sources before it was written: `InitSoundSampleSystem`
(`audio4.c` 0x00492130) clears `g_dsound` and `g_samples_ready` and returns 0 →
`InitSoundSystem` propagates the 0 → **`RunGame` ignores the result**
(`gamemain.c:317`). Every later sample entry is guarded by `g_samples_ready`,
including `KillSoundSampleSystem`, which returns before it would dereference the
null `g_dsound`.

---

## 6. Building, serving, testing

```bash
PY=$HOME/.venvs/legoland/bin/python
emcmake cmake -S portable -B portable/build-wasm -G Ninja -DLL_ILP32=ON \
    -DPython3_EXECUTABLE=$PY

ninja -C portable/build-wasm legoland_shimtest    # works today
ninja -C portable/build-wasm legoland_browser     # blocked on PORT-A, see below

cd portable/build-wasm && python3 -m http.server 8791
# then open http://localhost:8791/shimtest.html   (or legoland.html)
```

Both targets are `EXCLUDE_FROM_ALL`, so a plain `ninja -C portable/build-wasm` is
unchanged for the other lanes. `legoland_hostwin` **is** in the default build on
every toolchain, including native clang, so a signature error in the shim shows
up in `ninja -C portable/build` too.

### Link flags, and why

| flag | why |
| --- | --- |
| `-sASYNCIFY=1` | §1 |
| `-sASYNCIFY_STACK_SIZE=131072` | the side buffer for the unwound stack; the 4096 default overflows, because a `timeGetTime` yield can happen at any depth of the render path |
| `-sALLOW_MEMORY_GROWTH=1`, `-sINITIAL_MEMORY=256MB` | the game's heap plus the RES directories |
| `-sSTACK_SIZE=8MB` | the render path is deep and several game functions have kilobyte frames (`InitSession`'s `char msg[0x400]`, the RES path buffers) |
| `-sEXIT_RUNTIME=0` | `main` unwinds at the first yield and is rewound later |
| `-sERROR_ON_UNDEFINED_SYMBOLS=0` | `legoland_browser` only, until PORT-A's closure closes: an undefined symbol becomes a runtime trap the page reports, not a link failure |
| `--js-library src/browser/ll_canvas.js` | the canvas and the event queue |
| `--shell-file src/browser/index.html` | the page |

`ASYNCIFY_IMPORTS` is deliberately **not** set: every yield goes through
`emscripten_sleep`, which is already in Emscripten's default list.

### Assets

`--preload-file` bakes the tree into a `.data` file fetched before `main`. The
layout is dictated by `RES_OpenVolume` (`data2.c` 0x00489750): it `_splitpath`s
the volume name (so `"Legoland.res"` becomes the stem `Legoland`) and opens
`.\volumes\<stem>.res` first, then `<g_res_path><stem>.res`. So:

| cmake option | maps | size |
| --- | --- | --- |
| always | `gamedata/main` → `/gamedata` | 14 MB |
| `LL_PRELOAD_RES` (default **ON**) | `gamedata/disc/{Legoland,Graphics1,Graphics2}.res` → `/gamedata/volumes/` | 157 MB |
| `LL_PRELOAD_SPEECH` (default OFF) | `gamedata/disc/Speech` → `/gamedata/speech` | 58 MB |

`main.c` `chdir`s to `/gamedata`. `LL_PRELOAD_RES` defaults ON because
`InitSession` opens the three volumes **before** `InitHostSystemGPU`
(`startup.c:132`): without them the game stops before DirectDraw is ever touched
and no frame is drawn. A 170 MB `.data` file is fine over localhost and not fine
over the internet; the replacement is a WASMFS fetch backend, the natural
follow-up to this lane.

### What was actually tested

`legoland_shimtest` built, served on `http://localhost:8791/shimtest.html` and
driven with the in-app browser. It puts the shim through exactly the sequence the
startup spine puts it through (`DirectDrawCreate` →
`QueryInterface(IID_IDirectDraw2)` → `GetCaps` → `RegisterClassExA` /
`CreateWindowExA` → `SetCooperativeLevel(0x11)` → `SetDisplayMode(640,480,16)` →
`GetDisplayMode` + the depth classification → primary caps `0x4200` → back caps
`0x800` → `CreateClipper` + `SetClipList` → `DirectInputCreateA` + both devices),
then loops: pump like `ProcessSystemEvents`, read both devices like
`ScanKeyboard`/`ScanMouse`, Lock/draw/Unlock the back surface, `Blt` to primary.

Observed:

- the page reports `mode 640x480 16bpp, g_screen_depth would be 2 (2 = RGB565)`
  — the classification the game performs comes out right;
- a 640×480 RGB565 gradient renders on the canvas and animates;
- **39,480 frames presented over an ~8 minute run (~82 fps sustained)** with the
  page responsive throughout, no ASYNCIFY stack overflow and no memory growth
  problem — the yield does what §1 says it does, and it keeps doing it. (The
  shimtest has no 28 ms frame floor; the real game's `FlipPrimary` caps it at
  ~35 fps.)
- `timeGetTime` advances monotonically and in step with wall clock;
- pointer motion over the canvas moves the drawn cursor (relative deltas through
  `DIMOUSESTATE`) and a key event lights its bar (`g_key_state[DIK_SPACE] &
  0x80`), so both DirectInput devices read real state.

### `legoland_browser`: wired, blocked on PORT-A

The target is complete — closure, shim, whole game archive, preload, page — and
`closure_filter.py` verifiably does its job (70 traps dropped, 0 left in PORT-B's
six DLLs, `stubs.c`'s `timeGetTime` dropped too). It cannot link yet because the
**generated closure does not compile on wasm32**, in two places, both PORT-A's:

1. `gen/globals.c`, 12 × `error: redeclaration of '<sym>' with a different type:
   'unsigned char[]' vs 'unsigned int[N]'` — PORT-A deliverable 1, exactly as the
   brief describes it (`kVolLegoland`, `kCastleObjName`,
   `g_str_driving_school_roads`, `g_str_zebra_crossing`, …).
2. **A second one the brief does not mention**, found by building with
   `-DLL_ILP32=OFF` to get past (1): `gen/aliases.c` fails on non-Apple targets
   with `error: definition of variable with array type needs an explicit size or
   an initializer`. `gen_link.py`'s `data_alias()` non-Apple branch emits
   `extern unsigned char NAME[] __attribute__((alias("real")))`, and an alias
   attribute makes that a *definition*, so the incomplete array type is rejected.
   It needs a size (or `[1]`, or the Apple branch's `asm` spelling). 20+ sites.
   **PORT-A must fix this one too, or the wasm link cannot close.**

Everything else in the browser target compiled: `globals.c`, `stubs.c`,
`host_stubs.c`, all 258 game objects, the shim and `main.c`.

---

## 7. What is stubbed, and what diverges

| area | state | consequence |
| --- | --- | --- |
| GDI text (`TextOutA`, `DrawTextA`, `SelectObject(font)`) | no-op, reports success | **all GDI text is invisible.** The layout maths still runs, so the UI geometry is right and only the glyphs are missing. The fix is a bitmap font in `gdi32.c` drawing into the surface the DC came from |
| `StretchDIBits` | returns 0 scan lines | the AVI intro frames do not appear. Moot while there is no Indeo 5 decoder; `-nointro` skips them anyway |
| printing (`StartDocA`/`StartPage`/`EndPage`/`EndDoc`) | returns failure | the print path aborts at its first check |
| dialogs (`DialogBoxParamA`, `GetDlgItem`, `EndDialog`) | `-1` / 0 | no dialog runs; every caller treats non-positive as "did not run" |
| DirectSound | `DSERR_NODRIVER` | silent; tolerated, see §5 |
| MIDI out | `MMSYSERR_NOTSUPPORTED` | silent; `InitMIDIManager` ignores it |
| palette (`CreatePalette`, `SetPalette`, `SetEntries`) | real objects, no effect | unreachable: 565 keeps `g_screen_depth` at 2 |
| **mouse acceleration** | divergence | the browser gives absolute positions, which `ll_canvas.js` differences into DirectInput deltas; the game then applies its **own** acceleration (`UpdateControllerFromMouseData` doubles the delta up to twice, `input.c`), so the game cursor moves faster than the real pointer and drifts away from it. Pointer lock (feeding `movementX/Y`) fixes the source; undoing the game's acceleration would need a game edit, which this lane may not make |
| `GetKeyState` toggle bit | partial | only the "down" bit (0x8000) is tracked, not the CapsLock toggle bit (0x0001) |
| `AdjustWindowRect` | no-op | there is no window frame on a canvas, so the client rect is the window rect; `InitScreen`'s windowed arm then sizes the canvas exactly w × h |

---

## 8. For the integrator

**Things another lane must do.**

1. **PORT-A: `Sleep` must yield** (`emscripten_sleep(ms)`), never be a no-op.
   `RunGame`'s music wait is `while (g_music_disabled == 0) { PeekMessageA(...);
   Sleep(100); }` (`gamemain.c:330`) with no other yield in it. The contract is
   also written into `ll_host.h`'s KERNEL32 section.
2. **PORT-A: `gen/aliases.c` does not compile off Apple** — see §6. Not in the
   brief's list; it blocks the wasm link just as hard as the `globals.c` errors.
3. **PORT-A: `CreateFileA` must accept backslashes.** `RES_OpenVolume` opens
   `.\volumes\Legoland.res`; on a POSIX/WASMFS layer that has to become
   `./volumes/Legoland.res`.
4. **The command line must carry `-nomusic`** (it is `main.c`'s default).
   Without it the game waits forever for a music thread this port does not
   create — see 1. `-nointro` is also the default, since there is no Indeo 5
   decoder.

**Things the game would need changed** (written down rather than done, because
`LEGOLAND/*.c` is read-only for this lane):

- Nothing is *required*. The shim runs the game's control flow unmodified; that
  was the point of the ASYNCIFY decision. The only place a game edit would buy
  anything is the mouse-acceleration divergence in §7, and even that is better
  fixed with pointer lock on the host side.
- If ASYNCIFY's code-size and speed cost ever becomes the problem, the game-side
  alternative is a one-frame entry point, which is a large change to
  `gamemain.c` / `input.c`. The host-side alternative is `-sJSPI`, which needs no
  game change and no shim change beyond the flag — every yield already funnels
  through `ll_host_yield`.

**Files this lane touched:** `portable/cmake/browser.cmake`,
`portable/src/browser/{index.html,ll_canvas.js,main.c,shimtest.c,closure_filter.py}`,
`portable/src/hostwin/{ddraw,user32,gdi32,dinput,winmm,dsound}.c`,
`portable/hostwin/include/ll_host.h` (created; PORT-A's section left empty),
`portable/README.md` (PORT-B section), this note, and the status line in
`docs/SCOPE_PORT_WAVE.md`. Plus one line of `.gitignore`:
`portable/build-wasm/` was not ignored although the brief says both build dirs
are, so a `git add -A` in the tree commits ~80 MB of objects.
