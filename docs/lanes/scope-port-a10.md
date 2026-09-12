# Scope PORT-A10 — the dual-address gate, and the two PORT-P3 findings against the page

> **PORT-A10 — Status: IN PROGRESS (claimed 2026-09-12 by PORT-A10)** — branch
> `scope/PORT-A10`, cut from `feat/decomp-completion-next-steps-24a0d6`
> (`25005b2c`, the PORT-P3 merge). Tools and page only: no `LEGOLAND/*.c`, no
> `portable/src/hostwin/**`, no `portable/CMakeLists.txt`, no
> `docs/HANDOFF.md`. `tools/verify.py`, `audit.py` and `match.py` were not run.
> Brief: `docs/SCOPE_PORT_WAVE.md`.

**Three results, all measured.**

1. **`portable/tools/addr_sweep.py`** promotes PORT-P3's scratch script to a
   ctest with a baseline. It finds **18** names declared at more than one
   address, not 17: the eighteenth is `Track_Update`, a FUNCTION defined twice.
   `gen_link.py` now says so in the manifest, in a block comment at the top of
   the generated `globals.c` marking which address it actually emitted, and on
   stderr.
2. **P3-4 is two bugs, not one**, and both are now fixed and measured in
   tutorial lesson 1: `llLink`/`llPad` compared only the LLIDB *element* name,
   and `ObjDef +0x04` is **not** a complete placed-instance list — "Park
   Entrance" has an empty chain and 120 cells on the map. New `llFind(name)`,
   `llClasses()` and `llMapObjects()`.
3. **P3-7 is sharper than "throttled to 1 Hz" and the fix is cheaper than an
   oscillator.** A hidden tab runs at full rate for **thirty seconds** and then
   drops to exactly **0.50 fps**, flat, indefinitely: the whole main loop is one
   `setTimeout` chain and Chrome's background budget kills the chain. Delivering
   the 0/1 ms yields as **MessageChannel** messages — not timers, so no budget —
   holds **35.8 fps** straight through that cliff, with no AudioContext, no user
   gesture, no Worker and no library. `?awake=0` turns it off.

---

## 1. The dual-address gate

### 1a. What the class is, and why no byte gate can see it

`extern int g_view_left;  /* 0x004b95f4 */` in `scrolltick.c` and
`extern int g_view_left;  /* 0x008299ac */` in `coaster3d.c` are, on x86, two
different objects: each `.obj` carried its own address and the real linker gave
each spelling the memory it meant. The portable build has no `.obj` —
`gen_link.py` plans **one storage object per name** — so the two spellings
collapse, and every TU that spelled the loser is reading and writing memory
shifted by the difference, silently.

The C compiles to identical bytes either way, so `audit.py`, `relocs.py`,
`match.py` and `verify.py` are blind to this state **and to the fixed one**.
That is the same blindness `extern_sweep.py` was built for; this is its sibling.
`extern_sweep` catches one STATEMENT with several declarators and several
addresses. This catches one NAME with several addresses across FILES, which no
single statement reveals.

### 1b. The tool

```
python3 portable/tools/addr_sweep.py                    # sweep LEGOLAND/
python3 portable/tools/addr_sweep.py --baseline FILE    # the ctest gate
python3 portable/tools/addr_sweep.py --selftest         # shapes, no sources
python3 portable/tools/addr_sweep.py --names-only       # the cheap half
```

Two checks, because the hazard has two directions.

**NAME** — one name, several addresses. Citations come from three places, read a
*statement* at a time so a declaration wrapped over four lines is one citation
and not four:

* `extern <T> name ...;   /* 0x... */` for data,
* the same for function declarations, and
* `// FUNCTION: LEGOLAND 0x...` over a definition, which is the only address a
  recovered function HAS.

The address taken from a statement is the FIRST in its comments — deliberately
the same one `linkreport.scan_sources`, `gen_link`'s `DECL_RE` and
`cdecl._addr_for` take, because the sweep is about where those first-addresses
disagree between files. A statement with several declarators AND several
addresses is `extern_sweep`'s class and it gates that to zero.

**ADDR** — one address, several names whose declared sizes disagree. Sizes come
from `cdecl.py` (the only thing in the tree that knows `sizeof` of the game's
typedefs) and each name is taken at its WIDEST citation, because that is what
`gen_link` would size the object by. A narrower spelling that is **4 bytes or
fewer** is vetoed: that is the project's head-name convention
(`extern int g_copters_path0; /* 0x004c1124 */` is the first word of
`g_copters_paths`) and it reads the head correctly whichever object is emitted.
Without that veto the check reports 35 rows, 30 of them head names; with it, 5.

