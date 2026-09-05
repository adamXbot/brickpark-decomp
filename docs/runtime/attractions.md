# Attractions, customer scripts and ride callbacks

This page consolidates the attraction-side runtime contract. Offsets describe the original 32-bit records; hex addresses identify original globals and tables, not portable runtime addresses. `key` means the two-byte `{u8 x,u8 y}` placement square, compared as one little-endian word. `anchor` means that square plus the class's base offsets; world positions use 24.8 fixed point. Unknown padding remains unknown. [rides.c](../../LEGOLAND/rides.c), [mechrides.c](../../LEGOLAND/mechrides.c)

## Coverage

“Documented” means the recovered contract below is consolidated; it does not claim all corresponding C functions match the original binary. “Partial” marks unresolved layouts, absent table contents or unrecovered machine helpers. Mixed transport files are accounted for here and in [transport.md](transport.md); a cross-page description fulfils coverage without implying a source gap. [ridecb3.c](../../LEGOLAND/ridecb3.c), [mechrides.c](../../LEGOLAND/mechrides.c)

| Subsystem | Assigned source files | Status / remaining boundary |
| --- | --- | --- |
| Boarding, experience and shared helpers | [rides.c](../../LEGOLAND/rides.c), [ridemisc.c](../../LEGOLAND/ridemisc.c), [ridemisc2.c](../../LEGOLAND/ridemisc2.c), [ridemisc3.c](../../LEGOLAND/ridemisc3.c) | Documented for recovered contracts; partial external animation tables |
| Mechanical rides | [mechrides.c](../../LEGOLAND/mechrides.c) | Partial: rider machines and layouts recovered; several vehicle-machine helpers remain externs |
| Catapult | [catapult.c](../../LEGOLAND/catapult.c) | Partial: mechanics recovered; external layer/landing table values absent |
| Gold Rush, fort, castle level 1, temple | [goldrush.c](../../LEGOLAND/goldrush.c), [goldrush2.c](../../LEGOLAND/goldrush2.c), [goldrush3.c](../../LEGOLAND/goldrush3.c), [goldrush4.c](../../LEGOLAND/goldrush4.c) | Documented recovered scripts/tables; pan-offset arithmetic reconciled below |
| Joust and temple slide | [joust.c](../../LEGOLAND/joust.c), [joust2.c](../../LEGOLAND/joust2.c) | Partial: recovered rider/cycle mechanics documented; Temple Slide walk/end threshold bytes remain external |
| Carousel, Balloonz, earth slide, restaurants and cafe | [ridecb1.c](../../LEGOLAND/ridecb1.c), [ridecb3.c](../../LEGOLAND/ridecb3.c), [ridecb4.c](../../LEGOLAND/ridecb4.c) | Partial: missing contents of cafe/waiter and some seat tables |
| Western Town | [westtown.c](../../LEGOLAND/westtown.c), [westtown2.c](../../LEGOLAND/westtown2.c) | Documented scripts, door state and overlay ordering |
| Water Works and garden | [waterworks.c](../../LEGOLAND/waterworks.c) | Documented; garden fallback prose reconciled against callbacks |
| Castle and track-class adapter | [castleobj.c](../../LEGOLAND/castleobj.c) | Documented adapter/layout and recovered geometry across this page and [transport.md](transport.md#5-coaster-graph-physics-rendering-and-save) |
| Entrance, mechanics hut and transport callback frontier | [ridecb2.c](../../LEGOLAND/ridecb2.c), [ridecb5.c](../../LEGOLAND/ridecb5.c), [ridecb6.c](../../LEGOLAND/ridecb6.c), [ridecb7.c](../../LEGOLAND/ridecb7.c), [ridecb8.c](../../LEGOLAND/ridecb8.c), [ridecb9.c](../../LEGOLAND/ridecb9.c) | Documented recovered callback contracts, faults and [transport routes/vehicles](transport.md); boating queue coordinates remain external |
| Record, queue and sound helpers | [ridemisc4.c](../../LEGOLAND/ridemisc4.c), [ridetiny.c](../../LEGOLAND/ridetiny.c) | Documented 77 recovered helper bodies; seven machine-step bodies and Tower seat picker remain external |
| Providers and ride save chunks | [interfaces.c](../../LEGOLAND/interfaces.c), [ridesave.c](../../LEGOLAND/ridesave.c) | Documented provider roles and formats; stale field names reconciled below |

## Shared ride model and visitor experience

### Data structures

| Record | Size / fields |
| --- | --- |
| `ObjDef` / ride class | `0xd0` bytes; `+04` placed-instance list; `+0c/+10` base x/y; `+1c` flags; `+20 i16` kind; `+24/+25 i8` queue/exit offsets; `+2e i16` capacity; `+34 i16` ride code; `+36 i16` base attraction; `+38 i16` target trait; `+3c` footprint; `+64` build sprite; `+c4` LLIDB element; `+c8` visit counters; `+cc` rider list. The instance list at `+04` and rider list at `+cc` are distinct. [rides.c](../../LEGOLAND/rides.c), [mechrides.c](../../LEGOLAND/mechrides.c) |
| `RiderNode` | `0x14` bytes: `+00 next`, `+04 prev`, `+08 bloke`, `+0c u16 ride_id`, `+0e` padding, `+10 person`. [rides.c](../../LEGOLAND/rides.c) |
| Bloke fields used by rides | `+04 person`; `+0e u16` low-level state (`0` idle, `7` walking); `+24/+28 i32` target; `+34 i8` walk-path direction; `+35` ride pose; `+36 u8` seat/pan; `+37` occlusion band; `+38 i16` path/frame cursor; `+3c/+3e i16` in-ride offsets; `+40/+42 i16` fort substate/resume; `+4a/+4c i16` animation part/stop; `+50` animation ID or integer-walk-path pointer/ordinal, depending on the ride action; `+54` BNV path; `+58 i32` timer; `+60 u8` ride action; `+62 u16` flags; `+68/+6c i32` world; `+72/+73 u8` facing/new facing; `+98` movement scratch. Signedness of `+38/+4a/+4c` differs between local declarations: the tower animation consumer sign-extends them. [goldrush.c](../../LEGOLAND/goldrush.c), [mechrides.c](../../LEGOLAND/mechrides.c), [ridemisc2.c](../../LEGOLAND/ridemisc2.c), [ridetiny.c](../../LEGOLAND/ridetiny.c) |
| Visitor experience fields | Bloke `+78 u16` stay timer, `+7a u16` mood, `+7c u16` hunger, `+7e u8` trait, `+7f u8` speed, `+80 u8` statistic, `+81` name letter, `+88/+8c/+90` favourite rides, `+94` favourite food. [rides.c](../../LEGOLAND/rides.c) |
| Person controlled by ride | `+1c` screen pair, `+24` local pair, `+2c` z-sprite, `+30 i32` ride-ownership flag, `+3c float` depth; rotation `+40/+44/+48` and matrix `+58` are used by the sixteen-heading helper. [mechrides.c](../../LEGOLAND/mechrides.c), [goldrush2.c](../../LEGOLAND/goldrush2.c) |
| Draw context | `+00 i32 tag=0x103`, `+04 class element`, `+08 u16 square`; the render item's corresponding block starts at `+10`. [ridecb1.c](../../LEGOLAND/ridecb1.c) |
| Sound source / FX entry | Kind-2 source is 16 bytes `{kind@0, unused@4, x@8, y@c}`; kind 1 identifies a bloke at `+04`. FX records are 12 bytes `{name@0, field@4, sample@8}`. [rides.c](../../LEGOLAND/rides.c), [joust2.c](../../LEGOLAND/joust2.c), [waterworks.c](../../LEGOLAND/waterworks.c) |

### Rules and state machines

A placement's riders are filtered from its class-wide list by packed square. Ticks retain the next node before a state can remove the current node. Walking states set a 24.8 target, call `CalcMoveLine`, set low-level state 7, retain the raw direction plus `0x10`, turn using the resulting octant plus 3, then advance the action. State 7 is an AI walking state; references calling it “busy for seven ticks” do not establish a fixed seven-frame journey. Flags `8`, `0x40`, `0x80`, `0x100`, `0x20` respectively mean using the ride, queued, riding, sitting, and hired/worker membership in the relevant handlers. [westtown2.c](../../LEGOLAND/westtown2.c), [rides.c](../../LEGOLAND/rides.c), [ridemisc.c](../../LEGOLAND/ridemisc.c)

Mounting a BNV ride projects world position using `sx=(wx-wy)*tile_w>>9`, `sy=(wx+wy)*tile_h>>9`, adds the render origin, subtracts scroll, subtracts the halved ride pivot and building screen origin, then doubles the result for the 3D path origin. The person receives the ride z-sprite, ownership flag and depth. Dismounting clears ownership/z-sprite and uses `UnAdjustBlokePosition` plus `ScreenToMapRef` to return to map movement. This is distinct from the integer walk-path subsystem used by Gold Rush and Copters. [ridecb3.c](../../LEGOLAND/ridecb3.c), [goldrush3.c](../../LEGOLAND/goldrush3.c)

Removing a rider unlinks/frees its node; if it was the last rider at that placement, cell flag 4 is cleared. Food kind 5 uses mood event 9 for favourite food or 10 otherwise and resets hunger; other attractions use 11 for favourites or 12 otherwise. The visit count times 50 is added to the stay timer, previous item becomes current item, the visit counter increments, and long-term action becomes 23. Bulk eviction additionally returns the visitor to the base-plus-placement square, resets ride rendering, restores walking and kills its samples. The per-instance admission flag is bit 2 at instance `+0c`; closing/opening manipulates that bit. [rides.c](../../LEGOLAND/rides.c)

`GetAllBlokesOffRide` walks the class list, selects the placement, and for each rider without queued flag `0x40` sets flag8 and increments the action. It always returns1; it does not poll physical disembarkation. `PutWorkerOnRide` allocates/zeros a `0x14` node, copies the Bloke and Person pointers and packed placement, sets worker flag `0x20`, then links the node; failed allocation leaves the worker unchanged. [rides.c](../../LEGOLAND/rides.c)

Record searches return the first packed-square match or null, covering Barrels, Plane, Tower, Spider, Safari, Copters, Gold, Earth Slide and both restaurants. Constructors allocate and zero their full layouts, set the square, and prepend to the appropriate head. Barrels, Plane, Tower, Spider, Safari and Carousel then call their stop/reset helper on success; Gold has no further reset. Restaurant1 redundantly clears its three seats and frame; Earth Slide initializes state `+04=1` and clears frame/auxiliary fields. Copters alone calls its initializer even if allocation failed. [ridemisc4.c](../../LEGOLAND/ridemisc4.c), [ridetiny.c](../../LEGOLAND/ridetiny.c), [codex-d.md](../lanes/codex-d.md)

The nine free-all helpers for Plane, Barrels, Tower, Spider, Safari, Gold, Copters, Carousel and Balloonz repeatedly remove the **live head**, reading it again after removal. The seven machine-list ticks for the six mechanical rides and Earth Slide instead read each record’s **live next link after** its step callback; they do not cache next in advance as rider-removal loops do. The per-record step bodies remain external in this baseline. `NthRiderNodeIndex` returns a zero-based ordinal when found and the list length when absent. [ridetiny.c](../../LEGOLAND/ridetiny.c)

`SetSampleLooping` ORs sample flags `+1c` with `0x20`; `Copters_ResumeSFX` calls the single-sample resume helper and always returns0. `Fountain_InitSound` and `PowerStation_InitSound` post-increment their reference counters and load the relevant FX list only when the old count was0. These are separate resources from the Water Works shared table. [ridetiny.c](../../LEGOLAND/ridetiny.c)

### Tables and constants

| Rule | Recovered values |
| --- | --- |
| Ride enjoyment | Penalty `max(3*abs(target_trait-trait)-10,0)`; result `(ride_code-penalty+200)>>visits`. [rides.c](../../LEGOLAND/rides.c) |
| Viewed ride code | Same penalty clamped to 100; first visit `base_attract+100-penalty`, later `base_attract+(4-(1<<visits))*25-penalty`. [rides.c](../../LEGOLAND/rides.c) |
| Attraction score | Penalty is `max(target-trait-10,0)` when target is higher, otherwise `max(2*(trait-target)-10,0)`; same first/repeat base. Age 0 rejects food with `-100`; age 1 rejects food when not viewing; age 2 floors food at 20 and doubles when viewing; ages 3/4 give nonfood 0, set food scores below 20 to 50, then multiply food by 6 when viewing or 4 otherwise. [rides.c](../../LEGOLAND/rides.c) |
| Initial AI | Speed `12..24`, mood `10..50`, trait cast to byte from `Rand_Tween(0,140)-20`, statistic `5..10`; hunger uses configured maximum, stay uses park metric; name letters cycle A..Z; three favourite rides, one food; long-term action 2. Endpoint semantics follow the random helpers. [rides.c](../../LEGOLAND/rides.c) |
| Favourite eligibility | LLIDB `type_flags & 0x14 == 0x14`; food kind 5, ride kind neither 0 nor 5. Scan wraps from random start and returns 0 only after a full no-match circuit. [ridemisc.c](../../LEGOLAND/ridemisc.c), [fable-b-ridemisc.md](../lanes/fable-b-ridemisc.md) |

| Additional helper resource | Values |
| --- | --- |
| Fountain | Reference count `0x00667114`; five FX entries starting `0x004b8710`. [ridetiny.c](../../LEGOLAND/ridetiny.c) |
| Power station | Reference count `0x00667118`; two FX entries starting `0x004b8750`. [ridetiny.c](../../LEGOLAND/ridetiny.c) |
| Constructors outside the mechanical table | Restaurant1 `0x0c`, Earth Slide `0x24`, Carousel full allocation `0x2c`, Gold `0x2c`. [ridemisc4.c](../../LEGOLAND/ridemisc4.c) |

### Original bugs and compatibility decisions

Favourite selection does not advance the cursor after accepting a candidate: the random “Nth match” counter repeatedly consumes the same candidate, returning the first eligible class. A zero `rand()&0x1f` skips the scan and returns an uninitialised element pointer. Bounds-checked cell lookups in admission/last-rider removal are followed by unchecked dereferences; out-of-map squares are not safely handled by those functions. Kind-2 sound sources leave `+04` uninitialised. These are recorded original behaviours, not sensible defaults for a new runtime. [ridemisc.c](../../LEGOLAND/ridemisc.c), [rides.c](../../LEGOLAND/rides.c), [mechrides.c](../../LEGOLAND/mechrides.c)

The new zero-offset-next removers for Barrels, Restaurant1, Restaurant2, Carousel and Balloonz preserve both the empty-head dereference and unconditional freeing of the supplied pointer even if no matching predecessor was found. The free-all wrappers require each removal to update the head; their loop supplies no independent termination guard. [ridemisc4.c](../../LEGOLAND/ridemisc4.c), [ridetiny.c](../../LEGOLAND/ridetiny.c)

### Callback roles

Every handler receives the class LLIDB element first, then recovers its ObjDef. The operational mapping is `+8c` select/build UI, `+90/+94` placement cursor/query work, `+98` place, `+9c` remove, `+a0` draw descriptor, `+a4` load resources, `+a8` per-frame simulation, `+ac` free resources, `+b0` custom overlay, `+b8/+bc` load/save. ObjDef flag `0x20` arms `+a8`, `0x400` arms `+a0`, sprite flag `0x2000` arms `+b0`. Early labels “activate”, “interact” and “tick” are names, not proof of caller semantics. [interfaces.c](../../LEGOLAND/interfaces.c), [joust.c](../../LEGOLAND/joust.c), [ridecb1.c](../../LEGOLAND/ridecb1.c)

## Mechanical rides: Safari, Spider, Barrels, Space Tower, Plane and Copters

### Data structures

All six records are singly linked and keyed by placement. The common counters distinguish joined, seated this cycle, and riders still aboard; the last departure resets admission. [mechrides.c](../../LEGOLAND/mechrides.c)

| Class / size | Head | Key; next | Recovered fields |
| --- | --- | --- | --- |
| Safari `0x28` | `0x004cbf0c` | `+00`; `+10` | `+04 i32 seated`, `+08 i32 riders`, `+0c i32 frame`, `+14/+18/+1c i32 flags/revolutions/half`, `+20 i32 joined`, `+24 i32 timer`. [mechrides.c](../../LEGOLAND/mechrides.c), [ridemisc4.c](../../LEGOLAND/ridemisc4.c) |
| Spider `0x30` | `0x004cbf58` | `+00`; `+2c` | `+02/+03 u8 seated/riders`, `+04 i8 frame`, `+08 u32 flags`, `+10 i32 cycle`, `+14 u8 joined`, `+18 i32 timer`, one-based seat addressing `+1b+seat`, sixteen-byte physical array `+1c..+2b`. [mechrides.c](../../LEGOLAND/mechrides.c), [ridemisc4.c](../../LEGOLAND/ridemisc4.c), [ridetiny.c](../../LEGOLAND/ridetiny.c) |
| Barrels `0x34` | `0x0062fe08` | `+04`; `+00` | `+06/+07 u8 seated/riders`, `+08 i8 layer3 frame`, `+0c u32 flags`, `+18 u8 joined`, `+1c i32 timer`, `+20 i8 layer2 frame`, one-based seat addressing `+20+seat`, physical storage `+21..+33` (19 bytes; capacity is a separate parameter). [mechrides.c](../../LEGOLAND/mechrides.c), [ridemisc4.c](../../LEGOLAND/ridemisc4.c), [ridetiny.c](../../LEGOLAND/ridetiny.c) |
| Plane/Zoomer `0x24` | `0x0062fe9c` | `+00`; `+20` | `+02/+03 u8 seated/riders`, `+04/+05 i8 layer1/2 frame`, `+08/+0c/+10 i32 flags/revolutions/half`, `+14 u8 joined`, `+18 i32 timer`; one-based seat addressing `+1b+seat`, four-byte physical array `+1c..+1f`. [mechrides.c](../../LEGOLAND/mechrides.c), [ridemisc4.c](../../LEGOLAND/ridemisc4.c) |
| Copters `0xd8` | `0x004c11b4` | `+00`; `+04` | `+02/+03 u8 seated/riders`, `+08 u32 flags` (`1` running, `0x4000` filling), `+0c u16 mode`, six `0x20`-byte seat blocks at `+18`; each has flags `+00`, frame `+04`, rider node `+18`, flight stage `+1d`. [mechrides.c](../../LEGOLAND/mechrides.c), [ridemisc.c](../../LEGOLAND/ridemisc.c) |
| Tower `0xb4` | `0x0062fda8` | `+00`; `+08` | `+02/+03/+04 u8 seated/riders/joined`; flags `u32 +0c`, cycle `i32 +10`; four cars `+14..+a4`; eight occupancy bytes `+a4..+ab`; `+ac/+ad i8 layer3/5 frames`; `+b0 i32 timer`. [bswater3.c](../../LEGOLAND/bswater3.c), [ridetiny.c](../../LEGOLAND/ridetiny.c) |

A copter’s `0x20`-byte seat block additionally names `+08/+0c i32 layer_a/layer_b` and `+10/+14 i32 sprite_a/sprite_b`; flag bit0 selects the B pair, otherwise A. The common drawer uses the same signed frame for the layer and cached sprite. [joust2.c](../../LEGOLAND/joust2.c)

Each tower car is **36 bytes (`0x24`)**: `+00 i32 flags` with mask `1` (bit 0) meaning in service, `+08 i32 height`, `+0c i32 state` (0 idle, 2 loaded), `+14 i32` reset with height, `+18/+1c` two RiderNode pointers; other bytes remain unnamed. `ridesave.c` describes an overlapping serializer view beginning at record `+24`, with relative pointers `+08/+0c`: this reaches the same absolute rider pointers (`+2c/+30`, then stride `0x24`) but is not the actual car start. [bswater3.c](../../LEGOLAND/bswater3.c), [bswater2.c](../../LEGOLAND/bswater2.c), [ridesave.c](../../LEGOLAND/ridesave.c)

### Rules and state machines

Safari, Spider, Plane and Barrels mount people on named BNV curves, count seats, run, then use a separate dismount curve. Safari uses `manbox%02d`; Spider uses the same name family; Barrels uses `BoxBloke%02d`. Plane's on/off limits are 63/32 frames; Spider uses per-seat limits. Barrels leaves the boarding path alive until the alighting path replaces it. A capacity match starts the machine; a dwell timer permits less-than-full dispatch. The header's broad “180 frames for the tower” conflicts with the tower rider body's **200-frame** timer; retain 200 for that join action. Safari/Spider/Plane/Copters use 180; Barrels 400. [mechrides.c](../../LEGOLAND/mechrides.c)

| Ride | Rider actions and dispatch order |
| --- | --- |
| Safari | Machine tick first. `0` join/180 timer/clear walking path; `1` mount ON BNV `manbox<seat/2+1>`; `2` advance to path end or per-seat limit; `5` sit/count seated/dispatch at capacity; `7/8` mount and advance OFF BNV; `13` restore map movement and walk to queue offset; `14` leave/reopen. Actions `3,4,6,9..12` hold. [mechrides.c](../../LEGOLAND/mechrides.c) |
| Tower | Machine tick first. `0` join/claim seat/200 timer/walk below base; `1` clear pose words; `2,7` wait for animation completion; `3` walk to seat and face car; `4` sit, count seated, dispatch at capacity; `5` riding hold; `6` clear riding/free seat/walk pose; `8` exit to centred queue offset; `9` remove and reopen after last departure. Missing record stops the rider walk. [mechrides.c](../../LEGOLAND/mechrides.c) |
| Copters | Machine tick first. `0` claim slot and 180 timer; `1/2` board/follow flight path; `3` advance; `4` count seated/full; `5` sit and hold for machine; `6` walk pose; `7` advance; `8/9` reverse-path descent; `10` walk to own square centre; `11` leave/reopen. [mechrides.c](../../LEGOLAND/mechrides.c) |
| Barrels | `0` join/walk five tiles above base; `1/2` boarding BNV; `6` seated/full; `8/9` alighting BNV; `15` release slot/restore world movement; `16` exit two tiles east, with a random half-tile west jitter; `17` leave. Vehicle tick runs after the riders. [mechrides.c](../../LEGOLAND/mechrides.c) |
| Spider / Plane | `0` join and mount immediately; `1` boarding BNV; `5` seated/full; `7/8` alighting BNV; `13` release slot/z-sprite and convert back to map; `14` leave. Plane's machine tick follows the rider walk. Unlisted states are holds/defaults, not automatic advances. [mechrides.c](../../LEGOLAND/mechrides.c) |

Tower in-service state is **derived from the eight seat bytes** when `SpaceTower_CountSeated` runs: clear every car's service bit/state, then for each occupied seat re-enable car `seat>>1`, set state 2, and zero height/`+14`. Cars do not independently accumulate a persistent “in service” decision. Draw the lower body for every car; only a serviced car positions/draws its riders, then applies its upper matte. Height is subtracted from each cached screen anchor. [bswater3.c](../../LEGOLAND/bswater3.c), [bswater2.c](../../LEGOLAND/bswater2.c)

Tower waiting animations use Bloke part `+4a`, frame `+38`, stop `+4c`, and animation table `0x004b775c`. Each part has a frame count and step array. A 16-byte step stores whole and fractional x/y deltas; `Anim3D_OffsetAt` sums all steps through the current part/frame in 24.8 units. End-of-part resets the frame and advances the part; stop/end advances the ride action. The accumulated offset plus base square supplies the walk target. [ridemisc2.c](../../LEGOLAND/ridemisc2.c), [bswater3.c](../../LEGOLAND/bswater3.c)

Copters has six serialized seat records but the recovered flight/dispatch/draw loops operate on **five** copters. Full dispatch copies seated into riders, clears seated, sets mode 2/running, then handles slots in order `1,0,2,3,4`, setting flight stage 3 and frame 0. Start sound is followed by a paused loop resumed after `0xb54` milliseconds. Vehicle drawing orders slots `0,2,3,4,1`; the lower-level drawer resets three layers (`layer_a`, `spr_a`, `layer_b`) even though `spr_a` elsewhere indexes the sprite table. [ridemisc.c](../../LEGOLAND/ridemisc.c), [joust2.c](../../LEGOLAND/joust2.c)

Plane, Spider and Barrels choose `i=rand()%capacity`, scan occupied slots forward with wrap, mark physical slot i, and store/return seat ID `i+1`. The signed-byte capacity is caller-supplied. Safari’s seat helper instead counts matching-placement riders before the target and stores/returns that zero-based ordinal; Copters counts the same way but only returns it. Both ordinal helpers return0 if the target is absent. `SpaceTower_TakeSeat` finds the placement, calls the external eight-seat picker, stores its zero-based result as Bloke animation ID `+50`, and writes the low word of the corresponding stop-part entry to `+4c`; absent placement is a no-op. [ridemisc4.c](../../LEGOLAND/ridemisc4.c), [codex-d.md](../lanes/codex-d.md)

Full/dispatch helpers have distinct reset contracts. Plane and Safari copy seated to riders, clear seated and boarding flag `0x4000`, set running1, zero frame/half and loop FX entry0 with arguments `(1,1)`. Spider makes the same count/flag changes, zeroes frame/cycle and calls its sound starter. Barrels changes only the two counts and those flag bits. Tower copies seated to riders without an explicit seated-clear, sets running1, clears cycle, derives car state with `SpaceTower_CountSeated`, then starts sound. Tower/Spider square-release helpers fade their kind2 source with `-200`. [ridemisc4.c](../../LEGOLAND/ridemisc4.c), [ridetiny.c](../../LEGOLAND/ridetiny.c)

Copters actions1/8 first install a forward/reverse integer walk path, then `Copters_StepRider` scans six table words and replaces Bloke `+50` with the matching ordinal, or `-1` if absent. Actions2/9 call the misnamed `WalkPath_IndexOf`, which simply reads that field, and index the global table to recover the actual path for advancement. Thus the same field holds a pointer before conversion and an integer during flight; the helper is part of live riding, despite the lane’s “save ordinal” label. [ridetiny.c](../../LEGOLAND/ridetiny.c), [mechrides.c](../../LEGOLAND/mechrides.c), [scope-e.md](../lanes/scope-e.md)

### Tables and constants

| Table / property | Decoded contract |
| --- | --- |
| Mechanical resource globals | Safari/Spider/Barrels/Tower/Plane/Copters ObjDefs: `0x4cbec4,0x4cbf20,0x62fde4,0x62fd74,0x62fe58,0x4c1198`; build sprites: `0x4cbec8,0x4cbf28,0x62fde0,0x62fd60,0x62fe7c,0x4c1138`; draw descriptors: `0x4cbed0,0x4cbf40,0x62fdb0,0x62fd48,0x62fe60,0x4c1170`. FX tables/counts: Safari `0x4b4cb8/1`, Spider `0x4b4d88/1`, Tower `0x4b7618/1`, Plane `0x4b79d0/2`, Copters `0x4b4140/4`; Barrels has no FX list in this header. [mechrides.c](../../LEGOLAND/mechrides.c) |
| Tower body layers | Cars 0..3 → `6,4,0,2`; anchors cached at `0x0062fd88..0x0062fda4`. [bswater3.c](../../LEGOLAND/bswater3.c) |
| Tower seat-matte table `0x0062fd64` | `{NULL, "SpaceTower Seat2 Matte.lls", "SpaceTower Seat3 Matte.lls", NULL}` after loading. Globals called `g_spacetower_state`/`phase` in `mechrides.c` are entries 0/3 of this pointer table, not machine counters. [bswater3.c](../../LEGOLAND/bswater3.c) |
| Tower geometry/animation tables | Seat offsets: eight-byte rows at `0x004b77e8`; car facing rows: twenty bytes at `0x004b77a8`; these geometry rows remain external. Animation table `0x004b7758` has eight-byte `{i32 stop_part, animation pointer}` rows with stop parts `{4,3,4,3,2,1,2,1}`; `g_bloke_anim_ref@0x004b775c` is the shifted pointer-column view, not the row base. [mechrides.c](../../LEGOLAND/mechrides.c), [ridemisc4.c](../../LEGOLAND/ridemisc4.c), [codex-d.md](../lanes/codex-d.md) |
| Safari boarding limit `0x004b4cc4` | `{66,66,47,47,80,80,48,48}`; alighting limit table `0x004b4ce4` is identified without literal values. [mechrides.c](../../LEGOLAND/mechrides.c) |
| Spider boarding limit `0x004b4d9c` | `{0,32,28,20,24,12,16,20}`; exit limit table `0x004b4ddc` is identified but not decoded in these notes. [mechrides.c](../../LEGOLAND/mechrides.c) |
| BNV resources | Safari `{Safarirun, Safarion, Safarioff}.bnv`, z-sprite `z_Safari.lls`; corresponding three-path-plus-z-sprite sets for Spider and Barrels. Saved “sample” indices into these tables select BNV assets, not WAVs. [mechrides.c](../../LEGOLAND/mechrides.c) |
| Pivots / draw order | Safari rider offset `(-41,-95)`; Barrels pivot comes from layer 3 minus `(0x67,0x37)`. Barrels renders layer 3/riding people → actions `0,1,15` → front matte → layer 2 → actions `16,17` → second matte, using blit mode 0. [mechrides.c](../../LEGOLAND/mechrides.c) |
| Copters depth bands | Relative row gap `>=5 → band1`, `3..4 → band3`, `0..2 → band0`; interleave layers `0,2`, bands `2,1`, layers `3,4`, bands `3,4`, layer `1`, band `0`, then base matte. [mechrides.c](../../LEGOLAND/mechrides.c) |

Tower rendering separates the two queue animations identified by the `0x004b775c` animation-reference table: one group precedes the lower car layers/tower body, the other follows it; layers5 and3 then receive the two record frame bytes. Plane paints action13 customers before its body and action14 after, with riding people between its moving layers1 and2. Spider adds swing x/y only for riders with pose byte `+35==1`; the moving arm/car and separate front/back sprites surround those people. These draws can write sprite animation headers while painting, so the old mechanical header’s claim that rendering never changes any state is too broad. [mechrides.c](../../LEGOLAND/mechrides.c)

The Copters ordinal scan reads six words starting at `0x004c1124`: the five path entries through `0x004c1134`, then the build-sprite pointer at `0x004c1138` as entry5; `0x004c113c` is the exclusive end. Its forward/reverse path switch still assigns only copter indices0..4. Plane/Safari looped FX entry0 resolves to sample addresses `0x004b79d8`/`0x004b4cc0`. [ridetiny.c](../../LEGOLAND/ridetiny.c), [ridemisc4.c](../../LEGOLAND/ridemisc4.c), [scope-e.md](../lanes/scope-e.md), [codex-d.md](../lanes/codex-d.md)

### Original bugs and unresolved claims

All record unlinks in this family assume a nonempty head before reading `head->next`; empty-list removal faults. Safari does not unbuild when its record is absent. Copters fades sound using the record after freeing it, and also dereferences null when no record exists. Tower teardown clears the class rider head without freeing its nodes. Copter path selection above index 4 leaves its result uninitialised. Its render bucketing scans all class riders without filtering placement, so copies can draw each other's people. Tower's body-layer switch has no default for car indices outside 0..3. Plane collects at most four people in its unbounded local array; Spider and Barrels use sixteen-element arrays (the older Barrels comment says fifteen). [ridemisc3.c](../../LEGOLAND/ridemisc3.c), [mechrides.c](../../LEGOLAND/mechrides.c), [bswater3.c](../../LEGOLAND/bswater3.c)

Safari builds the BNV path name by writing the seat digits directly into the `manbox??` string at `0x004b4cac`; the original relies on that literal being writable. The mechanical BNV seed holds three integers, but these mount paths write only x/y and leave z uninitialised. [mechrides.c](../../LEGOLAND/mechrides.c)

The apparent Spider/Plane/Barrels seat overlap is now directly resolved by the recovered allocator bodies: physical slot0 produces valid ID1, and release’s `+1b+seat`/`+20+seat` reaches that slot. Only invalid ID0 reaches the preceding timer/frame. Allocation has no guard for signed capacity0 (division by zero), negative/oversized capacity (invalid indexing), or a full array (infinite scan). Copters record construction also calls its initializer with null on allocation failure, and that callee writes through the pointer. The ordinal path scan accepts the sixth non-path word and returns `-1` on failure without protecting the caller’s later table index. [ridemisc4.c](../../LEGOLAND/ridemisc4.c), [ridetiny.c](../../LEGOLAND/ridetiny.c), [mechrides.c](../../LEGOLAND/mechrides.c), [codex-d.md](../lanes/codex-d.md)

### Callback roles

The six providers live in `interfaces.c`. `*_Select`/`*_Tick` at `+8c` prepare placement; `*_Add` allocates records after generic placement; `*_Remove` removes records/footprint/riders; `*_Create` loads assets and flags; `*_Activate` or `SpaceTower_TickRiders` runs simulation; `*_Interact` draws. Tower and Copters also hide separately drawn vehicle layers in `+a0`. The save/load functions are in `ridesave.c`. [interfaces.c](../../LEGOLAND/interfaces.c), [mechrides.c](../../LEGOLAND/mechrides.c), [ridesave.c](../../LEGOLAND/ridesave.c)

## Catapult

### Data structures

`CatapultRec` is `0x3c` bytes, head `0x004c1118`: packed square `+00`, next `+04`, `i32 machine state +08`, signed idle/firing frames `+0c/+0d`, four RiderNode pointers `+10..+1c`, four shown bytes `+20..+23`, four signed throw-frame bytes `+24..+27`, four fire integers `+28..+34`, unnamed integer `+38`. The header calls `+24` unused, but the later `Catapult_StartSeatFlight`/`Catapult_StepSeatAnim` bodies explicitly use those four frame bytes. The saved seat indices refer to the class **rider** list, correcting `ridesave.c`'s “instance” label. [catapult.c](../../LEGOLAND/catapult.c), [ridesave.c](../../LEGOLAND/ridesave.c)

### Rules and state machines

Tick all machines first, then riders. Rider actions are `0` take a randomly selected free seat and walk to it, `1` arm that seat, `3` walk from the landing area, `4` leave; action 2 is a hold. The idle frame wraps at 16; arm and seat animations stop at 32. Arming sets a `3..34`-tick initial wait and throw counter 3. When the wait expires it resets to `50..69`, decrements the throw counter, fires on `rand()%100 <=45`, decrements the counter **again**, and advances/releases the rider when it is nonpositive. Machine firing starts only when idle. [catapult.c](../../LEGOLAND/catapult.c)

### Tables and constants

Four arm layer numbers are at `0x004b40a4`, four per-seat 24.8 y offsets at `0x004b40b4`, and four twelve-byte FX entries are resolved through `0x004b40d0`. The headers do not decode their individual values. Seat approach is base-plus-placement x minus `0xe0` with `±0x10` jitter, and y plus the seat table with `±0x20` jitter. Sound slots 0..2 are random seat throws; slot 3 starts the main arm. [catapult.c](../../LEGOLAND/catapult.c)

The firing-arm step fetches layer1’s sprite and discards the result before advancing its frame. This is a recovered redundant call, not an additional animation or resource dependency. [catapult.c](../../LEGOLAND/catapult.c)

### Original bugs

Free-seat scanning wraps forever if all four seats are occupied. The unlink reads a null head's next pointer on empty-list removal. Resource loading tolerates a missing definition/sprite in its initial guards but subsequently hides layers on the stale cached sprite. The throw count's double decrement is retained as the explicit body behaviour. [catapult.c](../../LEGOLAND/catapult.c)

### Callback roles

`Catapult_Select` (`+8c`), `Catapult_Place` (`+98`), `Catapult_GetDrawDesc` (`+a0`) supersede the older extern names `Catapult_Tick`, `Catapult_Add`, `Catapult_Draw`. `Catapult_Activate` simulates; `Catapult_Interact` draws the selected machine layer, four arm layers and local riders. Removal unbuilds/evicts before looking up the record, unlike the other mechanical rides. [catapult.c](../../LEGOLAND/catapult.c), [interfaces.c](../../LEGOLAND/interfaces.c)

## Gold Rush, fort, castle level 1 and temple

### Data structures

Gold Rush owns a `0x2c`-byte placement record, head `0x004c1204`, square `+00`, next `+0c`, six pan occupancy slots identified at `+14`. The other three classes have no corresponding placement machine list in this family. A walk path is an eight-byte `{i32 count, nodes*}` header and `count` twelve-byte `{i32 x,y,state}` nodes; a source polyline is `{i32 segment_count, Pos* deltas}`. Bloke `+34` is signed direction, `+38` current node; fort additionally uses `+40` substate, `+42` return substate and `+58` timer. [goldrush.c](../../LEGOLAND/goldrush.c), [goldrush3.c](../../LEGOLAND/goldrush3.c), [goldrush2.c](../../LEGOLAND/goldrush2.c)

### Rules and state machines

Gold Rush claims a pan, closes admission when appropriate, walks the same path out and home, pans, releases the slot, then leaves. The actions are: `0` claim/start path; `1` follow path and carry a pan at raw square `(x+5,y-3)`; `2` advance then walk to `(x+4.5,y-1.5)`; `3` pose at pan; `4` move to edge; `5` kneel and roll `15..45` panning ticks; `6` run pan animation until expiry, then clear `0x100`, release slot and refresh admission; `7` stand; `8` pose; `9` return to the fixed `(x+4.5,y-1.5)` waypoint; `10` start path at its last node; `11` reverse path and stop carrying the pan at the creek; `12` walk to anchor centre; `13` leave. Path nodes are tile corners: target `(anchor+node)<<8`; leaving either end advances the action. [goldrush.c](../../LEGOLAND/goldrush.c), [goldrush3.c](../../LEGOLAND/goldrush3.c), [ridemisc3.c](../../LEGOLAND/ridemisc3.c)

Fort first walks to the gate `(anchor.x-4,anchor.y)`, then delegates to the fort substate machine, then walks to raw placement centre, anchor centre and exits. Substate 1 waits until its timer expires; 2 chooses a random point in the interior and becomes 3; 3 rolls `rand()&3`: 0/1 choose one of 16 facings and wait `3..18` ticks before resuming 3, 2 chooses another point, 3 advances the rider action. The original `goldrush.c` first-pass substate descriptions are superseded by the implementation and lane note. The fort advances layer 2 and copies its frame onto its mask. [goldrush2.c](../../LEGOLAND/goldrush2.c), [fable-a-goldrush2.md](../lanes/fable-a-goldrush2.md), [goldrush.c](../../LEGOLAND/goldrush.c)

Temple's walk is a fixed out-and-back sequence relative to the anchor: `(-2,-4)`, `(-2,-6.5)`, `(-2.375,-8)`, `(-2,-9.5)`, `(-2.375,-12)`, then the reverse through `(-2,-9.5)`, `(-2.375,-8)`, `(-2,-6.5)`, `(-2,-4)`, centre `(0.5,0.5)`, then leave. Castle level1 actions are `0` gate `(-7,-1)`; `1` the same gate with independent x/y jitter; `2` choose idle `1..256`, facing `rand()&7` and wander timer `16..47`; `3` decrement idle and advance at zero, reroll facing whenever wander expires; `4` doorway `(-6.5,-0.5)`; `5` anchor centre `(0.5,0.5)`; `6` leave. The header calls the last waypoint the raw square, but the body uses the base-adjusted anchor. Its irregular jitter mapping is recorded below. All four set/clear the shared “using ride” flag and draw people behind building mattes. [goldrush.c](../../LEGOLAND/goldrush.c)

Gold pan claim selects the first zero one of six DWORD slots, writes1 and stores its zero-based index in the rider; no record or no free pan leaves the rider unchanged. Release clears the indexed slot if the placement exists, without checking the seat index. `GoldRush_RollPanTimer` sets `rand()%31+15` (15..45); `GoldRush_UpdateFullFlag` closes admission when the external free-pan predicate is false and reopens otherwise. [ridemisc4.c](../../LEGOLAND/ridemisc4.c), [ridetiny.c](../../LEGOLAND/ridetiny.c)

### Tables and constants

| Table | Values / meaning |
| --- | --- |
| Gold walk deltas `0x004b45e0` | `(-2,0), (0,-6), (-6,0), (0,1), (3,0)` tiles; polyline descriptor at `0x004b4608`; builder allocates 12 bytes per unit step plus 8. [goldrush.c](../../LEGOLAND/goldrush.c) |
| Pan slots `0x004b45b0` | Six `{pan,frac}` rows: `{0,0.8f},{0,0.2f},{1,0.8f},{1,0.2f},{2,0.8f},{2,0.2f}`. Three physical pans, two people per pan. [goldrush3.c](../../LEGOLAND/goldrush3.c) |
| Pan positions `0x004b4610` | Raw-square-relative 24.8 pairs `{0x400,0x060}`, `{0x400,0x3d0}`, `{0x400,0x700}`. [goldrush3.c](../../LEGOLAND/goldrush3.c) |
| Pan pose targets | Pose: pan+raw-square+`(0x80,0x80)`. Edge: x includes `-0x280+0x80-int(frac*512.0f)`, y `+0x80`; kneel uses the same x and y `-0x50`; stand first adds `0x80` to **world y**, then requests the edge target. `int(frac*512)` is 409 or 102. [goldrush3.c](../../LEGOLAND/goldrush3.c) |
| Fort area `0x004b4580` | `{-1,-3,3,3}` tiles in `.data`, a rectangle, not a `.rdata` waypoint list. Random coordinates use a byte roll times single-precision `1/255`, bits `0x3b808081`. [goldrush2.c](../../LEGOLAND/goldrush2.c), [fable-a-goldrush2.md](../lanes/fable-a-goldrush2.md) |
| Sixteen headings | Cases 0..15, in multiples of π/8: `-2,-3,12,11,10,9,8,7,6,5,4,3,2,1,0,-1`. x/z rotation stay zero; out-of-range input retains y and rebuilds the matrix. The actual source uses exact stored float approximations, e.g. π-like `3.14159179f` (`0x40490fd7`), not a recomputed mathematical π. [goldrush2.c](../../LEGOLAND/goldrush2.c), [fable-a-goldrush2.md](../lanes/fable-a-goldrush2.md) |
| Approach paving | Anchor-relative `(-1,0),(-2,0),(-2,-1),(-2,-2)`. [goldrush.c](../../LEGOLAND/goldrush.c) |
| Matte assets | Castle `Castle Matte.lls`; fort `fortmask.lls`; temple `temple_matte1.lls`, `temple_matte2.lls`; gold `goldwashmatte1.lls`, `goldwash.lls`, `goldwashmatte2.lls`, `goldmask.lls`. Castle registers to layer 2, temple layers 0/3, fort mask offset `(0x173,-0x7b)`. Gold mask frame is layer 1 modulo 8. [goldrush.c](../../LEGOLAND/goldrush.c) |

The exact float y-rotation literals for headings0..15 are `{-0.785397947f,-1.17809689f,4.71238756f,4.3196888f,3.92698979f,3.53429079f,3.14159179f,2.74889278f,2.35619378f,1.9634949f,1.57079589f,1.17809689f,0.785397947f,0.392698973f,0.0f,-0.392698973f}`. [goldrush2.c](../../LEGOLAND/goldrush2.c)

Gold’s four overlay cuts use raw placement coordinates in 24.8 units: x≤`x+7.5` and y≤`y−2.5`, then `goldmask`; x≤`x+7.5` and y≥`y−2.5`, then matte2; x>`x+7.5`, then matte1; finally whole-tile x>`x+8`, then build layer3. Boundary equality can put a person in both first passes. The separately loaded `goldwash.lls` sprite is not drawn here. [goldrush.c](../../LEGOLAND/goldrush.c)

### Original bugs and disagreements

The pan header’s **`0x30` southward-drift calculation mixes coordinate origins**. Let P be pan y plus the raw placement y: the edge target is P+128, kneeling targets P−80 (208 units north of the edge), and standing first adds 128 to current world y before ordering movement back to P+128. If kneeling has reached its target, that transient world write produces P+48, which is still 80 units north of the edge; it is not a 48-unit displacement from the pre-kneel edge. The rider machine executes these helpers only while low-level state is idle, waits while each helper’s state-7 walk runs, then action8 requests the fixed pan-centre pose and action9 the fixed return waypoint. Preserve the direct world shift and these fixed targets; there is no recovered additive “drift” operation to introduce. The exact low-level state-7 arrival/snapping policy remains outside these attraction bodies, so this reconciliation corrects the header arithmetic without inventing that consumer. [goldrush3.c](../../LEGOLAND/goldrush3.c), [goldrush.c](../../LEGOLAND/goldrush.c)

Castle level 1's independent x/y `rand()&3` jitter maps `0→-128`, `1→0`, `2→128`, `3→3`, giving a three-unit nudge on one roll in four. Its loader reads `def->layers` outside the null guard. Gold paving only changes path graphics, while removal performs full path removal with no refund. Gold's record unlink has the shared null-head fault. [goldrush.c](../../LEGOLAND/goldrush.c), [ridemisc3.c](../../LEGOLAND/ridemisc3.c)

### Callback roles

`CastleLevel1_GetInterfaces`, `Fort_GetInterfaces`, `Temple_GetInterfaces`, `GoldRush_GetInterfaces` install the class handlers. Their `+a8` names are `*_TickRiders` and `+b0` names `*_Draw`; Gold's additional `+98/+9c` pavement/record work and `+b8/+bc` `LoadGoldWash`/`SaveGoldWash` make it the only saved placement machine in this four-class cluster. [interfaces.c](../../LEGOLAND/interfaces.c), [goldrush.c](../../LEGOLAND/goldrush.c)

## Joust and temple slide

### Data structures

Joust has `0x24`-byte records, head `0x004c1250`, square `+00`, next `+04`, sample `+08` (cleared on load), queue indicator `+0c`, `u8 jousters +10`, `u8 spectators +11`, six stand seats `+12..+17`, two horse states `+18/+19`, `u8 cycle +1a`, published frame `+1b`, boarding gate `i32 +1c`, dismount gate `i32 +20`. `joust.c` names `+1a next_horse`; `joust2.c` establishes that it is a 0..63 cycle whose phase selects the horse. Temple Slide is `0x20` bytes, head `0x004cbfd4`, square `+00`, next `+08`, phase counter `i32 +04`, running flags `u32 +0c` (bit 0), and four `i32` lane flags `+10..+1c` (0 free). [joust.c](../../LEGOLAND/joust.c), [joust2.c](../../LEGOLAND/joust2.c), [ridesave.c](../../LEGOLAND/ridesave.c)

### Rules and state machines

Joust ticks riders then arena records. A missing rider's placement record returns from the **whole** update and skips the arena loop. Rider states 0..7 belong to jousters; spectator chains are `0x0a..0x10` and `0x14..0x1e`; states 8,9 and `0x11..0x13` are inert. State 0 approaches `(tile+1,tile-1)`. State 1 either queues a jouster or claims a random free one of six spectator seats. Spectators follow the appropriate side, watch while the post-increment test `timer++ > 150` is false at `0x0d/0x19`, release their seat and leave. Jousters wait for boarding gate, claim the current horse and retain its number at Bloke `+44`, mount the z-sprite/BNV path, then keep placing the rider at the cycle frame until the same `timer++ > 150` test in state4 and the dismount gate permit exit. [joust2.c](../../LEGOLAND/joust2.c)

Arena evaluation happens at cycle 0 for horse 0 and 32 for horse 1. With `(queue==0 || jousters>=2) && jousters!=0`, claimed horse state 1 stops the cycle, riding state 3 opens dismount and stops, others run. Otherwise horse state 0 opens boarding and stops; others run. Running starts the looping horse sample if needed, increments cycle modulo 64 and clears both gates; stopping fades/clears sound and preserves the cycle/gate. Horse states are `0 free,1 claimed,2 mounted,3 riding`; stand-seat flags use 0 free/2 taken. [joust2.c](../../LEGOLAND/joust2.c)

Temple Slide has four stored lanes but its allocator offers only lanes0 and3, preferring0; it routes the customer through the appropriate entrance, mounts that lane’s BNV path, changes animation at a lane-specific frame threshold, then releases the lane and walks out. Admission is recomputed from occupied lanes on release. Its BNV path origin uses the same ride projection contract above. The per-lane walk/end frame values remain external data, so a complete timed slide requires those assets/tables. [joust.c](../../LEGOLAND/joust.c)

Joust’s spectator waypoints below are 24.8 deltas from the anchor; `s` is its assigned seat ID. Every walking row advances to the next action unless stated otherwise. Jouster state2 walks `(0x100,-0x300)`, state6 dismounts toward `(0x200,-0x100)`, and state7 targets `(0,0)`, frees the horse and jumps to `0x1e`; state `0x1e` removes the rider. A rider waits in state1 if a jouster is already queued and all six stand seats are occupied. [joust2.c](../../LEGOLAND/joust2.c)

| Spectator actions | Targets / transition |
| --- | --- |
| `0a,0b,0c` | `(-0x100,-0x100)`, `(-0x364,-0x300)`, `(-0x364,200*s-0x638)`. [joust2.c](../../LEGOLAND/joust2.c) |
| `0d,0e,0f,10` | Set sit value3 and wait with `timer++>150`, then release seat; retrace `(-0x364,-0x300)`, `(-0x100,-0x100)`, `(0,0)` and jump to `1e`. [joust2.c](../../LEGOLAND/joust2.c) |
| `14,15,16,17,18` | `(-0x100,-0x100)`, `(-0x16a,-0x200)`, `(-0x16a,-0xf46)`, `(-0x364,-0xf46)`, `(-0x364,200*s-0x102e)`. [joust2.c](../../LEGOLAND/joust2.c) |
| `19,1a,1b,1c,1d` | Same sit/wait/release as `0d`; `(-0x364,-0xf46)`, `(-0x16a,-0xf46)`, `(-0x16a,-0x200)`, `(0,0)` and jump to `1e`. [joust2.c](../../LEGOLAND/joust2.c) |

Temple Slide action0 claims a lane. Lanes0/1 walk to anchor `(0.5,-5)` and jump straight to3; lanes2/3 walk to `(3.5,-2.5)` and become2, then approach raw-square plus footprint top-left plus `(4,2)` before3. Action1 is inert. Action3 mounts; action4 advances the BNV and holds the person’s animation at the path-provided frame. At the exact lane walk threshold it starts the walking animation, saves speed and uses speed `0x18`; strictly between that threshold and the lane end it holds model frame0. Exact lane end or BNV completion frees the path and selects5. Action5 clears riding/z-sprite, restores speed, sets world position to the lane’s exit offset from the class queue square and walks to that square’s centre. Action6 releases the lane, removes the rider and clears using-ride. [joust.c](../../LEGOLAND/joust.c)

### Tables and constants

Joust's BNV name is `manBox%02d` with horse+1; the cycle is 64 frames, horses half a cycle apart, watch/ride threshold150 with a post-increment comparison, spectator seats six. `Joust_ByteIsSet` returns whether the entire byte is nonzero, not its low bit: gated cycle0 selects horse0, gated cycle32 selects horse1. Temple slide path-name table `0x004b4f08` contains `manbox01..04`; four signed walk thresholds start at `0x004b4f18`, end thresholds at `0x004b4f1c`, but their values are not decoded in the assigned header. Joust FX sample is at entry offset `+08`, correcting `joust.c`'s old `sample@+04` declaration. [joust.c](../../LEGOLAND/joust.c), [joust2.c](../../LEGOLAND/joust2.c)

Temple Slide lane0..3 exit offsets in 24.8 units are `{(-0x280,0x700),(-0x180,0x180),(-0x180,0),(-0x280,-0x500)}` relative to its queue square. Joust BNV paths are `manBox01`/`manBox02`, depth interval `[-1617735.0f,-1617993.5f]`; Temple Slide uses `[-1617664.875f,-1617913.0f]`. The Joust `timer++>150` test first succeeds with an old value151, so a freshly zeroed counter requires152 idle handler visits, rather than exactly150. [joust2.c](../../LEGOLAND/joust2.c), [joust.c](../../LEGOLAND/joust.c)

### Original bugs

Temple Slide assigns lane0 even when both offered lanes0/3 are occupied; admission nevertheless tests all four stored lanes, so unused lanes1/2 keep it open in the normal allocation path. Missing-record lane allocation returns−1, which the caller casts to byte255; later path/threshold/exit indexing does not guard it. Release guards its lane write when the record is missing, then calls an unguarded full-flag lookup anyway. These are distinct original failure paths from its intentionally inert action1. [joust.c](../../LEGOLAND/joust.c)

Missing Joust records freeze every joust for that frame, including sound/cycle progress. Record removal assumes a non-null head. Joust placement can dereference allocation failure; Joust draw collection is unbounded by its eight-entry array. These are separate from default/hold states, which intentionally do not advance themselves. [joust.c](../../LEGOLAND/joust.c), [joust2.c](../../LEGOLAND/joust2.c)

### Callback roles

`Joust_GetInterfaces` and `TempleSlide_GetInterfaces` are in `ridesave.c`; Joust simulation's current implementation is `Joust_Update` in `joust2.c` (older extern `Joust_A8`), custom draw is `Joust_Draw`, and the slide's select/place/draw-descriptor/resource/ride handlers are in `joust.c`. Both use load/save callbacks for their record lists. [joust.c](../../LEGOLAND/joust.c), [joust2.c](../../LEGOLAND/joust2.c), [ridesave.c](../../LEGOLAND/ridesave.c)

## Carousel, Balloonz and earth slide

### Data structures

| Record | Layout |
| --- | --- |
| Carousel `0x2c` allocation | Head `0x006160c4`; next `+00`, square `+04`, boarded/aboard bytes `+06/+07`, signed frame `+08`, flags `i32 +0c` (`1 running,0x4000 boarding closed to newcomers`), revolutions `u8 +10`, half-tick counter `i32 +14`, visitor byte `+18`, timer `i32 +1c`, the short caller view exposes occupancy bytes `+20..+23`; its `0x24` size omits the allocated/zeroed tail `+24..+2b`, whose additional meaning is not established here. Seat IDs in blokes are **one-based**, while storage is zero-based: release `seat[b->seat-1]`; the older `+1f` reading is a biased displacement, not the array start. [ridecb3.c](../../LEGOLAND/ridecb3.c), [bswater.c](../../LEGOLAND/bswater.c), [ridemisc4.c](../../LEGOLAND/ridemisc4.c), [codex-d.md](../lanes/codex-d.md) |
| Balloonz | `0x20` bytes, head `0x00616060`; next `+00`, square `+04`, waiting `i32 +08`, riders byte `+0c`, six gondola bytes `+0d..+12`, half `+13`, wheel `+14`, previous wheel/zframe `+15`, unnamed/banner-related byte `+16`, alighting `+17`, docked/unloading integers `+18/+1c`. Created empty/zeroed. [ridecb3.c](../../LEGOLAND/ridecb3.c), [ridemisc3.c](../../LEGOLAND/ridemisc3.c) |
| Earth slide | `0x24` bytes, head `0x006160e8`; square `+00`, next `+0c`, car-away bit `0x8000` in `+10`, queue count byte `+18`, queue head `+1c`, tail `+20`; queue nodes are eight-byte `{next,rider}`. The initial `ridecb1.c` view ended before the recovered tail. [ridecb1.c](../../LEGOLAND/ridecb1.c), [ridemisc.c](../../LEGOLAND/ridemisc.c), [ridemisc3.c](../../LEGOLAND/ridemisc3.c) |

### Rules and state machines

Carousel actions `0` join/180 timer/approach, `1` select seat and mount `CarouselOn` BNV, `2` play until complete then action 5, `5` sit/count boarded/start at capacity, `7` stand and mount `CarouselOff`, `8` play then action 13 facing 3, `13` free seat and return to map, `14` leave/reopen. Actions 3,4,6,9..12 hold. Starting copies boarded to aboard, clears boarded, sets running, clears the boarding latch and starts sound. After rider ticks, all carousel machines tick; missing placement records skip that phase. `Carousel_StopRide` resets half-tick/platform counters, visitors and boarded count, chooses `rand()%2+3` revolutions, clears flags `0x4001` and fades the square sound after the machine has released its riders. [ridecb3.c](../../LEGOLAND/ridecb3.c), [ridemisc3.c](../../LEGOLAND/ridemisc3.c)

The Carousel machine is idle when neither flag is set. With any rider boarded it decrements a nonzero timer; only a tick that sees timer already0 closes admission and sets `0x4000`. Boarding waits for `boarded==visitors`, then starts and returns. Running increments the half-tick counter, advances one frame every second tick, wraps at64 and decrements the revolution byte. A later tick seeing zero revolutions calls `GetAllBlokesOffRide`, stops and returns immediately: the helper always returns1, contrary to the machine header’s suggestion that it waits for everyone to leave. Both transition returns skip that tick’s positioning and z-frame store. Otherwise the machine visits matching class riders with pose1 and evaluates `BlokeBox%02d` at its frame, then writes the z-sprite’s frame. [bswater.c](../../LEGOLAND/bswater.c), [rides.c](../../LEGOLAND/rides.c), [fable-a-bswater.md](../lanes/fable-a-bswater.md)

Balloonz has six gondolas and two 24-frame halves. Platform car is `wheel/8 + half*3`; rider placement uses the copied **wheel** value `wheel + half*24`. The older header says `zframe`, but that byte instead receives the wheel snapshot at rider-pass writeback and feeds the depth sprite before the machine pass advances the wheel. Actions `0..6` approach through fixed queue points, `7` waits for a docked gondola and claims it, `8` approaches its door, `9` mounts, `10` rides, `11` marks finished, `12` waits for its car to return while unloading, `13` clears platform, `14` walks away and frees car, `15` leaves. Car states are `0 free,1 claimed,2 boarding,3 finished`. At platform-aligned frames (`wheel%8==0`), pickup holds only if waiting is nonzero, riders<6, the car is free and `rand()%3==0`; a claimed car (state1) always holds, and a finished car (state3) holds while alighting is nonzero. Boarding state2 alone does not hold the wheel. Occupied wheels advance unless held; wrapping past23 resets wheel to0 and toggles half, and an advance clears docked/unloading. The published depth-sprite snapshot therefore precedes that tick’s wheel advance; it is not the source of the rider BNV frame. [ridecb3.c](../../LEGOLAND/ridecb3.c)

Earth slide ticks machines first. Action 0 claims a queue spot; 1 waits for head and car availability, launches and approaches the car; 2/3 advance along approach; 4 teleports to the launch approach and rolls `rand()%32+4`; 5 waits; 6 sits/counts boarders and starts full car; 7 holds; 8 stands, teleports to exit centre, leaves and reopens after the last rider. Launch removes the queued flag `0x40`, advances the head rider, pops the queue, fixes an empty tail and replans every remaining queuer. [ridecb1.c](../../LEGOLAND/ridecb1.c), [ridemisc.c](../../LEGOLAND/ridemisc.c)

`EarthSlide_JoinQueue` allocates/zeros an eight-byte node, sets its rider, marks queued flag `0x40`, appends through the external head/tail helper and increments the byte count only after allocation succeeds. `EarthSlide_IsFrontOfQueue` checks the head’s Bloke pointer. `EarthSlide_PopQueue` only unlinks the head, clears tail if it was that node and decrements the count; the caller owns freeing the removed node. Constructors and free-all wrappers use the shared record contracts above. [ridemisc4.c](../../LEGOLAND/ridemisc4.c), [ridetiny.c](../../LEGOLAND/ridetiny.c)

### Tables and constants

| Item | Values |
| --- | --- |
| Earth queue `0x004b65c0` | Four anchor-relative tile pairs `(0,4),(0,3),(0,2),(0,1)`; indexed by current queue length/position. [ridemisc2.c](../../LEGOLAND/ridemisc2.c), [ridemisc.c](../../LEGOLAND/ridemisc.c) |
| Earth approach | Actions 1..4 target `(-1.5,3.5)`, `(-3,3)`, `(-4,3)`, teleport `(-4,3)` relative to base-plus-square. [ridecb1.c](../../LEGOLAND/ridecb1.c) |
| Carousel paths/pivot | BNV names `BlokeBox%02d` with one-based seat; pivot from layer 0 minus `(0x58,0xcd)`. Occupied draw: layer0, Matte2, layer2, actions `0,1,13,14`, riding people/z-sprite, Matte1; empty draws layers `0,1,2`. [ridecb1.c](../../LEGOLAND/ridecb1.c), [ridecb3.c](../../LEGOLAND/ridecb3.c) |
| Carousel machine path/depth | Running BNV `0x0061608c`, distinct from ON `0x00616090` and OFF `0x00616098`; shared path buffer `0x004b64cc` with two decimal digits at byte8; rider depth pair `(-1617853.25f,-1618109.0f)`, path flag0. [bswater.c](../../LEGOLAND/bswater.c), [ridecb3.c](../../LEGOLAND/ridecb3.c) |
| Balloonz queue | Five interior queue waypoints `(-1,7),(-2,7),(-2,1),(-4,1),(-6,2)`; patience 500 ticks; ride timer `(rand()%3+4)*50`. Path-name template `Bloke??` at `0x004b64bc`. [ridecb3.c](../../LEGOLAND/ridecb3.c) |
| Balloonz platform helper | `Balloonz_CarAtPlatform` includes a defensive `half==1 && wheel>23` arm that normal callers cannot reach; their wheel is capped at23. [ridecb3.c](../../LEGOLAND/ridecb3.c) |
| Balloonz exact approach/platform deltas | In 24.8 units: action0 `(-0x9c,+0xfa)`; actions6/13 `(-0x632,+0x300)`; actions7/12 `(-0x564,+0x300)`; action8 `(-0x500,+0x26a)`; action14 `(-0x500,+0x900)`. Depth interval `[-1617692.375f,-1617904.25f]`. [ridecb3.c](../../LEGOLAND/ridecb3.c) |
| Balloonz overlay order | Actions `6,5` → base matte1; action4 → matte2; `0,1,2,3` → matte3; `7,14,15` → wheel; platform-only `8,9,13,14` before car; riders then banner. Banner wraps at `0x30`. [ridecb1.c](../../LEGOLAND/ridecb1.c) |

### Original bugs

The Balloonz loader ORs `0x2000` into **ObjDef flags**, producing `0x2420`, instead of the build sprite's flags: the build sprite is never armed for the custom overlay by that loader. Its overlay nevertheless exists and begins by adjusting an uninitialised offset whose result is overwritten. Array collection limits are not enforced: Balloonz six, Carousel ten. Carousel seat selection loops forever if every seat is occupied. Earth queue indexing is not capped at four: the fifth index reads the following string bytes at `0x004b65e0`, both when finding a spot and shuffling. Earth record removal has the shared null-head fault. [screencb2.c](../../LEGOLAND/screencb2.c), [ridecb1.c](../../LEGOLAND/ridecb1.c), [ridecb3.c](../../LEGOLAND/ridecb3.c), [ridemisc.c](../../LEGOLAND/ridemisc.c), [ridemisc2.c](../../LEGOLAND/ridemisc2.c), [ridemisc3.c](../../LEGOLAND/ridemisc3.c)

Earth Slide queue count can wrap because enqueue increments a byte without a limit check; popping a nonempty queue with count0 likewise underflows. The full Carousel allocation is `0x2c`, so reproducing only the older `0x24` local view truncates its record. Carousel/Balloonz record frees also unconditionally free an absent supplied pointer and retain the shared null-head fault. [ridemisc4.c](../../LEGOLAND/ridemisc4.c), [ridetiny.c](../../LEGOLAND/ridetiny.c), [codex-d.md](../lanes/codex-d.md)

### Callback roles

`Carousel_Tick` and `Balloonz_Tick` are `+a8` in `ridecb3.c`; their overlay draws and `EarthSlide_Tick` are in `ridecb1.c`. Placement/remove/draw-descriptor helpers are in `ridecb8.c`; resource/load callbacks continue in `screencb2.c`. `ridemisc3.c` supplies `Balloonz_NewRecord` and `Carousel_StopRide`. [ridecb1.c](../../LEGOLAND/ridecb1.c), [ridecb3.c](../../LEGOLAND/ridecb3.c), [ridecb8.c](../../LEGOLAND/ridecb8.c), [ridemisc3.c](../../LEGOLAND/ridemisc3.c), [screencb2.c](../../LEGOLAND/screencb2.c)

## Restaurants and Octopus Cafe

### Data structures

Restaurant 1 has a placement list at `0x00616144`, next `+00`, square `+04`, three occupancy bytes `+06..+08` copied as a unit, and animation byte `+09`; its people carry seat `+36`, depth band `+37`, service result `+46` and timer `+58`. Restaurant 2 has `0x40`-byte records, head `0x00616148`: next `+00`, square `+04`, dwell `+06`, waiter step `+07`, serve dwell `+08`, animation `+09`, walking `i32 +0c`, queued/seated bytes `+10/+11`, ready `i32 +14`, phase `+18`, idle `+1c`, seating/called/serving/leaving/clearing/returning gates `+20/+24/+28/+2c/+30/+34`, waiter x/y `+38/+3c`. [ridecb1.c](../../LEGOLAND/ridecb1.c), [ridecb3.c](../../LEGOLAND/ridecb3.c)

Octopus Cafe uses the map cell's five-bit user flags as a round-robin seat number; the customer retains the number in Bloke `+36` and seated flag in `+40`. The recovered tables use **32 seat IDs paired around 16 chair positions**: `g_cafe_pos` contains five approach points and sixteen chairs, the walk uses `seat>>1`, and the orientation table has 32 entries. The header's phrases “sixteen chairs” and “sixteen seat pairs” describe different levels of that pairing and should not be collapsed into sixteen valid seat IDs. [ridecb4.c](../../LEGOLAND/ridecb4.c)

### Rules and state machines

Restaurant 1 has eleven actions: `0` approach six tiles left and select a seat; all three free chooses random, otherwise last free; success sets service result 3/wait 300, full result 4/wait 500. `1` charges and enters the seat route or jumps to 8; `2,3` continue waypoints; `4` sits; `5` eats; `6` stands and waypoint3; `7` waypoint4/releases seat/jumps to9; `8` waits then falls into9; `9` walks one tile below base; `10` leaves. Five occlusion bands render people before their respective mattes; the building frame wraps at 15 in the draw path. [ridecb1.c](../../LEGOLAND/ridecb1.c)

Restaurant 2 has a three-customer batch and a waiter. Customer actions `0,1` approach/queue with 300 patience and temporary speed `0x15` (old speed saved at `+44`); `2` waits for seating and space; `3,4` enter/form batch; `5` calls waiter and faces model direction5; `6` follows serving-seat motion; `7,8` move from serving area; `9` leaves queue; `10` counts meal timer and bills exactly at 250; `11` moves into one of three table positions; `12` waits for the waiter, including 100 idle frames before sending it; `13,14` exit and restore speed; `15` leaves. The restaurant record's second pass has phases `0` approach dwell, `1` called/count serve dwell down, `2` waiter forward, `3` reverse animation step, `4` table dwell/open leaving and exit gates, `5` wait for empty queue/table before returning. Exact counter comparisons are recorded below. Building animation advances modulo 32. [ridecb3.c](../../LEGOLAND/ridecb3.c)

Restaurant 2 customer motion applies the same delta to both world axes: approaching subtracts `g_r2_path_dx[step+1]*8`, departing adds `g_r2_path_dy[step]*8`. The incoming path skips x-table entry0 (zero), so the two directions are one entry out of phase. [ridemisc3.c](../../LEGOLAND/ridemisc3.c)

Restaurant2 permits a partial customer batch when the last incoming person arrives: action4 closes seating and forces ready=3 when `queued==3` **or walking==0**. Action5 uses ready≥3 to call the waiter and then clears ready. Action6 applies the incoming customer delta while phase2 and not serving, waits in other non-serving phases, and advances once serving. Action12 counts idle only in phase0 with seated customers and no incoming walkers; its `idle++>100` condition sends the waiter, resets step/x/y and gates, then applies the outgoing customer delta. The waiter phases use exact tests: phase0 increments serve while `serve<=8`; phase1 decrements while `serve>=0`; phase4 increments swing while `swing<=8`; phase5 waits for both byte counts to be zero and then decrements swing while `swing>=0`. Each transition happens on the following visit, not at the threshold increment/decrement itself. [ridecb3.c](../../LEGOLAND/ridecb3.c)

Cafe action 0 claims the current five-bit seat and increments it modulo32, sets using-ride and waits50; actions1..4 charge/walk via the per-seat route, skipping `-1` route entries; 5 waits; 6 stands beside chair; 7 faces chair; 8 steps onto chair and sets seated/wait200; 9 sits (`0x100`, animation frame0); 10 eats; 11 stands and returns to stand offset; 12 clears seated and returns to chair square; 13..16 retrace the approach; 17 walks to origin; 18 leaves. Stage1/2 falls through into the walking block. Seat allocation never tests occupancy, so wraparound can reuse occupied places. [ridecb4.c](../../LEGOLAND/ridecb4.c)

`Restaurant2_StopSound` takes the packed square by value, fades every square-sourced sample using `-200`, then plays FX entry2 (elevator stop) with `(loop=0, argument=1)`. It is a fade-and-play transition, not merely silence. The sample is at `0x004b6988`, entry2’s `+08` field in the table at `0x004b6968`. [ridemisc4.c](../../LEGOLAND/ridemisc4.c), [codex-d.md](../lanes/codex-d.md)

### Tables and constants

| Table | Shape and decoded values |
| --- | --- |
| Restaurant1 waypoints `0x004b66f4` | Fifteen 24-byte rows indexed `seat*5+phase`: `{dx,dy,ox,oy,turn,band}`; target `((tile+delta)<<8)+offset`. Seat0 rows: `{-2,0,128,128,1,3}`, `{-2,1,128,150,1,5}`, `{-2,1,-80,150,0,4}`, `{-2,1,128,150,0,4}`, `{-2,0,128,128,1,5}`. Other ten rows are not decoded in these headers. [ridemisc2.c](../../LEGOLAND/ridemisc2.c) |
| Restaurant1 mattes | Bands1..5 → `RestMaskLevel1aa`, `RestMaskLevel1`, `RestMaskLevel2`, `RestMaskLevel3`, `RestMask_Main`. [ridecb1.c](../../LEGOLAND/ridecb1.c) |
| Restaurant2 customer targets | 24.8 deltas from anchor: action0 `(0x3c8,-0x200)`;1 `(0x3c8,-0x400+100*walking)`;2 `(0x2ce,-0x39c)`;3 `(0x16a,-0x39c)`;4 `(0x16a,100*queued-0x532)`;7 `(-0x79c,-0xc9c)`;8 `(-0xa00,-0xc9c)`;11 teleports to `(-0xa9c,100*seated-0xd2c)`;13 `(-0x200,-0x46a)`;14 `(-0x300,-0x46a)`. Counts are read at the stage indicated; they are not constant seat IDs. [ridecb3.c](../../LEGOLAND/ridecb3.c) |
| Restaurant2 waiter `0x004b685c`, `0x004b68e0` | Declared as two 33-entry integer tables; start x=`0x143`, then increment step before subtracting x-step and adding y-step. Return phase decrements step; it does not reverse waiter x/y stores. Seat-customer helper also selects tables beginning `0x004b6860` and `0x004b68e0`. The x table starts with zero; remaining contents are not present as initializers. [ridecb3.c](../../LEGOLAND/ridecb3.c), [goldrush.c](../../LEGOLAND/goldrush.c), [ridemisc3.c](../../LEGOLAND/ridemisc3.c) |
| Cafe positions `0x004b6990` | 21 `{i32 x,y}` 24.8 pairs, approach0..4/chair5..20. Individual coordinates remain external. [ridecb4.c](../../LEGOLAND/ridecb4.c) |
| Cafe walk `0x004b6a34` | 65 integers, four-step rows indexed `(seat>>1)*4+stage` for stages1..4; `-1` skips; leaving reads `(seat>>1)*4+17-stage`. Entry0 aliases position20's y and is not read. [ridecb4.c](../../LEGOLAND/ridecb4.c) |
| Cafe offsets `0x004b6b38`, directions `0x004b6be8` | Eleven `{stand_x,stand_y,sit_x,sit_y}` rows; 32 orientation indices. Final target adds raw cell position and offsets, with `x-0x80,y+0x80`. Rows not decoded in the header. [ridecb4.c](../../LEGOLAND/ridecb4.c) |
| Cafe table seat mapping | Table0..7 take four IDs in painter order: `{28,29,30,31}`, `{0,3,1,2}`, `{4,7,5,6}`, `{10,11,8,9}`, `{14,15,12,13}`, `{17,18,16,19}`, `{21,22,20,23}`, `{24,25,26,27}`. For table k, sprite k is followed by the first two diners, chair mask2k, last two diners and mask2k+1; a mask is emitted only when its associated diners exist. Table8 is the counter. [ridecb4.c](../../LEGOLAND/ridecb4.c) |
| Cafe depth `0x004b6d58` | Index `11*(world_tile_x-cell_x)-world_tile_y+cell_y` → band1..19. Draw bands1,2/table4;3,4/table5;5,6/table3;7/table2;8..10/counter;11,12/table6;13,14/table7;15,16/table1;17,18/table0; then nearest band. Full lookup bytes remain unknown. [ridecb4.c](../../LEGOLAND/ridecb4.c) |

### Original bugs and discrepancies

Restaurant1 free-seat count/index are initialized once **per class tick**, not once per customer; arrivals later in the same tick inherit earlier counts and can select occupied seats. Missing records return from whole class ticks, stopping later riders and any remaining instance phase. Restaurant1's draw array holds eight without a guard. Cafe depth lookup is unbounded for people outside its footprint; seat allocation is an occupancy-blind modulo32 counter. The table shapes support 32 seat IDs/16 chair pairs, so the header's implication that IDs16..31 alone must overrun the tables is not established by its own indexing formulas. [ridecb1.c](../../LEGOLAND/ridecb1.c), [ridecb3.c](../../LEGOLAND/ridecb3.c), [ridecb4.c](../../LEGOLAND/ridecb4.c)

Restaurant2’s body differs from several timing descriptions: action9 tests `queued--==0`, so the 1→0 transition does **not** clear leaving, while a call at0 wraps the unsigned byte to255 and clears it. The meal action tests old `wait<0`, decrements, and bills when the new value equals250. Forward waiter phase2 accepts old step32 and increments **before** reading, so it reads indices1..33; x[33] at `0x004b68e0` aliases y[0], while y[33] is beyond the stated 33-entry y table. The next rider pass can also ask the incoming helper for x[34] while step33 awaits the phase transition. These are source-level access facts; the external bytes do not establish whether the adjoining data was deliberately shared. Return phase3 decrements the animation step and clears serving but does not subtract/add the waiter coordinate deltas. [ridecb3.c](../../LEGOLAND/ridecb3.c), [ridemisc3.c](../../LEGOLAND/ridemisc3.c)

Restaurant2’s auxiliary layer draw sets layer1 from record step `+07`, layer3 from frame `+09`, layer6 from serve `+08`, and layers0/2 from swing `+06`. By service phase it draws: 0/1 → layers1,3 (phase0 sets layer6 only while walking is nonzero);2 → layer1;3 → layers1,3;4/5 → layers1,2,0. Layer6’s frame is set here but that layer is painted by the main overlay. The main overlay paints phase0/1 as layer5, actions5/6, layer6, action4, front; phase2 as gated action12, floor, walls, action6, upper, layer3; phase4/5 as actions7..9,13..15, floor/walls/upper/layer3. Every phase finishes with actions0..3 outside. [goldrush.c](../../LEGOLAND/goldrush.c), [ridecb4.c](../../LEGOLAND/ridecb4.c)

Restaurant2 phase2's upper blit takes the layer x offset but uses record `+38` halved for y; phase4/5's floor blit adds layer0 y but omits layer0 x. Its final phase0/1 overlay reuses layer6's offset. These asymmetric positions are explicit recovered behaviour. The draw header calls the selector “facing”, but it reads the same record `+18` field that the simulation identifies as the waiter/service phase; a separate building orientation is not established here. [ridecb4.c](../../LEGOLAND/ridecb4.c)

Cafe teardown releases nine table/counter sprite handles at `0x0081cd60..0x0081cd80` and sixteen chair-mask handles at `0x0081cda0..0x0081cddc`, then its money sound; it does not null those handles. The old teardown comment calls this evidence for sixteen seats, but these are sixteen **mask sprites**, and the current painter explicitly uses all 32 seat slots. [ridecb9.c](../../LEGOLAND/ridecb9.c), [ridecb4.c](../../LEGOLAND/ridecb4.c)

### Callback roles

`Restaurant1_Tick`/`Restaurant1_Draw` are `+a8/+b0` in `ridecb1.c`; `Restaurant2_Tick` is in `ridecb3.c`, `Restaurant2_Draw` in `ridecb4.c`, and an instance animation helper in `goldrush.c`. `OctopusCafe_Tick`/`OctopusCafe_Draw` are in `ridecb4.c`, with `CafeDrawTable` interleaving seated people and masks; teardown is `OctopusCafe_Destroy` in `ridecb9.c`. Earlier `ridecb2.c` analysis is superseded by those implementations. [ridecb1.c](../../LEGOLAND/ridecb1.c), [ridecb2.c](../../LEGOLAND/ridecb2.c), [ridecb3.c](../../LEGOLAND/ridecb3.c), [ridecb4.c](../../LEGOLAND/ridecb4.c), [goldrush.c](../../LEGOLAND/goldrush.c), [ridecb9.c](../../LEGOLAND/ridecb9.c)

## Western Town

### Data structures

All nine classes use the shared RiderNode list/action byte for customer state. Jail is the only class here with a placement list: `0x1c` bytes, head `0x0062fd3c`, next `+00`, square `+04`, signed door frame `+06`, integers taken `+08`, closing `+0c`, shut `+10`, opening `+14`, open `+18`. Door frame starts at9, with range0 open..9 closed. [westtown.c](../../LEGOLAND/westtown.c), [westtown2.c](../../LEGOLAND/westtown2.c)

The early `westtown.c` header calls LEGO SHOP1/MEDIA single-instance scalar-state buildings. Its later self-paving section and add/remove bodies establish the actual recovered purpose: they pave/strip their footprint, with state in the map and no own save chunk. No single-instance gameplay restriction should be inferred solely from that stale header. [westtown.c](../../LEGOLAND/westtown.c)

### Rules and state machines

Each row below gives action order, with coordinates in tiles relative to base-plus-placement unless marked raw. A walking entry advances after setting movement; “buy” invokes `Shop_BrowseAndBuy` with price index1. Every script marks using-ride on entry and clears it on final removal. The browse helper checks an inherited signed counter for zero and advances if zero, then decrements it; whenever the decremented counter is a multiple of32, a `rand()%100 <=30` roll bills and advances. It does not initialize that counter. [westtown2.c](../../LEGOLAND/westtown2.c)

| Class | Action script |
| --- | --- |
| Sheriff (0..7) | `0(-1,+.5)`, `1(-3,+.5)`, `2(-3,rand()%3)` with `.5` used for roll0 and facing8, `3 buy`, `4(-3,+.5)`, `5(-1,+.5)`, `6(0,0)`, `7 leave`. [westtown2.c](../../LEGOLAND/westtown2.c) |
| Bank (0..8) | `0(+.5,-1)`, `1(+1,-2)`, `2 raw(+2,-1), wait=rand()%8`; `3/4` decrement and alternate raw `(0,-1)/(+2,-1)` while positive, else5; `5(+1,-2)`, `6(+.5,-1)`, `7(0,0)`, `8 leave`. No purchase call. [westtown2.c](../../LEGOLAND/westtown2.c) |
| Saloon (0..9) | `0(-.5,0)`, `1(-2,-1)`, `2(-4,-.5)`, `3` coin toss `(-3.5,+1)` or `(-3.5,-2)`, face8; `4 buy`, `5(-4,-.5)`, `6(-2,-1)`, `7(-.5,0)`, `8(0,0)`, `9 leave`. [westtown2.c](../../LEGOLAND/westtown2.c) |
| LEGO Shop1 (0..6) | `0(-5,0)`, `1(-5+rand()%2,+3+rand()%2),wait=rand()%50`, `2 wait/spin every10`, `3(-5,rand()%3),wait=rand()%30`, `4 buy`, `5(0,0)`, `6 leave`. Facing spin increments0..8 and wraps past8 to0. [westtown2.c](../../LEGOLAND/westtown2.c) |
| LEGO Media (0..6) | `0(-2,0)`, `1` random one of `(-4,0),(-3,-2),(-4,-2)`, `2 buy`, `3 teleport` to one of those three, `4(-2,0)`, `5(0,0)`, `6 leave`. [westtown2.c](../../LEGOLAND/westtown2.c) |
| General Store (0..11) | `0(-.5,-1)`, `1(+.5,-2)`, `2(+.5,-3)`; `3` coin toss: far counter `(-1,-2.5),wait=rand()%50` or jump6; `4 wait/spin every10`; `5(+.5,-3)`; `6 advance`; `7 buy`; `8(+.5,-2)`; `9(-.5,-1)`; `10(+.5,+.5)`; `11 leave`. [westtown2.c](../../LEGOLAND/westtown2.c) |
| LEGO Shop2 (0..12) | `0(-.5,0)`, `1(-1,-.5)`, `2(-1.375,-.375)`, `3` random `(-3,0),(-1,-2),(-3,-2.5)`, `4 buy`, `5 teleport` among same spots: first two set `rand()%50` and action8, third action9; `6/7 inert`; `8 wait`; `9(-1,-.5)`; `10(-.5,0)`; `11(0,0)` falls through12 departure. [westtown2.c](../../LEGOLAND/westtown2.c) |
| Explorers (0..5) | `0 raw(0,+1)`, `1 raw(0,+2),wait=rand()%70+20`, `2 wait`, `3 raw(0,+1)`, `4 anchor centre`, `5 leave`. [westtown.c](../../LEGOLAND/westtown.c) |
| Jail (0..10) | `0` if free claim/walk `(-2,-1)`, otherwise `(-.5,0)` facing7; either wait `rand()%100`. `1 start opening/face3`; `2 await open, count down, start closing at0; every10 face2/4`; `3 await shut`; `4(-1,-.5),release,jump8`; `5/6 inert`; `7 wait`; `8(+.5,+.5),jump10`; `9 inert`; `10 leave`. After customer loop, each opening door decrements frame to0 and each closing door increments to9. [westtown2.c](../../LEGOLAND/westtown2.c) |

### Tables and constants

Overlay bands use the same action byte as the script. General Store draws actions4,5,6 behind shelf matte then other customers before front matte. Saloon groups4,5,6 behind matte2;2,3,7,8 behind matte1;0,1,9 in front. Media groups2..5 behind mask2 and0,1,6 behind mask1. LEGO Shop2 draws4,5,3,2,9,10,11, front matte, then1,12. Jail draws2,3; door layer1;1,4,8; cell mask;0,7,10. With no jail customers, only the door is drawn. Shared simple shops render customers then a single matte. [westtown.c](../../LEGOLAND/westtown.c), [westtown2.c](../../LEGOLAND/westtown2.c)

### Original bugs

General Store/Shop1 waits test equality with zero, so negative inherited/signed-remainder timers can traverse the 32-bit counter range instead of immediately finishing; Explorers decrements on the transition tick and leaves `-1`. LEGO Shop2 states6/7 and Jail5/6/9 never progress if entered. LEGO Shop2's final walk and departure occur in the same tick. A missing jail record returns before all later customers and door animations. Both opening and closing flags run independently, so simultaneous flags decrement then increment the same frame. Jail's loader sets `0x2000` on ObjDef instead of its sprite. Collection arrays have ten entries with no capacity check, and their signed-byte count has an additional overflow beyond127. [westtown.c](../../LEGOLAND/westtown.c), [westtown2.c](../../LEGOLAND/westtown2.c)

Self-paving adds graphics-only paths but removes full paths. Nine classes each call money-SFX load/free even though that subsystem has no reference count. Preserve these resource and map side effects when emulating the original. [westtown.c](../../LEGOLAND/westtown.c)

### Callback roles

`WesternTown_GetInterfaces` installs all nine classes. They share `Shop_GetDrawDesc` (`+a0`) and six share `Shop_Remove` (`+9c`); Jail, LEGO Shop1 and LEGO Media own placement/remove work. Per-customer `+a8` handlers are named `*_TickCustomers`, overlays `*_DrawOverlay`. Jail alone receives the ride-list save pair. `LegoShop1_Destroy` is implemented in `catapult.c` despite belonging to this family. [interfaces.c](../../LEGOLAND/interfaces.c), [westtown.c](../../LEGOLAND/westtown.c), [westtown2.c](../../LEGOLAND/westtown2.c), [catapult.c](../../LEGOLAND/catapult.c)

## Water Works and garden

### Data structures

Water block, shower and elephant fountain use `WaterRec` (`0x0c` bytes): next `+00`, packed square `+04`, state/timer/frame bytes `+08/+09/+0a`. Heads are `0x004cc02c/30/34` respectively. Water block and elephant records are saved; shower records are not. Water Works draw descriptor lives at `0x004cbff0`; garden reuses Western Town's descriptor at `0x0082c6a0`. Garden placements hold only an image index in cell user flags. Shared Water Works FX references are counted at `0x004cc028`. [waterworks.c](../../LEGOLAND/waterworks.c), [ridesave.c](../../LEGOLAND/ridesave.c)

### Rules and state machines

Water-piece placement cursor updates reject with error12 until a Water Works Entrance exists. Animation arming checks visitors, gardeners and handymen; water block/shower use a single square, elephant a four-by-four footprint-relative box. Water block arms splash state1 and plays the splash, runs until image LLS frame count then idles/frame0; its two-action swimmer script moves to anchor `(-.5,+.5)` and then leaves. Shower state1 warms for11 ticks; state2 plays the spray and repeats its last11 frames if occupied; state3 runs off for8 ticks before idle. While state2, its body is pinned to frame11 and its overlay supplies the moving spray. [waterworks.c](../../LEGOLAND/waterworks.c)

Elephant sprays into its forward trigger box; its tick sorts the separate squirt sprite in front of the elephant. Crocodile fountain has no resource loader and depends on another Water Works class having loaded the shared sound table. Hedge add/remove re-evaluates its four neighbors and itself; flowers select one random image once. [waterworks.c](../../LEGOLAND/waterworks.c)

### Tables and constants

| Item | Values |
| --- | --- |
| FX table `0x004b4fa8` | `[0] WaterworksShower01.wav` splash; `[1] spray or fountain.wav` entrance loop; `[2] Fountain01.wav` fountain loop. Load at reference0→1, free at1→0. [waterworks.c](../../LEGOLAND/waterworks.c) |
| Shower | Warmup11, runoff8; overlay view-adjusted `(-8,+0x14)`. [waterworks.c](../../LEGOLAND/waterworks.c) |
| Elephant | Trigger anchored at `(square.x+rect.left,square.y+rect.bottom)`, extent4×4; squirt offset `(-0x75,+0x8a)`, depth `0x20` below foot. [waterworks.c](../../LEGOLAND/waterworks.c) |
| Hedge image | Neighbor bits `N1,E2,S4,W8`; image `mask-1` if nonzero else14. Thus masks0 and15 both select14. Flowers use `rand()%image_count`. [waterworks.c](../../LEGOLAND/waterworks.c) |

### Original bugs and disagreements

An isolated hedge and a fully surrounded hedge share image14. Garden fills only the first three fields of the shared draw descriptor, leaving old square/flag fields. `Garden_GetInterfaces` matches only uppercase `HEDGE`/`FLOWERS` using case-sensitive comparison; all the other provider compares are insensitive. The header suggests differently cased names use a stale shared descriptor, whereas the later source note correctly says that without custom callbacks the **generic draw path** uses the class's sprite; no custom image/restitching behaviour is installed. Crocodile can be silent without another Water Works resource user. [waterworks.c](../../LEGOLAND/waterworks.c), [interfaces.c](../../LEGOLAND/interfaces.c)

### Callback roles

`WaterWorks_GetInterfaces` covers entrance, water block, shower, elephant and crocodile; `Garden_GetInterfaces` covers hedge/flowers. `+90` is placement validation, `+a8` arming/animation/swimmer work, `+a0` image/frame descriptor and `+b0` people/spray overlays. Garden has no per-frame/customer tick; its `+8c` selection is not simulation. Claims in old headers that only Water Works/log flume use `+90` are contradicted by the road/jungle cursor callbacks. [interfaces.c](../../LEGOLAND/interfaces.c), [waterworks.c](../../LEGOLAND/waterworks.c), [ridecb5.c](../../LEGOLAND/ridecb5.c), [ridecb7.c](../../LEGOLAND/ridecb7.c)

## Castle and the coaster class adapter

### Data structures

Castle dispatch is six `0x18`-byte rows at `0x0082ad20`: class element `+00`, select `+04`, cursor/update `+08`, add `+0c`, query/update2 `+10`, remove `+14`. The following six cached element pointers start at `0x0082adb0`, also the lookup's end bound. Castle/coaster live state starts at `0x00829ae0`, kind at `+00` (0 absent,1 placed), square as signed shorts `+08/+0a`. Castle footprint blocks are20 bytes, part records36 bytes with offsets and indices into `0x58`-byte records at `0x006102f8`. The removal cursor is `0x182c` bytes, origin `+1404/+1408`, footprint/check fields `+1414`. [castleobj.c](../../LEGOLAND/castleobj.c)

Track descriptors are `0x38` bytes: raised `+00`, two heights `+04/+08`, auxiliary geometry `+0c..+18`, draw/build/query/place/remove pointers `+1c..+2c`, carries-path `+30`, unnamed `+34`. Coaster save construction uses a20-byte size descriptor; the blob has node count `+08`, `{node,link}` array pointer `+0c`, additional pointer fields `+14/+1c`. Those three pointers are serialized as blob-relative offsets and relocated on load. [castleobj.c](../../LEGOLAND/castleobj.c)

### Rules and state machines

The castle is the special mandatory placement recognized by its named element global `0x0080ff64`. Six class rows share generic thunks; track families register descriptors, and raised ordinary pieces block cell bits1/2 while raised-path pieces clear them. Query handlers locate/latch track pieces and validate cursor footprints. Castle rider actions are sparse: `0` claim/approach `(x-8,y-1)`, `1→0x10`, `0x10` hand to interior/set0x20, `0x21` walk to square centre, `0x22→0x40`, `0x40` leave. Its overlay excludes actions `0x10..0x20`, then draws the layer2 castle sprite. Full coaster geometry is outside this adapter's recovered contract. [castleobj.c](../../LEGOLAND/castleobj.c)

### Tables and constants

Rows0..5 are `CASTLE OBJ`, `CASTLE_DUMMY`, `SQUARE_TRACK`, `SQUARE_TRACK_HEIGHT`, `SQUARE_TRACK_HEIGHT_0`, `SQUARE_TRACK_HEIGHT_PATH`. Descriptor addresses for the four track classes are `0x004b5d20`, `0x004b5d58`, `0x004b5d90`, `0x004b5dc8`; flat raised=0, others1; `+0c/+14=0x0f`, `+10/+18=0`, and only PATH carries a path. Header gives height values `0x19,2,0x0f` without an unambiguous per-class pair assignment, so no invented full table is supplied. Castle rider action dispatch index table is `0x41` bytes at `0x00425488`, selecting seven blocks at `0x0042546c`. [castleobj.c](../../LEGOLAND/castleobj.c)

### Original bugs and disagreements

A lookup miss resolves to row0. Dummy setup fills handlers but omits its element key, so normal dummy lookups miss and reach castle handlers. Raised-track cell writes omit the null guard present in flat track. The header calls `ROLLER_COASTER_LOAD/SAVE`'s `+8c` replacement a “per-frame tick”; the confirmed engine-wide role of `+8c` is placement/UI, so the defensible fact is that those marker classes install serializer functions in **that slot**, not that saving necessarily runs every frame. [castleobj.c](../../LEGOLAND/castleobj.c), [joust.c](../../LEGOLAND/joust.c)

### Callback roles

`CastleObj_GetInterfaces` handles the six rows plus `ROLLER_COASTER_LOAD` and `ROLLER_COASTER_SAVE`, which bypass rows and replace `+8c` with `LoadRollerCoaster`/`SaveRollerCoaster`; Castle itself has these in `+b8/+bc`. Five `CtThunk_*` functions dispatch `+8c/+90/+94/+98/+9c`. Castle's actual simulation/draw are `Castle_Activate`/`Castle_Interact`. [castleobj.c](../../LEGOLAND/castleobj.c)

## Entrance, Chuck Wagon and staff buildings

### Data structures

Entrance and Chuck Wagon use the common class rider list and Bloke action; entrance uses `+36` direction/lane selector and ObjDef `+40/+44` y/x origin. Mechanics Hut uses Bloke job/action state; its draw collects people in a fixed 30-entry local array. The hut/potting-shed evictor is parameterized by the class, packed square and shed flag, with no extra per-placement vehicle record. [ridecb1.c](../../LEGOLAND/ridecb1.c), [ridecb7.c](../../LEGOLAND/ridecb7.c), [ridecb8.c](../../LEGOLAND/ridecb8.c), [ridecb9.c](../../LEGOLAND/ridecb9.c), [ridemisc.c](../../LEGOLAND/ridemisc.c)

Brolly image data is a 20-byte prefix: count `i32 +04`, sprite-array pointer `+08`, doubled x-offset array `+0c`, doubled y-offset array `+10`. Placement stores `rand()%count` in map user flags; drawing uses their low byte to select all three arrays and arithmetic-shifts each offset by1. Its shared 20-byte descriptor contains sprite/offsets at `+00/+04/+08`, square word `+0c`, flags `+10`; the brolly sets the last field to0. [ridecb8.c](../../LEGOLAND/ridecb8.c)

### Rules and state machines

Entrance action0 chooses incoming (`+36=2`) at/left of the outside line or outgoing (`+36=1`) at/right of the inside line, alternates lane using a shared coin and randomness, then incoming goes1/outgoing50. Action50 makes the extra outbound approach, then1. Action1 adds the admission fee and plays payment sound **only for outgoing direction1**, walks through and becomes2. Action2 removes the rider and pops its long-term action. These apparently reversed payment semantics are what the recovered code says. [ridecb7.c](../../LEGOLAND/ridecb7.c)

Chuck Wagon’s overlay draws only the first matching customer and discards a subsequent screen-position query; the shared Foodcart overlay draws all matching customers. Chuck Wagon, Shark Cafe and the three foodcarts load the shared money sound and arm simulation/custom sprite drawing. Their resource destructors release the same shared sound. [ridecb8.c](../../LEGOLAND/ridecb8.c)

Chuck Wagon actions are0 approach `(0,+1)`;1 serving hatch `(-.5,+1)` immediately falls through2, faces7 and waits `(rand()&31)+4`;3 waits, bills item0 at completion;4 returns to `(+.5,+.5)`;5 leaves. [ridecb8.c](../../LEGOLAND/ridecb8.c)

Mechanics Hut states0..2 approach with relative 24.8 deltas `(0,+0x300)`, `(+0x400,+0x100)`, `(0,+0x400)`;3 clears using/hired, unlinks/frees rider and gives action`0x11` (repair round). Firing states`0x64..0x66` walk `(0,-0x300)`, `(-0x480,-0x100)`, `(0,-0x300)`;`0x67` unlinks, sets job`0x64`, refunds30 bricks. Evicting all staff restores base-square position and clears flags`0x28`; normal shed staff receive long-term action`0x10`, hut staff`0x11`. [ridecb9.c](../../LEGOLAND/ridecb9.c), [ridemisc.c](../../LEGOLAND/ridemisc.c)

Potting Shed has its own four-arm staff script: action0 marks using-ride and adds `0x900` to the existing target x;1 clears flags `0x28`, unlinks, gives long-term action `0x10`, then frees the rider node. Firing action `0x64` subtracts `0x900` from target x; both movement arms retain target y and start a state7 walk. Action `0x65` unlinks without freeing, sets job `0x64` and refunds the gardener. This mirrors the hut’s leak but uses the shed’s own stages and movement. [ridecb9.c](../../LEGOLAND/ridecb9.c)

### Tables and constants

Entrance incoming lane offsets are `{0x978,0xd78}` and outgoing `{0x878,0xc78}`. The four overlay y bands relative to y-origin are `[0x820,0x8d0]`, `[0x920,0x9d0]`, `[0xc20,0xcd0]`, `[0xd20,0xdd0]`; only the first matte uses caller blit mode, later four use0. Hut draws states0,1,2,3,`0x64,0x65,0x67`, its matte, then`0x66`. Hut dispatch maps0..`0x67`, with4..`0x63` inert. [ridecb1.c](../../LEGOLAND/ridecb1.c), [ridecb7.c](../../LEGOLAND/ridecb7.c), [ridecb9.c](../../LEGOLAND/ridecb9.c)

### Original bugs

Entrance visitors between the two test lines remain action0 forever, repeatedly flipping the lane coin; if both line tests pass, action increments twice then is overwritten with50. Action50 retains the previous target y. The entrance never clears using-ride flag8. Chuck Wagon never rests in action2 and decrements its timer on the transition tick, leaving`-1`. Hut firing (`0x67`) and eviction of already-firing staff unlink without freeing rider nodes; the draw array is unbounded. [ridecb7.c](../../LEGOLAND/ridecb7.c), [ridecb8.c](../../LEGOLAND/ridecb8.c), [ridecb9.c](../../LEGOLAND/ridecb9.c), [ridemisc.c](../../LEGOLAND/ridemisc.c)

### Callback roles

`Entrance1_Tick`/`Entrance1_Draw` implement `+a8/+b0`; `ChuckWagon_TickCustomers` is the current `+a8` implementation (older header `ChuckWagon_Activate`). `MechanicsHut_Tick`/`MechanicsHut_Draw` implement the corresponding staff slots. `MechanicsHut_EvictRiders` is shared with Potting Shed. `ridecb8.c` also provides stationary food/scenery resource and selection helpers: Shark Cafe Brolly uses an image-list draw descriptor and no tick, Dragon BBQ starts/fades a square-sourced looping sample. [ridecb1.c](../../LEGOLAND/ridecb1.c), [ridecb7.c](../../LEGOLAND/ridecb7.c), [ridecb8.c](../../LEGOLAND/ridecb8.c), [ridecb9.c](../../LEGOLAND/ridecb9.c), [ridemisc.c](../../LEGOLAND/ridemisc.c)

## Transport callback frontier

### Data structures

This section accounts for the transport records and mechanics recovered in the assigned mixed callback files; full transport animation/geometry lives in the transport sources. Boating station is`0x34` bytes, head`0x004cc074`: key`+00`, start bytes`+02/+03`, end`+04/+05`, route`+08`, frame`i32 +0c`, backwards`+10`, queue count`+14`, five people`+18..+28`, next`+2c`, take`+30`. Road is`0x20`, head`0x004cbeac`, next`+00`, owning school key`u16 +08`, origin`i32 +0c/+10`, kind byte`+14`, traversal mark`+18`, car/pedestrian claim bytes`+1c/+1d`. Pump is`0x10`, key/owner`+00/+02`, x/y`+04/+08`, next`+0c`. Driving school is`0x0c`, key`+00`, take`+04`, next`+08`, head`0x004c11bc`; school car is`0xd0`, next`+00`, driver`+cc`, head`0x004c10d4`. [ridecb5.c](../../LEGOLAND/ridecb5.c), [ridecb6.c](../../LEGOLAND/ridecb6.c), [ridecb8.c](../../LEGOLAND/ridecb8.c), [goldrush4.c](../../LEGOLAND/goldrush4.c)

Jungle station is`0x44`, head`0x00629c3c`: key`+00`, route endpoints`+02..+05`, route`+08`, queue length`+14`, five queue people`+18..+28`, dispatch timer`+2c`, three waiting-to-board people`+30..+38`, next`+3c`, value`+40`. Water is`0x1c`, head`0x0062fd2c`, key/owner`+00/+02`, links`+04`, scratch`+08/+0c/+14/+18`, next`+10`. Boat is`0x3f8`, head`0x00616164`, owner`+00`, current/target squares`+04..+10`, screen`+14/+18`,80 wobble pairs`+1c`,80 sprite codes`+29c`, state`+3e0`, three riders`+3e8..+3f0`, next`+3f4`. Older names swapping water and boat lists are superseded by these corrected types. [ridecb2.c](../../LEGOLAND/ridecb2.c)

### Rules and state machines

Roads are four-by-four blocks. Exact-origin lookup differs from covering-cell lookup. Kind5 is traffic lights; kind6 is the school's one-way entrance, joinable only from the permitted west side. The group ID is the owning school's packed square. A cursor rejects any three consecutive same-group neighbors that would create a solid2×2 road square. Zebra crossing is kind bit`0x10`, not a separate road record: removal can strip/refund that bit while keeping the road. Edits rebuild owner circuits by clearing traversal marks and running the shared walker. Water/road/decoration pieces contribute to the school/station take/value metric; jungle best-value reports the maximum. [ridecb5.c](../../LEGOLAND/ridecb5.c), [ridecb6.c](../../LEGOLAND/ridecb6.c), [ridecb2.c](../../LEGOLAND/ridecb2.c)

Driving School riders approach two rows above anchor and wait1..8; if `5*car_count < road_tile_count`, new-car result0 boards/sits,`-1` skips to getting off,`-2` retries2..33. Driving holds action2; action3 snaps back/walks to centre;4 leaves and kills samples. Before turns/straight maneuvers, `SchoolCarAtTarget` requires a covering road and checks the next block's traffic light. Entry is refused only for a crossing with **zero cars and nonzero pedestrian claims**; existing car occupancy permits a following car. A refusal rolls back that frame's position/integration/square. Same-block or non-road destination passes without changing counters. Straight maneuver moves two half-blocks with one endpoint waypoint. [castleobj.c](../../LEGOLAND/castleobj.c), [goldrush3.c](../../LEGOLAND/goldrush3.c), [goldrush4.c](../../LEGOLAND/goldrush4.c), [goldrush.c](../../LEGOLAND/goldrush.c)

The boating/jungle station rider queues both have five slots. Action0 appends at4 or rejects a full/occupied tail; existing queuers advance when the next slot is free, and reaching0 selects action1. Boating action1 requires a route and `take >= 6*BoatingSchool_CountWater(st)`, then attempts launch. The helper’s current body assigns the station key to every boat in the global list and counts nonzero assignments, so a nonzero station key uses the total global boat count, not water tiles or matching-school boats; success sits the rider, clears queue0, decrements its count and plays the launch sound pitched+10. Action2 holds. Action3 restores walking, clears riding, snaps to four tiles west/two south of the queue exit square and targets fixed-point offset `(-0xc0,+0x240)`;4 targets `(-0xc0,+0x80)`;5 targets centre and fades rider sounds over90;6 leaves. [ridecb5.c](../../LEGOLAND/ridecb5.c), [ridetiny.c](../../LEGOLAND/ridetiny.c)

Jungle action1 fills the first free one of the station’s three waiting-to-board slots, puts the visitor at off-screen `-0x270f`, and removes it from queue0;2 holds. Action3 converts the 3D person’s screen position back to the map (including the sprite half-width adjustment), clears riding and walks to the signed class exit offset;4 walks to the station centre;5 leaves. These waiting slots are separate from passengers already attached to a boat. [ridecb2.c](../../LEGOLAND/ridecb2.c)

Boating's once-per80-tick square step commits target to current, then mover state1 starts,4/8 steps with alternate turn preferences,16 finishes. Finish can return a different next boat after freeing the current, so the list walk must not advance it twice. Jungle docking state16 has leg3 freeze wobble64..79 at64; leg2 freeze0..63 at64 and aim south; leg1 aim south and freeze79..73 at72; leg0 frees and returns next. [ridemisc2.c](../../LEGOLAND/ridemisc2.c), [ridemisc.c](../../LEGOLAND/ridemisc.c)

`JungleCruise_AddValue` adds a signed scenery contribution to the first matching station’s `+40`, or does nothing when absent; it does not credit park money. `Pump_RemoveAllForSchool` caches next before removing each matching owner. `Road_TileRelease` converts unsigned 24.8 x/y by logical right shift8, then decrements pedestrian claims `+1d` only if the road exists and the byte is nonzero. [ridemisc4.c](../../LEGOLAND/ridemisc4.c), [ridetiny.c](../../LEGOLAND/ridetiny.c)

The recovered log-flume micro-helpers define a queue as `{path@0, head@4, tail@8}` with eight-byte `{next,rider}` nodes. `LFQueue_HasRider` checks only head. `LFQueue_FrontIsReady` requires head and its rider’s signed path index equal to the first path DWORD minus1. `LFPath_StartReverse` stores the path pointer at Bloke `+50`, sets index from the path’s signed low-word count minus1, sets walking direction−1, and increments action; zero count produces−1. `WalkPath_IndexOf` returns the raw `+50` field. `LFQuadBottomLeft` doubles tile width/height and returns `(doubled_width>>1, (doubled_height>>1)+doubled_height)`, the bottom-left quad coordinate. Full flume flow is in [transport.md](transport.md). [ridetiny.c](../../LEGOLAND/ridetiny.c), [scope-e.md](../lanes/scope-e.md)

### Tables and constants

| Item | Decoded values |
| --- | --- |
| Jungle passenger positions `0x0081cb80` | Three rows ×16 `{i32 x,y}` pairs; heading angle `a=i*22.5f`. Seat angles are `a`, `a+180.0f−16.0f`, `a+196.0f`; radians multiply by `0.017453292f`; x truncates `sin(angle)*−56.0`, y truncates `cos(angle)*28.0`. The angle constants are floats, radii doubles. This is a per-heading 56×28-pixel seat ellipse, distinct from the boat’s 80-frame movement buffers. [ridemisc2.c](../../LEGOLAND/ridemisc2.c) |
| Jungle queue offsets `0x004b72b0` (reverse indexing) | Slots0..4: `(-832,592),(-832,296),(-832,0),(-544,0),(-256,0)` in world fixed-point units. Boating’s analogous five offsets are indexed backwards from `0x004b52b0`, whose literals are not given here. [ridecb2.c](../../LEGOLAND/ridecb2.c), [ridecb5.c](../../LEGOLAND/ridecb5.c) |
| Road heading from16 body frames `0x004b4034` | `{3,3,4,5,5,5,6,7,7,7,0,1,1,1,2,3}`. [goldrush3.c](../../LEGOLAND/goldrush3.c) |
| Road ring / invalid corners | `N,NE,E,SE,S,SW,W,NW`; masks`0x07,0x1c,0x70,0xc1`. Block footprint`{0,0,3,3}`, pump`{0,0,0,3}`. [ridecb5.c](../../LEGOLAND/ridecb5.c), [ridecb8.c](../../LEGOLAND/ridecb8.c) |
| Jungle dock rectangles | `0x004b7278={0,0,4,4,next=&dock_b}`, `0x004b7260={0,-5,4,-1,next=NULL}`. Centers `(left+2,top+2)` supply route ends; start links north1/end south4. [ridecb9.c](../../LEGOLAND/ridecb9.c) |
| Water neighbor geometry | River links`N1,E2,S4,W8` step5 cells; cell footprint`{-2,-2,2,2}`. [ridecb2.c](../../LEGOLAND/ridecb2.c), [ridecb7.c](../../LEGOLAND/ridecb7.c) |
| Footprint edge tiles | Interior`+0`, west`+9`, north`+10`, northwest`+5`, southwest`+6`, south`+11`, southeast`+7`, northeast`+8`, east`+12`; x-edge tests win corners/one-cell-wide cases. Station paint uses only west/east edges. [ridecb7.c](../../LEGOLAND/ridecb7.c), [ridecb9.c](../../LEGOLAND/ridecb9.c) |
| Boating dock chain | Whole footprint`0x004cc078 → dockA 0x004cc060 → dockB 0x004cc048 → NULL`; dock masks1/4; station initial value5; left/right water tiles base+9/base+12. [ridecb5.c](../../LEGOLAND/ridecb5.c) |
| Boat dispatch index table `0x004193b0` | `{0,4,4,1,4,4,4,2,4,4,4,4,4,4,4,3}` for states1..16; outer state16 test makes its inner switch block unreachable. [ridemisc2.c](../../LEGOLAND/ridemisc2.c) |
| Driver rendering | Body palette livery1/2/3 →`0x0082c6bc/0x0082c6b8/0x0082c690`; driver sprite`0x00830f94`, frame last body frame`+0x10`; model heading `(body_frame+6)&15`. [goldrush4.c](../../LEGOLAND/goldrush4.c), [goldrush2.c](../../LEGOLAND/goldrush2.c) |

### Original bugs

| Fault / quirk | Source |
| --- | --- |
| Newly built boating station starts frame9999, but direction reverses only at exactly100; it increments beyond valid image frames and never plays normal building animation from that start. | [ridecb5.c](../../LEGOLAND/ridecb5.c) |
| Water/river removal dereferences a null bounds-checked cell for out-of-map coordinates. Jungle station update can dereference a missing station. Jungle station removal assumes a nonempty station head. | [ridecb5.c](../../LEGOLAND/ridecb5.c), [ridecb2.c](../../LEGOLAND/ridecb2.c) |
| Road cursor/placement with no cardinal neighbor can use an uninitialised school key (low pointer bits); UI constraints normally prevent it. School route reset dereferences a missing kind6 entrance; boating route rebuild dereferences a missing station. | [ridecb5.c](../../LEGOLAND/ridecb5.c), [ridecb6.c](../../LEGOLAND/ridecb6.c) |
| Water placement's owner is overwritten by each probe; final route rebuild uses the last probed arm's owner. Neighbor edits can join corners across station boundaries. Monkey-fish placement can retain an uninitialised owner if both probes fail. | [ridecb6.c](../../LEGOLAND/ridecb6.c), [ridecb7.c](../../LEGOLAND/ridecb7.c) |
| Boating removal increments WATER count twice and not MERMAID; jungle removal increments boat-class count twice. Jungle footprint paint/restore includes bottom row but excludes rightmost column. | [ridecb8.c](../../LEGOLAND/ridecb8.c), [ridecb2.c](../../LEGOLAND/ridecb2.c), [ridecb9.c](../../LEGOLAND/ridecb9.c) |
| Road/pump teardown frees before unlinking, caching next but comparing against the freed address. Pump removal sets the full-background-redraw flag before proving a pump exists. | [ridecb6.c](../../LEGOLAND/ridecb6.c), [ridemisc3.c](../../LEGOLAND/ridemisc3.c), [ridecb8.c](../../LEGOLAND/ridecb8.c) |
| Jungle station's three waiting-to-board pointers`+30..+38` are serialized raw; five queue people and actual boat's three rider slots are indexed. Saved station pointers can therefore be stale on reload. | [ridecb2.c](../../LEGOLAND/ridecb2.c), [ridecb7.c](../../LEGOLAND/ridecb7.c) |

`BoatingSchool_CountWater` mutates every boat owner/square word before testing it. Nonzero input returns total boat-list length; zero input returns0 after clearing all keys. It reloads the supplied key each iteration, preserving aliasing if the argument points into a boat. The station rider’s caller passes its station record, so simply checking launch capacity can reassign all boats to that station. [ridetiny.c](../../LEGOLAND/ridetiny.c), [ridecb5.c](../../LEGOLAND/ridecb5.c), [scope-e.md](../lanes/scope-e.md)

### Callback roles

These files implement transport select/cursor/place/remove/resource/tick/save slots: `Roads_CalcCursor`, `Roads_Add`, `Roads_Remove`; `BoatingSchool_Add`, `BoatingSchool_Tick`, `BoatingSchool_Remove`, `LoadBoatingSchool`/`SaveBoatingSchool`; `BsWater_Add`/`BsWater_CalcCursor`/water removal, mermaid add/cursor; `JungleCruise_Add`/cursor2/tick/remove/save/load; `JcWater_*`, `MonkeyFish_*`, `MonkeyTree_CalcCursor`. `BsWater_Probe` in `joust2.c` supplies the lake-arm test. `goldrush.c`'s `StepSchoolCar` and the helper contracts above connect callbacks to transport movement. [ridecb2.c](../../LEGOLAND/ridecb2.c), [ridecb5.c](../../LEGOLAND/ridecb5.c), [ridecb6.c](../../LEGOLAND/ridecb6.c), [ridecb7.c](../../LEGOLAND/ridecb7.c), [ridecb8.c](../../LEGOLAND/ridecb8.c), [ridecb9.c](../../LEGOLAND/ridecb9.c), [joust2.c](../../LEGOLAND/joust2.c), [goldrush.c](../../LEGOLAND/goldrush.c)

## Ride save chunks and provider boundaries

### Data structures

The ordinary ride chunk is a stream of repeated **`i32 1; raw record`**, terminated by **`i32 0`**. Records include original pointer bytes but loaders rewrite next links and designated saved references. The shared list records are the sizes/layouts above. The list called `RideDef.instances` in `ridesave.c` is physically at `+cc`, hence is the rider list: its node `+08` is the Bloke and `+10` the Person3D. The Person3D fixup at `+2c/+30` resolves the z-sprite pointer using the saved ownership/index value; Bloke `+54` points to the BNV path, whose first pointer/index pair resolves its path set. `ridesave.c`’s “map object”, “sample” and “instance” field names are therefore stale labels, not evidence of audio or of the actual placed-instance list at ObjDef `+04`. [ridesave.c](../../LEGOLAND/ridesave.c), [mechrides.c](../../LEGOLAND/mechrides.c), [catapult.c](../../LEGOLAND/catapult.c)

### Rules and state machines

Ordinary loaders read the flag, allocate/read record, clear next, link it, restore references, and repeat. The corresponding writers generally check every stream operation. Rider references are one-based list positions with zero null. A saved Person3D z-sprite index0 clears ownership; a nonzero index selects its class table. BNV paths have a separate convention: a nonnull path resolves its saved index directly, with no zero guard; mechanical RUN/ON/OFF path indices are `0/1/2`. Thus index0 is a valid running path. Catapult writes a temporary record with four rider pointers replaced by positions in ObjDef`+cc`; Copters loads six rider references; Tower converts eight car rider slots. Zoomer load restarts and immediately pauses the looping map-square sound. [ridesave.c](../../LEGOLAND/ridesave.c), [mechrides.c](../../LEGOLAND/mechrides.c), [catapult.c](../../LEGOLAND/catapult.c)

Transport chunks instead store **`i32 count; count raw records`** for each list. Jungle order is station/water/monkey-fish/monkey-tree/boat, boating station/water/mermaid/boat, driving school/road/pump/car. Load restores saved people indices and rebuilds station circuits. Coaster uses a separate relocatable blob: no castle is one zero integer; null read blob means failure, sentinel`-1` means absent coaster successfully loaded. [ridecb2.c](../../LEGOLAND/ridecb2.c), [ridecb6.c](../../LEGOLAND/ridecb6.c), [ridecb7.c](../../LEGOLAND/ridecb7.c), [ridecb8.c](../../LEGOLAND/ridecb8.c), [castleobj.c](../../LEGOLAND/castleobj.c)

### Tables and constants

| Ordinary ride | Raw size; next; special fixup |
| --- | --- |
| Gold / Jail / Elephant / Water | `0x2c;+0c`, `0x1c;+00`, `0x0c;+00`, `0x0c;+00`. [ridesave.c](../../LEGOLAND/ridesave.c) |
| Joust | `0x24;+04`; sample`+08=0` on load, then rider references. [ridesave.c](../../LEGOLAND/ridesave.c) |
| Safari / Spider / Barrels / Temple slide / Zoomer | `0x28;+10`, `0x30;+2c`, `0x34;+00`, `0x20;+08`, `0x24;+20`; restore BNV/z-sprite tables; Zoomer sound restart. [ridesave.c](../../LEGOLAND/ridesave.c), [mechrides.c](../../LEGOLAND/mechrides.c) |
| Catapult | `0x3c;+04`; rider references`+10,+14,+18,+1c`, one-based. [ridesave.c](../../LEGOLAND/ridesave.c), [catapult.c](../../LEGOLAND/catapult.c) |
| Copters | `0xd8;+04`; six rider references`+30,+50,+70,+90,+b0,+d0`. [ridesave.c](../../LEGOLAND/ridesave.c) |
| Tower | `0xb4;+08`; rider pairs`(+2c,+30),(+50,+54),(+74,+78),(+98,+9c)`. [ridesave.c](../../LEGOLAND/ridesave.c), [bswater3.c](../../LEGOLAND/bswater3.c) |

### Original bugs and disagreements

`SpaceTower_Save` replaces live rider pointers with indices **in place** and never restores the pointers after writing, corrupting the continuing live ride. Catapult avoids this by converting a copy. Ordinary loaders have no general rollback for partially rebuilt lists and do not bounds-check restored list indices. They seed an append cursor with null without clearing the existing head first: a nonempty incoming chunk replaces/leaks an old chain, while an empty incoming chunk leaves the old head untouched. Allocation results are not checked before passing the destination to the reader. Count-prefixed transport loaders ignore read results and return1. Jungle/boating replace the first lists without freeing old chains; their **boat** pass starts at the existing head and overwrites that head's next rather than appending at the tail. The Jungle header calls that last pass “water”, retaining its old reversed list names; the body operates on `g_jc_boats`. Driving load zeros each head first but still leaks the prior records. [ridesave.c](../../LEGOLAND/ridesave.c), [ridecb2.c](../../LEGOLAND/ridecb2.c), [ridecb6.c](../../LEGOLAND/ridecb6.c), [ridecb8.c](../../LEGOLAND/ridecb8.c)

### Callback roles

Each provider compares the LLIDB class name and overrides only its applicable ObjDef slots after generic callbacks are installed. The ODF loader calls resource load immediately afterward. `interfaces.c` houses the smaller and multi-class providers; `ridesave.c` houses Joust/Temple Slide providers; castle has its dispatch layer. A runtime should bind slot meanings by their callers and implementations, preserving old names only as address aliases. [interfaces.c](../../LEGOLAND/interfaces.c), [ridesave.c](../../LEGOLAND/ridesave.c), [castleobj.c](../../LEGOLAND/castleobj.c)

## Remaining source limits

The external-only tables called out above are genuine gaps: cafe geometry/orientation/depth, restaurant2 waiter steps, remaining restaurant1 seat rows, catapult layers/landing offsets, temple-slide thresholds, tower seat/direction rows and some mechanical timing tables. The C declares addresses and shapes but the reviewed headers do not provide their literal contents. This document does not turn unidentified data into guessed coordinates or use stale header match percentages as evidence of behavioural completeness. Plane/Spider/Barrels seat allocators are now recovered; the remaining external helper boundary includes the seven per-record machine steps, Tower seat picker, mechanical reset helpers and Earth Slide queue append. Full animations additionally depend on original `.bnv`, sprite and image-list assets. [ridecb4.c](../../LEGOLAND/ridecb4.c), [ridecb3.c](../../LEGOLAND/ridecb3.c), [ridemisc2.c](../../LEGOLAND/ridemisc2.c), [catapult.c](../../LEGOLAND/catapult.c), [joust.c](../../LEGOLAND/joust.c), [mechrides.c](../../LEGOLAND/mechrides.c), [ridemisc4.c](../../LEGOLAND/ridemisc4.c), [ridetiny.c](../../LEGOLAND/ridetiny.c)
