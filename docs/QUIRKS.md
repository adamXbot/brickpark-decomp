# Shipped-game quirks — the candidate list for port-side fixes (2026-09-13)

The decompilation reproduces `LEGOLAND.EXE` byte for byte, bugs included. The
sources carry **137 `ORIGINAL BUG` annotations** and the port-wave lane notes
add a handful more found by playing. None of them may be fixed in the VC6 text
(the bytes would move). All of them *can* be fixed in the portable build, in a
`#ifdef LEGOLAND_PORTABLE` arm, which VC6 never sees.

## Proposed policy (to agree before the first fix lands)

1. Every quirk fix lives in a `LEGOLAND_PORTABLE` arm **and** inside
   `#ifndef LL_FAITHFUL` so a faithful portable build (`-DLL_FAITHFUL=ON`) still
   reproduces the shipped behaviour for A/Bs and save-compat oracles.
2. Every fix gets a row in the table below (status, commit), an A/B measured in
   the browser (before/after on one build), and — where it changes what a save
   round trip produces — a note in the save-compat oracle.
3. Crash-class quirks (null dereferences the original gets away with) are
   guarded, not "fixed": the guarded arm returns quietly where the original
   would have faulted. On x86 a read from address 0..0xc faults; on wasm32 it
   reads linear memory 0..0xc and silently returns whatever is there, so these
   are *worse* in the port, not better.
4. Debug-trace-only bugs (a `%d` of a pointer, a format with one argument too
   few) are left alone.

## A. Player-visible — the short list to go through together

| # | where | what the player sees | fix shape | risk |
| --- | --- | --- | --- | --- |
| Q1 | `popup2.c:81` `DrawPopUpExtra` | the pop-up **control bar's caption box is the wrong width**: `n*20 + 0x7a` uses the vertical line pitch where the horizontal cell pitch `0x20` belongs; for a 0-line pop-up the caption overhangs the OK icon by 9 px | one constant in a portable arm (`n*0x20 + ...`), confirm against the strip's own walk | low |
| Q2 | `popup2.c:48` `DrawPopUpExtra` | the pop-up **footer's two corner tiles are swapped** (bottom-right art at the left end, bottom-left at the right) | swap `[8]`/`[6]` in the footer row, portable arm | low |
| Q3 | `misc3.c:1019` pop-up placement | when the pop-up's y is kept, the function still **returns the clamp limit, not y** — callers that use the return value position a second element off the pop-up | return `g_popup_y`, portable arm; needs a check of the two callers | low |
| Q4 | `misc3.c:1054` `MeasurePopUpTitle`/`MeasurePopUpBody` | **every pop-up resize leaks a GDI memory DC** (PORT-B13 made the shim survive it; the game still leaks) | `DeleteDC(dc)` before each return, portable arm | none (shim already tolerant) |
| Q5 | `savegame.c` BLK4 (PORT-M18 §3) | **every load adds `visitorLimit` ghost visitors** on top of the restored chain — the visitor counter is not in the save and is never restored | one tally + one store in a portable arm; changes what a `.sav` round trip produces (policy rule 2) | medium |
| Q6 | `musicthread.c:610` | the **Egypt → Inca music transition plays the Inca → Egypt file** (slot 7 pastes the `ietran2` path) | one string in a portable arm | low |
| Q7 | `ridecb5.c:1079` `BoatingSchool` | a **freshly built Boating School never animates** (counter seeded 9999, only turns round at exactly 100) until a save/load rewrites it | seed or compare fix in a portable arm | low |
| Q8 | `ridemisc.c:231` favourite-object pick | the random "pick the Nth match" **always returns the first match**, and one time in 32 returns **stack junk** as a visitor's favourite | advance the cursor in the accept arm; initialise `e` | low–medium (visitor AI behaviour changes) |
| Q9 | `uimisc2.c:338` `EnqueueObjectHelp` | inserting a help event with priority ≥ the head's **discards the whole existing queue** | link in front instead of assigning the head, portable arm | medium (more help bubbles appear than the shipped game shows) |
| Q10 | `ridecb1.c:523` | two ride customers reaching state 0 **in the same tick share one free-seat computation** — the second gets the first one's answer | move the two locals into the rider loop, portable arm | low |
| Q11 | `junglecruise.c:210` | a **single `=` stamps the station's square onto every boat** instead of counting the station's boats | `==` in a portable arm; needs an A/B on the Jungle Cruise | medium |
| Q12 | `ridecb8.c:324/412` Boating School teardown | the **water class's placed-object count ends two too high and the mermaid's one too low** after tearing down a school — eventually a build limit lies | count the mermaid class in the second call, portable arm | low |
| Q13 | `logflume.c:2390` | the log-flume **cursor's y is built from the footprint's right edge** (x uses the left); should be the top | `v[1]`, portable arm | low |
| Q14 | `coaster4.c:511` | coaster placement **truncates to whole units instead of snapping to the 5-unit grid** (the cast is applied before the scale) | reorder the cast, portable arm | medium (changes where coasters land) |
| Q15 | `screens3.c:914` Adventurers menu | unlike the other three menus it **does not restore the previous menu index** when the test fails | mirror the other three, portable arm | low |
| Q16 | `screencb2.c:675/704` | a draw flag is OR'd into the **ObjDef's flags instead of the build sprite's** — the intended sprite never draws through the slot | point the `|=` at the sprite, portable arm; A/B on the Ball… object | low |
| Q17 | `render4.c:195` | a rare map tile (flags 0x3+0x8, object with no draw callback) is **stamped on its diamond neighbour** | add the half offsets to the second copy, portable arm | low |
| Q18 | `person3d.c:1210` | the model centring box **repeats corner 3 and omits corner 6** — every minifigure is centred slightly off | emit `(bmax.x, bmax.y, bmin.z)`, portable arm | low (pixel shift of every person — measure) |
| Q19 | `workers2.c` (PORT-M21 §7) | `g_worker_on_mouse` is **never cleared by a successful drop**; harmless to the player as far as measured, confuses every probe | clear it in the drop arm — *measure first* that nothing reads it afterwards | low |
| Q20 | `schoolcar7.c:49` | the three **diagonal headings return two uninitialised dwords** (no `default`) | a `default` arm, portable | low |
| Q21 | `logflume2.c:1715` | the N\|W flume arm **tests the same condition twice** — that corner shape is never chosen | the mirror test, portable arm | low |
| Q22 | `fpui5.c:71` | the panel strip's removal scan **skips the entry that slid into the hole** — harmless only because a class appears once | `i--` after a removal, portable arm | none |

