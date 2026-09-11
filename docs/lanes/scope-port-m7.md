# Scope PORT-M7 — the 72 cast forwarders: declaration and slot moved together

> **Status: IN PROGRESS (claimed 2026-09-12 by PORT-M7).** Branch
> `scope/PORT-M7` from `7b1ae15d` (the PORT-M6 merge). PORT-M6 section 4b left
> 72 rows in the generated closure's `## Cast forwarders` table and recommended
> a lane of their own, because "the declaration and the slot type have to move
> in the same commit". They do. **72 → 0.**

## 1. What a cast forwarder is, and why it is a trap and not a wart

`gen_link.py` has to define every symbol the 258 game objects reference and no
game TU defines. Many of those are **stale registration names**: a file that
fills a callback table declares a name at an address whose body the tree
already carries under a different name. The generator bridges the two with a
forwarder, and it takes the forwarder's signature from the **referencing
object** — i.e. from the registering file's `extern` line. When that line
disagrees with the body, the forwarder has to cast:

```c
extern void Bank_TickCustomers(unsigned int);
void Bank_Activate(void) { ((void (*)(void))&Bank_TickCustomers)(); }
```

That cast is not cosmetic. It lowers to a `call_indirect` whose type is not the
target's, so either it traps at run time or — worse, and this is PORT-M2
section 4's finding — binaryen's `directize` pass rewrites the constant-index
`call_indirect` into a **direct** call with the wrong type and the module fails
validation hundreds of functions away from the cause.

PORT-M6 retired the ten rows whose only disagreement was the RETURN type. The
72 that remained were argument-list disagreements, and M6's warning was that
they must not be picked off one at a time:

> Today the forwarder is `() -> void`, which is also how the declaring file
> types the table slot, so the indirect call through the slot *succeeds* and the
> body simply reads a garbage argument. Correct the declaration alone and the
> forwarder becomes `(i32) -> void` while the slot stays `() -> void` — and the
> call that works today starts trapping.

**That warning was right in shape and wrong in fact for 71 of the 72 rows, and
this lane's first job was to establish which.** The ObjDef / RideDef callback
slots are declared `void*` in the registering files (`interfaces.c:86-100`,
`ridesave.c:60-81`), so those files carry **no slot type at all** — the
`call_indirect`'s type comes from the *calling* file's function-pointer
declaration, and those were already the body's shape:

| slot | call site | the type the call site uses | one of our bodies |
| --- | --- | --- | --- |
| `+0xa0` draw | `renderview.c:1221` `def->draw(def->ctx, base)`, declared `SpriteDesc* (*draw)(void*, BPos)` | `(i32, i32) -> i32` | `Shop_GetDrawDesc(ShopElem*, unsigned short)` ✔ |
| `+0xa4` create | `llidb_odf.c:281`, declared `void (*fa4)(LLElem*)` | `(i32) -> void` | `Bank_LoadResources(ShopElem*)` ✔ |
| `+0xa8` sim | `renderview.c:1130` `cls->prerender(cls->ctx)` | `(i32) -> void` | `Bank_TickCustomers(ShopElem*)` ✔ |
| `+0xac` teardown | `sysmisc.c:664` `d->dtor(d->dtor_arg)` | `(i32) -> void` | `TempleSlide_FreeResources(RideElem*)` ✔ |
| `+0x9c` remove | `objmap2.c:1895` `def->remove(obj, bp, ctx)` | `(i32, i32, i32) -> void` | `LegoMedia_Remove(void*, ShopTile, void*)` ✔ |
| `+0x90` update | `eventtick.c:400` and five more | `(i32, i32, i32) -> void` | `Track_Update(RideElem*, int, int)` ✔ |
| `+0xb8` load | `savegame.c:1365` `def->cb_load(def->elem)` | `(i32) -> i32` | `LoadZoomer(RideElem*)` ✔ |
| `+0x8c`, `+0x98` add, `+0xb0` interact | **no call site anywhere in the recovered tree** (PORT-M3 section 2, re-checked — see below) | — | — |

So for 71 rows the **forwarder was the only wrong thing in the chain**: the
slot already wanted what the body already had, and the cast was what broke it.
Correcting the declaration does not start a trap — it ends one. Three of the
slots involved (`+0xa8`, `+0xa4`, `+0xa0`) are reached every frame a class with
`flags & 0x20` or `0x400` is on the map, and `g_icon_handler1/2` is reached by
the first click on the map screen's OK icon.

