# Lane PORT-C — headless tests of the recovered code against the real gamedata

Brief: `docs/SCOPE_PORT_WAVE.md` § PORT-C. Branch `scope/PORT-C`.
Owns `portable/cmake/tests.cmake`, `portable/tests/**`, `tools/oracle_*.py`
and its section of `portable/README.md`. `LEGOLAND/*.c` was read-only
throughout: not one game source was touched.

## What this is

The first tests of the recovered game's **behaviour**. One executable with a
subcommand per test:

```
cmake -S portable -B portable/build -G Ninja -DPython3_EXECUTABLE=$PY
ninja -C portable/build legoland_tests
ctest --test-dir portable/build --output-on-failure
```

`legoland_tests` is linked exactly as `legoland_linkcheck` is — whole-archive
`legoland_core` plus the generated closure — so a test that reaches something
nobody has ported yet dies in `gen_link`'s trap printing the symbol's name,
which is itself the result worth having. Every expected value is generated at
build time by a `tools/oracle_*.py` over `gamedata/`, so **nothing derived from
the game's assets is committed**: the repository holds scripts, counts and
digests only.

## The tests

| test | entry points | oracle | runs today | result |
| --- | --- | --- | --- | --- |
| `save_framing` | `BeginMeasuredBlock` 0x0047d790, `EndMeasuredBlock` 0x0047d800, `SaveGameWrite` 0x0047d760, `SaveGameRead` 0x0047d730, `FindeIneList` 0x0047d880 | `tools/oracle_savechunks.py` (the framing contract in `docs/runtime/persistence.md` + `LEGOLAND/profiles.c`'s header) | **native 64-bit and wasm32** | **24 checks pass.** The engine's nesting, the back-patched words and the EOF behaviour are all as specified. |
| `tile_geometry` | `GetTileDimensions` 0x00460540, `GetTileCentre` 0x0045ad60, `GetTileBounds` 0x0045acc0, `OverNewTile` 0x00483650, `CrossTileCentre` 0x004837d0 | `tools/oracle_tilegeom.py` over `tools/tilemap.py`'s per-cell placement, grid sampled from GLONE.MAP | **native 64-bit and wasm32** | **169 checks pass.** `GetTileCentre` and `GetTileBounds` agree on every sampled cell (the centre is the rect's midpoint), with a non-zero viewport origin and a non-zero 8.8 scroll. |
| `res_archive` | `RES_OpenVolume` 0x00489750, `RES_LoadDirectory` 0x004895a0, `AddMasterDir` 0x00489440, `RES_FindVolumeDir` 0x00489550, `RES_OpenFileFromVolume` 0x00489a00, `RES_ReadFile` 0x00489cf0, `RES_SetFilePointer` 0x00489d70, `RES_GetFilePointer` 0x00489db0, `RES_GetFileSize` 0x00489ce0, `RES_CloseFile` 0x00489de0 | `tools/oracle_res.py` over `tools/resfile.py` + `tools/leveldata.py` | **wasm32 only** — waiting on PORT-A's closure | Compiles and links; not yet executed. Native run segfaults (exit 139) for the reason in finding 1, which is the expected outcome. |
| `llidb_icm` | `LLIDB_LoadICM` 0x0047aff0, `LLIDB_GetCount` 0x0047b2d0, `LLIDB_GetElement` 0x0047b2e0, `LLIDB_FindElement` 0x0047b330, `ElemID` 0x0047b3f0 | `tools/oracle_icm.py` over `tools/tilemap.py`'s `load_icm` | **wasm32 only** — waiting on PORT-A's closure | Compiles and links. Native run stops on its own first check with `sizeof(LLElem) got 40, want 20` and says why — the designed behaviour, not a crash. |
| `loadpos` | `LoadPos` 0x0043f660, `BuildYRotationMatrix` 0x00443360, `MatrixMultiply` 0x00443270, `CopyMatrix` 0x00443490, `UnloadPos` 0x0043f7d0, `RES_OpenFile` 0x00489b60 | `tools/oracle_geom.py` over `tools/geom.py`, plus the load-time Y rotation `geom.py` documents but does not apply | **wasm32 only** — waiting on PORT-A's closure | Compiles and links; not yet executed (it goes through the same RES path as `res_archive`). |

