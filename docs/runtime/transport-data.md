# Transport binary and asset evidence

This supplement closes runtime-data gaps in [transport.md](transport.md). It uses the read-only original executable, SHA256 `c50865b60bfcb26c0a7329a75fb772b10ae234af324f669901906e5abb0e2bd9`, plus named original model assets. It does not execute the game or compile reconstructed C. The source baseline is `cf8e88c845dc4b109bbb2bd9f2a31d1177a9c9aa`; original instruction evidence resolves disagreements with that baseline's comments. Addresses are preferred-image VAs, integers are little-endian, and `f` denotes the exact stored IEEE binary32 value. BSS destinations are runtime storage, never fabricated static input. The extraction procedure at the end checks every static block against a SHA256 before printing its values.

## Shared water artwork and boat geometry

Both water tables contain the following same sixteen rows. A row is selected by the four cardinal arm bits; its 25 bytes paint the five rows of a 5×5 patch, top to bottom, left to right. Entries are tile indices added to tileset entry0. The two 400-byte hashes are identical. Consumers: [bswater.c](../../LEGOLAND/bswater.c), [junglecruise.c](../../LEGOLAND/junglecruise.c).

| Mask | Five screen/map rows of tile indices |
| --- | --- |
| 0x0 | 5,10,10,10,8 / 9,0,0,0,12 / 9,0,0,0,12 / 9,0,0,0,12 / 6,11,11,11,7 |
| 0x1 | 9,0,0,0,12 / 9,0,0,0,12 / 9,0,0,0,12 / 9,0,0,0,12 / 6,11,11,11,7 |
| 0x2 | 5,10,10,10,10 / 9,0,0,0,0 / 9,0,0,0,0 / 9,0,0,0,0 / 6,11,11,11,11 |
| 0x3 | 9,0,0,0,1 / 9,0,0,0,0 / 9,0,0,0,0 / 9,0,0,0,0 / 6,11,11,11,11 |
| 0x4 | 5,10,10,10,8 / 9,0,0,0,12 / 9,0,0,0,12 / 9,0,0,0,12 / 9,0,0,0,12 |
| 0x5 | 9,0,0,0,12 / 9,0,0,0,12 / 9,0,0,0,12 / 9,0,0,0,12 / 9,0,0,0,12 |
| 0x6 | 5,10,10,10,10 / 9,0,0,0,0 / 9,0,0,0,0 / 9,0,0,0,0 / 9,0,0,0,4 |
| 0x7 | 9,0,0,0,1 / 9,0,0,0,0 / 9,0,0,0,0 / 9,0,0,0,0 / 9,0,0,0,4 |
| 0x8 | 10,10,10,10,8 / 0,0,0,0,12 / 0,0,0,0,12 / 0,0,0,0,12 / 11,11,11,11,7 |
| 0x9 | 2,0,0,0,12 / 0,0,0,0,12 / 0,0,0,0,12 / 0,0,0,0,12 / 11,11,11,11,7 |
| 0xa | 10,10,10,10,10 / 0,0,0,0,0 / 0,0,0,0,0 / 0,0,0,0,0 / 11,11,11,11,11 |
| 0xb | 2,0,0,0,1 / 0,0,0,0,0 / 0,0,0,0,0 / 0,0,0,0,0 / 11,11,11,11,11 |
| 0xc | 10,10,10,10,8 / 0,0,0,0,12 / 0,0,0,0,12 / 0,0,0,0,12 / 3,0,0,0,12 |
| 0xd | 2,0,0,0,12 / 0,0,0,0,12 / 0,0,0,0,12 / 0,0,0,0,12 / 3,0,0,0,12 |
| 0xe | 10,10,10,10,10 / 0,0,0,0,0 / 0,0,0,0,0 / 0,0,0,0,0 / 3,0,0,0,4 |
| 0xf | 2,0,0,0,1 / 0,0,0,0,0 / 0,0,0,0,0 / 0,0,0,0,0 / 3,0,0,0,4 |

Boat and Jungle motion inputs are likewise byte-identical. In N/E/S/W index order, the step record is four ints `(dx,dy,sx,sy)`; each arc is four floats `(a0,a1,ox,oy)`. The consumer generates80 samples at radius640 with angle step `(a1−a0)/80`; these are the exact inputs to that existing algorithm. [bswater3.c](../../LEGOLAND/bswater3.c), [roads.c](../../LEGOLAND/roads.c)

| Entry | Step | Clockwise arc | Anticlockwise arc |
| --- | --- | --- | --- |
| N | (0,1,0,-1) | (270,180,1,-1) | (90,180,-1,-1) |
| E | (-1,0,1,0) | (360,270,1,1) | (180,270,1,-1) |
| S | (0,-1,0,1) | (90,0,-1,1) | (270,360,1,1) |
| W | (1,0,-1,0) | (180,90,-1,-1) | (0,90,-1,1) |

The decoration allocation boundary is closed by original instructions: monkey tree `0x433d44: push8; 0x433d46: call0x49e4ff`, fish `0x434142: push0xc; 0x434144: call0x49e4ff`, and extra decoration `0x434891: push8; 0x434893: call0x49e4ff`. Their `{own u16,owner u16,next}` records therefore have exactly the removal-view sizes8/12/8; the fish inserts a cleared dword at+4 and puts next at+8. Fish `+4` is runtime state, not a larger allocation. Compare the fish constructor in [ridecb7.c](../../LEGOLAND/ridecb7.c) and removal consumers in [junglecruise.c](../../LEGOLAND/junglecruise.c). Owner probes remain unchecked as those sources describe.

## Flume records and overlay ordering

The common track footprint is `{left=0,top=0,right=2,bottom=2,next=0}` at0x4b4728. Geometry uses differences2×2, while inclusive terrain rectangles use their own stated comparisons. Shape lookup at0x4b473c is `[6,7,8,9,4,5,0,1,2,3,10]`; kind3 uses indexdir, kind1 uses4+dir, kind2 uses6+dir, kind4 uses10. [logflume.c](../../LEGOLAND/logflume.c), [logflume2.c](../../LEGOLAND/logflume2.c)

Two distinct record formats share a misleading `{count,pointer}` declaration in the old source:

| Table | Payload and actual interpretation |
| --- | --- |
| Entrance A,0x4b47b8 →0x4b4798 | 4 pairs `(0,−1),(3,0),(0,−2),(−4,0)` |
| Entrance B,0x4b47e8 →0x4b47c0 | 5 pairs `(0,0),(0,1),(3,0),(0,3),(−4,0)` |
| Corner1,0x4b4808 →0x4b47f0 | 2 triples `(0,3,null),(4,5,null)` |
| Corner3,0x4b4828 →0x4b4810 | 2 triples `(0,3,null),(4,5,null)` |
| Hold-up,0x4b4858 →0x4b4830 | 3 triples `(0,8,null),(9,9,null),(10,11,null)` |

Entrance pairs are path displacement vectors consumed by `LFAnim_Create` at0x412100, with stride8. Each segment contributes truncated Euclidean length `int(sqrt(dx²+dy²))` samples. Overlay triples instead have stride12 and mean `{unused-by-drawer, cumulativeChildPieceThreshold, sprite}`. `LFPiece_DrawAnim` at0x40b290 selects the compound piece's endpoint+30 or endpoint+34 using flag0/nonzero, walks child-list+4 or child-list+0 respectively, and for each child draws any boat overlapping that child, from the owning run's inline boat array. It increments a child-piece counter, compares it with frame+4 at0x40b31e–328, draws a nonnull frame+8 when that cumulative threshold is reached, advances12 bytes, and continues. The count is **not reset** when switching overlay records. At child-list end it draws the current nonnull sprite once more. It passes the caller's x/y unchanged to the sprite draw. Frame+0 is never loaded and the table count is never used as a bound by this drawer. These raw instructions supersede the old `{dx,dy,sprite}` description; offsets come from the call sites themselves. [logflume.c](../../LEGOLAND/logflume.c), [lfentrance.c](../../LEGOLAND/lfentrance.c)

Loaded sprite pointers replace the nulls at0x4b47f8/4804,0x4b4818/4824 and0x4b4838/4850; hold-up's middle+4844 stays null. Since the drawer does not bound its table walk, a longer unexpected child list may overrun the overlay records. This is an original unchecked-input contract, not a claim that ordinary layouts trigger it. [logflume.c](../../LEGOLAND/logflume.c), original0x40b348–35a.

Queue path samples are12 bytes: signed-int x/y at+0/+4 and an unused dword at+8, initialized zero by the constructor and preserved in raw queue saves. Original position getter0x4120e0 reads only x/y. The constructor zeroes `8+12*count` bytes, then fills coordinates. Its write occurs before updating the interpolation accumulators, so each segment's first coordinate repeats: for sample j in a segment of integer length n, use the segment start when j=0, otherwise start+trunc((j−1)*(dx,dy)/n). After n samples the next segment starts at start+(dx,dy). Thus entrance A has10 samples `(0,0),(0,−1),(0,−1),(1,−1),(3,−1),(3,−1),(3,−3),(3,−3),(2,−3),(1,−3)`; B has11 `(0,0),(0,1),(0,1),(1,1),(3,1),(3,1),(3,2),(3,4),(3,4),(2,4),(1,4)`. The leading zero-length B segment emits no samples. This original repeated-first-sample behaviour is retained; the final endpoint itself is omitted. Original0x412100–283,0x4120e0–f5; [logflume5.c](../../LEGOLAND/logflume5.c), [logflume6.c](../../LEGOLAND/logflume6.c).

The overlay overlap predicate0x40b210 accepts the boat's own piece, its forward+8 neighbour for z≥0.5, or its backward+c neighbour for z<0.5. This is compound-child ordering, not the separate rider queue at run+2c. The entrance compound piece is also the previously unnamed run+18 pointer: construction sets it to the entrance-class piece at the station square and the entrance draw queries rider presence through it. [lfentrance.c](../../LEGOLAND/lfentrance.c), original0x40b210–262.

