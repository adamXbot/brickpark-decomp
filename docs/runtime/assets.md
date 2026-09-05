# Assets, animation, audio and host services

## Resource and image database

### Data structures

LLIDB elements are20 bytes: name`+0`, image/file name`+4`, type/flags`+8`, parsed data`+c`, reference count`+10`. Count/capacity/page table are `0x006691a4/0x006691a0/0x006691a8`; pages hold256 elements (`0x1400` bytes). Parsed TSF/ILF/CSP descriptors are36 bytes, count`+4`, sprite array`+8`; TSF adds base-slot`+0`, code array`+c`, second array`+10`, parent`+14`; ILF/CSP put dx/dy arrays at`+c/+10`. TSM output is eight-byte `{element,loaded}` pairs terminated `{-1,-1}`. [llidb.c](../../LEGOLAND/llidb.c), [llidb_load.c](../../LEGOLAND/llidb_load.c), [memdb.c](../../LEGOLAND/memdb.c), [DECOMP.md](../DECOMP.md#llidb-image-database-asset-resolution)

| Asset | Recovered stream and result | Evidence |
| --- | --- | --- |
| `.res` | u32 directory offset; packed member data from+4; hierarchical tail directory. Leaf signature `ffffffff, X, 0, size, data offset, NUL name` | [FORMATS.md](../FORMATS.md), [RE_CONTEXT.md](../RE_CONTEXT.md) |
| `.icm` | u32 count, count×20-byte element records, then two length-prefixed strings per element; stored pointer values are stale | [data2.c](../../LEGOLAND/data2.c), [FORMATS.md](../FORMATS.md) |
| `.tsm` | u32 count, length/self-name, then count length/tileset-name strings; parsed elements load recursively. This is the actual loader sequence | [llidb_load.c](../../LEGOLAND/llidb_load.c) |
| `.tsf` | u32 count, length/name, count×two dwords, count×length/sprite names, optional length/parent name. Allocates tile slots and optionally links descriptor at parent data`+74` | [llidb_load.c](../../LEGOLAND/llidb_load.c) |
| `.ilf` / `.csp` | u16 count, u16 type, length/name, count×signed dword dx/dy pairs, count×length/sprite names; **both loaders read offsets and double them in memory** | [llidb_load.c](../../LEGOLAND/llidb_load.c) |
| `.odf` | Builds`0xd0` ObjDef, resolves sprite/icon/build sprite/child, retains English/record0 localization, then standard callbacks, library-or-custom branch and external finalize; see the callback index for the DLL-failure fallback | [llidb_odf.c](../../LEGOLAND/llidb_odf.c) |

The tile-info table at `0x00801f40` has eight-byte entries `{set pointer, u16 code, u16 padding}`; the code is not a full dword despite the shared header's older declaration. `LoadMapTiles` reserves slot0, loads `MAPPING 1`, `BASIC TILES 1`, `NORMAL PATH TILES`, and the four scroll-arrow cursors; the basic set supplies the default ground sprite and the path set supplies `0x00832bf0`. [maprestore.c](../../LEGOLAND/maprestore.c), [pathtile2.c](../../LEGOLAND/pathtile2.c)

### Rules

Lookups are case-insensitive. Registration deduplicates by name, grows pages as needed, and initializes observed element fields; do not assume unwritten data fields are already null. Lazy load tests loaded bit1 and dispatches type masked with`0xfff0`: `0x10/0x1010` ODF,`0x20` TSM,`0x40` TSF,`0x400` ILF,`0x2000` CSP;`0x200/0x800` have no backing file. Class use additionally marks bit4 and loads its sibling even if the primary load failed. [llidb.c](../../LEGOLAND/llidb.c), [memdb.c](../../LEGOLAND/memdb.c), [saveprof.c](../../LEGOLAND/saveprof.c)

ODF missing required names report errors; missing icon falls back to `InstituteIcon.lls`; notice radius is clamped at least1. The loader links the record into `ObjectClassList=0x00669240`. CSP verifies every sprite result before committing; ILF has no equivalent per-sprite null test. TSF auto-plays multi-frame images of types2/3. [llidb_odf.c](../../LEGOLAND/llidb_odf.c), [llidb_load.c](../../LEGOLAND/llidb_load.c)

`RES_OpenFile` resolves volume/member or plain path and `RES_ReadFile` shares one OS file handle per volume, tracking member offsets. Mounting checks the disc label and CDFS. The all-drive probe scans all32 mask bits and retains the **last** matching CD drive. “Ensure mounted” uses the argument only to choose a probe, always looks for `LEGOLAND`, and minimizes/restores the window around retry/cancel UI. [res.c](../../LEGOLAND/res.c), [data2.c](../../LEGOLAND/data2.c), [sysmisc.c](../../LEGOLAND/sysmisc.c), [sysmisc2.c](../../LEGOLAND/sysmisc2.c)

### Tables, constants and format disagreements

`RE_CONTEXT.md` is an early snapshot: it treats COMP substreams as unknown and reports different discovery counts. The later decoded COMP format in `FORMATS.md` is used below. `FORMATS.md` itself contains conflicting abbreviated TSM/ILF descriptions: use the actual `llidb_load.c` read sequence above, not its “per-entry mappingName/tilesetName” TSM shorthand or ILF list without offsets. Real-asset variants were not re-parsed in Scope J, so these disagreements are not silently erased. [RE_CONTEXT.md](../RE_CONTEXT.md), [FORMATS.md](../FORMATS.md), [llidb_load.c](../../LEGOLAND/llidb_load.c)

ICM failures -1 through-6 are duplicate name, cannot create/update ICM, missing element, no identification key, no filename, cancelled. `LLIDB_LoadDataByIndex` at`0x47b7b0` actually reports these errors; it does not load valid indices. `LLIDB_UnLoadODFData` at`0x47cdd0` actually unloads TSF type`0x40`; `LLIDB_UnLoadLLSData` at`0x47c6a0` tears down ODF classes despite its name. [sysmisc3.c](../../LEGOLAND/sysmisc3.c), [sysmisc.c](../../LEGOLAND/sysmisc.c)

### Original bugs

TSF unloading does not null-check code/second arrays or clear `element->data`. `LoadTextFile` returns an uninitialized pointer for missing files; allocation failures in it and `LoadLocSet` leak resource handles. `LookupTextureName` with null list uses an uninitialized face pointer and with zero count reuses the original list as its uninitialized chest pointer. The drive-specific CD probe discards `toupper`'s result. Loader allocations, read results and length buffers are often unchecked; this specification does not infer a safe error contract where none was recovered. [sysmisc3.c](../../LEGOLAND/sysmisc3.c), [data3.c](../../LEGOLAND/data3.c), [sysmisc2.c](../../LEGOLAND/sysmisc2.c), [llidb_load.c](../../LEGOLAND/llidb_load.c)

### Callback/library installation

DLL records are16 bytes `{next,module,refcount,GetInterfaces}`, head`0x00669244`. ODF flag`0x10000` requests `.\dlls\<stem>.dll`; registration fills a temporary record, equal module handles share reference counts. The 14-entry **library** table is not a simple copy of 14 ObjDef slots: its init entry is called once and is not stored at`+a4`. Nonnull custom pointers override ordinary slots;`+b4/+b8/+bc` are copied even when null. Exact mapping is in [callbacks](callbacks.md). [loaders.c](../../LEGOLAND/loaders.c)

## Encoded images, maps and installer

### Data structures

COMP has40-byte header: magic at0, width/height/bpp at4/8/c, count/flags10, reserved14, s0 at18 (block size-24), s1 at1c (pixel **word count**), s2 at20 (length bytes), s3 at24 (opcode count). Payload at28 is `2*s1` RGB555 bytes, then s2 length bytes, then packed control dwords. Control size is `ceil(s3/16)*4`; s0 includes the16 bytes of stream descriptors. [FORMATS.md](../FORMATS.md)

MAP order is length/name, mapping name, terrain name, u16 width/height, class-name count/list, placed-object count and `{u32 class index,x,y}`, pathset count/list, four size-prefixed tile/map/RF/user layers, count×20-byte extra overlay records, optional `BRIDGES!` tag and bridge terrain name, then the terrain-tile stream. Overlay records are `{i32 x,y,2x,2y,u32 image}`. For host grid prerequisites see [world](world.md). [FORMATS.md](../FORMATS.md), [loadmap.c](../../LEGOLAND/loadmap.c)

InstallShield-Z uses signature`0x8c655d13`. Each filename follows its42-byte metadata; within metadata, expanded/stored sizes are dwords+16/+20, stream offset+24, DOS time+28, then u8 name length/name. DCL streams start`00 06`: uncoded8-bit literals and4,096-byte window. Full archive extraction rules remain in [INSTALLSHIELD_Z.md](../INSTALLSHIELD_Z.md).

### Rules

COMP consumes LSB-first two-bit opcodes: 00/01 one literal pixel,10 one transparent pixel,11 reads a length byte. Length0 ends the scanline. Positive length consumes a subopcode:00 literal run,01 repeated pixel,10/11 transparent run. Transparency comes from opcodes; black pixel0 is opaque. RGB555 channels expand using `(v<<3)|(v>>2)`. [FORMATS.md](../FORMATS.md)

Tile graphics RLE starts after a two-byte marker. Control top bits:00 sets accumulator without advancing cells;40 literal run;80 fill; c0 skip/empty. For advancing modes low-six count0 means64. Ground uses accumulator-1 as tileset group; accumulator`0x20` chooses path/object interpretation. Terrain words use high-byte-minus1 tileset group and low-byte delta; `ffff` ends a solid run, with intervening byte skips. Layer flag decoders must follow their own semantics, not assume the tile accumulator applies to all four streams. [FORMATS.md](../FORMATS.md), [loadmap.c](../../LEGOLAND/loadmap.c)

### Constants, bugs and callbacks

Ground/path sprites are32×16; tile width is twice height. Coaster design LTX is width/height dwords plus width×height bytes (shipped examples32×32); LPT has count145 and BGR0 entries. `.lms/.lfm` geometry is only partially covered by the supplied references. The old LoadBaseMap note about decoding map flags again as base tiles describes a **fixed reconstruction error**, not an original bug to preserve. Asset callbacks resolve through LLIDB and class installation. [FORMATS.md](../FORMATS.md), [loadmap.c](../../LEGOLAND/loadmap.c)

## People, outfits and animation files

### Data structures

| Record | Recovered layout | Evidence |
| --- | --- | --- |
| Morph `.3d` at loader`0x43fa80` | u32 frames; per frame u32 vertex count + float triples, u32 normal count + float triples; u32 face count, u32 Gouraud count, triangle index triples,36-byte face records | [person3d.c](../../LEGOLAND/person3d.c) |
| Face | flags`+0` (`0x2000` flat colour), RGB`+4`, texture/ramp id`+8`, three float UV pairs`+c`;36 bytes | [person3d.c](../../LEGOLAND/person3d.c) |
| Person3D | `0x94`; kind`+8`, Bloke`+c`, screen`+1c/+20`, depth weights`+34/+38`, scale`+40..48`, frame`+4c`, private face/part array`+50`, depth`+54`, orientation`+58..78`, appearance selectors`+7c/+80`, sex`+84`, animation`+88`, leg/arm colours`+8c/+90` | [person3d.c](../../LEGOLAND/person3d.c), [blokeanim.c](../../LEGOLAND/blokeanim.c) |
| `.loc` | texture count`+0`, model context`+4`, texture stem`+c` (capacity not proven), relative pointers`+2c/+30` relocated after load | [data3.c](../../LEGOLAND/data3.c), [lanes/fable-c-data3save.md](../lanes/fable-c-data3save.md) |
| Outfit patch | six bytes: relative texture id short, x/y/w/h bytes; patch table at context`+30`, texture base at`+4` | [anim2.c](../../LEGOLAND/anim2.c) |
| Position table at loader`0x43f660` | two counts followed by count×per×48-byte records `{three raw 32-bit scalars,matrix[9]}`;40-byte descriptor with scale triple at8 and item array24 | [loaders.c](../../LEGOLAND/loaders.c) |
| RIN | on disk sprite count, NUL names, frame count, frame×sprite-count slot indices.32-byte result: offsets0/4, remap8, riders c, sprite/frame counts10/14, arrays18/1c | [rin.c](../../LEGOLAND/rin.c) |
| BINV/BNV | magic`0x0101`, frame count u16+2, frame-list offset20; names/vertices/links are file-relative pointers relocated on load. Name nodes next4, vertices8, name c, orientation10; vertex is20 bytes with short x/y and float z | [rin.c](../../LEGOLAND/rin.c), [bnvpath.c](../../LEGOLAND/bnvpath.c) |
| BNVPath | `0x48`: bin0, tag4, name20 bytes at8, vertical scale1c, z base20, x/y24/28, step x/y30/34, person height3c, frame delta40, recalc44 | [bnvpath.c](../../LEGOLAND/bnvpath.c) |

### Rules

Morph loading negates vertex/normal Y, normalizes normals as floats, converts both clouds in place to16.16, computes frame bounds and shares topology. Gouraud faces use three normals; remaining faces one. Frames are selected without interpolation. Live animation tables initialize six visitor animations per sex, two Geoff, one Tracy; the larger counts inferred from global gaps in `blokeanim.c` are storage gaps, not proof of additional loaded animations. Frame wrapping uses signed remainder. Nonlooping completion returns1 and rewinds to0. [person3d.c](../../LEGOLAND/person3d.c), [data2.c](../../LEGOLAND/data2.c), [blokeanim.c](../../LEGOLAND/blokeanim.c)

Per-person copies choose stable outfit/colour indices only when stored selectors=-1. Kind1 uses the selected sex's two patch tables; colour indices are random&7, with leg index7 rerolled. Kinds3/2 use colours(4,1)/(0,3). Texture patches remap all three UVs only if the whole triangle lies within source x..x+w+1 and y..y+h+1 after clamping UVs to[0,1]. Remapping expands by source texture size, translates/scales into destination patch, then renormalizes by destination texture size. Registration increments texture ids even for failed loads. [savechunks.c](../../LEGOLAND/savechunks.c), [anim2.c](../../LEGOLAND/anim2.c), [data3.c](../../LEGOLAND/data3.c)

Render3DPerson clips a160×120 window, selects the locked surface target and temporarily sets x87 control word`0x7f`. Model pipeline: X/Z scales×0.447, transpose rotation for light, stand vertices on minimumY and centre X/Z, rotate and compute depth keys, then project `X=ox+2(x+z), Y=oy+y-x+z`. Draw only negative screen cross products, swapping winding on negative matrix parity. Shade uses max-like `(dot<0 ? 1 : dot)+0x3333`. Gouraud/flat and colour/texture select four rasterizers. A cursor-pixel hit identifies person kinds1/2/3 as`0x306/307/308`. [person3d.c](../../LEGOLAND/person3d.c), [rin.c](../../LEGOLAND/rin.c)

Position tables rotate matrices90° aroundY at load without changing positions. RIN draws slots in reverse file order and interleaves eligible riders before each sprite. BNV path scale is`49152/(near-far)`; initial position is caller's, recalc1, delta frame0. BNV placement normalizes orientation rows, sums eight vertices, derives cell coordinates and writes slope/height before orientation. [loaders.c](../../LEGOLAND/loaders.c), [rin.c](../../LEGOLAND/rin.c), [bnvmove.c](../../LEGOLAND/bnvmove.c), [bnvpath.c](../../LEGOLAND/bnvpath.c)

BINV frame-list nodes include their next link at `+08`, after count/head at `+00/+04`; `GetBinVFrame` walks that link. Internal offsets relocate from the file base, and null offsets remain null. RIN sprite names are NUL-terminated strings read byte by byte, expanded as `<directory>\<name>.lls`; every frame contains one integer slot index per sprite. The reverse draw walk sets each layer's LLS frame to the selected RIN frame before drawing. [rin.c](../../LEGOLAND/rin.c)

`LoadPalette` skips an eight-byte header and packs256 RGB triples into16-bit colors, using565 only for screen selector2 and555 otherwise. `LoadColourTable` discards an18-byte TGA header, reads256 BGR entries and the32,768-byte15-bit-to-palette-index lookup, builds a no-collapse DirectDraw palette and a parallel RGB555 table. These two palette input layouts are different. [rin.c](../../LEGOLAND/rin.c)

`GetNearestColour` uses RGB555 as an index into the palette lookup for display mode0, returns RGB555 directly for mode1 and RGB565 for mode2; other modes return0. The corresponding transparent values are `0xfe`, `0x3ff` and `0x7ff`. RGB555 allocates five bits per channel; RGB565 allocates six to green. [sweep1.c](../../LEGOLAND/sweep1.c)


### Tables, disagreements and original bugs

Light is`{-0x1800,-0x5000,0x3000}`, window centring anchors80/90, depth scale`0x40000000 / ((zmax-zmin)>>5)`. Palette loading is256 RGB triples after an8-byte header, packed565 for depth selector2 and555 otherwise. `colours.tga` has18-byte header,256 BGR entries, then`0x8000` RGB555→palette-index bytes. [person3d.c](../../LEGOLAND/person3d.c), [rin.c](../../LEGOLAND/rin.c)

`FORMATS.md` calls loader`0x43f660` a `.3d` loader and interprets asset bytes as rigid transforms; the current source names that function `LoadPos`, while `LoadAnim3D` at`0x43fa80` reads morph meshes. These are distinct parsers. Prefer the appropriate current loader contract; the old count/asset interpretation needs revalidation against original members and is **not** resolved by choosing one extension globally. [FORMATS.md](../FORMATS.md), [loaders.c](../../LEGOLAND/loaders.c), [person3d.c](../../LEGOLAND/person3d.c)

Original render defects: bounding corners duplicate(minX,maxY,maxZ) and omit(maxX,maxY,minZ); parity-swapped screen vertices keep depth/UV/shade in original triangle index order; flat pass rechecks parity per face and only overwrites shade0. Unknown character kinds leave table/colour selectors uninitialized. `RecolourModelParts` never reads key2: it matches key1 only and chooses replacement by triangle-half position; its four-byte read of a three-byte RGB key includes an ignored extra byte. [person3d.c](../../LEGOLAND/person3d.c), [blokeanim.c](../../LEGOLAND/blokeanim.c), [savechunks.c](../../LEGOLAND/savechunks.c), [savegame2.c](../../LEGOLAND/savegame2.c)

`FixUpLocSetPointers` adds the LOC allocation base to both fields`+2c/+30` unconditionally. A zero stored offset therefore becomes the base pointer; this helper does not preserve a null sentinel. [tinystubs.c](../../LEGOLAND/tinystubs.c)

### Callbacks

Animation instances and BNV paths are consumed by ride activation/render callbacks; transport-specific behaviour in `anim2.c` and `posstep.c` is integrated in [transport](transport.md). RIN rider insertion uses the same seat list described in [world](world.md). [anim2.c](../../LEGOLAND/anim2.c), [posstep.c](../../LEGOLAND/posstep.c), [rin.c](../../LEGOLAND/rin.c)

## Audio and host lifecycle

### Data structures

Sample definitions and playable instances share56-byte records: next0, refcount4, fade8, SoundSource16 bytes at c (`kind,obj,x,y`), flags u16 at1c, padding1e, due time20, callback24, definition/alias parent28, DirectSound buffer2c, owned heap buffers30/34. Instances link to the root definition and duplicate its buffer. Source kinds are0 none,1 Bloke,2 map ref,3 level xy. [audio2.c](../../LEGOLAND/audio2.c), [audio3.c](../../LEGOLAND/audio3.c), [sysstubs.c](../../LEGOLAND/sysstubs.c)

IMT uses state`0x004bf778`, event`0x0079a6a0`, command`a6a4`, argument`a6a8`, current theme`a6ac`. MIDI track is28 bytes: owner0, length4, data8, cursor c, unused10, time14, active/pending shorts18/1a. Narration retains source WAV/PCM formats, ACM stream/header and source/decoded buffers. [sysmisc2.c](../../LEGOLAND/sysmisc2.c), [audio4.c](../../LEGOLAND/audio4.c), [music.c](../../LEGOLAND/music.c)

MidiFile is24 bytes: tick scale`+00`, tempo`+04`, clock`+08`, track count short`+0c`, track-pointer array`+10`, playing short`+14`. Loading consumes the big-endian MThd fields, computes `division × 20000`, loads each MTrk buffer, links each track to its owner and initializes tempo to`0x100`. The track reader leaves fields other than its length/data/active and later owner unspecified until playback. `PlayMIDI` publishes the file as current, clears its clock, sets playing, and resets every track's clock/position with active/pending set1. Header tags, read results and allocations are not validated here. [music.c](../../LEGOLAND/music.c), [audio4.c](../../LEGOLAND/audio4.c)


Narration's WAVEFORMATEX view is18 bytes: tag/channels shorts`+0/+2`, sample rate/bytes-per-second dwords`+4/+8`, block alignment/bits/extension-size shorts`+c/+e/+10`. Source-format pointer is`0x007cacb0`, data offset`0x007cacb4`, full data length`0x0079ac04`, remaining bytes`0x007cacac`. The restaurant FX table has three12-byte rows at`0x004b6968`, with the loaded sample at`+8`, correcting an older screen declaration that put it at`+4`. [audio5.c](../../LEGOLAND/audio5.c)

### Rules

Playable start requires ready system, nonnull instance and nonnull definition. Loop mask`0x04` controls DirectSound looping; start clears mask`0x02`, stop sets it. Individual pause mask`0x01`, fading mask`0x08` and global mute/pause govern resumption. Source1 uses Person3D **screen** position, source2 projects its map tile, source3 subtracts pixel scroll. Pan is centred x×4 clamped±10000; attenuation folds positions inside viewport to zero distance and applies master dB minus squared outside distance/60. Values below-3000 or above0 map to-10000. [audio2.c](../../LEGOLAND/audio2.c), [audio3.c](../../LEGOLAND/audio3.c), [audio4.c](../../LEGOLAND/audio4.c), [sysmisc.c](../../LEGOLAND/sysmisc.c), [sysmisc2.c](../../LEGOLAND/sysmisc2.c)

Explicit sample volume0..100 maps to `(volume-100)*32`; UI slider conversion instead spans-4000..0 with its bottom snapped to-10000. Fade detaches source and sets fading state; kind0 matches unsourced records,1 matches object,2/3 positions. Definition deletion destroys instances, its heap blocks, master buffer, then record. Narration preparation does not start playback; `ResumeCurrentTrack` starts the prepared stream. MIDI reads its integer fields big-endian. [audio3.c](../../LEGOLAND/audio3.c), [audio4.c](../../LEGOLAND/audio4.c), [music.c](../../LEGOLAND/music.c)

`PauseCurrentTrack` actually destroys speech: it stops playback, unprepares/closes ACM, closes the file, destroys the audio buffer, frees source/decoded/format allocations and clears state. Narration preparation first tries `speech\<name>`, then that path under the resource root; it refuses while speech is active. It decodes blocks of ten source block-align units to16-bit PCM with the original channel count/rate, prepares ACM, creates a`0xa000`-byte playback buffer, applies speech volume and sets state1. Preparation and resumable playback must not be conflated with this destructive “pause.” [audio4.c](../../LEGOLAND/audio4.c)

DirectMusic band and motif playback queues secondary segments on the next measure (`0x2080`). Template playback composes with activity0; shaped playback uses ten measures, shape2, activity3, no intro/end. Both set999 repeats and play on the next measure (`0x2000`), then release the composed segment. Blending uses the same shaped composition and auto-transition flags`0x2022`, releasing both resulting references. These wrappers require music instance/ready state but ignore composition/playback failure results. The composition engine and media content remain external; its wrapper parameters are fully recorded here. [music.c](../../LEGOLAND/music.c)


Music mailbox opcodes1 stop,3 transition theme,4 theme; states1/2 setup,5/6 queued,7 running. Theme normally wraps signed `%5`, but state5/6 posts raw input. Sound init runs window→samples→music; shutdown samples→music. Input shutdown keyboard→mouse→DirectInput. MIDI init starts timer before opening output and teardown kills timer before closing output. Music setup can report success after thread creation failure. [sysmisc2.c](../../LEGOLAND/sysmisc2.c), [lifecycle.c](../../LEGOLAND/lifecycle.c), [audiomisc.c](../../LEGOLAND/audiomisc.c), [sysstubs.c](../../LEGOLAND/sysstubs.c), [util.c](../../LEGOLAND/util.c)

MIDI initialization requests a20ms periodic timer at10ms resolution and opens the MIDI mapper with no callback, then reports success regardless of those API results. Game-map teardown releases its23-entry FX table while leaving the cached `CASTLE OBJ` element owned by LLIDB. Sample-system teardown returns0 when already down; on the active path it releases and clears the interface before clearing the ready flag, so a reentrant release can still observe the system as ready. [lifecycle.c](../../LEGOLAND/lifecycle.c), [util.c](../../LEGOLAND/util.c)


`ReadNarrationWaveHeader` seeks to0 and requires RIFF/WAVE signatures, discards the RIFF size and treats the next chunk as the format without checking its ID. It allocates at least18 bytes, reads exactly the declared format size and forces extension size0 when that size is at most18. It then reads chunk IDs/sizes until data: every unwanted payload is allocated/read/freed in full, with no odd-byte padding adjustment. Data records its length and current file offset, leaving the descriptor at its first payload byte. Short reads fail; allocations are unchecked, and a short format read retains the allocated global block. Rewind seeks back to the recorded data offset and resets remaining bytes to the full length. [audio5.c](../../LEGOLAND/audio5.c), [tinystubs.c](../../LEGOLAND/tinystubs.c), [narration lane](../lanes/fable-d-audio5.md)

`ClearSampleSource` requires the ready system, sample and definition, then restores only master volume; previous pan is retained. `RefreshSampleVolumes` walks defined live samples and refreshes each source only when GetStatus returns exactly0; the returned status bits are ignored. Restaurant2 starts FX rows0/1 with argument pairs(0,1)/(1,1) at source kind2 and the placement's byte x/y, leaving the unused source-object field uninitialized. [audio5.c](../../LEGOLAND/audio5.c)

Narration is considered playing only at state3. Option audio pause/resume walks all live instances, applying individual sample pause/resume. Theme transition accepts only IMT states1/2, queues command3 with signed theme%5 and signals the event. Keyboard teardown releases a nonnull device but leaves its pointer unchanged. The empty FreePlayInit, RenderIconsHook, DrawPopUpEnd, RenderViewCellProbe and DebugErrorSink bodies have no additional behavior. Other UI/render microhelpers from this mixed file are consolidated in [presentation](presentation.md), with world/script operations in [world](world.md) and [persistence](persistence.md). [tinystubs.c](../../LEGOLAND/tinystubs.c)

### Tables, constants, bugs and callbacks

Viewport for sound is config`+10/+12`, not outer screen dimensions`+0/+2`. Unknown SoundSource kinds leave source position or matching flag uninitialized. Sample allocation initializes only observed fields and has no failure guard; narration retains unchecked allocations/API results and unbounded path building. SFX definition tables and dynamic DirectMusic composition are partly external; no browser audio scheduling or replacement is implemented here. [sysmisc2.c](../../LEGOLAND/sysmisc2.c), [audio3.c](../../LEGOLAND/audio3.c), [sysmisc.c](../../LEGOLAND/sysmisc.c), [sysstubs.c](../../LEGOLAND/sysstubs.c), [audio4.c](../../LEGOLAND/audio4.c), [music.c](../../LEGOLAND/music.c)

DirectDraw presents software-rendered images; fullscreen mode tries16bpp then8bpp, classifying display mode as0=8bit,1=555,2=565. Present runs sprite animation/cursor, enforces28ms minimum frame period, blits a640×480 window, retries once after surface loss, then updates frame accounting. Clock reads frozen-held/current system ticks minus epoch; blink toggles every512ms. `Rand_Max(max)` is CRT rand modulo(max+1), so it is inclusive and retains modulo bias; `Rand_Tween(lo,hi)` shifts that result. [sysmisc.c](../../LEGOLAND/sysmisc.c), [util.c](../../LEGOLAND/util.c)

The debug heap prepends16 bytes, with last11 tag characters and a decimal line field, or an all-tag string allocation; `_msize` drives accounting. `__DEBUG_TAG` deliberately leaves an orphan marker allocation. Whole-list deletion assumes next at the first dword. The sweep2/sweep5 comments retain historical warnings about **stand-in tails beyond an early return**. In this baseline the named anchor helpers are only declarations, with no calls in those files. The warning therefore does not prove that their present small bodies contain fabricated tails. Empty functions and prefix-only provenance still cannot establish a missing larger behavior; treat the warning as a source-history limit rather than an invented gameplay rule. [memdb.c](../../LEGOLAND/memdb.c), [listdel.c](../../LEGOLAND/listdel.c), [sweep2.c](../../LEGOLAND/sweep2.c), [sweep5.c](../../LEGOLAND/sweep5.c)

Other leaf services expose counter/clip/volume/instance state without defining larger algorithms. The original platform seams include DirectDraw, DirectInput, DirectSound, DirectMusic/WinMM, Indeo/AVI and Win32; a portable implementation needs replacements but no specific replacement architecture is prescribed by the recovered sources. [sweep2.c](../../LEGOLAND/sweep2.c), [sweep3.c](../../LEGOLAND/sweep3.c), [sweep4.c](../../LEGOLAND/sweep4.c), [sysstubs.c](../../LEGOLAND/sysstubs.c), [BINARIES.md](../BINARIES.md)

The build FX table at`0x004b9228` has23 twelve-byte records `{name,pad,sample}`. Class flag`0x40000` selects effect1; otherwise flag`0x80000` selects effect0 when `0x200000` is also set, or `rand()%5+3` (indices3…7) without it; other classes select0. First-name table sizes are90 and83, surname table107; their string contents remain external. [mapinit.c](../../LEGOLAND/mapinit.c), [loaders.c](../../LEGOLAND/loaders.c), [blokelist.c](../../LEGOLAND/blokelist.c)

`LoadPos` only establishes three raw four-byte scalars followed by nine matrix floats; this loader does not interpret the first triple. A float-position interpretation requires further consumer evidence. [loaders.c](../../LEGOLAND/loaders.c)

The remaining schema boundaries are unused bytes in each20-byte BNV vertex, opaque LOC context fields and texture-name capacity, and person/model fields whose meanings are not established by these views. Animation, boat artwork and seat tables declared without values still require their external data. Completed leaf and audio-wrapper contracts do not become partial solely because they call native services; historical matching status is tracked separately. [bnvpath.c](../../LEGOLAND/bnvpath.c), [data3.c](../../LEGOLAND/data3.c), [person3d.c](../../LEGOLAND/person3d.c), [blokeanim.c](../../LEGOLAND/blokeanim.c), [anim2.c](../../LEGOLAND/anim2.c)
