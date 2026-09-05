# LEGOLAND runtime specification

This is the consolidated recovered contract at `origin/main` commit `cf8e88c845dc4b109bbb2bd9f2a31d1177a9c9aa` (2026-09-05). The source inventory contains **199 C files**; the scope brief's approximate 240 is not the count at this baseline. This document and its linked pages implement the documentation-only [Scope J brief](SCOPE_J_runtime_spec.md).

| Subsystem / specification | Source files | Coverage |
| --- | --- | --- |
| [World, visitors, construction and staff](runtime/world.md) | [36 files](runtime/coverage.md#world): map/object/path, Bloke AI, construction, work orders, currency/power | 36 documented; 0 partial |
| [Saves, profiles and scripts](runtime/persistence.md) | [7 files](runtime/coverage.md#persistence): savegame, savechunks and profile families | 7 documented; 0 partial |
| [Assets, animation, audio and host services](runtime/assets.md) | [33 files](runtime/coverage.md#assets): LLIDB, resource/model loaders, audio and platform helpers | 33 documented; 0 partial |
| [Transport](runtime/transport.md) | [35 files](runtime/coverage.md#transport): boating, jungle cruise, flume, roads, school cars and coaster | 35 documented; 0 partial |
| [Attractions and customer scripts](runtime/attractions.md) | [30 files](runtime/coverage.md#attractions): ride helpers, mechanical rides, themed rides, providers and ride saves | 30 documented; 0 partial |
| [Presentation](runtime/presentation.md) | [58 files](runtime/coverage.md#presentation): input, menus, screens, sprites, terrain and software rendering | 58 documented; 0 partial |
| [Callback registration index](runtime/callbacks.md) | Cross-reference of screen, interfaces, ridesave, castleobj and loaders | All 83 named classes indexed; three alternate library registrations; both formerly missing power-station add bodies recovered |
| **Total C-file coverage** | [199 individual source rows](runtime/coverage.md) | **199 documented; 0 partial; 0 not yet** |

“Documented” means the recovered contract is consolidated. “Partial” identifies an unresolved data or behavioral contract; it does not classify binary matching. All 199 source contracts and the 63 former partial entries are covered. Opaque reserved storage and original undefined inputs remain explicit. This is documentation coverage, not a claim of tested runtime equivalence. Each page cites its evidence and names its limits. [Coverage and definitions](runtime/coverage.md)

## Reading and implementation conventions

The pages present layouts, behavioral rules, decoded tables/constants, original bugs and callback roles. Most follow that order within each subsystem; presentation groups shared records first and gathers its original bugs in a final reference section. The callback index supplies the complete registration mapping rather than repeating it in every subsystem. [World](runtime/world.md), [transport](runtime/transport.md), [presentation](runtime/presentation.md), [callbacks](runtime/callbacks.md)

Sizes and offsets describe the original 32-bit, little-endian records. A virtual address identifies original evidence; it is not a required browser address. Different local struct names can describe the same storage, and identical offset names can have class-specific meanings. Serialized pointers, padding and uninitialized values are recorded explicitly; they do not acquire invented deterministic values. [Persistence](runtime/persistence.md), [assets](runtime/assets.md), [attractions](runtime/attractions.md)

Prefer the original exported global names when available. The [export-name table](DECOMP.md#global-names-from-the-export-table) maps `lpConfig`, `GameMap`, `MapStats`, `ObjectClassList`, `FirstBloke`, worker lists, scroll fields, render/input globals and video settings to reconstruction aliases. `lpConfig` contains both screen and map dimensions; the scroll integer portion is screen pixels, while walking coordinates use map units. Equal addresses in competing local declarations require checking consumers before merging layouts. [World records](runtime/world.md), [presentation coordinates](runtime/presentation.md)

A bug is part of the recovered behavior, including unchecked allocations, unbounded indices, stale pointers and uninitialized returns. A replacement may deliberately choose safer behavior, but that is a compatibility decision to record separately. A source comment claiming “matched” or a function marker does not establish complete semantics for a stand-in tail. No compiler, original-game run, pixel comparison or save round-trip was performed in Scope J. [Scope restrictions](SCOPE_J_runtime_spec.md), [host/helper limits](runtime/assets.md), [verification record](runtime/verification.md)

## Mechanics quick index

| Contract | Location |
| --- | --- |
| Cell/grid, doors, footprint validation, map loading, path and route costs, visitor thresholds `1000/2400/4000/7000`, work orders and power | [World](runtime/world.md) |
| Save framing and all numbered chunks, selected visitor/worker payloads, script-event lists/strings, packed profiles | [Persistence](runtime/persistence.md) |
| Resource archives, LLIDB, CSP/ILF offsets, COMP, MAP, installer, morph/position/BNV/RIN assets, sound state and host lifecycle | [Assets](runtime/assets.md) |
| `BsBoat 0x3f4`, reserved destinations and preference; flume `{path,head,tail}`, `drop-step+z`, ramp/splash; car manoeuvres; TrackJoint, RK4/vector operations, lap state and landing | [Transport](runtime/transport.md) |
| Rider/seat records, 16 headings, queue/pan offsets, tower derived service state, ride/customer scripts, per-ride chunks | [Attractions](runtime/attractions.md) |
| Report one-based line index, panel slide states, free-play terminator, sprite layouts, shade ramps, z commands and draw ordering | [Presentation](runtime/presentation.md) |
| ObjDef slot roles, inheritance/overwrite order, exact names and original VAs for registered handlers | [Callbacks](runtime/callbacks.md) |

## Reconciled disagreements

This register highlights consequential disagreements; detailed source citations and other local corrections remain beside the affected contract.

| Conflicting evidence | Adopted interpretation / remaining uncertainty |
| --- | --- |
| `logflume.c` LFAnimRefs versus `logflume4.c` LFQueue | Shared record is `{path, head, tail}`. Boat mask1 means moving; run mask2 means splash-active. Cursor and length are separate, and interpolation is piecewise linear. At` t=1`,2-point paths read beyond their endpoints;4-point indexing depends on x87 precision. The function’s exact index rule is recovered; normal-play reachability has not been execution-tested. [Transport](runtime/transport.md) |
| Coaster energy and model-loader aliases | New derivative consumers establish route`+28` as total energy and return parameter-speed/motor-power derivatives; old speed/acceleration names must not replace that physics. Model-loader argument2 returns byte length, correcting older pointer/mode guesses. [Transport](runtime/transport.md) |
| Coaster TrackJoint and TrackNode declarations | TrackJoint is `{direction, float height, node}`. The allocated`0xa4` piece contains graph prefix`0x2c`,footprint`0x14`,world position`0x0c` and inline geometry`0x58` at`+4c`; the position helper returns the inline address, not a stored pointer. [Transport](runtime/transport.md) |
| Space-tower, carousel and plane slot descriptions | Tower car base is `+14`, stride36; old save views use biased bases. Some seat arrays are one-based, explaining biased pointer displacements. Carousel allocation is`0x2c`; its original definition has ten seats at`+20..+29`. Older`0x24` declarations expose only four occupancy bytes. Tower dwell body uses200 rather than header180. [Attractions](runtime/attractions.md), [transport](runtime/transport.md) |
| Gold Rush “pan drift” prose versus placement code | Kneeling changes the target from panY+128 to panY−80 (−208); standing adds128 to current worldY and reissues the saved target. The abbreviated `-0x50/+0x80` description does not prove cumulative `+0x30` drift; the original state7 completion now proves there is **no** exact snap. It restores the saved state within twice-speed radius, preserving endpoint error. [Instruction proof](runtime/core-data.md#movement-arrival-and-coordinate-units) [Attractions](runtime/attractions.md) |
| Category statistics header versus field consumers | Category definition count is`+04`, working count`+00`, occupied cells`+0c`; publication and income use these body-derived meanings. [World](runtime/world.md) |
| Carousel discharge header versus shared helper | GetAllBlokesOffRide always returns1 after changing rider actions/flags, so the zero-revolution tick stops and returns before positioning; it does not wait for physical discharge. [Attractions](runtime/attractions.md), [transport](runtime/transport.md) |
| Ride-save BNV path indices versus blanket sentinel claims | Stored path indices are zero-based:0 selects the first run. Person3D z-sprite binding has its own zero special case. [Attractions](runtime/attractions.md) |
| Temple-slide and restaurant shorthand versus counter operations | Temple-slide allocation offers only lanes0/3 despite testing all four for admission. Restaurant2 postincrement reads indices1..33; index33 aliases the first y slot, and a one-to-zero queue decrement does not clear leaving. [Attractions](runtime/attractions.md) |
| Cell door labels; PTP visited-map and corner offsets | Cell `+12` is a door field. The PTP visited map is192×192, distinct from the256×256 cell grid; the corner-block byte is at`+0d`, not the older lane's`+1d`. Numeric door comparisons take precedence over rotated prose labels. [World](runtime/world.md), [objdoor.c](../LEGOLAND/objdoor.c), [older door note](lanes/fable-a-objrect.md) |
| PrintItem tree, RenderItem descending order | PrintItem is a sorted doubly linked list with a roving cursor; RenderItem insertion is ascending. [World](runtime/world.md), [presentation](runtime/presentation.md), [print-list lane](lanes/fable-b-workorder3.md) |
| Work-order owner field, worker allocation | Park-funded repair `+24` is a float top-up, not an owner pointer; hired workers use heap allocation despite the broad pooled-Bloke header claim. [World](runtime/world.md) |
| Pending script event and string terminator | Pending state is a list. Both original writer and reader consume exactly length bytes; the reader adds NUL locally. The old length+1-on-disk claim is disproved. [Persistence](runtime/persistence.md), [event lane](lanes/fable-b-savechunks2.md) |
| Profile block and name region | The200-byte block starts at`+43`, not`+a3`. The32-byte raw disk-name region coexists with a temporary editor length byte at+1e, whose writes corrupt length30/31 names; it is not evidence for two disk flags. [Persistence](runtime/persistence.md) |
| Narration/header and FX layout shorthand | WAV parsing assumes the first post-WAVE chunk is format, ignores its ID and RIFF size, and does not pad odd chunks. Restaurant FX sample pointers are at`+8`, not the older screen declaration's`+4`. [Assets](runtime/assets.md) |
| Early asset descriptions versus loader sequence | TSM includes a self-name before tile-set names; both ILF and CSP loaders read offsets. LoadAnim3D and LoadPos describe different animation records; abbreviated `.3d` statements must not merge them. [Assets](runtime/assets.md), [FORMATS.md](FORMATS.md), [RE_CONTEXT.md](RE_CONTEXT.md) |
| LLIDB function names versus bodies | `LLIDB_UnLoadLLSData` tears down object classes; `LLIDB_UnLoadODFData` tears down TSF data; `LLIDB_LoadDataByIndex` reports index errors. Names alone are insufficient to dispatch resource types. [Assets](runtime/assets.md) |
| Rendering/UI headers versus consumers | Scroll units are fixed-point pixels; shade data is separately allocated; overview “busy” is a cache-ready latch. The plain RLE path shares an override bug. ZBufferHelper uses two-bit commands (16/dword), RenderTiledSprite exits, and blink mask0x100 produces256ms phases. Horizontal panel scrolling still applies snapped vertical displacement. [Presentation](runtime/presentation.md) |
| Popup kind`0x10b/0x10c` labels | Deletion handlers identify gardener/mechanic orders, and details consumers agree with WorkOrder fields. The conflicting ride/queue label is rejected; the displayed popup branch is dormant: map hits can produce10b/c, but setup resets these kinds without populating the display payload. [Presentation](runtime/presentation.md) |
| ODF loader sequence versus interface-header shorthand | Successful DLL loading bypasses custom registration; failed loading clears the flag and invokes the fallback initializer. The ordinary path invokes no+a4 callback, and the recovered finalizer only patches three Water Works classes. [Callbacks](runtime/callbacks.md), [llidb_odf.c](../LEGOLAND/llidb_odf.c) |
| “Tick/Activate/Interact” callback names | Slot usage distinguishes placement`8c`, simulation`a8` and custom rendering`b0`; library index7 is called immediately rather than stored at`a4`. [Callbacks](runtime/callbacks.md) |
| Visitor AI comments versus original instructions | Line-angle storage changes meaning during turning; plan6 reads an LLIDB element at the class-flags displacement; failed admission can strand plan5; state9 can restore twice and call a leave pointer gated by the enter pointer. Full transitions and fault paths preserve the original operations. [High-level](runtime/ai-data.md), [low-level](runtime/ai-low-data.md) |

## Coverage closure and verification limits

The original 63 partial rows were investigated against the executable and shipped assets. Full numeric/name tables, missing painter/ride helpers, position/model formats, callback finalization, profile editing and script framing now have direct evidence. All 26 high-level AI slots and 16 low-level states now have complete evidence pages, including their helper chains and original faults. The current-main additions are included in the 199-file inventory. No source entry remains partial or unassigned; opaque reserved bytes, original undefined inputs and native host services are described explicitly rather than assigned invented behavior.

Scope J completes the documentation deliverable. Original-game execution, save round-trips, pixel/audio parity and a replacement runtime are separate verification work and were not performed here. The original defects and FPU-dependent paths are specified; their frequency in normal play has not been measured. [Verification](runtime/verification.md)

## Original-data evidence

The recovery pages bind new claims to the original executable digest, exact addresses, interpreted tables and full-path asset manifests. Their published checks validate bytes, displayed values, directory links and complete stream consumption.

- [Core: names, power, cooldowns, saves, character assets and record reconciliation](runtime/core-data.md)
- [Transport: water/boat tables, flume geometry, coaster records and model formats](runtime/transport-data.md)
- [Attractions: seat/queue/landing tables and machine helpers](runtime/attractions-data.md)
- [Presentation: all prices/keys/control positions, painter bodies and graphics streams](runtime/presentation-data.md)
- [High-level AI: all visitor/worker plans, routing, admission and reservation contracts](runtime/ai-data.md)
- [Low-level AI: all movement states, tile callbacks and helper transitions](runtime/ai-low-data.md)
