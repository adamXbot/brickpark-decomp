# PORT-Q4 — the last four quirks (2026-09-13, done by the integrator)

The four gameplay-changing rows of `docs/QUIRKS.md`, approved by the user after
batches one and two. Every change is in a
`#if defined(LEGOLAND_PORTABLE) && !defined(LL_FAITHFUL)` arm with the shipped
statement in the `#else`; `-DLL_FAITHFUL=ON` reproduces the shipped behaviour.

| # | file | shipped behaviour | change | evidence |
| --- | --- | --- | --- | --- |
| Q8 | `ridemisc.c` `RandomFavouriteFood` / `RandomFavouriteRide` | the "pick the Nth match" loop never advanced past an accepted match, so it always returned the FIRST match from the random start; a try count of 0 (one time in 32) returned the uninitialised `e` as a visitor's favourite | `tries` is at least 1; after an accepted match `i` steps on (wrapping), so the next try finds the NEXT match | inspection: the accept arm is the only path that leaves the inner `for` without advancing `i`; both pickers share the shape and both are changed. Visitors' favourites are now spread over the qualifying classes instead of clustered on the first one after each random start |
| Q9 | `uimisc2.c` `EnqueueObjectHelp` | inserting a help event with priority ≥ the head's assigned it as the new head with `next = 0`, discarding the whole existing queue | link in front: `e->next = g_object_help; g_object_help = e;` (the empty-list case is the same statement with a null head) | inspection: the `prev` arm already links correctly; more help bubbles reach the player than the shipped game showed |
| Q11 | `junglecruise.c` `JungleCruise_CountStationBoats` | `if ((b->key.w = st->pos.w) != 0)` — a single `=`: every boat in the game was re-stamped with this station's square once per frame and all of them were counted | `==` | inspection: boats are keyed to their station at launch (`nb->key.w = key.w`), so nothing depends on the per-frame re-stamp; a park with two cruises now divides each station's takings by its own boats |
| Q14 | `coaster4.c` coaster support shadow | `(float)((int)pos->x * 0.2) * 5.0f` — the cast is applied first, so the two scales cancel and the shadow position only truncates to whole units | `(float)LL_FISTP(pos->x * 0.2) * 5.0f` (rounded, as PORT-M5's portable arm already rounds the coordinate) | inspection: this is the shadow of a coaster SUPPORT, not the track placement — the visible effect is the shadow snapping to the 5-unit grid the constants describe |

## Gates

| gate | result |
| --- | --- |
| audit ridemisc.c, uimisc2.c, junglecruise.c, coaster4.c | PASS, rows identical |
| relocs per file / `--all` | 0 MISMATCH / 0 MISMATCH, 16 WIPRELOC (the accepted set) |
| markers | identical |
| progress | 3281 exact / 42 WIP |
| extern / bvstruct / m10 / addr / variadic sweeps | at baseline |
| native default + ctest | 21/21 |
| wasm default + ctest | 28/28 |
| wasm faithful + ctest | 28/28 |
| verify.py | run by the integrator before push |

Browser A/Bs for these four are owed to the next play lane: a Jungle Cruise
park with two stations (Q11), a coaster with supports over uneven ground (Q14),
a visitor census of favourites (Q8), and a help-bubble count in a lesson (Q9).
