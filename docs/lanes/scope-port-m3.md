# Scope PORT-M3 — the typed-callback pass: every table the game calls through

> **Status: IN PROGRESS (claimed 2026-09-12 by PORT-M3).** Branch
> `scope/PORT-M3` from `09f14a5b` (the PORT-M2 merge). The follow-up PORT-M2
> opened: **function pointers whose call site disagrees with the body the slot
> actually holds.** wasm-ld cannot see these — a cast or a `void*` slot hides
> them, and an address-taken-only reference never gets a
> `signature_mismatch:` stub — so they are invisible to every existing gate and
> they trap at runtime with a bare indirect-call type error.

## 1. What this lane is looking for, and why the old gates cannot see it

PORT-M1 and PORT-M2 closed **direct** call mismatches: wasm-ld resolves a
direct call made through the wrong prototype to a `signature_mismatch:<name>`
stub and says so. This lane is about **indirect** calls, where there is no
warning at all:

* a `call_indirect` carries the *call site's* type and the table entry carries
  the *body's* type; if they differ the call traps (`RuntimeError: ... indirect
  call type mismatch`), with nothing in the link log;
* wasm-ld only builds a mismatch stub for a symbol it sees **called directly**.
  A function whose address is merely *taken* (`def->cb_a8 = Foo;`) gets the
  real function index in the table however wrong the local declaration is —
  confirmed on this tree: `screens2.c` declares `TitleNewInput(Icon*, int)`
  while `screens3.c` defines it `(Icon*, int, int, int)`, and the whole module
  has exactly **one** `signature_mismatch` symbol (`printf`, generator-side);
* `clang -Wincompatible-function-pointer-types` finds nothing either, because
  the disagreement is always CROSS-translation-unit: the storing file declares
  the callee with the slot's shape, so the store is locally consistent. Turning
  the warning back on over all 258 sources yields **4** warnings tree-wide
  (section 6), none of them the real problem.

So the census has to be built from the objects. Three tools, all in the
scratchpad (`port-m3-*.py`), all reading data the build already produces:

1. **real signatures** — `linkreport.wasm_object_sigs()` over the 258 wasm
   objects gives `name -> (params, results)` for every DEFINED body (3347) and
   the signature every object REFERENCES it with (2853).
2. **call sites** — every game source recompiled to `-O0 -g` LLVM IR; every
   `call` through a register is an indirect call, and the IR prints the exact
   lowered type (`float` vs `i32` included) plus the `!dbg` line and the
   `getelementptr` that loaded the pointer, i.e. the struct and field index.
   **475 indirect call sites; 185 of them game-side** (the other 290 are
   DirectDraw / DirectSound / DirectInput / DirectMusic COM vtables, which are
   the host shim's contract, not the game's).
3. **what is in each slot** — the `->cb_xx = Foo;` stores scanned out of the
   sources, and for the tables that live in `.data`, the generated
   `gen-browser/globals.c`, which names every function the closure re-points
   each table word at.

Cross-referencing the three gives, per slot: the type the call site uses and
the set of real signatures of the bodies that reach it. **Every slot whose body
set contains more than one wasm signature is unfixable from the call site
alone** — the original tolerated it because on x86 cdecl the caller pushes and
the caller cleans up, so a callee that ignores the last argument, or reads a
dword the caller pushed as a float, costs nothing.

## 2. The inventory, ordered by how early the front end reaches it

`CALL` is the type the call site uses; `BODIES` the real signatures found in
the slot. A row is a trap exactly when they differ.

| # | when | slot / table | call site | CALL | BODIES | verdict |
| --- | --- | --- | --- | --- | --- | --- |
| 1 | title screen, every frame | `g_present` 0x004b9ca4 | render2.c:226 | `() -> i32` | `() -> i32` (`PresentFlip`, `FlipPrimary`) | **consistent** |
| 2 | title screen, every frame | `Icon +0x28` render | fpui.c:671/705/739, popupmisc.c:459 | `(i32) -> i32` | `(i32) -> i32` x15 | **consistent** |
| 3 | first click on any icon | `Icon +0x2c` input | fpui.c:774/777, uimisc.c:607 | `(i32, i32, i32, i32) -> i32` | 64 bodies agree, **17 are `(i32, i32) -> i32`** | **TRAP — fixed, section 4** |
| 4 | first click on the OK / back icon of a screen | `g_icon_handler1` 0x006687bc, `g_icon_handler2` 0x006687c0 | fpui.c:768/787 | `(i32, i32, i32, i32) -> i32` | 15 agree, **4 are `(i32, i32) -> i32`** (`FreePlayAcceptInput`, `InGamePrimaryIcon`, `ReportNextPageInput`, `AdvertGoBackInput`) | **TRAP — fixed with row 3** (same bodies) |
| 5 | advisor movie on the front end | `AdvisorClip +0x20` stop / `+0x24` tick | advisor.c:300 | `(i32) -> void` | `(i32) -> void` (`Castle_Tick`, `Track_Tick`), `() -> void` (`AdvisorMovieTick`) | tick slot mixed; `stop` consistent. Open, section 5 |
| 6 | a sprite with a painter | `SpriteRec +0x08` image | sprite2.c:253 | `(i32) -> void` | `(i32) -> void` | **consistent** (cast `(SpriteDrawFn)`) |
| 7 | first `.bmp`/level load | `g_level_db_sections` 0x004bb6f8 keyword handlers | levelkw.c:937/956 | `(i32, i32, i32) -> i32` | 68 agree, **22 are `(i32, i32) -> i32`** | **TRAP — open, section 5** |
| 8 | every event tick in a level | `g_event_tick[70]` 0x004b9d44 | fpui3.c:657 | `(i32) -> i32` | `(i32) -> i32` x68 | **consistent** |
| 9 | every bloke, every tick | `g_lowlevel_ai[]` 0x004bd34c | blokemisc.c:107, lowlevelai.c:607 | `(i32) -> void` | `(i32) -> void` x16 | **consistent** |
| 10 | a placed object's build cursor | `ObjDef +0x90` place/update/effect | eventtick.c:400, gameframe.c:991, mappath.c:585, objmap2.c:556, objrect.c:385, workorder2.c:1018, buildtick.c:242/285 | `(i32, i32, i32) -> void` | `(i32, i32, i32) -> void` x26 | **consistent** |
| 11 | a placed object, deselect | `ObjDef +0x94` | gameframe.c:958-1069, eventtick.c:511, mappath.c:478, fpui3.c:295, misc3.c:700 | `(i32, i32) -> void` | `(i32, i32) -> void` x20 | **consistent** |
| 12 | removing an object | `ObjDef +0x9c` | objmap2.c:1895 | `(i32, i32, i32) -> void` | `(i32, i32, i32) -> void` x55 | **consistent** |
| 13 | class resource init | `ObjDef +0xa4` | llidb_odf.c:281 | `(i32) -> void` | `(i32) -> void` x33 | **consistent** |
| 14 | drawing a class' sprite descriptor | `ObjDef +0xa0` | renderview.c:1221 | `(i32, i32) -> i32` | 9 agree, **1 is `(i32,i32,i32,i32) -> void`** (`DrawBasicPath`) | **TRAP — open, section 5** |
| 15 | a class' per-frame sim | `ObjDef +0xa8` | renderview.c:1130 | `(i32) -> void` | 18 agree, **2 are `() -> void`** (`BoatingSchool_Tick`, `JungleCruise_Tick`) | **TRAP — open, section 5** |
| 16 | class teardown | `ObjDef +0xac` | sysmisc.c:664 | `(i32) -> void` | 7 agree, **20 are `() -> void`** | **TRAP — open, section 5** |
| 17 | save a game | `ObjDef +0xbc` | savegame.c:941 | `(i32) -> i32` | 5 agree, **15 are `() -> i32`**, 8 alias | **TRAP — open, section 5** |
| 18 | load a game | `ObjDef +0xb8` | savegame.c:1365 | `(i32) -> i32` | 5 agree, **10 are `() -> i32`**, 8 alias | **TRAP — open, section 5** |
| 19 | appraisal / goal counting | `ObjDef +0xc0` | appraisal.c:172-212, eventtick2.c:232 | `(i32, i32) -> i32` | `(i32, i32) -> i32` x5 | **consistent** |
| 20 | castle-class dispatch | `CtIface +0x04..+0x14` | castleobj.c:844-877, coaster.c:525 | 1-3 args, per slot | see section 5 | mostly consistent; the `update2` cast is wrong |
| 21 | a track piece rebuild | `TrackDesc +0x20` build | coaster.c:330 | `(i32, i32) -> void` | `(i32, f32) -> void` (`TrackNode_SetBothHeights`), `(i32) -> void` (`TrackNode_LoadDescHeights`) | **TRAP — fixed, section 3** |
| 22 | a track piece draw/place/remove/query | `TrackDesc +0x1c/+0x24/+0x28/+0x2c` | coaster.c:323/1405, schoolcar.c:693, unref1.c:185/192, coaster4.c:94/116 | `(i32) -> void` / `(i32) -> i32` | same | **consistent** |
| 23 | drawing any coaster/track curve | `RouteGeom->vt` slots, reached through the two **casts** in coaster10.c:170/212 | coaster10.c | `(i32, i32, i32) -> void` | `(i32, f32, i32) -> void` x15, `(i32, i32) -> i32` x3 | **TRAP — fixed, section 3** |
| 24 | the same vtable, typed path | `GeomEval +0x04`, `PieceHooks +0x04`, `PosHooks +0x00/+0x04` | coaster13.c:236, coaster12.c:432/439, coaster3d.c:319/321, unref3.c:460-470 | `(i32, f32, i32) -> void` | `(i32, f32, i32) -> void` | **consistent** |
| 25 | the same vtable, element-keyed pair | `PosHooks` via `hooks[slot]` | schoolcar3.c:489/491 | `(i32, i32, i32) -> void` | slot 3 of each group holds `TrackCurve_GatherParams` / `GetLimits` / `GetQuarterTurnSamples`, all `(i32, i32) -> i32` | **open finding, section 5** |
| 26 | coaster physics (RK4) | `PhysObj +0x00..+0x30` (10 slots) | schoolcar6.c:162-181 | per slot, `f32` where the maths needs it | `g_car_pool_hooks` bodies | **consistent** |
| 27 | Romberg integration | `PhysOps +0x00..+0x24` | coastershade2.c:680-693, unref1.c:985-995 | per slot | same pool | **consistent** |
| 28 | fast square roots | `g_fast_sqrt` 0x00829a58, `g_fast_rsqrt` 0x00829a5c | coaster9.c:102, coastertiny.c:212 | `(f32) -> f32` via cast | `() -> void` declarations; the bodies are x87-ABI asm | **open, PORT-B5 territory, section 5** |
| 29 | track solver | `g_track_solver` 0x004b63fc, `g_find_bracket` | coaster13.c:485/490, unref3.c:566/571 | `(i32, f32, f32, i32) -> i32` | see section 5 | checked, consistent |
| 30 | report screen | `g_report_setters[25]` 0x004b7e38 | eventtick.c:633 | `(i32, i32) -> void` | not re-pointed by the closure (see section 5) | open |

`ObjDef +0x8c` (tick / select-for-placement), `+0x98` (add) and `+0xb0`
(custom draw) are **registered but never called through** anywhere in the
recovered tree — the 475-site IR census finds no indirect call on those three
offsets. Their body sets are mixed anyway (`+0x8c`: 27 `() -> void` against 2
`(i32) -> void`; `+0xb0`: 14 six-argument against 4 four-argument), so the
first consumer that appears will need section 5's treatment. Recorded here so
the next lane does not have to re-derive it.

## 3. The fixes in this lane (the cast / float family)

### 3a. coaster10.c — the geometry vtable casts (the brief's item 3)

`TrackCurve_EvaluateUp` (0x00429b90) and `TrackCurve_EvaluatePosition`
(0x00429a80) reach the curve's method table through a hand-written cast:

```c
(*(void (__cdecl **)(void*, int, Vec3f*))((char*)geom->vt + 0x1c))(geom, t, out);
(*(void (__cdecl **)(void*, int, Vec3f*))((char*)geom->vt + mode * 8))(geom, t, out);
```

The table is `g_car_class_vt` (0x004dd5e0, `CarClassTablesInit` in
schoolcar.c), three eight-slot groups — cubic, line, arc — and
`g_curve_line_hooks` / `g_curve_arc_hooks` (0x004dd600 / 0x004dd620) are
interior aliases of it, +0x20 and +0x40. Every position/tangent/offset/up
body in it is

```
TrackCurve_CubicPosition / CubicTangent / CubicOffsetPlus / CubicOffsetMinus
TrackCurve_LinePosition  / LineTangent  / LineOffsetPlus  / LineOffsetMinus
TrackCurve_ArcPosition   / ArcTangent   / CoasterArc_GetPosRail0 / ...Rail2
TrackCurve_NormalAt / LineUpVector / CubicUpVector            (i32, f32, i32) -> void
```

i.e. `(curve, float t, Vec3f* out)` — **`f32` in the second parameter**, while
the cast says `int`. Both casts therefore trap on wasm at the first coaster
draw. The portable arm casts to the bodies' real type and moves `t` across as
the same bits with `LL_ASFLT` (it is a dword parameter holding a float, the
lever PORT-M2 section 3 documents; `t` is an lvalue, so the macro is safe):

```c
(*(void (**)(void*, float, Vec3f*))((char*)geom->vt + 0x1c))(geom, LL_ASFLT(t), out);
```

The `#ifndef` arm keeps the original text, `__cdecl` included, so VC6 sees the
lever it needs.

