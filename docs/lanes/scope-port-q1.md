# PORT-Q1 — the first quirk batch (2026-09-13, done by the integrator)

The lane agent stalled at its first step (the fifth harness stall of the
evening), so the integrator did the batch in the integration worktree. Policy:
`docs/QUIRKS.md`. Every change is in a
`#if defined(LEGOLAND_PORTABLE) && !defined(LL_FAITHFUL)` arm; VC6 sees the
original text; `-DLL_FAITHFUL=ON` (new CMake option) restores the shipped
behaviour. The A/B is one default build against one faithful build, same
lesson-1 park, same LEGO Toy Shop pop-up, same click.

## Verdicts

| # | verdict | what changed | A/B |
| --- | --- | --- | --- |
| Q1 | **fixed** | `popup2.c` `DrawPopUpExtra`: the control-bar caption rect's right edge is the OK gadget's x − 3 instead of `left + n*20 + 0x7a` | the "Delete" caption on the expanded pop-up (the delete-confirm strip): shipped centres it in a rect that runs 9 px under the green tick, so the word sits ~4 px right of the free span's centre; fixed centres it in the span left of the tick. Crops `docs/lanes/q1-delete-strip-ab.png` (top fixed, bottom faithful) |
| Q2 | **not a defect — reverted** | (none) | the `.lls` NAMES are what is swapped, not the paint: `g_pu_bg[8]` is the 0xbc-wide LEFT piece (the middle slices start at px + 0xbc) and `[6]` the narrow right piece. Painting them "by name" leaves the footer's middle unpainted (measured: scenery shows through the strip). The shipped footer is correct. QUIRKS.md row corrected |
| Q3 | **left — no visible effect** | (none) | `ClampPopUpToScreen`'s only caller (`popup.c:1386`) discards the return value, so returning `limit` instead of `y` reaches nothing |
| Q4 | **fixed** | `misc3.c` `MeasurePopUpTitle` / `MeasurePopUpBody`: `DeleteDC(dc)` before the return (declaration added in the portable arm) | `llGdi().objs.live`: fixed 4 → 4 after ~30 pop-up open/close cycles; faithful 4 → 6 (`byClass.dc` 2) in the same session. The shim already survives the leak (PORT-B13); the game no longer makes it |
| Q6 | **left — no asset** | (none) | the port's `gamedata/` holds no `.sgt` files at all and no `eitran2` string anywhere (the transition names live only in the exe's own list); the port has no DirectMusic. The one-string fix is moot until music exists |
| Q7 | **fixed, A/B owed** | `ridecb5.c` `BoatingSchool_Add`: `st->frame = 0` instead of `0x270f` (the value a loaded record starts from) | the tick loop (`ridecb5.c:1385-1399`) only turns round at `frame == 100`, so a seed of 9999 counts past `nframes` for ever and `LLSSetFrame` is never called. Lesson 1's theme has no Boating School and the free-play route needs a virgin profile; the browser A/B (build one, 200-frame hash) is owed to the next free-play lane |
| G1 | **guarded** | `gameframe.c:1231`: the off-map removal arm is skipped when `c` is NULL | reached only from the object-hit branch with an off-map selection square; guard by construction (the shipped code dereferences NULL) |
| G2 | **guarded** | `screencb.c`: both cursor calcs (Boating School, Jungle Cruise) return when `MapCellAtRef` is NULL | reached only with one of those classes armed and the cursor off the map; guard by construction. Hovering the map border in lesson 1 (a plain class) answers hit 0x10a with no trap on both builds |

## Gates

| gate | result |
| --- | --- |
| audit popup2.c, misc3.c, ridecb5.c, gameframe.c, screencb.c | PASS, rows identical (misc3's WIP row unchanged) |
| relocs per file | 0 MISMATCH |
| relocs `--all` | 0 MISMATCH, 16 WIPRELOC (M21's accepted set), exit 2 |
| markers | identical |
| progress | 3281 exact / 42 WIP |
| extern / bvstruct / m10 / addr / variadic sweeps | at baseline |
| native default build + ctest | 21/21 |
| wasm default build + ctest | 28/28 |
| wasm faithful build (`-DLL_FAITHFUL=ON`) + ctest | 28/28 |
| verify.py | run by the integrator before push |

## Notes for the next quirk lane

* The expanded pop-up (control bar with caption + tick + cross) is
  `g_popup.expanded` at 0x007fdfa4, set by `PU_DeleteInput` (uimisc.c:340). For
  an A/B without clicking the delete icon, poke `g_popup_info + 0xe4` to 1 while
  a pop-up is open.
* Canvas crops: `drawImage` from the game canvas into a scratch canvas and
  return the data URL; a result over the tool limit is saved to a file the
  integrator decodes (`png_from_result.py`).
* The faithful build wants the same seven targets as the default one before
  `ctest` (`install_paths` needs `legoland_pathtest`).