The "no call site" row is worth re-deriving rather than inherited, and it is
cheap to: **every declaration of `+0x8c`, `+0x98` and `+0xb0` in all 258
sources is `void*`, never a typed function pointer** (`castleobj.c:175/178/184`,
`interfaces.c:86/89/95`, `ridesave.c:67`, `screen.c:551`, `loaders.c:140/149`),
so there is no `call_indirect` type for them to disagree with. For those three
slots this lane makes the forwarder agree with its own body, which is all that
can be decided locally.

**What that leaves open, and it is not this lane's to close.** `+0xb0` and
`+0x98` have genuinely MIXED body sets — `castleobj.c:1538` and
`logflume.c:2332` describe `+0xb0` as a *draw* pass whose bodies are
`(piece, mode)` two-argument, while the fifteen `*_Interact` rows below are
`(elem, x, y, sq, clip, mode)` six-argument. Both shapes are now declared
truthfully, which is strictly better than all of them lying the same way, but
the first consumer that dispatches through `+0xb0` with one fixed type will
trap on the others and needs PORT-M3 section 5's adapter treatment at the
registration sites. Recorded so the next lane does not re-derive it.

**One row genuinely had a live slot type that had to move with it**, and it is
the one M6 predicted: `mapscreen.c:52` declares the icon-handler slot itself

```c
typedef void (*IconHandler)(void);          /* 0x006687bc, 0x006687c0 */
```

while `appraisal.c:68`, `appraisalscreen.c:21`, `screens2.c` and `fpui.c`'s two
call sites all say `char (*)(Icon*, int, int, int)`. Both halves moved in the
same commit.

## 2. Method

1. `grep -n '(\*)' portable/build-wasm/gen-browser/aliases.c` after a **clean**
   wasm build is the work list; the generator's `manifest.md` prints the same 72
   rows with the emitted and the real wasm signature side by side.
2. `scratchpad/port-m7-census.py` — for each `(alias, body)` pair, every textual
   occurrence of the alias in all 258 sources, and the body's definition with
   the `// FUNCTION:` marker that proves the address. Result: **every one of the
   72 aliases has exactly ONE `extern` line and one registration site**, in five
   files. No alias is declared twice, which is why this is a 72-line diff and
   not a tree-wide sweep.
3. The body's definition signature, not the manifest's `i32` spelling, is what
   the new declaration carries (pointers as `void*`, the by-value 2-byte tile
   union as the `unsigned short` it lowers to, `int` kept as `int`).
4. `scratchpad/port-m7-slotsweep.py` — the independent check: every
   `def->cb_XX = Name;` store in the tree that is **live with
   `LEGOLAND_PORTABLE` defined**, against the slot's call-site type, with each
   name's real wasm signature read out of the built objects
   (`linkreport.collect_wasm_sigs`) or out of the generated forwarder. This is
   what found section 5.

## 3. The 72 rows

The edit is the same in all but one case: the stale `extern` gets an
`#ifdef LEGOLAND_PORTABLE` arm stating the body's real signature. VC6 compiles
the `#ifndef` world, in which the declaration region is character-for-character
what it was, so no byte can move — and a cdecl caller that pushes arguments a
callee ignores is what the original did anyway.

```c
#ifndef LEGOLAND_PORTABLE
extern void Bank_Activate(void);           /* 0x00438960 */
#else   /* PORT-M7: westtown2.c void Bank_TickCustomers(ShopElem* elem) */
extern void Bank_Activate(void* elem);   /* 0x00438960 */
#endif
```

`declared in` is the line number at `7b1ae15d`, before this lane's edits.