**Worth recording for whoever owns the coaster lane:** slot 6 of each group
(`+0x18`, i.e. `mode * 8` with `mode == 3`) is NOT a position getter — it is
`TrackCurve_GatherParams` / `TrackCurve_GetLimits` /
`TrackCurve_GetQuarterTurnSamples`, all `(i32, i32) -> i32`. A
`TrackCurve_EvaluatePosition(at, 3, t, out)` call would trap even after this
fix, and it would have been nonsense on x86 too (it would read the int return
as a position write). Nothing in the tree passes `mode == 3` to it; the
element-keyed *pair* at that offset is what schoolcar3.c reaches (row 25).

### 3b. coaster.c + coaster12.c — the TrackDesc `build` slot

`TrackNode_Build` (0x0041cfd0) is `n->desc->build(n, mode)` with the slot
declared `void (*build)(TrackNode*, int)`. The four descriptor tables in
`.data` hold two different bodies in that word (`globals.c`, 0x004b5d20 /
0x004b5d58 / 0x004b5d90 / 0x004b5dc8, word 8):

| table | body | real signature |
| --- | --- | --- |
| `g_track_desc_flat` | `TrackNode_SetBothHeights` coaster12.c 0x00427a80 | `(i32, f32) -> void` |
| `g_track_desc_height`, `_height0`, `_heightpath` | `TrackNode_LoadDescHeights` coaster12.c 0x00427c70 | `(i32) -> void` |