## Coaster descriptors and geometry inputs

The common `TrackDesc` payload is **0x34 bytes**, ending at path flag+30. Four square descriptors have a trailing zero alignment dword and are spaced0x38 apart. The castle descriptor has no such dword:0x4b5b7c already starts the next `CAST...` string. A universal0x38 descriptor size would read that string as a field. Joint parameters at+0c/+14 contain a direction mask and a packed pair of signed shorts; the shipped offsets are allzero. [coaster.c](../../LEGOLAND/coaster.c), [coaster5.c](../../LEGOLAND/coaster5.c), raw descriptor blocks below.

| Descriptor | VA | flags,heightIn,heightOut | joint masks | hooks +1c,+20,+24,+28,+2c | path+30 |
| --- | --- | --- | --- | --- | --- |
| Castle | 0x4b5b48 | (1, 0, 0) | (2, 4) | 0x423940,0x423970,0x423990,0x0,0x0 | 0 |
| Flat | 0x4b5d20 | (0, 0, 0) | (15, 15) | 0x427a40,0x427a80,0x4286e0,0x4275c0,0x4275b0 | 0 |
| High | 0x4b5d58 | (1, 25, 25) | (15, 15) | 0x427c30,0x427c70,0x4286e0,0x4275c0,0x4275b0 | 0 |
| Low | 0x4b5d90 | (1, 2, 2) | (15, 15) | 0x427c30,0x427c70,0x4286e0,0x4275c0,0x4275b0 | 0 |
| Path | 0x4b5dc8 | (1, 15, 15) | (15, 15) | 0x427c30,0x427c70,0x4286e0,0x427ff0,0x427f70 | 1 |

The raw0x429910 predicate, called `TrackJointSloped` in source, returns1 exactly when descriptor flags==0 and outgoing direction equals `Opposite(incoming)`; otherwise0. It tests the entire flags dword for zero, not just bit0. Helpers0x429940/0x429990 count those predicates along predecessor/successor links until reaching a descriptor with bit0 set, returning the stopping node through their output pointer. This closes the externally declared classification used by height-profile construction. [coaster5.c](../../LEGOLAND/coaster5.c)

| Input | VA | Exact ordered values |
| --- | --- | --- |
| Edge midpoints | 0x4b5e00 | (1,0,0); (2,1,0); (1,2,0); (0,1,0) |
| Half steps | 0x4b5e30 | (0.5,0,0); (-0.5,0,0); (0,0.5,0); (0,-0.5,0) |
| Headings | 0x4b5e60 | (1,0,0); (0,-1,0); (-1,0,0); (0,1,0) |
| Corners | 0x4b5e90 | (0,0,0); (2,0,0); (2,2,0); (0,2,0) |
| Straight heading order | 0x4b5ec0 | (0,2,1,3) |
| Straight lateral offsets | 0x4b5ee0 | (-4,-2,0,2,4) |
| Corner heading pairs | 0x4b5ef4 | (0,3); (2,3); (2,1); (0,1) |
| Corner radius pairs | 0x4b5f14 | (1.5,0.5); (0.5,1.5); (1.5,0.5); (0.5,1.5) |

The four Vec3 source arrays begin0x4b5e00/30/60/90, as the body loads establish. The older introductory prose listing0x4b5df8/e28/e58/e88 starts eight bytes too early and includes preceding records. Each source component is multiplied by20 for live geometry. The builder then emits8 arcs and20 straights, with the exact pair/radius/order inputs above. [coaster3d.c](../../LEGOLAND/coaster3d.c)

## Static support and shadow models

Both models use the LMS triangle schema below, with live vertex buffers and static topology. The shadow header at0x4b6150 is `{vertices=12,smoothNormals=0,triangles=16,positions=0x6137e8,faceNormals=0x4b6110,smoothNormalPtr=0,flatFaces=0x4b6010,flatCount=16,smoothFaces=0,smoothCount=0,texturedFaces=0,texturedCount=0}`. Support header0x4b6300 is `{8,0,8,0x614858,0x4b6240,0,0x4b6270,8,0,0,0,0}`. This is a compact shared-normal representation: only5 and4 normal vectors respectively are referenced, despite larger triangle totals. [coaster4.c](../../LEGOLAND/coaster4.c), [coastertiny.c](../../LEGOLAND/coastertiny.c)

| Input | Exact values |
| --- | --- |
| Shadow vertex template | (1,1,0); (-1,1,0); (-1,-1,0); (1,-1,0); (1,1,0); (-1,1,0); (-1,-1,0); (1,-1,0); (1,1,0); (-1,1,0); (-1,-1,0); (1,-1,0) |
| Shadow face normals | (1,0,0); (0,1,1); (-1,0,0); (0,-1,0); (0,0,-1) |
| Support vertex template | (1,-1,-1); (1,1,-1); (-1,1,-1); (-1,-1,-1); (1,-1,1); (1,1,1); (-1,1,1); (-1,-1,1) |
| Support face normals | (0,0,-1); (0,0,1); (1,0,0); (-1,0,0) |
| Support material0 | first dword1; following8 byteszero, unused by flat-color pass |

Each face below gives all eight shorts `(material,faceNormal,vertex0,vertex1,vertex2,extra0,extra1,extra2)`; the final three arezero.

| Face | Shadow | Support |
| --- | --- | --- |
| 0 | (0,4,0,1,4,0,0,0) | (0,0,0,1,2,0,0,0) |
| 1 | (0,4,4,1,5,0,0,0) | (0,0,0,2,3,0,0,0) |
| 2 | (0,4,1,2,5,0,0,0) | (0,1,7,6,5,0,0,0) |
| 3 | (0,4,5,2,6,0,0,0) | (0,1,7,5,4,0,0,0) |
| 4 | (0,4,2,3,6,0,0,0) | (0,2,0,5,1,0,0,0) |
| 5 | (0,4,6,3,7,0,0,0) | (0,2,0,4,5,0,0,0) |
| 6 | (0,4,3,0,7,0,0,0) | (0,3,2,7,3,0,0,0) |
| 7 | (0,4,7,0,4,0,0,0) | (0,3,2,6,7,0,0,0) |
| 8 | (0,0,4,5,8,0,0,0) | — |
| 9 | (0,0,8,5,9,0,0,0) | — |
| 10 | (0,1,5,6,9,0,0,0) | — |
| 11 | (0,1,9,6,10,0,0,0) | — |
| 12 | (0,2,6,7,10,0,0,0) | — |
| 13 | (0,2,10,7,11,0,0,0) | — |
| 14 | (0,3,7,4,11,0,0,0) | — |
| 15 | (0,3,11,4,8,0,0,0) | — |

The shadow builder scales the first four template x values by5 and mistakenly assigns y=x; the next eight x/y values scale by1.5. Its static z values arezero. Ground projection and the upper sloped plane are rebuilt by the draw helper. Support vertices scale x×2.5,y×8.2 and preserve z bits. No external model geometry is needed for these two models. The shadow's material record at0x615f70 begins zero-initialized in PE BSS, so its first dword selects palette index0. This draw path only reads that record; it does not load an external material file. [schoolcar8.c](../../LEGOLAND/schoolcar8.c), [coaster4.c](../../LEGOLAND/coaster4.c), [coastertiny.c](../../LEGOLAND/coastertiny.c)

## Complete track-piece and route-node storage

Original `GetTrackNodeWorldPos` at0x41cff0 tests node state bit0. For a raised node it copies the cached Vec3 at+40 and returns `lea eax,[esi+0x4c]` at0x41d012. For ordinary geometry it calculates world position and calls descriptor+24. Thus an allocated piece is exactly `0x2c graph prefix +0x14 footprint +0x0c cached world position +0x58 inline geometry =0xa4`. The footprint at+2c ends before the world position, rather than overlapping it; the earlier0x24 footprint/padding view was too broad. The same0x58 geometry union supports line/cubic/arc forms already described in [transport.md](transport.md); its unused variant fields do not imply an unimplemented tail. Constructors0x41d5b0/0x41d630 allocate0xa4 and class attach/build paths select and populate geometry. [coaster.c](../../LEGOLAND/coaster.c), [coaster4.c](../../LEGOLAND/coaster4.c), [coaster3d.c](../../LEGOLAND/coaster3d.c)

The complete0xec route-node allocation has the following runtime interpretation. Original setter0x41e820, frame builder0x41e9e0, drawing0x41ea70, cursor helpers0x42a5e0/0x42a620/0x42a680, and clip helpers0x41e950/0x41e970/0x41e990 establish the missing regions. These are original instruction ranges reproduced by the check below; [schoolcar8.c](../../LEGOLAND/schoolcar8.c) supplies the surrounding seat/init and node-list contracts.

| Offset | Size | Meaning |
| --- | --- | --- |
| +00/+04 | 4+4 | flags / model kind |
| +08/+40 | 0x38 each | front / rear TrackCursor |
| +78/+98 | 0x20 each | two RouteSeat records |
| +b8 | 0x0c | midpoint of the two mode2 cursor samples |
| +c4 | 4 | potential energy = mass0.1f × midpoint.z × −0.00134937500115484f (bits0xbab0dd83) |
| +c8 | 0x1c | clip rectangle: four intbounds, mask, two ring links |
| +e4/+e8 | 4+4 | previous / next route node |

Each TrackCursor is `{float currentT +00; RoutePos current +04; float wheelPhase[2] +18; float previousT +20; RoutePos previous +24}`. `0x42a620` updates only the current pair. `0x42a5e0` initializes both current and previous pairs, without clearing phases. `0x42a680` samples two wheel placements, adds the result of0x42a670 to each phase, draws the wheel models, then snapshots current into previous. That increment helper returns0.0f, so the recovered code preserves existing phase values rather than advancing them. Mode mapping at0x4b6408 is `[0,2,1]`; mode2 therefore requests geometry pair1, position slot2, despite the old `GetTailTangent` name. Both cursor samples use offset4.8f. Rear placement is solved at separation30.0f by0x429f30; node midpoint is their average. Frame forward is front minus rear, then0x429af0 constructs its basis. [coastertiny.c](../../LEGOLAND/coastertiny.c), [schoolcar8.c](../../LEGOLAND/schoolcar8.c), original0x41e820–8ed,0x41e9e0–a6f,0x42a640–669,0x42a670–676,0x42a680–77e.