193 checks run and pass today; three tests are built, linked and registered for
the Emscripten build and will run the moment `gen_link.py --ilp32` emits
compilable globals.

Every one of the seven test sources compiles clean under `emcc -Wall` at
wasm32. The wasm32 *link* still fails in PORT-A's territory, with the same 12
errors the brief describes:

```
gen/globals.c: error: redeclaration of 'kCastleObjName' with a different type:
               'unsigned char[]' vs 'unsigned int[4]'   (x12)
```

## Globals each test has to initialise

The closure rebuilds `.data` from the exe, so every global below already holds
its shipped value; what follows is what a test has to **overwrite** because the
game's own `InitSession` would otherwise have set it.

| test | global | address | why |
| --- | --- | --- | --- |
| `save_framing` | `g_savefile_fd` | 0x006691b0 | the open save descriptor; the test opens its own scratch file |
| | `g_chunk_depth` | 0x006691fc | placeholder stack depth, zeroed before the script |
| | `g_elist` / `g_elist_count` | 0x00669200 / 0x006691b4 | the element table `FindeIneList` searches |
| `tile_geometry` | `g_tile_sprites[0]` | 0x00805f60 | must point at a `Sprite` whose +0x16 height is the ground tile height |
| | `g_default_tile` | 0x00667ca4 | set to 0 so only the first slot is touched |
| | `g_map` | — | a header with width/height at +0x14/+0x16 and the viewport origin at +0x20/+0x22 |
| | `g_scroll_x` / `g_scroll_y` | 0x00667cb4 / 0x00667cb8 | 8.8 fixed point |
| `res_archive`, `loadpos` | `g_res_path` | 0x00813b04 | set to `"./"`; it is **both** the `"%s%s.res"` volume prefix and the drive root `RES_FindVolumeOnResPath` probes |
| | `g_master_vols`, `g_master_dirs` | 0x00798628, — | left as the closure has them (zero before `InitSession`); the mount builds them |
| `llidb_icm` | none | — | `LLIDB_LoadICM` allocates the page table and writes `g_llidb_count` / `g_llidb_capacity` / `g_llidb_pages` itself |

`FreeTileSpace` (0x0045aa90) is deliberately **not** tested: it memsets `n * 4`
bytes across `g_tile_sprites[]`, which is only right where a slot is 4 bytes.
It belongs in a wasm32-only tile-space test, together with `AllocTileSpace`
(0x0045a9b0), and is the obvious next test for this lane.

## Divergences and findings

These are the point of the lane. None is papered over.

### 1. `RES_LoadDirectory` stores a pointer in an `int` — the ILP32 proof

`LEGOLAND/resaudio2.c` (0x004895a0), faithfully recovered:

```c
node->sub += (int)base;
RES_LoadDirectory((RImgNode*)node->sub, v, base, dir);
```

`base` is the malloc'd directory image and `node->sub` is an `int`. That is
correct ILP32 code and is how the format works: the image's links are 32-bit
offsets rewritten in place into 32-bit pointers. On a 64-bit host the pointer
is truncated and sign-extended back, and the walk dereferences garbage —
`legoland_tests res_archive` on the native build exits 139 (SIGSEGV) before it
prints a line. **Verdict: neither side is wrong.** This is the sharpest
available demonstration that the 64-bit build is a symbol census only, and it
is why three of the five tests are wasm32-only. Worth quoting in
`portable/README.md`'s "ILP32 is the real target" section.

### 2. `tools/resfile.py` silently drops the archives' alias members

`parse_leaves` keeps a `seen` set of data offsets and skips a leaf whose offset
it has already emitted (`if off in seen: continue`). The archives genuinely
carry **two directory records pointing at one blob**, and `RES_LoadDirectory`
files both:

| archive | engine (tree walk) | directory byte scan | `resfile.parse_leaves` |
| --- | --- | --- | --- |
| Graphics1.res | 574 | 574 | 570 |
| Graphics2.res | 905 | 905 | 898 |
| Legoland.res | 583 unique (595 records) | 582 | 581 |

The members `resfile.py` loses are real: `CASTLE LEVEL 1.BMP`,
`INTERFACEEXINSTITUTEICON.LLS`, `WATER WORKS SHOWER.BMP`, `YES BUILD.LLS` in
Graphics1.res; seven cruise-ship `.lls` in Graphics2.res;
`MANPAN.LOWERBOD04.3D` in Legoland.res. **Verdict: the Python decoder is
wrong.** The dedupe is a robustness hack for a whole-file scan, and it throws
away directory entries the game resolves by name. `resfile.py` is outside this
lane's file ownership, so it is left alone and reported here; the fix is to key
the `seen` set on `(name, offset)` instead of `offset`, or to restrict the scan
to the directory image the way `leveldata.res_index` does.

### 3. Every byte scan misses zero-length members

Both Python recoveries reject `size == 0`. `Legoland.res` contains one such
member, `3DData\New\zoom2.pos`, which the engine files with size 0 (and
`RES_ReadFile` then correctly returns 0 bytes for). **Verdict: a decoder
filter, not a fact about the format.** The oracle therefore holds the C to the
tree walk and reports the byte-scan numbers alongside it.

### 4. `RES_LowSeek` / `RES_LowRead` are `SetFilePointer` / `ReadFile`

`LEGOLAND/res.c`, `memdb.c` and `sweep4.c` declare them
`__declspec(dllimport)` against IAT slots `[0x4ab104]` and `[0x4ab264]`, which
`portable/tools/win32_imports.txt` already names `SetFilePointer` and
`ReadFile`. Because the local names differ, `linkreport.py` files them under
"Unclassified (83)" and `gen_link.py` emits traps for them — so the RES read
path traps even once the kernel32 shim exists. **For the integrator:** this is
an alias pair, not missing work. Either `win32_imports.txt` should learn the
two local names, or `gen_link`'s alias pass should map an extern whose only
address comment is an IAT slot onto that slot's import.

### 5. The twelve "CRT-range wrappers" are libc, and four are one address

`portable/README.md`'s census files `MemAlloc`, `MemFree`, `HeapAlloc_w`,
`HeapFree_w`, `RES_FreeFile`, `ReleaseAnimInstance`, `CRT_calloc`, `rand_w`,
`sprintf_w`, `Format`, `NameCompare` and `DebugPrint` as "game-fn". Their
addresses say otherwise: 0x0049e4ff is `malloc`, 0x0049e4d0 is `free`,
0x0049e573 is `sprintf`, 0x0049e4b2 is `rand`, 0x004a020e is `calloc`,
0x004aab90 is `_stricmp`. `RES_FreeFile`, `HeapFree_w`, `MemFree` and
`ReleaseAnimInstance` are **all four at 0x0049e4d0** — an alias family, not
four functions. This lane's stopgap implements all twelve in eleven lines; the
census delta is in the next section.

### 6. The CD check gates every `RES_OpenFile` call

`RES_OpenFile` (res.c) opens with `RES_EnsureMounted(0)`, which loops on
`RES_FindVolumeOnResPath` (sysmisc2.c) until `GetVolumeInformationA` reports a
**CDFS** volume whose name is **"LEGOLAND"**, nagging with a modal
`MessageBoxA` in between. Headless that is an infinite loop through two traps.
**For PORT-A and PORT-B:** the host shim must answer `GetVolumeInformationA`
with `fsname = "CDFS"` and `volname = "LEGOLAND"` (and `GetLogicalDrives` /
`GetDriveTypeA` for the other arm), or the game never opens a single asset.
This lane's stopgap does it; it is four lines.

### 7. `LEGOLAND.ICM` vs `Legoland.icm` — case sensitivity