so the slot is mixed and **both** bodies disagree with the call. The second
argument really is a float: coaster5.c passes `rec->head_node->jout.f04`, the
joint's stored height dword, and `SetBothHeights` stores it into two `float`
fields with `fstp`. The fix needs both ends, and the table is filled by the
closure from `.data`, so the only place to stand is the definitions:

* coaster12.c gives `TrackNode_LoadDescHeights` a portable-arm twin of the
  canonical shape (`(TrackNode*, float)`) over the renamed matched body, M1's
  rename pattern;
* coaster.c's portable arm declares the slot `void (*build)(TrackNode*, float)`
  and passes `LL_ASFLT(mode)`.

After it the slot is uniformly `(i32, f32) -> void`.

## 4. The fixes in this lane (the icon input arity)

The `Icon +0x2c` input handler is called with four arguments in both call sites
that exist (`fpui.c:774/777` — `p->input(p, ev, g_mouse.x - p->x, g_mouse.y -
p->y)` — and `uimisc.c:607`), and `g_icon_handler1`/`g_icon_handler2` are
called with four as well (`fpui.c:768/787`). 64 handler bodies have that shape.
**17 do not**, and they are all front-end or in-game UI icons — the earliest
thing a user can break:

| body | defined in | registered by |
| --- | --- | --- |
| `AdvertBillundInput` 0x00490090 | uimisc3.c | mapscreen3.c |
| `AdvertCaliforniaInput` 0x00490110 | uimisc3.c | mapscreen3.c |
| `AdvertGoBackInput` 0x00490050 | uimisc3.c | mapscreen3.c (also `g_icon_handler2`) |
| `AdvertWindsorInput` 0x004900d0 | uimisc3.c | mapscreen3.c |
| `CertGoBackInput` 0x004902c0 | uimisc3.c | mapscreen4.c |
| `CertPrintInput` 0x00490300 | uimisc3.c | mapscreen4.c |
| `ChildrenBarInput` 0x00475c50 | uimisc3.c | iconui.c |
| `CloseChildrenBarInput` 0x00475c90 | uimisc3.c | iconui.c |
| `FreePlayAcceptInput` 0x0048ac60 | uimisc.c | fpui2.c (also `g_icon_handler1`) |
| `IndicatorInput` 0x0046fbc0 | uimisc.c | iconui.c (two sites) |
| `PU_PrevInput` 0x004733b0 | uimisc.c | bighelp.c |
| `ReportNextPageInput` 0x00490b20 | uimisc.c | mapscreen2.c (also `g_icon_handler1`) |
| `PU_Delete2Input` 0x004734d0 | fpui5.c | bighelp.c, uimisc.c |
| `ReportHintInput` 0x00490be0 | mapscreen4.c | mapscreen2.c |
| `ReportPrevPageInput` 0x00490b90 | uimisc2.c | mapscreen2.c |
| `SaveDeleteOkInput` 0x0048e450 | frontend2.c | frontend2.c |
| `InGamePrimaryIcon` 0x00474880-family | tinystubs.c | sysstubs.c (`SetInGameIconHandlers`) |