Seats contain local Vec3+00, occupant/car+0c, then occupied-query/attach/destruction-request/frame-draw hooks+10/+14/+18/+1c installed as0x4273c0/0x4273d0/0x4273f0/0x427410. Initial positions are `(x,0,−4)` and `(x−16,0,−4)` for kind-x table`[0,4,8]`. Draw temporarily translates a `{position,basis}` frame by the seat's local coordinates, calls occupied car+20 and restores all three translation components. The+18 hook calls occupied car+24 (destructor), while separate0x4273e0 clears/returns the seat car pointer; empty seats skip destruction-request/frame-draw. Original0x4273d0–4ef; [schoolcar8.c](../../LEGOLAND/schoolcar8.c), [coastertiny.c](../../LEGOLAND/coastertiny.c), [coaster8.c](../../LEGOLAND/coaster8.c).

## Coaster9 helper reconciliation

The later source confirms prior binary evidence for seat destruction/frame draw, geometry offsets, potential-energy+c4 and the three model passes. It also resolves two misleading names: ModelRecord_CopyToken copies a complete CRLF-delimited line, and RouteNode_AddPending draws a matching train once before setting bit0; it does not link a pending queue. The source and lane are [coaster9.c](../../LEGOLAND/coaster9.c) and [codex-e.md](../lanes/codex-e.md).

Original0x41e670 compares target with front/rear piece pointers at+c/+44. The callee0x41ea70 sets the car view, draws both wheel cursors, draws the kind-selected model/material, then calls both seat draw methods. Wrapper0x41eaf0 skips completed or unmatched nodes and sets bit0 after drawing. Original0x41dca0 derives speed from energy minus the potential sum and mass, with a nonpositive clamp; it is not a read of raw route+28. These newly clarified aliases use the same storage and equations already recovered above. [coaster9.c](../../LEGOLAND/coaster9.c), [coaster8.c](../../LEGOLAND/coaster8.c)

The travel multiplier at0x4ab404 is one binary32 value, bits0x3ad013a9, exact value0.001587499980814755; its nominal source literal is0.0015875f. `Route_TravelPerTick` multiplies tangent length by supplied speed and this constant. The manifest and extraction block below pin its bytes. `ModelImage_FindName`0x422400 searches lines through GetName and a case-insensitive compare; the index helpers select OBJ versus TXT. All these bounded helper spans are included in the original instruction recipe. [coaster9.c](../../LEGOLAND/coaster9.c), [coaster8.c](../../LEGOLAND/coaster8.c)

## LMS, LFM, LTX and palette assets

The source's six LMS pointer fixups are correct, but the remaining fields are now identified by the original render passes. These disk offsets become pointers after load. Raw0x420e90 projects position count+00 from pointer+0c, then calls flat0x420810, smooth0x420a20 and textured0x420c40 passes. `0x420fb0` sends the eight Vec3 beginning+30 to0x426750 for bounds. The shipped six models named by `ROLLERCOASTER.obj` include this0x60-byte bounding-box block before their vertex data at+90; the four other loose test/older meshes start vertices at+30 and have no separate bounds block. [schoolcar7.c](../../LEGOLAND/schoolcar7.c), [coaster6.c](../../LEGOLAND/coaster6.c), [schoolcar.c](../../LEGOLAND/schoolcar.c), original consumers above.

| Offset | Type | Disk/runtime meaning |
| --- | --- | --- |
| +00 | u32 | vertex count |
| +04 | u32 | smooth-normal count |
| +08 | u32 | total triangle count |
| +0c | offset/pointer | Vec3 positions, count+00 |
| +10 | offset/pointer | Vec3 face normals; disk models have count+08, built-in models share fewer normals |
| +14 | offset/pointer | Vec3 smooth normals, count+04 |
| +18/+1c | offset/pointer,u32 | flat-colored triangle array/count |
| +20/+24 | offset/pointer,u32 | smooth-colored triangle array/count |
| +28/+2c | offset/pointer,u32 | textured triangle array/count |
| +30…8f | optional8Vec3 | bounding box corners for the six registered train/rider models |

Every triangle is16 bytes: signed-short material/LFM index+00, face-normal index+02, vertex indices+04/+06/+08, and three extra shorts+0a/+0c/+0e. Flat and textured passes use the face-normal index; smooth uses the three extra shorts as per-vertex normal indices. The remaining extras are ignored by the flat/textured consumers. All three reject nonpositive projected signed area and triangles whose ORed clip bits lack any required low bit; this is back-face rejection plus common-edge clipping, not a per-vertex requirement. They submit raster modes2/3/4. [coaster3d.c](../../LEGOLAND/coaster3d.c), original0x420810–a12,0x420a20–c31,0x420c40–e82.

LFM is a raw sequence of12-byte material records, with no header. First dword is a **palette or texture index**, according to the consuming triangle list. For textured triangles bytes+4…9 are `(u0,v0,u1,v1,u2,v2)`; +a/+b are unused. Colored records ignore all8 trailing bytes. Many original unused bytes are0xcd, retained by copying but never meaningful pointers. The original loader0x4206d0 does no relocation/conversion and returns the raw byte length through its second argument. The rider substitution loops replace the first dword only, comparing indices returned by palette/part searches even though the local C declarations are pointer-typed. [coaster6.c](../../LEGOLAND/coaster6.c), [coaster7.c](../../LEGOLAND/coaster7.c), [schoolcar8.c](../../LEGOLAND/schoolcar8.c)

LTX is `{u32 width,u32 height,u8 texel[width*height]}`. Each texel indexes the shared shade/palette ramp. All ten shipped LTXs are32×32,1032 bytes. Original0x423080–13b reads dimensions+0/+4, begins pixels+8 and maps each byte through0x829c60. The textured span setup0x4288bd–906 takes the width/height logarithms and maps the0…255 UV coordinate domain to the texture dimensions; no per-file pointers need relocation. [schoolcar8.c](../../LEGOLAND/schoolcar8.c), [schoolcar5.c](../../LEGOLAND/schoolcar5.c)

`ROLLERCOASTER.obj` registers, in order: `sit.logirlsit`, `sit.lomansit`, `coastertrain.wheel01`, `coastertrain.midcar`, `coastertrain.tailcar`, `coastertrain.headcar`. `ROLLERCOASTER.txt` registers texture indices0…9: `Chest girly2`, `coastwheel`, `Chest visitor1`, `Chest_V13`, `Chest_V2`, `Face02`, `Chest_V9`, `Face13`, `Face01`, `man_head`. Records are CRLF separated. The palette is a count145 followed by145 little-endian RGB dwords; exact index order is below. [coaster7.c](../../LEGOLAND/coaster7.c), [coastertiny.c](../../LEGOLAND/coastertiny.c), original asset bytes checked below.

| Starting index | RGB values in consecutive index order |
| --- | --- |
| 0 | 0000ff 000000 007bc6 732910 008c4a bdcede f71821 ffffff ffd600 f11a22 |
| 10 | ffd100 008b4a 722916 191919 adc7ba ffce00 d6ece2 42411c fdd414 0e120a |
| 20 | f1f4f1 5b6758 8ec7ac 269845 f7fcf9 dcc4bc b4691c c1b72c ba674f 0a8942 |
| 30 | a8ac87 f8c401 f8eee4 fddc3b b5ced6 57a8d7 86b18a fff7d6 ffe77b d8c620 |
| 40 | 4298b5 7fbde2 298cce aed7ed 98cae9 f7f7ff 1884c6 a5d6ef 5aa9d6 eff7ff |
| 50 | cee7f7 3194ce 1084c6 c6deef deeff7 b1d6ef b5deef 4ca2d6 087bc6 f7ce08 |
| 60 | e7c608 100800 dabb0c 95820b 3a3202 655806 cead08 181800 251f00 786a06 |
| 70 | 4f4505 bfa308 efc608 ab9408 ffe721 ffa0a5 f7777b f7636b fc8689 ffcece |
| 80 | ffffef fffff7 feeb54 f8f4f1 fcf7ba 09060e 150e0b e2deca 413511 4d473a |
| 90 | fde01a c4bd94 847e69 040404 947a15 faf7dc 261f08 252019 080800 695611 |
| 100 | f7c608 f9ce0a d1b113 151001 ffd608 291804 120d02 b19418 9c7916 100808 |
| 110 | 342308 0c0804 6e5d11 efce10 e4c015 181000 47310a d6b71b 1d1303 5a460d |
| 120 | f7d608 f7de18 f8f55c ffff63 f3f76b f7d610 ffce08 fffb6b f5f770 fff746 |
| 130 | f7e83d fcfc73 737b74 808378 8b8c80 797d84 d2d076 ffef3d 7e8391 fbf889 |
| 140 | ffd610 76808f 6f7491 ffe629 f7d618 |

The asset checker validates all ten available LMS/LFM pairs, including the four unregistered loose models, and all ten LTXs. Vertex/normal/face arrays consume each LMS exactly to EOF; every used index is range-checked. The following file hashes fix the actual geometry/material/image inputs without substituting reconstructed placeholders.

