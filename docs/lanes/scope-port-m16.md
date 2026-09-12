# Scope PORT-M16 — P2-2: the LEGOLAND theme button lost for good

> **PORT-M16 — Status: IN PROGRESS (claimed 2026-09-12 by PORT-M16)** — branch
> `scope/PORT-M16`, cut from the PORT-P2 merge (`6cd9c232`). A matching-side
> lane (VC6-gated) with licence to edit `portable/src/hostwin/*.c` if the cause
> turned out to be host timing. It did not.
> Brief: `docs/SCOPE_PORT_WAVE.md`; the defect is PORT-P2's **P2-2**
> (`docs/lanes/scope-port-p2.md` §3). Files: this note, `LEGOLAND/fpui2.c`
> (one rename), `portable/src/browser/replays/m16-theme-button.js`.

**Result in one line: P2-2 is not a race, and it has nothing to do with the
side panel's animation, MAP mode, or the spacing between clicks. It is THREE
CLICKS at any frame rate, and it is `g_popup` — the name PORT-P2 itself filed
as P2-5 row 7 and PORT-P3 as P3-2. `bighelp.c:456` and `popup.c:661` give
`g_popup` the 376-byte `PopUpUI` at `0x007fdea4`; `fpui2.c:1443` gives the same
name the request block at `0x007fdec0`, which is that struct's own `+0x1c`.
One namespace keeps one address, so every store in `PopUpInfoSetUp` landed
`0x1c` low — `type` onto `g_info_icon_g` and `obj` onto `g_info_icon_d`, two of
the ten pop-up ICON POINTERS that `DisableInfoPopUPIcons` dereferences with
`flags |= 0x400` every frame. Touch the LEGOLAND button, click bare ground, and
the button's own `Icon*` is parked in `g_info_icon_d` and hidden for the rest
of the level. The ORIGINAL cannot do this. One rename fixes it, and fixes P3-2
(no gardener or mechanic could be hired) with it.**

---

## 1. First, the measurement environment — because it invalidates the "race"

PORT-P2 filed P2-2 as a race because it reproduced at 1300 ms between clicks
and not at 1500. That number is an artefact of the tab it was measured in.

`emscripten_sleep` is `setTimeout`, and Chrome clamps a **hidden** tab's chained
timers, so the game does not run at 33 fps in a background pane — it runs at
about one frame per second or worse. Measured on this build, same page, same
pane, `document.hidden === true` throughout:

| timers | frames in 2 s | effective fps |
| --- | --- | --- |
| stock `setTimeout` (what every lane has been using) | 1 | **0.5** |
| `M16.setFloor(1000)` (the clamp, made explicit) | 1 in 3 s | **0.33** |
| `M16.realtime()` — `setTimeout` re-implemented on a `MessageChannel` chain | 66 | **35.8** |

`setTimeout(r, 1300)` also returned after **1503 ms** under the clamp, so the
"1300 vs 1500" axis was not even the axis it was labelled. PORT-P2's own §2
caveat records 0.29 fps in a soak and then reports 32-35 fps "taken live while
driving"; both are true at different moments, which is exactly what makes a
clamped tab a bad instrument for a wall-clock question.

`M16.realtime()` in `portable/src/browser/replays/m16-theme-button.js` is the
fix for the instrument: `MessageChannel` messages are tasks, not timers, and
Chrome does not clamp them. It gives 35 fps and accurate sleeps in a hidden
pane, and **every measurement below was taken with it installed**. Any lane
that cares about wall clock should install it first; any lane that does not
should not quote frame rates.

With it installed, PORT-P2's own `P2.themeButtonRace()` was run unchanged in
Lesson 2 at 1000, 1100, 1200, 1300, 1400, 1500 and 1600 ms, and the button
survived every time. That is the first half of the verdict: **the race does not
exist.** The second half took a fuzzer.

---

## 2. What actually happens, frame by frame

### 2a. Finding the state without a `LL_DBG_TABLE` entry

`main.c` belongs to PORT-A10 this round, so nothing here is named. Everything
below is read out of the running heap by signature, which turns out to be
cheaper than adding names anyway:

* **the four theme icons** — an `Icon` record is 0x40 bytes with `group` at
  `+0x14`; scan for `group == 0x9a` with `y == 379` at `+0x0e` and a plausible
  input slot at `+0x2c` (a wasm function pointer is a small table index, so
  anything above 40000 is not one). Four hits, x = 8 / 105 / 202 / 299, input
  slots 136..139 — `Legoland`/`Western`/`Castle`/`AdventureThemeInput`.
* **`g_theme_icon[4]`** — the one place in memory holding those four addresses
  in a row (2071520 on this build).
* **`g_panel_state`** — the only 16 bytes matching `{2, 1, 0, 0x86}`, the tuple
  `InitGameInterface` writes at `:1067`.
