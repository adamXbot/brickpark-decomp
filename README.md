# LEGOLAND decompilation

A [matching decompilation](https://github.com/isledecomp/isle) of **LEGOLAND**
(PC, 2000, Krisalis Software / LEGO Media) — in the spirit of the LEGO Island
and LEGO Alpha Team decomps, with the long-term goal of a portable build that
runs natively and **in the browser** without emulation.

**Status: bootstrapping.** The disc-to-assets pipeline works end-to-end and the
game binary is identified and symbol-rich.

- **Disc extraction — solved, clean-room.** The disc ships the whole game
  program, level data, DirectMusic content and character animations inside an
  InstallShield 5.x `main.z` archive (PKWARE DCL "implode" compression;
  `unshield` does *not* read this older format). [`tools/iscab.py`](tools/iscab.py)
  + [`tools/blast.py`](tools/blast.py) extract all 331 members byte-exactly
  (`legoland.exe` verified as a valid 802 KB PE, every member's magic matches
  its extension). See [docs/INSTALLSHIELD_Z.md](docs/INSTALLSHIELD_Z.md).
- **Binary identified.** `legoland.exe` is a single **Visual C++ 6.0** GUI
  executable (linker 6.0, built 2000-04-07) on a **DirectDraw** 2-D isometric
  engine (DDRAW/DINPUT/DSOUND, DirectMusic, Indeo-5 AVI — *no* Direct3D). It
  **exports 716 functions under readable C names** (`AddObjectToMap`,
  `AddRollerCoasterPath`, `Get_Path_Directions`, `SetPersonDirection` …) — the
  full internal API, dumped to [`symbols/`](symbols/). See
  [docs/BINARIES.md](docs/BINARIES.md).
- **Asset formats — in progress.** `COMP` sprite archives (`Graphics*.res`),
  the `Legoland.res` world geometry, the level files (`.ltx`/`.lms`/`.lfm`),
  tile sets (`.tsf`/`.tsm`), image lists (`.ilf`) and object defs (`.odf`).
  See [docs/FORMATS.md](docs/FORMATS.md).
- **Browser product — planned.** A client-side **LEGOLAND Data Lab** that
  decodes the real sprites, levels, speech and music in the browser, no
  emulation — the same milestone the Alpha Team decomp reached. See
  [docs/ROADMAP.md](docs/ROADMAP.md).

## The game, technically

LEGOLAND is a theme-park builder (build rides, paths and shops; visitors —
"blokes" in the code — walk the park). Engine facts recovered from the binary:

- Single `legoland.exe` (VC6, ~695 KB `.text`), plus `Uninst.dll`.
- **DirectDraw** software/blitter 2-D rendering — sprites are 16-bit
  (RGB) `COMP`-compressed images at up to 640×480.
- **DirectMusic** dynamic score (`.sty` styles, `.sgt` segments, `.bnd`
  bands, chord maps) — hence `LoadMusicStyle` / `BlendMusic` in the exports.
- **DirectInput** mouse/keyboard, **DirectSound** SFX, **Indeo 5** AVI
  cutscenes (`Ir50_32.dll`).
- A CD check ("Please insert the LEGOLAND CD-ROM") to neutralise for a
  portable build.

## Getting started

1. Obtain your own copy of the game (disc or `LEGOLAND.iso`). Nothing
   copyrighted lives in this repo.
2. Extract the disc:
   ```bash
   # mount the ISO, then pull the game archive out of the installer:
   python3 tools/iscab.py extract /path/to/main.z -o gamedata/main
   ```
   The loose `Graphics1.res`, `Graphics2.res`, `Legoland.res`, the `Speech/`
   WAVs and the `.avi` cutscenes are copied straight off the disc.
3. `pip install reccmp pefile`, then
   `reccmp-project detect --search-path original`.

## Legal

This repository contains **no assets, code, or data** from the game — only
clean-room analysis tools and human-written code, MIT-licensed. You need your
own copy of LEGOLAND to build or run anything. Not affiliated with or endorsed
by the LEGO Group or Krisalis Software.
