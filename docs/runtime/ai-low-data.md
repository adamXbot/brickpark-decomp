# Low-level visitor AI: original instruction evidence

This page closes the16 low-level state handlers declared only through the dispatch table, plus11 missing immediate helper bodies. A twelfth helper, `DoRndWalkPathTileAction`, already exists in `bnvmove.c` and was independently byte-checked for closure of the source wrapper. The28 bounded bodies contain1,800 x86 instructions/4,668 bytes. They were read from the original executable, never executed or compiled; current C implementations are cited only where their bodies already supply the called contract. [original executable](../../../legoland/original/legoland.exe), [blokeai.c](../../LEGOLAND/blokeai.c), [bnvmove.c](../../LEGOLAND/bnvmove.c), [core dispatch evidence](core-data.md#low-level-ai-dispatch)

The input is802,857 bytes with SHA-256 `c50865b60bfcb26c0a7329a75fb772b10ae234af324f669901906e5abb0e2bd9`. The extractor below accepts only this file and reads only raw-backed PE sections. Function ends are exclusive and exclude following alignment padding. All28 extents decode contiguously; every instruction is reachable from the entry by following fall-through and direct branches, every direct jump stays at a valid instruction start within that body, and each terminal path returns. Dynamic calls are the named tile callbacks and restored-state dispatch described below. [original executable](../../../legoland/original/legoland.exe)

## Dispatch and record fields

Dispatch indexes the16-pointer table at `0x004bd34c` with unsigned state word+0x0e, without bounds checking. `pending` means word+0x10, a single saved low state. `DoPendingAction@0x00483240` copies pending to state, clears pending and returns the restored word; it does not change positions. The high-level dispatcher later clears event byte+0x64 and forces walk-delay byte+0x75 to1. [original executable](../../../legoland/original/legoland.exe), [core-data.md](core-data.md#low-level-ai-dispatch), [sweep3.c](../../LEGOLAND/sweep3.c), [blokeai.c](../../LEGOLAND/blokeai.c)

| Field | Used low-level meaning |
| --- | --- |
| `+04 pointer` | Person3D; kind at Person+8 chooses ordinary versus worker obstacle rules. |
| `+0c/+0e/+10 u16` | High plan / low state / saved low state. |
| `+20 i32` | Special-tile wait tick counter; reset on entry, incremented while waiting. |
| `+24/+28 i32` | Fixed-point line target. |
| `+3a u16; +44/+46 u16; +70 u16` | State8 repeat counter, initial/current vertical velocity, height; arithmetic wraps to16 bits. |
| `+54 u32` | Timestamp sampled by state14 against global0x008119a4, unsigned difference. |
| `+5c u32` | State4/14 test low seven bits; if zero they clear the full DWORD and low state. |
| `+62 u16` | Bit2 (mask0x2) on-path; bit4 per-tile action fired; bit8 special-tile/held activity membership. |
| `+64 u8` | Event bits delivered to the next high-level plan:1 obstacle,2 invalid/nonwalkable path result,4 path decision. |
| `+68/+6c i32` | Current world position in24.8 units. |
| `+72/+73/+74/+75 u8` | Current facing, requested facing, animation selector, turn/wait delay. |
| `+7f u8; +98 MoveLine` | Walking speed; higher-precision line accumulator. |

These offsets are consumer-specific views of the shared172-byte Bloke. The numeric bit names above state what these handlers test or write; they do not rename the same storage when attraction scripts reuse it. [original executable](../../../legoland/original/legoland.exe), [tilehelp.c](../../LEGOLAND/tilehelp.c), [workers.c](../../LEGOLAND/workers.c), [attractions.md](attractions.md#shared-ride-model-and-visitor-experience)

## Shared predicates and movement order

Let `RF(p)=GetCurrentRFFlags(p)` and `CF(p)=Get_MapFlags(p)`. Define `inside(p)` by `0≤x<width*256` and `0≤y<height*256`, and `walkable(p)=inside(p) && ((RF(p)&1) || ((CF(p)&0x10) && !(RF(p)&2)))`. `GetCurrentRFFlags` resolves dynamic resource flags; raw `Get_RFFlags` does not. `OverNewTile` compares bits above bit7, `RndWalk_LeftTile` also clears Bloke flag4 on a crossing, and `CrossTileCentre` detects bit7 crossing on its facing-selected axis while remaining in the tile. These distinctions control when obstruction checks and tile callbacks fire. [map.c](../../LEGOLAND/map.c), [pathmisc2.c](../../LEGOLAND/pathmisc2.c), [tilehelp.c](../../LEGOLAND/tilehelp.c)

A heading step computes proposed position from `DirectionDelta`, then may intercept a special-tile entry before its other tests. A line step tests arrival **before** calling `NavigMoveLine`; if not arrived, that call advances the internal accumulator before obstacle/tile checks. Rejected proposals leave world coordinates unchanged but do not roll back the already-advanced accumulator. Committing calls tile enter/leave bookkeeping, stores proposed world x/y and calls `Bloke_MoveAnim@0x00483830`; state7 omits tile bookkeeping entirely. No line-arrival handler snaps coordinates. [original executable](../../../legoland/original/legoland.exe), [bnvmove.c](../../LEGOLAND/bnvmove.c), [bnvpath.c](../../LEGOLAND/bnvpath.c), [core-data.md](core-data.md#movement-arrival-and-coordinate-units)

`DirectionDelta@0x004831a0` masks the direction to an unsigned byte, then uses it as an **unchecked** index into eight signed16 pairs at `0x004bd32c`. It sign-extends the speed argument from16 bits, multiplies each component and arithmetic-shifts right8. Table SHA-256 is `e6c650614328323da7c29a4d3aa1130661edfc7e7af0998e5b5dfbe95e5dc60d` for32 bytes. [original executable](../../../legoland/original/legoland.exe)

| Facing | dx coefficient | dy coefficient |
| --- | --- | --- |
| 0 | -181 | -181 |
| 1 | 0 | -256 |
| 2 | 181 | -181 |
| 3 | 256 | 0 |
| 4 | 181 | 181 |
| 5 | 0 | 256 |
| 6 | -181 | 181 |
| 7 | -256 | 0 |

`ArrivalRadius@0x004841a0` computes signed32-bit `(world_x-target_x)^2+(world_y-target_y)^2 <= radius^2`; products/sum retain x86 low32 bits. Callers use radius `2*speed`. Extremely large differences can therefore overflow the signed test. `StrictInterior@0x00483160` separately requires x>0 and y>0, excluding the outer zero-coordinate axes, unlike the usual inside predicate. [original executable](../../../legoland/original/legoland.exe), [core-data.md](core-data.md#movement-arrival-and-coordinate-units)

## All sixteen state handlers

| State / VA | Complete transition and side-effect contract |
| --- | --- |
| 0 / `0x004838a0` | Passes format `Frame %d.. Bloke %d rethinking\n`, global0x008119a4 and Bloke pointer to DBPrintf. Its source body is a no-op, so there is no AI mutation. |
| 1 / `0x004838c0` | Pre-decrements byte+75. Only a resulting zero sets it back to1 and restores pending; initial0 wraps255 and does not complete immediately. |
| 2 / `0x00483ef0` | Heading step; special-tile interception can return early. Calls LeftTile then CrossTileCentre. On centre crossing marks flag4, samples current RF, and if current is not walkable AND its CF0x10 is clear, sets state0/event2. Otherwise RF0x24 sets state0/event4. RF8 selects the first available outgoing direction from the proposed tile after excluding reverse, requests a turn, then still commits the proposed movement. Other paths commit directly. |
| 3 / `0x00484090` | Heading step with LeftTile/centre processing, but no special-entry interception. At centre marks flag4; nonwalkable current→state0/event2, RF0x24→state0/event4, RF8→state0 with no event; those return before movement. Otherwise commits. |
| 4 / `0x00483d10` | Heading proposal; interception returns early. Runs random-walk tile-specific wrapper, then avoidance if that returns0; commits only if both return0. Afterwards, except interception return, low seven bits of +5c being0 clear both state and full +5c. The state does not itself decrement +5c. |
| 5 / `0x004838e0` | If facing differs from requested facing, calls the gated one-octant turn helper. Once equal, forces delay1 and restores pending, including when already equal on entry. |
| 6 / `0x00484220` | Arrival→state0. Otherwise advances line. Only on a new tile checks HitObstacle: true→state0/event1. Otherwise nonwalkable proposed tile→state0/event2. Commits all accepted steps; same-tile steps bypass both tests. |
| 7 / `0x004845d0` | Arrival→restore pending. Otherwise advances line, stores world position and walking animation with no tile callbacks, obstacle check or admission check. |
| 8 / `0x00484630` | If height+70 is0, copies initial velocity+44 to velocity+46 and post-decrements repeat word+3a; old count0 restores pending after storing0xffff. Otherwise/remaining repeats add velocity to height and decrement velocity, both modulo16 bits. Its TEST DX,DX / JA clamp writes0 only when the resulting height is0; negative signed values remain nonzero. Each active tick sets animation0, requests next clockwise octant `(facing+1)&7`, and calls turn helper. |
| 9 / `0x00484790` | Polls the tile one full square ahead. If raw RF low2 bits are not3, clears flag8/restores pending but continues this handler. Calls tile descriptor+18 if present (otherwise response2). Response low2 bits1 or2: tests descriptor+1c, calls+20 if that gate is nonnull, clears flag8, restores pending and immediately dispatches the restored low state in the same call. Response0 or3 increments+20 and sets animation2. Malformed cell/pointer cases retain the faults described below. |
| 10 / `0x00483e20` | If current position is walkable, sets state0 immediately. Otherwise heading proposal→special interception→path-square stop helper→avoidance→commit, stopping at the first nonzero helper result. Unlike4/14 it has no +5c reset test. |
| 11 / `0x00484470` | Arrival→state0. Otherwise advance line; only a new tile invokes ordinary HitObstacle. True→state0/event1; otherwise commit. It does not require the new tile to be walkable. |
| 12 / `0x00484520` | Same as11 with the worker-obstacle helper0x483580, which exempts RF2 cells carrying CF0x8800. |
| 13 / `0x004848e0` | Holds only while the mouse hit-record type has mask0x200 and its object pointer equals this Bloke. Otherwise clears flag8 and restores pending. |
| 14 / `0x00483d90` | Computes heading proposal but does nothing else until unsigned32 `(global0x8119a4 - Bloke+54) >=50`; then follows state4 interception, tile action, avoidance, commit and low-seven-bit +5c reset order. |
| 15 / `0x00484350` | If on-path flag2 already set, state0 immediately. Arrival also state0. Otherwise advance line; on a new tile, HitObstacle OR a walkable proposed tile causes state0 with no event. All remaining cases commit. This is the counterpart to6: it stops when reaching walkable ground. |

All table rows above are decoded from the fingerprinted handler bodies below. Existing helper contracts are DBPrintf, pending-state restoration, line advancement, tile centre/new-tile tests, facing scheduling and RF/path direction helpers. The original state8 clamp is preserved as an unsigned-zero behavior; the prose does not substitute a conventional ballistic clamp. [original executable](../../../legoland/original/legoland.exe), [sweep1.c](../../LEGOLAND/sweep1.c), [sweep3.c](../../LEGOLAND/sweep3.c), [workers.c](../../LEGOLAND/workers.c), [tilehelp.c](../../LEGOLAND/tilehelp.c), [bnvmove.c](../../LEGOLAND/bnvmove.c)

### Event delivery to high-level plans

| Low state | Completion/stop cause | Event byte+64 mutation |
| --- | --- | --- |
| 2 | Not walkable and no CF0x10 exception; RF0x24 decision | OR2; OR4 respectively |
| 3 | Not walkable; RF0x24 decision; RF8 decision | OR2; OR4; unchanged respectively |
| 6 | New-tile obstacle; new tile not walkable | OR1; OR2 respectively |
| 11,12 | New-tile obstacle under selected rule | OR1 |
| 6,11,12,15 | Arrival | Unchanged |
| 15 | Already on path, obstacle or arrival at walkable ground | Unchanged |

Each listed event stop writes low state0. None overwrites the event byte; OR preserves prior bits until DoHighLevelAI clears it. Line state7 restores pending instead of forcing0. High-level consumers must therefore distinguish successful state0 arrival from a nonzero event; they cannot infer an event from every stop. [original executable](../../../legoland/original/legoland.exe), [blokeai.c](../../LEGOLAND/blokeai.c), [high-level evidence](ai-data.md)

## Missing helper closure

`TurnOneOctant@0x00483850` pre-decrements the byte delay; nonzero returns. A zero sets delay3, then updates facing by+1 when `(current-requested)&4` is nonzero, otherwise−1, finally masks7. The helper itself does not test equality; state5 does. `WaitingAnimation@0x00483890` only writes selector+74=2. [original executable](../../../legoland/original/legoland.exe)

`WorkerObstacle@0x00483580` returns1 for an out-of-bounds proposed position. Otherwise define `blocked(p)=(RF(p)&2) && !(CF(p)&0x8800)`; it returns1 only when the current cell is not blocked and the proposed cell is blocked. Thus a worker already inside such a cell can leave or move within it. The ordinary `HitObstacle@0x00483510` uses the same edge idea with RF2 alone. [original executable](../../../legoland/original/legoland.exe), [workers.c](../../LEGOLAND/workers.c)

`Avoidance@0x00483c20` dereferences Person kind without a null check. Ordinary kinds turn if `HitPathEdge`, ordinary HitObstacle, or `(rand()&0x3ff)<threshold`, where threshold is0 when on-path flag2 is set and20 otherwise. Kinds2/3 use HitPathEdge or WorkerObstacle and have no random background turn. On a turn, direction=`rand()&7`, forced odd by OR1 when on-path, and passed to NewDirForAction; the helper returns1 regardless of whether that direction was already current. A zero result permits the caller’s proposed step to commit. [original executable](../../../legoland/original/legoland.exe), [pathbuild.c](../../LEGOLAND/pathbuild.c), [workers.c](../../LEGOLAND/workers.c)

`PathSquareStop@0x00483b60` first requires no new tile, a centre crossing and action flag4 still clear; it then requires current position walkable and `FindPathSquare(&world_x)` nonnull. Success writes state0 and returns1, otherwise0. The pointer passed at `0x00483bf2` is Bloke+0x68 with raw24.8 x/y; no shift creates map-square coordinates. FindPathSquare compares its input directly against the stored map-square rectangle, so the apparent units mismatch is an original operation, not silently normalised in the specification. [original executable](../../../legoland/original/legoland.exe), [pathsq.c](../../LEGOLAND/pathsq.c), [tilehelp.c](../../LEGOLAND/tilehelp.c)

`DoRndWalkPathTileAction@0x00483920` is already source-covered and agrees with this bounded byte check. It requires current walkability; RF8 chooses the first filtered nonreverse direction; RF0x24 chooses a random direction after removing reverse and the two adjacent reverse diagonals; RF0x10 chooses the first filtered direction or random0..7 if no bits. Each handled branch marks flag4 and returns NewDirForAction’s result. It obtains map-square coordinates by shifting world by8, unlike the PathSquareStop call. [original executable](../../../legoland/original/legoland.exe), [bnvmove.c](../../LEGOLAND/bnvmove.c), [workers.c](../../LEGOLAND/workers.c), [pathtile2.c](../../LEGOLAND/pathtile2.c)

### Special-tile interception and callback ownership

Raw RF low bits3 identify the dynamic/special resource case. The cell tile ID at+8 indexes8-byte entries at `0x00801f40`; the entry’s first pointer is the descriptor. Its slots+18/+1c/+20 are queried status, enter and leave callbacks. Status takes world x/y and defaults to2 when absent. Enter/leave also take world x/y. The map row table is `0x00801400` with20-byte cells. These dynamic callbacks reuse the resource contract already read by GetCurrentRFFlags; they are not extra unknown AI handlers. [original executable](../../../legoland/original/legoland.exe), [map.c](../../LEGOLAND/map.c), [tilehelp.c](../../LEGOLAND/tilehelp.c), [world.md](world.md)

`InterceptEntry@0x00483300` first requires a new tile, strict interior bounds, raw proposed RF low bits3 and dynamic status low bits3. Only then it calls StartSpecialWait and returns1; otherwise0. StartSpecialWait saves current state to pending, sets state9 and flag8, resets wait count+20, computes the tile one full square ahead from **current position and facing**, and calls that tile’s +1c enter callback if present. It does not move the Bloke. [original executable](../../../legoland/original/legoland.exe), [pathtile2.c](../../LEGOLAND/pathtile2.c), [map.c](../../LEGOLAND/map.c)

`TileCallbacks@0x00483680` does nothing unless the proposal crosses a tile boundary. If raw current RF is3, it resolves the current tile, tests descriptor+1c and, when +1c is nonnull, invokes descriptor+20 without checking +20, then clears flag8. If raw proposed RF is3, it calls proposed descriptor+1c when present and then sets flag8 regardless of whether a callback existed. It does not store the proposed coordinates; caller commits afterwards. [original executable](../../../legoland/original/legoland.exe), [map.c](../../LEGOLAND/map.c)

Original faults are part of this contract: cell lookup branches can produce null and then immediately dereference+8; raw RF/CF getters lack bounds checks; leave paths gate on+1c but call+20, so a null+20 still faults if enter exists. State9’s no-longer-special branch restores pending and continues, allowing a second restore to clear the state to0; successful release immediately calls the restored table slot without a bounds guard. Nested activity also has only one pending word, not a stack. [original executable](../../../legoland/original/legoland.exe), [map.c](../../LEGOLAND/map.c), [sweep3.c](../../LEGOLAND/sweep3.c)

## Exact instruction witnesses

These short witnesses make the nonstandard branches directly reviewable; the reproduction command prints all instructions for all28 bodies when passed `--disasm`. Addresses and bytes are checked by the same function manifests. [original executable](../../../legoland/original/legoland.exe)

```asm
00484655 jne 0x484661       ; old repeat count: zero falls through to restore
00484658 call 0x483240
00484665 add edx,ecx        ; 16-bit height is stored from DX
00484667 dec ecx           ; velocity decremented separately
00484668 test dx,dx
0048466f mov word ptr [eax+0x46],cx
00484673 ja 0x48467b        ; TEST clears CF: any nonzero DX branches, including negative
00484675 mov word ptr [eax+0x70],0

0048488b mov ecx,dword ptr [eax+0x1c]
0048488e test ecx,ecx
00484890 je 0x48489a
00484894 call dword ptr [eax+0x20] ; different callback from the tested pointer
004848a1 call 0x483240
004848a9 mov ax,word ptr [ebx+0x0e]
004848ad call dword ptr [eax*4+0x4bd34c] ; same-tick restored-state dispatch

00483dbb add esp,8
00483dbe sub edx,ecx
00483dc2 cmp edx,0x32
00483dc5 jb 0x483e1a        ; unsigned elapsed comparison, not signed or equality
```

## Function fingerprints and call frontier

The first16 rows are in dispatch-index order. The following11 are missing helpers closed here; the final row is the source-covered junction helper checked independently. All referenced immediate non-CRT targets outside this manifest have existing C markers in the source files cited below. The original CRT rand call at `0x0049e4b2` is the only unmarked immediate external target. [original executable](../../../legoland/original/legoland.exe), [sweep1.c](../../LEGOLAND/sweep1.c), [sweep3.c](../../LEGOLAND/sweep3.c), [bnvmove.c](../../LEGOLAND/bnvmove.c), [bnvpath.c](../../LEGOLAND/bnvpath.c), [pathmisc2.c](../../LEGOLAND/pathmisc2.c), [pathbuild.c](../../LEGOLAND/pathbuild.c), [pathsq.c](../../LEGOLAND/pathsq.c), [tilehelp.c](../../LEGOLAND/tilehelp.c), [map.c](../../LEGOLAND/map.c), [workers.c](../../LEGOLAND/workers.c), [sweep2.c](../../LEGOLAND/sweep2.c), [bigsim.c](../../LEGOLAND/bigsim.c), [pathtile2.c](../../LEGOLAND/pathtile2.c)

| Function | VA → exclusive end | Instructions / bytes | SHA-256 |
| --- | --- | --- | --- |
| State0 diagnostic no-op | `0x004838a0 → 0x004838ba` | 8 / 26 | `72ad34d7f27dfad0633822e893029a9d7c66ae8345673ac0d29bff1974f7585a` |
| State1 byte delay | `0x004838c0 → 0x004838da` | 10 / 26 | `9263e6f071317c38c3603c251b742a226bef16f44066c165fa31b40fc6ac2755` |
| State2 path following | `0x00483ef0 → 0x00484087` | 159 / 407 | `32114564983baf2df09cd809a97a89383f9f51eebafe5033e53c84f7de125b07` |
| State3 path decision stop | `0x00484090 → 0x0048419f` | 114 / 271 | `901b391427b8e14a72bff4ef526539281a59a38d66e5ce7fc30f0325c99de367` |
| State4 random walk | `0x00483d10 → 0x00483d8b` | 54 / 123 | `004d82c2ecc36e32be94ab0dea7348c63d68f19c47d9aad5a24c1b24c1e917e1` |
| State5 turn | `0x004838e0 → 0x00483911` | 19 / 49 | `7a34cb4779c4bbe4ee99815324e719297c2611c4abc133de400c432524318b75` |
| State6 line on walkable ground | `0x00484220 → 0x00484346` | 112 / 294 | `b6bd90830f6888dfa96890ec0f00e83796076b25d188cbb8e1dc8935f07b1e71` |
| State7 unrestricted line | `0x004845d0 → 0x0048462e` | 35 / 94 | `2b07778f4364372c9a4975297e72d4faa57f2569144693139bc5e0e9275ecbb3` |
| State8 hopping/turning | `0x00484630 → 0x00484692` | 34 / 98 | `11ece7c503125dd827cb1934116a3ea3447b58cad292bcc9293befa27574071c` |
| State9 special-tile wait | `0x00484790 → 0x004848d1` | 123 / 321 | `1ed22b5dc57599d9bd0fe46825884359436da17519a611a9f5768666ea100d95` |
| State10 find walkable ground | `0x00483e20 → 0x00483eee` | 88 / 206 | `277ac87a083aa27f5745610205368da35b0cf15d184f51cda8e218003450d8c7` |
| State11 line obstacle check | `0x00484470 → 0x0048451c` | 64 / 172 | `8bacb5c94529bbc1ac0386667847b28a8f7af21f179c80d60e7507de4ac2afae` |
| State12 worker line obstacle check | `0x00484520 → 0x004845cc` | 64 / 172 | `2a256514878dcecb8cd426b13f4d0c5bb31bf080f932e0723ff3c108be3a5674` |
| State13 mouse-held wait | `0x004848e0 → 0x00484904` | 11 / 36 | `efe47caf5c3b52faf7e8444162e3253834f9cc76c5716564cff829a2e40c3a2f` |
| State14 delayed random walk | `0x00483d90 → 0x00483e1e` | 60 / 142 | `3b498aa454e9184688420bc5f9e47f1c40ae99359ec127336a09799d839fc933` |
| State15 line until path | `0x00484350 → 0x0048446c` | 108 / 284 | `d60e86f46213e4ec5921ea87d40c26a632d70ccfb8faa76d2afe399bbe9a3ebb` |
| Direction delta | `0x004831a0 → 0x004831cf` | 13 / 47 | `6daa3e135a6e147ed679a96901b7ce33e10cec66864508daaf6bc16f27ff7464` |
| Special-tile entry interception | `0x00483300 → 0x004833c2` | 79 / 194 | `bc7ceb40a863228025685156a4b150a1ef1ffafa64f27d3c1e72654971c3c366` |
| Worker obstacle edge | `0x00483580 → 0x0048364c` | 74 / 204 | `a423193d10cb9a4e65ff857c638440b7a1d1a4ca3c2a79929453e65dd08c0718` |
| Tile enter/leave callbacks | `0x00483680 → 0x0048379a` | 107 / 282 | `5d9bf223b1277db5a4a3e22b0379210691ad9015f66e96f565db5ddbb8879efe` |
| Turn one octant | `0x00483850 → 0x00483883` | 21 / 51 | `284f811f09f5f5488e480868b6c35ac4400b6fde86ae34af7c37cac109559ece` |
| Waiting animation selector | `0x00483890 → 0x00483899` | 3 / 9 | `17a3897997ca7d47612ca5705a07052f4341ac80b26bb6992d2983eb55a36e14` |
| Path-square stop check | `0x00483b60 → 0x00483c16` | 76 / 182 | `4b95ed089176b43af7c8a4fc477ff640ce11ca5576f8664ad2298fd18b95206f` |
| Random obstacle avoidance | `0x00483c20 → 0x00483d0b` | 97 / 235 | `8794aff5852d3f3ddefa043daa4f149b3612b051035fba1c74c8e26343839d2b` |
| Arrival radius | `0x004841a0 → 0x004841d3` | 21 / 51 | `39efaafbf13f73b39ea241148a5fc2900687dc85b80c03a7c12b7b77093420d7` |
| Strict interior bounds | `0x00483160 → 0x00483199` | 21 / 57 | `d595e0827cff498fa8fb3639b2983d9b8e4d15b701e7d0c670b2b6367fc92128` |
| Start special-tile wait | `0x00483260 → 0x004832f2` | 54 / 146 | `1ff235efca785d622c9bb2d0a59ea688967c8ddf928ece740cce310548ba2b07` |
| Random-walk tile action (source-covered) | `0x00483920 → 0x00483b09` | 171 / 489 | `1b61ea16e8353a79c6a24e15908dc1a6cdeb24c07381404e922f2a07b792bd88` |

## Read-only reproduction

Save this code in a temporary file and run `python3 check_low_ai.py /path/to/legoland.exe`; optional `--disasm` prints the exact instruction listing. Capstone decodes the bytes without running them. Hashes, contiguous decoding, reachable instruction coverage, direct branches, terminal RETs, table membership, direction coefficients and the complete immediate-call frontier are independently asserted. [original executable](../../../legoland/original/legoland.exe)

```python

from pathlib import Path
import sys, struct, hashlib
import capstone
from capstone.x86_const import X86_OP_IMM
raw = Path(sys.argv[1]).read_bytes()
assert len(raw)==802857
assert hashlib.sha256(raw).hexdigest()=="c50865b60bfcb26c0a7329a75fb772b10ae234af324f669901906e5abb0e2bd9"
def u(fmt,at): return struct.unpack_from("<"+fmt,raw,at)
pe=u("I",60)[0]
assert raw[pe:pe+4]==b"PE\0\0"
opt=pe+24
assert u("H",opt)[0]==0x10b
base=u("I",opt+28)[0]
sections=[]
for i in range(u("H",pe+6)[0]):
    at=opt+u("H",pe+20)[0]+i*40
    _,rva,size,offset=u("4I",at+8)
    assert offset+size<=len(raw)
    sections.append((base+rva,size,offset))
def read(va,n):
    for start,size,offset in sections:
        if start<=va and va+n<=start+size:
            p=offset+va-start
            return raw[p:p+n]
    raise ValueError((hex(va),n,"not file-backed"))
table=read(0x4bd34c,64)
assert hashlib.sha256(table).hexdigest()=="03820b747995544c325d2932a04b1153062a532f51f8f181e7aad0f8460ae46e"
directions=read(0x4bd32c,32)
assert hashlib.sha256(directions).hexdigest()=="e6c650614328323da7c29a4d3aa1130661edfc7e7af0998e5b5dfbe95e5dc60d"
assert struct.unpack("<16h",directions)==(-181,-181,0,-256,181,-181,256,0,181,181,0,256,-181,181,-256,0)
# label, start, exclusive end, instruction count, SHA-256
functions=[

    ('State0 diagnostic no-op',0x4838a0,0x4838ba,8,'72ad34d7f27dfad0633822e893029a9d7c66ae8345673ac0d29bff1974f7585a'),

    ('State1 byte delay',0x4838c0,0x4838da,10,'9263e6f071317c38c3603c251b742a226bef16f44066c165fa31b40fc6ac2755'),

    ('State2 path following',0x483ef0,0x484087,159,'32114564983baf2df09cd809a97a89383f9f51eebafe5033e53c84f7de125b07'),

    ('State3 path decision stop',0x484090,0x48419f,114,'901b391427b8e14a72bff4ef526539281a59a38d66e5ce7fc30f0325c99de367'),

    ('State4 random walk',0x483d10,0x483d8b,54,'004d82c2ecc36e32be94ab0dea7348c63d68f19c47d9aad5a24c1b24c1e917e1'),

    ('State5 turn',0x4838e0,0x483911,19,'7a34cb4779c4bbe4ee99815324e719297c2611c4abc133de400c432524318b75'),

    ('State6 line on walkable ground',0x484220,0x484346,112,'b6bd90830f6888dfa96890ec0f00e83796076b25d188cbb8e1dc8935f07b1e71'),

    ('State7 unrestricted line',0x4845d0,0x48462e,35,'2b07778f4364372c9a4975297e72d4faa57f2569144693139bc5e0e9275ecbb3'),

    ('State8 hopping/turning',0x484630,0x484692,34,'11ece7c503125dd827cb1934116a3ea3447b58cad292bcc9293befa27574071c'),

    ('State9 special-tile wait',0x484790,0x4848d1,123,'1ed22b5dc57599d9bd0fe46825884359436da17519a611a9f5768666ea100d95'),

    ('State10 find walkable ground',0x483e20,0x483eee,88,'277ac87a083aa27f5745610205368da35b0cf15d184f51cda8e218003450d8c7'),

    ('State11 line obstacle check',0x484470,0x48451c,64,'8bacb5c94529bbc1ac0386667847b28a8f7af21f179c80d60e7507de4ac2afae'),

    ('State12 worker line obstacle check',0x484520,0x4845cc,64,'2a256514878dcecb8cd426b13f4d0c5bb31bf080f932e0723ff3c108be3a5674'),

    ('State13 mouse-held wait',0x4848e0,0x484904,11,'efe47caf5c3b52faf7e8444162e3253834f9cc76c5716564cff829a2e40c3a2f'),

    ('State14 delayed random walk',0x483d90,0x483e1e,60,'3b498aa454e9184688420bc5f9e47f1c40ae99359ec127336a09799d839fc933'),

    ('State15 line until path',0x484350,0x48446c,108,'d60e86f46213e4ec5921ea87d40c26a632d70ccfb8faa76d2afe399bbe9a3ebb'),

    ('Direction delta',0x4831a0,0x4831cf,13,'6daa3e135a6e147ed679a96901b7ce33e10cec66864508daaf6bc16f27ff7464'),

    ('Special-tile entry interception',0x483300,0x4833c2,79,'bc7ceb40a863228025685156a4b150a1ef1ffafa64f27d3c1e72654971c3c366'),

    ('Worker obstacle edge',0x483580,0x48364c,74,'a423193d10cb9a4e65ff857c638440b7a1d1a4ca3c2a79929453e65dd08c0718'),

    ('Tile enter/leave callbacks',0x483680,0x48379a,107,'5d9bf223b1277db5a4a3e22b0379210691ad9015f66e96f565db5ddbb8879efe'),

    ('Turn one octant',0x483850,0x483883,21,'284f811f09f5f5488e480868b6c35ac4400b6fde86ae34af7c37cac109559ece'),

    ('Waiting animation selector',0x483890,0x483899,3,'17a3897997ca7d47612ca5705a07052f4341ac80b26bb6992d2983eb55a36e14'),

    ('Path-square stop check',0x483b60,0x483c16,76,'4b95ed089176b43af7c8a4fc477ff640ce11ca5576f8664ad2298fd18b95206f'),

    ('Random obstacle avoidance',0x483c20,0x483d0b,97,'8794aff5852d3f3ddefa043daa4f149b3612b051035fba1c74c8e26343839d2b'),

    ('Arrival radius',0x4841a0,0x4841d3,21,'39efaafbf13f73b39ea241148a5fc2900687dc85b80c03a7c12b7b77093420d7'),

    ('Strict interior bounds',0x483160,0x483199,21,'d595e0827cff498fa8fb3639b2983d9b8e4d15b701e7d0c670b2b6367fc92128'),

    ('Start special-tile wait',0x483260,0x4832f2,54,'1ff235efca785d622c9bb2d0a59ea688967c8ddf928ece740cce310548ba2b07'),

    ('Random-walk tile action (source-covered)',0x483920,0x483b09,171,'1b61ea16e8353a79c6a24e15908dc1a6cdeb24c07381404e922f2a07b792bd88'),

]
assert struct.unpack("<16I",table)==tuple(f[1] for f in functions[:16])
assert len(functions)==28 and sum(f[3] for f in functions)==1800
assert sum(f[2]-f[1] for f in functions)==4668
md=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_32)
md.detail=True
external=set()
known={f[1] for f in functions}
for label,start,end,count,digest in functions:
    code=read(start,end-start)
    assert hashlib.sha256(code).hexdigest()==digest,label
    ins=list(md.disasm(code,start)); at=start
    assert len(ins)==count and ins[-1].mnemonic=="ret",label
    nodes={i.address:i for i in ins}
    for i in ins:
        assert i.address==at,label
        at+=i.size
        if i.mnemonic=="call" and i.operands[0].type==X86_OP_IMM:
            target=i.operands[0].imm
            if target not in known: external.add(target)
        if "--disasm" in sys.argv:
            print(f"{i.address:08x} {i.bytes.hex():20} {i.mnemonic:8} {i.op_str}")
    assert at==end,label
    pending=[start]; visited=set()
    while pending:
        at=pending.pop()
        if at in visited: continue
        assert at in nodes,(label,hex(at))
        visited.add(at);i=nodes[at]
        if i.mnemonic.startswith("ret"):continue
        if i.group(capstone.CS_GRP_JUMP):
            assert i.operands[0].type==X86_OP_IMM,(label,"indirect branch")
            pending.append(i.operands[0].imm)
            if i.mnemonic=="jmp":continue
        pending.append(at+i.size)
    assert visited==set(nodes),(label,"unreachable/missing instruction")
    print("function OK",label,count,end-start)

assert external=={0x453a20,0x45c010,0x45c020,0x45c050,0x45c830,0x461610,0x461630,0x461760,0x4807f0,0x481790,0x483240,0x4833d0,0x483400,0x4834a0,0x483510,0x483650,0x4837a0,0x4837d0,0x483830,0x483b10,0x4846a0,0x49e4b2}

print("external immediate calls",[hex(x) for x in sorted(external)])
# Consequences of the original unsigned-zero and byte-wrap instructions.
assert ((0-1)&255)==255
assert [x for x in (0,1,0x7fff,0x8000,0xffff) if x>0]==[1,0x7fff,0x8000,0xffff]
assert ((0-1)&0xffff)==0xffff
print("PASS: 16 dispatch slots, 28 bodies, 1800 reachable instructions, call frontier, direction table")

```
