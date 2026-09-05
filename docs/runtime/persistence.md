# Saves, profiles and scripts

The byte layouts below describe the recovered 32-bit game format. “Pointer-free” only applies to explicitly converted fields: many raw structures still contain stale pointers or padding. [savegame.c](../../LEGOLAND/savegame.c), [savechunks.c](../../LEGOLAND/savechunks.c), [savegame2.c](../../LEGOLAND/savegame2.c)

## Save-game container

### Data structures and ordered chunks

`profiles/<profile>save<slot>.sav` contains 32 bytes from the literal `00002 LEGOLAND Save Game V0.02 \x1a`, then outer measured block13. A measured block is a four-byte **absolute end file offset**, followed by its payload; it is not a payload length. Save primitives use CRT descriptor `0x006691b0`, return1 only for a complete transfer, and back-patch nested offsets through `0x006691bc`, depth`0x006691fc`. The original loader merely consumes each framing dword and reads payloads positionally. [savegame.c](../../LEGOLAND/savegame.c), [profiles.c](../../LEGOLAND/profiles.c), [saveprof.c](../../LEGOLAND/saveprof.c)

| Block | Payload in stream order | Evidence |
| --- | --- | --- |
| 1 | u32 element count; each `{u32 name length, name bytes, u32 flags & 0x3000e}`; u32 emitted TSF count and length/name pairs | [savegame.c](../../LEGOLAND/savegame.c) |
| 2 | raw map/config `0x44`; width×height raw-sized `0x14` Cell copies, row-major, with object and tile fields converted as below | [savegame.c](../../LEGOLAND/savegame.c) |
| 3 | MapStats `0x3f0`, scroll x/y dwords, edit-mode `0x0c`, scripts, report, currency, button flash `0x24` | [savegame.c](../../LEGOLAND/savegame.c) |
| 4 | u32 visitor count; each pool index u32 + BlokeSave `0x124`; optional BNVPath `0x48` if its saved pointer-presence field is nonzero | [savegame.c](../../LEGOLAND/savegame.c) |
| 5 / 6 | gardeners / mechanics: u32 count + count×WorkerSave `0xdc` | [savechunks.c](../../LEGOLAND/savechunks.c) |
| 7 / 8 | gardener / mechanic orders: count; each raw `0x3c` order + u32 class-name length + raw name + nrects×20-byte Rect | [savechunks.c](../../LEGOLAND/savechunks.c) |
| 9 | castle-placed flag; construction count and raw `0xc00` slots; each ODF element's eight-byte class tag, optional per-visitor visit bytes, class count, instances, riders, class-specific save payload; ride totals `0x200` | [savegame.c](../../LEGOLAND/savegame.c), [ridesave.c](../../LEGOLAND/ridesave.c) |
| 10 | path-square count; each Rect20 + squared distance dword + flags dword | [savegame2.c](../../LEGOLAND/savegame2.c) |
| 11 | occupied construction-slot count; each 12-byte slot with object replaced by save-element index (`SaveBuildSlots`, formerly `SaveBlock11`) | [savegame2.c](../../LEGOLAND/savegame2.c) |
| 12 | length/name for terrain, length/name for optional bridge terrain (zero length absent), count and `0x14` bytes per terrain overlay | [savegame.c](../../LEGOLAND/savegame.c) |
| 13 | wraps blocks1–12 | [savegame.c](../../LEGOLAND/savegame.c) |

The element table is `g_elist` at `0x00669200`, count`0x006691b4`; include elements with referenced bit4, forcing `PATH CONTROL` into the set. Load resolves every name, loads its data, then merges saved flags using `(current & 0xfffcfff1) | saved`. Tile codes save as `((TSF index+1)<<8) | (tile-base)` and decode to base+low byte; zero remains absent. Cell object is converted to an element index only when flags intersect `0x08a8`; otherwise0. [savegame.c](../../LEGOLAND/savegame.c), [profiles.c](../../LEGOLAND/profiles.c)

Block9 instances serialize three four-byte reads at instance offsets `+0c,+0e,+10`, **overlapping** within a `0x14` record. Riders save `{u32 bloke pool index,u16 ride/seat id}`; load recreates links, Bloke and Person3D references. Visit bytes exist only for class types other than0 and2. Ride-specific chunks invoke `+bc` save / `+b8` load; their per-ride records are in [transport](transport.md), [attractions](attractions.md) and [presentation](presentation.md). [savegame.c](../../LEGOLAND/savegame.c), [ridesave.c](../../LEGOLAND/ridesave.c)