## B. Crash class — null dereferences the original gets away with (guard, don't fix)

These read (or write!) through a null cell/record pointer. x86 faults; wasm32
reads linear memory 0..0x10 and carries on with garbage, or corrupts the low
words (PORT-M20 saw emscripten catch one as "corrupted heap memory area
(address zero)"). Each wants `if (!p) return;` in a portable arm.

Reads: `bswater.c:140,449,562`, `bswater2.c:142,378`, `bswater3.c:77`,
`anim2.c:314`, `junglecruise.c:334,632,1185`, `lfentrance.c:361`,
`mappath.c:362`, `objrect.c:653`, `logflume3.c:475`, `logflume.c:1784`,
`gameframe.c:1231` (a click off the map edge), `screencb.c:96` (hovering the
map border), `screencb2.c:942`, `ridecb2.c:716,951`, `ridecb5.c:380`,
`ridecb6.c:1020,1142`, `sysmisc3.c:93`, `texture.c:244`, `unref3.c:221`,
`unref7.c:732`, `unref6.c:368`, `workorder3.c:298,339`.
Writes: `roads.c:105` (a road block laid over the map edge writes through
null), `bswater.c:449` (lake square within two cells of the edge).

## C. Leaks and harmless UB (leave, unless a session ever reaches them)

`movie.c:38,572` PAVIFILE per movie; `resaudio2.c:169` ACM stream per converted
sample; `text.c:136`, `fpui4.c:151` single-node lists never freed;
`savechunks2.c:253`; `schoolcar.c:109` freed before unlink; `screencb.c:662/782`
frees the record after the one unlinked; `screen.c:61`, `texture.c:343` double
free on OOM BMP load ("do not fix" in the source); the uninitialised-read family
(`data3.c:95,199`, `mantex.c:252,312`, `logflume.c:1023,1681,1751`,
`logflume5.c:352`, `lfentrance.c:1015`, `ridecb5.c:933`, `ridecb6.c:760`,
`unref1.c:835`, `uimisc2.c:598`, `schoolcar4.c:90`); `mapscreen4.c:250` (an
all-empty hint table spins forever), `:332` (signed expiry after 2^31 ms);
`ridecb8.c:684` timer off by one; `unref3.c:360,387`, `unref2.c:467`,
`coaster3d.c:797` dead clamp; `sysmisc2.c:258`, `sysmisc.c:432`;
`screencb4.c:63` (masked); `audio5.c:183`; `narration2.c:696`; `render3.c:390`;
`savegame2.c:242`; `unref5.c:589`; `ridecb1.c:1454`.

## D. Debug traces only (leave)

`movie3.c:50`, `workorder3.c:112`.

## Status

| # | status | commit |
| --- | --- | --- |
| — | policy agreed 2026-09-13; `LL_FAITHFUL` CMake option added; first batch Q1–Q4, Q6, Q7 + the two reachable crash guards (gameframe.c:1231, screencb.c:96) briefed to PORT-Q1 | — |