None of the seventeen is ever called directly — every use is a store into a
callback slot (checked by name across all 258 sources), which is why the
two-argument spelling survived: the body reads only `icon` and `ev`, so on x86
the two extra pushed dwords are invisible to it.

The fix is M1's rename pattern in the **defining** file, because that closes
every registration site at once (`PU_Delete2Input` is stored from two files,
and three of these are also stored into `g_icon_handler1/2`):

```c
#ifdef LEGOLAND_PORTABLE
#define ChildrenBarInput ChildrenBarInput_vc6_body
#endif
// FUNCTION: LEGOLAND 0x00475c50
char ChildrenBarInput(Icon* p, int ev)
{ ... the matched body, untouched ... }
#ifdef LEGOLAND_PORTABLE
#undef ChildrenBarInput
char ChildrenBarInput(Icon* p, int ev, int dx, int dy)
{ (void)dx; (void)dy; return ChildrenBarInput_vc6_body(p, ev); }
#endif
```

The `#define` sits ABOVE the `// FUNCTION:` marker, never between the marker
and the signature, and the emitted bytes cannot move: VC6 compiles the
`#ifndef` world, in which the file is character-for-character what it was.

## 5. What still traps, with the recipe for each

Everything in this section is a **mixed body set**: two or more real signatures
reach one call site, so no call-site or declaration edit can fix it. Two
treatments are available and the choice is about where the table is filled:

* **registration-site adapter** — when the slot is written by game code
  (`screen.c`, `interfaces.c`, `castleobj.c`, `loaders.c`, `ridesave.c` write
  every `ObjDef` callback), the portable arm can store a static adapter of the
  canonical type that calls the real body with the arguments it has:
  `static void ll_cbac_Castle_Destroy(void* e) { (void)e; Castle_Destroy(); }`.
  One file holds all of a slot's adapters, VC6 never sees them, and no
  definition moves. Nothing in the tree compares a `cb_*` slot against a
  function address (checked), so substituting the adapter is safe.
* **definition-side forwarder** — when the table lives in `.data` and the
  closure fills it (`g_level_db_sections`, the `g_track_desc_*` descriptors,
  `g_car_class_vt`), the only place to stand is the defining file, exactly as
  section 3b and section 4 do it.

| slot | canonical | bodies needing an adapter | treatment | count |
| --- | --- | --- | --- | --- |
| `ObjDef +0xbc` save | `(i32) -> i32` | `Balloonz_Save`-style 15 × `() -> i32` | registration-site, screen.c + ridesave.c | 15 |
| `ObjDef +0xb8` load | `(i32) -> i32` | 10 × `() -> i32` | same | 10 |
| `ObjDef +0xac` teardown | `(i32) -> void` | 20 × `() -> void` | same | 20 |
| `ObjDef +0xa8` sim | `(i32) -> void` | `BoatingSchool_Tick`, `JungleCruise_Tick` | same | 2 |
| `ObjDef +0xa0` draw desc | `(i32, i32) -> i32` | `DrawBasicPath` `(i32,i32,i32,i32) -> void` | same; check which of the two is the real +0xa0 contract first — a four-argument painter in a two-argument slot smells like a second registration bug of the PORT-M2 1a kind | 1 |
| `g_level_db_sections` keyword handlers | `(i32, i32, i32) -> i32` | 22 × `(i32, i32) -> i32` (`LevelKw_CAPACITYCAP`, `CLEAR`, `DEGRADE`, `ENDLEVEL`, `ENTRANCEFEE`, `EXTENDPARK`, `FEATURE`, `FLASHBUTTOFF`, `FLASHBUTTON`, `FMV`, `GLUE`, …) | definition-side forwarder in the levelkw*.c that defines each | 22 |
| `ObjDef +0x8c` tick/select | unknown — no call site | 27 × `() -> void` vs 2 × `(i32) -> void` | wait for the consumer | 29 |
| `ObjDef +0xb0` custom draw | unknown — no call site | 14 × 6-arg vs 4 × 4-arg | wait for the consumer | 18 |
| `cb_activate`, `cb_destroy`, `cb_draw` (the library-table names in interfaces.c) | per the 14-pointer library table | `cb_activate` 9 × `(i32) -> void` vs 2 × `() -> void`; `cb_destroy` 25 × `() -> void` vs 3 × `(i32) -> void` | registration-site | 5 |
| `AdvisorClip +0x24` tick | `(i32) -> void`? advisor.c's own `tick` slot is `(void)` and the stored `Castle_Tick`/`Track_Tick` are `(i32)` | read advisor.c's call site again with the slot table in hand | 3 |

