# Scope PORT-B7 — typing a name, and the one call that kills the front end

> **Status: IN PROGRESS (claimed 2026-09-12 by PORT-B7).** Branch
> `scope/PORT-B7` from `feat/decomp-completion-next-steps-24a0d6` @ `1a7b38a4`
> (the PORT-B6 merge). Files owned: `portable/src/hostwin/{ddraw,user32,gdi32,
> dinput,winmm,dsound,avifil32,msacm32}.c`, `portable/src/browser/**`,
> `portable/cmake/browser.cmake`, additions to
> `portable/hostwin/include/ll_host.h`. `LEGOLAND/*.c` is read-only for this
> lane and was **not touched**, so the VC6 gate has nothing to check.

---

## 1. Headline

**The name editor works.** A click on slot 1 of PLAYER DETAILS opens the NEW
PROFILE popup, `EnterNewProfile` runs every frame, and the letters a-d-a-m
arrive as real key events, are decoded by `GetInputChar` out of the DirectInput
key array, land in `g_temp_profile.name` **read back live out of the game's own
memory**, and are drawn in the blue name field with the blinking cursor after
them. Backspace removes one. Nothing in the shim had to change for any of it —
PORT-B6's press latch and `dikOf` were already right, and the two split records
the integrator merged were the whole of what was missing.

**And the front end then dies, every time, about 0.4 s after that click:**

```
RuntimeError: function signature mismatch
   at UnreferenceSprite      (spritemisc.c 0x00497bd0)
   at FreeCachedTextEntry    (pathmask.c   0x00455ee0)
   at ExpireCachedText       (render5.c)
   at RenderingComplete
   at GameFrame
   at main
```

`spritemisc.c` declares one vtable slot `void` that three other files declare
`long`. It is a one-line, twice-cited, **matching-lane** fix (§4 B1), and until
it lands no screen past the popup is reachable on an honest build. With the slot
type corrected in a throwaway build, the walk went four screens further in ten
minutes (§3).

---

## 2. What was measured, and how

Everything below is `http://localhost:8871/legoland.html?args=-nointro+WINDEBUG`
driven by DOM events the page's own listeners receive, with the frame hashed
before and after each step and — the part that is new — the game's own globals
read out of linear memory between steps.

### 2a. The tooling this lane had to build first (all committed)

| where | what | why it was needed |
| --- | --- | --- |
| `index.html` | `llMove/llClick/llKey/llType(text)` in GAME pixels | a walk is now a list of (x, y) pairs read out of the recovered sources, not a screenshot-measuring exercise. They also encode the rule that the FIRST move must be (320,240), because the pointer is relative and that is where the game's cursor starts |
| `index.html` | `llAlive()` — frames presented in half a second | "the screen did not change" and "the game is not running" are different facts and the walk needs the second one first |
| `index.html` | an `unhandledrejection` handler feeding the TRAP banner + `llStats().dead` | **the page used to swallow the death.** Under ASYNCIFY the game's loop is resumed from a promise callback, so a `RuntimeError` inside it surfaced as an unhandled rejection and nothing else: canvas frozen on the last frame, fps figure frozen at its last value, `llFrameHash()` happily returning. This lane spent an hour on "the keyboard stopped working" that was really "the module died 0.4 s ago" |
| `main.c` | `ll_dbg_addr(n)` → the address of 7 recovered globals; `llPeek()` reads them | "did the character reach the editor" is not a question a screenshot can answer. This is what turned a guess into `{profileName: "adam", profileLen: 4, curProfileSlot: 1, nameWidth: 36, newProfilePopup: 1}` |
| `browser.cmake` | `legoland_browser_named` (`legoland_dbg.html`) — the same optimised link **plus `-g2`** | `name_trap.py` turns the innermost `wasm-function[N]:0xOFF` into a call site, but the CALLERS stay bare indices without a name section, and the stack above is five of them. `legoland_headless_debug` cannot help here: this mismatch is only reachable by a CLICK. `-O0` is not available for the browser target (headless.cmake:97), `-g2` on top of `-O2` is |
| `ll_canvas.js` | `globalThis.LL_DEBUG = LL` | `LL` is a closure variable inside the linked JS library, so "the key reached the DOM but not the game" had nothing in between to look at. `LL_DEBUG.events` showed `[1, 30, 65, 0]` queued and drained — which is how the keyboard was cleared as a suspect in one call |
| `dinput.c` | `ll_host_key_set` trace; first keyboard poll's size **and destination address** | the delivery half of B6's poll line (three answerable states instead of one shrug), and the cheap test for a split `g_key_state` — compare the traced address against `Module._ll_dbg_addr(0)` |