| Mesh | Vertices | Smooth normals | Triangles | Flat | Smooth | Textured |
| --- | --- | --- | --- | --- | --- | --- |
| coaster.Line01.lms | 119 | 58 | 366 | 318 | 48 | 0 |
| coaster.wheel01.lms | 14 | 26 | 24 | 0 | 0 | 24 |
| coastertrain.headcar.lms | 99 | 40 | 312 | 292 | 20 | 0 |
| coastertrain.midcar.lms | 28 | 40 | 104 | 84 | 20 | 0 |
| coastertrain.tailcar.lms | 48 | 58 | 158 | 110 | 48 | 0 |
| coastertrain.wheel01.lms | 14 | 26 | 24 | 0 | 0 | 24 |
| damo.Box01.lms | 8 | 24 | 12 | 0 | 0 | 12 |
| sit.logirlsit.lms | 113 | 91 | 154 | 82 | 62 | 10 |
| sit.lomansit.lms | 93 | 79 | 124 | 62 | 50 | 12 |
| sitgirl.logirl.lms | 113 | 91 | 154 | 82 | 62 | 10 |

| File | Bytes | SHA256 |
| --- | --- | --- |
| ROLLERCOASTER.obj | 116 | `687321206c62577c2a9a9c0761dec0f12ff18226ed329e23ec2d790f37f95e8d` |
| ROLLERCOASTER.txt | 107 | `50d19a5e903f84a5e8debc6e032379a845a7ec4137bd1bcf2005d8ae4d8c7acc` |
| ROLLERCOASTER0000.ltx | 1032 | `8916986d81bb082e9a871aedfe3bc7c466e41f2743407214ab60b79b882718bb` |
| ROLLERCOASTER0001.ltx | 1032 | `c41051e86b085a88dce2388c1d8c1456a322e29e746abbb3da9bd337eca13db8` |
| ROLLERCOASTER0002.ltx | 1032 | `4b913075ae0c8e43a356d4c96b2b552fec7f1256a408d75a4529c9301db8b505` |
| ROLLERCOASTER0003.ltx | 1032 | `c61f613ffab9642ddd5bd502ea4d5bbd74129bcd081ea4624df0f93013eb3c44` |
| ROLLERCOASTER0004.ltx | 1032 | `70d8571ee4ef78fb468987a6bdeb6a58b6b7ba85f38dfab130be288fcd75be95` |
| ROLLERCOASTER0005.ltx | 1032 | `9034f62ba9a335973e690c8aa94e1a611e31a03f3bdada1039825f7fd32fa788` |
| ROLLERCOASTER0006.ltx | 1032 | `a2c793a5896682c63f86d438c6c14e1836308482d601c7beea8aa607c23eac1a` |
| ROLLERCOASTER0007.ltx | 1032 | `e89fae8658d193e3da9bb10363116aaa838a7b553484122281357a4ea4a2804f` |
| ROLLERCOASTER0008.ltx | 1032 | `47b75f3912ea24b1c67ac9cebb21110f6d53521dde45b48376aa45d442c93532` |
| ROLLERCOASTER0009.ltx | 1032 | `2484e43bdb57764aa9906f509850832cd2a4d9d61e79925a67a96af62d050433` |
| Rollercoaster.lpt | 584 | `bf27bee9e631dca0911a3f847b09c0d6f8328ef7e5927e272f1fccc920da78d1` |
| coaster.Line01.lfm | 12 | `de66807afa64bbe18c1e9ca78633c026dcee7be300f3dffc7353e002c9b8c2d2` |
| coaster.Line01.lms | 12420 | `66659c520ae86b863d0286bbdfec8d8b9be8d5c777771156800a54b322838e9c` |
| coaster.wheel01.lfm | 288 | `a416df9d09ade5153d29d27743b47da21c52222e0002a48c1abb5b81b740a90e` |
| coaster.wheel01.lms | 1200 | `c6ea6b9c9409dc3dc571fc9f19b1a26b036388e482d08c8549cb3bedc0acf073` |
| coastertrain.headcar.lfm | 12 | `86fbb29c2651dcbb804c8d4ac1f3bed87221427976fecd213cd280b71bc8522b` |
| coastertrain.headcar.lms | 10548 | `93975e75c2de773ee13501af8b519050a87cd9d7ec7be3385963aa2d9133de26` |
| coastertrain.midcar.lfm | 12 | `86fbb29c2651dcbb804c8d4ac1f3bed87221427976fecd213cd280b71bc8522b` |
| coastertrain.midcar.lms | 3872 | `c2a4ebf9e317481acd6744b02fbd552511000fb4e42a3f8ab28682ed3908b2aa` |
| coastertrain.tailcar.lfm | 12 | `86fbb29c2651dcbb804c8d4ac1f3bed87221427976fecd213cd280b71bc8522b` |
| coastertrain.tailcar.lms | 5840 | `543944e79d16c3643bf25bfc99cbbabacb08657c6a6f27c3d0cb8956f8ad68e3` |
| coastertrain.wheel01.lfm | 288 | `ed029615b5dd41f76aff5e0cd94c61ae114331a6f3d855ea5a7b53a6fc75216b` |
| coastertrain.wheel01.lms | 1296 | `d33b669f6e439ff5763ceef2962e5f97b44f21cff2928193b230eba3d4d1b8e6` |
| damo.Box01.lfm | 144 | `e22dcc32efcbbe7f50c3e71d77326976953b690d3e5495cc714dcdb16bff882f` |
| damo.Box01.lms | 768 | `db120c40b18761d7c6a09c90bbb6fe9013cb158d36f72c5c70e15e92f9834ac4` |
| sit.logirlsit.lfm | 180 | `1e2874c7144e3c8abfc446aac6d719c412633a6d1e4aa3a92e7e782ba0c81ef4` |
| sit.logirlsit.lms | 6904 | `0318d1c44f9e09e174799898c2e179e684fcb8a73ff5f4febee871c9673e225a` |
| sit.lomansit.lfm | 192 | `4241427940d3231267abfd7e0e28837fc947904198f390e3b544e38f4efcae6c` |
| sit.lomansit.lms | 5680 | `7a4ebf17ad21d1a16130d82c3415b8ad28416dade238c01bdcbc5a43ae79c4bd` |
| sitgirl.logirl.lfm | 180 | `3be0b23721a1086c9d9821456440b01a0901353093a5ac79ff4cbef081cad128` |
| sitgirl.logirl.lms | 6808 | `e287107fba4931eb58e72aa93056f65b1ea69074357bba7b2e48a4610fa0f89d` |

## Original ODF footprints and flume image lists

The original archive `Legoland.res` has SHA256 `b8cd7ee4a98c7da31e0f8aeb7495717a8ea320a73aa98a515b62ab13055627e2`. All15 members were independently checked against the complete tail-directory tree, using `Objdesc/` for ODF and `ImageData/` for ILF; basename scanning is not the membership proof. The ODF loader reads the first length dword, then copies file+4 to runtime ObjDef+4; therefore the four signed footprint ints at file+3c/+40/+44/+48 retain those runtime offsets. It clears next at+4c separately. These values belong to the named archive members, not to stale hard-coded object callbacks embedded in old ODF headers. Creation callbacks may subsequently replace/adjust geometry; the ordinary flume grid uses the distinct static0…2 footprint above. [llidb_odf.c](../../LEGOLAND/llidb_odf.c), [logflume.c](../../LEGOLAND/logflume.c), [logflume2.c](../../LEGOLAND/logflume2.c)

| ODF member | Archive offset | left,top,right,bottom |
| --- | --- | --- |
| LOG FLUME ENTRANCE.ODF | 0x48bee4 | (-5, -5, 7, 4) |
| LOG FLUME TRACK.ODF | 0x489f20 | (0, 0, 1, 1) |
| LOG FLUME SPECIAL CORNER 1.ODF | 0x48b7ac | (-3, -3, 5, 5) |
| LOG FLUME SPECIAL CORNER 2.ODF | 0x488c4c | (-2, -2, 5, 5) |
| LOG FLUME SPECIAL CORNER 3.ODF | 0x488a80 | (-2, -3, 6, 5) |
| LOG FLUME SPECIAL CORNER 4.ODF | 0x4888a0 | (-2, -3, 6, 5) |
| LOG FLUME TUNNEL.ODF | 0x48b998 | (-3, -4, 5, 5) |
| LOG FLUME CSAW.ODF | 0x48bb40 | (-5, -3, 8, 4) |
| LOG FLUME HOLD UP.ODF | 0x4886ec | (-6, -6, 7, 7) |
| LOG FLUME DROP.ODF | 0x488534 | (-2, -9, 3, 14) |
| CASTLE.ODF | 0x490dc8 | (-4, -6, 6, 8) |

The image-list decoder reads u16 count/u16 type (here2), a length-prefixed list name, count×2 signed-int sprite offsets, then count length-prefixed sprite names. The tables below show file offsets; `LLIDB_LoadILFData` doubles both x/y values when populating runtime arrays, so rendered ILF offsets are twice these signed integers. The exact overlays and z-mask images are therefore reproducible from archive data rather than unspecified image tables. This list format agrees with [llidb_load.c](../../LEGOLAND/llidb_load.c); the flume consumers are [logflume.c](../../LEGOLAND/logflume.c), [logflume4.c](../../LEGOLAND/logflume4.c).

### LOG FLUME IMAGE LIST.ILF

| Index | x/y offset | Sprite |
| --- | --- | --- |
| 0 | (-30, -36) | flumec1a.lls |
| 1 | (-30, -36) | flumec2a.lls |
| 2 | (-31, -36) | flumec3a.lls |
| 3 | (-31, -37) | flumec4a.lls |
| 4 | (-30, -36) | flumes1.lls |
| 5 | (-31, -36) | flumes2.lls |
| 6 | (-30, -48) | flumee2.lls |
| 7 | (-30, -36) | flumee3.lls |
| 8 | (-30, -37) | flumee4.lls |
| 9 | (-31, -47) | flumee1.lls |
| 10 | (-31, -48) | flumee5.lls |

### LOG FLUME TRACK ENDY LIST.ILF

| Index | x/y offset | Sprite |
| --- | --- | --- |
| 0 | (33, -54) | nuend1.lls |
| 1 | (32, -16) | nuend2.lls |
| 2 | (-39, -18) | nuend3.lls |
| 3 | (-40, -55) | nuend4.lls |

