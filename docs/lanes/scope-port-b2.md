# Lane PORT-B2 — the first front-end frame and the first click

Brief: `docs/SCOPE_PORT_WAVE.md` "PORT-B2". Branch `scope/PORT-B2`, cut from
main `8e02a675`. Predecessor: `docs/lanes/scope-port-b.md` — read that first;
its §1 (the ASYNCIFY main-loop decision) and §3 (the DirectDraw vtable slot
table) are still the contract and nothing here changes them.

Owned files: `portable/cmake/browser.cmake`, `portable/src/browser/**`,
`portable/src/hostwin/{ddraw,user32,gdi32,dinput,winmm,dsound}.c`,
`portable/src/hostwin/ll_font.c` (new), additions to
`portable/hostwin/include/ll_host.h`, this note, the PORT-B2 part of
`portable/README.md`. `LEGOLAND/*.c` is read-only for this lane and was not
touched; `gen_link.py`, `kernel32.c`, `msvcrt.c` and `tests.cmake` belong to
PORT-A2 and PORT-C and were not touched either. Everything this lane found that
lives in one of those files is written down in §5 instead of being fixed here.

Where the page was when this lane opened: `legoland.html` ran the game's own
`WinMain` through `DirectDrawCreate`, the window and the CD check, then failed
in `RES_OpenVolume`. That was PORT-A2's, and it landed while this lane ran; §6
records how far the page gets now.

---

## 1. The host calls between "resources mounted" and the first front-end flip

The path, from the sources (`startup.c` `InitSession` 0x0047f880 → `gamemain.c`
`RunGame` 0x00459520 → `gameframe.c` `GameFrame` 0x00458c00 → `mapscreen.c`
`InitScreens` 0x00458640 → `RenderFrontEndScreen` 0x00458740):

```
InitSession
 ├ RES_EnsureMounted(1) ──────────── MessageBoxA loop      (b)
 ├ RES_OpenVolume x3 ─────────────── MessageBoxA on failure
 ├ LoadStrings ───────────────────── fopen(".\strings\stab.str"); exit(1) on failure
 ├ InitHostSystemGPU ─────────────── DirectDrawCreate, QueryInterface, GetCaps,
 │                                   AddFontResourceA("Lego.ttf")
 ├ InitScreen ────────────────────── the 4 CreateFontIndirectA        (a)
 │                                   RegisterClassExA / CreateWindowExA
 │                                   SetCooperativeLevel(0x11), SetDisplayMode
 │                                   CreateSurface primary/back/vidmem
 │                                   CreateClipper + SetClipList
 │                                   LoadColourTable → CreatePalette + SetPalette (c)
 │                                   ShowCursor(0) if g_screencfg->cursor == 0  (d)
 ├ InitInputSystem ───────────────── DirectInputCreateA + 2 devices     §4
 ├ LoadSprite x8 ─────────────────── the game's own pointer sprites     (d)
 └ RunGame
    ├ ShowTitleScreen ───────────── LoadSprite + PrintSprite + RenderingComplete
    │                                ← THE FIRST FRONT-END PRESENT. No GDI text.
    ├ while (!g_music_disabled) { PeekMessageA; Sleep(100); progress_tick; }
    └ GameFrame → InitScreens(0) → RenderFrontEndScreen
                                     ← the first frame WITH text
```

`RenderFrontEndScreen` holds the draw surface **locked** for the whole frame
body and every GDI text printer unlocks it around its own `GetDC`/`ReleaseDC`
(`PushRenderingStatusAndUnlockVideoSurface` / `PopRenderingStatus`,
`surface.c`), because real GDI needs an unlocked surface. In this shim the pixel
buffer is a plain `malloc` block that is valid either way, so lock state is
deliberately not consulted — recorded here because it is the first thing a
reader will wonder about.

### The gaps that were there, and what closed them

