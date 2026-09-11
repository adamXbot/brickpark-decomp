# Scope PORT-B10 — PLAY the park: the tutorial, end to end

> **PORT-B10 — Status: IN PROGRESS (claimed 2026-09-12 by PORT-B10)** — branch
> `scope/PORT-B10`, cut from the PORT-M9 merge. Host-shim side
> (`portable/src/hostwin/**`, `portable/src/browser/**`, `ll_host.h`).
> `LEGOLAND/*.c` is READ-ONLY for this lane: game-side defects are recorded
> with file, line and an owner, never edited.
> Brief: `docs/SCOPE_PORT_WAVE.md`.

**Result in one line: the tutorial is playable.** The Duty Manager's briefing
is read, both its pages turned, the LEGOLAND menu opened, a Space Tower Ride
bought for 40 bricks, paths laid by dragging, the park saved through the game's
own Save Game screen, the page reloaded, and the saved park loaded back —
**34.7 fps, `llStats().dead` null, `traps: []` throughout, no trap named all
session.**

Two defects were found and fixed in the shim, both in the same file and both
with the same shape: the face PORT-B2 drew is monospaced and too tall, and the
game's own shipped data is written for a face that is neither. One further
class is open and is game-side (§4).

---

## 1. The walk — every screen, its input, its hash

Served `portable/build-wasm` on 8806 and opened
`legoland.html?args=-nointro+WINDEBUG&beat=1000`. Every step is a page-hook
call in GAME pixels; `llFrameHash()` after each.

**Start from a virgin IDBFS**, or the walk diverges at step 4: once a saved game
exists, the profile's OK leads to the TITLE screen (which now has "Saved games"
lit) instead of straight to the tutorial select, and every hash on the way
changes with it. In the page console:

```js
for (const d of await indexedDB.databases())
  indexedDB.deleteDatabase(d.name);       // "/gamedata/profiles"
location.reload();
```

The `money=` column is `llPark().money`, i.e. `g_bricks`, i.e. the number the
money bar prints. It is the load-bearing half of this table: the hashes say the
frame changed, the money says the GAME did something.

```js
// --- the front end -------------------------------------------------------
await llMove(320, 240);      // 0x8ef2428c  the cursor and the game agree
await llClick(260, 188);     // 0x50684f2d  slot 1 -> the name editor
await llType('adam');        // 0x23d5edf9  read back live from g_temp_profile
await llClick(505, 345);     // 0x87948170  OK -> the tutorial/progress screen
await llClick(252, 362);     // 0xe59fe2c4  TUTORIAL level 1
await llClick(577, 419);     // 0x99551094  "To the game.." -> BRIEFING p1
// --- the Duty Manager's briefing (front-end screen 7, the notepad) -------
await llClick(455, 445);     // 0x08b70513  the flashing arrow -> p2
await llClick(577, 419);     // 0x2a6f7198  Thumbs Up -> THE PARK   money=1000
// --- the park ------------------------------------------------------------
await llClick(558, 243);     // 0xbce8c330  close "You have a new object"
await llClick( 53, 394);     // 0x201fb5cd  the flashing LEGOLAND button
await llClick(558, 243);     // 0xa09f9618  close "Space Tower Ride"  money=1030
await llClick( 27, 156);     // 0x8d45f500  pick the ride in the menu
await llClick(330, 200);     // 0x9eb191f8  BUILD IT -> "Your First Ride!"
                             //                                       money=990
await llClick(577, 419);     // 0xa2505fa9  Thumbs Up -> back to the park
// --- paths (a DRAG, not a click: see §3) ---------------------------------
await llClick( 40, 443);     // 0xe8005f18  arm the PATH tool
await llDrag(292, 296, 266, 232, 10);  // 0x51af50cc  a run of path
await llDrag(222, 257, 245, 135, 14);  // 0x9a6c1564  a second run, to the main path
```

