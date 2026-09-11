# Scope PORT-M12 — the Space Tower's "two halves", measured; and the ten +0xa0 draw handlers the sweeps could not see

> **PORT-M12 — Status: DONE (2026-09-12 by PORT-M12)** — branch
> `scope/PORT-M12`. Matching side: `LEGOLAND/*.c` under
> `#ifdef LEGOLAND_PORTABLE`, VC6-gated per file. `portable/src/**` untouched
> (the defect is not shim-side); two SWEEPS changed, `tools/` and
> `portable/tools/`. `docs/HANDOFF.md` read-only. Brief: the browser build's
> rendering defect on a placed ride (PORT-B10's PARK-3).

**Result in two lines.** The Space Tower is **not displaced** — the second
"half" is the ride's own launch gantry and rocket, at the right-hand end of one
160x247 sprite, and every one of its seven layer offsets is byte-identical to
`SPACE TOWER SPRITE.CSP`. The hunt found a different live defect on the way:
**ten more `+0xa0` draw-descriptor bodies in PORT-M11's by-value-square class**,
invisible to both sweeps because all ten are stored in the slot under a name
that is not the name of their body.

| measurement | before | after |
| --- | --- | --- |
| `+0xa0` slot vs body, resolved by ADDRESS | **18** stores / 10 bodies | **0** |
| `+0xa0` slot vs body, resolved by NAME (what M11's sweep did) | 0 | 0 — *it never could see them* |
| struct-RETURN spelling disagreements, tree-wide | **0** (measured, brief suspect (b)) | 0 |
| Space Tower ILF layer offsets vs `SPACE TOWER SPRITE.CSP` | **7/7 exact** | 7/7 exact |
| Park Entrance / LEGO Toy Shop vs `ENTRANCE.CSP` / `LEGO SHOP 1 SPRITE.CSP` | **4/4 and 2/2 exact** | exact |

---

## 1. Reproduced, and measured off the screen

`legoland.html?args=-nointro+WINDEBUG` on 8811, PORT-B10's replay to the built
Space Tower. The ride lands on map square **(63, 35)**, footprint
`{-2,-2,4,3}` = 7x6 = the 42 owner cells the map walk reports.

Every number below was read out of the running heap (`llAddrs()` +
`Module._g_*`, then the ObjDef at `+0x14/+0x18/+0x1c/+0x3c..+0x48/+0x64` and
the ILF table at sprite `+0x08`), and the boxes were drawn back onto the canvas
with a scratch `llCrop`-style overlay so the arithmetic could be checked against
the pixels:

```
tile 32x16   scroll (245,622)   view origin (121,32)
GetTileBounds(63,35)    = {left 308, top 194, right 339, bottom 209}
ObjDef +0x14/+0x18      = (-78, -400)          flags 0x580426  (0x400 set)
anchor = tb.left + HalfOffset(-78), tb.top + HalfOffset(-400) = (269, -6)
```

and the seven ILF layers, `x = anchor + HalfOf(dx,dy)`:

| layer | file | dx,dy | half | sprite | lands at |
| --- | --- | --- | --- | --- | --- |
| 0 | spaceseat3.lls | 126, 360 | 63,180 | 34x31 | (332,174) |
| 1 | **spacet1s.lls** | 0, 0 | 0,0 | **160x247** | (269,-6) |
| 2 | spaceseat4.lls | 125, 397 | 62,198 | 33x30 | (331,192) |
| 3 | **spacet2.lls** | 228, -148 | **114,-74** | 38x216 | **(383,-80)** |
| 4 | spaceseat2.lls | 38, 364 | 19,182 | 34x31 | (288,176) |
| 5 | spacet1.lls | 9, 10 | 4,5 | 72x89 | (273,-1) |
| 6 | spaceseat1.lls | 44, 396 | 22,198 | 30x30 | (291,192) |

Layer 3 is the odd one out by eye: **+114 px right and -74 px up**, i.e. it
draws a 38x216 column over what looks like bare grass, 2.4 tiles right of the
shaft and 4.6 tiles above the pad. That is the thing that reads as "half the
building at another anchor square", and it is what this lane was sent after.

## 2. It is not displaced — three independent measurements

**(a) The offsets are the shipped data, to the byte.**
`SPACE TOWER SPRITE.CSP` (200 bytes, `Legoland.res` leaf 102) is
`u16 layers | u16 2 | u32 namelen | name | {i32 dx, i32 dy} * 7 | {u32 len,
name} * 7`. Decoded (`/tmp/port-m12-csp.py`, kept out of the tree):

```
layer 0  dx=126  dy=360   spaceseat3.lls      layer 4  dx=38   dy=364   spaceseat2.lls
layer 1  dx=0    dy=0     spacet1s.lls        layer 5  dx=9    dy=10    spacet1.lls
layer 2  dx=125  dy=397   spaceseat4.lls      layer 6  dx=44   dy=396   spaceseat1.lls
layer 3  dx=228  dy=-148  spacet2.lls
```

Seven pairs for seven pairs, identical to the heap. The ILF loader, the halving
(`HalfOf` in printlist.c, `HalfOffset` in renderview.c, `HALF` in math3d.c — all
three truncate toward zero, and all six values in play here are even anyway) and
`GetTileBounds` are all faithful. Same check on the other two placed classes:
`ENTRANCE.CSP` 4/4 — `(149,-415) (-201,-360) (-372,-219) (-80,-185)` for
TOWER1 / TSTILE / TOWER2 / Booth — and `LEGO SHOP 1 SPRITE.CSP` 2/2 —
`(0,0) (194,-43)` for lshop1 / lshop1a. Nothing in the tutorial park's object
sprites is off by a pixel.

**(b) Layer 3 is a ROCKET, and it stands on the ride's own gantry.**
Poking `dx[3] -= 200, dy[3] += 200` in the live ILF table moved a red-and-white
**rocket with a flame trail** down and left across the tower; the gold-and-red
spire under it did **not** move. Then hiding layer 1 (`flags |= 0x4000`, which
is what `PrintSprite` skips) removed the spire **and** left the shaft mattes,
the canopy wheel and the rocket standing. So the spire is part of
`spacet1s.lls`, the ride's own 160x247 body, at relative x 121..159 — and
layer 3's canvas is relative x 114..152, y -74..142, whose **bottom edge meets
the gantry's top within 6 px**. It is one ride: tower on the left of the sprite,
launch gantry on the right, rocket climbing out of the gantry. The final
screenshot (S5) shows the grey pad running continuously under both.

**(c) Nothing else in the frame is split.** All 111 owner cells in the tutorial
park belong to four classes (`Path` x108, `Park Entrance`, `Space Tower Ride`,
`LEGO Toy Shop`); boxes drawn for every layer of the three that have sprites sit
on their content. The four live blokes all carry a sane `Person3D +0x1c` screen
pair (`(-6,-52) (-303,120) (-116,214) (118,316)` — three simply off the left of
the view); no visitor sprite is split.

**So PORT-B10's PARK-3 is CLOSED as "not a defect".** B10 saw "a squat block,
no taller than the park entrance"; that was the real bug and PORT-M10's
`AddObjectToBuildList` fix cured it. What is on the screen now is the ride as
shipped, and the "two halves" reading is the gantry.

## 3. What the hunt did find: ten more `+0xa0` bodies in M11's class

PORT-M11 §1b closed the ObjDef **+0xa0 DRAW** slot for eight classes. The slot
is `SpriteDesc* (*draw)(void* ctx, BPos base)` (renderview.c:304) and
renderview.c:1238 calls it that way, so on wasm32 the 2-byte square is passed
**indirectly**, as a pointer to a shadow-stack temp, while a body spelling it
`unsigned short` takes that pointer's low half for a map square. Same i32
arity: `wasm-ld` is silent, `linkreport` sees no conflict,
`test_callback_types.c` checks the slot not the parameter.

**Ten more were live.** Both sweeps pair the name stored in the slot with a body
of the SAME NAME, and every one of these ten is stored under a stale alias:

| stored as | at | body | file |
| --- | --- | --- | --- |
| `SpaceTower_Draw` | interfaces.c:854 | `SpaceTower_GetDrawDesc` | mechrides.c |
| `Copters_Draw` | interfaces.c:715 | `Copters_GetDrawDesc` | mechrides.c |
| `SafariRide_Draw` | interfaces.c:768 | `SafariRide_GetDrawDesc` | mechrides.c |
| `SpiderRide_Draw` | interfaces.c:815 | `SpiderRide_GetDrawDesc` | mechrides.c |
| `SpinningBarrels_Draw` | interfaces.c:917 | `SpinningBarrels_GetDrawDesc` | mechrides.c |
| `PlaneRide_Draw` | interfaces.c:964 | `PlaneRide_GetDrawDesc` | mechrides.c |
| `Catapult_Draw` | interfaces.c:670 | `Catapult_GetDrawDesc` | catapult.c |
| `Joust_A0` | ridesave.c:155 | `Joust_GetDrawDesc` | joust.c |
| `TempleSlide_A0` | ridesave.c:331 | `TempleSlide_GetDrawDesc` | joust.c |
| `Shop_Draw` | interfaces.c:1365 and **eight more** | `Shop_GetDrawDesc` | westtown.c |

18 stores, 10 bodies — every western-town shopfront shares the last one.
`bodies.get(fn)` returned `None` for all 18 and the rows were dropped **in
silence**, which is the one thing a gate may never do. gen_link's own manifest
says the tree has **262 function aliases (stale extern names)**, so the hole is
that wide.

**How much of a pixel does it move? None, today.** All ten bodies do the same
five things and the square lands only in `g_*_draw.f0c`, which is
**write-only in the whole tree** (renderview.c's `SpriteDesc` reads
`sprite/dx/dy/mode/layer_mask`, never `+0x0c`). So this is not the render
defect. It is still a live read of a shadow-stack address as a map square, in
exactly the class M11 closed, and it is one field-read away from being visible —
`JcMonkeyFish_GetDrawDesc`'s `f->pos.w == arg` scan (M11 §1b) is the same
shape and *was* visible.

**Fixed** with PORT-M3's `_vc6_body` rename, the shape M11 used: VC6 compiles
the matched text unchanged, the portable build exports a wrapper of the slot's
own shape over it (`RideTile` / `ShopTile`, each the file's existing 2-byte
union). The PORT-M7 declarations that carry the alias names had to move with
them — `interfaces.c` x8 and `ridesave.c` x2 now take a 2-byte
`LLDrawSquare` (a portable-arm union those two files did not have; cf. M11 §5.6,
which asks `ll_portable.h` for a shared `LL_SQUARE`). Without that, gen_link
types the alias forwarder from the declaration and hands the wrapper a `u16`
where it now expects a pointer — strictly worse than before.

## 4. Both sweeps changed, so this cannot hide again

**`tools/port_m10_bvstruct_sweep.py`** (the slot section M11 added; M11 §5.4
already asks for it in the round gate) now resolves a stored name through its
own declaration's `/* 0xADDR */` comment and looks the body up **by address**
when the name misses. The name path is tried first, so an unaliased store costs
nothing. **18 -> 0** on this tree; `--selftest` still passes its five
assertions.

**`portable/tools/bvstruct_sweep.py`** (PORT-A9's replacement) did not know
PORT-M3's `#define X X_vc6_body` rename, which M11's scratch tool has known
since its line 218. Without it every repaired body is compared **in its VC6
spelling** against a declaration that now agrees with the WRAPPER — 10 false
NOISY rows for this commit alone. It now follows the rename and reports the
wrapper's signature. `--selftest` still passes (23 source checks + the
emcc-vs-clang controls).

*The struct-RETURN direction the brief asks about is a third thing neither
sweep covers, and it is CLEAN — see §5.*

## 5. The other four suspects, each with its measurement

**(b) return-by-value `Offset` / `Pos` across TUs — CLEAN, and the sweep should
still learn it.** A struct RETURN is a different ABI question from a by-value
parameter: x86 cdecl returns an 8-byte pair in `eax:edx` and a 4-byte one in
`eax`, wasm32 returns any multi-member aggregate through a hidden `sret`
pointer. A scratch sweep keyed on address over every `// FUNCTION:` marker and
every address-carrying `extern` (`/tmp/port-m12-rettype.py`) found **6**
addresses whose return spelling differs at all, and every one of the six is a
resolver miss on my side, not a disagreement:

* `0x00441ee0 GetRenderOffsetForLayer` — 14 declarations + the definition, all
  `Offset`; `legoland.h:34` and every local copy are `{int ox; int oy;}`.
* `0x00442cc0 GetScreenCoordsForObject` — 24 spellings; 20 `Offset`, 4 `Pos`
  (logflume2.c, logflume5.c, ridecb8.c, waterworks.c), and **`Pos` is
  `{int x; int y;}` in all four** — the same 8 bytes, same two members, same
  wasm sret. ABI-identical.
* `0x00411220 / 0x00411290 / 0x004112c0` the three LF quad helpers and
  `0x00499a70 GetOrderCentre` — all `Pos`, 8 bytes, everywhere.

So **suspect (b) is not the defect**, and there is no live instance of the class
in the tree. `bvstruct_sweep.py` still cannot see it — it classifies parameters
only — and a one-address disagreement would be as silent as the parameter class.
**Recommendation for the next tools lane: give the sweep a RETURN section**, on
the same address key; the scratch script is 190 lines and its output is already
in the shape the existing `show()` wants.

**(a) M11's +0xa0 change for this class — not the cause, and reverting it is
unnecessary.** The Space Tower's own `+0xa0` was never in M11's eight (§3 above
is why), so there was nothing of M11's to revert here; and the field the square
reaches is write-only. Measured directly instead: the descriptor's `f04/f08`
come from `ObjDef +0x14/+0x18` = `(-78,-400)`, and the anchor computed from them
lands the 160x247 body exactly on the pad.

**(c) M5's rounding sites — not in this path.** The object-placement chain is
integer end to end: `GetTileBounds` (pathbuild.c:195, shorts and shifts),
`HalfOffset`, `PrintSprite`'s `HalfOf`. No `fistp` site is between the map
square and the blit; the float projection (`renderview.c` overview,
`tri3d.c`) only serves the overview map and the 3D people. An off-by-one there
would shimmer everything by a pixel, not move a piece 114 px.

**(d) the shim's `Blt` / `BltFast` source rect — not in this path either.**
Object sprites are painted by the game's own software blitter
(`PrintSprite -> RenderSprite -> SoftPrint*`) into the surface between one
`Lock` and one `Unlock` (docs/runtime/presentation.md §5); `ddraw.c`'s `Blt`
carries the finished frame, not the sprite. Nothing in
`portable/src/hostwin/**` was touched.

## 6. Before / after

Both frames are the same square, the same scroll, with the seven ILF layer
boxes drawn from the numbers in §1 (magenta = L1 body, red = L3 rocket, blue =
L5 canopy, the four car boxes labelled).

* **before** (`0x53363e94` at frame 1032 of the pre-fix build): canopy, four
  legs, blue/gold shaft, seats on the pad; the red L3 box off to the right over
  what reads as grass, with the flame column inside it; the gold/red gantry
  directly below it, inside the magenta L1 box.
* **after** (`0x59967fd9`, frame 3590): pixel-for-pixel the same ride. The
  anchor, `GetTileBounds`, the ObjDef offsets and all seven layer offsets read
  back **identical** — `(63,35)`, `{308,194,339,209}`, `(-78,-400)`,
  `(269,-6)`, `126/360 0/0 125/397 228/-148 38/364 9/10 44/396`. The front-end
  frame hash is `0x093021ac` in both builds, and `llPark()` walks
  `money 1000 -> 960`, `people 3 -> 4`, `visitorLimit 3 -> 4` exactly as
  PORT-M11 §1e recorded.

The fix is not supposed to move a pixel — `f0c` is write-only — and it does not.
What it removes is a read of a shadow-stack address by ten classes, nine of them
not reachable in the tutorial at all (Copters, Safari, Spider, Spinning
Barrels, Plane, Catapult, Joust, Temple Slide, and all nine western-town
shopfronts), i.e. exactly the classes a page walk cannot gate.

## 7. Gates

| gate | result |
| --- | --- |
| `audit.py` on the 6 touched `LEGOLAND/*.c` | **6 x `PASS: 0 function(s) failed the extent gate`**, same rows as base, 0 REJECT / FAIL / COMPILE FAILED |
| the VC6 view | every added and every removed line is inside a `#ifdef LEGOLAND_PORTABLE` arm — the 10 replaced declarations were already in `#else /* PORT-M7 */` arms; VC6's text does not move, which is why the audit rows are unchanged |
| `relocs.py` per touched file | **0 MISMATCH** |
| `relocs.py --all` tree-wide | **0 MISMATCH** |
| `progress.py --check` | **3281 exact / 42 WIP**, 665/675 exports (98.5%); report regenerated and committed |
| `portable/tools/extern_sweep.py` | clean — "the class is closed" |
| `portable/tools/bvstruct_sweep.py` | **0 unaccepted silent, 1 silent (accepted), 5 noisy** = the base numbers; `--selftest` PASSES |
| `tools/port_m10_bvstruct_sweep.py` | **slot 0** (18 before, by address); `--selftest` PASSES its 5 assertions |
| struct-RETURN sweep (scratch, §5) | 0 live disagreements |
| wasm: `emcmake cmake` + `ninja` + the 7 extra targets | exit 0, no wasm-ld signature warning, "imported with more than one signature: 0" |
| wasm `ctest` | **24/24** |
| native: `cmake` + `ninja` + `legoland_tests legoland_cbtypes` + `ctest` | **17/17** |
| the page, 8811 | 3 590 frames, **`traps: []`, `dead` null**, 35.7 fps, 99.1 % non-black, the tower built and the park walking |

## 8. Open, for the integrator

1. **`bvstruct_sweep.py` has no SLOT section at all.** PORT-A9's file replaced
   PORT-M10's scratch tool but carried over only the decl-vs-def half; the SLOT
   vs BODY direction — the one that found all 18 of this lane's sites and all 22
   of M11's — lives only in `tools/port_m10_bvstruct_sweep.py`. Until it moves,
   **both** have to be in the round gate. M11 §5.4 asks for the same thing.
2. **Give the sweep a RETURN section** (§5). No live instance today; the class
   is as silent as the parameter one and nothing else in the tree looks at it.
3. **`LLDrawSquare` is the fourth ad-hoc 2-byte square typedef** a lane has had
   to add to a file that had none (M11 §5.6 counted the first three). One
   `LL_SQUARE` in `portable/hostwin/include/ll_portable.h`, which is
   force-included everywhere, would make the next one a one-line fix.
4. **`0x00439c90` is still in the accepted baseline** (`LegoMedia_Remove` vs
   `MediaShop_Remove`, +0x9c). It is the same alias-name shape as this lane's
   ten, one slot along, and the baseline's reason — "the removal handler is
   registered through the scalar name" — is a description of the bug, not an
   argument against fixing it. Worth a look now that the alias resolution
   exists.
5. **PORT-B10's PARK-3 should be struck from the open list** (§2). PARK-1, the
   `LINK "SPACE TOWER RIDE"` objective, is a separate question and this lane did
   not touch it.
6. **A harness note, not a finding.** The browser page runs at **0.5 fps** while
   the Browser pane is hidden (`document.visibilityState === "hidden"`): the
   ASYNCIFY loop is `requestAnimationFrame`-driven and Chrome throttles it to
   ~2 s a frame, so startup never reaches `WinMain` inside a reasonable wait. A
   `MessageChannel` shim over `requestAnimationFrame` (and `setTimeout` for
   delays <= 60 ms) restores 35.7 fps and changes nothing the game can observe.
   Any lane driving the page from a hidden pane needs it.

## 9. Files touched

| file | change |
| --- | --- |
| `mechrides.c` | 6 `_vc6_body` renames + wrappers (SpaceTower, Copters, SafariRide, SpiderRide, SpinningBarrels, PlaneRide) |
| `joust.c` | 2 (Joust, TempleSlide) |
| `catapult.c` | 1 (Catapult) |
| `westtown.c` | 1 (Shop — the one all nine shopfronts share) |
| `interfaces.c` | the portable `LLDrawSquare` typedef; 8 PORT-M7 declarations move to it |
| `ridesave.c` | the same typedef; 2 declarations (`Joust_A0`, `TempleSlide_A0`) |
| `tools/port_m10_bvstruct_sweep.py` | slot-to-body resolution by ADDRESS when the name misses |
| `portable/tools/bvstruct_sweep.py` | follows PORT-M3's `_vc6_body` rename |
| `docs/LEGOLANDPROGRESS.HTML` | regenerated (line numbers moved) |

No `docs/HANDOFF.md`, no `portable/src/**`, no `tools/verify.py` run.