BlokeSave is a **selected flattening**, not a verbatim Person3D dump: first `0x9c` bytes collect Bloke execution state, selected element indices and world position; following selected Person3D fields and appearance values complete the `0x124` record. The `savegame.c` header's “whole 0x94-byte Person3D” description is broader than the staged field list. A precise field map follows. [savegame.c](../../LEGOLAND/savegame.c)

| Saved offset | Original source fields / role |
| --- | --- |
| `00/02/04` | Bloke words `0c/0e/10`; pad at06 |
| `08/0c` | element fields `14/18`, save indices or -1 |
| `10..24` | six dwords `1c..30` |
| `28..4f` | ten dwords `34..5b`; element7/order and element8/BNV retain their field roles |
| `50/54/56/58` | tick`5c`, action`60`, flags`62`, byte`64` |
| `5a/5c/5e`, `60..64` | words`78/7a/7c`; bytes`7e..82` |
| `68/6c/70/74` | Bloke fields `88/8c/90/94`; conversions must follow the writer, not assume every one is an element |
| `78/7c/80/82..85` | world`68/6c`, word`70`, direction/phase bytes`72..75` |
| `88..9b` | Bloke dwords`98..a8` |
| `9c`, `a0..a8`, `ac/b0`, `b4/b8` | Person`08`, scale`10..18`, screen`1c/20`, pair`24/28` |
| `bc/c0/c4/c8`, `cc..d4`, `d8/dc/e0` | Person`34/30/38/3c`, Euler rotation`40..48`, `4c/88/54` |
| `e4..107`, `108/10c/110/114` | Person nine dwords`58..78`, `7c/80/8c/90` |
| `118/11c/120` | Person`84`; widened Bloke word`0a`; widened Bloke byte`08` |