**A caveat on the hashes, since this lane spent time on it.** They are
reproducible *given the same IDBFS state and the same build* — the walk above
was replayed from a wiped IndexedDB on the committed build and the eight
front-end values came back identical. But several front-end screens carry LIVE
ANIMATION: the advisor panel plays an AVI (`RenderAdvisorIcon`, screens3.c) and
the flashing icons blink on a wall clock, so a hash taken 300 ms later on those
screens differs, and any hash taken on a screen with a *bubble* up depends on
where the cursor was. The values that hold session to session are the ones on
still screens — `0x87948170` (tutorial select), `0xe59fe2c4`, `0x08b70513`
(briefing p2) repeated across three separate sessions here. Treat the rest as
"this is what it was", and treat `llPark()` as the assertion.

The figures below are from the first pass, when the profile already existed;
the replay from a virgin IDBFS reproduced every `llPark()` value in it
(1000 -> 1030 -> 990, people 3, visitorLimit 3) and every fps within a tenth.

| screen | what is on it | `llPark()` | fps | dead / traps |
| --- | --- | --- | --- | --- |
| PLAYER DETAILS | the clipboard, `adam` in slot 1, seven EMPTY | — | 32.8 | null / 0 |
| tutorial select | the five tutorial markers | — | 33.2 | null / 0 |
| briefing p1 | the notepad, Jonathan, the flashing arrow | — | 34.0 | null / 0 |
| briefing p2 | "Click the Thumbs Up button to play" | — | 32.5 | null / 0 |
| **the park** | terrain, path network, the entrance and its LEGOLAND banners, money bar, rating bar, the six-button toolbar, the mini-notepad | money **1000**, people 3, visitorLimit 3 | 34.0 | null / 0 |
| LEGOLAND menu | the side panel: Toy Shop 30, Space Tower Ride 40 with its new-object gold star | money 1030 | 33.9 | null / 0 |
| "Your First Ride!" | the notepad again, three paragraphs | money **990** (the ride cost 40) | 32.6 | null / 0 |
| the park, built | the Space Tower on its pad, two runs of new path | people **4**, visitorLimit **4** | 35.0 | null / 0 |
| OPTIONS (screen 5) | three volume rows, Save, Load, Return to game, Exit | — | 35.2 | null / 0 |
| SAVE GAME (screen 4) | "Save Game / adam", eight slots | — | 34.6 | null / 0 |
| LOAD GAME (screen 4) | slot 1 = `tutorial1` | — | 33.0 | null / 0 |

**The money is the proof that the game is playing and not just drawing.** It is
1000 on arrival, 1030 after the park has run a while (the tutorial level's
bricks accrue), 990 the instant the ride is placed — the Space Tower's price is
40 and the side panel says so. `llPark()` reads `g_bricks` (sweep1.c:51), which
is exactly what `RenderMoneyBar` (money.c:154) prints.

**The visitor cap moves with the park**: `g_visitor_limit` (blitmisc.c:127) is 3
on arrival and 4 once the ride is standing, and the live `g_people_head` chain
(blokeai.c:164) tracks it. `g_num_visitors` stays 0 — see §4, PARK-1.

Screenshots taken with `computer screenshot` over the page at a 700 px
viewport; `llSnapshot()` and `llCrop(x,y,w,h,scale)` give the same frames as
data URLs for anything that needs to be looked at closely.

### Save, reload the page, load it back

Through the game's own UI, with no page trickery:

```js
await llClick(357, 443);           // the toolbar's OPTIONS button
await llClick(516, 222);           // SAVE  (screens2.c:1175, sprite at 0x1df,0xbd)
await llClick(221, 155);           // slot 1 -> the name editor opens
await llType('tutorial1');
await llClick( 99, 155);           // the slot's own tick
await llClick(287, 127);           // the popup's green OK   -> slot 1 = "tutorial1"
// reload the page completely, then:
await llClick(260, 188); await llClick(505, 345);   // profile adam -> the title screen
await llClick( 85, 362);           // "Saved games" -> LOAD GAME, slot 1 = tutorial1
await llClick(221, 155);           // select the slot (SaveSlotInput just lights it)
await llClick(564, 362);           // the load screen's Accept
```

