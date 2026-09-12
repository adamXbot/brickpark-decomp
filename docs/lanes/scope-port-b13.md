# Scope PORT-B13 — the three black text boxes, and the two harness findings

> **PORT-B13 — Status: DONE (2026-09-12)** — branch `scope/PORT-B13`, cut from
> `origin/main` `63ee1432` (the PORT-P4 merge). Files touched:
> `portable/src/hostwin/{gdi32,user32,ddraw}.c`,
> `portable/hostwin/include/ll_host.h` (declarations only),
> `portable/src/browser/{index.html,main.c}`,
> `portable/src/browser/replays/b13-01-*.js`, `portable/README.md`,
> `docs/SCOPE_PORT_WAVE.md`, these notes. **`LEGOLAND/*.c` is READ-ONLY for
> this lane and nothing in it is changed on this branch**, so the VC6 gates
> have nothing to check. Brief: `docs/SCOPE_PORT_WAVE.md`.

**Result in one line: P4-3 is not `SetBkMode` and not `ll_font.c`. It is a GDI
OBJECT-TABLE LEAK in `gdi32.c` — `DeleteDC` released a memory DC's attributes
and not the object slot `CreateCompatibleDC` took, so every tooltip the game
drew leaked one of 256 slots; once the table was full `obj_new` handed back a
handle with no slot in it, `ll_host_brush_colour` fell back to BLACK, and the
cached-text sprites' fill went black underneath a colour key that was still the
INK the game asked for. The key missed the fill, the sprite blitted opaque, and
the text printed on top of it — which is exactly the money readout, the
objective bubble and the info pop-up body, all three at once, for the rest of
the session.**

| finding | verdict | root cause | fix |
| --- | --- | --- | --- |
| **P4-3** three text boxes fill solid black | **REAL — root cause proved, fixed, A/B on one build** | `portable/src/hostwin/gdi32.c` `DeleteDC` leaked the object slot; the table was a fixed 256 and degraded silently when full | `DeleteDC` frees the slot; the table grows to the handle format's 4095 (§1) |
| P4-3's stated suspicion (`SetBkMode(TRANSPARENT)` ignored) | **REFUTED** | `llGdi().setBkMode` is `{transparent: 4344, opaque: 0}` across a whole lesson — every call is honoured | — |
| **P4-4** the canvas collapses, `llPoint` maps every pixel to one spot | **REAL — fixed** | `max-width: 100%` let the flex line shrink the canvas; `MouseEvent.clientX` is a `long`, so the driver's fractions were truncated | the canvas keeps its intrinsic size and wears an outline, so the rect is exactly 640x480 (§2) |
| **P4-5** `?awake=1` wakes the game, not the driver | **REAL — fixed** | `llSleep` set real timers, which a hidden tab clamps to ~1 Hz | `llSleep` yields through the keep-awake MessageChannel hop (§3) |

Everything below was measured on **one build**, in the **lesson-5 park**,
`portable/build-wasm` served on 8893, `legoland.html?args=-nointro+WINDEBUG&awake=1`,
with the same script either side. The "before" column is a throwaway build with
**one `#ifdef`** backing out the `DeleteDC` slot release and capping the table
at its old 256 — nothing else differs.

---

## 1. P4-3 — the fill and the colour key have to be the same colour

### 1.1 What the game is actually doing

The three boxes are not three bugs. All three are **cached-text sprites**:

| box | drawn by | ink / paper |
| --- | --- | --- |
| the money readout | `LEGOLAND/money.c:157` `PrintCachedText(text, …, 200, h, 0, 0, 0xff0000, 0xffffff)` | ink `0xff0000`, paper white |
| the objective help bubble | `LEGOLAND/fpui2.c:1005` `FindCachedText(text, font, 0x10, 0x96c6da, 0)` | ink `0x96c6da`, paper black |
| the info pop-up's body | `LEGOLAND/popup.c:1405` `PrintCachedText(info, …, 2, 0x10, 0xff0000, 0xffffff)` | ink `0xff0000`, paper white |

`PrintCachedText` (bubblecache.c:275) looks the tuple up, rasterises it if it
misses, and blits the sprite. The rasteriser is a FUNCTION SPRITE whose painter
is `DrawCachedTextSprite` (bubblecache.c:419, `0x00455a50`), and its body is the
whole of this finding:

```c
nearest = GetNearestColor(s, e->ink & 0xffffff);
SetBkMode(s, 1);                       /* TRANSPARENT */
SetBkColor(s, nearest);
brush = CreateSolidBrush(nearest);
FillRect(s, &rc, brush);               /* the WHOLE cell, in the ink colour */
DeleteObject(brush);
SetTextColor(s, e->paper & 0xffffff);
…
DrawTextA(s, e->text, strlen(e->text), &rc, e->format);
…
key = GetNearestColour(nearest & 0xff, (nearest >> 8) & 0xff, (nearest >> 16) & 0xff);
ck.high = key; ck.low = key;
e->sprite->surface->vtbl->SetColorKey(e->sprite->surface, 8, &ck);   /* line 459 */
```

**The fill is meant to be invisible.** The game fills the cell with the ink
colour and then makes that same colour the sprite's COLOUR KEY, so
`RenderSprite` (gpu.c:`0x00488a10`, `flags & 0x40` → `DDBLT_KEYSRC`) drops every
filled pixel and blits only the text. The naming is inverted from the intuition:
`ink` is the colour that vanishes and `paper` is the colour the letters are
printed in.

So a solid black box under legible text is not "an opaque background". It is
**the fill and the key being two different colours**, and only two numbers can
tell you which of them moved.

### 1.2 The two numbers, and the chain that separates them

`llGdi()` (new, §4) reports both. Driven to the lesson-5 park and left alone,
they agree exactly — `fillLast.colorref 0x96c6da`, `fillLast.c565 0xde32`,
`ck.lastLow 0xde32`. Then:

1. **`DeleteDC` leaked the object slot.** In this shim a memory DC is two
   things — an entry in the DC-attribute table (fg/bg/`bk_mode`/selections) and
   a slot in the OBJECT table that `obj_new` handed `CreateCompatibleDC` — and
   `DeleteDC` released only the first. So every *balanced*
   `CreateCompatibleDC`/`DeleteDC` pair the game makes still leaked a slot, and
   the game makes one per tooltip: `fpui2.c:1009/1016` (`HTBubbleHelp`),
   `bighelp.c:321/328`, `bubblecache.c:505/512`.
2. **The table was a fixed 256 and failed silently.** With every slot taken,
   `obj_new` returned `0x4c47_5000` — the class with **no slot** — rather than
   failing, and `obj_get` rejects a slot of 0.
3. **So `ll_host_brush_colour` fell back to BLACK**, and `FillRect` wrote
   `0x0000` across the whole cell.
4. **The key was still the ink.** `GetNearestColour` is the GAME's own function
   (sweep1.c:241) and never went near the object table, so it kept returning
   `0x001f` for the money readout's `0xff0000`. Key `0x001f`, fill `0x0000`: the
   key matches nothing, `DDBLT_KEYSRC` drops nothing, the cell blits opaque.
5. **The text survives** because it is drawn after the fill, in `paper`. White
   digits on a black box, which is precisely what PORT-P4 described.

It is persistent and survives a MAP round trip because the table never recovers,
and it arrives *later in the session* because a cached sprite that was painted
before exhaustion stays correct — the black arrives with the next cache MISS.
That is why the money readout goes black on the next value it shows and not the
moment the table fills.

### 1.3 The A/B — 281 hovers is all it takes

Identical script both sides: cold load → lesson 5 → `:COLDHARDCASH` (which
changes the money TEXT, so `PrintCachedText` misses its cache and rasterises a
fresh sprite) → hover the five toolbar buttons in rotation → `:COLDHARDCASH`
again → read the three rectangles. Each hover is one tooltip measure, i.e. one
`CreateCompatibleDC`/`DeleteDC` pair.

| | **before** | **after** |
| --- | --- | --- |
| hovers to the stop condition | **298** (table full) | 320, the cap — never fills |
| `llGdi().objs.live` / `byClass.dc` | **255 / 251** | **4 / 0** (the four LOGFONTs, nothing else) |
| `objs.exhausted` | **1**, then 2, then 8 | **0** |
| `fill.noBrush` (fills on an unresolved brush) | **1**, then 7 | **0** of 285 fills |
| **A** money readout, x 220..430 y 8..30 | **51.7% pure black** | **0.0%** |
| **B** help bubble, x 420..630 y 318..398 | **86.6% pure black** | **1.8%** (79.7% `dedfd6`, its own paper) |
| **C** panel caption box, x 3..208 y 357..383 | **70.8% pure black** | **4.4%** (82.7% `dec794`, its own paper) |
| `fillLast.colorref` / `fillLast.c565` | **`0x0` / `0x0`** | `0x96c6da` / **`0xde32`** |
| `ck.lastLow` (the key the game set) | `0x1f` — **a different colour** | **`0xde32`** — the same one |
| `setBkMode` | `{transparent: 4317, opaque: 0}` | `{transparent: 4347, opaque: 0}` |
| `blt.keysrcUnset` | 0 of 18,349 | 0 of 19,883 |
| traps / fps | 0 / 35.2 | 0 / 35.7 |