| host entry | what the game needs from it | was | now |
| --- | --- | --- | --- |
| `IDirectDrawSurface::GetDC` +0x44 | a DC identifying the surface; HRESULT never checked, so a failure would leave `hdc` uninitialised | returned the surface pointer | unchanged, plus it resets the DC's attributes — a DC from GetDC is a *fresh* DC, and `PrintLimitedText` (0x00454c70) sets a text colour it never restores |
| `CreateFontIndirectA` | four fonts from one LOGFONT: 24/700, 28/400, 20/700, 18/600, face "Lego"; handles stored at 0x0066808c..0x00668098 and told apart by `SelectFont` | one opaque cookie each | a real font object carrying the metrics derived from `lfHeight`/`lfWeight` |
| `SelectObject` | called four times per Print with TWO object classes (region, font) and restored in reverse; the return is stored and handed back | a fresh cookie every time, so the restore was a no-op | returns the previously selected object **of the same class**, from a real object table |
| `CreateRectRgn` / `DeleteObject` | the clip region every Print builds from `g_clip_rect` (0x4bdea0) and deletes after | cookie / no-op | a real rect, honoured as the clip box; DeleteObject frees the slot and clears any DC still referencing it |
| `SetTextColor` / `SetBkColor` / `SetBkMode` / `SetTextAlign` | set per Print, read by the draw call; `PrintCentColref` stores the old colour and restores it | reported "was 0", remembered nothing | per-DC state, returning the previous value |
| `TextOutA` | `Print` (0x00454ba0), the only TextOutA on this path, with `SetBkMode(OPAQUE)` | no-op | draws the bitmap face into the surface, opaque cell fill included |
| `DrawTextA` | **both** the draw call and the game's only text-extent call — 11 sites pass DT_CALCRECT | returned 1, left the rect alone | full layout: CENTER/RIGHT/VCENTER/BOTTOM/WORDBREAK/SINGLELINE/NOCLIP/CALCRECT, returns the height, writes the measured rect |
| `FillRect` | `DrawCachedTextSprite` (bubblecache.c:439) fills the sprite with a brush and then colour-keys **that exact colour** away | no-op → the key would not match the fill | fills with the brush colour, packed 565 the same way the game's `GetNearestColour` packs it (sweep1.c:219) |
| `GetNearestColor` | its result becomes both the fill and the colour key | identity | identity (correct: a 565 surface holds exactly what is written) |
| `MoveToEx` / `LineTo` | renderview.c:2835 draws ride links with the selected pen | no-op | real, Bresenham, in the pen's colour |
| `MessageBoxA` | `RES_EnsureMounted` LOOPS until IDCANCEL; `InitSession` reports 3 fatal failures through it | always IDOK → the loop never exits | answers per button set; always the answer that does not ask again |
| `SystemParametersInfoA(SPI_GETMOUSE)` | fills 3 ints that become the game's own mouse acceleration | reported success, filled nothing → acceleration ran on stack garbage | `{6, 10, 0}` |
| `ShowCursor` | `InitScreen` calls `ShowCursor(0)` when `g_screencfg->cursor == 0` | display count started at 1, so it stayed >= 0 and visible | starts at 0, so the call reaches -1 and hides |
| palette (c) | `LoadColourTable` runs **unconditionally** in InitScreen's fullscreen arm: `CreatePalette(0x44)` + `SetPalette` on the primary, HRESULTs unchecked | real objects, no effect | unchanged, and that is correct — `SetScreenDisplayMode` classifies the reported RGB565 as `g_screen_depth = 2`, and every paletted path (`LoadColourTable`'s effect, `ResendPalette` spritemisc.c:171, rin.c's palette) is guarded by `g_screen_depth == 0`. The front end is 8-bit paletted only on a display that cannot do 16 bpp, which this host never reports. |
| cursor (d) | **confirmed: the game draws its own pointer.** `SetPointer` (sweep2.c:92) just stores one of 8 sprites loaded in InitSession, and `FlipPrimary` (sysmisc.c:563) blits `g_current_pointer` at `g_gfx_point` just before presenting. The game never calls `GetCursorPos`/`SetCursorPos`, and `ProcessSystemEvents` calls `SetCursor(0)` after every dispatched message | already hid the canvas cursor | unchanged |