* **`g_menu_index` / `g_theme_closed[4]`** — a word `5` with `1,1,1,1` exactly
  176 bytes later (`0x004baff8` and `0x004bb094`, and the 176 is the three
  16-aligned gen blocks between them).
* **`g_side_icons`** — walk `next` back from any icon to the head, then find
  the one word in memory that points at it.

A general x86-VA → wasm-address model was tried (parse `gen/globals.c`, split
`.data` from `.bss` by "has an initialiser", 16-byte stride in declaration
order) and **does not hold**: fitting it against the 84 live addresses
`llAddrs()` already gives produced no dominant base (best 8 votes of 84). The
signature scan is the reliable way; the note is here so the next lane does not
spend the hour.

### 2b. The record that goes bad, and the write that does it

The symptom is exact, and it is **not** a lost `Icon`:

| | LEGOLAND | Western | Castle | Adventurers |
| --- | --- | --- | --- | --- |
| healthy | `0x6012` | `0x6412` | `0x6412` | `0x6412` |
| after | **`0x6412`** | `0x6412` | `0x6412` | `0x6412` |

`0x400` is the DISABLED bit. The `Icon` record itself is untouched —
`g_theme_icon[0..3]` still hold the same four addresses, the records are still
linked into `g_side_icons`, their sprites, x, y and input slots are unchanged.
Writing `flags &= ~0x400` by hand brings the button, its bubble help and the
panel straight back, which is what makes "four empty pills" the whole of it:
the pill's mean colour goes `[165,168,195]` → `[128,142,209]`, the same value
the three profile-hidden pills read.

Three measurements pin the writer:

1. **A canary.** Set all four flag words to `0x800012 | visible` and wait. Only
   icon 0 comes back hidden, and the canary bit `0x800000` SURVIVES — so it is
   an `|= 0x400` on one icon, not a wholesale write. That eliminates
   `UpdateThemeIconsFromProfile` and `UpdateThemeIconsFromFlags`, which write
   all four from a table (and with the profile at `[1,0,0,0]` would have
   re-hidden icons 1-3, which stayed visible).
2. **The period.** Clear the bit and it is back in **1 frame** (15-28 ms), with
   no input at all. A per-frame pass, not a click handler and not the 200 ms
   script tick.
3. **The pointer.** Scan the low 4 MB for words equal to the LEGOLAND `Icon*`:
   three holders — `g_theme_icon[0]`, `g_focussed_icon`, and **one more**.
   Zero that third word and the flag **stays clear**; restore it and it is
   re-set within 23 ms. That word is `0x007fdea8`.

`0x007fdea8` is `g_info_icon_d` (`iconui.c:91`) / `g_cb_icon_ok`, and the block
it sits in reads, at the moment of failure:

```
+0x00  0x00000109   <- an INT, where g_info_icon_g / g_pu_icon_mech live
+0x04  <Icon* of the LEGOLAND theme button>   <- g_info_icon_d / g_cb_icon_ok
+0x08  0
+0x0c  300
+0x10  120          <- the click point
```

which is `{type, obj, ref, pos.x, pos.y}` — the first five members of
`fpui2.c`'s `PopUpInfo`, written at the **block base** instead of at `+0x1c`.

### 2c. The chain, end to end

```
gameframe.c:1258   HandleMapClick, on a click with no edit mode and no icon hit:
                       PopUpInfoSetUp(g_hit_info, pt.x, pt.y)
gameframe.c:1025   the bare-ground branch sets g_hit_info.type = 0x109 and does
                   NOT re-assign g_hit_info.obj -- which, because the 12-byte hit
                   record is shared with the ICON hit test (PORT-A6 §7's
                   gameframe.c pair), is still the last Icon the mouse touched
fpui2.c:1530-32    g_popup.type = type; g_popup.obj = obj; g_popup.ref = ref;
                   -- bound to 0x007fdea4, so 0x1c LOW
iconui.c:233       DisableInfoPopUPIcons: g_info_icon_d->flags |= 0x400, every
                   frame, on whatever pointer is in that slot
```

Live proof of the hit record's part, taken at the moment of failure:
`llSel().hit` = `{type: 0x109, obj: <the LEGOLAND Icon>, cell: 0}`.

### 2d. Why the original is fine

In the shipped binary `fpui2.c`'s block really is at `0x007fdec0`. The same
stores land in the request's own `type`/`obj`/`ref`/`pos`; `type == 0x109`
matches no `case` in `PopUpInfoSetUp`'s switch, so the function falls straight
through to `ResetInfoStruct()`; and nothing anywhere dereferences
`g_popup.obj`. A stale `Icon*` sitting in a data word is inert. The ten pop-up
icon pointers at `0x007fdea4`, `0x007fdea8`, `0x007fdfc0` … are written only by
`Load_PopUpInfo` and are never anything but pop-up icons.