* The game wrote `/gamedata/profiles/1save1.sav` (**153 637 bytes**) and
  `1save1.sh` (272 bytes), beside the `Profile1.txt` from the earlier session.
* **Both survived a full page reload** — the IDBFS mount PORT-B9 landed covers
  saved games, not only profiles. The LOAD GAME screen listed `tutorial1` on
  the fresh page, which means the 272-byte header was read back correctly too.
* Loading it restored **the park as it was**: the Space Tower on its pad, both
  runs of hand-laid path, the entrance, money 1010, `gameMode` 3. No trap.
* One thing to know for the next lane: on the LOAD screen a click on the slot
  only *selects* it — `SaveSlotInput` (screens3.c:1776) in load mode does
  nothing but `g_cur_save_slot = p->slot`, and the double-click shortcut the
  progress screen has is not wired here. The load is the screen's own Accept
  icon at game (564, 362). A minute was lost to that; it is not a defect.

---

## 2. What was broken, and is now fixed (shim side)

### B10-1 — the Duty Manager's briefing was clipped mid-word

**Symptom.** Every line of the tutorial letter stopped dead at game x = 470:
*"Hi, I'm Jonathan, the Duty Manager. Sorry your firs"*, *"hasn't gone
according to plan. Believe me, anything"*. The last word of every line was
cut in half and the rest of it was gone.

**Cause, and why 470 is the number.** `LoadHelpTextFor` (movie.c:481) reads
`Intervals\<key>` through `LoadTextFileLines` — as LINES, into `g_rep_lines`.
`PrintScreenMode7` (mapscreen4.c:311) prints them one at a time through
`PrintReportLine` (uimisc2.c:472), which builds a rect
`left = x, right = x + 0x1cc` and draws **DT_SINGLELINE**. mapscreen4.c passes
`x = 0xa`, so the box is exactly 10..470, and the text does not wrap: it is
*already wrapped, in the shipped data*. The longest line in the tutorial
letter, *"forgot, we'll practice building paths, too. There's no point in"*, is
63 characters. PORT-B2's face advanced every character by `gw + 1 + bold` — 9
pixels for this font — so 63 characters measured 567 and a fifth of every line
was outside the rect.

(The font is the eighteen-pixel one, not the twenty: PORT-B2's header table
mapped `SelectFont`'s ids to the four LOGFONTs one place out. screen.c:1866-69
names the handles by address and text.c:67-70 names the same four addresses;
font 1 is `g_font_20`, font 2 is `g_font_18`, font 3 is `g_font_28`, the
default is `g_font_24`. Corrected in the header.)

**Fix.** The face is proportional. `face_build` measures each glyph's ink
extent off the art; `glyph_advance` scales that into the ink box and adds one
column of side bearing; `draw_glyph` draws the ink **at the pen** rather than at
the left edge of a six-column box, so the advance means what it says (without
that second half a full stop, which inks columns 2 and 3 of 6, would be drawn a
third of a cell right of where its four pixels of advance put it and would sit
on the letter after it). 69 of the 95 glyphs still use all six columns and
still cost the full `gw + 1`; over real English the mean is 4.87 of 6 columns.
The 63-character line now measures **433** against the 460 the game allows.

A bold weight no longer buys a pixel of advance. Bold here is the glyph smeared
one pixel right, and at a 7-pixel ink box one extra pixel per character is 14%
of the line — most of what broke the report. The smear still draws; it lands in
the side bearing.

**Evidence.** Before: `"Hi, I'm Jonathan, the Duty Manager. Sorry your firs|"`.
After: every line of both briefing pages and of "Your First Ride!" complete
inside the notepad, including the 63-character one.

### B10-2 — the money readout printed its zeroes as something that read as "A"

**Symptom.** The park's money bar showed `1A3A`, `1A4A`, `1A9A`, `124A`. The
first and third glyphs were digits; the zeroes were not legible as zeroes.