### 2b. Driving the page — what works and what does not

* **`computer left_click` / `computer key` (the automation verbs) are not
  reliable on this page.** Measured, with a listener counting events on the
  canvas: a whole batch of hovers and clicks at correctly mapped coordinates
  produced **zero** `mousemove`/`mousedown` on the canvas, while a click at the
  same moment landed on `document` at a client coordinate ~2.96x the tool
  coordinate. The tool's coordinate frame is reported per screenshot
  (`coordinate frame: 800x824` for a 422x435 viewport) and is neither the CSS
  viewport nor the screenshot's own pixels; worse, the mapping changed when the
  viewport was emulated with `resize_window`. This is PORT-B6's finding B6
  widened: **drive this page with dispatched DOM events, not with the pointer**.
  `llMove/llClick/llKey` exist so that nobody has to rediscover it.
* Two agents shared one browser during this session and **navigated each other's
  tabs**. Symptom: `Module._ll_dbg_addr is not a function` on a page that had it
  a minute earlier, because the tab was now showing another lane's build on
  another port. Check `location.href` in the same call as any measurement, and
  serve on a port nobody else is using.
* **Never touch `Module.HEAPU8`.** It is not an exported runtime method, and the
  accessor calls Emscripten's `abort()` — which sets `ABORT` and quietly stops
  the game. That cost this lane a whole round of "input stopped arriving". The
  bare globals `HEAPU8` / `HEAP32` are the ones to read (`llPeek` does).

---

## 3. Screens reached, with the exact input, and the hashes

`?args=-nointro WINDEBUG`, a fresh load each time. Every step is one call to the
page's own driver, so the whole walk replays verbatim:

```js
// paste into the console of legoland.html (or legoland_dbg.html)
await llMove(320, 240);              // REQUIRED FIRST: sync the relative pointer
await llMove(260, 188);              // hover slot 1
await llClick(260, 188);             // open the NEW PROFILE popup
await llType('adam');                // the name
await llClick(505, 345);             // the Accept icon (screens2.c: 0x1ef,0x14f)
await llClick(283, 369);             // "Select tutorial level"
await llClick(326, 198);             // the park-movie screen   (alternative branch)
```

| # | screen | input | frame hash | state |
| --- | --- | --- | --- | --- |
| 1 | **PLAYER DETAILS**, eight EMPTY slots | load | `0x9d9b7c50` | 96.5% non-black, 32.5-34 fps |
| 2 | the same + bubble help over slot *n* | `llMove(260, 188 + 38n)` | `0xadc09f10` `0x7b97ab50` `0x33eaa700` `0x3bec28b8` `0x5f3c8b41` `0xf789691c` `0x69113681` `0xbb971644` (n = 0..7) | all eight distinct; the tooltip follows the cursor |
| 3 | **NEW PROFILE popup** on slot 1: prompt "Enter player details", blue name field, red close icon | `llClick(260, 188)` | `0x81316ae1` | `llPeek()` → `newProfilePopup: 1, curProfileSlot: 1`. **The module dies ~0.4 s later on an honest build (§4 B1).** |
| 4 | the field with **`a|`** typed, cursor bar after it | `llKey('KeyA','a')` | `0x97de826d` | `profileName: "a", profileLen: 1, nameWidth: 8` — read out of `g_temp_profile`. Reached on an honest build by racing the death |
| — | the rest of this table needed the B1 fix; see §3a | | | |

### 3a. With B1 corrected (a throwaway build, never committed)

The slot type was flipped in `ddraw.c` for one experiment — the WRONG place to
fix it, done only to see what is behind the wall — and reverted. Everything in
this sub-table therefore describes a build that is not this branch, and says so:

| # | screen | input | frame hash |
| --- | --- | --- | --- |
| 4 | name field reading **`adam|`** | `llType('adam')` | `0x26cb44e1` (per keystroke: `0x82477bf9` `0x91590a4c` `0xbfdfdb58` `0xb3f74238`; Backspace → `0xa62a6169`) |
| 5 | **the front-end MAIN MENU** — the LEGOLAND gates with seven bubble icons | `llClick(505, 345)` (Accept) | `0x865e2ebd` / `0x7e019630` |
| 6 | main menu, bubble tooltips | `llMove(112, 191)` → "Select a player" | `0x005da54c` |
| 7 | **"Select tutorial level"** — the notepad with Lessons 1-5 | `llClick(283, 369)` | `0x367962dc` |
| 8 | the same, Lesson 1 hovered, tooltip "Select level" | `llMove(150, 145)` | `0x1d671c6d` / `0x3796953c` |
| 9 | **the park-advert screen** — LEGOLAND CALIFORNIA / WINDSOR / BILLUND | `llClick(326, 198)` from the main menu | `0xba7df26c` |
| 10 | the same, tooltip "LEGOLAND Windsor movie" | `llMove(318, 73)` | `0xe6c39e5c` |

The name typed on screen 4 is kept: after Accept, `llPeek()` still reads
`profileName: "adam"` and `newProfilePopup: 0`.

Screen 10 is the AVI shim behaving exactly as PORT-B6 §1a predicted: clicking a
park movie changes nothing, because `AVIFileOpenA` reports `AVIERR_FILEOPEN` and
`PlayMovie` returns without entering the player.

**THE PARK WAS NOT REACHED**, so there is no map-screen hash and no park fps in
this lane. Two clicks that should start something — the tutorial screen's
right-hand bubble at (575, 297), and a bubble at (86, 388) on the park-advert
screen — **wedge the page**: the JS main thread stops responding (every
`javascript_tool` call times out at 45 s, and a screenshot at 30 s), which means
the game is in a loop that never reaches a yield point rather than one that
merely stops presenting. It survived a 2.5-minute wait. The last console lines
are the game's own `"cannot open output file"` (profiles.c:351/381/552 —
`fopen("profiles\\Profile%d.txt")`, a BACKSLASH path) repeated. Not diagnosed;
both observations are from the §3a build, so they need re-confirming once B1
lands. **Owner: the next PORT-B lane**, and the first thing to try is a
`ll_host_trace` in the shim's `PeekMessageA`/`Sleep` to see whether the loop is
in the game or in the yield.

---

## 4. Blockers, with owners

| # | what | owner | state |
| --- | --- | --- | --- |
| **B1** | `spritemisc.c` declares the sprite owner's vtable slot +0x08 `void`; `gpu.c`, `sprite2.c` and `printlist.c` all declare the same slot `long`. The first cached-text sprite to be freed kills the module | **a matching lane (PORT-M)** | diagnosed, named, twice cited — the row is below |
| B2 | two clicks past the main menu wedge the page in a non-yielding loop | PORT-B (next lane) | §3, not diagnosed |
| B3 | the browser-automation pointer and key verbs do not reach this page | tooling | worked around: `llMove/llClick/llKey` in the page |
| B4 | `Module.HEAPU8` aborts the runtime | Emscripten, by design | documented; read the bare `HEAPU8` |

### B1 — one vtable slot, two declared types, and the whole front end

The call, `spritemisc.c:128`:

```c
/* 0x00497bd0 UnreferenceSprite */
owner = res->owner;                    /* SpriteRes +0x04 */
if (owner)
    owner->vtbl->Destroy(owner);       /* vtable +0x08 */
```

and its declaration three lines up, `spritemisc.c:23-26`:

```c
/* The owner object's vtable; slot +0x08 is its destructor (__stdcall). */
typedef struct OwnerVtbl {
    char pad0[8];                                 /* +0x00 */
    void(__stdcall* Destroy)(struct Owner* self); /* +0x08 */
} OwnerVtbl;
```

**The owner is an `IDirectDrawSurface` and +0x08 is its COM `Release`.** Two
independent citations, both for the same record and the same slot:

* `sprite2.c:12-14` names the field — "`+0x04 surface   IDirectDrawSurface*`
  holding the pixels" — and `sprite2.c:50-57` declares the slot
  `long(__stdcall* Release)(DDSurface*); /* +0x08 */`.
