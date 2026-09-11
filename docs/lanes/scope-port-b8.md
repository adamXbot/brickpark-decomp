# Scope PORT-B8 — from the main menu into the park

> **Status: IN PROGRESS (claimed 2026-09-12 by PORT-B8).** Branch
> `scope/PORT-B8` from `feat/decomp-completion-next-steps-24a0d6` @ `9c646312`
> (the PORT-M5 merge). Files owned: `portable/src/hostwin/{ddraw,user32,gdi32,
> dinput,winmm,dsound,avifil32,msacm32}.c`, `portable/src/browser/**`,
> `portable/cmake/browser.cmake`, additions to
> `portable/hostwin/include/ll_host.h`. `LEGOLAND/*.c` is read-only for this
> lane and was **not touched**, so the VC6 gate has nothing to check.

---

## 1. Headline

**B2 is diagnosed, named, and it is not in the shim.** The click that starts the
park hangs in

```c
/* screens3.c:1313  KillLowMarkerSprites, 0x0048bd70 */
while (KillSprite(g_low_markers[i].lit) == 0)
    ;
```

because `g_low_markers[i].lit` is **NULL** — and it is NULL because
`LoadLowMarkerSprites` asked for the sprite by a name pointer the generated
closure never re-pointed. `gen-browser/globals.c` emits, verbatim:

```c
/* 0x004beca0 .data 600 bytes */
__attribute__((aligned(16))) unsigned int g_low_markers[150] = {
    0x004bed40u, 0x004bed2cu, 0x00000014u, 0x0000001eu,   /* <- RAW x86 VAs */
    0x00000082u, 0x00000000u, 0x00000000u, 0x004bed40u,
    ...
    0x000000e2u, 0x00000000u, 0x00000000u, 0x72707041u,   /* "Appr" */
```

`0x004bed40` and `0x004bed2c` are `"Appraisal_Yes.lls"` and
`"Appraisal_No.lls"` — and they are **inside this very object**, at `+0xa0` and
`+0x8c` (the last four words above are the start of `"Appraisal_No.lls"`).
`UnreferenceSprite` (0x00497bd0, the function `KillSprite` names) returns 0 for a
NULL sprite, so the loop is infinite, makes **no host call of any kind**, and
takes the page's main thread with it. `g_level_markers` (the levels 6..15 screen)
has the same 18 words wrong, so `KillLevelMarkerSprites` — `ProgressAcceptInput`,
the other way into a park — hangs identically.

The class is 1211 words in 177 objects (§2d). **Owner: PORT-A, `gen_link.py`.**

Two other things settled on the way:

* **PORT-B7's second B2 observation does not reproduce.** The advert screen's
  Go Back at (69, 386) now returns to the main menu cleanly
  (`0xa3e7d020` -> `0x689f8fd8`, 33.3 fps, no trap). That click was measured on
  B7's throwaway build; on an honest build with B1 merged it is fine.
* **The profile save path works, and the `"cannot open output file"` lines in
  the console are not a fault.** They come from `ScanForProfiles`
  (profiles.c:351) probing the seven *absent* slots in READ mode, seven per
  call. Accepting the name editor writes `/gamedata/profiles/Profile1.txt`,
  272 bytes (`0x110`, the `fwrite(&g_temp_profile, 0x110, 1, f)`), with `adam`
  at offset 0, and the game reads it back within the session (§5).

**The park was not reached**, because the only two doors into it both hang on
the same closure defect and the fix is in another lane's file. There is
therefore no map-screen hash and no park fps in this lane; §3 records the front
end, and §6 says who owns what.

---

## 2. B2 — the non-yielding loop

### 2a. The instrument: the heartbeat

PORT-B7 left B2 as "the page's main thread stops responding", which is the one
state this port had no way to interrogate: `llFrameHash`, `llStats`, `llPeek`
and every `javascript_tool` call run ON that thread, so all of them time out
together and none of them can say why. (Nor can the tab be navigated away:
`navigate` on a wedged tab times out at 300 s. Close it and open a new one.)

`stderr` still works. The devtools console is filled by the browser process over
CDP, not by the page's main thread, so a line written from C inside a loop that
never yields still arrives and `read_console_messages` still returns it.

`ll_host_beat(const char*)` (user32.c, declared in `ll_host.h`) is therefore one
call at the top of every host entry point a spin loop could plausibly sit on:

| file | entry points beaten |
| --- | --- |
| `ddraw.c` | `Lock` `Unlock` `Blt` `BltFast` `Flip` `GetFlipStatus` |
| `user32.c` | `PeekMessageA` `WaitMessage` `DispatchMessageA` `DrawTextA` |
| `winmm.c` | `timeGetTime` |
| `dinput.c` | `GetDeviceState` `GetDeviceData` |
| `dsound.c` | `GetCurrentPosition` `GetStatus` `Lock` `Play` |
| `msacm32.c` | `acmStreamConvert` |

It prints two things, and only when `$LL_HOST_BEAT` is set (the page's
`?beat=<ms>`; a normal run pays one predictable branch per host call):

* a **timed line** every `<ms>` — the last entry point, the calls since the
  previous line, the total, and the wall clock since the last yield;
* a **32-entry ring of the DISTINCT entry points** with their repeat counts,
  dumped on every **wrap** rather than on the timer. That is the part that
  matters. A timed line is up to `<ms>` stale, and the front end makes ~360,000
  host calls a second, so a timed line can never carry the last calls before a
  wedge. Counting in *transitions* instead means the last line a wedged tab
  prints always ends within 32 distinct host calls of the loop the game never
  left.

### 2b. What a healthy front-end frame looks like

```
BEAT t=11483 last=winmm.timeGetTime calls=181126 total=2717843 since_yield=0ms
BEAT   ring: ... ddraw.Blt*1 ddraw.Lock*1 ddraw.Unlock*1  (x25)
             user32.PeekMessageA*1 dinput.GetDeviceState*2
             ddraw.Lock*1 ddraw.Unlock*1 winmm.timeGetTime*21384
             ddraw.Blt*1 winmm.timeGetTime*1
```

Read right to left that is one frame: ~25 sprite blits, the pump's single
`PeekMessageA`, the keyboard and mouse polls, the back-buffer lock, then
**21,384 `timeGetTime` calls** inside `FlipPrimary`'s 28 ms floor, then the
present `Blt`. See §4.

### 2c. The measurement

`?args=-nointro+WINDEBUG&beat=3000`, the walk of §3 to the tutorial screen, then

```js
console.error('B8MARK ==== clicking ACCEPT_ON_REPORT (577,419) ====');
llClick(577, 419);          // NOT awaited: the await never returns
```

and the console read back over CDP afterwards. Three facts:

1. The page wedges — every later `javascript_tool` call times out at 45 s, and
   it survives four minutes (PORT-B7 saw 2.5).
2. **The heartbeat stops dead at the click.** The last line is an ordinary
   frame's ring; nothing follows it, ever. So the loop calls **no** ddraw, no
   user32, no winmm, no dinput, no dsound entry point — it is not a shim call
   that forgot to yield, and it is not a long load either (a load presents
   through `progress_tick`, which is `Lock`/`Blt`).
3. The last ring ends in the front-end frame's own draw — `DrawTextA`, the
   blits, the present — which is where `RenderFrontEndScreen` runs
   `CheckFocussedIcon`, i.e. exactly where an icon's `+0x2c` input handler is
   called. The loop is inside the handler.

The handler is `LowProgressAcceptInput` (screens3.c 0x0048bf90). Its first act
past the click guard is `KillLowMarkerSprites()`, and that is §1's `while`.

### 2d. The closure defect, measured

`portable/tools/gen_link.py` re-points a data word that holds an x86 address to
the linked object. It does **not** do so when the target has no symbol of its
own but lands *inside* an object whose extent was widened to swallow it — which
is precisely what PORT-A6's `cdecl.py` widening produces: `g_low_markers` is
emitted as 600 bytes (5 x 0x1c of records = 140, then 460 bytes of the string
literals that followed it in `.data`), and there is **no** `/* 0x004bed40 ... */`
header anywhere in `globals.c`, no alias in `aliases.c`, and no runtime patch.
The word is emitted raw and the game dereferences `0x004bed40` as a linear
memory address.

`port-b8-rawptr.py` (scratch) parses every `/* 0xVA .sec N bytes */` object out
of `gen-browser/globals.c`, builds the address ranges, and counts initialiser
words in `[0x00401000, 0x00900000)` that land inside one of them. A word in that
range is a pointer with near-certainty: coordinates and small enumerations are
below it, and packed ASCII (`0x72707041` = `"Appr"`) is above it.