### LOG FLUME IMAGE LIST 2.ILF

| Index | x/y offset | Sprite |
| --- | --- | --- |
| 0 | (-115, -126) | flumec1.lls |
| 1 | (0, 0) | flumec2.lls |
| 2 | (0, 0) | flumec3.lls |
| 3 | (0, 0) | flumec4.lls |

### LOG FLUME TRACK ZBUFFER IMAGE LIST.ILF

| Index | x/y offset | Sprite |
| --- | --- | --- |
| 0 | (-500, -173) | z_cornne.lls |
| 1 | (-94, -377) | z_cornes.lls |
| 2 | (-231, -309) | z_cornsw.lls |
| 3 | (-362, -244) | z_cornwn.lls |
| 4 | (-43, -217) | z_strns.lls |
| 5 | (-180, -149) | z_strew.lls |
| 6 | (-138, -264) | z_endn.lls |
| 7 | (-216, -230) | z_ende.lls |
| 8 | (-486, -96) | z_ends.lls |
| 9 | (-348, -164) | z_endw.lls |
| 10 | (-311, -84) | z_box.lls |

| Archive member | Offset | Bytes | SHA256 |
| --- | --- | --- | --- |
| Objdesc/LOG FLUME ENTRANCE.ODF | 0x48bee4 | 516 | `b2b5d6a2c6388c0e41008c7d18d1a1fb7af4f3d132906166f4037baec26f691b` |
| Objdesc/LOG FLUME TRACK.ODF | 0x489f20 | 448 | `fe1daa629870fd8c8d31931e19c209dc1f87cbd1df98dbe57769680603b5ed23` |
| Objdesc/LOG FLUME SPECIAL CORNER 1.ODF | 0x48b7ac | 492 | `8cc1f57a508850dc7c54c54a10a71e4ba02836d0429bba095b9d938b403f3c8b` |
| Objdesc/LOG FLUME SPECIAL CORNER 2.ODF | 0x488c4c | 447 | `837ae9d7638c1e3a7c41b609dc65eac664649e2cb14a9870909be94e72df8770` |
| Objdesc/LOG FLUME SPECIAL CORNER 3.ODF | 0x488a80 | 460 | `5f45fd9ea29832dd907a7e382a3064b0bb1f3ab0bdae6c76722a21f360c995fb` |
| Objdesc/LOG FLUME SPECIAL CORNER 4.ODF | 0x4888a0 | 477 | `80dd30c985c37bac3253f3341d8e62ecf4bc625bc9e68e5a493a97f10e6bd055` |
| Objdesc/LOG FLUME TUNNEL.ODF | 0x48b998 | 421 | `6d2b1f8df05b400c045a5cf1b5917494fde12990a7eb1cd2aa57d079e63bb355` |
| Objdesc/LOG FLUME CSAW.ODF | 0x48bb40 | 471 | `313b848ca114ee19b9a53f92124213fee9c77b3bd2a9cbdd1e0cf7e264a12c83` |
| Objdesc/LOG FLUME HOLD UP.ODF | 0x4886ec | 434 | `63101f6ba7764647cb7e22f43c1624270b5141e2b65f5606490a6458f9c8cd80` |
| Objdesc/LOG FLUME DROP.ODF | 0x488534 | 439 | `7d32a28c1428f2c4a706035f2b9f6b03eef7784ea9aa3d2be4431ae3e7a51cde` |
| Objdesc/CASTLE.ODF | 0x490dc8 | 483 | `b2abdab7062e9c774fcc81504ea5e53d57a839f131ab2941e85eb16f558aeca7` |
| ImageData/LOG FLUME IMAGE LIST.ILF | 0x454cf8 | 285 | `13fa9d26f6979a77271afb762d836afa711a920f57acb3d6c273fb23393bd554` |
| ImageData/LOG FLUME TRACK ENDY LIST.ILF | 0x453fe8 | 121 | `eb9dee182ff0961d4b33230b4ecea91c5d15347c5f705d7cb40c67d8885b5535` |
| ImageData/LOG FLUME IMAGE LIST 2.ILF | 0x454a30 | 122 | `f277f7d8f9bcb0b1a981e3f1d071eacdeb2fa74b64aa91ece8174bf1719b421f` |
| ImageData/LOG FLUME TRACK ZBUFFER IMAGE LIST.ILF | 0x454908 | 293 | `4a723d9c4ebadbc01ceaa191258ffe1d245deeb2fcdcd619dcf8b1dfc38be0b3` |

## Flume endpoint arithmetic

Original0x411305–30f calculates `1/(n−1)` and stores step as binary32. It divides float t by that stored value at0x411317, stores the quotient as binary64 at0x41131b and calls floor before integer conversion. The two control points are indexed before coordinate interpolation, with no clamp. For n=2,t=1 this gives segment1, reading forward points1/2 or reverse points0/−1: an out-of-range second read is certain even when its interpolation weight iszero. For n=4, nearest-rounded step is`0x3eaaaaab`≈0.3333333432674408. A wider division yields2.9999999105930355 and segment2; a24-bit-significand, round-to-nearest x87 division rounds that quotient to3 before the binary64 spill and selects segment3. Rounding mode also remains part of the original ambient x87 environment. The two cases must not be collapsed into either a universal safe-endpoint or universal out-of-range statement. [logflume7.c](../../LEGOLAND/logflume7.c), original0x4112f0–3c1.

Callers advance to the next piece only after z>1, not z≥1; no guard proves that equality is unreachable in normal play. The specified normal table sizes are2 and4. A conforming implementation must choose its numeric fidelity policy explicitly; this documentation records the original comparisons/rounding boundary and does not claim gameplay execution. [logflume6.c](../../LEGOLAND/logflume6.c), [logflume7.c](../../LEGOLAND/logflume7.c)

```python
import math, struct
f32=lambda x:struct.unpack("<f",struct.pack("<f",x))[0]
step=f32(1/3)
assert struct.unpack("<I",struct.pack("<f",step))[0]==0x3eaaaaab
wide=1.0/step
assert math.floor(wide)==2 and math.floor(f32(wide))==3
assert math.floor(1.0/f32(1.0))==1
print("PASS: two-point endpoint and four-point precision distinction")
```

## Static block manifest

This table specifies the exact extraction type/count and SHA256 of every block above. `h/i/I/f/B` mean signed16/signed32/unsigned32/binary32/unsigned8. Duplicate boat/Jungle hashes are checked independently at both addresses.

