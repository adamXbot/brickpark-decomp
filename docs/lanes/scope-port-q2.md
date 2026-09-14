# PORT-Q2 — quirk batch two, and Q5 (2026-09-13, done by the integrator)

Both batch-two lane agents stalled at their first step (the harness's stream
watchdog, sixth and seventh stall of the run), so the integrator did the batch in
the integration worktree. Policy: `docs/QUIRKS.md`. Every change is in a
`#if defined(LEGOLAND_PORTABLE) && !defined(LL_FAITHFUL)` arm with the original
statement in the `#else`; VC6 sees the original text.

## Q5 — the load ghosts, measured

Same walk on the default build (fixed) and the faithful build (`-DLL_FAITHFUL=ON`),
each from a virgin IDBFS: `P1.start()` (free play), let the park fill, save through
OPTIONS → SAVE GAME slot 1, `ll_flush_profiles()`, full page reload, load through
the title screen, then read `llPark()` three times ten seconds apart.

| | at save | after load, settled |
| --- | --- | --- |
| fixed | people 28, count 28, limit 30 | people **30**, count 30, ghosts **0** (sim 2042 → 2757) |
| faithful | people 30, count 30, limit 30 | people **60**, count 30, ghosts **30** (sim 2609 → 3181) |

The fix is PORT-M18 §3's: tally the blokes BLK4 restores and store the tally into
`g_visitor_count` after the loop (`savegame.c`, LoadGame). The shipped game left
the counter at the zero `ClearBlokeList` wrote, so `SpawnVisitor` topped the park
up from zero. **Policy rule 2:** a `.sav` round trip on the default build now
produces a different park from the shipped game (the right one); the faithful
build reproduces the shipped result for save-compatibility oracles.

## The eleven mechanical items

| # | file | change | evidence |
| --- | --- | --- | --- |
| Q10 | `ridecb1.c` `Restaurant1_Tick` | `nfree = 0; freeidx = 0;` at the top of `case 0`, so each rider counts the seats afresh | inspection: the only reads are inside `case 0` |
| Q12 | `ridecb8.c` | the second `IncrementObjectCount` takes `g_bs_mermaid_cls` | inspection: the surrounding code sets `obj.cls` to the water class then the mermaid class in the same order |
| Q13 | `logflume.c` `LFEntrance_Remove` | `axis_y.origin = g_lf_footprint.v[1]` | inspection: x uses `v[0]`; `logflume.c:1674-1675` and `:1755-1756` build the same origin from `(v[0], v[1])` |
| Q15 | `screens3.c` `AdventureThemeInput` | the else-arm is the LEGOLAND/Egypt/Inca buttons' (`screens3.c:810`) verbatim | inspection |
| Q16 | `screencb2.c` | `g_bz_layers->flags \|= 0x2000` | inspection: `screencb.c:202` `OctopusCafe_Create` sets the same flag on `def->sprite->flags`; the ObjDef's +0x1c has no 0x2000 meaning |
| Q17 | `render4.c` `PaintTileLayer` (WIP 12.3%) | the second copy's no-callback arm paints at `(px + state.halfw, py + halfh)` | inspection: the first copy's arms and the second copy's other arm all offset by the half tile |
| Q18 | `person3d.c` | `box[6] = (bmax.x, bmax.y, bmin.z)` | free-play figures crop, fixed vs faithful: a crop of the free-play figures (image not kept in the repository) — figures stand on the path in both; no visible shift at 3× |
| Q19 | `workers2.c` | `g_worker_on_mouse = 0` on a successful drop | readers: `RenderWorkerOnMouse` (behind `g_drag_lock`, gameframe.c:718), `GetSelectedBloke` (rin.c:576, behind `g_selection_lock`), workers.c:302 (the cancel path — correctly a no-op after a drop), workers.c:372 (the pick-up, which sets it) |
| Q20 | `schoolcar7.c` `RotateByHeading` | `default: (0, 0)` | the original returned its frame's two stale dwords; callers only pass cardinal headings |
| Q21 | `logflume2.c` | the 0x41 arm's second disjunct is the mirror (`!fwd && !fwd && back && back`) | inspection: the other seven elbows |
| Q22 | `fpui5.c` `RemoveNewObjectMarker` (WIP) | `i--` after `count--` | inspection: a class appears once, so no behaviour change today |

## Gates

| gate | result |
| --- | --- |
| audit on the 12 files | PASS, rows identical (WIP rows render4/person3d/workers2/fpui5 unchanged) |
| relocs per file / `--all` | 0 MISMATCH / 0 MISMATCH, 16 WIPRELOC (the accepted set) |
| markers | identical |
| progress | 3281 exact / 42 WIP |
| extern / bvstruct / m10 / addr / variadic sweeps | at baseline |
| native default + ctest | 21/21 |
| wasm default + ctest | 28/28 |
| wasm faithful + ctest | 28/28 |
| verify.py | run by the integrator before push |

One integrator slip worth recording: the first Q5 edit landed in the SAVE side
of `savegame.c` (both halves carry a `/* ---- BLK 5..8 */` comment) and broke
the portable compile; the build gate caught it before anything was committed.
