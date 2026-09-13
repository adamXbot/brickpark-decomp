# PORT-P7 — the owed quirk A/Bs (2026-09-13, done by the integrator)

The play-lane agent stalled at its first step (the eighth harness stall of the
run), so the integrator measured by hand on one default build and one
`-DLL_FAITHFUL=ON` build, both at `origin/main` 5c7f1c12, served on 8950 / 8951,
each from a virgin IDBFS through `P1.start()` (free play).

## Q8 — visitors' favourites (measured)

Census of the 30-bloke chain (`g_people_head`, Bloke +0x88 / +0x8c / +0x94) once
the free-play park had filled to its limit.

| build | blokes | distinct `favourite_ride1` | distinct `favourite_ride2` | distinct `favourite_food` | junk pointers |
| --- | --- | --- | --- | --- | --- |
| fixed | 30 | 11 (10 classes + "none") | 12 | 4 (3 + "none") | 0 |
| faithful | 30 | 11 | 8 | 3 | 0 |

The distinct-count alone hides the finding; the CLUSTER shows it:

| build | largest cluster on one class, ride1 / ride2 / food | blokes with NO favourite, ride1 / ride2 / food |
| --- | --- | --- |
| fixed (first census, sim ~6000) | 3 / 3 / 3 of 30 | 14 / 13 / 25 |
| fixed (second census, sim 12460) | 3 / 3 / 2 of 30 | 14 / 9 / 27 |
| faithful (sim ~8000) | **12 / 16 / 24** of 30 | 1 / 4 / 0 |

On the faithful build the shipped picker piles 12, 16 and 24 of the 30 visitors
onto one class each (the first match after each random start, and the food
list is short), which is the defect. On the fixed build no class holds more
than 3.

**Open question, recorded rather than explained:** the fixed build also shows
14 of 30 visitors with NO ride favourite and 25–27 with no food favourite,
where the faithful build shows 1 and 0. The base fix could produce it (after an
accepted match the scan could reach the original `start` sentinel and return
0), so the integrator refined the arm to move the sentinel to the index after
the accepted match — a full circle then re-finds the match instead of returning
0 — and re-measured on a rebuilt module: still 14. Either the browser served
the cached module (the URL did not change) or the zeros come from elsewhere
(a satisfied favourite being cleared, which the fixed spread makes reachable).
The next lane must load with a cache-busting query, re-census, and read the
picker's return in the debugger. Until then Q8 stands as "fixed, cluster gone,
zero-favourite count unexplained".

## Class-B edge guards — not reachable by placement in this park

Driving School armed, the view scrolled to the west edge (`P4.centerOn(2, 29)`,
cells (0..3, 27..31) all on screen), `P4.placeAtCell` at six edge cells: the
game's own cursor refuses every one (`CursorIsValid` false), on the default
build. So `roads.c:105`'s null write is not reachable through normal placement
on the free-play map; the guard stays "by construction". `llPeek(0, 64)`
unchanged, no traps. A map whose border admits a school would be needed to
drive the faithful arm's write.

## Not measurable in this session, with reasons

| # | why | what the next lane needs |
| --- | --- | --- |
| Q7 Boating School | the free-play template (`FreePlayTest.txt`) offers 24 classes; `llClasses()` lists no Boating School and no Jungle Cruise (only the Driving School and its road) | a park template or lesson that offers the Boating School (its theme tab), then the 300-frame hash sample |
| Q11 Jungle Cruise | as Q7: no Jungle Cruise class in the template; and `g_jc_boats` / `g_jc_stations` are not in `main.c`'s `LL_DBG_TABLE`, so the boat keys cannot be read without adding rows and rebuilding | add the two rows (and `g_object_help`, `g_bs_stations`) to the debug table, rebuild both builds, use a template with the cruise |
| Q9 help queue | `g_object_help` not in the debug table; counting bubbles visually needs a long lesson walk | the debug-table row above; then chain length after a placement burst in Lesson 2 |
| Q14 support shadow | the coaster editor walk (PORT-P2/P3) was not attempted in the time | one coaster with a support over uneven ground, crop fixed vs faithful |

The harness's safety classifier timed out on roughly one call in three during
this session, which is what limited the coverage.