| # | stale name | addr | slot | declared in | the body at that address | emitted | the body has |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 1 | `Bank_Activate` | 0x00438960 | `cb_activate` | interfaces.c:794 | `void Bank_TickCustomers(ShopElem* elem)` (westtown2.c:356) | `() -> void` | `(i32) -> void` |
| 2 | `Bank_Create` | 0x00438870 | `cb_create` | interfaces.c:791 | `void Bank_LoadResources(ShopElem* elem)` (westtown.c:373) | `() -> void` | `(i32) -> void` |
| 3 | `Bank_Interact` | 0x00438900 | `cb_interact` | interfaces.c:795 | `void Bank_DrawOverlay(ShopElem* elem, int x, int y, ShopTile* sq, void* clip, int mode)` (westtown.c:400) | `() -> void` | `(i32, i32, i32, i32, i32, i32) -> void` |
| 4 | `CastleLevel1_Activate` | 0x00402dc0 | `cb_activate` | interfaces.c:110 | `void CastleLevel1_TickRiders(RideElem* elem)` (goldrush.c:839) | `() -> void` | `(i32) -> void` |
| 5 | `CastleLevel1_Add` | 0x00403060 | `cb_add` | interfaces.c:108 | `void CastleLevel1_Place(void* obj, Pos* pos)` (goldrush.c:413) | `() -> void` | `(i32, i32) -> void` |
| 6 | `CastleLevel1_Interact` | 0x00402d00 | `cb_interact` | interfaces.c:111 | `void CastleLevel1_Draw(RideElem* elem, int x, int y, MapSquare* sq, void* clip, int mode)` (goldrush.c:521) | `() -> void` | `(i32, i32, i32, i32, i32, i32) -> void` |
| 7 | `Catapult_Add` | 0x00403970 | `cb_add` | interfaces.c:355 | `void Catapult_Place(void* obj, Pos* pos)` (catapult.c:473) | `() -> void` | `(i32, i32) -> void` |
| 8 | `Catapult_Draw` | 0x004039e0 | `cb_draw` | interfaces.c:356 | `RideDrawDesc* Catapult_GetDrawDesc(RideElem* elem, unsigned short tile)` (catapult.c:498) | `() -> void` | `(i32, i32) -> i32` |
| 9 | `Copters_Add` | 0x00404600 | `cb_add` | interfaces.c:390 | `void Copters_Place(void* obj, Pos* pos)` (mechrides.c:321) | `() -> void` | `(i32, i32) -> void` |
| 10 | `Copters_Draw` | 0x00404490 | `cb_draw` | interfaces.c:393 | `RideDrawDesc* Copters_GetDrawDesc(RideElem* elem, unsigned short tile)` (mechrides.c:794) | `() -> void` | `(i32, i32) -> i32` |
| 11 | `Fort_Activate` | 0x00406660 | `cb_activate` | interfaces.c:133 | `void Fort_TickRiders(RideElem* elem)` (goldrush.c:977) | `() -> void` | `(i32) -> void` |
| 12 | `Fort_Add` | 0x00406860 | `cb_add` | interfaces.c:136 | `void Fort_Place(void* obj, Pos* pos)` (goldrush.c:419) | `() -> void` | `(i32, i32) -> void` |
| 13 | `Fort_Interact` | 0x004062c0 | `cb_interact` | interfaces.c:134 | `void Fort_Draw(RideElem* elem, int x, int y, MapSquare* sq, void* clip, int mode)` (goldrush.c:604) | `() -> void` | `(i32, i32, i32, i32, i32, i32) -> void` |
| 14 | `GeneralStore_Activate` | 0x004378e0 | `cb_activate` | interfaces.c:772 | `void GeneralStore_TickCustomers(ShopElem* elem)` (westtown2.c:687) | `() -> void` | `(i32) -> void` |
| 15 | `GeneralStore_Create` | 0x004375d0 | `cb_create` | interfaces.c:769 | `void GeneralStore_LoadResources(ShopElem* elem)` (westtown.c:278) | `() -> void` | `(i32) -> void` |
| 16 | `GeneralStore_Interact` | 0x00437670 | `cb_interact` | interfaces.c:773 | `void GeneralStore_DrawOverlay(ShopElem* elem, int x, int y, ShopTile* sq, void* clip, int mode)` (westtown.c:1009) | `() -> void` | `(i32, i32, i32, i32, i32, i32) -> void` |
| 17 | `GoldRush_Activate` | 0x004072b0 | `cb_activate` | interfaces.c:184 | `void GoldRush_TickRiders(RideElem* elem)` (goldrush.c:1266) | `() -> void` | `(i32) -> void` |
| 18 | `GoldRush_Add` | 0x004075f0 | `cb_add` | interfaces.c:186 | `void GoldRush_Place(void* obj, Pos* pos)` (goldrush.c:713) | `() -> void` | `(i32, i32) -> void` |
| 19 | `GoldRush_Interact` | 0x00406b10 | `cb_interact` | interfaces.c:185 | `void GoldRush_Draw(RideElem* elem, int x, int y, MapSquare* sq, void* clip, int mode)` (goldrush.c:1404) | `() -> void` | `(i32, i32, i32, i32, i32, i32) -> void` |
| 20 | `Institute_Activate` | 0x0043a1e0 | `cb_activate` | interfaces.c:806 | `void Explorers_TickCustomers(ShopElem* elem)` (westtown.c:1206) | `() -> void` | `(i32) -> void` |
| 21 | `Institute_Create` | 0x0043a0f0 | `cb_create` | interfaces.c:803 | `void Explorers_LoadResources(ShopElem* elem)` (westtown.c:465) | `() -> void` | `(i32) -> void` |
| 22 | `Institute_Interact` | 0x0043a180 | `cb_interact` | interfaces.c:807 | `void Explorers_DrawOverlay(ShopElem* elem, int x, int y, ShopTile* sq, void* clip, int mode)` (westtown.c:492) | `() -> void` | `(i32, i32, i32, i32, i32, i32) -> void` |
| 23 | `JailCell_Activate` | 0x00438430 | `cb_activate` | interfaces.c:784 | `void JailCell_TickCustomers(ShopElem* elem)` (westtown2.c:1170) | `() -> void` | `(i32) -> void` |
| 24 | `JailCell_Create` | 0x00438070 | `cb_create` | interfaces.c:781 | `void JailCell_LoadResources(ShopElem* elem)` (westtown.c:917) | `() -> void` | `(i32) -> void` |
| 25 | `JailCell_Interact` | 0x00438150 | `cb_interact` | interfaces.c:787 | `void JailCell_DrawOverlay(ShopElem* elem, int x, int y, MapSquare* sq, void* clip, int mode)` (westtown2.c:1435) | `() -> void` | `(i32, i32, i32, i32, i32, i32) -> void` |
| 26 | `Joust_A0` | 0x00408c50 | `cb_a0` | ridesave.c:91 | `RideDrawDesc* Joust_GetDrawDesc(RideElem* elem, unsigned short arg)` (joust.c:207) | `() -> void` | `(i32, i32) -> i32` |
| 27 | `Joust_A4` | 0x00407b50 | `cb_a4` | ridesave.c:84 | `void Joust_LoadResources(RideElem* elem)` (joust.c:652) | `() -> void` | `(i32) -> void` |
| 28 | `Joust_Add` | 0x004079e0 | `cb_add` | ridesave.c:90 | `void Joust_Place(void* obj, Pos* pos)` (joust.c:525) | `() -> void` | `(i32, i32) -> void` |
| 29 | `Joust_B0` | 0x00408580 | `cb_b0` | ridesave.c:88 | `void Joust_Draw(RideElem* elem, int x, int y, RideTile* sq, void* clip, int mode)` (joust.c:2286) | `() -> void` | `(i32, i32, i32, i32, i32, i32) -> void` |
| 30 | `LegoShop1_Activate` | 0x00439460 | `cb_activate` | interfaces.c:814 | `void LegoShop1_TickCustomers(ShopElem* elem)` (westtown2.c:590) | `() -> void` | `(i32) -> void` |
| 31 | `LegoShop1_Create` | 0x00439200 | `cb_create` | interfaces.c:809 | `void LegoShop1_LoadResources(ShopElem* elem)` (westtown.c:594) | `() -> void` | `(i32) -> void` |
| 32 | `LegoShop1_Interact` | 0x00439400 | `cb_interact` | interfaces.c:815 | `void LegoShop1_DrawOverlay(ShopElem* elem, int x, int y, ShopTile* sq, void* clip, int mode)` (westtown.c:639) | `() -> void` | `(i32, i32, i32, i32, i32, i32) -> void` |
| 33 | `LegoShop2_Activate` | 0x00439950 | `cb_activate` | interfaces.c:820 | `void LegoShop2_TickCustomers(ShopElem* elem)` (westtown2.c:849) | `() -> void` | `(i32) -> void` |
| 34 | `LegoShop2_Create` | 0x004396d0 | `cb_create` | interfaces.c:817 | `void LegoShop2_LoadResources(ShopElem* elem)` (westtown.c:667) | `() -> void` | `(i32) -> void` |
| 35 | `LegoShop2_Interact` | 0x00439760 | `cb_interact` | interfaces.c:821 | `void LegoShop2_DrawOverlay(ShopElem* elem, int x, int y, MapSquare* sq, void* clip, int mode)` (westtown2.c:1385) | `() -> void` | `(i32, i32, i32, i32, i32, i32) -> void` |
| 36 | `LoadPlaneRide` | 0x0043e110 | `cb_load` | interfaces.c:579 | `int LoadZoomer(RideElem* elem)` (ridesave.c:787) | `() -> i32` | `(i32) -> i32` |
| 37 | `LoadSpiderRide` | 0x004168f0 | `cb_load` | interfaces.c:471 | `int LoadSpider(RideElem* elem)` (ridesave.c:621) | `() -> i32` | `(i32) -> i32` |
| 38 | `LoadSpinningBarrels` | 0x0043c690 | `cb_load` | interfaces.c:545 | `int LoadSBarrel(RideElem* elem)` (ridesave.c:490) | `() -> i32` | `(i32) -> i32` |
| 39 | `MapScreenIconHandler` | 0x00475080 | `g_icon_handler1/2` | mapscreen.c:106 | `char MapIconInput(Icon* p, int buttons, int a3, int a4)` (screens3.c:680) | `() -> void` | `(i32, i32, i32, i32) -> i32` |
| 40 | `MediaShop_Activate` | 0x00439ef0 | `cb_activate` | interfaces.c:828 | `void LegoMedia_TickCustomers(ShopElem* elem)` (westtown2.c:468) | `() -> void` | `(i32) -> void` |
| 41 | `MediaShop_Add` | 0x00439c60 | `cb_add` | interfaces.c:824 | `void LegoMedia_Add(ShopElem* elem, Pos* at)` (westtown.c:716) | `() -> void` | `(i32, i32) -> void` |
| 42 | `MediaShop_Create` | 0x00439c20 | `cb_create` | interfaces.c:823 | `void LegoMedia_LoadResources(ShopElem* elem)` (westtown.c:704) | `() -> void` | `(i32) -> void` |
| 43 | `MediaShop_Interact` | 0x00439d40 | `cb_interact` | interfaces.c:829 | `void LegoMedia_DrawOverlay(ShopElem* elem, int x, int y, ShopTile* sq, void* clip, int mode)` (westtown.c:1079) | `() -> void` | `(i32, i32, i32, i32, i32, i32) -> void` |
| 44 | `MediaShop_Remove` | 0x00439c90 | `cb_remove` | interfaces.c:825 | `void LegoMedia_Remove(void* obj, ShopTile tile, void* ctx)` (westtown.c:725) | `() -> void` | `(i32, i32, i32) -> void` |
| 45 | `PlaneRide_Add` | 0x0043dfe0 | `cb_add` | interfaces.c:577 | `void PlaneRide_Place(void* obj, Pos* pos)` (mechrides.c:310) | `() -> void` | `(i32, i32) -> void` |
| 46 | `PlaneRide_Draw` | 0x0043e010 | `cb_draw` | interfaces.c:578 | `RideDrawDesc* PlaneRide_GetDrawDesc(RideElem* elem, unsigned short arg)` (mechrides.c:385) | `() -> void` | `(i32, i32) -> i32` |
| 47 | `SafariRide_Add` | 0x00414fc0 | `cb_add` | interfaces.c:433 | `void SafariRide_Place(void* obj, Pos* pos)` (mechrides.c:266) | `() -> void` | `(i32, i32) -> void` |
| 48 | `SafariRide_Draw` | 0x00414ff0 | `cb_draw` | interfaces.c:434 | `RideDrawDesc* SafariRide_GetDrawDesc(RideElem* elem, unsigned short arg)` (mechrides.c:346) | `() -> void` | `(i32, i32) -> i32` |
| 49 | `Saloon_Activate` | 0x00438f10 | `cb_activate` | interfaces.c:800 | `void Saloon_TickCustomers(ShopElem* elem)` (westtown2.c:1011) | `() -> void` | `(i32) -> void` |
| 50 | `Saloon_Create` | 0x00438c60 | `cb_create` | interfaces.c:797 | `void Saloon_LoadResources(ShopElem* elem)` (westtown.c:429) | `() -> void` | `(i32) -> void` |
| 51 | `Saloon_Interact` | 0x00438d00 | `cb_interact` | interfaces.c:801 | `void Saloon_DrawOverlay(ShopElem* elem, int x, int y, ShopTile* sq, void* clip, int mode)` (westtown.c:1045) | `() -> void` | `(i32, i32, i32, i32, i32, i32) -> void` |
| 52 | `Sheriff_Activate` | 0x00437c90 | `cb_activate` | interfaces.c:778 | `void Sheriff_TickCustomers(ShopElem* elem)` (westtown2.c:267) | `() -> void` | `(i32) -> void` |
| 53 | `Sheriff_Create` | 0x00437ba0 | `cb_create` | interfaces.c:775 | `void Sheriff_LoadResources(ShopElem* elem)` (westtown.c:314) | `() -> void` | `(i32) -> void` |
| 54 | `Sheriff_Interact` | 0x00437c30 | `cb_interact` | interfaces.c:779 | `void Sheriff_DrawOverlay(ShopElem* elem, int x, int y, ShopTile* sq, void* clip, int mode)` (westtown.c:345) | `() -> void` | `(i32, i32, i32, i32, i32, i32) -> void` |
| 55 | `Shop_Draw` | 0x0043a390 | `cb_draw` | interfaces.c:766 | `ShopDrawDesc* Shop_GetDrawDesc(ShopElem* elem, unsigned short arg)` (westtown.c:159) | `() -> void` | `(i32, i32) -> i32` |
| 56 | `SpaceTower_Add` | 0x0043b4b0 | `cb_add` | interfaces.c:503 | `void SpaceTower_Place(void* obj, Pos* pos)` (mechrides.c:299) | `() -> void` | `(i32, i32) -> void` |
| 57 | `SpaceTower_Draw` | 0x0043b4e0 | `cb_draw` | interfaces.c:500 | `RideDrawDesc* SpaceTower_GetDrawDesc(RideElem* elem, unsigned short tile)` (mechrides.c:773) | `() -> void` | `(i32, i32) -> i32` |
| 58 | `SpiderRide_Add` | 0x004160f0 | `cb_add` | interfaces.c:468 | `void SpiderRide_Place(void* obj, Pos* pos)` (mechrides.c:277) | `() -> void` | `(i32, i32) -> void` |
| 59 | `SpiderRide_Draw` | 0x00416120 | `cb_draw` | interfaces.c:469 | `RideDrawDesc* SpiderRide_GetDrawDesc(RideElem* elem, unsigned short arg)` (mechrides.c:359) | `() -> void` | `(i32, i32) -> i32` |
| 60 | `SpinningBarrels_Add` | 0x0043c540 | `cb_add` | interfaces.c:541 | `void SpinningBarrels_Place(void* obj, Pos* pos)` (mechrides.c:288) | `() -> void` | `(i32, i32) -> void` |
| 61 | `SpinningBarrels_Draw` | 0x0043c570 | `cb_draw` | interfaces.c:543 | `RideDrawDesc* SpinningBarrels_GetDrawDesc(RideElem* elem, unsigned short arg)` (mechrides.c:372) | `() -> void` | `(i32, i32) -> i32` |
| 62 | `TempleSlide_A0` | 0x00417300 | `cb_a0` | ridesave.c:276 | `RideDrawDesc* TempleSlide_GetDrawDesc(RideElem* elem, unsigned short arg)` (joust.c:220) | `() -> void` | `(i32, i32) -> i32` |
| 63 | `TempleSlide_A4` | 0x00417150 | `cb_a4` | ridesave.c:269 | `void TempleSlide_LoadResources(RideElem* elem)` (joust.c:680) | `() -> void` | `(i32) -> void` |
| 64 | `TempleSlide_A8` | 0x00417430 | `cb_a8` | ridesave.c:272 | `void TempleSlide_Update(RideElem* elem)` (joust.c:1799) | `() -> void` | `(i32) -> void` |
| 65 | `TempleSlide_AC` | 0x00417200 | `cb_ac` | ridesave.c:270 | `void TempleSlide_FreeResources(RideElem* elem)` (joust.c:710) | `() -> void` | `(i32) -> void` |
| 66 | `TempleSlide_Add` | 0x004172d0 | `cb_add` | ridesave.c:275 | `void TempleSlide_Place(void* obj, Pos* pos)` (joust.c:536) | `() -> void` | `(i32, i32) -> void` |
| 67 | `TempleSlide_B0` | 0x00416fa0 | `cb_b0` | ridesave.c:273 | `void TempleSlide_Draw(RideElem* elem, int x, int y, RideTile* sq, void* clip, int mode)` (joust.c:892) | `() -> void` | `(i32, i32, i32, i32, i32, i32) -> void` |
| 68 | `Temple_Activate` | 0x00416b50 | `cb_activate` | interfaces.c:158 | `void Temple_TickRiders(RideElem* elem)` (goldrush.c:1083) | `() -> void` | `(i32) -> void` |
| 69 | `Temple_Add` | 0x00416e00 | `cb_add` | interfaces.c:161 | `void Temple_Place(void* obj, Pos* pos)` (goldrush.c:425) | `() -> void` | `(i32, i32) -> void` |
| 70 | `Temple_Interact` | 0x00416a60 | `cb_interact` | interfaces.c:159 | `void Temple_Draw(RideElem* elem, int x, int y, MapSquare* sq, void* clip, int mode)` (goldrush.c:546) | `() -> void` | `(i32, i32, i32, i32, i32, i32) -> void` |
| 71 | `Track_Update90` | 0x004275d0 | `cb_90` | castleobj.c:664 | `void Track_Update(RideElem* elem, int screen, int mode)` (coaster.c:1790) | `() -> void` | `(i32, i32, i32) -> void` |
| 72 | `sub_457970` | 0x00457970 | `(direct call)` | gameframe.c:909 | `int FootprintClearanceTest(Pos p)` (bubblecache.c:366) | `(i32, i32) -> i32` | `(i32) -> i32` |