```
emitted objects: 2042
raw-VA words that land inside an emitted object: 1211, in 177 objects

  GUID_NULL                      264 words -> GUID_NULL, g_build_objs, g_cert_message, ...
  g_fp_table                     135 words -> g_bs_fx, g_bs_water_cursors, g_castle_desc, ...
  kThemeSame                     102 words -> g_outfitA_tab1, g_outfitB_n1, g_outfitB_pal0, ...
  g_level_db_sections             91 words -> g_level_db_sections, g_str_none, g_str_purge
  g_power_table                   63 words -> g_str_driving_school_roads, g_view_oy, ...
  g_near_offsets                  63 words -> GUID_NULL, g_cert_message, g_name_space, ...
  g_lowlevel_ai                   59 words -> g_jc_cursors, g_jctree_cursors, g_render2_b, ...
  g_build_followups               46 words -> g_bs_fx, g_castle_desc, g_lf_norect_msg, ...
  kWaterWorksElephantFountain     20 words -> g_render2_b, g_zbuf_pixels, g_zbuffer, ...
  g_level_markers                 18 words -> g_low_markers            <-- B2
  g_low_markers                   17 words -> g_low_markers, g_zbuffer_storage   <-- B2
  ...
```

The two B2 rows are the whole of the progress screens' sprite names:

| word | raw value | is | wants to be |
| --- | --- | --- | --- |
| `g_low_markers[i].lit_name`, i = 0..4 | `0x004bed40` | `"Appraisal_Yes.lls"` | `(unsigned)((char*)g_low_markers + 0xa0)` |
| `g_low_markers[i].dim_name`, i = 0..4 | `0x004bed2c` | `"Appraisal_No.lls"` | `(unsigned)((char*)g_low_markers + 0x8c)` |
| `g_level_markers[i].lit_name/.dim_name`, i = 0..8 | `0x004bedf4` .. `0x004beeb8` | `Pro_Belgium_Lit.lls` … `Pro_Egypt_Unlit.lls` | `g_low_markers + 0x154` .. `+ 0x218` |

Both sprite files exist in the archives and are the right size, so nothing is
missing from the assets:

```
$ python3 tools/resfile.py list gamedata/disc/Graphics1.res --all | grep -i appraisal
  79  off=0x004dec6c  size=  710  appraisal_no.lls    COMP 36x27 bpp16
  80  off=0x004def34  size=  636  appraisal_yes.lls   COMP 36x27 bpp16
```

**The visible symptom, which was on screen the whole time:** the tutorial screen
draws its five lesson names and **no tick** beside the current lesson.
`InitTutorialScreen` inserts one icon per marker with `g_low_markers[i].lit`,
and that sprite is NULL.

### 2e. What the shim did NOT need

Nothing. Every yield point behaved: `PeekMessageA` yields once per pass,
`timeGetTime` yields every 4 ms, `Sleep` goes through `ll_host_sleep_hook`, and
the heartbeat shows `since_yield` at 0-4 ms on every line of a healthy run. The
wedge is a game-visible loop fed a null pointer by the closure; no shim change
can reach it, and none was made.

---

## 3. Screens reached, with the exact input, and the hashes

`?args=-nointro WINDEBUG`, a fresh load each time, driven by the page's own
`llMove/llClick/llType` (PORT-B7; the browser automation's pointer and key verbs
still do not reach this canvas).

```js
await llMove(320, 240);          // REQUIRED FIRST: sync the relative pointer
await llClick(260, 188);         // slot 1 -> NEW PROFILE popup
await llType('adam');
await llClick(505, 345);         // Accept  -> the MAIN MENU
await llClick(252, 362);         // New     -> "Select tutorial level"
// await llClick(577, 419);      // Accept_On_Report -> WEDGES (§2)
```