Deliberately still stubbed, with the consequence: `StretchDIBits` returns 0 scan
lines (no Indeo 5 decoder; `-nointro` skips the AVIs anyway); printing reports
failure at `StartDocA`, so certificate.c's path aborts at its first check;
`CreateCompatibleDC` is a cookie with no pixels, which is all the measuring
callers (`HTBubbleHelp` fpui2.c:997, `bighelp.c:316`) want from it — measuring
through one works, drawing into one is a no-op.

## 2. The face — `portable/src/hostwin/ll_font.c`

"Lego" is a proportional TrueType face that is not in this tree and could not be
rasterised here, so the faithful thing is impossible and the useful thing is a
face whose METRICS are in the same ballpark, so that the game's own layout lands
where it landed on Windows. A 6x7 ink box scaled to `lfHeight` gives ~0.45 em of
advance — a normal proportional figure — so a 640-pixel line holds about 53
characters of the 24-pixel font. Lines that fitted still fit; lines that wrapped
still wrap, within a character.

| lfHeight | lfWeight | ink box | advance | who selects it |
| --- | --- | --- | --- | --- |
| 24 | 700 | 10x20 | 12 | `SelectFont(hdc, default)` |
| 28 | 400 | 11x23 | 12 | `SelectFont(hdc, 3)` |
| 20 | 700 | 8x16 | 10 | `SelectFont(hdc, 1)` |
| 18 | 600 | 7x15 | 9 | `SelectFont(hdc, 2)` |

Bold (weight >= 600) is the glyph smeared one pixel right, costing one pixel of
advance — the relationship a real bold face has. The glyphs are authored as
ASCII art, one line per character: **this lane's own drawing**. No bitmap from
the game, from Windows or from any third-party face is reproduced.

## 3. Node safety — `portable/src/browser/ll_canvas.js`

`legoland_hostwin` links into `legoland_tests` and `legoland_headless`, which run
under node with no `document`, no `window` and no canvas. Every JS-library
function degrades; nothing throws. A `ReferenceError` here would abort the wasm
call that triggered it, which in a test harness reads as a mysterious failure
deep inside the game rather than as "there is no DOM".

**The probe must be lazy and must go through `globalThis`.** The first version
used `typeof document === 'undefined'` at module scope; emcc's `-O2` JS
optimizer minifies the library with no DOM in scope, constant-folds that at
BUILD time, and emits `headless:true` — so the *browser* page ran headless and
painted nothing while the frame counter climbed. A property access on
`globalThis`, evaluated lazily, is opaque to the optimizer. Verified in the
emitted `shimtest.js`.

Headless `present16` hashes a sample of the surface (FNV-1a over every 17th
pixel of every 7th row; both strides coprime with the row length, so the samples
walk across the surface rather than down one column), which lets a headless
harness prove pixels are being produced *and changing*.

Proof: `node portable/build-wasm/shimtest.js --frames 300` → 300 frames, C-side
present count equals the JS frame count, non-zero checksum, `MB_RETRYCANCEL`
answered IDCANCEL (2), `MB_OK` answered IDOK, `SPI_GETMOUSE` filled `{6,10,0}`,
`PASS`, exit 0.

## 4. Input — checked against `input.c` / `input2.c`, not assumed

The shim was already right about most of this; what follows is the verification,
because "it looked right" is not a check. Every row was read out of the sources.

