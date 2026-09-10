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
| 1. main-loop decision | **done** (this section; committed first) |
| 2. `ddraw.c` + `ll_canvas.js` present path | see §3 |
| 3. `user32.c` / `gdi32.c` | see §4 |
| 4. `dinput.c` / `winmm.c` / `dsound.c` | see §5 |
| 5. `browser.cmake` + `index.html` + serve/test | see §6 |

Sections 3–6 are filled in as each lands; §7 is the stub register and §8 the
integrator hand-off.
