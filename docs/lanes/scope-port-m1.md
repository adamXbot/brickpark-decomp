# Scope PORT-M1 — the prototype conflicts, and the spine to the title screen

> **Status: the three proved-live conflicts closed, 130 wasm-ld signature
> mismatches down to 19, census 542 -> 437 (2026-09-11).** Branch
> `scope/PORT-M1` from main `b28e35cb`. The first lane that edits
> `LEGOLAND/*.c` for the port, so every change is an `#ifdef
> LEGOLAND_PORTABLE` arm and every touched file is gated: **audit.py PASS on
> all 156 files, 0 REJECT/FAIL/COMPILE FAILED, relocs.py 0 MISMATCH,
> progress.py 3281 exact / 42 WIP unchanged.**

## 1. What a prototype conflict actually is, and how to see one

On x86 cdecl a declaration that disagrees with the definition about the return
type costs nothing (the caller ignores EAX) and a declaration with the wrong
argument count costs nothing either (the caller cleans its own stack). The
original's translation units disagreed about 542 signatures and the shipped
build was fine.

On wasm a declaration **is** a function type. wasm-ld resolves a call made
through the wrong type to a stub named `signature_mismatch:<name>` whose body is
`unreachable`, and at link-time `-O2` binaryen inlines that `unreachable` into
the caller — so the failure arrives as a bare `RuntimeError: unreachable`
attributed to whatever function made the call, with no name and no `TRAP` line.

**The technique that names the frame** (PORT-A2's, confirmed and used all
round): relink the same objects at link-time `-O0`, which leaves the stub as a
real function.

```
emcc -O0 -g2 -sNODERAWFS=1 -sALLOW_MEMORY_GROWTH=1 -sINITIAL_MEMORY=134217728 \
     -sSTACK_SIZE=8388608 -sEXIT_RUNTIME=1 -sASSERTIONS=1 --profiling-funcs \
     CMakeFiles/legoland_headless.dir/src/headless/main.c.o \
     CMakeFiles/legoland_headless.dir/src/headless/node_shim.c.o \
     liblegoland_hostwin.a -Wl,--whole-archive liblegoland_core.a \
     liblegoland_gen_browser.a -Wl,--no-whole-archive -o headless_O0.js
```

```
RuntimeError: unreachable
    at headless_O0.wasm.signature_mismatch:InitHostSystemGPU
    at headless_O0.wasm.InitSession
    at headless_O0.wasm.GameMain
    at headless_O0.wasm.WinMain
```

That one command turns every remaining conflict into a named frame. It costs
about 40 seconds and is the single most useful thing in this lane.

The census (`portable/tools/linkreport.py`, section "Prototype conflicts") is
the complete list; the wasm-ld warnings are the subset that survives
dead-code elimination. **They are not the same set and both matter**: the census
includes conflicts in code nothing reaches yet, and it emits one row per
distinct wrong signature, which the wasm-ld warning does not (the warning names
only the first conflicting object).

## 2. How far the game runs now

Measured with `legoland_headless` under node (`LL_HOST_TRACE=1
LL_CD_DIR=$PWD/gamedata/disc LL_DATA_DIR=$PWD/gamedata/main`). Before this lane
the spine died in `__BMPLoader`; it now runs 70 host calls deep and reaches the
game's own title screen draw.

| after | furthest point | why it stopped |
| --- | --- | --- |
| PORT-A2 (lane start) | `LoadSprite("erase it.lls")` | `signature_mismatch:RES_CloseFile` inlined into `__BMPLoader` |
| the three named conflicts | `InitSession` | `signature_mismatch:InitHostSystemGPU` |
| the 81 return-type conflicts | **`ShowTitleScreen` -> `PrintSprite` -> `RenderSprite` -> `SoftBlitSprite` -> `SoftBlitRLEPlain` -> `RLEPaintHit`** | `LEGOLAND/rlepaint.c:1016: RLEPaintHit() is an inline-asm body that has not been ported yet` |
| the arity and alias batches | the same point, unchanged | the same |