## 4. The one row that is a different ABI, not a different arity

`sub_457970` (row 72) is the only one where the original's two prototypes were
not the same call by accident of cdecl — they were **literally the same call**,
and nothing was ever wrong in the original:

| | |
| --- | --- |
| `gameframe.c:909` declares | `extern int sub_457970(int x, int y);  /* 0x00457970 */` |
| `gameframe.c:1082` calls | `if (sub_457970(pos.x, pos.y))` |
| `bubblecache.c:366` defines | `int FootprintClearanceTest(Pos p)`, `Pos` = `{int x; int y}` |

On x86, `(int x, int y)` and `(Pos p)` push the same two dwords in the same
order and the callee reads the same two stack slots. There is no garbage
argument, no ignored return, no shipped bug: the two spellings are one calling
convention. On **wasm32** an 8-byte aggregate is passed INDIRECTLY, so the body
is `(i32 pointer) -> i32` while the stale prototype is `(i32, i32) -> i32`, and
the generated bridge handed a by-value x where a pointer was expected:

```c
unsigned int sub_457970(unsigned int a0, unsigned int a1)
{ return ((unsigned int (*)(unsigned int, unsigned int))&FootprintClearanceTest)(a0, a1); }
```

The portable arm therefore calls the body the way the body is written, through
the same shim shape `gameframe.c:900` already uses for `PopUpInfoSetUp`:

```c
extern int        FootprintClearanceTest(Pos p);                         /* 0x00457970 */
static int ll_footprint_clearance_test(int x, int y)
{
    Pos p;
    p.x = x;
    p.y = y;
    return FootprintClearanceTest(p);
}
#define sub_457970(_x, _y) ll_footprint_clearance_test((_x), (_y))
```

`sub_457970` is now referenced by no object, so the closure stops generating it
at all — the alias disappears rather than being fixed. This is the whole-tree
drag-build path (`g_input.drag_*`, placing a row of objects in one gesture);
before this lane it was a guaranteed fault the first time a player drag-built.

**No other row of the 72 bridges a genuine original disagreement.** Every one
is a name the registering file spelled differently from the defining file, at
an address both agree on, with arguments the body reads normally. Nothing in
this lane reproduces a garbage argument or an ignored return, because there was
none to reproduce.

## 5. Adjacent, found by the same sweep: the +0xac teardown slot in `interfaces.c`

Section 2's sweep, run after the 72 were closed, still reported **35 live
disagreements, all in `interfaces.c`**, all of the class PORT-M3 section 4b
already solved for `screen.c` (20) and `castleobj.c` (1) and never applied here:

* 33 `def->cb_destroy = X;` where `X` is a body that genuinely takes no
  argument (`westtown.c:384 void Bank_FreeResources(void)` is typical) and
  `sysmisc.c:664` calls `+0xac` as `d->dtor(d->dtor_arg)`;
