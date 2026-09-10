# Scope PORT-A3 — one block per object, trap naming, and the node spine

> **Status: IN PROGRESS (claimed 2026-09-11 by PORT-A3).** Branch
> `scope/PORT-A3` from main `b28e35cb`. PORT-A2's follow-up on the generator and
> the node harness. Nothing in `LEGOLAND/*.c` is touched, so the VC6 gate has
> nothing to check for this lane.

## 1. ONE block per object: interior aliases

**Done.** `gen_link.py` sized every rebuilt global by the gap to the next NAMED
address, which is correct for unnamed data (it is what lets PORT-A2 re-point a
pointer into the middle of a block) and wrong for a named object that other
names reach into at an offset: the object came out as several separately
16-byte-aligned arrays.

The generator now reads an object's extent out of the game's own declaration —
array bounds times an element size that is a language fact — and emits ONE
block, with the other names as offset aliases of it.

### The measurement: 15 objects, 100 interior names

Every case in the image, from `gen/manifest.md`'s new section. The ones that
matter to a running game are marked.

| host | addr | block | declared | interior names |
| --- | --- | --- | --- | --- |
| `g_key_state` **(input)** | 0x007fdda0 | 260 | 256 | 4: `g_left_ctrl`+0x1d, `g_left_shift`+0x2a, `g_right_shift`+0x36, `g_right_ctrl`+0x9d |
| `g_ddraw1`/`g_gpu_state` **(graphics)** | 0x00667d70 | 984 | 984 | 34: `g_ddraw`+4, `g_drvcaps`+8, `g_helcaps`+0x184, `g_primary`/`g_screen_surface`+0x300, `g_surface_74`+0x304, `g_surface_78`+0x308, `g_draw_surface`+0x30c, `g_clipper`+0x310, `g_screen_palette`+0x314, `g_colour_mode`/`g_pixel_fmt`/`g_screen_depth`+0x318, the four font/`g_gdi_obj` slots +0x31c..+0x328, `g_ddsd`/`g_lock`+0x32c, `g_ddsd_height`+0x334, `g_ddsd_width`+0x338, `g_ddsd_pitch`+0x33c, `g_ddsd_bits`+0x350, `g_render_clip`+0x398, `g_target`+0x3a8, `g_video_locked`+0x3d4 |
| `g_cur_save_slot` **(profile)** | 0x0080ffe4 | 220 | 4 | 2: `g_save_type`+1, `g_profile_unlocked`+2 |
| `g_if_icon_pressed` | 0x007fdcc0 | 64 | 36 | 8 theme/cursor icons +4..+0x20 |
| `g_if_icon_normal` | 0x007fdd40 | 48 | 36 | 8 theme/cursor icons +4..+0x20 |
| `g_cafe_chair_mask` | 0x0081cda0 | 64 | 64 | 15 `g_oct_tab_*` +4..+0x3c |
| `g_cafe_sprites` | 0x0081cd60 | 36 | 36 | 8 `g_oct_tent_*`/`g_oct_kiosk` +4..+0x20 |
| `g_copters_path0` | 0x004c1124 | 24 | 24 | 5: paths 1-4 and `g_copters_layers` |
| `g_car_class_vt` | 0x004dd5e0 | 100 | 96 | 2: `g_curve_line_hooks`+0x20, `g_curve_arc_hooks`+0x40 |
| `g_rate_t0` | 0x00832928 | 20 | 20 | 5: `g_mood_low`+4, `g_rate_t1`+8, `g_mood_high`/`g_rate_t2`+0xc, `g_rate_t3`+0x10 |
| `g_spacetower_car_matte` | 0x0062fd64 | 16 | 16 | 3 |
| `g_bloke_age_limits` | 0x004b8334 | 16 | 16 | 2 |
| `g_appraisal_flags`/`g_report_state` | 0x00665ff8 | 160 | 160 | 1: `g_goal`+0x14 |
| `g_save_extra` | 0x00798728 | 12 | 12 | 2 |
| `g_level_done` | 0x0080ffd4 | 15 | 15 | 1: `g_have_profile`+5 |

Three of those are load-bearing right now:

* **`g_key_state`.** `extern unsigned char g_key_state[256]` (input.c:53), and
  the four alias offsets ARE the DIK codes: 0x1d LCONTROL, 0x2a LSHIFT,
  0x36 RSHIFT, 0x9d RCONTROL. `ScanKeyboard` (input.c:107) does one
  `GetDeviceState(kbd, 256, g_key_state)`; split up, that wrote 256 bytes into a
  29-byte object, and `IsLShiftDown`/`IsRShiftDown` (sysstubs.c:181/184,
  `g_left_shift >> 7`) plus `g_left_ctrl`/`g_right_ctrl` (unref6.c:158) read a
  byte of the wrong key, so shifted text entry and movie.c's Ctrl+Q skip were
  dead. This is PORT-B2 finding 2, closed.
* **`g_gpu_state`.** `extern char g_gpu_state[0x3d8]` (util.c:79) is the host
  GPU block gpu.c:10 maps field by field, and `CheckHostSystemGPU`
  (util.c 0x004637c0) clears the WHOLE of it with one `memset` before calling
  `InitHostSystemGPU`. Split up, that memset cleared nothing but the DirectDraw
  pointer: both surfaces, the clipper, the palette, the four GDI fonts, the
  `DDSURFACEDESC` the software renderer locks through (`g_ddsd` +0x32c, with
  `g_ddsd_height`/`width`/`pitch`/`bits` its own fields) and `g_video_locked`
  all kept their stale values across a display-mode change.
* **`g_cur_save_slot`.** gameframe.c:169 reads `CurProfile+0x44` as a dword
  (`g_cur_save_slot_wide`) that spans the save slot, the save type (+1) and the
  first two bytes of the 200-byte unlocked block (+2).

### Before and after, measured

`git show b28e35cb:portable/tools/gen_link.py` run over the same objects emits

```c
__attribute__((aligned(16))) unsigned char g_key_state[29];   /* 0x007fdda0 */
__attribute__((aligned(16))) unsigned char g_left_ctrl[13];
__attribute__((aligned(16))) unsigned char g_left_shift[12];
__attribute__((aligned(16))) unsigned char g_right_shift[103];
__attribute__((aligned(16))) unsigned char g_right_ctrl[103];
```

and a probe linked against that globals.c reports the offsets the game would
have seen:

| symbol | image offset | before | after |
| --- | --- | --- | --- |
| `g_left_ctrl` | +0x1d (29) | +256 | +29 |
| `g_left_shift` | +0x2a (42) | +272 | +42 |
| `g_right_shift` | +0x36 (54) | +749392 | +54 |
| `g_right_ctrl` | +0x9d (157) | +749280 | +157 |
| `g_primary` | +0x300 (768) | +570288 | +768 |
| `g_ddsd_bits` | +0x350 (848) | +32 | +848 |

Note `g_ddsd_bits` at +32: the "direct indexing still works" half of PORT-B2's
finding was true only for `g_key_state`. In the GPU block the fragments were
*close enough to overlap*, so `g_ddsd_bits` aliased another field of the same
lock descriptor — the renderer's `lpSurface` and the surface pitch were two
names for nearby bytes of the wrong object.

### How the interior alias is emitted, and why that form

There is no offset form of `__attribute__((alias))`, and no way in C at all to
place a symbol at an interior offset of another. The assembler has one:

```c
#if defined(__APPLE__)
__asm__(".globl _g_left_shift\n.set _g_left_shift, _g_key_state+42\n");
#else
__asm__(".globl g_left_shift\n.set g_left_shift, g_key_state+42\n");
#endif
```

`portable/README.md`'s PORT-A section records that module-level asm does not
survive the wasm backend — that is true of a `.text` *function* alias, which is
why `fn_alias` became a forwarder. A DATA symbol is different: in the wasm
object format a data symbol is a (segment, offset, size) triple, so an interior
label is exactly representable and `.set` lowers to it. Verified on both
backends before writing any of this (two TUs, the alias read from the other
one): wasm `interior == big+42`, Mach-O likewise.

One trap, which cost a link: **a tentative definition is a COMMON symbol under
`-fcommon`, and the assembler cannot resolve `.set alias, common+off` at all** —
it does not error, the alias silently stays undefined, and the link fails on the
alias name with no hint about why. A zero-filled host block therefore gets an
explicit `= {0}`, which makes it a real `.bss` definition; the object is no
bigger. Only host blocks get it, so nothing else in globals.c changes.

### The rules the pass follows