**Verdict: a port defect, not a shipped bug.** Nothing to preserve, no
portable-only guard needed — the collision is in a name we invented.

---

## 3. The same 0x1c also closes PORT-P3's P3-2

`PopUpInfoSetUp`'s `0x103` arm compares the clicked class's element against
`g_popup.elem_shed` (`+0xf0`) and `g_popup.elem_hut` (`+0xf4`) to decide whether
the click hires a gardener or a mechanic. Read at the block base those two
words are not the elements. Measured in the same park, same frame:

| read at | `elem_shed` | `elem_hut` | `elem_path` | `elem_entr` |
| --- | --- | --- | --- | --- |
| base `+0xf0` (before) | 0 | 0 | 0 | 2 |
| base `+0x10c` (after) | 13629936 | 13630236 | 13628176 | 13628516 |

So `ce == e` and `ce == g_popup.elem_hut` could never match and
`GenerateGardener` / `GenerateMechanic` were unreachable from the pop-up —
which is exactly PORT-P3's **P3-2, "no worker can ever be hired"**. The same
rename fixes it. (Lesson 3's `NEEDGARDENERS` objectives, which are the natural
end-to-end test, are still behind P2-1's scroll clamp and could not be played
from here.)

---

## 4. The fix

`LEGOLAND/fpui2.c`, one declaration and its 23 uses inside `PopUpInfoSetUp`:

```c
extern PopUpInfo g_popup;          ->   extern PopUpInfo g_popup_request;
```

with a block comment at the declaration recording the collision. **Both
builds** — the name is ours, not the shipped binary's, so this is a pure
identifier change; no byte moves, no `LEGOLAND_PORTABLE` arm, no codegen lever
touched. `gen_link` now emits

```
__asm__(".globl g_popup_request\n.set g_popup_request, g_info_icon_g+28\n");
```

i.e. an interior alias at `+0x1c` of the merged `0x007fdea4` block — exactly
`0x007fdec0` — while `g_popup` stays at `0x007fdea4` for `bighelp.c` and
`popup.c`, which is what they always meant.

**For the integrator:** PORT-M15 is sweeping the same 17-name class (P2-5 /
P3-3) and `fpui2.c`, `bighelp.c` and `popup.c` are on its file list, so it may
well rename this pair too. The edit here is deliberately the minimum — one
declaration, its uses inside one function, and a comment. Take whichever lands
first; they are the same change.

---

## 5. Proof

Same build, same park (Lesson 2, virgin profile `m16`, 35 fps with
`M16.realtime()` installed), with and without the single rename.

### 5a. The three-click repro (`M16.repro()`)

`llClick(53,394)` — `llClick(53,394)` — `llClick(300,120)`, 1200 ms apart.
(300,120) must be bare ground: `llSel().hit.typeHex` reads `0x109` there. A
cell carrying an object gives `0x103`, `HandleMapClick` re-assigns `obj`, and
the stale icon never reaches the pop-up — which is why the defect looked
position- and timing-dependent.

| | without the rename | with the rename |
| --- | --- | --- |
| flags after | **`6412,6412,6412,6412`** | `6012,6412,6412,6412` |
| LEGOLAND pill mean RGB | **`[128,142,209]`** (empty) | `[165,168,195]` |
| `llSel().hit` at the click | `{0x109, <LEGOLAND Icon>, 0}` | `{0x1, 0, 0}` |
| block `+0x00` / `+0x04` | `265` / `<LEGOLAND Icon>` | `20194256` / `13714688`, both group `0x2c3` pop-up icons |
| block `+0x1c` … `+0x2c` | untouched | the request's own words; on a `0x103` click they read `{0x103, <MapObj 13630336>, 5924, 399, 217}`, on the repro's `0x109` click they are zero because `PopUpInfoSetUp` falls through to `ResetInfoStruct()` |
| `dead` / `traps` | null / `[]` | null / `[]` |

### 5b. Permanence, without the rename (`M16.dead()`)

