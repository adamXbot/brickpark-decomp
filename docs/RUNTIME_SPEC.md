# LEGOLAND runtime specification

This is the consolidated recovered contract at `origin/main` commit `f22f7cc7fa95f2d5740f89f4b53ae2624cc9e474` (2026-09-05). The source inventory contains **183 C files**; the scope brief's approximate 240 is not the count at this baseline. This document and its linked pages implement the documentation-only [Scope J brief](SCOPE_J_runtime_spec.md).

| Subsystem / specification | Source files | Coverage |
| --- | --- | --- |
| [World, visitors, construction and staff](runtime/world.md) | [35 files](runtime/coverage.md#world): map/object/path, Bloke AI, construction, work orders, currency/power | 27 documented; 8 partial |
| [Saves, profiles and scripts](runtime/persistence.md) | [6 files](runtime/coverage.md#persistence): savegame, savechunks and profile families | 6 partial |
| [Assets, animation, audio and host services](runtime/assets.md) | [30 files](runtime/coverage.md#assets): LLIDB, resource/model loaders, audio and platform helpers | 11 documented; 19 partial |
| [Transport](runtime/transport.md) | [30 files](runtime/coverage.md#transport): boating, jungle cruise, flume, roads, school cars and coaster | 19 documented; 11 partial |
| [Attractions and customer scripts](runtime/attractions.md) | [27 files](runtime/coverage.md#attractions): ride helpers, mechanical rides, themed rides, providers and ride saves | 9 documented; 18 partial |
| [Presentation](runtime/presentation.md) | [55 files](runtime/coverage.md#presentation): input, menus, screens, sprites, terrain and software rendering | 28 documented; 27 partial |
| [Callback registration index](runtime/callbacks.md) | Cross-reference of screen, interfaces, ridesave, castleobj and loaders | All 83 named classes indexed; three alternate library registrations; two declared handlers lack recovered bodies |
| **Total C-file coverage** | [183 individual source rows](runtime/coverage.md) | **94 documented; 89 partial; 0 not yet** |

“Documented” means the recovered contract is consolidated. “Partial” identifies unknown fields, external tables, conflicting interpretations or incomplete behavior; it does not classify binary matching. All source files are accounted for, but this is not a claim that a complete replacement runtime can yet be implemented without further recovery. Each page cites its evidence and names its limits. [Coverage and definitions](runtime/coverage.md)

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
| `logflume.c` LFAnimRefs versus `logflume4.c` LFQueue | Shared record is `{path, head, tail}`. Boat mask1 means moving; run mask2 means splash-active. Cursor and length are separate, and interpolation is piecewise linear. Exact `t == 1` reachability remains unresolved. [Transport](runtime/transport.md) |
| Coaster TrackJoint and TrackNode declarations | TrackJoint is `{direction, float height, node}`. The `0x50` node is a prefix of an allocated `0xa4` piece; overlapping variant tails must not be treated as one full footprint type. [Transport](runtime/transport.md) |
| Space-tower, carousel and plane slot descriptions | Tower car base is `+14`, stride36; old save views use biased bases. Some seat arrays are one-based, explaining biased pointer displacements. Tower dwell body uses200 rather than header180. [Attractions](runtime/attractions.md), [transport](runtime/transport.md) |
| Gold Rush “pan drift” prose versus placement code | The actual target changes and restoration are documented; the abbreviated `-0x50/+0x80` description does not prove cumulative `+0x30` drift. [Attractions](runtime/attractions.md) |
| Cell door labels; PTP visited-map and corner offsets | Cell `+12` is a door field. The PTP visited map is192×192, distinct from the256×256 cell grid; the corner-block byte is at`+0d`, not the older lane's`+1d`. Numeric door comparisons take precedence over rotated prose labels. [World](runtime/world.md), [objdoor.c](../LEGOLAND/objdoor.c), [older door note](lanes/fable-a-objrect.md) |
| PrintItem tree, RenderItem descending order | PrintItem is a sorted doubly linked list with a roving cursor; RenderItem insertion is ascending. [World](runtime/world.md), [presentation](runtime/presentation.md), [print-list lane](lanes/fable-b-workorder3.md) |
| Work-order owner field, worker allocation | Park-funded repair `+24` is a float top-up, not an owner pointer; hired workers use heap allocation despite the broad pooled-Bloke header claim. [World](runtime/world.md) |
| Pending script event and string terminator | Pending state is a list. The reader consumes exactly length bytes and adds NUL; the old length+1 framing claim conflicts, and the writer remains external. [Persistence](runtime/persistence.md), [event lane](lanes/fable-b-savechunks2.md) |
| Profile block and name region | The200-byte block starts at`+43`, not`+a3`. Thirty-name-bytes-plus-flags versus32-name-bytes remains unresolved. [Persistence](runtime/persistence.md) |
| Early asset descriptions versus loader sequence | TSM includes a self-name before tile-set names; both ILF and CSP loaders read offsets. LoadAnim3D and LoadPos describe different animation records; abbreviated `.3d` statements must not merge them. [Assets](runtime/assets.md), [FORMATS.md](FORMATS.md), [RE_CONTEXT.md](RE_CONTEXT.md) |
| LLIDB function names versus bodies | `LLIDB_UnLoadLLSData` tears down object classes; `LLIDB_UnLoadODFData` tears down TSF data; `LLIDB_LoadDataByIndex` reports index errors. Names alone are insufficient to dispatch resource types. [Assets](runtime/assets.md) |
| Rendering/UI headers versus consumers | Scroll units are fixed-point pixels; shade data is separately allocated; overview “busy” is a cache-ready latch. The plain RLE path shares an override bug. Horizontal panel scrolling still applies snapped vertical displacement. [Presentation](runtime/presentation.md) |
| Popup kind`0x10b/0x10c` labels | Ride/queue versus gardener/mechanic work-order descriptions remain inconsistent; payload producers and consumers need a joint follow-up. [Presentation](runtime/presentation.md) |
| ODF loader sequence versus interface-header shorthand | Successful DLL loading bypasses custom registration; failed loading clears the flag and invokes the fallback initializer. Normal-path initializer ownership remains external. [Callbacks](runtime/callbacks.md), [llidb_odf.c](../LEGOLAND/llidb_odf.c) |
| “Tick/Activate/Interact” callback names | Slot usage distinguishes placement`8c`, simulation`a8` and custom rendering`b0`; library index7 is called immediately rather than stored at`a4`. [Callbacks](runtime/callbacks.md) |

## Outstanding recovery boundaries

The main remaining data gaps are external artwork/boat-arc/model tables, complete cafe/waiter/seat and landing tables, input/price/control-position tables, and names or meanings of some record fields. Decoded tables are included where the sources expose their values; declaring a table address does not recover its contents. [Transport limits](runtime/transport.md), [attraction limits](runtime/attractions.md), [presentation limits](runtime/presentation.md), [asset limits](runtime/assets.md)

The main behavioral gaps are legacy stand-in tails, specialized renderer/cursor details, missing external vehicle helpers, script-string writer framing, exact flume endpoint safety, profile name interpretation and popup payload interpretation. The callback register preserves two power-station add handlers with provider names and VAs because this baseline has declarations but no marked bodies. No unresolved claim is promoted to verified compatibility by the documentation checks. [Coverage](runtime/coverage.md), [callbacks](runtime/callbacks.md), [verification](runtime/verification.md)

The source set is intentionally fixed for this document. Subsequent scope branches can change names or conclusions; update the affected page, callback row and coverage boundary together when integrating new evidence. [Scope J work log](runtime/WORKLOG.md)