1. The extent comes from the game's own `extern` declaration: array bounds times
   `elem_size(type)`, which knows the primitive types and "every pointer is 4
   bytes" and **nothing else**. A `Pos`, an `FXEntry[]` or a `RideDef*` table has
   no size until somebody writes the struct, so such a declaration hosts nothing
   and keeps the gap tiling. That is the "where known, else the gap tiling" rule
   of the brief, and it is why the pass finds 15 objects and not 200.
2. The scan stops at any address inside the extent that is NOT an extern-only
   data name — a function, or data a game object defines — and clamps the extent
   there rather than claiming storage someone else owns. No case in this image
   needs the clamp; it is there so the next one cannot go wrong silently.
3. The merged block's size is `max(declared extent, the tiled end of the last
   absorbed address)`, so the total coverage of `.rdata`/`.data` is exactly what
   it was: PORT-A2's pointer resolution sees the same tiling, and the census
   numbers below are unchanged.
4. An absorbed address no longer gets a `symbol_at` entry, so a pointer word
   whose value equals it resolves through `containing()` to `(host, offset)`
   rather than to the alias name. One less symbol the prologue has to declare,
   and it cannot depend on an alias that the `missing_data` filter might not
   emit.

### Census: unchanged, which is the point

`gen/manifest.md`, wasm32 `-DLL_ILP32=ON`, before → after:

```
- globals defined: 2374 -> 2259 (3703048 bytes both times), data aliases: 238
- objects merged from interior-aliased names: 0 -> 15 (100 offset aliases)
- words re-pointed at a symbol address (ilp32): 272 -> 272
- pointer words re-pointed INTO a block (ilp32): 441 -> 441
- synthesised ll_gap_ blocks for unnamed data: 0 -> 0
- pointer words left raw: 12 -> 12
- host API stubs: 95 -> 95
```

115 fewer definitions, the same bytes, the same pointer resolution.

### The proof: `portable/tests/test_keystate.c`

A new `legoland_tests keystate`, 29 checks, registered on BOTH toolchains
(`ll_add_test(keystate ... FALSE)`): every check is a byte offset inside a byte
array, which is the same on LP64 and ILP32, it needs no `gamedata/` and no host
shim — `IsLShiftDown`/`IsRShiftDown` are real recovered game code that touches
nothing but these globals. It is the one test in `portable/tests/` with no
oracle, because its expectation is the original's own layout.

What it checks: the four DIK offsets; that a 256-byte `ScanKeyboard`-shaped
write into `g_key_state` does not reach `g_info_icon_g` at 0x007fdea4 (the first
named address past the array); that each alias reads back the byte written at
its own index, with an `i*7+0x11` pattern so no two indices share a value;
`IsLShiftDown`/`IsRShiftDown` through all four shift combinations; eight offsets
of the GPU block including the four `DDSURFACEDESC` fields; that
`CheckHostSystemGPU`'s `memset(g_gpu_state, 0, 0x3d8)` really clears
`g_ddraw1`, `g_primary`, `g_ddsd_bits` and `g_video_locked`; and the
slot/type/unlocked dword overlay.

Against the pre-fix generator every offset check fails with the numbers in the
table above.

**For the integrator:** this test needed three small append-only edits to
PORT-C's files (`portable/tests/ll_tests.h` one prototype,
`portable/tests/ll_tests.c` one table row, `portable/cmake/tests.cmake` one
source and one `ll_add_test`). The brief for this lane authorises adding a test
file under `portable/tests/`; PORT-C is closed.

### The limit nobody should trip over: an interior alias is invisible to the optimizer

`g_ddraw1` and `g_gpu_state` are two distinct extern objects as far as the C
type system is concerned, so a compiler that sees a store through one and a load
through the other IN THE SAME FUNCTION may assume they cannot overlap and keep
the cached value. This is not type-based aliasing — `-fno-strict-aliasing` does
not change it — it is the distinct-global assumption, and nothing short of LTO
visibility into the `.set` could fix it.

Measured, in `test_keystate.c`'s own code: at `-O2` on wasm32,
`memset(g_gpu_state, 0, 0x3d8); g_ddraw1 == 0` read the pre-memset pointer, and
the three-byte save-slot overlay read its pre-store value. Both pass at `-O0`
natively. The test's cross-alias accesses therefore go through `volatile`
lvalues, which force the real load and store.

**The condition is: one translation unit declaring two names whose extents
OVERLAP.** Two names in the same merged object at different, non-overlapping
offsets (`g_clipper` at +0x310 and `g_primary` at +0x300) are unaffected, and so
is a host and an alias declared in two different TUs, which is the common case —
`sysstubs.c` declares `g_left_shift` and `g_right_shift` and never mentions
`g_key_state`; `input.c` declares `g_key_state` and never mentions the shifts.

A tree-wide sweep finds **45 overlapping declaration pairs inside a single TU**,
and **42 of them are same-address aliases that predate this change** (`g_map` /
`g_level_map` in gameframe.c, `g_castle` / `g_castle_state` in coaster.c,
`g_screen` / `g_game` / `g_level_rec` in input.c — the 228 "stale extern names"
row of the census). The three the interior-alias pass adds:

| TU | overlapping pair | does a single function write one and read the other? |
| --- | --- | --- |
| `screens3.c` | `g_cur_save_slot_wide` (int) vs `g_cur_save_slot`, `g_save_type` | no — the dword read and the byte writes are in different functions |
| `screens3.c` | `g_level_done[15]` vs `g_have_profile` (+5) | no |
| `mechrides.c` | `g_copters_paths[5]` vs `g_copters_path0..4` | no — the table is read, the slots are written, in different ride callbacks |

So no game path is affected today. It is worth knowing because the next object
someone merges might not be so lucky, and because the failure mode is silent: a
stale value, not a trap.

## 2. Naming a trap, as a command

**Done.** `portable/tools/name_trap.py` plus the `legoland_headless_debug`
target.

### Why `--profiling-funcs` was not enough

A prototype conflict is neither a `TRAP` nor an undefined symbol. `wasm-ld`
replaces the mismatched call with a stub it names `signature_mismatch:<callee>`
whose entire body is `unreachable`. `--profiling-funcs` keeps that name in the
wasm name section — and emcc's link-time `-O2` then runs `wasm-opt`, which
**inlines a one-instruction body into every caller**. The name survives in the
section and no frame ever mentions it; all that reaches node is
`RuntimeError: unreachable at wasm-function[26]`.

So the debug target changes exactly one thing: `-O0 -g2` at LINK time. That
leaves `wasm-opt` out, the stub stays a real function, and the stack reads

```
signature_mismatch:InitHostSystemGPU
InitSession
GameMain
WinMain
main
```

It is a second target rather than a build-type switch because it has to be
buildable in the SAME build directory as the harness it explains: the game
objects are shared and only the link differs, so naming a trap costs one link,
not a reconfigure and a rebuild of 258 sources. ASYNCIFY is what makes that
possible — the browser target cannot be linked at `-O0` (unoptimised ASYNCIFY of
`RunAppraisalScreen` exceeds wasm's per-function local limit,
`portable/README.md`) and the headless harness has no ASYNCIFY at all.

### What the script prints

```
$ python3 portable/tools/name_trap.py
== legoland_headless_debug: linked, wasm-ld warned about 133 mismatched signatures
== last 12 host calls
   ... HOST ReadFile(4, 23598 bytes)