Three further clicks on the button at 1.8 s: `editState` 0 every time and the
frame hash identical (`0x89a8f750`, `0x89a8f750`, `0xf352c560` — the third
differs only by the park's own animation). Two MAP round trips: flags still
`6412,6412,6412,6412`. `dead` null, `traps` `[]` throughout — it is a dead
button, not a crash, exactly as PORT-P2 described.

### 5c. PORT-P2's own replay across the window (`M16.gapSweep()`)

`P2.themeButtonRace(gap)` unchanged, with the rename, in Lesson 2:

| gap (ms) | 1000 | 1100 | 1200 | 1300 | 1400 | 1500 | 1600 |
| --- | --- | --- | --- | --- | --- | --- | --- |
| theme flags after | `6012` | `6012` | `6012` | `6012` | `6012` | `6012` | `6012` |
| panel state `f00/f04/f08/f0c` | `3/0/1/3` | `3/0/1/3` | `3/0/1/3` | `3/0/1/3` | `3/0/1/3` | `3/0/1/3` | `3/0/1/3` |
| `g_menu_index` | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
| `editState` at "armed" | 1 | 1 | 1 | 1 | 1 | 1 | 1 |
| `dead` | null | null | null | null | null | null | null |
| fps | 35.7 | 35.7 | 35.6 | 35.7 | 35.5 | 35.7 | 35.7 |

The panel reopens (`f00 = 3`, `f08 = 1`, menu 0) and an object arms
(`editState 1`) after every one of them. For completeness the same replay was
also run WITHOUT the rename, before the repro was isolated — Lesson 2 at 1000,
1100, 1200, 1300 (twice), 1400 and 1500 ms, and Lesson 1 at 1300 ms (twice,
once with a per-frame recorder attached that dragged the page down to 13 fps
and once at 32 fps) — and **the button survived all of them**. The race
PORT-P2 named genuinely does not reproduce at a real frame rate in either
lesson on either build, which is why a fuzzer was needed to find the real
trigger. Three slower runs at simulated clamp floors of 150, 400 and 1000 ms
did not reproduce it either; they simply stopped delivering the clicks (at a
1000 ms floor the whole six-click replay spanned **two** game frames).

### 5d. Fuzz (`M16.fuzz(3, seed)`)

Randomised sequences of 4-7 clicks drawn from the theme row, the panel, the
five tool buttons, the MAP button and the map itself, with gaps of 100-1700 ms,
and a `MessageChannel`-rate watch on the LEGOLAND flag word.

| build | seed 7 | seed 11 | seed 23 |
| --- | --- | --- | --- |
| without the rename | **HIT on iteration 2** (`6012` → `6412`) | — | — |
| with the rename | 0 hits / 5694 polls | 0 hits / 5476 polls | 0 hits / 6769 polls |

`dead` null and `traps` `[]` in all of them.

### 5e. Lesson 1, played end to end on the way

Lesson 1 was completed in this session to unlock Lesson 2: the Space Tower
built (money 1070 → 1030), `LINK "LEGO SHOP 1"` satisfied by a three-square
path drag, the shop erased (+30), the congratulations screen and the progress
screen with Lesson 2 lit. Zero traps, `dead` null. Building works.

---

## 6. Gate table

| gate | result |
| --- | --- |
| `$PY tools/audit.py LEGOLAND/fpui2.c` | 14 rows, all `[OK]`, `mismatch=0`, `PASS: 0 function(s) failed` — rows identical to base (`PopUpInfoSetUp` 192i/664B = orig) |
| `$PY tools/relocs.py LEGOLAND/fpui2.c \| grep MISMATCH` | empty (14 functions checked) |
| `$PY tools/relocs.py --all \| grep MISMATCH` | empty tree-wide |
| `$PY tools/progress.py --check` | `3281 exact functions total; 42 WIP` — report regenerated and committed (line numbers moved) |
| `portable/tools/extern_sweep.py` | `0 multi-address extern statement(s) — the class is closed` |
| `portable/tools/bvstruct_sweep.py` | `0 unaccepted silent site(s), 1 silent, 5 noisy` (unchanged) |
| `tools/port_m10_bvstruct_sweep.py` | `SLOT vs BODY … 0 site(s)` |
| wasm build + 7 extra targets + `ctest` | **24/24 PASS** |
| native build + `legoland_tests` `legoland_cbtypes` + `ctest` | **17/17 PASS** |
| page, Lesson 2, 35 fps | `dead` null, `traps` `[]` across every run above |

`tools/verify.py` was not run (integrator's gate).

---

## 7. What this lane did NOT do

* **P2-1 is untouched.** The `g_view_*` scroll clamp still makes Lesson 2
  unwinnable, so the flowers cannot be planted and Lessons 3-5 are still
  unreachable. That is PORT-M15's row 1-4.
* **The other 13 rows of the P2-5 / P3-3 class are untouched** — only
  `g_popup` was renamed, because only `g_popup` was on the path to P2-2.
  `g_avi_open_count` (`movie.c:232`) is still the one PORT-P2 said to check
  first.
* **P3-2 is fixed but not played.** The hire path's four element pointers are
  demonstrably correct now (§3), but the objectives that exercise it —
  Lesson 3's `NEEDGARDENERS`, and the mechanic's hut in the main game — are
  behind P2-1.
* **`M16.realtime()` is a page-console helper, not a shim change.** Nothing in
  `portable/src/hostwin/**` was touched: the throttling is the browser's
  policy for hidden tabs and is not something the port should work around in
  C. It belongs in the replay so measurements are honest.
