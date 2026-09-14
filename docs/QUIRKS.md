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
| Q2 | `popup2.c:48` `DrawPopUpExtra` | ~~footer corner tiles swapped~~ **not a defect**: the `.lls` NAMES are swapped, not the paint — `[8]` is the 0xbc-wide left piece, `[6]` the narrow right one; painting "by name" leaves the footer's middle unpainted (PORT-Q1 measured it) | none | — |
| Q3 | `misc3.c:1019` pop-up placement | when the pop-up's y is kept, the function still **returns the clamp limit, not y** — callers that use the return value position a second element off the pop-up | return `g_popup_y`, portable arm; needs a check of the two callers | low |
| Q4 | `misc3.c:1054` `MeasurePopUpTitle`/`MeasurePopUpBody` | **every pop-up resize leaks a GDI memory DC** (PORT-B13 made the shim survive it; the game still leaks) | `DeleteDC(dc)` before each return, portable arm | none (shim already tolerant) |
| Q5 | `savegame.c` BLK4 (PORT-M18 §3) | **every load adds `visitorLimit` ghost visitors** on top of the restored chain — the visitor counter is not in the save and is never restored | one tally + one store in a portable arm; changes what a `.sav` round trip produces (policy rule 2) | medium |
| Q6 | `musicthread.c:610` | the **Egypt → Inca music transition plays the Inca → Egypt file** (slot 7 pastes the `ietran2` path) | one string in a portable arm | low |
| Q7 | **fixed and measured** (PORT-Q1; integrator 2026-09-14) | Lesson 5 PLACEs a Boating School at load, which runs `BoatingSchool_Add`. Default build: the counter seeds 0 and the direction flag flips to 1 at 100 — the animation is **one-shot**, not a loop. Faithful build: seeds 9999 (11575 at sim 2352), flag 0, no flip over 357 frames, so the frame-setting call is never reached. On-screen frame change not captured: the map layer is dirty-redrawn, so a canvas diff over the building read 0 even after re-arming the counter |
| Q8 | `ridemisc.c:231` favourite-object pick | the random "pick the Nth match" **always returns the first match**, and one time in 32 returns **stack junk** as a visitor's favourite | advance the cursor in the accept arm; initialise `e` | low–medium (visitor AI behaviour changes) |
| Q9 | `uimisc2.c:338` `EnqueueObjectHelp` | inserting a help event with priority ≥ the head's **discards the whole existing queue** | link in front instead of assigning the head, portable arm | medium (more help bubbles appear than the shipped game shows) |
| Q10 | `ridecb1.c:523` | two ride customers reaching state 0 **in the same tick share one free-seat computation** — the second gets the first one's answer | move the two locals into the rider loop, portable arm | low |
| Q11 | **fixed and measured** (PORT-Q4; integrator 2026-09-14, game level 6) | Two Jungle Cruises on one park; a second station's dispatch check triggered by the same rider poke in both builds. Default: the first station's boat kept its own key and the second station launched its own boat at once. Faithful: within 100 ms BOTH of the first station's boats were re-keyed to the second station, and the second station could not launch (riders waiting, timer -105) because the shared count of 2 made `take 12 <= 6 * 2` — one busy cruise freezes every other cruise in the park |
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

## Found while reaching the game levels (2026-09-14)

Not a quirk — a port defect, fixed in both builds. `catapult.c` declared the Catapult's FX table as one `void*`, so the portable closure gave it 8 bytes where `Load_FXList` walks 48, and `g_catapult_sample` (entry 0's +0x08) became a separate block. Loading the Catapult class trapped in sprintf, which blocked **game levels 2, 4, 6, 7, 9 and 10** on Accept. The table is now declared `FXEntry[4]`; `screencb5.c`'s local `FXEntry` was corrected from 8 to 12 bytes for the same reason (the entrance FX table, whose sample `ridecb7.c` plays as `g_entrance_pay_sample`). VC6 audit rows identical; relocs 0 MISMATCH.

Also a port defect, fixed in both builds (2026-09-15, found playing game level 1): **leaving visitors walked to the top edge of the map before heading for the gate.** `goalstate.c` declared `g_entrance_x` (0x004b8320) as a lone `int` and `BlokeAction_LeavePark` reads it as a `Pos`; `mapobj.c` and `savegame.c` write the y half as `g_entrance_y` (0x004b8324). The closure emitted two 4-byte objects 16 bytes apart, so the y read alignment padding: every leaving visitor's target was (65, 0) instead of the gate at (65, 53). The path-square route to that spot failed eight times (`stuck` at Bloke +0x82), the tile flood fill then walked the visitor off the paths to row 0, and only there did it join the gate's queue and walk ~52 tiles straight back. Measured on the unfixed build, game level 1 steady state: two departures took 132.5 s and 149.7 s, both reaching y 0.1. Slots stayed full that long, so fewer visitors arrived to pay the 15-coin fee. `g_entrance_x` is now declared `Pos`; the closure emits one 8-byte block with `g_entrance_y` at +4 (ctest `keystate` checks it). VC6 audit 14/14 OK, relocs 0 MISMATCH. The sweep for other `(Struct*)&g_scalar` casts found none live.