**Cause — two compounding.** `RenderMoneyBar` (money.c:154) does
`sprintf(text, "%5d", bricks)` and hands it to the cached-text blitter with a
box **`g->h` tall** — the coin-bar sprite's own height, about 20 pixels — while
the font it asks for (`f1 = 0`) is the lfHeight **24** one.
`DrawCachedTextSprite` (bubblecache.c:421) clips the draw to that box. PORT-B2
made the ink box 5/6 of the cell and centred it, which put a 20-pixel ink box
inside a 24-pixel cell starting 2 pixels down — running past the bottom of a
20-pixel box, so every '0' lost the curve that closes it. With the slashed-zero
art (`XX.XXX / XXXXXX / XXX.XX` in the middle rows), what was left was an apex,
two sides and a crossbar: a capital A.

**Fix.** The ink box is two thirds of the cell, with the slack biased above it
as internal leading (`(line_h - box_h) / 3`, so a caller that gives the text
less room than the font's cell loses descender before it loses a capital), and
the '0' is a plain oval. Digits appear in money and price contexts with no
letters beside them, so an unslashed zero costs nothing.

**Evidence.** `llAscii(228,2,80,24,110,true)` over the money bar now renders
`.XXXX. / XX..XX x5 / .XXXX.` three times after the "1": the park reads
**1000**, and 1010 and 1020 as it earns.

---

## 3. The driver needed one more verb

**The path tool cannot be driven with clicks.** A click places ONE square;
a run of path is a drag. The tutorial's second objective is a run of eight or
ten, so `llClick` alone cannot exercise the run logic at all — and this lane
lost twenty minutes to "path building is broken" before finding it was
"path building is a drag".

`window.llDrag(x0, y0, x1, y1, steps)` presses, moves and releases in game
pixels. The intermediate moves are real `mousemove`s at ~90 ms, which is three
frames at the 28 ms floor: the game samples DirectInput once a frame, so a drag
delivered as two events is a drag it never saw move.

Two more hooks, both because **the game draws all of its own text and numbers**
and a driver that can only diff frame hashes can prove the frame changed but
never that it is right:

* `llPark()` — money, live people, visitor count, sim frame, game/screen mode,
  out of the game's globals (five new `LL_DBG_TABLE` entries; the `Bloke` chain
  is walked with a cycle guard). "Did the ride cost 40" is not a question a
  hash can answer; with this it is 1030 before the click and 990 after.
* `llAscii(x,y,w,h,thr,light)` — a rectangle of the canvas as ASCII ink, and
  `llCrop(x,y,w,h,scale)` for a magnified PNG of one. Twenty rows of llAscii is
  how B10-2 was caught and how it was proved fixed.

---

## 4. Performance: the port is not the limit

**34.67 fps in the park with the ride standing and the full HUD**, measured over
20 s (694 frames, **28.85 ms per frame**), `dead` null, `traps: []`.

That is not a number to improve. `FlipPrimary` (sysmisc.c:569) enforces the
game's own frame floor with

```c
while (timeGetTime() - g_flip_time < 0x1c) ;
```

0x1c = 28 ms, so the game's own ceiling is **35.71 fps** and the port is at
**97.1%** of it. The whole frame — the software terrain pass, every sprite,
the GDI text, the present blit and the entire host shim — fits inside the 28 ms
the game spends waiting, with 0.85 ms to spare. The renderer, the present path
and PORT-A7-3's 1-byte reads are all off the critical path; there is nothing a
Chrome profile would find worth moving, because moving it would not produce a
frame any sooner.

Where the 0.85 ms goes is the spin's own granularity. `timeGetTime` is the
port's yield point (winmm.c:74) and yields at most every 4 ms, so the 28 ms
floor is about seven ASYNCIFY yields and — with `?beat=` on — about 21 600
traced host calls a second, ~630 a frame. Turning the beat off removes the
tracing but not the yields; the floor is still 28 ms either way.