`--selftest` has a positive control for each half and the negatives that make a
gate possible at all: the same address from several files is **not** a collision,
a wrapped declaration is ONE citation, an address in prose above a declaration is
not a citation of it, a `#define` rename is recorded rather than ignored, and the
three baseline verdicts (accepted / GREW / FIXED) plus "a MISSING baseline fails
instead of passing quietly" are each exercised.

### 1c. What it finds: 18 names, not 17

PORT-P3's 17 reproduce exactly. The eighteenth is a **function**:

| name | addresses | verdict |
| --- | --- | --- |
| `Track_Update` | coaster.c:1809 `0x004275d0`, castleobj.c:1264 `0x00427b20` | **not a live defect** — two real definitions under two markers, and PORT-M7 already gave the second its own symbol with `#define Track_Update Track_Update_427b20` (castleobj.c:690), declaring coaster.c's as `Track_Update90`. The C has two distinct symbols; only the marker NAME still collides, which is all the sweep can see. The row is kept, with that reason, so the gate notices if the mitigation is ever dropped. |

P3's own table is otherwise confirmed name for name, and the generator's choice
is confirmed against the emitted `globals.c`: `g_popup` -> `0x007fdea4` (so
`fpui2.c` is the loser), `g_view_left` -> `0x008299ac` (so `scrolltick.c` is).

The 5 ADDR rows are all the project's documented "one object, several views"
convention, and the sources say so at each site:

| address | names | why it is accepted |
| --- | --- | --- |
| `0x0066809c` | `g_lock`=124, `g_ddsd`=108 | gpu.c:195 declares `struct LockState { DDSurfaceDesc ddsd; WinRect render_clip; }` and `#define g_ddsd g_lock.ddsd`, because `GenerateNewImageFromZBuffer` restores the descriptor with a `rep movsd` and the clip field by field |
| `0x007cad60` | `g_temp_profile`=272, `g_temp_name`=30 | the profile NAME is the record's first field |
| `0x007fded4` | `g_newobj`=168, `g_mock_defs`=80, `g_new_obj_defs`=80 | the "new object unlocked" carousel: one `NewObjStrip`, and two files naming its first array alone |
| `0x007fffc4` | `g_view`=36, `g_mapref`=8, `g_edit_cursor_origin`=8, `g_cursor_mapref`=4 | `EditCursor+0x1404`, the build cursor's map reference, spelled by fourteen files four ways |
| `0x00813a40` | `g_input`=164, `g_input_cfg`=156, `g_cursor`=36, three scalars | the input/cursor block. Also the address `g_ui_flags`' NAME row resolves to, so the two rows move together |

### 1d. The gate, and the race with PORT-M15

`portable/tests/addr_collisions.txt` holds all 23 rows with a reason each. The
rules are the ones `rawwords_baseline.txt` established:

* a row **not** in the file, or a row that **grew** an address or **changed** a
  size -> the test FAILS;
* a row in the file that is **no longer reported** -> the test passes and prints
  `FIXED`; drop the row in the same commit as the fix.

That last rule is the whole point. PORT-M15 is renaming the 17 in the game files
in parallel with this lane. With a "must be exactly this list" gate, whichever of
us merged second would break the build. With this one, the baseline holds P3's
open findings today and goes green the moment M15's renames land, and M15 tidies
the file whenever it likes. `scope/PORT-M15` had no commits beyond the merge base
when this was written, so the baseline holds all 17.

ctests: `addr_sweep` (the gate) and `addr_sweep_selftest`. Sources only — no
gamedata, no image, no build products — so both run in CI on both toolchains
beside `extern_sweep` and `bvstruct_sweep`. Native ctest 17 -> 19, wasm 24 -> 26.

### 1e. gen_link stops choosing silently

`lr.scan_sources()` keeps one address per name with a `setdefault`: the first
file wins and nothing downstream is told there was a second. That silent choice
is how the scroll clamp came to read the coaster's zeroed clip rect. The
generator cannot fix it — one of the two spellings needs a different name, and
`LEGOLAND/*.c` is not its to edit — but it can refuse to hide it. It now emits:

* `- names with conflicting addresses: 18 (Track_Update, g_avi_open_count, ...)`
  in the manifest summary;
* a `## Names declared at more than one address` section attributing every
  citation to `file:line`, with the emitted address in bold;
* a `gen_link: WARNING ...` line on stderr;
* and, at the top of the generated `globals.c`, a block comment a reader cannot
  miss:

```c
/* ==========================================================================
 * 18 NAME(S) ARE DECLARED AT MORE THAN ONE ADDRESS.
 * ...
 * `->` marks the address this file actually DEFINES the name at. Every other
 * address on the row is a spelling that silently lost.
 */
/*   g_popup
 *   -> 0x007fdea4  bighelp.c:456, popup.c:661
 *      0x007fdec0  fpui2.c:1443
 */
...
#ifdef LL_FAIL_ON_ADDR_COLLISION
#error "gen_link: 18 name(s) declared at two addresses -- ..."
#endif
```

The `#error` is opt-in (`-DLL_FAIL_ON_ADDR_COLLISION`) rather than on: the build
has to keep running while the known rows are open. After M15 lands it is the
one-flag way to keep a new one from being merged.

---

## 2. P3-4 — `llLink` / `llPad`, and the new `llFind`

Reproduced and fixed in **tutorial lesson 1** (`ObjList1`, 84x84 map, four
classes), served from `portable/build-wasm` on 8833, page
`legoland.html?args=-nointro+WINDEBUG`.

### 2a. It is two bugs

**THE NAME.** Every `ObjDef` carries two: the LLIDB element name
(`+0xc4 -> +0x00`) and the display name (`+0x78`). The old lookup compared only
the element name — so every name a player, a screenshot or `llCellAt` itself
would give came back `found: false`.

**THE INSTANCES.** `ObjDef +0x04` is not a complete placed-instance list.
Measured, in the same park:

| class | `+0x04` chain | on the map |
| --- | --- | --- |
| `LEGO SHOP 1` / "LEGO Toy Shop" | 1 instance, base (74,61) | 54 cells, anchor (74,61) — agrees |
| `ENTRANCE 1` / "Park Entrance" | **empty** | **120 cells**, anchor (81,40), bbox (78,32)-(83,51) |
| `PATH CONTROL` / "Path" | **empty** | 108 cells, anchor (44,31) |
| `SPACE TOWER RIDE` / "Space Tower Ride" | empty | nothing — correctly empty, it is the class lesson 1 asks you to build |

So the old probe's `{found: true, insts: []}` for a placed object was right about
the chain and wrong about the park, and a driver reading it concludes "the game
never instantiated it" — a defect that is not there. PORT-P3 lost ten minutes to
exactly that on the Mechanic's Hut.

### 2b. The fix

* `llClasses()` — the ObjDef census: both names, type, flags, entrance offset,
  footprint and the `+0x04` chain.
* `llMapObjects()` — the MAP census: every distinct object the cells reference,
  with the anchor the cells agree on (`Cell +0x04/+0x05`), the cell count, the
  bounding box and the flags. 84x84 is 7k reads; cheap enough to run per call.
* The shared matcher tries, in order: exact element name, exact display name,
  either case-insensitively, either **normalised** (upper-cased, non-alphanumerics
  stripped — which is what makes `MECHANICS HUT` match `Mechanic's Hut`).
  Among equal candidates the one with instances wins. `llLink` reports which
  rule fired in `matchedBy`.
* Instances come from the chain when it has any and from the map census when it
  does not. **Both sources report the same anchor** — the shop agrees at (74,61)
  either way — so `EventTick_Link`'s arithmetic is untouched. Each row says
  `source: 'chain' | 'map'` and the result carries `instSource`.
* `llPad` inherits all of it and, instead of a bare `null`, now returns
  `{notFound: true, ...}` or `{noInstance: true, ...}` so a null says which.
* `llFind(name)` — every PLACED instance with its class, both names, anchor,
  footprint, entrance offset, cell count, bbox and flags. The match is a
  substring of either normalised name, so `llFind('hut')` finds the hut. With no
  argument it is the whole park census.

One subtlety worth recording: the two sources name the same placed object by
**different pointers**. The map cells of the LEGO Toy Shop carry `0x00d00ffc`,
which is the class's own LLIDB *element*, while its `+0x04` chain node is
`0x0134a468`. `llFind` therefore deduplicates on (class, anchor), not on the
record address — keyed on the address it listed the one shop twice.

### 2c. Measured, before and after