| contract | source | shim |
| --- | --- | --- |
| `GetDeviceState(kbd, 256, g_key_state)`, bit 0x80 = down | `input.c:107` | OK |
| both scan loops spin **forever** on any non-`DI_OK` except `DIERR_NOTACQUIRED` (0x8007000C) and `DIERR_INPUTLOST` (0x8007001E) — and nothing in those loops yields | `input.c:106`, `input.c:126` | OK: `GetDeviceState` always returns `DI_OK`. This is why it must never fail: a failure is a wedged tab, not an error |
| `GetDeviceState(mouse, 16, &g_mouse_state)`; DIMOUSESTATE `{long lX, lY, lZ; unsigned char rgbButtons[4]}`; axes RELATIVE (`c_dfDIMouse.dwFlags = DIDF_RELAXIS`) | `input.c:40`, `input.c:125` | OK: deltas accumulated and zeroed on read |
| buttons read at indices 0, 2, 1 → controller bits 1, 4, 2 (left, middle, right) | `input.c:279` | OK: `ll_canvas.js` maps DOM button 1→2 and 2→1, so `rgbButtons[1]` is right and `[2]` is middle |
| wheel: `lZ <= -granularity` / `>= +granularity`, one step per poll, no accumulator | `input.c:133` | OK: ±120 per notch |
| `g_wheel_granularity` **is initialised to 60 (0x3c) in the image**, and only overwritten if `GetProperty(DIPROP_GRANULARITY)` returns DI_OK | `globals.c`, `sysmisc.c:241` | OK either way: the shim reports 120 and 120 crosses both thresholds |
| `DIPROP_GRANULARITY` is `MAKEDIPROP(3)` — the integer 3 cast to a pointer; must be compared numerically, never dereferenced | `sysmisc.c:200` | OK |
| `GetCapabilities` must report `dwFlags != 0` (`((caps.dwFlags == 0) & 1) == 0`) | `sysmisc.c:245`, `sysmisc3.c` | OK: `DIDC_ATTACHED` |
| both `Create*Device` end with `GetDeviceStatus(GUID) == 0` | `sysmisc.c:249` | OK |
| `CreateMouseDevice` calls `SetCooperativeLevel` **before** `SetDataFormat` | `sysmisc.c:229` | order-independent in the shim |
| no buffered mode anywhere: no `DIPROP_BUFFERSIZE`, no `GetDeviceData` call in the whole game | grep over `LEGOLAND/` | OK |
| `GetWindowLongA(hwnd, GWL_HINSTANCE)` must be non-null — passed straight to `DirectInputCreateA` unchecked | `input2.c:266` | OK |
| the window proc handles exactly four messages: `WM_CLOSE`, `WM_CHAR` (only `(char)wp == 8`, backspace; everything else swallowed), `WM_KILLFOCUS`/`WM_CANCELMODE`, `WM_SETFOCUS`. **No mouse messages at all** | `input2.c:230` | OK: only backspace is synthesised as WM_CHAR |
| `GetKeyState` is called for exactly one key, `VK_CAPITAL` (0x14), and only bit 0 (the toggle) is read | `input2.c:325` | **known gap**: the shim tracks the down bit, not the toggle bit, so CapsLock never reads as latched and `GetInputChar` gets the case wrong. `GetTypedChar` — the one the controller fold actually calls — does not use it. See §5 finding 4 |
| `SystemParametersInfoA(3, ...)` fills the acceleration triple | `gameframe.c:323` | **fixed this lane** |

DIK mapping: the table in `ll_canvas.js` is the PS/2 set-1 scan codes, which is
what DIK codes are. The eight the controller fold reads are `0xcb` LEFT,
`0xcd` RIGHT, `0xc8` UP, `0xd0` DOWN, `0x39` SPACE, `0x0f` TAB, `0x01` ESCAPE,
`0x1c` RETURN (`input.c:442`), and all eight are in the table. The game's own
59-entry DIK→character map at 0x004bad58 (`input2.c:114`) covers the letters,
both digit rows, backspace, escape, both enters, space and the shift/caps keys —
every one of those is in the table too, so the cheat codes and the name-entry
field have the codes they need. Its last entry has DIK 0 and is still iterated,
so `g_key_state[0]` must stay 0: `ll_canvas.js` only pushes a mapped code and
`ll_host_key_set` ignores index 0, so it does.

## 5. Findings for other lanes