* `printlist.c:97-100` and `gpu.c:114-118` declare the same slot of the same
  interface the same way, independently, each with its own `DDSurfaceVtbl`
  ("IDirectDrawSurface vtable: +0x08 Release, ..."). Three files `long`, one
  file `void`; the shim's body (`ddraw.c` `ll_surf_Release`, slot 0x08) returns
  the remaining reference count, as DirectDraw does.

On x86 `__stdcall` the caller simply ignores EAX, so the disagreement is free.
On wasm the type is an immediate on the `call_indirect`, so the call **traps**:

```
$ python3 portable/tools/name_trap.py --wasm portable/build-wasm/legoland_dbg.wasm --at 0xf3a94
== INDIRECT CALL TYPE MISMATCH
   caller      UnreferenceSprite   (module offset 0xf3a94)
   call site   call_indirect type 0 = (i32) -> void
```

**The recipe** (a matching lane's, under the usual guard, never between a
`// FUNCTION:` marker and its signature; `audit.py` + `relocs.py` on the file):

```c
typedef struct OwnerVtbl {
    char pad0[8];                                 /* +0x00 */
#ifdef LEGOLAND_PORTABLE
    /* The owner is an IDirectDrawSurface and +0x08 is its COM Release, which
     * returns the remaining reference count -- sprite2.c:50-57, printlist.c:97,
     * gpu.c:114 all declare this slot `long`. x86 ignores the result; wasm
     * checks the type at the call. */
    long(__stdcall* Destroy)(struct Owner* self); /* +0x08 */
#else
    void(__stdcall* Destroy)(struct Owner* self); /* +0x08 */
#endif
} OwnerVtbl;
```

The body is unchanged (the result is still discarded), so the `#ifndef` arm's
bytes cannot move.

**Why it is fatal and not cosmetic.** `RenderingComplete` calls
`ExpireCachedText` every frame; the first entry to expire takes
`FreeCachedTextEntry` → `KillSprite` → `UnreferenceSprite` → this call. On
PLAYER DETAILS nothing expires for thousands of frames because the eight
tooltips are the same eight strings; the moment the popup replaces slot 1's
caption the cache turns over and the module is dead within ~24 frames.
Reproduced on every attempt, from two different slots.

**Why it was invisible until now.** It is not a trap, not an undefined symbol
and not a wasm-ld warning (PORT-M3 inventoried the tables that are *statically*
visible; this slot is written at runtime by the shim). The page showed a live
fps figure and a frozen picture — which is exactly the "the game ignores input"
report that PORT-B4 and PORT-B6 both chased. §2a's death banner is this lane's
answer to that class of report.

### B1a — and it is the ONLY one of its kind

The obvious next question is how many more of these are waiting, and it has a
cheap answer, because only a `void`-declared slot can trap this way: a slot
declared `int`, `long` or `unsigned long` is `(…)->i32` whichever of the three
a TU picked, and wasm cannot tell them apart. So:

```
$ grep -rn 'void\s*(\s*__stdcall\s*\*' LEGOLAND/*.c
LEGOLAND/lifecycle.c:88:typedef void (__stdcall* MMTimeProc)(unsigned int id, ...
LEGOLAND/spritemisc.c:26:    void(__stdcall* Destroy)(struct Owner* self); /* +0x08 */
```

**Two, and the other one is right.** `MMTimeProc` is WINMM's `timeSetEvent`
callback; its only body is `MIDITimerTick` (music2.c 0x00480570), which is
declared `void __stdcall` with the same five arguments, and `winmm.c`'s own
`LLTimeProc` matches. So `spritemisc.c:26` is the whole of this class in the
tree, and fixing it removes it.

A wider sweep of every `__stdcall*` slot declared in two or more TUs
(`port-b7-vtsweep.py`, scratch) finds three more disagreements and **all three
are harmless on wasm32**: `IDSoundVtbl +0x04/+0x08` (lifecycle.c says `long`
where audio3.c/input2.c/musicthread.c say `unsigned long`) and `SurfaceVtbl
+0x7c` `SetPalette` (spritemisc.c `int` vs rin.c `long`). Worth fixing for
tidiness, not for correctness.

That is the *declared* half. The other half — an indirect call through a slot
the game fills with its own bodies — is PORT-M3's typed-callback pass, and at
this one call site `name_trap.py --at` still lists 186 functions whose address a
rebuilt global holds and whose type is not `(i32) -> void`. Those are candidates
only; the real target here was the shim's, written into the vtable at runtime.