| call | before | after |
| --- | --- | --- |
| `llLink('ENTRANCE 1')` | `found: true, insts: []`, verdict "no instance (vacuous)" | `matchedBy: "elem"`, `instSource: "map"`, 1 instance base (81,40), 120 cells |
| `llLink('Park Entrance')` | **`found: false`** | `matchedBy: "name"`, same instance |
| `llLink('park entrance')` | `found: false` | `matchedBy: "name (case)"` |
| `llLink('LEGO Toy Shop')` | **`found: false`** | `matchedBy: "name"`, `instSource: "chain"`, base (74,61) |
| `llPad('ENTRANCE 1')` | **`null`** | 154 cells, base (81,40) |
| `llPad('Park Entrance')` | `null` | 154 cells |
| `llLink('SPACE TOWER RIDE')` | `insts: []` | `insts: []`, `instSource: "none"` — the fix invents nothing |
| `llFind()` | — | 3 placed objects: Path (108 cells), Park Entrance (120), LEGO Toy Shop (54) |
| `llFind('SHOP')` | — | 1 |
| `llFind('mechanic')` | — | 0 (correct: lesson 1 has no hut) |

Nothing in `portable/src/browser/main.c` needed to change: every probe lives in
the shell page.

---

## 3. P3-7 — the hidden tab

### 3a. What is actually happening

P3 called it "Chrome throttles setTimeout to 1 Hz". Measured again, it is worse
and more specific. The port yields through `emscripten_sleep`, which the
generated loader implements as

```js
_emscripten_sleep = function (ms) { let innerFunc = () => new Promise(resolve => setTimeout(resolve, ms)) ... }
```

so **the game's entire main loop is one `setTimeout` chain**, and Chrome's
background budget applies to chains.

It does not apply immediately, which is why this is easy to miss and why a first
look can say "the hidden tab is fine". Frames presented per 10 s window, fresh
load, `?awake=0`, hidden background tab, nothing polling it:

| window (s) | frames | fps |
| --- | --- | --- |
| 0–10 | 336 | 33.6 |
| 10–20 | 335 | 33.5 |
| 20–31 | 335 | 33.5 |
| **31–41** | **5** | **0.50** |
| 41–51 | 5 | 0.50 |
| 51–61 | 5 | 0.50 |
| ... 61–121, every window | 5 | 0.50 |

**Full rate for ~30 seconds, then exactly five wake-ups per ten seconds,
flat, for as long as you leave it.** The `?beat=1000` heartbeat says the same
thing from inside the wasm — one line at `t=59493`, the next at `t=102493`,
**seven host calls in the 43 seconds between them** — with `since_yield` still
reading 0 ms, because from the game's point of view no time has passed at all.

Two traps for anyone re-measuring this. The first is the 30-second delay: a
measurement that runs for twenty seconds sees nothing wrong. The second is that
**driving the tab from a debugger un-throttles it** — a run polled every few
seconds with `javascript_tool` recovers to 33 fps and stays there, so the
numbers above come from an in-page sampler that was started, left alone, and
read once at the end.

### 3b. The three candidates, measured in the same hidden tab

| mechanism | measured | cost |
| --- | --- | --- |
| `requestAnimationFrame` | **SUSPENDED** — three frames did not arrive in six seconds | cannot be a hidden-tab fallback at all. It only helps the visible-but-unfocused case, which was never throttled here |
| a Web Worker timer | works: 0.3 ms for `setTimeout(0)`, 5.8 ms for `setTimeout(4)` | a Worker, a Blob URL and a `postMessage` round trip per yield — and the game yields ~1400 times in five seconds |
| a `MessageChannel` hop | **0.003 ms** per hop over 200 hops | one channel, one queue. A port message is not a timer, so no timer budget applies |
| P3's audio oscillator | works | an AudioContext the autoplay policy will not start without a user gesture — and a page driven by a test runner may never get one — plus a permanently audible tab |

### 3c. What the page does now