| Name | VA | Format | Bytes | SHA256 |
| --- | --- | --- | --- | --- |
| boat_step | 0x4b5118 | 16i | 64 | `7d97884b7fb853e64a0318ab480bc2087f4a54ee295fd70f95ef8784122ab958` |
| boat_cw | 0x4b5158 | 16f | 64 | `597cdc7ed15cd2863700fa1b1a51ed217162edd3d2e34ee0deccfcece751d0e0` |
| boat_ccw | 0x4b5198 | 16f | 64 | `54116b8cc1db5f433c76d882a2e39fae29aad885abbeba25a8d7f786ed327bb2` |
| boat_water | 0x4b53d4 | 400B | 400 | `a49cb80982e8f0aa938b6dea8f57eb87cf76fea1d8de130e4f3d4e11b1b42da3` |
| jungle_step | 0x4b7148 | 16i | 64 | `7d97884b7fb853e64a0318ab480bc2087f4a54ee295fd70f95ef8784122ab958` |
| jungle_cw | 0x4b7188 | 16f | 64 | `597cdc7ed15cd2863700fa1b1a51ed217162edd3d2e34ee0deccfcece751d0e0` |
| jungle_ccw | 0x4b71c8 | 16f | 64 | `54116b8cc1db5f433c76d882a2e39fae29aad885abbeba25a8d7f786ed327bb2` |
| jungle_water | 0x4b72e4 | 400B | 400 | `a49cb80982e8f0aa938b6dea8f57eb87cf76fea1d8de130e4f3d4e11b1b42da3` |
| seat_x | 0x4b559c | 3f | 12 | `ec59471fa91d47300ee1ceb1c4d6a5baa520636059cff91e1fd5e30faaee48a2` |
| seat_spacing | 0x4b55a8 | f | 4 | `140efb356462f70dd1c7f1dfb10bcc07d0f14d439043fb9e9d50f4d7be71ea96` |
| travel_scale | 0x4ab404 | f | 4 | `da6b03367dd2eb305461b255b3f7b3827143184bf4da43c5c5bcf0b1e690931d` |
| cursor_modes | 0x4b6408 | 3i | 12 | `be3e63ddb18e272dd8a8ba102772e6585e672d87230e0048635f47405926109f` |
| flume_footprint | 0x4b4728 | 5i | 20 | `5f99c8d22bf8b1db0be8128a0af2fbb40436682c0daf022c3da82fe5acb4383d` |
| flume_shapes | 0x4b473c | 11i | 44 | `5f001244746a24cdb14de2ef3238b498aebe81342d44dcb615175d4a64fa6d75` |
| flume_entry_a | 0x4b4798 | 8i | 32 | `e63ef0dff91a0585054f9af18667419b27ab47feaf97fc0fe6438446bb3be6aa` |
| flume_entry_a_ref | 0x4b47b8 | 2I | 8 | `2bc5ec44b3dd7a25a9ab8d9efecfeed8bfd1c9676557377d4b9bff5c4bf3acd8` |
| flume_entry_b | 0x4b47c0 | 10i | 40 | `2546caa54c11d742bdefc3ac54bbcd82f51a09b7df6d715adbc60cfe0f76f84e` |
| flume_entry_b_ref | 0x4b47e8 | 2I | 8 | `e50553535fad30af614a0e36dba5ffbcc9033640dcd7961c82dc08d8f63762a8` |
| flume_overlay_a | 0x4b47f0 | 6i | 24 | `5dc75f79ae1cb06acaf8e1922ba2fb74e96bca31a25445ca34ab835d33883e34` |
| flume_overlay_a_ref | 0x4b4808 | 2I | 8 | `e103726dde5d7dac3480854427ad68c6aece0e232212a8f59946210c7ab0a7e1` |
| flume_overlay_c | 0x4b4810 | 6i | 24 | `5dc75f79ae1cb06acaf8e1922ba2fb74e96bca31a25445ca34ab835d33883e34` |
| flume_overlay_c_ref | 0x4b4828 | 2I | 8 | `30abbd7ece43b76986a1a03879c19239061e4ea85b2d866f29a8902fc7509e8c` |
| flume_overlay_hold | 0x4b4830 | 9i | 36 | `b12f43e74b758c9c5d7f19bfa8382c059c33b7892eaa60247f183c7783a67ccc` |
| flume_overlay_hold_ref | 0x4b4858 | 2I | 8 | `33fccd0e7416c43f0b9d23b8c587398f2e7a0400041121647004ab79193079f4` |
| desc_castle | 0x4b5b48 | 13I | 52 | `4a48a6758199a67a52b7cfb25e1e282a4b9524e94ea306af0fe1feb73d8780be` |
| desc_flat | 0x4b5d20 | 13I | 52 | `50a24c3971ddf2089b473b61f9b6e7bd2e6bb1bb65c223d22752d862a717ba3c` |
| desc_high | 0x4b5d58 | 13I | 52 | `cdc339b4d918ef603012d402d819cbf53be4b9e20ec80790ee75af36cb3a7ece` |
| desc_low | 0x4b5d90 | 13I | 52 | `669b29b18347e9ba25c02c4503d0a0abf453a47d11af36cf1e3956d0450f9c42` |
| desc_path | 0x4b5dc8 | 13I | 52 | `68a63c166b8ae9e46a7a47a7843894f38c49389cef1465528f57b02affca6bd9` |
| edge_mid | 0x4b5e00 | 12f | 48 | `74d0fbdc8ca100697989054557c536dac08fd2a569fa1fa027a82bdb5aea46cf` |
| half_step | 0x4b5e30 | 12f | 48 | `f14f9b56937c66cceec048c12d99ea88f7a2b693453995b8ffdc4f132521159b` |
| heading | 0x4b5e60 | 12f | 48 | `27cdbfb759eb506ecddfaeb8e61446e2fb6b72bcaef47a216c1f5b23de4c4b56` |
| corner | 0x4b5e90 | 12f | 48 | `a5c36d9c4e47cddc6a615bb58f9b8fbafc2fd89739c50b0207112d7aaabf2442` |
| straight_dir | 0x4b5ec0 | 4i | 16 | `3c52e07ea6f9c688f7921e6114ac155e13c5922f6fe7dd46e242c18e42262a1e` |
| straight_offset | 0x4b5ee0 | 5i | 20 | `3229b6b66fd6e55d89a1c9af690b3768a4f656241f3db2e874b61d33d0c7570f` |
| corner_dirs | 0x4b5ef4 | 8i | 32 | `69d69f97d95b9b27b4a055c25e338c3ea91a3bfc3a3203e68bc9495714ab7544` |
| corner_radii | 0x4b5f14 | 8f | 32 | `885f5b37327009cd11fdad10d32ffa29c7393f76e173dfc67ed1ebb1ac44c97f` |
| shadow_vertices | 0x4b5f80 | 36f | 144 | `2382b5e2f20b7c71aa1a219d532fd3ee946844eafd949885e61a5ced06c4b28e` |
| shadow_faces | 0x4b6010 | 128h | 256 | `b149cfade221e59ae480ec76b99d75b0d05bc433bfc069ff93c90149629710dd` |
| shadow_normals | 0x4b6110 | 15f | 60 | `26e28b20817b8c0579ba0e03078def8883e59a7ad14f22f49512293dcd9a60df` |
| shadow_header | 0x4b6150 | 12I | 48 | `7a5724ba8663064f2d9ca6830feebcf6901b214009b722922963c177c355a112` |
| support_vertices | 0x4b61e0 | 24f | 96 | `889599cf3c94cb8aa5b688c0b57cf49c8a3afce17a596a9caf78d4150a14bdb3` |
| support_normals | 0x4b6240 | 12f | 48 | `7a1e12ffee95b278d2cc9f298583d2bec8d18a0518950e4c8f56331e64ca99ad` |
| support_faces | 0x4b6270 | 64h | 128 | `b0a42bdbd90d908093c5389ba3e9be762fdae210eb4899973e922788a22760dd` |
| support_material | 0x4b62f0 | 3I | 12 | `ca888f40c3caca805b37a5434c75de5550616e0795e7602fb91156f22dd90851` |
| support_header | 0x4b6300 | 12I | 48 | `702015a1764807fcc21cb9281dd69fc6513519ac4ee6d0e2a7cb8a40c0df7833` |
| node_potential_scale | 0x4ab434 | f | 4 | `66446397825e27bc81df9e9b3aadb2408e6c6ffc6a56b48412d5d05f84399b01` |

## Reproducible read-only extraction

Run this Python3 snippet with `EXE` pointing at the original executable. It reads bytes only, validates the preferred image base and SHA256, rejects non-file-backed VAs, validates all block hashes, and prints all exact decoded values. It does not use reconstructed object files or live process memory. The optional instruction loop requires `capstone`; it prints the bounded consumers used as evidence above.