Two details worth keeping:

* **`objs.byClass.dc` climbs one per hover and never comes down** — 11, 27, 43,
  59, 75, 91, 107, 123, 139, 155, 171, 187, 203, 219, 236 sampled every twentieth
  hover before the fix; flat at 0 after it. That straight line IS the defect.
* **B and C were still clean at the moment the table filled** and went black
  only on their next rasterise — B when the tooltip changed, C when the object
  panel was opened. A cached sprite painted before exhaustion stays correct
  forever; the black arrives with the next cache MISS. That is why PORT-P4 saw
  the money box go black "later in the session" and not at the moment of
  failure, and why a MAP round trip does not clear it: the round trip redraws
  from the same poisoned sprites.

### 1.4 The fix

Two changes, both in `portable/src/hostwin/gdi32.c`:

* **`DeleteDC` frees the object slot** it was given. That is the defect and the
  whole of the A/B above.
* **The object table grows** — 256 to start, doubling, capped at 4095 because
  the handle format (`0x4c47 | class << 12 | slot`) has a 12-bit slot field —
  and `obj_new` traces loudly when it is ever genuinely exhausted. This is not
  belt-and-braces for the leak above; it is cover for the game's OWN leak:
  `misc3.c:1054` records it in the recovery notes —

  > ORIGINAL BUG: the memory DC from `CreateCompatibleDC` is never released —
  > neither the old font is selected back nor is `DeleteDC` called […] Every
  > pop-up resize leaks a DC.

  `MeasurePopUpTitle` and `MeasurePopUpBody` each leak one, and Windows absorbs
  it because a process gets ~10,000 GDI handles. 4095 slots is ~2,000 pop-up
  opens, which no session reaches; and if one ever did,
  `llGdi().objs.exhausted` says so instead of turning text boxes black.

Nothing in `LEGOLAND/*.c` changed, and nothing about `SetBkMode`, `SetBkColor`,
`ExtTextOut`, `ll_font.c` or the colour-key blit needed to.

---

## 2. P4-4 — the canvas is never scaled again

`llPoint` maps a game pixel through `r.left + gx * r.width / 640` and
`ll_canvas.js` inverts exactly that, so the arithmetic was never wrong. What was
wrong is that **`MouseEvent.clientX` is a `long`**: whatever fraction `llPoint`
computes is truncated on the way into the event, so the smaller the canvas the
more game columns land on one client pixel. Measured, before, in a 312 px pane
(`canvas { max-width: 100% }`, rect 174x131, scale 0.272):

| game x sent | clientX delivered | game x the page's own inverse reads back |
| --- | --- | --- |
| 320 | 101 | 320.00 |
| 321 | 101 | 320.00 |
| 322 | 101 | 320.00 |
| 323 | 101 | 320.00 |
| 324 | 102 | 323.68 |
| 325 | 102 | 323.68 |
| 326 | 102 | 323.68 |
| 327 | 102 | 323.68 |

Eight distinct columns, **two** distinct answers. PORT-P4's collapsed canvas
(rect width 2) is the same thing at its limit: all 640 columns, one answer.

The fix is in the page's CSS: `flex: none` so the flex line cannot shrink it,
`min-width/min-height: min-content` so the floor is the canvas's OWN intrinsic
size (and therefore follows a mode change rather than hard-coding 640x480), and
the frame moved from a `border` to an `outline` so the border box equals the
content box. `getBoundingClientRect()` is then exactly 640x480 and `llPoint` is
the identity — not 642/640 as it was even at full size.

| pane | before: rect / scale | after: rect / scale | after: 320..327 round-trip |
| --- | --- | --- | --- |
| 1100x900 | 642x482 / 1.0031 | **640x480 / 1.0000** | 320, 321, 322, 323, 324, 325, 326, 327 |
| smallest the pane allows (654 / 312 px) | 174x131 / 0.2719 | **640x480 / 1.0000** | 320, 321, 322, 323, 324, 325, 326, 327 |

A narrow pane now scrolls and `#log` wraps underneath instead of the one surface
the driver measures in being squashed. `llPoint` also throws, naming the
measured rect, if the canvas is ever below half size — P4-4 cost PORT-P4 three
page loads precisely because nothing said why the walk had diverged.

---

## 3. P4-5 — the driver waits on the same hop the game does

