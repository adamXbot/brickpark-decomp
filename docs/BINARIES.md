# The LEGOLAND binaries

## `legoland.exe`

| | |
| --- | --- |
| Size | 802,857 bytes |
| Kind | PE32 GUI executable, i386 |
| Compiler | **Visual C++ 6.0** (linker version 6.0) |
| Built | 2000-04-07 14:27 UTC |
| Sections | `.text` 0x0a9d46 · `.rdata` 0x0838c · `.data` (mostly BSS) · `.rsrc` |
| Exports | **716 named symbols:** 675 functions in `.text`, 41 data symbols (see [`symbols/legoland.exports.txt`](../symbols/legoland.exports.txt)) |

The single biggest asset for the decomp: the exe exports its entire internal
API under **plain, descriptive C names**. No demangling needed, no leaked beta
required (LEGO Island needed one). Prefix histogram sketches the subsystems:

```
Get 71   Render 43   Load 37   Set 32   Init 24   Add 24   Remove 21
Kill 18  Save 14     Print 13  Create 10 Clear 10  Play 9   Bloke 8
```

Notable named functions that map the engine:

- **Rendering (DirectDraw):** `InstallDirectDraw`, `AddBlokeToRenderList`,
  `Render*` (43), `AddPathTileGFX`.
- **World / build mode:** `AddObjectToMap`, `AddPathTile`, `AddBasicPath`,
  `AddRollerCoasterPath`, `AddObjectToBuildList`, `AddNewObjectClass`,
  `AddBricks`.
- **Visitor AI ("blokes"):** `Add3DBlokeToList`, `AdjustBlokePosition`,
  `AllocBlokeCounters`, `Find3DPersonFromBloke`, `SetPersonDirection`,
  `Get_Path_Directions`.
- **Dynamic music (DirectMusic):** `LoadMusicStyle`, `LoadMusicSegment`,
  `LoadMusicBand`, `LoadMusicChordMap`, `BlendMusic`, `DMusicInitialised`.

## Imports (the platform surface for a portable port)

```
DDRAW.dll    DirectDraw    2-D blitter rendering (no Direct3D)
DINPUT.dll   DirectInput   mouse / keyboard
DSOUND.dll   DirectSound   SFX
WINMM + ole32              DirectMusic (COM) dynamic score
AVIFIL32 + Ir50_32         Indeo-5 AVI cutscenes
MSACM32                    audio codec manager
GDI32 USER32 KERNEL32 ADVAPI32 VERSION
```

A portable fork replaces exactly these seams: DirectDraw → an SDL/canvas
blitter, DirectInput/DirectSound → SDL, DirectMusic → a MIDI/pre-rendered
fallback, Indeo AVI → transcoded video.

## `Uninst.dll`

53,248-byte PE32 DLL — the InstallShield uninstaller stub, not game code.