* 2 `def->cb_activate = X;` (`Shower_Activate`, `ElephantFountain_Activate`,
  `waterworks.c`) against `renderview.c:1130`'s `cls->prerender(cls->ctx)`.

These cannot be fixed by a declaration edit — the bodies really are
parameterless — so each gets M3's registration-site adapter of the slot's own
type, which drops the argument x86 cdecl dropped for free. 35 adapters, 35
guarded store sites, one file, no byte moves. This closes the class: **after
this lane no `def->cb_* = ...` store anywhere in the 258 sources disagrees with
its slot's call-site wasm type** (`port-m7-slotsweep.py` reports 0).

Classes affected: the whole western town (General Store, Sheriff, Jail Cell,
Bank, Saloon, Explorers Institute, Lego Shop 1/2, Media Shop), the garden
(Hedge, Flowers), water works (Shower, Elephant Fountain, Water Block, WW
Entrance) and all eleven log-flume pieces, plus Castle Level 1, Fort, Temple,
Gold Rush, Catapult, Copters, Space Tower, Spinning Barrels. Every one of those
trapped the module on teardown before this lane.

## 6. The compile-time gate

`portable/tests/test_callback_types.c` gained an append-only PORT-M7 section:
**ten tables, 142 entries**, each table holding the ALIAS *and* the body that
address really holds in the slot's own call-site type, so the initialiser
compiles only when alias, body and slot are all the same wasm type. The 35
adapters of section 5 are file-local statics and so are listed in a comment, as
M3 does. `legoland_cbtypes` builds on both targets
(`-Werror=incompatible-function-pointer-types`).

