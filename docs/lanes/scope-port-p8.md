# PORT-P8 — the owed quirk A/Bs, finished (2026-09-13, done by the integrator)

PORT-P7 left four rows owed and one open question. This pass closes the open
question, settles Q9 and Q7/Q11/Q14 with measurements rather than guesses, and
adds the debug-table rows the next lane will want. Both builds rebuilt at
PORT-P7's tree plus seven `LL_DBG_TABLE` rows, served on **fresh ports** (8952
default, 8953 faithful) — the port change is what resolved P7's open question.

## Q8 — the open question was the browser's module cache

P7 measured the fixed build twice and saw 14 of 30 visitors with no ride
favourite, which it could not explain and recorded as possibly meaning the fix
was wrong. It was not: the page had been reloaded at the same URL, so the
browser served the **cached `legoland.wasm`** from before the wrap-sentinel
refinement. Re-measured on a fresh port (a new origin, no cache):

| build | largest cluster on one class (ride1 / ride2 / food) | visitors with NO favourite |
| --- | --- | --- |
| **fixed** (sim 9733) | **7 / 4 / 11** of 30 | **0 / 0 / 0** |
| **faithful** (sim 16199) | **12 / 16 / 24** of 30 | 1 / 4 / 0 |

Distinct classes chosen: fixed 12 / 15 / 5, faithful 11 / 8 / 3. No junk
pointers on either build in these runs.

**Verdict: the fix is right and P7's worry is retired.** The shipped picker puts
16 of 30 visitors on one second-favourite ride and 24 of 30 on one food; the
fixed picker's worst cluster is 11 (food, where the list is genuinely short) and
it leaves nobody without a favourite. The `start = i` refinement that P7 added
blind is confirmed by this run: without it a wrapped scan returned 0, which is
exactly the 14/30 the cached module was still showing.

**Lesson for every future A/B: serve the rebuilt module on a NEW PORT** (or a
cache-busting query). Reloading the same URL can silently measure the old build.

## Q9 — the help queue never holds an entry, on either build

`g_object_help` added to the debug table, then sampled at **60 Hz** (a
`setInterval(…, 16)` reading the head and walking the chain) for **8,783
samples, about 2.4 minutes** of lesson-2 play on the **faithful** build, which
is where the defect lives.

| | head non-null, any sample | longest chain seen |
| --- | --- | --- |
| faithful, lesson 2, 8,783 samples | **0** | **0** |
| default, free play | 0 | 0 |

`KillObjectHelp` (fpui5.c:229) pops and frees the head as soon as the advisor
displays it, and it runs from the frame loop, so an event queued by
`ShowScriptStepText` is gone again within a frame or two. The shipped discard
needs **two enqueues before one display**, which neither the tutorial-2 script
nor free play produces.

**Verdict: the fix stands as correct by construction (a two-line list
insertion), and the defect is NOT observable in tutorial 2 or free play.** A
lane wanting to see it needs a script that fires two texts in one frame, or a
hook that counts enqueues against displays.

## Q7, Q11, Q14 — the free-play park has none of those rides

P7 suspected the panel; it is the park. `llClasses()` returns the same **24**
classes with **every** theme tab open (LEGOLAND / Wild West / Castle /
Adventurers), so it is the park's whole class list, not the visible page:

> BRICKOLA Kiosk, Balloonz Ride, Copters Ride, Dragon BBQ, Driving School,
> Driving School Road, Earth Slide Ride, Flowers, Food Kiosk, Greenhouse,
> Hedge, Ice-cream Kiosk, LEGO Media Shop, LEGO Restaurant, LEGO Toy Shop,
> Park Entrance, Path, Pine Tree, Saloon, Shark Café, Small Power Station,
> Space Tower Ride, Temple Slide, Visitor Crossing

No Boating School (**Q7**), no Jungle Cruise (**Q11**), no roller coaster and no
log flume (**Q14**). The rides exist in the GAME levels, not the free-play
template: `gamedata/disc/Legoland.res` carries `game level one.txt` …
`game level nine.txt` and an `INTERVAL "boating school.txt"`.

**What the next lane needs:** a way into a game level. PORT-P1's free-play poke
is the model — `g_cur_profile.level_done[5]` unlocks Free Play — so the
equivalent for the game levels (and their own map) is the unlock to find. Until
then Q7, Q11 and Q14 are unmeasurable in the browser, and all three fixes stand
on source inspection, which is recorded as such in `docs/QUIRKS.md`.

## New `LL_DBG_TABLE` rows (145–151)

`g_object_help`, `g_jc_boats`, `g_jc_stations`, `g_jc_water`, `g_bs_stations`,
`g_bs_water`, `g_road_list` — the list heads Q9 and Q11 read. Indices stay
contiguous (the page walks until a name comes back 0) and no name is duplicated.

## Gates

| gate | result |
| --- | --- |
| `LEGOLAND/*.c` touched | **none** (page only) — no VC6 gate owed, no verify owed |
| wasm default build + ctest | 28/28 |
| wasm faithful build + ctest | 28/28 |
| traps across both sessions | `[]`, `dead` null |