**1. `fopen` does not translate backslashes — PORT-A2 (CRT shim).** This is the
current blocker on the front-end path, and it is one line. `msvcrt.c`'s
`ll_path()` normalises `\` to `/` for `_open`/`_stat`/`_chdir`, but the game also
calls plain `fopen`, which resolves to emscripten's libc and passes the path
through untouched — so `LoadStrings`' `fopen(".\\strings\\stab.str", "r")` cannot
resolve even with the file present, and its `if (!f) exit(1)` kills the page with
no message. Proven from the page: wrapping `FS.lookupPath` to translate
backslashes makes the `exit(1)` disappear and the game runs on. `fopen`,
`freopen` and any other CRT path entry the game uses need the same `ll_path()`
treatment `_open` already gets.

**2. `g_key_state[256]` is emitted as five separate 16-byte-aligned arrays —
PORT-A2 (`gen_link.py`).** `gen-browser/globals.c` has

```c
__attribute__((aligned(16))) unsigned char g_key_state[29];   /* 0x007fdda0 */
__attribute__((aligned(16))) unsigned char g_left_ctrl[13];   /* 0x007fddbd */
__attribute__((aligned(16))) unsigned char g_left_shift[12];  /* 0x007fddca */
__attribute__((aligned(16))) unsigned char g_right_shift[103];/* 0x007fddd6 */
__attribute__((aligned(16))) unsigned char g_right_ctrl[103]; /* 0x007fde3d */
```

In the image those five names are INTERIOR ALIASES of one 256-byte array:
`g_left_shift` is `g_key_state + 0x2a`, `g_right_shift` is `+ 0x36`, and so on.
Emitted as separate objects with 16-byte alignment they are at 0, 32, 48, 64 and
176 instead of 0, 0x1d, 0x2a, 0x36 and 0x9d. Two consequences:

- `GetDeviceState(kbd, 256, g_key_state)` writes 256 bytes into a 29-byte
  object. Direct indexing still works — `g_key_state[0xc8]` reads exactly the
  byte the shim wrote at +200, so the arrow keys are fine — but the write only
  stays in bounds if the linker happens to place the five consecutively.
- The four alias symbols read the wrong bytes. `IsLShiftDown`/`IsRShiftDown`
  (`sysstubs.c:180`) and `IsCtrlDown` therefore never see their keys, so
  `GetInputChar`'s shift handling and movie.c's Ctrl+Q skip are dead.

The fix is in the generator: a symbol that is an interior alias of a larger
contiguous region should be one array plus offset aliases, which is the same
machinery `data_alias()` already has.

**3. Where the page stops now — PORT-A2.** With finding 1 worked around, the
next stop is a wasm `RuntimeError: unreachable` raised directly under
`callMain`, before `InitHostSystemGPU`'s `DirectDrawCreate` and before any
`InitScreen` call, so somewhere in `LoadStrings`' parse or immediately after it.
The build has no name section at `-O2`, so the frame is only
`wasm-function[32]`; a `-O1 -g2` build would name it in one run. Two candidates,
both worth ruling out before anything else:
  - the 542 prototype conflicts PORT-A's census reports. `wasm-ld` still prints
    `function signature mismatch` for `SetupControllers`, `AddHelpMessage`,
    `RenderList` and others, and on wasm each of those calls traps with
    `unreachable`, which is exactly this signature.
  - `LoadStrings`' own reproduced bugs (`narration2.c:685`): `char num[4]` with
    `num[n] = 0` after an unbounded digit run — and the string table really does
    contain four-digit ids (`GetString(0x9c4)` = 2500), so `num[4] = 0` writes
    one past the array on every one of them. Harmless on x86; not necessarily
    harmless after `-O2` on wasm.

**4. `GetKeyState`'s toggle bit — this lane's shim, left undone deliberately.**
`GetInputChar` reads `GetKeyState(VK_CAPITAL) & 1`, the *toggle* bit, and the
shim only tracks the down bit. Tracking a latch in `ll_canvas.js` from
`KeyboardEvent.getModifierState('CapsLock')` would close it. Not done here
because `GetInputChar` is not on the front-end path — `GetTypedChar` is, and it
does not consult CapsLock.

**5. The asset tree is flat.** `gamedata/main` has no subdirectories at all,
while the game asks for `.\strings\stab.str`. Handled in the preload map for
that one file (§ commit "trace the DirectX half too"); the sweep in that commit
message shows nothing else needs it. Worth telling whoever regenerates
`gamedata/` that the directory structure was lost.

**6. `ll_canvas.js` / `index.html` were not link dependencies.** Editing either
left `ninja` reporting "no work to do" while the browser served the previous
build. Fixed with `LINK_DEPENDS` on both wasm targets; it cost this lane two
debugging rounds and would cost anyone else the same.

## 6. What the page does now

Two builds, both from a clean `portable/build-wasm`:

**`shimtest.html`, this branch alone.** 640x480 RGB565, ~90 fps sustained, the
gradient animating. All four fonts render and are distinguishable; `DT_CALCRECT`
measures and the following `DrawTextA` centres inside the measured box;
`SetBkMode(OPAQUE)` fills the cell with the `SetBkColor` colour; a string
starting inside the clip region is cut at its right edge after exactly 13 of its
33 characters, which is the arithmetic (x=500, region right 632, advance 10) and
not an eyeball. `MessageBoxA(MB_RETRYCANCEL)` returns 2 and `MB_OK` returns 1,
both shown in the page's modal field.

**`legoland.html`, with `scope/PORT-A2` merged locally** (scratch branch
`port-b2-plus-a2`; `scope/PORT-B2` itself contains only this lane's files). The
traced run reaches, in order: the version block, the mutex, `DirectDrawCreate`,
the CD check (`GetDriveTypeA("D:\") = 5`, `GetVolumeInformationA`: LEGOLAND on
CDFS), then **all three RES volumes open and their directories load** —
Legoland.res, Graphics2.res, Graphics1.res, 24426 + 31523 + 23598 bytes of
directory. PORT-A2's re-pointing fix works. Then `LoadStrings` stops it: first
with `exit(1)` because the string table was not at `.\strings\stab.str` (fixed
here), then, with the file mapped, still `exit(1)` because `fopen` will not
translate the backslashes (§5 finding 1), and with that worked around at the VFS
level, a wasm `unreachable` trap (§5 finding 3). No frame has been presented by
the game yet; nothing the page reaches traps in PORT-B/PORT-B2's six DLLs.