Four more open items that are NOT arity mismatches:

* **`g_fast_sqrt` / `g_fast_rsqrt`** (0x00829a58 / 0x00829a5c) are declared
  `void (*)(void)` and called through a `(float (*)(float))` cast — a custom
  x87 ABI (the argument arrives in ST(0)). `-Wcast-function-type-strict` flags
  both sites. The bodies are inline-asm in coastermath.c, PORT-B5's file, so
  the fix has to land with the C fallbacks for those bodies: either the pointer
  becomes `float (*)(float)` in both arms and the asm body is given that
  prototype, or the portable arm calls the C helper directly. **Listed for
  PORT-B5**, not touched here.
* **`schoolcar3.c` hooks[slot]** (row 25): `o->hooks[slot].get_pos(o, elem.p,
  &pos)` is a *three*-argument call on a pair whose slot-3 entries
  (`TrackCurve_GatherParams`, `TrackCurve_GetLimits`,
  `TrackCurve_GetQuarterTurnSamples`) are two-argument `i32`-returning bodies.
  coaster3d.c's header comment says slot 1 is keyed by a float and slot 3 by an
  element pointer, and that the two spellings are deliberate caller-side
  levers; but the *bodies* at slot 3 do not have the shape either spelling
  assumes. Either `slot` is never 3 at that site — in which case the cast is
  merely unsound — or the recovery of one of the three is wrong about its
  parameters. It needs a coaster-geometry read, not a type edit.
* **`g_report_setters[25]`** (0x004b7e38) and `g_lt_action_handlers`
  (0x004b8368) are not emitted as re-pointed tables in `globals.c`; they are
  interior words of a larger block, so this census cannot name their entries.
  Whoever closes them should extend `port-m3-datatables.py` to walk interior
  aliases.
* **`castleobj.c:862`** calls the `update2` slot through `(CtAdd)`, i.e.
  `(elem, a)`, while the other castle thunk casts use the slot's own arity.
  `CastleDummy_Update2` / `Castle_Update2` / `Track_Update2` are all
  `(i32, i32) -> void`, so the cast happens to be right; it is listed only
  because a cast that names a *different* slot's typedef is how row 23 went
  wrong.

## 6. Gate results

(filled in per batch)

## 7. Census

(filled in at the end)