The host-call trace to that point: version block, `CreateMutexA`,
`WaitForSingleObject`, `DirectDrawCreate`, `GetLogicalDrives` +
`GetDriveTypeA("C:\")=3` / `("D:\")=5`, `GetVolumeInformationA("D:\")` =
`LEGOLAND on CDFS`, then the three volumes opened and their directories read
(`Legoland.res` 24,426 bytes of directory at 16,399,660; `Graphics2.res` 31,523
at 120,326,388; `Graphics1.res` 23,598 at 19,939,176), `RegisterClassExA`, four
`CreateFontIndirectA`, `CreateWindowExA("LEGOLAND") 640x480`, `SetDisplayMode
640x480 16bpp (RGB565)`, three `CreateSurface` (primary + back + offscreen), the
two DirectInput devices, then the sprite loads and the first blit.

**The next blocker is NOT a prototype conflict and is not this lane's.**
`RLEPaintHit` (`LEGOLAND/rlepaint.c:1016`) is one of the 26 inline-asm bodies
still standing on `LL_UNPORTED_ASM()`, and it is the inner loop of the RLE
sprite blitter — i.e. the thing that draws every sprite in the game. It is the
single highest-value item left in `portable/`: nothing can be seen on the canvas
until the RLE blitters have C fallbacks. `SoftBlitRLEPlain` (softblit.c) and
`SoftBlitSprite` are its callers, so the porting lane should start there.

**The browser page reaches exactly the same point**, which is the useful part:
`http://localhost:8793/legoland.html?trace=1` (serve `portable/build-wasm` with
`python3 -m http.server 8793`) replays the whole trace above and then loads the
title screen's own sprites out of `Graphics1.res` — seven small ones and then a
**343,366-byte** one, which is the title artwork — before the same
`rlepaint.c:1016` abort. So the page is now one ported blitter away from putting
a picture on the canvas.

`legoland_tests` is **5/5, 193 checks** under node, `loadpos` included. PORT-A2
left it at 4/5 with `loadpos` trapping in `LoadPos`; that trap was
`signature_mismatch:RES_CloseFile`, so it closed with deliverable 1 and needed no
test change. The native 64-bit build and `legoland_linkcheck` still build and
link.

`--stages`, which walks InitSession's own sequence (startup.c 0x0047f880),
now runs to the end with no failure at all: all three volumes, `LoadStrings`,
`InitHostSystemGPU` = 1, `InitScreen` = 1, `InitInputSystem` = 1,
`RES_OpenFile(".\graphics\erase it.lls")` = 1402 bytes, **`LoadSprite("erase
it.lls")` returns a sprite**, `LLIDB_LoadICM()`, `LLIDB_RegisterNewElement("BUILD
MENU")`. The loader that PORT-A2 left broken is fully working.

## 3. The three proved-live conflicts (deliverable 1)

| symbol | definition | wrong declaration | evidence that the definition is right |
| --- | --- | --- | --- |
| `RES_CloseFile` | `int RES_CloseFile(RFile*)` sweep4.c 0x00489de0 | `void RES_CloseFile(void*)` in 13 files | the body ends `xor eax, eax` / `pop esi` / `ret` and takes an `or eax, 0xffffffff` exit for a null argument — it returns 0 or -1 |
| `RES_CloseVolume` | `int RES_CloseVolume(RVol*)` sweep4.c 0x00489dc0 | `void RES_CloseVolume(void*)` startup.c | `mov ecx,[eax+0x24]` / `dec ecx` / `mov [eax+0x24],ecx` / `mov eax,ecx` / `ret` — it returns the decremented refcount |
| `DBPrintf` | `void DBPrintf(void)` sweep1.c 0x00453a20 | variadic in 36 files, eight of them `int` | the address is a single `ret`: the shipped build compiled the debug printf away, so the bytes say nothing about the parameters. 36 translation units declare it variadic and call it with format strings, which is what the source had; an empty cdecl body emits `ret` with or without parameters, so the two shapes are byte-identical |

The 13 `RES_CloseFile` files: data3.c, listdel.c, llidb_load.c, llidb_odf.c,
loaders.c, loadmap.c, mantex.c, movie3.c, music.c, objdesc.c, person3d.c,
screen.c, texture.c.