== RuntimeError: unreachable
== named stack, innermost first
   signature_mismatch:InitHostSystemGPU / InitSession / GameMain / WinMain / main
== PROTOTYPE CONFLICT, live: signature_mismatch:InitHostSystemGPU <- InitSession
   DEFINED  () -> i32    in gpu.c   <-- has the body, so this is the right one
   DECLARED () -> void   by startup.c
   LEGOLAND/startup.c:38: extern void  InitHostSystemGPU(void);   /* 0x00463700 */
   LEGOLAND/util.c:80:    extern int   InitHostSystemGPU(void);   /* 0x00463700 */
```

The two signatures and the file on each side come from
`linkreport.wasm_prototype_conflicts`, read out of the wasm objects; the
declaration lines come from a grep of `LEGOLAND/*.c`. That is the whole work
item: the file and line to put the `#ifdef LEGOLAND_PORTABLE` prototype in, and
which side is right (the definition, always — it has the body the matcher
validated against the original bytes).

It distinguishes the three ways this port dies:

| symptom | who owns it |
| --- | --- |
| `TRAP <dll> <symbol> from <callers>` | the host shim, or a matching lane (an unwritten body) |
| `signature_mismatch:X <- Y` | a matching lane: one caller-side prototype |
| a game function at the top of the stack | a real fault in that body (a null vtable slot, an `LL_UNPORTED_ASM` stub) |

### `--stages <skip list>`: how the list gets ENUMERATED

One live conflict kills the process, so it hides every conflict behind it.
`--stages` now takes a skip list of stage tags, which is the only way to step
over the one just found and see the next; `--stages none` runs every stage
including `RunGame`, the whole front end. The stage list follows `InitSession`
exactly now: all eight cursor sprites, all five `LLIDB_RegisterNewElement`
calls, then `RunGame`.

## 3. THE LIVE PROTOTYPE CONFLICTS — for PORT-M1

`wasm-ld` warns about **133** mismatched signatures that survive its DCE (the
census counts 542 declarations in all). Those are the candidates. These are the
ones **measured live on the path the game actually takes**, in the order the game
hits them — each found by re-running `name_trap.py` after skipping the previous
one:

| # | symbol | defined | declared | hit by | note |
| --- | --- | --- | --- | --- | --- |
| 1 | `InitHostSystemGPU` | `() -> i32` gpu.c:417 | `() -> void` **startup.c:38** | `InitSession` (startup.c:148) | **the current wall**, for node AND the browser page. util.c:80 already declares it `int`, so startup.c is alone and the fix is one word. |
| 2 | `RES_CloseFile` | `(i32) -> i32` sweep4.c | `(i32) -> void` in **13 files**: data3.c:29, listdel.c:102, llidb_load.c, llidb_odf.c, loaders.c:48, loadmap.c:39, mantex.c:39, movie3.c:155, music.c:362, objdesc.c:52, person3d.c:228, screen.c:81, texture.c:43 | `__BMPLoader` <- `LoadSprite` | PORT-A2 found this one. data2.c:569, render3.c:191 and rin.c:256 already say `int`. |
| 3 | `InitSoundSystem` | `() -> i32` lifecycle.c | `() -> void` **gamemain.c:93** | `RunGame` | new; the first thing `RunGame` does. |
| — | `RES_CloseVolume` | `(i32) -> i32` sweep4.c | `(i32) -> void` startup.c | `InitSession` lines 140/153/161/183 | PORT-A2's; on FAILURE paths only, so it sits behind #1 and #2 rather than in front of them. |
| — | `DBPrintf` | `() -> void` sweep1.c | variadic in 30+ files | `__BMPLoader`'s failure branch | PORT-A2's. sweep1.c's body is EMPTY at 0x00453a20, and an empty cdecl body compiles to `ret` whether or not it declares parameters, so `void DBPrintf(const char* fmt, ...) { }` should be byte-identical and fixes all 30+ call sites at once. |
| — | `RunGame` | `() -> void` gamemain.c:310 | — | — | this harness had it wrong, not the game. Fixed here. |

**The order matters, and it is cheap to get.** After fixing #1, run
`python3 portable/tools/name_trap.py` and the next one names itself. After
fixing each, delete the matching tag from `headless_spine`'s
`--stages loadsprite,rungame` line in `portable/cmake/headless.cmake`, which
turns that ctest into a ratchet.

Three names in the warning list deserve a look on their own because they are not
game functions: **`printf`, `sprintf` and `time`**, plus the generated
forwarders' `Format` and `HeapFree_w`. A game source declares a CRT function
with a signature emcc's headers disagree about; those call sites are poisoned
the same way and will not produce a game symbol in the stack.

The full 133:

```
AddBasicObject AddHelpMessage AddLevelFlag AddPTPOpenNode AddPTPRouteNode
AddRepairOrderForObject AddTrackSquareCount ApplyConsTileMap ApplyDestrTileMap
CalcMoveLine Castle_StartCoasterIfComplete CheckFocussedIcon
CheckWorkerOnMouseStatus ClampPopUpToScreen ClearSampleSource ClosePrimaryPopUp
Coaster3D_BuildTrackMesh Coaster3D_DrawMesh Coaster3D_EndFrame
CoasterModel_SetDirectory Copters_StepRider DBPrintf Dino_InitSound
DisplayAdvisorHelp DrawPathTileOverlay DrawSupportModel Format
Fountain_InitSound FreePlayItemUpdate GenerateGardener GenerateMechanic
HeapFree_w InitHostSystemGPU InitMusicSystem InitSoundSystem
JungleCruise_ProbeRiver JungleCruise_TraceRoute KLIBAUDIO_SetAVIVolume
KLIBAUDIO_UnLockAVISoundBuffer KillImage KillSoundSystem KillSprite
LFQueue_StepFront LLIDB_CloseICM LLIDB_GetElement LLIDB_LoadDataByIndex
LLIDB_LoadICM LLIDB_RegisterNewElement LLIDB_UnLoadData LLIDB_UnLoadTSMData
LLSStop LoadAppraisalScreenSprites LoadCoasterModelSet LoadMapTiles
LoadSavedGamesList MakeBloke MarkObjectTiles MarkWorkersOnMap Mat4_Transpose
MemScratchInit ModelRecord_GetName NewScriptEvent PTPVisitTile PU_NextInput
PauseCurrentTrack PhysObj_Init PlayInstanceOfSample PlayMovie PlayNarrationFile
PopUpInfoSetUp PositionRouteCars PowerStation_InitSound PrimeMovieAudio
PrintBackground PrintSprite PrintSpriteXY PtInRect RES_CloseFile
RES_CloseVolume RES_SetFilePointer Raster_RestoreState Raster_SaveState
ReferenceSprite RegisterDetailImage ReleaseSprite RemoveProfile RenderBlock
RenderSpriteScaledOffset RepairCellTick Restaurant1_WalkToSeatSpot
RestoreScriptStepHelp ResumeCurrentTrack RewindNarrationBuffer
RouteSeat_AttachCar RouteSeat_DetachCar RouteSystemInit Route_SetSpeed
Route_SetTrainAt SaveProfileToDisk ScreenToMapRef ScreenToMapRef2 SetBridges
SetInfoPanelText SetPersonPosition SetPointer SetSampleFade SetupControllers
ShowMessage SkipMeasuredBlock SpaceTower_PlaceCar StopMovieAudio
StopNarrationPlayback StoreNewSaveGameToDisk TrackCurve_EvaluateDerivative
TrackCurve_EvaluateOffset TrackCurve_EvaluatePosition TrackCurve_EvaluateUp
Track_MeasureDistance UnInitModelTextures UnlinkScriptStep
UnregisterDetailImage UpDateCurrentProfile UpDateCurrentSaveSlotInfo
UpdateHelpIconForText UpdateMovieAudio UpdateSampleSource UpdateSoundVols
printf sprintf time
```

## 4. The node path and the browser page: they no longer differ

**Done, and the brief's premise was stale.** The brief says the node harness
"dies with `unreachable` right after `HOST DirectDrawCreate` (before the CD
check), unlike the browser page, which passes DirectDrawCreate and the CD check
and dies after the loader". On this tree both run to the same instruction.

Node (`legoland_headless`, no arguments, `LL_HOST_TRACE=1`):

```
HOST GetFileVersionInfoSizeA("Legoland.exe") / GetFileVersionInfoA / VerQueryValueA
HOST CreateMutexA("LegolandGameMutex") / WaitForSingleObject(1, 0)
HOST DirectDrawCreate
HOST GetLogicalDrives / GetDriveTypeA("C:\")=3 / GetDriveTypeA("D:\")=5
HOST GetVolumeInformationA("D:\"): LEGOLAND on CDFS
HOST CreateFileA(".\volumes\Legoland.res")  -> CreateFileA("D:\Legoland.res")
     GetFileSize / SetFilePointer(16399660) / ReadFile(24426 bytes)     [directory]
     ... the same for Graphics2.res (31523) and Graphics1.res (23598)
RuntimeError: unreachable    = signature_mismatch:InitHostSystemGPU <- InitSession
```

The browser page (`legoland.html?trace=1`, served with
`python3 -m http.server` in `portable/build-wasm`, read with the in-app browser)
prints the identical trace down to the last `ReadFile`, then
`Uncaught (in promise) RuntimeError: unreachable` with no name (its link is
`-O2`). **The only difference left in the whole trace** is that the page opens
`".\volumes\Legoland.res"` once and node opens it and then falls through to
`"D:\Legoland.res"` — because the page's preload HAS a `volumes/` directory
under `/gamedata` and `gamedata/main` does not.

None of the four candidate causes the brief lists is involved: not `node_shim.c`
versus `ll_canvas.js`, not the absent ASYNCIFY, not `ll_js_display_open` under
node, not the message-box hook. `InitScreen` opens its 640x480 display and
`InitInputSystem` creates both DirectInput devices under node (the
`headless_spine` test proves it), and `MessageBoxA` is never reached because the
CD probe succeeds.

**Proved it is not this lane's own fix either.** A clean wasm build with
b28e35cb's `gen_link.py` — no interior aliases, `g_key_state[29]` and zero
`.set` aliases in `gen-browser/globals.c` — stops at the same frame,
`signature_mismatch:InitHostSystemGPU <- InitSession <- GameMain <- WinMain <-
main`. The brief describes an earlier tree state (most likely one without
`$LL_CD_DIR` wired, or before PORT-A2's `fopen` wrapper let `OpenDebugLog` and
`LoadStrings` through).

### So CI can run the spine without a browser: ctest `headless_spine`

`legoland_headless --stages loadsprite,rungame` must print `--- done` with no
`TRAP`, no `unreachable` and no `RuntimeError`:

```
volumes mounted, g_res_path="D:\"
RES_OpenVolume("Legoland.res")  -> "LEGOLAND"  595 members, 17239686 bytes
RES_OpenVolume("Graphics2.res") -> "GRAPHICS2" 905 members, 120408430 bytes
RES_OpenVolume("Graphics1.res") -> "GRAPHICS1" 574 members, 20018301 bytes
--- LoadStrings()            GetString(0xcb) = "LEGOLAND ERROR"
--- InitHostSystemGPU()      = 1
--- InitScreen()             = 1       640x480, window "LEGOLAND", RGB565
--- InitInputSystem()        = 1       keyboard + mouse devices
--- RES_OpenFile(...)        kind 0 -> ".\graphics\erase it.lls" = 1402 bytes
--- LoadSprite() x8          SKIPPED   (live conflict #2, RES_CloseFile)
--- LLIDB_LoadICM()          = 0
--- LLIDB_RegisterNewElement() x5
--- RunGame()                SKIPPED   (live conflict #3, InitSoundSystem)
--- done
```

The two skips are exactly the two live conflicts on that path. **Delete each one
from that line as PORT-M1 closes the conflict** — that is what makes this a
ratchet rather than a snapshot, and it is the cheapest possible regression test
for the whole loader stack.

## 5. Install paths over a preloaded MEMFS

**Done:** `portable/src/headless/pathtest.c`, target `legoland_pathtest`, ctest
`install_paths`, 20 checks, all green.

`ll_host_resolve_path` had no test, and on this Mac it could not have had a
useful one. Every other node target uses `-sNODERAWFS=1`, which goes straight to
APFS, and **APFS is case-insensitive by default**: `stat("LEGOLAND.ICM")`
succeeds on the first try, `ll_resolve_inplace` returns immediately, and the
case-folding loop never executes. Proof, one line: `ls gamedata/main/LEGOLAND.ICM`
lists `Legoland.icm`. The filesystem that matters is the browser's preloaded
MEMFS, which is case-SENSITIVE whatever machine packaged it — and so is Linux CI
on ext4.

So `legoland_pathtest` is linked with **no NODERAWFS and two
`--preload-file` trees**, which is exactly the browser's filesystem, and run
under node. The fixture is four empty files generated by `cmake` from nothing —
their NAMES are the whole point — so no game asset is involved and the test runs
with or without `gamedata/`:

```
/gamedata/Legoland.icm              asked for as LEGOLAND.ICM
/gamedata/stab.str                  asked for as ".\strings\Stab.str"
/gamedata/Graphics/Erase It.lls     a real subdirectory, mixed case, a space
/cd/Legoland.res                    the emulated D: drive
```

The first check is that MEMFS really is case-sensitive here
(`open("LEGOLAND.ICM")` must FAIL), so every pass after it is the resolver's
work and not the filesystem's. Then: exact and `.\`-prefixed; `LEGOLAND.ICM`,
`legoland.icm` and `.\LEGOLAND.ICM` all to `Legoland.icm`;
`".\graphics\erase it.lls"` and `".\GRAPHICS\ERASE IT.LLS"` to
`Graphics/Erase It.lls` (both components wrong-cased, and a space in the name);
`".\strings\stab.str"`, `".\strings\Stab.str"` and `".\STRINGS\STAB.STR"` to
`stab.str` through the flattened-install fallback; `"D:\Legoland.res"`,
`"D:\LEGOLAND.RES"` and `"d:\legoland.res"` to `/cd/Legoland.res`; three paths
with nothing behind them that must come back normalised and NOT invented; and
`create=1` normalising only. Every resolved path is also opened, because a
resolver that returns a plausible string nothing can open is the failure mode
worth catching.

**Verdict: `ll_host_resolve_path` is correct over MEMFS, including the
`LEGOLAND.ICM` -> `Legoland.icm` case PORT-C filed.** Two things the test found
along the way:

1. **`getenv` sees the host environment only under NODERAWFS.** Emscripten seeds
   its environment from node's `process.env` as part of NODERAWFS; a MEMFS build's
   `getenv` returns almost nothing. So a page cannot be told `$LL_CD_DIR` through
   the environment, which is why PORT-B's `src/browser/main.c` calls
   `setenv("LL_CD_DIR", LL_GAMEDATA "/volumes", 0)` itself — and why this test
   does the same instead of using ctest's `ENVIRONMENT` property. Had it relied
   on `ENVIRONMENT`, all three `D:` checks would have passed as no-ops against an
   unmapped path.
2. **A path that already `stat`s is returned unchanged**, so `".\x"` comes back
   as `"./x"` while `"LEGOLAND.ICM"` is rewritten to `"Legoland.icm"`. That is
   right — it opens — and the test compares with a leading `"./"` ignored rather
   than pinning a normalisation nothing depends on.

## 6. Census

`portable/tools/linkreport.py` over `legoland_core.dir`, both build trees,
before and after this lane: **every row identical.**

| row | count |
| --- | --- |
| game-fn | 12 (all CRT-range thunks, forwarded, 0 trapped) |
| game-data | 2612 |
| alias | 228 |
| host | 95 |
| crt | 43 |
| unknown | 86 |
| duplicates | 0 |
| asm stubs | 26 |
| prototype conflicts | 542 (133 survive wasm-ld's DCE as warnings) |

`gen/manifest.md`, wasm32 `-DLL_ILP32=ON`, before -> after:

| line | before | after |
| --- | --- | --- |
| globals defined | 2232 (3705348 bytes) | **2145** (3705348 bytes) |
| data aliases (same address) | 380 | **367** |
| objects merged from interior-aliased names | — | **15 (100 offset aliases)** |
| words re-pointed at a symbol address | 272 | 272 |
| pointer words re-pointed INTO a block | 441 | 441 |
| synthesised `ll_gap_` blocks | 0 (0 bytes) | 0 (0 bytes) |
| pointer words left raw | 12 | 12 |
| function aliases (stale extern names) | 240 | 240 |
| unwritten game function stubs | 0 | 0 |
| host API stubs | 95 | 95 |

87 fewer definitions and 13 fewer same-address aliases, for the same 3,705,348
bytes: the 100 interior names live at 87 addresses, 13 of which had a second
name. The bytes are identical because a merged block covers exactly what its
tiled pieces did — which is also why the pointer resolution is untouched.

### Tests

| | before this lane | after |
| --- | --- | --- |
| native ctest | 2/2 | **3/3** (+`keystate`) |
| wasm32 ctest | 4/5 | **7/8** (+`keystate`, +`headless_spine`, +`install_paths`) |

The one failure is `loadpos`, which is live conflict #2 (`RES_CloseFile`) and
unchanged: it is a finding, not a regression.

## 7. For the integrator

* **Nothing in `LEGOLAND/*.c` is touched**, so the VC6 gate has nothing to check
  for this lane. PORT-B's shim files and `src/browser/**` are untouched too, and
  so are `portable/CMakeLists.txt` and `docs/HANDOFF.md`.
* Three small **append-only edits to PORT-C's files** for the new test, which the
  brief authorises and which PORT-C (closed) has no claim on:
  `portable/tests/ll_tests.h` one prototype, `portable/tests/ll_tests.c` one
  table row, `portable/cmake/tests.cmake` one source line and one `ll_add_test`.
* **Build from CLEAN directories**: the generator changed.
* **No patch is owed to PORT-B.** Deliverable 3's fix turned out not to be in
  PORT-B's files — or in anyone's host code. It is live prototype conflict #1,
  one word in `LEGOLAND/startup.c:38`.
* **For PORT-M1**: §3 is the list, in execution order, with the file and line for
  each. Start with `InitHostSystemGPU` — it is one word in one file and it is the
  only thing between this port and the front end, for node and the browser
  alike. Then `RES_CloseFile` (13 files), then `InitSoundSystem`. After each,
  `python3 portable/tools/name_trap.py` names the next one, and a skip tag comes
  off `headless_spine`'s command line in `portable/cmake/headless.cmake`.