This field map follows the staging declaration. The per-plan words are intentionally interpreted by the active state; matrix bits, scratch values and reserved words retain their exact mapped representation. Person scale/Euler rotation and depth-mask roles are reconciled against actual consumers in [core evidence](core-data.md#bnv-vertex-and-person-field-closure). [savegame.c](../../LEGOLAND/savegame.c)

WorkerSave is `0xdc` bytes and uses a different selection. Padding is not a portable contract; the table records where it occurs. [savechunks.c](../../LEGOLAND/savechunks.c)

| Saved offset | Original fields / role |
| --- | --- |
| `00/02/04`, `06` | Bloke words`0c/0e/10`, padding |
| `08/0c`, `10..24`, `28..4f` | uninitialized element fields; Bloke dwords`1c..30`; ten dwords`34..5b` with order at saved`44` zeroed |
| `50/54/56/58/59/5a` | Bloke`5c`, byte`60`, word`62`, bytes`64/7f/82`; pad`55/5b` |
| `5c/60/64/66..69` | Bloke`68/6c`, word`70`, bytes`72..75`; padding`6a..6b` |
| `6c..7f` | Bloke dwords`98..a8` |
| `80`, `84..8f`, `90/94`, `98..a3` | Person`08`, scale`10..18`, screen`1c/20`, Euler rotation`40..48` |
| `a4/a8/ac`, `b0..d3`, `d4/d8` | Person`4c/88/54`, nine dwords`58..78`, widened Bloke word`0a` and byte`08` |

### Load/save behaviour

LoadGame checks only the numeric version prefix (`atoi` after placing NUL at byte5), restores the saved pool slot, rebuilds Person3D instances and ride bindings, then recalculates map render order and entrance origin. It forces edit-mode fields to3 and0 and sets GamePad bit`0x20`. Path-square loading deletes the old list and pushes new nodes at its head, reversing order. `rect.next` is still a stale on-disk pointer; rebuilding PathSquare.next does **not** sanitize that embedded Rect link. [savegame.c](../../LEGOLAND/savegame.c), [savegame2.c](../../LEGOLAND/savegame2.c)

Before worker serialization, workers inside huts (plan5) are unseated; their `0x08|0x20` flags clear. For those hut occupants, action byte`+60 >= 0x64` causes removal; otherwise world position`+68/+6c` is copied from target`+24/+28` and an idle plan is applied. Count is taken after this mutation. WorkerSave contains a selected Bloke/Person subset, with WorkOrder pointer zeroed at saved `+44`. Order chunks encode the assigned worker as its trade-list index; loading those chunks rebuilds both sides of the link and resolves the target class by name. [savechunks.c](../../LEGOLAND/savechunks.c)

`SkipMeasuredBlock` consumes a single four-byte framing value and does not seek past a payload. `LoadIconStateChunk` reads exactlyfour dwords (16 bytes) into the theme-enabled array and returns whether the read succeeded. [tinystubs.c](../../LEGOLAND/tinystubs.c)

### Constants and original bugs

WorkerSave +08/+0c are eight uninitialized stack bytes. Evicted SeatSlots leak. Short reads can leave half-linked worker chains with garbage next pointers; order tails depend on the saved next pointer being0. Allocation failures are unchecked in multiple readers. Version rejection leaks the save descriptor; class tags, terrain writes and construction writes can ignore failure. An eight-byte class tag is not guaranteed NUL-terminated. These are original compatibility hazards; no deterministic value is specified for stack garbage. [savechunks.c](../../LEGOLAND/savechunks.c), [savegame.c](../../LEGOLAND/savegame.c), [savegame2.c](../../LEGOLAND/savegame2.c)

### Callback slots

Save/load are paired ObjDef `+bc/+b8`; class-specific serializers are dispatched after generic instances and riders, not independent top-level chunks. [savegame.c](../../LEGOLAND/savegame.c), [callbacks.md](callbacks.md)

## Script state

### Data structures

ScriptEvent is `0x44`: next`+0`, element`+4`, text`+8`, kind`+c`, flags`+10` (`0x20` means owned text), param`+38`, absolute time`+3c`. ScriptStep is20 bytes with id`+4`, text`+8`, two event-list pointers`+c/+10`; the pending state at `0x00668784` is an **event list**, not one event. Each event node saves raw68 bytes, element-name script string, text script string. An all-zero record except next=-1 terminates the list. [savechunks2.c](../../LEGOLAND/savechunks2.c), [savechunks.c](../../LEGOLAND/savechunks.c)

Script strings are signed length (-1 means null) followed by exactly length bytes; the recovered reader allocates length+1 and adds NUL locally. This contradicts the older `savechunks.c` header's length+1-on-disk statement. The original writer at`0x0046c620` is now recovered: it uses `strlen`, writes−1 for null,0 for empty, and otherwise exactly length bytes without NUL. It returns0 on either write failure and1 on success. [Writer instruction evidence](core-data.md#script-string-writer) [savegame2.c](../../LEGOLAND/savegame2.c), [savechunks.c](../../LEGOLAND/savechunks.c)

### Rules and stream order

Script state inside block3 saves: four icon-enabled dwords (16 bytes total); two128-byte texts; string count/list; elapsed time; byte-count and10 bytes; pending event list; repeated `{step id,event list,event list,string}`; id=-1; one-based current-step index (0 absent). Both save/load refresh `g_script_now=GetGameTimer()` and reset error count; reader/error-tracking paths increment `0x006687a0`, which LoadScripts polls. The string writer itself only returns success/failure and does not increment this counter. Event time saves as time-now and restores by addition; script start reload unusually uses **now+elapsed**, as written. Stored control bytes beyond10 are discarded one byte at a time; <=10 invokes reset hooks first. [savechunks.c](../../LEGOLAND/savechunks.c), [savechunks2.c](../../LEGOLAND/savechunks2.c)

Resetting the script timer assigns current game time. Script-step and event list destruction recurses through next links, freeing the suffix before the current node; it does not iterate, bound depth or detect cycles. [tinystubs.c](../../LEGOLAND/tinystubs.c)

### Constants, bugs and callbacks

The last event sentinel consumes a full `0x44` record without trailing strings. The writer temporarily mutates event time around the write and restores it before checking the result; only interruption inside the write exposes relative live time. Element-name failure can leak the just-read name; allocation of new events and script strings is unchecked. AddHelpMessage uses unbounded formatting into shared scratch. These script dispatch mechanisms are separate from the ObjDef callback matrix. [savechunks2.c](../../LEGOLAND/savechunks2.c), [savegame2.c](../../LEGOLAND/savegame2.c), [sysstubs.c](../../LEGOLAND/sysstubs.c)

## Profiles

### Data structures

Profile files `profiles/Profile%d.txt` and save headers `%dsave%d.sh` hold one packed `0x110` record. Offsets: name region`00..1f`; setting`20`; save flag byte`24`; volume/settings dwords`28/2c/30`; 15-byte stats`34..42`; **200-byte block at43**, ending10a; tail dword10b; final byte10f. `saveprof.c` prose claiming block+a3 is a typo: its declaration and `profiles.c` both put it at43. The disk name region is32 raw bytes. The new-profile editor aliases byte`+1e` as its temporary length and allows a nominal maximum31 characters, producing a self-overwrite defect at lengths30/31; this does not establish two on-disk flag fields. [Editor instruction evidence](presentation-data.md) [profiles.c](../../LEGOLAND/profiles.c), [saveprof.c](../../LEGOLAND/saveprof.c)

Live profile at `0x0080ffa0` differs: speech/music/SFX settings at24/28/2c; tail30; stats34; profile/save slots43/44; saved flag45; block46. Profile list nodes are `0x11c` bytes: next, record, loaded flag, slot. [profiles.c](../../LEGOLAND/profiles.c)

### Rules, constants, bugs and callbacks

Eight profile slots load8→1 and prepend, leaving slot1 first. Reset defaults include setting5 and three values75, zero stats/block, tail dword0 then first byte1; saved byte24 and final10f are not reset. The 200-byte initialization hook is a bare return, so no hidden initialization is inferred. Directory probing can close an invalid find handle. Profile popup callbacks use the general icon system, with new popup27 pixels above its parent, string`0x50`, flag`0x2000`. [profiles.c](../../LEGOLAND/profiles.c), [saveprof.c](../../LEGOLAND/saveprof.c)

## Representation and validation

All seven primary save/profile source files were read for their headers, layout declarations and relevant serializer notes. Exact bulk field maps are complete for container, script events, profiles, path squares and order records; visitor/worker payloads have complete source-offset maps, with state-dependent scratch and reserved bytes preserved explicitly. Model field roles and the temporary profile-name alias are resolved by their consumers; uninitialized stack bytes intentionally have no invented deterministic value. No original save round-trip was run in this documentation-only scope. [savegame.c](../../LEGOLAND/savegame.c), [savechunks.c](../../LEGOLAND/savechunks.c), [savechunks2.c](../../LEGOLAND/savechunks2.c), [savegame2.c](../../LEGOLAND/savegame2.c), [saveprof.c](../../LEGOLAND/saveprof.c), [profiles.c](../../LEGOLAND/profiles.c)

## Current-main save and shared helpers

The new [savemisc2.c](../../LEGOLAND/savemisc2.c) provides18 helper bodies. Its SaveScriptString independently agrees with the recovered original writer. SaveIconStateChunk writes four32-bit values, each the inverse of control-icon flag`0x400`; the earlier “16 bytes” size was correct but a byte-per-icon interpretation was not. LoadBuildSlots reads a count and count×12-byte records, converting each saved object index through `GeteListPtr(index)->data`, then clears only the object field of slots count..255. Neither read, the count bound nor the element pointer is checked.

`CollectUsedTSFTables` scans2048 tile slots, excludes sprite sentinel−1 and null sets, and emits a set whenever it differs from the last emitted set. Thus a repeated set separated by another emitted set appears again; skipped slots do not clear the remembered set. This is run deduplication, not global uniqueness. `RemoveGoals(code)` saves next, unlinks each matching goal and calls its event destructor. `GetObjRiderN` initializes the shared rider cursor and advances at most the class’s signed capacity; it returns the selected zero-based ordinal or0. `SkipStrings` advances over n NUL-terminated strings with no buffer bound. [savemisc2.c](../../LEGOLAND/savemisc2.c)

The remaining shared helpers are texture/image/cache and narration lifecycle operations, consolidated in [assets](assets.md#current-main-texture-and-narration-helpers). These mixed-file cross-references preserve one primary source entry. The new main snapshot is`263cf60b173a8d054e356c5916341cc664033713`.