PORT-M3's generated tables are untouched; the lane's section is appended after
`ll_callback_type_pairs()` and counted by `ll_m7_callback_type_pairs()`.

## 7. Gate results

Every row measured on the committed tree, `LEGOLAND_CL` = the wibo VC6 `cl`.
`relocs.py` exits 2 on a clean file (bit 1 = "unresolved or skipped"), so the
column is a `grep -c MISMATCH`, per HANDOFF §4.

| file | rows | what changed | `audit.py` | `relocs.py` MISMATCH |
| --- | --- | --- | --- | --- |
| `interfaces.c` | 59 + 35 | 59 stale declarations given the body's real signature; 35 `+0xac`/`+0xa8` adapters (section 5) | 15 `[OK]`, 0 `[WIP]` | 0 |
| `ridesave.c` | 10 | the Joust and Temple Slide declarations | 27 `[OK]`, 0 `[WIP]` | 0 |
| `mapscreen.c` | 1 | `MapScreenIconHandler` **and** the `IconHandler` slot typedef | 12 `[OK]`, 0 `[WIP]` | 0 |
| `castleobj.c` | 1 | `Track_Update90` | 39 `[OK]`, 0 `[WIP]` | 0 |
| `gameframe.c` | 1 | `sub_457970` → `FootprintClearanceTest(Pos)` (section 4) | 10 `[OK]`, 0 `[WIP]` | 0 |