```python
from pathlib import Path
import hashlib, struct
EXE = Path("../legoland/original/legoland.exe")  # from the Scope J worktree
blob = EXE.read_bytes()
assert hashlib.sha256(blob).hexdigest() == "c50865b60bfcb26c0a7329a75fb772b10ae234af324f669901906e5abb0e2bd9"
pe = struct.unpack_from("<I", blob, 0x3c)[0]
assert blob[pe:pe+4] == b"PE\0\0"
base = struct.unpack_from("<I", blob, pe+24+28)[0]
assert base == 0x400000
count = struct.unpack_from("<H", blob, pe+6)[0]
opt = struct.unpack_from("<H", blob, pe+20)[0]
sections = []
for i in range(count):
    p = pe + 24 + opt + 40*i
    virtual_size, rva, raw_size, raw_offset = struct.unpack_from("<4I", blob, p+8)
    sections.append((base+rva, raw_size, raw_offset))
def at(va, length):
    for start, size, raw in sections:
        if start <= va and va+length <= start+size:
            out = blob[raw+va-start:raw+va-start+length]
            assert len(out) == length
            return out
    raise ValueError(f"Unbacked VA {va:#x}, length {length:#x}")
blocks = [
    ('node_potential_scale', 0x4ab434, 'f', '66446397825e27bc81df9e9b3aadb2408e6c6ffc6a56b48412d5d05f84399b01'),
    ('boat_step', 0x4b5118, '16i', '7d97884b7fb853e64a0318ab480bc2087f4a54ee295fd70f95ef8784122ab958'),
    ('boat_cw', 0x4b5158, '16f', '597cdc7ed15cd2863700fa1b1a51ed217162edd3d2e34ee0deccfcece751d0e0'),
    ('boat_ccw', 0x4b5198, '16f', '54116b8cc1db5f433c76d882a2e39fae29aad885abbeba25a8d7f786ed327bb2'),
    ('boat_water', 0x4b53d4, '400B', 'a49cb80982e8f0aa938b6dea8f57eb87cf76fea1d8de130e4f3d4e11b1b42da3'),
    ('jungle_step', 0x4b7148, '16i', '7d97884b7fb853e64a0318ab480bc2087f4a54ee295fd70f95ef8784122ab958'),
    ('jungle_cw', 0x4b7188, '16f', '597cdc7ed15cd2863700fa1b1a51ed217162edd3d2e34ee0deccfcece751d0e0'),
    ('jungle_ccw', 0x4b71c8, '16f', '54116b8cc1db5f433c76d882a2e39fae29aad885abbeba25a8d7f786ed327bb2'),
    ('jungle_water', 0x4b72e4, '400B', 'a49cb80982e8f0aa938b6dea8f57eb87cf76fea1d8de130e4f3d4e11b1b42da3'),
    ('seat_x', 0x4b559c, '3f', 'ec59471fa91d47300ee1ceb1c4d6a5baa520636059cff91e1fd5e30faaee48a2'),
    ('seat_spacing', 0x4b55a8, 'f', '140efb356462f70dd1c7f1dfb10bcc07d0f14d439043fb9e9d50f4d7be71ea96'),
    ('travel_scale', 0x4ab404, 'f', 'da6b03367dd2eb305461b255b3f7b3827143184bf4da43c5c5bcf0b1e690931d'),
    ('cursor_modes', 0x4b6408, '3i', 'be3e63ddb18e272dd8a8ba102772e6585e672d87230e0048635f47405926109f'),
    ('flume_footprint', 0x4b4728, '5i', '5f99c8d22bf8b1db0be8128a0af2fbb40436682c0daf022c3da82fe5acb4383d'),
    ('flume_shapes', 0x4b473c, '11i', '5f001244746a24cdb14de2ef3238b498aebe81342d44dcb615175d4a64fa6d75'),
    ('flume_entry_a', 0x4b4798, '8i', 'e63ef0dff91a0585054f9af18667419b27ab47feaf97fc0fe6438446bb3be6aa'),
    ('flume_entry_a_ref', 0x4b47b8, '2I', '2bc5ec44b3dd7a25a9ab8d9efecfeed8bfd1c9676557377d4b9bff5c4bf3acd8'),
    ('flume_entry_b', 0x4b47c0, '10i', '2546caa54c11d742bdefc3ac54bbcd82f51a09b7df6d715adbc60cfe0f76f84e'),
    ('flume_entry_b_ref', 0x4b47e8, '2I', 'e50553535fad30af614a0e36dba5ffbcc9033640dcd7961c82dc08d8f63762a8'),
    ('flume_overlay_a', 0x4b47f0, '6i', '5dc75f79ae1cb06acaf8e1922ba2fb74e96bca31a25445ca34ab835d33883e34'),
    ('flume_overlay_a_ref', 0x4b4808, '2I', 'e103726dde5d7dac3480854427ad68c6aece0e232212a8f59946210c7ab0a7e1'),
    ('flume_overlay_c', 0x4b4810, '6i', '5dc75f79ae1cb06acaf8e1922ba2fb74e96bca31a25445ca34ab835d33883e34'),
    ('flume_overlay_c_ref', 0x4b4828, '2I', '30abbd7ece43b76986a1a03879c19239061e4ea85b2d866f29a8902fc7509e8c'),
    ('flume_overlay_hold', 0x4b4830, '9i', 'b12f43e74b758c9c5d7f19bfa8382c059c33b7892eaa60247f183c7783a67ccc'),
    ('flume_overlay_hold_ref', 0x4b4858, '2I', '33fccd0e7416c43f0b9d23b8c587398f2e7a0400041121647004ab79193079f4'),
    ('desc_castle', 0x4b5b48, '13I', '4a48a6758199a67a52b7cfb25e1e282a4b9524e94ea306af0fe1feb73d8780be'),
    ('desc_flat', 0x4b5d20, '13I', '50a24c3971ddf2089b473b61f9b6e7bd2e6bb1bb65c223d22752d862a717ba3c'),
    ('desc_high', 0x4b5d58, '13I', 'cdc339b4d918ef603012d402d819cbf53be4b9e20ec80790ee75af36cb3a7ece'),
    ('desc_low', 0x4b5d90, '13I', '669b29b18347e9ba25c02c4503d0a0abf453a47d11af36cf1e3956d0450f9c42'),
    ('desc_path', 0x4b5dc8, '13I', '68a63c166b8ae9e46a7a47a7843894f38c49389cef1465528f57b02affca6bd9'),
    ('edge_mid', 0x4b5e00, '12f', '74d0fbdc8ca100697989054557c536dac08fd2a569fa1fa027a82bdb5aea46cf'),
    ('half_step', 0x4b5e30, '12f', 'f14f9b56937c66cceec048c12d99ea88f7a2b693453995b8ffdc4f132521159b'),
    ('heading', 0x4b5e60, '12f', '27cdbfb759eb506ecddfaeb8e61446e2fb6b72bcaef47a216c1f5b23de4c4b56'),
    ('corner', 0x4b5e90, '12f', 'a5c36d9c4e47cddc6a615bb58f9b8fbafc2fd89739c50b0207112d7aaabf2442'),
    ('straight_dir', 0x4b5ec0, '4i', '3c52e07ea6f9c688f7921e6114ac155e13c5922f6fe7dd46e242c18e42262a1e'),
    ('straight_offset', 0x4b5ee0, '5i', '3229b6b66fd6e55d89a1c9af690b3768a4f656241f3db2e874b61d33d0c7570f'),
    ('corner_dirs', 0x4b5ef4, '8i', '69d69f97d95b9b27b4a055c25e338c3ea91a3bfc3a3203e68bc9495714ab7544'),
    ('corner_radii', 0x4b5f14, '8f', '885f5b37327009cd11fdad10d32ffa29c7393f76e173dfc67ed1ebb1ac44c97f'),
    ('shadow_vertices', 0x4b5f80, '36f', '2382b5e2f20b7c71aa1a219d532fd3ee946844eafd949885e61a5ced06c4b28e'),
    ('shadow_faces', 0x4b6010, '128h', 'b149cfade221e59ae480ec76b99d75b0d05bc433bfc069ff93c90149629710dd'),
    ('shadow_normals', 0x4b6110, '15f', '26e28b20817b8c0579ba0e03078def8883e59a7ad14f22f49512293dcd9a60df'),
    ('shadow_header', 0x4b6150, '12I', '7a5724ba8663064f2d9ca6830feebcf6901b214009b722922963c177c355a112'),
    ('support_vertices', 0x4b61e0, '24f', '889599cf3c94cb8aa5b688c0b57cf49c8a3afce17a596a9caf78d4150a14bdb3'),
    ('support_normals', 0x4b6240, '12f', '7a1e12ffee95b278d2cc9f298583d2bec8d18a0518950e4c8f56331e64ca99ad'),
    ('support_faces', 0x4b6270, '64h', 'b0a42bdbd90d908093c5389ba3e9be762fdae210eb4899973e922788a22760dd'),
    ('support_material', 0x4b62f0, '3I', 'ca888f40c3caca805b37a5434c75de5550616e0795e7602fb91156f22dd90851'),
    ('support_header', 0x4b6300, '12I', '702015a1764807fcc21cb9281dd69fc6513519ac4ee6d0e2a7cb8a40c0df7833'),
 ]
for name, va, fmt, digest in blocks:
    raw = at(va, struct.calcsize("<"+fmt))
    assert hashlib.sha256(raw).hexdigest() == digest, name
    print(name, hex(va), struct.unpack("<"+fmt, raw))
for left, right, length in [(0x4b5118,0x4b7148,64), (0x4b5158,0x4b7188,64),
                            (0x4b5198,0x4b71c8,64), (0x4b53d4,0x4b72e4,400)]:
    assert at(left,length) == at(right,length)
print("PASS: all static blocks and duplicate tables")
```

The following optional block uses the same `at` function and produces the original instruction evidence used above. Bounds contain the observed returns or explicitly cited constructor prefixes; the function-byte SHA comes from the hash-pinned executable. A VA list is intentional: it does not follow arbitrary embedded data as code.

```python
import capstone
md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
for lo, hi in [
    (0x41dca0,0x41dcf2), (0x41e630,0x41e66b), (0x41e670,0x41e692), (0x41eaf0,0x41eb27),
    (0x4222f0,0x4222ff), (0x422300,0x422335), (0x422340,0x42238a), (0x422400,0x42246b),
    (0x4266b0,0x4266d3), (0x4266e0,0x4266f7),
    (0x4112f0,0x4113c2), (0x429a80,0x429abd), (0x429b90,0x429bac), (0x429bb0,0x429c10),
    (0x40b210,0x40b263), (0x40b290,0x40b38a), (0x4120e0,0x4120f6), (0x412100,0x412284),
    (0x41cff0,0x41d035), (0x41e820,0x41e8ee),
    (0x41e950,0x41e961), (0x41e970,0x41e981),
    (0x41e990,0x41e9d7), (0x41e9e0,0x41ea70), (0x41ea70,0x41eae8),
    (0x420810,0x420a13), (0x420a20,0x420c32), (0x420c40,0x420e83),
    (0x420e90,0x420fae), (0x420fb0,0x420fd0), (0x423080,0x42313c),
    (0x426750,0x4267a1), (0x4273d0,0x4274f0),
    (0x429910,0x42993e), (0x429940,0x429987), (0x429990,0x4299d7),
    (0x42a5e0,0x42a612), (0x42a620,0x42a63d),
    (0x42a640,0x42a66a), (0x42a670,0x42a677), (0x42a680,0x42a77f),
    (0x433d20,0x433d8e), (0x434100,0x434193), (0x434860,0x4348ce),
]:
    raw = at(lo,hi-lo)
    print(hex(lo),len(raw),hashlib.sha256(raw).hexdigest())
    insns = list(md.disasm(raw,lo))
    assert insns and insns[-1].address + insns[-1].size == hi
    for insn in insns:
        print(f"{insn.address:08x} {insn.mnemonic:8} {insn.op_str}")
```

Run the next read-only block from the worktree after the static extractor. The model/index validation is independent of Capstone. It prints all model vectors, triangles, material bytes and image bytes, so no binary-derived payload depends on a private extraction file.