While `document.hidden` is true, a `setTimeout` of 0 or 1 ms with a single
function argument is delivered as a `MessageChannel` message instead of a timer.
Everything else — every timer longer than 1 ms, every call with extra arguments,
every call while the tab is visible — goes to the real `setTimeout`, unchanged.
Longer timers are deliberately left alone: they are real waits (the 5 s profile
flush, `MessageBoxA`'s pause) and hurrying them would change behaviour.

A hopped call still gets a handle (a negative one, so it cannot collide with a
real timer id) and `clearTimeout` still cancels it. Nothing in the yield path
cancels a sleep, but a substituted `setTimeout` that quietly cannot be cleared is
a trap for whatever is added next, and the bookkeeping is two lines.

The patch is installed in the shell's own script block, which runs before
`{{{ SCRIPT }}}`, so the module's `setTimeout` lookup finds it.

**Measured, same protocol, same tab, `?awake=1` (the default):**

| window (s) | frames | fps |
| --- | --- | --- |
| 0–10 | 358 | 35.8 |
| 10–20 | 357 | 35.7 |
| 20–30 | 357 | 35.7 |
| **30–41** | **378** | **35.7** |
| 41–51 | 357 | 35.7 |
| 51–61 | 357 | 35.7 |
| 61–71 | 357 | 35.7 |
| 71–81 | 357 | 35.7 |
| 81–91 | 357 | 35.7 |

3266 frames in 92 s, on a tab that had by then been hidden for two and a half
minutes — and `llAwake()` reported **46570 hops against 2 real timers**, which is
the other half of the claim: the yield is essentially the only thing being
substituted. Flat through the 30-second cliff and past it, at the game's own
ceiling
(`FlipPrimary` spins until 28 ms have passed = 35.71 fps). `llStats().traps`
stayed 0 and `llStats().dead` null, and the whole lesson-1 walk in §2 was driven
in a hidden tab at 35.7 fps. **0.50 fps -> 35.8 fps, a factor of 71.**

### 3d. The trade-off, stated plainly

A hidden tab now **burns a full core**, because the game really is running. For
a testing page that is the point: every replay in
`portable/src/browser/replays/` is driven from a pane that is hidden more often
than not, and a driver cannot tell "the click did nothing" from "the tab was
asleep". For a **shipping** page it is wrong — a player who switches tabs expects
the game to idle — so it is a switch, `?awake=0` turns it off, and a shipping
shell should default it off. `llAwake()` reports whether it is installed and
engaged and how many hops and real timers have gone through it; `llAwake(false)`
/ `llAwake(true)` pin it for a measurement, `llAwake(null)` hands it back to the
visibility state.

No audio permission is involved at all, which is the concrete advantage over the
oscillator: nothing needs a user gesture, nothing makes noise, and a headless
driver with no gesture available gets the same 35.8 fps.

---

## 4. Census

| | before | after |
| --- | --- | --- |
| native ctest | 17 | **19** (`addr_sweep`, `addr_sweep_selftest`) |
| wasm ctest | 24 | **26** |
| names declared at >1 address | unsurfaced | **18**, all attributed, gated |
| addresses with incompatible sizes | unsurfaced | **5**, all ruled on |
| gen_link manifest rows | — | `+ names with conflicting addresses` and a section |
| page probes | 23 | **27** (`llFind`, `llClasses`, `llMapObjects`, `llAwake`) |
| hidden-tab fps (after the 30 s cliff) | 0.50 | **35.8** |

## 5. Files touched

| file | change |
| --- | --- |
| `portable/tools/addr_sweep.py` | new — the two checks, the baseline reader, `--selftest` |
| `portable/tests/addr_collisions.txt` | new — 18 NAME rows and 5 ADDR rows, each with a reason |
| `portable/cmake/headless.cmake` | the `addr_sweep` and `addr_sweep_selftest` ctests |
| `portable/tools/gen_link.py` | the manifest line, the manifest section, the stderr warning, the `globals.c` block and the opt-in `#error` |
| `portable/src/browser/index.html` | `llClasses`, `llMapObjects`, `llFind`, the shared matcher and the map fallback in `llLink`/`llPad`; the keep-awake and `llAwake` |
| `docs/SCOPE_PORT_WAVE.md` | the PORT-A10 status line |
| `docs/lanes/scope-port-a10.md` | these notes |
| `README.md` | the PORT-A10 section |

No `LEGOLAND/*.c`, no `portable/src/hostwin/**`, no `portable/CMakeLists.txt`,
no `docs/HANDOFF.md`, and `portable/src/browser/main.c` needed no change.
`tools/verify.py`, `audit.py` and `match.py` were not run.

## 6. For the integrator

* **`addr_sweep` will stay green through PORT-M15's merge in either order.** A
  baseline row that stops being reported prints `FIXED` and passes. When M15
  lands, drop the fixed rows from `portable/tests/addr_collisions.txt`; the
  sweep's own output names exactly which.
* The `Track_Update` row is the one row in that file that is **not** open work.
  Do not hand it to M15 as an 18th rename — PORT-M7 already solved it, and
  re-solving it would only remove the `#define` the row exists to protect.
* `gen_link.py` now imports `addr_sweep`, so the two files move together.
* The keep-awake makes a hidden tab spin a core. If a future shipping shell is
  cut from this page, flip the `?awake` default (one line: `awakeWanted`).
* `portable/src/browser/replays/p3-lesson4.js` still carries `p3KeepAwake()`
  (the oscillator). It is harmless and is left alone — the page no longer needs
  it, and the replay is PORT-P3's file.