## 7. Building and testing

```bash
PY=$HOME/.venvs/legoland/bin/python
emcmake cmake -S portable -B portable/build-wasm -G Ninja \
    -DCMAKE_BUILD_TYPE=Release -DLL_ILP32=ON -DPython3_EXECUTABLE=$PY
ninja -C portable/build-wasm
ninja -C portable/build-wasm legoland_shimtest legoland_browser

# headless, no DOM -- the node-safety proof
node portable/build-wasm/shimtest.js --frames 300     # prints PASS

# in a browser
cd portable/build-wasm && python3 -m http.server 8792
#   http://localhost:8792/shimtest.html    the shim on its own
#   http://localhost:8792/legoland.html    the game
#   add ?trace=1 for the host call trace (kernel32 AND the DirectX half)
```

The page's header carries the four things a run succeeds or fails on: the frame
count and a measured fps, the last `MessageBoxA` with the answer the shim gave
it, any `TRAP` line (lifted out of the log into a red banner — one TRAP line is
the whole explanation for a run that stops dead), and the trace toggle. The
toggle reloads the page rather than flipping live, because `LL_HOST_TRACE` is
cached on first use. The log is capped at 4000 lines: a traced run emits
thousands a second and an unbounded log runs the tab out of memory before the
game does.

If the renderer hangs, close the tab and read the console from a fresh one —
`Asyncify.state` tells you whether the wasm is unwound (3), running, or stopped
(0), and a `setTimeout` latency test tells you whether the main thread is
blocked by a spin that is not yielding.

Keep the native build green too — `legoland_hostwin` is in its default build, so
a signature error in the shim shows up there first:

```bash
cmake -S portable -B portable/build -G Ninja -DPython3_EXECUTABLE=$PY
ninja -C portable/build
```

## 8. Status

| deliverable | state |
| --- | --- |
| 1. front-end host-call census + fix every gap | **done** — §1; GDI text visible and measuring, MessageBoxA answering, palette and cursor questions settled with evidence |
| 2. node-safe JS library | **done** — §3; `node shimtest.js --frames 300` prints PASS |
| 3. DirectInput shapes verified against input.c/input2.c | **done** — §4; two gaps found, one fixed (SPI_GETMOUSE), one recorded (§5.4), one handed to PORT-A2 (§5.2) |
| 4. page ergonomics | **done** — §7 |
| 5. merge PORT-A2 and look at a frame | **done, no frame yet** — §6; the volumes load, `LoadStrings` is the wall, and the three reasons are in §5 |