`?awake=1` (PORT-A10) delivers **0-or-1 ms** timers as MessageChannel messages
while the document is hidden, which is every yield the game's ASYNCIFY main loop
makes. `llMove`/`llClick`/`llDrag`/`llKey` wait 60–600 ms, so they went to real
timers, and a hidden tab clamps those to ~1 Hz. The game ran at its ceiling
while the driver crawled.

`llSleep` now yields through that same hop while it is engaged — a zero-delay
`setTimeout` is exactly the shape the hop intercepts — until the wall clock says
the wait is up. One FIFO queue shared with the game's own yields, so the game
keeps presenting while the driver waits. When the hop is not engaged (visible
page, or `?awake=0`) it is one real timer, as before. It is exported as
`window.llSleep` so a replay no longer has to write its own pump, and
`llAwake().waits` counts them.

Measured hidden, one build, with the clamp installed explicitly (Chrome's
documented hidden-tab behaviour: a real timer longer than 1 ms does not fire
before the next 1 s tick — the hop's own 0/1 ms timers are left alone, so the
GAME is awake in both columns):

| | before (`setTimeout`) | after (the hop) |
| --- | --- | --- |
| one `llClick` | **3024 ms** | **818 ms** |
| one `llType` of 20 characters | **48101 ms** | **5645 ms** |
| fps during | 35.7 | 35.7 |

PORT-P4 measured 3.2 s and 0.81 s for the click on their own pane; both
reproduce to the tens of milliseconds.

**One honest correction to P4-5.** The ambient clamp did **not** engage in this
lane's Browser pane: hidden, unclamped, eight consecutive `llSleep(150)`s
measured 152/156/152/156/152/156/152/156 ms and `llClick` cost 824 ms before the
fix and 811 ms after. So the hop is not a speed-up in every pane — it is what
makes the wait **independent of whether the clamp is biting**, which is the
property a replay needs, and the clamped column above is what it buys when it
is. The fix costs nothing when it is not: 5605 ms vs 5641 ms for `llType(20)`
unclamped.

---

## 4. `llGdi()` — the probe, and what it is for

New page hook, reads only, backed by `LLGdiStats` in `ll_host.h` and filled by
`gdi32.c`, `user32.c` and `ddraw.c`; `Module._ll_gdi_stats()` hands the page its
address and the struct carries its own word count so the page can check the
layout it is decoding. The object census is walked in `ll_gdi_stats()` (main.c)
and **not** in `ll_host_gdi_stats()`, because the drawing paths call the latter
once per `FillRect`, per `DrawTextA` and per `Blt` — 19,883 keyed blits in one
drain — and must not walk a 4,096-entry table to bump a counter.

```
llGdi() -> {
  objs:      { live, high, exhausted, capacity, byClass: {font, dc, brush, rgn, …} },
  dcEvictions,
  setBkMode: { transparent, opaque },
  textOut, drawText,
  fill:      { calls, noTarget, noBrush },
  fillLast:  { brush, colorref, c565, rect, surface },
  ck:        { sets, lastLow },
  blt:       { keysrc, keysrcUnset, plain }
}
```

The point of it is §1.1: a black box on the canvas is two different bugs that
look identical, and `fillLast.c565` against `ck.lastLow` is what separates them.
`setBkMode` is what refuted P4's first guess in one reading. `objs.exhausted`
and `fill.noBrush` are what named the cause. `blt.keysrcUnset` is the other half
— a `DDBLT_KEYSRC` blit on a source with no key set would be the same symptom
from the opposite direction, and it has stayed 0 throughout (28,173 keyed blits
in the drain alone).

---

## 5. What this lane did NOT do

* **`LEGOLAND/*.c` is untouched.** `misc3.c`'s two leaked DCs are the original's
  own behaviour and stay; the shim now survives them rather than the game being
  corrected. The one-line `DeleteDC(dc)` each of them wants is written out here
  for whoever owns a `LEGOLAND_PORTABLE` arm, and it is NOT applied:
  `MeasurePopUpTitle` (misc3.c:1059, `0x00471840`) and `MeasurePopUpBody`
  (misc3.c:1101, `0x004717a0`) would each need a `DeleteDC(dc)` before the
  `return`, which changes instruction counts in two matched bodies for no
  port-side benefit now that the table grows.
* **P4-1 and P4-2 are not touched** — they are a render lane's and an
  object-list lane's, not the page's.
* The `dcEvictions` counter shows the DC-ATTRIBUTE table (8 entries, LRU) really
  does recycle — 12 evictions over a lesson. Nothing depends on it today
  (`GetDC`/`ReleaseDC` bracket every use, single-threaded), but it is now
  visible if something ever does.