---

## 5. What the shim did NOT need

Recorded because the brief asked for shim-side gaps and there were none in the
keyboard path:

* **DIK mapping** — correct. `g_key_map` at 0x004bad58 comes out of the image
  intact (`{0x1e,'A'}, {0x30,'B'}, ... {0xc8,-11}`), the closure emits it as one
  592-byte object, and `ll_canvas.js`'s `dik` table agrees with it.
* **key repeat** — not wanted. `GetInputChar` fires on the DOWN EDGE
  (`cur & 0x80 && !prev`), so a held key types exactly one character, which is
  what the original does.
* **keydown/keyup in one drain** — already handled by PORT-B6's press latch; a
  key is reported down by at least one poll however briefly it was held.
  Measured: one 150 ms press produces one `key_set ... down`, 4-5
  `key state DIK 0x1e down (poll reports it)` lines, one `key_set ... up`, and
  exactly one character.
* **character vs scancode** — the game wants SCANCODES. `EnterNewProfile` reads
  `GetInputChar()`, which indexes `g_key_state[]` by DIK and maps through
  `g_key_map`, doing its own shift/caps folding; `WM_CHAR` is used for exactly
  one character in the whole program (backspace, `LegoLandWindowProc`), and
  `user32.c` already synthesises that one. Nothing to add.
* `g_key_state` is **one** object: the first keyboard poll now traces its
  destination address, and it equals `Module._ll_dbg_addr(0)`.

---

## 6. Performance

| where | fps | note |
| --- | --- | --- |
| PLAYER DETAILS, idle | **32.4-34.0** (rolling), 32.7 average over 568 frames | the 28 ms flip floor `FlipPrimary`/`PresentFlip` spin on is 35.7 Hz, so this is the ceiling, unchanged from PORT-B4's 33.5 |
| PLAYER DETAILS, pointer moving, tooltips redrawing | 33-34 | no measurable cost |
| NEW PROFILE popup with the editor running (text + cursor through `DrawTextA` every frame) | 33.3 | the GDI text path is not a bottleneck |
| main menu, tutorial screen, park-advert screen (§3a build) | 33.3-34.0 | every front-end screen sits on the floor |
| **the park** | **not measured — the park was not reached** (§3, B1) | |

`llAlive()` returns 17-18 frames per 500 ms on every screen above, which is the
same number; there is no screen in the front end where the shim costs anything.
Nothing was found to fix in the present path, so nothing was changed in it.

---

## 7. Gates

| gate | result |
| --- | --- |
| wasm build from a CLEAN dir, all seven targets | builds |
| wasm `ctest` | 14/14 |
| native build + `legoland_tests` + `ctest` | 8/8 |
| generated host traps in `gen-browser/host_stubs.c` | **0** |
| `LEGOLAND/*.c` touched | **none** — no audit/relocs run needed |

## 8. Reproducing

```bash
PY=$HOME/.venvs/legoland/bin/python
emcmake cmake -S portable -B portable/build-wasm -G Ninja -DCMAKE_BUILD_TYPE=Release \
    -DLL_ILP32=ON -DPython3_EXECUTABLE=$PY
ninja -C portable/build-wasm legoland_browser legoland_browser_named
(cd portable/build-wasm && python3 -m http.server 8871)
# http://localhost:8871/legoland.html?args=-nointro+WINDEBUG          the page
# http://localhost:8871/legoland_dbg.html?args=-nointro+WINDEBUG      + a name section
```

then §3's script in the console. To name the next mismatch:

```bash
# llStats().dead carries the stack; take the innermost wasm-function[N]:0xOFF
python3 portable/tools/name_trap.py --wasm portable/build-wasm/legoland_dbg.wasm --at 0x<OFF>
```

Scratch scripts (not committed): `port-b7-serve.sh` (serve the build dir on a
port of its own), `port-b7-headless.sh` (run `legoland_headless` under a
wall-clock cap — the front end does not terminate), `port-b7-names.py` (map a
`wasm-function[N]` to its name through `name_trap.Module`),
`port-b7-vtsweep.py` (§4 B1a — every `__stdcall*` vtable slot declared in two or
more TUs, grouped by struct and by +0xNN, with the return-type disagreements
called out).