| # | screen | input | frame hash | state |
| --- | --- | --- | --- | --- |
| 1 | **PLAYER DETAILS**, eight EMPTY slots | load | `0x9d9b7c50` | 96.5% non-black, 32-33 fps, 0 traps |
| 2 | NEW PROFILE popup on slot 1 | `llClick(260,188)` | `0x81316ae1` | `llPeek()` -> `newProfilePopup:1, curProfileSlot:1` |
| 3 | the field reading `adam|` | `llType('adam')` | `0xb3f74238` | `profileName:"adam", profileLen:4, nameWidth:36` |
| 4 | **the MAIN MENU** (the LEGOLAND gates) | `llClick(505,345)` | `0x865e2ebd` / `0x7e019630` | `Profile1.txt` now exists (§5) |
| 5 | PLAYER DETAILS again, slot 1 reading **adam** | `llClick(74,163)` (Reg) | `0xa699e321` | read back off disk |
| 6 | slot 1 selected (no popup) | `llClick(260,188)` | `0x428bb06c` | |
| 7 | **"Select tutorial level"** — the notepad, Lessons 1-5 | `llClick(252,362)` (New) | `0x83065c2b` / `0x1e3888c4` | 98.3% non-black. **No tick beside Lesson 1** — §2d |
| 8 | **the park-advert screen** — CALIFORNIA / WINDSOR / BILLUND | `llClick(320,211)` (Movie) | `0x25b199d4` / `0xa3e7d020` | 94.5% non-black |
| 9 | the same, tooltip "LEGOLAND Windsor movie" | `llMove(318,73)` | `0xff014614` | |
| 10 | back at the MAIN MENU from the advert screen | `llClick(69,386)` (Go Back) | `0x689f8fd8` | **B7's second wedge does not reproduce** |
| 11 | main menu, tooltip "Saved games" | the same move | `0x689f8fd8` | |
| — | **the park** | `llClick(577,419)` | — | **wedges, §2** |

### 3a. The main menu's six icons, in game pixels

Measured against `InitTitleScreen` (screens2.c 0x0048fc40), whose table gives
each sprite's TOP-LEFT; the bubbles are 110x110, so the centre is `+55`:

| icon | top-left (source) | centre to click | handler |
| --- | --- | --- | --- |
| New | `(0xca,0x138)` = (202,312) | **(252,362)** | `TitleNewInput` -> progress / tutorial screen |
| Free | `(0x9a,0x08)` = (154,8) | (204,58) | `TitleFreeInput` -> screen 3; **inert today**, the icon does not take the click |
| Reg | `(0x18,0x71)` = (24,113) | (74,163) | `TitleRegInput` -> PLAYER DETAILS |
| Exit | `(0x1e1,0x13)` = (481,19) | (531,69) | `TitleExitInput` |
| Load | `(0x19,0x118)` = (25,280) | (75,330) | `TitleLoadInput` -> saved games |
| Movie | `(0x10e,0xa1)` = (270,161) | (320,211) | `TitleMovieInput` -> advert screen |

and the tutorial screen's two bubbles (`InitTutorialScreen`, screens3.c
0x0048bde0; both 110x110 per `resfile.py`):

| icon | top-left | centre | handler |
| --- | --- | --- | --- |
| `Goback_On_Tut.lls` | `(0x20a,0xf5)` = (522,245) | (577,300) | `ProgressTutorialInput` — **also hangs**, same `KillLowMarkerSprites` |
| `Accept_On_Report.lls` | `(0x20a,0x16c)` = (522,364) | **(577,419)** | `LowProgressAcceptInput` — **the door to the park**, hangs |

PORT-B7's `(575,297)` was the upper bubble (Go Back / Tutorial), not the lower
one; both hang, for the same reason, which is why the park was never reached
from either.

---

## 4. Performance

| where | fps | note |
| --- | --- | --- |
| PLAYER DETAILS, idle | 32.1-33.4 (avg 32.4) | |
| NEW PROFILE popup, editor running | 32.6 | |
| main menu | **33.3** (avg 32.6), `llAlive()` = 17 frames / 500 ms | |
| tutorial screen | 32.5 | |
| park-advert screen | 32.6 | |
| **the park** | **not measured — the park was not reached** (§1, §2) | |

Every front-end screen sits on the 28 ms flip floor (35.7 Hz), unchanged from
PORT-B4's 33.5 and PORT-B7's 32.7. Nothing in the present path was found to fix
and nothing in it was changed.