```python
MAIN = Path("../legoland/gamedata/main")
assets = [
    ('ROLLERCOASTER.obj', 116, '687321206c62577c2a9a9c0761dec0f12ff18226ed329e23ec2d790f37f95e8d'),
    ('ROLLERCOASTER.txt', 107, '50d19a5e903f84a5e8debc6e032379a845a7ec4137bd1bcf2005d8ae4d8c7acc'),
    ('ROLLERCOASTER0000.ltx', 1032, '8916986d81bb082e9a871aedfe3bc7c466e41f2743407214ab60b79b882718bb'),
    ('ROLLERCOASTER0001.ltx', 1032, 'c41051e86b085a88dce2388c1d8c1456a322e29e746abbb3da9bd337eca13db8'),
    ('ROLLERCOASTER0002.ltx', 1032, '4b913075ae0c8e43a356d4c96b2b552fec7f1256a408d75a4529c9301db8b505'),
    ('ROLLERCOASTER0003.ltx', 1032, 'c61f613ffab9642ddd5bd502ea4d5bbd74129bcd081ea4624df0f93013eb3c44'),
    ('ROLLERCOASTER0004.ltx', 1032, '70d8571ee4ef78fb468987a6bdeb6a58b6b7ba85f38dfab130be288fcd75be95'),
    ('ROLLERCOASTER0005.ltx', 1032, '9034f62ba9a335973e690c8aa94e1a611e31a03f3bdada1039825f7fd32fa788'),
    ('ROLLERCOASTER0006.ltx', 1032, 'a2c793a5896682c63f86d438c6c14e1836308482d601c7beea8aa607c23eac1a'),
    ('ROLLERCOASTER0007.ltx', 1032, 'e89fae8658d193e3da9bb10363116aaa838a7b553484122281357a4ea4a2804f'),
    ('ROLLERCOASTER0008.ltx', 1032, '47b75f3912ea24b1c67ac9cebb21110f6d53521dde45b48376aa45d442c93532'),
    ('ROLLERCOASTER0009.ltx', 1032, '2484e43bdb57764aa9906f509850832cd2a4d9d61e79925a67a96af62d050433'),
    ('Rollercoaster.lpt', 584, 'bf27bee9e631dca0911a3f847b09c0d6f8328ef7e5927e272f1fccc920da78d1'),
    ('coaster.Line01.lfm', 12, 'de66807afa64bbe18c1e9ca78633c026dcee7be300f3dffc7353e002c9b8c2d2'),
    ('coaster.Line01.lms', 12420, '66659c520ae86b863d0286bbdfec8d8b9be8d5c777771156800a54b322838e9c'),
    ('coaster.wheel01.lfm', 288, 'a416df9d09ade5153d29d27743b47da21c52222e0002a48c1abb5b81b740a90e'),
    ('coaster.wheel01.lms', 1200, 'c6ea6b9c9409dc3dc571fc9f19b1a26b036388e482d08c8549cb3bedc0acf073'),
    ('coastertrain.headcar.lfm', 12, '86fbb29c2651dcbb804c8d4ac1f3bed87221427976fecd213cd280b71bc8522b'),
    ('coastertrain.headcar.lms', 10548, '93975e75c2de773ee13501af8b519050a87cd9d7ec7be3385963aa2d9133de26'),
    ('coastertrain.midcar.lfm', 12, '86fbb29c2651dcbb804c8d4ac1f3bed87221427976fecd213cd280b71bc8522b'),
    ('coastertrain.midcar.lms', 3872, 'c2a4ebf9e317481acd6744b02fbd552511000fb4e42a3f8ab28682ed3908b2aa'),
    ('coastertrain.tailcar.lfm', 12, '86fbb29c2651dcbb804c8d4ac1f3bed87221427976fecd213cd280b71bc8522b'),
    ('coastertrain.tailcar.lms', 5840, '543944e79d16c3643bf25bfc99cbbabacb08657c6a6f27c3d0cb8956f8ad68e3'),
    ('coastertrain.wheel01.lfm', 288, 'ed029615b5dd41f76aff5e0cd94c61ae114331a6f3d855ea5a7b53a6fc75216b'),
    ('coastertrain.wheel01.lms', 1296, 'd33b669f6e439ff5763ceef2962e5f97b44f21cff2928193b230eba3d4d1b8e6'),
    ('damo.Box01.lfm', 144, 'e22dcc32efcbbe7f50c3e71d77326976953b690d3e5495cc714dcdb16bff882f'),
    ('damo.Box01.lms', 768, 'db120c40b18761d7c6a09c90bbb6fe9013cb158d36f72c5c70e15e92f9834ac4'),
    ('sit.logirlsit.lfm', 180, '1e2874c7144e3c8abfc446aac6d719c412633a6d1e4aa3a92e7e782ba0c81ef4'),
    ('sit.logirlsit.lms', 6904, '0318d1c44f9e09e174799898c2e179e684fcb8a73ff5f4febee871c9673e225a'),
    ('sit.lomansit.lfm', 192, '4241427940d3231267abfd7e0e28837fc947904198f390e3b544e38f4efcae6c'),
    ('sit.lomansit.lms', 5680, '7a4ebf17ad21d1a16130d82c3415b8ad28416dade238c01bdcbc5a43ae79c4bd'),
    ('sitgirl.logirl.lfm', 180, '3be0b23721a1086c9d9821456440b01a0901353093a5ac79ff4cbef081cad128'),
    ('sitgirl.logirl.lms', 6808, 'e287107fba4931eb58e72aa93056f65b1ea69074357bba7b2e48a4610fa0f89d'),
 ]
for name, size, digest in assets:
    raw = (MAIN/name).read_bytes()
    assert len(raw)==size and hashlib.sha256(raw).hexdigest()==digest, name
palette = (MAIN/"Rollercoaster.lpt").read_bytes()
ncolors = struct.unpack_from("<I",palette)[0]
assert len(palette)==4+4*ncolors and ncolors==145
for f in sorted(MAIN.glob("*.lms")):
    raw=f.read_bytes(); h=struct.unpack_from("<12I",raw)
    nv, ns, nf, vp, fnp, snp, fp, nflat, sp, nsmooth, tp, ntex=h
    assert nf==nflat+nsmooth+ntex
    assert vp in (0x30,0x90)
    assert snp==vp+12*nv and fnp==snp+12*ns and fp==fnp+12*nf
    assert sp==fp+16*nflat and tp==sp+16*nsmooth and len(raw)==tp+16*ntex
    mats=(MAIN/(f.stem+".lfm")).read_bytes(); assert len(mats)%12==0
    print(f.name,"header",h,"materials",mats.hex())
    for label,start,count in [("positions",vp,nv),("smooth normals",snp,ns),("face normals",fnp,nf)]:
        print(label,[struct.unpack_from("<3f",raw,start+12*i) for i in range(count)])
    for label,start,count in [("flat",fp,nflat),("smooth",sp,nsmooth),("texture",tp,ntex)]:
        rows=[struct.unpack_from("<8h",raw,start+16*i) for i in range(count)]
        print(label,rows)
        for row in rows:
            assert 0<=row[0]<len(mats)//12
            assert all(0<=v<nv for v in row[2:5])
            if label=="smooth": assert all(0<=v<ns for v in row[5:8])
            else: assert 0<=row[1]<nf
            material=struct.unpack_from("<I",mats,12*row[0])[0]
            assert material < (10 if label=="texture" else ncolors)
for f in sorted(MAIN.glob("*.ltx")):
    raw=f.read_bytes(); width,height=struct.unpack_from("<2I",raw)
    assert (width,height)==(32,32) and len(raw)==8+width*height
    assert max(raw[8:])<ncolors
    print(f.name,width,height,raw[8:].hex())
print("PASS: 10 LMS/LFM pairs, 10 textures, palette and name lists")

RES=Path("../legoland/gamedata/disc/Legoland.res")
archive=RES.read_bytes()
assert hashlib.sha256(archive).hexdigest()=="b8cd7ee4a98c7da31e0f8aeb7495717a8ea320a73aa98a515b62ab13055627e2"
members=[
    ('Objdesc/LOG FLUME ENTRANCE.ODF', 0x48bee4, 516, 'b2b5d6a2c6388c0e41008c7d18d1a1fb7af4f3d132906166f4037baec26f691b'),
    ('Objdesc/LOG FLUME TRACK.ODF', 0x489f20, 448, 'fe1daa629870fd8c8d31931e19c209dc1f87cbd1df98dbe57769680603b5ed23'),
    ('Objdesc/LOG FLUME SPECIAL CORNER 1.ODF', 0x48b7ac, 492, '8cc1f57a508850dc7c54c54a10a71e4ba02836d0429bba095b9d938b403f3c8b'),
    ('Objdesc/LOG FLUME SPECIAL CORNER 2.ODF', 0x488c4c, 447, '837ae9d7638c1e3a7c41b609dc65eac664649e2cb14a9870909be94e72df8770'),
    ('Objdesc/LOG FLUME SPECIAL CORNER 3.ODF', 0x488a80, 460, '5f45fd9ea29832dd907a7e382a3064b0bb1f3ab0bdae6c76722a21f360c995fb'),
    ('Objdesc/LOG FLUME SPECIAL CORNER 4.ODF', 0x4888a0, 477, '80dd30c985c37bac3253f3341d8e62ecf4bc625bc9e68e5a493a97f10e6bd055'),
    ('Objdesc/LOG FLUME TUNNEL.ODF', 0x48b998, 421, '6d2b1f8df05b400c045a5cf1b5917494fde12990a7eb1cd2aa57d079e63bb355'),
    ('Objdesc/LOG FLUME CSAW.ODF', 0x48bb40, 471, '313b848ca114ee19b9a53f92124213fee9c77b3bd2a9cbdd1e0cf7e264a12c83'),
    ('Objdesc/LOG FLUME HOLD UP.ODF', 0x4886ec, 434, '63101f6ba7764647cb7e22f43c1624270b5141e2b65f5606490a6458f9c8cd80'),
    ('Objdesc/LOG FLUME DROP.ODF', 0x488534, 439, '7d32a28c1428f2c4a706035f2b9f6b03eef7784ea9aa3d2be4431ae3e7a51cde'),
    ('Objdesc/CASTLE.ODF', 0x490dc8, 483, 'b2abdab7062e9c774fcc81504ea5e53d57a839f131ab2941e85eb16f558aeca7'),
    ('ImageData/LOG FLUME IMAGE LIST.ILF', 0x454cf8, 285, '13fa9d26f6979a77271afb762d836afa711a920f57acb3d6c273fb23393bd554'),
    ('ImageData/LOG FLUME TRACK ENDY LIST.ILF', 0x453fe8, 121, 'eb9dee182ff0961d4b33230b4ecea91c5d15347c5f705d7cb40c67d8885b5535'),
    ('ImageData/LOG FLUME IMAGE LIST 2.ILF', 0x454a30, 122, 'f277f7d8f9bcb0b1a981e3f1d071eacdeb2fa74b64aa91ece8174bf1719b421f'),
    ('ImageData/LOG FLUME TRACK ZBUFFER IMAGE LIST.ILF', 0x454908, 293, '4a723d9c4ebadbc01ceaa191258ffe1d245deeb2fcdcd619dcf8b1dfc38be0b3'),
 ]
for name,offset,size,digest in members:
    raw=archive[offset:offset+size]
    assert len(raw)==size and hashlib.sha256(raw).hexdigest()==digest,name
    if name.endswith(".ODF"):
        assert struct.unpack_from("<I",raw)[0]==0x78
        print(name,"footprint",struct.unpack_from("<4i",raw,0x3c))
    else:
        count,kind,length=struct.unpack_from("<HHI",raw)
        assert kind==2
        p=8+length; offsets=struct.unpack_from("<"+str(count*2)+"i",raw,p);p+=8*count
        for i in range(count):
            length=struct.unpack_from("<I",raw,p)[0];p+=4
            text=raw[p:p+length].decode("latin1");p+=length
            print(name,i,offsets[2*i:2*i+2],text.rstrip("\0"))
        assert p==len(raw)
print("PASS: 11 ODF footprints and 4 flume image lists")
```