Every diff in `LEGOLAND/*.c` is **pure insertion** — `git diff --numstat` shows
0 deletions in all five files, so the `#ifndef LEGOLAND_PORTABLE` world VC6
compiles is character-for-character what it was, and no `// FUNCTION:` marker
line is touched.

Tree-wide:

| gate | result |
| --- | --- |
| `progress.py --check` | **3281 exact / 42 WIP**, unchanged; `665/675 exports exact (98.5%)`. `--check` reported `docs/LEGOLANDPROGRESS.HTML` stale because the insertions moved line numbers, so the report is regenerated and committed; `--check` then exits clean |
| `grep -c '(\*)' gen-browser/aliases.c`, clean wasm build | **72 → 0** |
| `port-m7-slotsweep.py` (every live `def->cb_* =` store vs its slot's call-site type) | 35 → **0** |
| wasm `ctest` | 16/16 |
| native `ctest` | 10/10 |
| `legoland_headless -nointro`, 90 s alarm | reaches the game loop (exits 142 = the alarm, i.e. still running), 2639 host-trace lines, **no trap / RuntimeError / indirect-call line** |
| the same harness **on a pre-lane build** | `7b1ae15d`'s five sources checked back out, built clean into `portable/build-wasm-base` (72 casts, as expected), run with the same environment: **2639 lines, byte-identical to the lane's trace** (`diff -q`). So the lane removed 72 latent traps and changed nothing the front-end path observes. Batch-by-batch traces are identical too |
| `headless_spine` ctest | PASS — this is the pinned regression gate (PORT-A4 pins the title-screen first present: 98% non-black, checksum `0x4a092b01`), so a changed render path would fail here, not only in the trace |
| `legoland_cbtypes` (`-Werror=incompatible-function-pointer-types`) | builds on both targets |
| both builds from a CLEAN directory | wasm and native reconfigured and relinked from scratch at each batch |

## 8. What the integrator must know

1. **`grep -c '(\*)' portable/build-wasm/gen-browser/aliases.c` is 0** on a
   clean build, and the generator's own census line now reads
   `- cast forwarders (latent indirect-call type mismatch): 0`. The
   `## Cast forwarders: a stale name typed unlike its body` **table has
   disappeared** from `gen-browser/manifest.md` (the generator emits the heading
   only when the table is non-empty) — that heading vanishing is the lane's
   deliverable, not a generator regression. The census line is the gate to
   quote; the heading is not.
2. **`sub_457970` is gone from the closure** — the alias is no longer
   referenced by any object, so `gen_link.py` stops defining it:
   `function aliases (stale extern names)` is **262**, one fewer than before.
3. The only file touched under `portable/` is `portable/tests/test_callback_types.c`,
   append-only, as the brief directs. `portable/tools/**` is PORT-A7's and was
   not touched.
4. Section 5's 35 adapters are adjacent work, in a separate commit, and can be
   dropped on their own if the integrator would rather they came from a PORT-M
   lane of their own — but they are live traps today and they are M3's own
   recipe, not a new mechanism.
5. Two declarations elsewhere still name the icon-input slot with two arguments
   rather than four (`bigscreens.c:57 typedef char (*IconInputFn)(Icon*, int);`
   used only for `g_icon_handler1/2` assignments). The bodies stored through it
   are already four-argument after PORT-M3, and no call site reads the typedef,
   so nothing traps — but it is the last stale spelling of that slot type in
   the tree and a future lane should make it agree with `appraisal.c:68`.