`LLIDB_LoadICM` opens the literal `"LEGOLAND.ICM"` (0x004bc120); the shipped
file is `gamedata/main/Legoland.icm`. That only resolves on a case-insensitive
filesystem — macOS APFS, and node's `NODERAWFS` on top of it. **On Linux CI it
will fail.** The host shim's path translation needs a case-insensitive
fallback, or the asset tree needs normalising. Recorded, not worked around:
the test runs as-is on this machine.

### 8. `.pos` / `.3d`: two readings of the same 48 bytes, both right

`tools/geom.py` reads each 48-byte element as twelve floats (a vec3 position
plus a 3x3 matrix); `LEGOLAND/loaders.c`'s `LoadPos` reads the first three
words into `int` fields and only the last nine as floats. The loader never
looks at those three words, it only copies them, so both readings are
consistent with the code. Noted because the two docstrings disagree in wording
and a reader will trip over it. All four `.pos` members the oracle picks
(`earth.pos`, `copters-old.pos`, `caro.pos`, `spinb.pos`) decode to **exactly**
`8 + per*count*48` bytes with zero bytes left over, which is strong independent
confirmation of the layout.

## Census before/after (brief rule 2)

`portable/tools/linkreport.py` over `portable/build/CMakeFiles/legoland_core.dir`:

| what the link still needs | before this lane | after (stopgap compiled in) |
| --- | --- | --- |
| unwritten game function stubs | 12 | **0** |
| unresolved-name stubs | 83 | **81** (`RES_LowSeek`, `RES_LowRead`) |
| host API stubs | 149 | **139** |
| extern-only globals rebuilt | 2232 (3 705 348 bytes) | unchanged |
| function aliases | 228 | unchanged |
| `legoland_linkcheck` links | yes | yes |

## Notes for the integrator

1. **`portable/tests/support/ll_test_host.c` is a stopgap designed to
   self-retire.** `tests.cmake` compiles it into `legoland_core` only while
   `portable/src/hostwin/kernel32.c` does not exist, and prints which branch it
   took at configure time. The moment PORT-A lands, it stops being compiled and
   the tests run against the real shim. It had to go into `legoland_core`
   rather than the test executable because that is how `gen_link.py` decides
   what to trap (defined set from every object under `legoland_core.dir`,
   referenced set from the `LEGOLAND` sources only) — the same mechanism
   `src/hostwin/msvcrt.c` uses. Defining these symbols in the test executable
   would collide with the traps `gen_link` would still emit.
   **If PORT-A's shim does not define some of the twelve CRT wrappers, move
   those few into it rather than re-enabling this file.**
2. **No hook was needed in the game.** Nothing in `LEGOLAND/` had to change for
   any of these five tests; every entry point is reachable from outside with
   globals the test sets up itself.
3. `enable_testing()` is called from `tests.cmake`, not `CMakeLists.txt`, so
   `ctest` works without PORT-C touching the shared file.
4. The oracle headers land in `<build>/gen-tests/` and are regenerated whenever
   the oracle script changes. They are **not** a dependency of `all`:
   `ninja legoland_tests` builds them.
5. `tests.cmake` returns early with a `STATUS` message when `gamedata/` is
   missing, so a checkout without assets still configures.

## Next for this lane

- Run the three ILP32 tests under node the moment PORT-A's closure links
  (`ctest --test-dir portable/build-wasm`); nothing else should be needed.
- `AllocTileSpace` / `FreeTileSpace` over the real tile-slot map, wasm32 only.
- `LLIDB_LoadTSMData` / `LoadTSFData` / `LoadILFData` / `LoadCSPData` against
  `tools/tilemap.py`'s `Loader` (the global tile-base allocation order is the
  interesting part). Not attempted here because they call `LoadSprite`, which
  pulls in the `.lls`/COMP image loader — worth its own test first.
- The `LoadBaseMap` layer RLEs against `tilemap.decode_tilegfx` /
  `decode_flags`. The decoders are ready; the C side is only reachable through
  the whole of `LoadBaseMap`, so it needs the object-class and render paths up
  first (or a hook, which would be the integrator's call).