`DBPrintf` is the one that needs a DEFINITION of a different shape, so sweep1.c
uses the rename pattern: `#define DBPrintf DBPrintf_vc6_body` under
`LEGOLAND_PORTABLE` above the marker, the marker and its signature untouched,
then `#undef` plus `void DBPrintf(const char* fmt, ...) { (void)fmt; }` after
the body. logflume2.c's non-variadic `void DBPrintf(const char*)` needed an arm
of its own; the eight `int` declarations got one too.

## 4. The return-type-only conflicts (81 symbols, 92 files)

Same defect, no judgement needed beyond "the definition has the body". Each
declaration gets a portable arm with the definition's return type, its own
parameter text kept verbatim, and the `/* 0x... */` comment on both arms.

Two were not declaration-side fixes:

* **`SkipMeasuredBlock`** (0x0047d7e0) ends `call SaveGameRead` / `add esp,0xc` /
  `ret`, so EAX leaves it holding SaveGameRead's result. savegame.c's thirteen
  `if (!SkipMeasuredBlock())` tests read a real value, and tinystubs.c's `void`
  — byte-identical, because nothing touches EAX after the call — is the wrong
  side. Rename pattern, with an `int` twin that returns `SaveGameRead(&length, 4)`.
* **`LLIDB_UnLoadTSMData`** really does fall off its end (savemisc2.c says so and
  spells it `void`), and memdb.c's `LLIDB_UnLoadData` does `return
  LLIDB_UnLoadTSMData(e)` — propagating whatever EAX holds, which it can because
  it also falls off its end. The portable arm of that switch case drops the
  value rather than inventing one.

The full table, generated from the census before the sweep:

| symbol | definition | wrong declaration | declared in |
| --- | --- | --- | --- |
| `AddHelpMessage` | `(i32, i32) -> i32` in sysstubs.c | `(i32, i32) -> void` | eventmake.c, softblit.c |
| `AddLevelFlag` | `(i32, i32) -> i32` in eventgoalprim.c | `(i32, i32) -> void` | eventtick.c, levelkw2.c |
| `AddPTPOpenNode` | `(i32, i32, i32) -> void` in workorder4.c | `(i32, i32, i32) -> i32` | bnvmove.c |
| `AddPTPRouteNode` | `(i32, i32) -> i32` in pathmisc.c | `(i32, i32) -> void` | workorder3.c |
| `AddTrackSquareCount` | `(i32) -> i32` in schoolcar.c | `(i32) -> void` | coaster.c |
| `CheckFocussedIcon` | `() -> i32` in fpui.c | `() -> void` | gamemain.c, mapscreen.c |
| `CheckWorkerOnMouseStatus` | `(i32) -> void` in workers2.c | `(i32) -> i32` | uimisc3.c |
| `ClampPopUpToScreen` | `(i32) -> i32` in misc3.c | `(i32) -> void` | popup.c |
| `ClearSampleSource` | `(i32) -> i32` in audio5.c | `(i32) -> void` | audio3.c |
| `ClosePrimaryPopUp` | `() -> i32` in movie.c | `() -> void` | tinystubs.c |
| `Coaster3D_BuildTrackMesh` | `(i32, i32, i32, i32, i32) -> i32` in schoolcar3.c | `(i32, i32, i32, i32, i32) -> void` | schoolcar.c |
| `Coaster3D_DrawMesh` | `(i32) -> i32` in schoolcar3.c | `(i32) -> void` | schoolcar.c |
| `CoasterModel_SetDirectory` | `(i32) -> i32` in coastertiny.c | `(i32) -> void` | schoolcar.c |
| `Copters_StepRider` | `(i32) -> i32` in ridetiny.c | `(i32) -> void` | mechrides.c |
| `FreePlayItemUpdate` | `(i32, i32) -> i32` in fpui5.c | `(i32, i32) -> void` | fpui.c |
| `GenerateGardener` | `(i32, i32) -> i32` in workers2.c | `(i32, i32) -> void` | uimisc.c |
| `GenerateMechanic` | `(i32, i32) -> i32` in workers2.c | `(i32, i32) -> void` | uimisc.c |
| `InitHostSystemGPU` | `() -> i32` in gpu.c | `() -> void` | startup.c |
| `InitSoundSystem` | `() -> i32` in lifecycle.c | `() -> void` | gamemain.c |
| `JungleCruise_ProbeRiver` | `(i32, i32, i32) -> i32` in junglecruise.c | `(i32, i32, i32) -> void` | screencb5.c |
| `KLIBAUDIO_UnLockAVISoundBuffer` | `(i32) -> void` in audio2.c | `(i32) -> i32` | movie2.c |
| `KillImage` | `(i32) -> i32` in sprite2.c | `(i32) -> void` | data3.c, savemisc2.c, saveprof.c, spritemisc.c |
| `KillSoundSystem` | `() -> i32` in audiomisc.c | `() -> void` | gamemain.c |
| `LLIDB_CloseICM` | `() -> i32` in memdb.c | `() -> void` | startup.c |
| `LLIDB_GetElement` | `(i32, i32) -> i32` in llidb.c | `(i32, i32) -> void` | fpui2.c, gamemain.c, ridemisc.c, savegame.c, unref4.c, unref6.c |
| `LLIDB_LoadDataByIndex` | `(i32) -> void` in sysmisc3.c | `(i32) -> i32` | memdb.c |
| `LLIDB_LoadICM` | `() -> i32` in data2.c | `() -> void` | startup.c |
| `LLIDB_RegisterNewElement` | `(i32, i32, i32) -> i32` in llidb.c | `(i32, i32, i32) -> void` | startup.c |
| `LLIDB_UnLoadData` | `(i32) -> i32` in memdb.c | `(i32) -> void` | castleobj.c, logflume.c, profiles.c, ridecb8.c, screencb2.c, waterworks.c |
| `LLIDB_UnLoadTSMData` | `(i32) -> void` in savemisc2.c | `(i32) -> i32` | memdb.c |
| `LLSStop` | `(i32) -> i32` in layervis.c | `(i32) -> void` | goldrush.c, joust2.c, llidb_odf.c, logflume.c, screencb.c, screencb2.c +2 |
| `LoadCoasterModelSet` | `(i32) -> i32` in schoolcar4.c | `(i32) -> void` | schoolcar.c |
| `LoadMapTiles` | `() -> i32` in pathtile2.c | `() -> void` | gamemain.c |
| `LoadSavedGamesList` | `(i32) -> i32` in profiles.c | `(i32) -> void` | bigscreens.c |
| `MarkObjectTiles` | `(i32) -> i32` in pathmisc.c | `(i32) -> void` | mapobj.c |
| `Mat4_Transpose` | `(i32, i32) -> i32` in coaster12.c | `(i32, i32) -> void` | coaster10.c |
| `MemScratchInit` | `() -> i32` in schoolcar.c | `() -> void` | coaster.c |
| `ModelRecord_GetName` | `(i32, i32, i32) -> i32` in coaster8.c | `(i32, i32, i32) -> void` | schoolcar8.c, unref2.c |
| `PTPVisitTile` | `(i32, i32, i32) -> void` in workorder4.c | `(i32, i32, i32) -> i32` | workorder2.c |
| `PauseCurrentTrack` | `() -> i32` in audio4.c | `() -> void` | appraisal.c, appraisalscreen.c, fpui5.c, gamemain.c, goalstate.c, uimisc2.c |
| `PlayInstanceOfSample` | `(i32, i32, i32, i32) -> i32` in audio3.c | `(i32, i32, i32, i32) -> void` | appraisal.c, fpui2.c, fpui3.c, fpui4.c, gameframe.c, mapscreen4.c +9 |
| `PlayMovie` | `(i32, i32, i32) -> i32` in uimisc2.c | `(i32, i32, i32) -> void` | eventtick.c, gameframe.c, gamemain.c, uimisc.c, uimisc3.c |
| `PlayNarrationFile` | `(i32) -> i32` in audio4.c | `(i32) -> void` | appraisalscreen.c, fpui5.c, uimisc2.c |
| `PrimeMovieAudio` | `(i32) -> i32` in movie2.c | `(i32) -> void` | movie.c |
| `PrintSprite` | `(i32, i32, i32, i32, i32) -> i32` in printlist.c | `(i32, i32, i32, i32, i32) -> void` | logflume2.c |
| `PrintSpriteXY` | `(i32, i32, i32) -> void` in tinystubs.c | `(i32, i32, i32) -> i32` | render4.c |
| `RES_SetFilePointer` | `(i32, i32) -> i32` in memdb.c | `(i32, i32) -> void` | loadmap.c |
| `Raster_SaveState` | `(i32) -> i32` in coaster8.c | `(i32) -> void` | schoolcar3.c |
| `ReferenceSprite` | `(i32) -> i32` in spritemisc.c | `(i32) -> void` | iconui.c, screens3.c, unref6.c |
| `RegisterDetailImage` | `(i32) -> i32` in tinystubs.c | `(i32) -> void` | sprite2.c |
| `ReleaseSprite` | `(i32) -> i32` in spritemisc.c | `(i32) -> void` | popup.c |
| `RemoveProfile` | `(i32) -> i32` in profiles.c | `(i32) -> void` | screens3.c |
| `RenderBlock` | `(i32, i32, i32, i32, i32) -> i32` in gpu.c | `(i32, i32, i32, i32, i32) -> void` | bighelp.c, bubblecache.c, fpui2.c, popup.c, renderview.c |
| `RenderSpriteScaledOffset` | `(i32, i32, i32, i32, i32, i32) -> i32` in bigrender.c | `(i32, i32, i32, i32, i32, i32) -> void` | renderlist.c |
| `RepairCellTick` | `(i32, i32) -> i32` in workorder2.c | `(i32, i32) -> void` | bigsim.c, workers2.c |
| `RestoreScriptStepHelp` | `() -> i32` in sysstubs.c | `() -> void` | eventtick.c, savegame.c, uimisc.c |
| `ResumeCurrentTrack` | `() -> i32` in uimisc3.c | `() -> void` | appraisalscreen.c, fpui5.c, uimisc2.c |
| `RewindNarrationBuffer` | `() -> i32` in movie.c | `() -> void` | uimisc3.c |
| `RouteSeat_AttachCar` | `(i32, i32) -> i32` in coastertiny.c | `(i32, i32) -> void` | coaster8.c, schoolcar8.c |
| `RouteSeat_DetachCar` | `(i32) -> i32` in coastertiny.c | `(i32) -> void` | coaster8.c, schoolcar8.c |
| `RouteSystemInit` | `() -> i32` in schoolcar.c | `() -> void` | coaster.c |
| `SaveProfileToDisk` | `() -> i32` in profiles.c | `() -> void` | screens3.c |
| `ScreenToMapRef` | `(i32, i32, i32) -> i32` in objmap2.c | `(i32, i32, i32) -> void` | bighelp.c, castleobj.c, coaster.c, coaster3d.c, logflume.c, logflume9.c +15 |
| `SetInfoPanelText` | `(i32, i32) -> i32` in movie.c | `(i32, i32) -> void` | gameframe.c, screens3.c, uimisc2.c |
| `SetPointer` | `(i32) -> i32` in sweep2.c | `(i32) -> void` | appraisalscreen.c, gameframe.c, gamemain.c, uimisc.c, uimisc3.c |
| `SetSampleFade` | `(i32, i32) -> i32` in audio2.c | `(i32, i32) -> void` | eventtick.c |
| `SetupControllers` | `() -> i32` in input2.c | `() -> void` | gamemain.c |
| `ShowMessage` | `(i32) -> i32` in workorder2.c | `(i32) -> void` | bigsim.c, workers2.c |
| `SpaceTower_PlaceCar` | `(i32, i32) -> i32` in bswater3.c | `(i32, i32) -> void` | bswater2.c |
| `StopMovieAudio` | `() -> i32` in movie2.c | `() -> void` | movie.c |
| `StopNarrationPlayback` | `() -> i32` in savemisc2.c | `() -> void` | audio4.c |
| `StoreNewSaveGameToDisk` | `() -> i32` in profiles.c | `() -> void` | screens3.c |
| `UnInitModelTextures` | `() -> i32` in savemisc2.c | `() -> void` | rin.c |
| `UnlinkScriptStep` | `(i32) -> i32` in uimisc3.c | `(i32) -> void` | fpui3.c |
| `UnregisterDetailImage` | `(i32) -> i32` in savemisc2.c | `(i32) -> void` | sprite2.c |
| `UpDateCurrentProfile` | `() -> i32` in profiles.c | `() -> void` | bigscreens.c, eventgoalprim.c, gameframe.c, screens3.c, unref7.c |
| `UpDateCurrentSaveSlotInfo` | `() -> i32` in profiles.c | `() -> void` | screens3.c |
| `UpdateHelpIconForText` | `(i32) -> i32` in screens3.c | `(i32) -> void` | bigscreens.c |
| `UpdateMovieAudio` | `(i32, i32) -> i32` in movie2.c | `(i32, i32) -> void` | movie.c |
| `UpdateSampleSource` | `(i32) -> i32` in sysmisc.c | `(i32) -> void` | audio2.c, audio3.c, narration2.c |
| `UpdateSoundVols` | `() -> i32` in audio3.c | `() -> void` | gameframe.c, musicthread.c, uimisc.c, uimisc2.c |

## 5. The arity conflicts (21 symbols)

Twenty of the 31 arity conflicts are "the body never reads the last
argument(s)". The exported symbol has to have the CALLERS' arity on wasm, and
the definition has to keep the text VC6 compiles, so these use the rename
pattern with a forwarder: the defining file renames the matched body, and a
forwarder under the real name takes what the callers pass, discards the extra
arguments and calls the body.

**A forwarder, not a macro, on purpose.** `gen_link.py --ilp32` re-points
pointer words in the rebuilt `.data` at these symbols' addresses, so the
exported name has to stay a real function whose address a table can hold.

| symbol | definition | callers pass | defining file |
| --- | --- | --- | --- |
| `ApplyConsTileMap` | `(void)` | 2 | sweep2.c |
| `ApplyDestrTileMap` | `(void)` | 2 | sweep2.c |
| `PrintBackground` | `(void)` | 2 | sweep2.c |
| `Castle_StartCoasterIfComplete` | `(void)` | 1 | schoolcar.c |
| `Coaster3D_EndFrame` | `(void)` | 1 | schoolcar.c |
| `Dino_InitSound` | `(void)` | 1 | screencb7.c |
| `Fountain_InitSound` | `(void)` | 1 | ridetiny.c |
| `PowerStation_InitSound` | `(void)` | 1 | ridetiny.c |
| `InitMusicSystem` | `(void)` -> int | 1 | sysstubs.c |
| `LoadAppraisalScreenSprites` | `(void)` | 1 | appraisal.c |
| `MakeBloke` | `(void)` -> `Bloke*` | 1 | blokeai.c |
| `MarkWorkersOnMap` | `(void)` | 1 | workorder4.c |
| `Raster_RestoreState` | `(void)` | 1 | coastertiny.c |
| `SetBridges` | `(int which)` | 2 | eventgoalprim.c |
| `PhysObj_Init` | `(PhysObj*)` | 2 | coastertiny.c |
| `KLIBAUDIO_SetAVIVolume` | `(IDSBuffer*)` -> long | 2 | audiomisc.c |
| `DisplayAdvisorHelp` | `(const char*, int)` -> int | 3 | iconui.c |
| `ScreenToMapRef2` | `(Pos*, Pos*)` -> int | 3 | pathtile2.c |
| `PU_NextInput` | `(Icon*, int)` -> char | 4 | uimisc.c |
| `DrawPathTileOverlay` | `(Pos*, int, int)` | 4 | render5.c |

`ScreenToMapRef2` is the clearest of them: ridecb2.c's declaration already names
the extra argument `unused`.

**`AddBasicObject` is the other direction and gets the other treatment.**
objmap2.c defines it with three parameters and annotates the third as "unused
third handler arg (its slot homes bp)" — it is the handler-table shape, and the
address appears in the rebuilt `.data`, so the exported symbol must keep all
three. Its 21 caller files get a portable arm declaring the three-parameter form
plus a same-named function-like macro that supplies the missing `0`:

```c
#ifndef LEGOLAND_PORTABLE
extern void  AddBasicObject(void* obj, Pos* pos);               /* 0x0045efe0 */
#else
extern void AddBasicObject(void* ll_obj, void* ll_pos, void* ll_ctx); /* 0x0045efe0 */
#define AddBasicObject(_a1, _a2) AddBasicObject((_a1), (_a2), 0)
#endif
```

The macro cannot recurse: the standard forbids re-expanding a macro name inside
its own expansion, so the inner `AddBasicObject(...)` is the function.

## 6. Conflicts against the generated closure and against libc

Five of the wasm-ld warnings never appear in the census, because the census only
compares game sources with each other. These are a game declaration against
`liblegoland_gen.a(aliases.c.o)` or against emscripten's libc.

| symbol | what it is | verdict |
| --- | --- | --- |
| `KillSprite` | a stale name for 0x00497bd0 = `UnreferenceSprite` (spritemisc.c), which "returns 1 only when it actually destroyed the resource" | 28 files declared it `void`, 10 `int`; the `int` ones are right, swept |
| `Format` | a stale name for 0x0049e573 = the CRT's `sprintf` | three files declared it `void`; `int` is right, swept |
| `HeapFree_w` | 0x0049e4d0 = the CRT's `free`, which returns nothing | three files declared it `int` — audio3.c documents that as the lever that keeps its callers' discarded tail call a `call`+`pop ecx` rather than a `jmp`. The portable arm is `void`, and `FreePlayableSample`'s `return HeapFree_w(s)` drops the value |
| `time` | render5.c declares `long time(long*)`; **emscripten's `time_t` is 64-bit** | the portable arm is `long long time(long long*)` with the local in `SaveCertificateBitmap` widened to match. This was not only a link warning: the host's `time` writes eight bytes through that pointer, i.e. four bytes past a `long` local |
| `printf` | the generated `aliases.c` emits it as `(i32) -> void`; every game declaration is `int printf(const char*, ...)` and libc agrees | **a `gen_link.py` defect, not a game one** — nothing in `LEGOLAND/*.c` can fix it. For PORT-A: the alias/forwarder for a CRT name must take its signature from the CRT, not from a source scan |

## 7. What is left: the 19 remaining warnings, with the recipe for each

All 19 need a call-site change, not just a prototype, which is why they are not
in this lane's batches. Three distinct kinds:

### (a) struct-by-value against flattened ints (6 symbols)

The original passed a small struct by value, which on x86 is just its words
pushed in order — so a declaration taking `(Pos from, Pos to, void* path)` and
one taking `(int fx, int fy, int tx, int ty, void* path)` push identical bytes
and both were correct. **On wasm32 a by-value struct is passed as a POINTER to a
copy**, so the two are genuinely different ABIs and no prototype edit can bridge
them: the minority side's call sites have to build or unpack the struct.

| symbol | definition | the odd files | note |
| --- | --- | --- | --- |
| `CalcMoveLine` | `(Pos from, Pos to, MoveLine*)` bnvmove.c | ridecb2.c, ridecb5.c | 26 other files agree with the definition; only these two flattened it |
| `AddRepairOrderForObject` | `(WClass*, Pos pos)` workers2.c | scrolltick.c, workorder2.c | both declare `(cls, int x, int y)` |
| `JungleCruise_TraceRoute` | `(JcRoutePos here, JcRoutePos target, BPosW*, int*)` jcroute.c | junglecruise.c | two structs flattened to four ints |
| `Restaurant1_WalkToSeatSpot` | `(Bloke*, Pos tile, int phase)` ridemisc2.c | ridecb1.c | |
| `SetPersonPosition` | 3 params, sweep1.c | logflume6.c | here the DEFINITION is the flattened one and logflume6.c is the odd file (`(Person3D*, Pos)`) |
| `LFQueue_StepFront` | 3 params, lfmisc2.c | logflume4.c | same direction as SetPersonPosition (`(LFQueue*, Pos)`) |
| `PopUpInfoSetUp` | 4 params, fpui2.c | gameframe.c, popup.c | both pass a 2-word key struct by value plus x, y |
| `PtInRect` | the host shim's Win32 `(const RECT*, POINT)` in `portable/src/hostwin/user32.c` | cursorseg.c | cursorseg.c flattens POINT to `(r, int x, int y)`. user32.c is PORT-B's file, so the fix belongs on the cursorseg.c side |

The recipe: give the odd file a portable arm declaring the definition's shape,
and wrap its call sites so the portable arm builds the struct
(`Pos p; p.x = px; p.y = py;`) or unpacks it (`pos.x, pos.y`). A forwarder in
the defining file also works for the flattened-definition direction and is
cheaper when only one file disagrees.

### (b) a float passed as its bit pattern (6 symbols)

| symbol | definition | callers |
| --- | --- | --- |
| `Route_SetSpeed` | `(CoasterRoute*, float v)` schoolcar.c | coaster.c, coaster8.c, schoolcar5.c, schoolcar6.c declare `int v` |
| `PositionRouteCars` | `(i32, f32, i32)` schoolcar.c | the same four files |
| `Route_SetTrainAt` | `(i32, f32, i32)` coaster11.c | schoolcar7.c |
| `TrackCurve_EvaluateDerivative` | `(i32, i32, i32, f32, i32)` coaster10.c | coaster9.c (all i32), coaster13.c (f32 in the wrong slot) |
| `TrackCurve_EvaluateOffset` / `EvaluatePosition` / `EvaluateUp` | coaster9.c / coaster10.c | coaster13.c, coastertiny.c |
| `Track_MeasureDistance` | `(i32, f32, i32, f32, i32, f32) -> f32` coaster13.c | coaster7.c has i32 in the last slot |

**Do not simply retype these declarations to `float`.** coaster8.c calls
`Route_SetSpeed(rt, *(int*)&value->v[1])` — it deliberately reinterprets a
float's BITS as an int so that the argument moves without touching the FPU,
which is the caller-side codegen lever HANDOFF section 3 warns about. Retyping
the parameter to `float` would convert the bit pattern numerically and pass a
completely different value. The portable arm must declare `float` AND pass the
float itself (`value->v[1]`), per call site.

### (c) one name, two different functions (1 symbol)

`NewScriptEvent` is not a prototype conflict at all. sysstubs.c defines
`ScriptEvent* NewScriptEvent(int kind, int mode)` at **0x00468910**; levelkw.c
and movie3.c declare `void* NewScriptEvent(void* a, void* b, void* c)` at
**0x004689f0** — a different function. The portable link collapses both onto the
two-parameter one, so **levelkw.c and movie3.c currently call the wrong
function**. This needs a rename (0x004689f0 wants its own name), not a
signature fix, and it is a genuine recovery finding: worth checking whether the
two-argument and three-argument event constructors are a pair.

## 8. Gate results

| batch | files | audit.py | `[OK]` rows | relocs.py |
| --- | --- | --- | --- | --- |
| the three proved-live conflicts | 23 | PASS x23, 0 REJECT/FAIL | 321 | 0 MISMATCH |
| return-type-only | 92 | PASS x92, 0 REJECT/FAIL | 1654 | 0 MISMATCH |
| arity forwarders + AddBasicObject macro | 37 | PASS x37, 0 REJECT/FAIL | 932 | 0 MISMATCH |
| closure/libc conflicts | 34 | PASS x34, 0 REJECT/FAIL | 668 | 0 MISMATCH |

`progress.py` reports 3281 exact / 42 WIP before and after, i.e. the two
rename-pattern definitions (`DBPrintf`, `SkipMeasuredBlock`) and the 21
forwarders cost nothing.

**One note for the integrator:** `docs/LEGOLANDPROGRESS.HTML` records a line
number for every function, and a lane that inserts `#ifndef` lines moves them,
so `tools/progress.py --check` is stale until the report is regenerated. The
counts are unchanged; only the source links moved. This lane did not touch that
file (it is the integrator's).

## 9. Census

| | before | after |
| --- | --- | --- |
| `linkreport.py` prototype conflicts | 542 | **437** |
| wasm-ld `function signature mismatch` (distinct symbols) | 130 | **19** |
| wasm-ld warning lines | 531 | 62 |

The census falls by less than the warnings do, and that is expected: the
remaining 437 rows are mostly the address-taken callback declarations (all of
screen.c's table entries, the `WaterBlock_*` / `WWEntrance_*` / `ZebraCrossing_*`
families declared `void Foo()` in the file that stores them in a table). Those
do not trap through the link, because the table holds the real function, and
they should be fixed by the rename/typed-callback pass rather than one arm at a
time.