## Status

| # | status | commit / evidence |
| --- | --- | --- |
| Q1 | fixed (PORT-Q1) | caption centred in the free span; A/B by crop of the Delete strip: the caption sits centred in the free span instead of overhanging the tick |
| Q2 | not a defect | see row; reverted after measuring |
| Q3 | left, no visible effect | the only caller discards the return value |
| Q4 | fixed (PORT-Q1) | `llGdi().objs.live` fixed 4→4 vs faithful 4→6 |
| Q5 | **fixed (PORT-Q2)** | save at 28 visitors, reload, load: fixed settles at people 30 / count 30 / ghosts 0 (sim 2042→2757); faithful 60 / 30 / ghosts 30. **A `.sav` round trip now differs from the shipped game** (policy rule 2): the visitor counter is restored to the number of blokes in the save |
| Q6 | left, no asset | the port ships no `.sgt` music at all |
| Q7 | fixed (PORT-Q1), **unmeasurable in free play** (PORT-P8) | the free-play template offers 24 classes and no Boating School (same list on all four theme tabs); it lives in the GAME levels. Needs a game-level unlock |
| Q8 | **fixed and measured** (PORT-Q4 / PORT-P7 / PORT-P8) | largest cluster on one class 7/4/11 of 30 fixed vs **12/16/24** faithful, and **0** visitors left with no favourite vs the shipped 1/4/0. P7's unexplained 14/30 was the browser's cached module — see `scope-port-p8.md` |
| Q9 | fixed (PORT-Q4), **not observable** (PORT-P8) | `g_object_help` never held an entry in 8,783 samples at 60 Hz of lesson-2 play on the FAITHFUL build: the advisor pops each event within a frame, so the shipped discard needs two enqueues in one frame. The fix is a two-line list insertion, correct by construction |
| Q11 | fixed (PORT-Q4), **unmeasurable in free play** (PORT-P8) | no Jungle Cruise in the free-play template either; debug rows for `g_jc_boats`/`g_jc_stations` are now in place for whoever reaches a game level |
| Q14 | fixed (PORT-Q4), **unmeasurable in free play** (PORT-P8) | no roller coaster or log flume in the free-play template |
| Q10 | fixed (PORT-Q2), by inspection | `nfree`/`freeidx` reset at the top of case 0, per rider |
| Q12 | fixed (PORT-Q2), by inspection | the second call counts `g_bs_mermaid_cls` |
| Q13 | fixed (PORT-Q2), by inspection | `v[1]` (top), as the x uses `v[0]` and every other flume origin in the file is `(v[0], v[1])` |
| Q15 | fixed (PORT-Q2), by inspection | the else-arm is the other three theme buttons' (screens3.c:810) verbatim |
| Q16 | fixed (PORT-Q2), by inspection | `g_bz_layers->flags |= 0x2000` — the sprite's +0x10, as `OctopusCafe_Create` does for the same flag |
| Q17 | fixed (PORT-Q2), by inspection | the second copy's arm paints at `(px + halfw, py + halfh)` like every other draw in that copy |
| Q18 | fixed (PORT-Q2) | corner 6 emitted; free-play figures stand where they stood (compared by crop, fixed vs faithful) — no visible regression |
| Q19 | fixed (PORT-Q2), by inspection | cleared on a successful drop; `RenderWorkerOnMouse` is behind `g_drag_lock` (gameframe.c:718) and rin.c:576 only skips the selected bloke while `g_selection_lock` |
| Q20 | fixed (PORT-Q2) | `default:` returns (0, 0); callers only pass cardinal headings |
| Q21 | fixed (PORT-Q2), by inspection | the mirror disjunct, as the other seven elbows |
| Q22 | fixed (PORT-Q2), by inspection | `i--` after the removal |
| G1 `gameframe.c:1231`, G2 `screencb.c` (two cursor calcs) | guarded (PORT-Q1) | by construction |
| class B | guarded (PORT-Q3): 24 files, every listed site except `ridecb2.c:951` (a WIP body — needs a matching pass first) plus the twelve record-unlink walks of one shape; the two neighbour-lookup families left as the source proves them non-null; free-play spot check owed | `docs/lanes/scope-port-q3.md` |

Policy agreed 2026-09-13; `LL_FAITHFUL` CMake option on `legoland_core`. Notes: `docs/lanes/scope-port-q1.md`, `scope-port-q2.md`, `scope-port-q3.md`, `scope-port-q4.md`. Every class-A row is now decided. PORT-P7 and PORT-P8 finished the measurements: Q5 and Q8 are measured against the faithful build, Q9 is proved not observable, and Q7/Q11/Q14 cannot be reached because the free-play template ships none of those rides — they need a GAME-LEVEL unlock, which is the one thing still owed (`scope-port-p7.md`, `scope-port-p8.md`). The class-B road-edge write is unreachable by placement: the cursor refuses every map-edge cell.
