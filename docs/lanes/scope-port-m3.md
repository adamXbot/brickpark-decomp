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

Two caveats on the call-site census, both small and both worth writing down.
It cannot see an indirect call inside one of the 26 bodies that are still
`LL_UNPORTED_ASM()` (12 files; rlepaint.c has eight and tri3d.c four) because
those bodies have no code yet — the blitters and the x87 maths, none of which
dispatches through a game table. And an indirect call made through a pointer the
compiler can constant-fold does not appear as an indirect call at all; at -O0,
which is what this census compiles, nothing folds.

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
| 5 | advisor movie on the front end | `AdvisorClip +0x20` stop / `+0x24` tick | advisor.c:300 | `(i32) -> void` / `() -> void` | `stop` is only ever written 0 (advisor.c:186) and `tick` only ever `AdvisorMovieTick` `() -> void` | **consistent** (the first census pass conflated this `tick` field with `CtIface`'s, which holds `Castle_Tick` / `Track_Tick` `(i32) -> void` and is called through a matching cast) |
| 6 | a sprite with a painter | `SpriteRec +0x08` image | sprite2.c:253 | `(i32) -> void` | `(i32) -> void` | **consistent** (cast `(SpriteDrawFn)`) |
| 7 | first `.bmp`/level load | `g_level_db_sections` 0x004bb6f8 keyword handlers | levelkw.c:937/956 | `(i32, i32, i32) -> i32` | 68 agree, **22 are `(i32, i32) -> i32`** | **TRAP — open, section 5** |
| 8 | every event tick in a level | `g_event_tick[70]` 0x004b9d44 | fpui3.c:657 | `(i32) -> i32` | `(i32) -> i32` x68 | **consistent** |
| 9 | every bloke, every tick | `g_lowlevel_ai[]` 0x004bd34c | blokemisc.c:107, lowlevelai.c:607 | `(i32) -> void` | `(i32) -> void` x16 | **consistent** |
| 10 | a placed object's build cursor | `ObjDef +0x90` place/update/effect | eventtick.c:400, gameframe.c:991, mappath.c:585, objmap2.c:556, objrect.c:385, workorder2.c:1018, buildtick.c:242/285 | `(i32, i32, i32) -> void` | `(i32, i32, i32) -> void` x26 | **consistent** |
| 11 | a placed object, deselect | `ObjDef +0x94` | gameframe.c:958-1069, eventtick.c:511, mappath.c:478, fpui3.c:295, misc3.c:700 | `(i32, i32) -> void` | `(i32, i32) -> void` x20 | **consistent** |
| 12 | removing an object | `ObjDef +0x9c` | objmap2.c:1895 | `(i32, i32, i32) -> void` | `(i32, i32, i32) -> void` x55 | **consistent** |
| 13 | class resource init | `ObjDef +0xa4` | llidb_odf.c:281 | `(i32) -> void` | `(i32) -> void` x33 | **consistent** |
| 14 | drawing a class' sprite descriptor | `ObjDef +0xa0` | renderview.c:1221 | `(i32, i32) -> i32` | 9 agree, **1 is `(i32,i32,i32,i32) -> void`** (`DrawBasicPath`) | **TRAP — left open on purpose, section 5** |
| 15 | a class' per-frame sim | `ObjDef +0xa8` | renderview.c:1130 | `(i32) -> void` | 18 agree, **2 are `() -> void`** (`BoatingSchool_Tick`, `JungleCruise_Tick`) | **TRAP — fixed, section 4b** |
| 16 | class teardown | `ObjDef +0xac` | sysmisc.c:664 | `(i32) -> void` | 7 agree, **20 are `() -> void`** | **TRAP — fixed, section 4b** |
| 17 | save a game | `ObjDef +0xbc` | savegame.c:941 | `(i32) -> i32` | 5 agree, **15 are `() -> i32`**, 8 alias | **TRAP — fixed, section 4b** |
| 18 | load a game | `ObjDef +0xb8` | savegame.c:1365 | `(i32) -> i32` | 5 agree, **10 are `() -> i32`**, 8 alias | **TRAP — fixed, section 4b** |
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

## 4b. The fixes in this lane (one wasm type per ObjDef slot)

Four `ObjDef` slots have a live call site and a mixed body set. All four are
written by game code — `screen.c`, `interfaces.c`, `ridesave.c` and
`castleobj.c` are the only files that assign them — so the portable arm can
register an **adapter of the slot's own type** that drops the argument the body
never read, which is exactly what x86 cdecl did for free:

```c
#ifdef LEGOLAND_PORTABLE
extern int SaveBoatingSchool(void);
static int ll_cb_save_SaveBoatingSchool(void* ll_elem)
{
    (void)ll_elem;
    return SaveBoatingSchool();
}
#endif
...
#ifndef LEGOLAND_PORTABLE
        def->cb_save = SaveBoatingSchool;
#else
        def->cb_save = ll_cb_save_SaveBoatingSchool;   /* PORT-M3 */
#endif
```

| slot | canonical | adapters | in |
| --- | --- | --- | --- |
| `+0xac` teardown | `(i32) -> void` | 21 | screen.c 20, castleobj.c 1 |
| `+0xb8` load | `(i32) -> i32` | 17 | screen.c 6, interfaces.c 10, castleobj.c 1 |
| `+0xbc` save | `(i32) -> i32` | 21 | screen.c 8, interfaces.c 10, ridesave.c 2, castleobj.c 1 |
| `+0xa8` sim | `(i32) -> void` | 2 | screen.c (`BoatingSchool_Tick`, `JungleCruise_Tick`) |

61 adapters, 62 store sites, in four files. Two details are worth recording:

* **the adapter is safe to substitute** because nothing in the tree ever
  compares a `cb_*` slot against a function address — checked by name over all
  258 sources. The slot is written, read and called, never matched.
* **screen.c's declarations had to move first.** It declares every callback
  K&R-style (`extern void SaveBoatingSchool();`, 203 of them), which says
  nothing about the parameters AND gets the return type wrong for the save /
  load pair, so the adapter could not call them. 35 of those declarations got a
  portable arm with the body's real prototype. The remaining 168 are
  address-taken only and harmless; rewriting them would close census rows and
  change no behaviour.

**Three alias names fixed themselves by being declared properly.**
`LoadPlaneRide`, `LoadSpiderRide` and `LoadSpinningBarrels` are stale names no
game TU defines; `gen_link.py` bridges each with a forwarder whose signature it
takes from the *registering file's declaration*, and interfaces.c declared them
`int (void)` while the real bodies (`LoadZoomer`, `LoadSpider`, `LoadSBarrel`,
ridesave.c) are `(i32) -> i32` — the canonical type. The generator therefore
emitted

```c
unsigned int LoadPlaneRide(void) { return ((unsigned int (*)(void))&LoadZoomer)(); }
```

i.e. the casting forwarder PORT-M2 section 4 showed binaryen can turn into an
invalid direct call. Those three are now declared with the real shape, so the
generator emits a plain forwarder and the slot gets the right type with no
adapter at all. `Joust_A0`, `TempleSlide_A0/A8/AC` and `Joust_AC` have the same
shape and are listed in section 5 — they need ridesave.c's declarations to
change, which this lane did not get to.

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
| `ObjDef +0xa0` draw desc | `(i32, i32) -> i32` | `DrawBasicPath` `(i32,i32,i32,i32) -> void` | same; check which of the two is the real +0xa0 contract first — a four-argument painter in a two-argument slot smells like a second registration bug of the PORT-M2 1a kind | 1 |
| **CLOSED in 4b:** `+0xac`, `+0xb8`, `+0xbc`, `+0xa8` | — | — | registration-site adapters, 61 of them | 0 left |
| **CLOSED in 5b:** `g_level_db_sections` keyword handlers | `(i32, i32, i32) -> i32` | the 22 two-argument `LevelKw_*` bodies, all in levelkw3.c | definition-side twins | 0 left |
| `ObjDef +0x8c` tick/select | unknown — no call site | 27 × `() -> void` vs 2 × `(i32) -> void` | wait for the consumer | 29 |
| `ObjDef +0xb0` custom draw | unknown — no call site | 14 × 6-arg vs 4 × 4-arg | wait for the consumer | 18 |
| `cb_activate`, `cb_destroy`, `cb_draw` (the library-table names in interfaces.c) | per the 14-pointer library table | `cb_activate` 9 × `(i32) -> void` vs 2 × `() -> void`; `cb_destroy` 25 × `() -> void` vs 3 × `(i32) -> void` | registration-site | 5 |

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

## 5b. The level-file keyword table (fixed)

`g_level_db_sections` (0x004bb6f8) is the level parser's keyword dispatch: 90
entries, called as `table[i].handler(words, nwords - 1, extra)` by
`ParseKeywordSections` (levelkw.c:937/956). 68 handlers are
`(char**, int, int) -> int`; **22 are `(char**, int) -> int`**, and all 22 are
defined in ONE file, levelkw3.c — `LevelKw_CAPACITYCAP`, `CAPACITYSCALE`,
`CLEAR`, `DEGRADE`, `ENDLEVEL`, `ENTRANCEFEE`, `EXTENDPARK`, `FEATURE`,
`FLASHBUTTOFF`, `FLASHBUTTON`, `FMV`, `GLUE`, `HAP_FACTOR`, `INTERVAL`,
`MAXBLOKES`, `MAXCAPACITY_MAXVISITORS`, `MESSAGE`, `MINCAPACITY_MINVISITORS`,
`PLACE`, `PURGE`, `REPORT`, `UNGLUE`.

The table is filled from `.data` by the closure, so there is no registration
site to stand on: each of the 22 gets the rename-pattern twin with the table's
own three-argument shape (the third dword is `extra`, which these bodies never
read). None of the 22 is called directly anywhere — every use is the table — so
the twin is the only caller of the matched body.

This one is worth flagging to the integrator as a **behaviour** risk as well as
a type risk: these are the keywords a level file uses most (`MESSAGE`, `PLACE`,
`CLEAR`, `FMV`, `INTERVAL`), so a level load would have trapped on the first one
it met.

## 6. Gate results

Every touched file: `audit.py` PASS with the same `[OK]` / `[WIP]` rows,
0 REJECT / FAIL / COMPILE FAILED, `relocs.py` 0 MISMATCH.

| file | `[OK]` | `[WIP]` | audit.py | relocs.py MISMATCH | section |
| --- | --- | --- | --- | --- | --- |
| castleobj.c | 39 | 0 | PASS | 0 | 4b |
| coaster.c | 59 | 0 | PASS | 0 | 3b |
| coaster10.c | 16 | 0 | PASS | 0 | 3a |
| coaster12.c | 22 | 2 | PASS | 0 | 3b |
| fpui5.c | 4 | 1 | PASS | 0 | 4 |
| frontend2.c | 23 | 0 | PASS | 0 | 4 |
| interfaces.c | 15 | 0 | PASS | 0 | 4b |
| levelkw3.c | 22 | 0 | PASS | 0 | 5b |
| mapscreen4.c | 5 | 0 | PASS | 0 | 4 |
| ridesave.c | 27 | 0 | PASS | 0 | 4b |
| screen.c | 3 | 0 | PASS | 0 | 4b |
| tinystubs.c | 56 | 0 | PASS | 0 | 4 |
| uimisc.c | 35 | 0 | PASS | 0 | 4 |
| uimisc2.c | 14 | 0 | PASS | 0 | 4 |
| uimisc3.c | 23 | 0 | PASS | 0 | 4 |
| **15 files** | **363** | **3** | **PASS, 0 REJECT / FAIL / COMPILE FAILED** | **0** | |

`progress.py --check`: **3281 exact / 42 WIP**, unchanged. `docs/LEGOLANDPROGRESS.HTML` was reported
stale for the reason PORT-M1 predicted — the report records a line number per
function and an inserted `#ifndef` moves them — so it is regenerated and
committed; it reports 665/675 exports exact (98.5%), 3281 exact, 42 WIP, and
the diff is the one table line whose source links moved.

**A second, cheaper proof that the VC6 side cannot have moved**
(`port-m3-vc6view.py` in the scratchpad): each file is reduced to what the
preprocessor leaves with `LEGOLAND_PORTABLE` undefined — `#ifndef` arms kept,
`#ifdef` arms and `#else` bodies dropped — and compared with the same reduction
of the file at the lane's base revision. **All 15 files: IDENTICAL.** The byte
gate confirms it the expensive way; this confirms it in a second, and it is the
check to run first when a portable arm is suspected of leaking.

For the record, `clang`'s own function-pointer warnings over all 258 sources
(`-Wincompatible-function-pointer-types -Wcast-function-type`, which the
portable build turns off with `-Wno-everything`) produce exactly four rows. None
of them is one of this lane's traps, but all four are worth keeping in view:

```
coaster8.c:339  seat->attach = RouteSeat_AttachCar;   int (RouteSeat*, CoasterCar*) into void (*)(...)
coaster8.c:340  seat->detach = RouteSeat_DetachCar;   int (RouteSeat*) into void (*)(...)
coaster9.c:102    ((float (*)(float))g_fast_sqrt)(value)     cast from void (*)(void)
coastertiny.c:212 ((float (*)(float))g_fast_rsqrt)(value)    cast from void (*)(void)
```

The first two are return-type-only, so the wasm types do differ (`-> i32` vs
`-> void`); the **call sites** (coaster8.c:326 `seat->occupied(seat)`,
unref1.c:269-299) agree with the bodies, not with the struct, so the slot
declarations are the wrong side and it is a one-line portable arm for whoever
owns coaster8.c next. The last two are section 5's x87 pair.

## 7. Census

| | at lane start (`09f14a5b`) | after |
| --- | --- | --- |
| `linkreport.py` prototype conflicts | 420 | **411** |
| wasm-ld `function signature mismatch` (distinct symbols) | 1 (`printf`, generator-side) | 1, unchanged |
| indirect call sites, game-side | 185 | 185 |
| slots/tables with ONE wasm type | 17 of 30 | **26 of 30** — rows 3, 4, 7, 15, 16, 17, 18, 21 and 23 closed; 14, 25, 28 and 30 left open with a recipe |
| (slot, body) pairs type-checked at compile time | 0 | **454** (`portable/tests/test_callback_types.c`) |
| `ctest` | 8/8 | 8/8 |
| `legoland_headless` under node | stops in `RLEPaintHit` (rlepaint.c:1016) | the same, same trace |

The prototype-conflict census barely moves, and that is expected: it counts
*declaration* disagreements, and this lane's fixes are mostly adapters and
definition-side twins, neither of which is a declaration. The nine rows that did
close are screen.c's 35 K&R declarations collapsing onto the bodies' real
prototypes (the census emits one row per distinct wrong signature, not per
file).

## 8. The type-check test

`portable/tests/test_callback_types.c` is a NEW file;
`portable/cmake/tests.cmake` belongs to PORT-C and was not touched, so the
integrator or PORT-C should add

```cmake
# compile-only: the check IS the compile
add_library(legoland_cbtypes OBJECT "${LL_TESTS_DIR}/test_callback_types.c")
target_compile_options(legoland_cbtypes PRIVATE
                       -Werror=incompatible-function-pointer-types)
```

to wire it into `ninja`. Until then it is run by hand:

```
emcc -c -Wall -Werror=incompatible-function-pointer-types \
     portable/tests/test_callback_types.c -o /tmp/cbtypes.o
```

It gives every slot the function-pointer type its **call site** uses and
declares every body registered into that slot with the signature its
**definition** really has, both printed from the same wasm-signature -> C
mapping (`i32 -> void*`, `f32 -> float`), so an initialiser compiles only when
the two are the same wasm type — exactly the condition for the `call_indirect`
not to trap. 454 (slot, body) pairs over 14 slots and tables, the 90-entry
keyword table and the curve method table included.

Verified to FAIL on a real mismatch (the negative control): changing one
declaration from `(void*, float, void*)` to `(void*, void*, void*)` gives

```
error: incompatible function pointer types initializing 'const ll_fn_g_car_class_vt'
(aka 'void (*const)(void *, float, void *)') with an expression of type
'void (void *, void *, void *)'
```

Each group ends with a comment naming the bodies it could NOT check and why:
either they reach the slot through one of this lane's adapters (file-static, so
it cannot be named from outside) or they are stale names no game TU defines,
where the forwarder's type comes from the registering file's declaration rather
than from a body.

## 9. Regenerating the census

The lane's tooling is five scratchpad scripts, none committed (scratch by
convention), each 30-60 lines over data the build already produces:

| script | what it does |
| --- | --- |
| `port-m3-sigs.py` | `linkreport.wasm_object_sigs` over `build-wasm/.../LEGOLAND/*.o` -> `{name: real wasm signature}` plus every referencing signature, as JSON |
| `port-m3-indirect.py` | `-O0 -g` LLVM IR of all 258 sources -> every indirect call site with its lowered type, the `getelementptr` slot the pointer came from, and the source line |
| `port-m3-stores.py` / `port-m3-datatables.py` | who stores what into which slot (from the sources) and what the closure puts in each `.data` table (from `gen-browser/globals.c`) |
| `port-m3-vc6view.py` | the `LEGOLAND_PORTABLE`-undefined reduction of a file against the same reduction at the lane's base |
| `port-m3-gentest.py` | writes `portable/tests/test_callback_types.c` from the census |

The one sentence to carry forward: **a slot is safe iff the set of real
signatures of the bodies that reach it has exactly one element, and that element
is the call site's type.** Everything in this lane is that sentence applied
thirty times.