**One number worth carrying forward, which the heartbeat is the first thing to
measure:** the front end makes **~360,000 host calls a second**, and
**~180,000 of them per 500 ms are `timeGetTime`** — a single frame's flip spin
is 21,384 calls (§2b). `FlipPrimary`'s `while (timeGetTime() - g_flip_time <
0x1c) ;` is a busy-wait that the shim can only make *safe* (it yields every
4 ms), not *cheap*. At the front end's 33 fps there is CPU to spare, so this
costs nothing visible; in the park, where the software renderer will want every
cycle, a spin burning a third of a million calls a second is the first thing to
look at. The shim cannot shorten the spin without changing what the game
measures — the honest fix is for `timeGetTime` to return the *floor's end* once
the yield has actually slept past it, which is a behaviour change and wants a
lane of its own with the park's numbers in hand.

---

## 5. Save / load

**Within a session: it works.** Measured from the page, with the game's own
`FS` (`?args=-nointro WINDEBUG`, no `beat`):

```js
FS.readdir('/gamedata/profiles')            // before: []
await llMove(320,240); await llClick(260,188);
await llType('adam'); await llClick(505,345);
FS.readdir('/gamedata/profiles')            // after:  ['Profile1.txt'] (272 bytes)
FS.readFile('/gamedata/profiles/Profile1.txt').slice(0,4)  // 61 64 61 6d = "adam"
```

272 = `0x110`, exactly `SaveProfileToDisk`'s `fwrite(&g_temp_profile, 0x110, 1, f)`
(profiles.c 0x00491910). Going back to PLAYER DETAILS then shows **adam** in
slot 1, drawn from the file: `ScanForProfiles` counts it and the list reads it.

**The `"cannot open output file"` lines are not a fault.** `ScanForProfiles`
(profiles.c:341-357) opens `profiles\Profile%d.txt` for `i = 1..7` in **READ**
mode and `printf(g_msg_cannot_open)` for each one that is absent — seven lines
per call, as shipped. The backslash is handled: `ll_host_resolve_path`
(kernel32.c) normalises `\` to `/`, and the `create` flag stops it rewriting a
path that is being made. `main.c` already creates `/gamedata/profiles` before
`WinMain`. Nothing here needs changing; PORT-B7's reading of those lines as "the
last console lines before the wedge" was a coincidence of them being the only
lines the run ever printed.

**Across a page reload: it does not persist, by construction.** The profile
lives in MEMFS, which is built fresh from the `.data` package on every load, so
a reload always starts at eight EMPTY slots. Making it persist is a ~40-line
change in `portable/src/browser/main.c` — `FS.mount(IDBFS, {}, '/gamedata/profiles')`,
one `FS.syncfs(true, ...)` before `WinMain` (awaitable under ASYNCIFY, which
this target already has, by spinning `emscripten_sleep` on a flag the callback
clears), a `FS.syncfs(false, ...)` on a timer or after each present-count
milestone, and `-lidbfs.js` on the link. It was **not** done in this lane: the
link alone costs 10-15 minutes on this machine under the current load and the
B2 diagnosis had the budget. It is a clean, self-contained next job — **owner:
the next PORT-B lane**, and nothing else depends on it.

---

## 6. Blockers, with owners

| # | what | owner | state |
| --- | --- | --- | --- |
| **B2** | **1211 data words in 177 objects of `gen-browser/globals.c` are still RAW x86 addresses.** They point *inside* an object whose extent `cdecl.py` widened to swallow them, so there is no symbol to re-point to and `gen_link.py` emits the VA. 30 of them are the progress screens' sprite names, which makes `LoadSprite` fail, which makes `g_low_markers[i].lit` NULL, which makes `while (KillSprite(NULL) == 0) ;` (screens3.c:1320, 1273) an infinite loop with no host call — **the only two doors into the park** | **PORT-A** (`portable/tools/gen_link.py`) | diagnosed, quantified, recipe in §2d. The fix is one rule: a pointer word whose target has no symbol but lies inside an emitted object's extent becomes `(unsigned)((char*)<obj> + <offset>)`. `port-b8-rawptr.py` is the census and can be the gate |
| B2a | the same 1211 words are a latent fault everywhere else they are read — `g_fp_table` (135), `kThemeSame` (102), `g_level_db_sections` (91), `g_power_table` (63), `g_lowlevel_ai` (59) are all tables the park needs | PORT-A | same fix; expect more of the park to work the moment it lands |
| B3 | `while (KillSprite(x) == 0) ;` is unguarded against a failed load in FOUR places (screens3.c:1320/1322, 1281/1283). Even with B2 fixed, any sprite the archives cannot serve hangs the program rather than degrading | a matching lane (PORT-M) — **record only**; the original has the same loop and the bytes must not move. Worth a `LEGOLAND_PORTABLE` guard only if PORT-A cannot close B2 | recorded, not fixed |
| B4 | `TitleFreeInput`'s Free-play bubble (204,58) does not take a click on the main menu with a fresh profile | unknown | observed once, not diagnosed |
| B5 | profiles do not survive a page reload (MEMFS) | PORT-B (next lane) | §5, ~40 lines + `-lidbfs.js` |
| B6 | **a wedged tab cannot be navigated away** — `navigate` times out at 300 s. Close the tab and open a new one | tooling | worked around; noted here so the next lane does not lose five minutes to it |
| B7 | PORT-B7's second wedge (the advert screen's Go Back) **does not reproduce** on an honest build | — | closed, §3 row 10 |

---

## 7. Gates

| gate | result |
| --- | --- |
| wasm build, all seven targets | builds |
| wasm `ctest` | **16/16** |
| native `cmake -S portable -B portable/build && ninja` | builds |
| `LEGOLAND/*.c` touched | **none** — no audit/relocs run needed |
| generated host traps | **0** |
| page, front end, traps | **0**, 32-33 fps |

**Build note for the next lane:** this machine ran at load average 90 during
this lane (several agents building at once) and a `legoland_browser` link took
10-15 minutes. Do not start two `ninja` runs in the same build directory — two
concurrent links produced a `legoland.wasm` with **no ASYNCIFY instrumentation**
(`Aborted(Assertion failed: missing Wasm export: asyncify_start_unwind)` at
instantiate, and a 991 KB rather than 1336 KB module). `rm -f legoland.wasm
legoland.js legoland.html` and relink once.

## 8. Reproducing

```bash
PY=$HOME/.venvs/legoland/bin/python
emcmake cmake -S portable -B portable/build-wasm -G Ninja -DCMAKE_BUILD_TYPE=Release \
    -DLL_ILP32=ON -DPython3_EXECUTABLE=$PY
ninja -C portable/build-wasm legoland_browser
(cd portable/build-wasm && python3 -m http.server 8801)
# http://localhost:8801/legoland.html?args=-nointro+WINDEBUG            the page
# http://localhost:8801/legoland.html?args=-nointro+WINDEBUG&beat=3000  + the heartbeat
```

then §3's script in the console. To see B2 without a browser at all:

```bash
grep -A3 '0x004beca0 .data' portable/build-wasm/gen-browser/globals.c
python3 <scratch>/port-b8-rawptr.py   # the 1211-word census
```

Scratch scripts (not committed): `port-b8-rawptr.py` (§2d's census),
`port-b8-build-wasm.sh`, `port-b8-relink.sh`.

## 9. Appendix — the census script

Not committed as a tool (`portable/tools/*` is PORT-A's). Copy it wherever it
is wanted; it reads only `gen-browser/globals.c`.

```python
"""Count initialiser words in the generated closure that are still RAW x86
addresses pointing inside another emitted object.  A word in
[0x00401000, 0x00900000) is a pointer with near-certainty: coordinates and
small enumerations are below it, packed ASCII is above it."""
import re, sys, bisect, collections

path = sys.argv[1] if len(sys.argv) > 1 else "portable/build-wasm/gen-browser/globals.c"
lines = open(path).read().split("\n")
hdr = re.compile(r"/\* (0x[0-9a-f]{8}) (\.\w+) (\d+) bytes")
word = re.compile(r"0x([0-9a-f]{8})u")

objs, pending = [], None
for i, ln in enumerate(lines):
    m = hdr.search(ln)
    if m:
        pending = (int(m.group(1), 16), int(m.group(3)))
        continue
    m2 = re.match(r"__attribute__\(\(aligned\(\d+\)\)\) \w[\w ]*?\**\s*(\w+)\[(\d+)\]", ln)
    if m2 and pending:
        objs.append((pending[0], pending[1], m2.group(1), i))
        pending = None
objs.sort()
starts = [o[0] for o in objs]

def owner(va):
    k = bisect.bisect_right(starts, va) - 1
    if k < 0:
        return None
    s, n, nm, _ = objs[k]
    return nm if s <= va < s + n else None

at, cur = {o[3]: o for o in objs}, None
bad, badobj = collections.Counter(), collections.Counter()
for i, ln in enumerate(lines):
    if i in at:
        cur = at[i]
    if cur is None:
        continue
    if ln.startswith("};"):
        cur = None
        continue
    for m in word.finditer(ln):
        v = int(m.group(1), 16)
        if 0x00401000 <= v < 0x00900000:
            t = owner(v)
            if t:
                bad[(cur[2], t)] += 1
                badobj[cur[2]] += 1

print("emitted objects: %d" % len(objs))
print("raw-VA words that land inside an emitted object: %d, in %d objects"
      % (sum(bad.values()), len(badobj)))
for name, n in badobj.most_common(25):
    tgts = sorted({t for (s, t), c in bad.items() if s == name})
    print("  %-34s %4d words -> %s" % (name, n, ", ".join(tgts[:4])))
```