**Recommendation: do nothing.** Revisit only if a later lane makes a frame
take longer than 28 ms of real work — a park full of visitors and a coaster
running is the case to re-measure, and this lane could not get visitors in
(PARK-1 below).

---

## 5. Open, game-side — with owners

| # | what | evidence | owner |
| --- | --- | --- | --- |
| **PARK-1** | **`LINK "SPACE TOWER RIDE"` never satisfies, so no visitors arrive.** The tutorial's objective 1 is `NEED "SPACE TOWER RIDE", 1` and is met (the ride is built, the "Your First Ride!" page fires). Objective 2 is `[PERMANENT]` + `LINK "SPACE TOWER RIDE"`, and its PROMPT — *"Click the PATH button and then link the Space Tower Ride to the main path."* — never goes away, however much path is laid, including runs that visibly join the ride's pad to the main path network. The game's own briefing page says *"we also built a path from the ride to the park entrance"*, i.e. the link is supposed to be there before the player lays anything. `g_num_visitors` (0x00832bd0) stays 0 for the whole session while `g_visitor_limit` correctly rises 3 -> 4. | The script text is in `Legoland.res`, `Scripts\` bucket, at `NEED`/`LINK` around the `OBJLIST1_07/08` cues. Reproduce with §1's script and then watch `llPark().numVisitors` over any length of soak. 20 minutes of park time, 0 visitors. | **PORT-M / a game-side lane.** The `LINK` objective is evaluated by the path/route reachability code (`goalstate.c`, `mappath.c`, `pathmisc.c`). Two candidate shapes, both of this wave's known classes: a route search walking a structure whose declaration is too small (B9-1) or a callback slot in the path/route class whose recovered signature disagrees with its call site (M7/M8). Worth running `name_trap.py` against it only if it traps — it does not; it silently answers "not linked", which is the quieter failure PORT-M9 §1 describes for data addresses. |
| **PARK-2** | **A bubble drawn over the bottom panel is never erased.** With the cursor at game (577, 419) — inside the interface panel — the game raises the map's "Outside your park" bubble help over the mini-notepad and the advisor photo. When the bubble goes, the part of it that lay over the panel stays on screen for the rest of the session: a region hash of (400,400)-(640,480) goes `0x9aaf4dcd` -> `0xe0bcff1c` (bubble up) -> `0x0bba02cc` and **never returns**, through four seconds of frames, a toolbar click and an open/close of the LEGOLAND menu. The map area, which is redrawn every frame, is clean. | `rhash(400,400,240,80)` before/during/after, in §1's park. Repeats every time. | **Undecided — needs the original to settle, then a game-side lane.** `FlipPrimary` blits ONE persistent back surface to the primary (it is not a flip chain), so anything drawn over a region the game does not redraw persists in the original too; this may be faithful. What is worth checking first is the *cause*: the cursor was over the PANEL, not the map, and the game still computed a map square for it and raised a map bubble. If `g_input.map_x/map_y` are meant to be gated on the cursor being inside the map viewport (mapscreen4.c's in-game click handling), the bubble should never have been raised and the residue is a symptom, not the bug. |
| **PARK-3** | **The Space Tower renders as a squat block, not a tower.** The side-panel icon and the new-object popup both show a tall tower with a ball on top; on the map the placed ride is a cluster of white and red bricks about 2 tiles across and no taller than the park entrance. No trap, no missing sprite, and the object is otherwise fully functional (it costs 40, it raises the visitor cap, it saves and loads). | §1's park after `llClick(330,200)`; compare the menu icon at game (27,156) with the map object. | **Unowned — needs a render-side lane.** Could be the ride's 3D model not being drawn above its base (person3d/tri3d path), could be an animation state the ride only leaves once it is linked and operating (see PARK-1, on which this may depend). Do PARK-1 first. |
| **PARK-4** | **`gamedata/Lego.TTF` is right there, 77 012 bytes.** The shim's face is this lane's own ASCII art because "Lego" could not be rasterised — but the game ships the TrueType file, and the portable build already mounts it at `/gamedata/Lego.TTF`. A ~700-line public-domain rasteriser in the host shim would give the game its ACTUAL metrics, and every box the game sizes from `DrawTextA` would land where it landed on Windows instead of within a few percent. §2's two defects are both "our metrics are not the game's metrics"; this closes the class rather than another instance of it. | `FS.stat('/gamedata/Lego.TTF')` on the running page. | **PORT-B, a future lane.** Not this one: it is a bigger thing than the tutorial needed, and the proportional face gets the shipped text inside the shipped boxes today. |

**A7-2 is closed and was re-confirmed**: the MAP toolbar button, which killed
the module for PORT-B9 and PORT-M9 with `function signature mismatch` in
`MakeSprite`, is not reachable as a trap on this build. No trap of any kind
appeared this session — `llStats().traps` was `[]` and `dead` was `null` from
the first frame to the last, across roughly 25 000 presented frames, so
`name_trap.py --at` had nothing to name.

**Sound is silent by design and nothing asks otherwise.** The game calls its
sample system throughout the walk — `PlayInstanceOfSample(g_snd_click, 0, 1, 0)`
on every icon that accepts a click (screens3.c:1765, :2192 and the rest of the
Input handlers), `LoadMoneySFX`/`KillMoneySFX` around the money bar
(audiomisc.c:186/194), and the script cues name a `.WAV` per line
(`OBJLIST1_07.WAV` and friends, in the script text itself). All of it goes to
PORT-B4's silent `IDirectSound` with a wall-clock play cursor and none of it
blocks: no wait in the walk ever failed to return. MIDI is
`MMSYSERR_NOTSUPPORTED` and `InitMIDIManager` ignores the result (winmm.c's
header). Nothing here needs doing until there is an audio backend to do it to.

---

## 6. Gates

| gate | result |
| --- | --- |
| `emcmake cmake -S portable -B portable/build-wasm` + `ninja` (clean dir) | exit 0 |
| `ninja ... legoland_headless legoland_headless_debug legoland_tests legoland_pathtest legoland_shimtest legoland_browser legoland_browser_named` | exit 0 |
| wasm `ctest` | **17/17 passed** |
| native `cmake -S portable -B portable/build` + `ninja` + `ninja legoland_tests` + `ctest` | **11/11 passed** |
| the page, `?args=-nointro+WINDEBUG&beat=1000` | ~25 000 frames, `dead` null, `traps: []` |

One note for whoever runs the native gate next: the brief's
`cmake -S portable -B portable/build -G Ninja && ninja -C portable/build` does
NOT build `legoland_tests` (it is not in the default target set on the native
config the way it is on the wasm one), so `ctest` straight after it reports ten
tests "Not Run" and `pointer_words` failed — which looks alarming and is not.
`ninja -C portable/build legoland_tests` first, then `ctest`: 11/11.

## 7. Files touched

| file | change |
| --- | --- |
| `portable/src/hostwin/ll_font.c` | proportional advance from each glyph's ink extent; the ink drawn at the pen; ink box 2/3 of the cell with the leading biased above it; the '0' unslashed; the SelectFont id table corrected |
| `portable/hostwin/include/ll_host.h` | `LLFontMetrics.advance` is now the NOMINAL advance; `ll_font_text_width` is the measure |
| `portable/src/browser/main.c` | five `LL_DBG_TABLE` entries: `g_bricks`, `g_bricks_full`, `g_num_visitors`, `g_people_head`, `g_visitor_limit` |
| `portable/src/browser/index.html` | `llDrag`, `llPark`, `llAscii`, `llCrop`, and the header's hook list |
| `portable/README.md` | this lane's section |
| `docs/SCOPE_PORT_WAVE.md`, `docs/lanes/scope-port-b10.md` | status line, these notes |

No `LEGOLAND/*.c`, no `docs/HANDOFF.md`, no `portable/CMakeLists.txt`,
no `portable/tools/*`, no `kernel32.c`/`msvcrt.c`, no `tests.cmake`,
no `headless/**`.
