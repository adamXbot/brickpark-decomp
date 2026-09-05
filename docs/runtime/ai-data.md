# High-level AI: original dispatch and visitor plans

This page closes the high-level AI behavior formerly external to blokeai.c/bigsim.c. It covers all 26 slots at `0x004b8368`, using existing C for six worker handlers and bounded disassembly for the remaining visitor/leaf targets and their missing admission helpers. The source baseline is `cf8e88c845dc4b109bbb2bd9f2a31d1177a9c9aa`; the original executable SHA-256 is `c50865b60bfcb26c0a7329a75fb772b10ae234af324f669901906e5abb0e2bd9`. No compiler or game was run. [Original executable](../../../legoland/original/legoland.exe), [dispatch table](core-data.md), [world](world.md), [blokeai.c](../../LEGOLAND/blokeai.c)

All numbers in action/state tables are hexadecimal unless marked decimal. Plan is the u16 at bloke+0x0c, low-level state the u16 at +0x0e, and action the u8 at +0x60; they are separate machines. The table is indexed by unsigned plan without bounds checking, null-checked, then called with the bloke pointer. After every dispatch, including null/no-op slots, walk-delay +0x75 becomes1 and event byte +0x64 becomes0. NewLongTermAction writes plan, zeros action/state/step(+0x10)/scratch(+0x1c), and immediately dispatches; nested transitions can therefore run another plan in the same tick. [blokeai.c](../../LEGOLAND/blokeai.c), [blokelist.c](../../LEGOLAND/blokelist.c), [original executable](../../../legoland/original/legoland.exe)

## Dispatch coverage

The 104-byte table is 26 little-endian function pointers, SHA-256 `bc260d183c96d76ce61ec1cdd006779462f3dd6921a9afdf324bdc43bf8e94de`. Its 18 distinct nonnull targets are all accounted for below. Address-only names identify behavior without inventing exported symbols. The idle states 18/19 are distinct from idle-job-search states10/11. [Original executable](../../../legoland/original/legoland.exe), [blokeai.c](../../LEGOLAND/blokeai.c), [blokemisc.c](../../LEGOLAND/blokemisc.c)

| Plan | Target | Complete disposition |
| --- | --- | --- |
| 0,4,7,8,9,a,b,c | 0x484910 | Bare ret; only dispatcher postconditions apply |
| 5 | Null | No call; only dispatcher postconditions apply |
| 1 | 0x44f170 | Set low state4 |
| 2 | 0x44ebf0 | Enter through park entrance, join entrance owner, then resume plan6 |
| 3 | 0x44ed70 | Route to entrance, leave and destroy visitor |
| 6 | 0x44f610 | Choose object, route, check running/capacity and attempt admission |
| d | 0x44fe80 | Find unreserved CAFE BROLLY, approach/reserve, wait and leave |
| e | 0x450250 | Visitor animation then state d; resume visitor/worker plan by person kind |
| f | 0x450450 | Face selected object, short random wait, mark visit, plan6 |
| 10 | 0x49a480 | Set state e, then RunGardenerJob; existing source |
| 11 | 0x49a4b0 | Set state e, then RunMechanicJob; existing source |
| 12 | 0x49a4e0 | Gardener build state machine; existing source |
| 13 | 0x49a7f0 | Mechanic build state machine; existing source |
| 14 | 0x450330 | Face direction4, state d, then resume gardener/mechanic idle |
| 15 | 0x49ba10 | Gardener repair state machine; existing source |
| 16 | 0x49bd20 | Mechanic repair state machine; existing source |
| 17 | 0x44fe10 | Move toward cached entrance tile through state f, then plan6 |
| 18 | 0x49a4a0 | Set low state e; no job-search call |
| 19 | 0x49a4d0 | Set low state e; no job-search call |

## Shared fields, motion and selection contracts

Position +0x68/+0x6c and leg target +0x24/+0x28 are 24.8 coordinates. Long destination is +0x2c/+0x30; +0x46 is a packed owner tile. Selected/previous LLIDB elements are +0x14/+0x18. Flags are u16 +0x62; +0x64 reports movement events. Direction output is +0x72, reused line-angle/turn-target byte +0x73, retry count +0x82, temporary countdown +0x58, elapsed counter +0x5c, signed leaving score +0x7a and logging character +0x81. Person kind is person(+4)->+8: visitor1, gardener2, mechanic3. [rides.c](../../LEGOLAND/rides.c), [workers2.c](../../LEGOLAND/workers2.c), [blokeanim.c](../../LEGOLAND/blokeanim.c), [original executable](../../../legoland/original/legoland.exe)

Below, `line(target,S)` means store target at +0x24/+0x28; compute `u8(CalcMoveLine(world,target,&bloke[0x98])+0x10)`; set low state S and angle +0x73; call NewDirForAction with `(angle>>5)+3`. This preserves byte wrapping before the shift. When NewDirForAction schedules turning state5, it overwrites +0x73 with the quantized requested direction; state5 reads it in that role. Exceptions that write +0x72 directly or use fixed facing are stated. `flags64` is sampled before the dispatcher's final clear; state6 reports mask1 for obstacle or mask2 for nonwalkable, while state b reports mask1 for obstacle. Successful arrival leaves these bits clear. [bnvmove.c](../../LEGOLAND/bnvmove.c), [workers.c](../../LEGOLAND/workers.c), [low-level evidence](ai-low-data.md), [original executable](../../../legoland/original/legoland.exe)

SuggestNextMove(world,destination,&leg), `0x482050`, returns −2 for no current path square, −1 for no destination/route,1 for an intermediate path-square leg,2 for same-square completion. Outputs receive +0x80 center bias. The high-level jump tables also define −3 and0 arms even though this recovered helper does not return them. PTPSuggestNextMove `0x4824d0` is the cardinal tile-search fallback:0 no route,1 intermediate centered tile,2 final requested destination. Other results leave actions unchanged. Route search/visited structures, line stepping and turning are already source-covered; they are not left as unknown helper calls here. [bnvmove.c](../../LEGOLAND/bnvmove.c), [simcore.c](../../LEGOLAND/simcore.c), [workorder3.c](../../LEGOLAND/workorder3.c), [workorder4.c](../../LEGOLAND/workorder4.c), [original executable](../../../legoland/original/legoland.exe)

The object selector rebuilds the candidate list, calls CalculateRideCodes for the visitor, resets its best cursor, then repeatedly calls ShuffleObjKeys(&destination,&class). That helper does one ascending bubble-sort pass and returns the node immediately before its shrinking cursor, yielding high-ranked candidates first. The score calculation and per-class visit counters remain those in rides.c/sweep3.c; the cutoff at this consumer is strictly **score>10**. [fpui2.c](../../LEGOLAND/fpui2.c), [rides.c](../../LEGOLAND/rides.c), [objmap.c](../../LEGOLAND/objmap.c), [sweep3.c](../../LEGOLAND/sweep3.c)

## Plan2: park entry

Entrance element is `0x006661c4`; its class is element+0xc and its first placed object comes from GetFirstObjectMatching. Original entry/exit offset pairs at `0x004b8318/0x004b8328` initialize to `(4096,0)`, meaning 16 tiles in x. Entrance route destination `0x004b8320` initializes to `(13568,10240)` and is updated by map setup; cached entrance tile `0x0066b460` is a separate tile-unit value. These mutable coordinates must not be replaced by fixed initial map positions. [objdoor.c](../../LEGOLAND/objdoor.c), [mapobj.c](../../LEGOLAND/mapobj.c), [savegame.c](../../LEGOLAND/savegame.c), [original executable](../../../legoland/original/legoland.exe)

| Action | Entry handler0x44ebf0 |
| --- | --- |
| 0 | Set flag8. Target x=`(owner.x+class.right(+44)+6)<<8`, y=`(owner.y+class.bottom(+48)-5)<<8`; world=target+entry-offset pair. Compute line, set state7 and +72 directly to quantized angle; increment action to1. Does not write +73 or call turning helper here |
| 1 | Store entrance element at+14; attempt Join(bloke,class,0). Success increments action to2, saves current plan/action into +a/+8, switches to null plan5 and ORs dirty mask0x40 at0x668610 |
| 2 | Clear flag8, NewLongTermAction(6) |
| Other | No handler work |

The save operation at `0x44ebb0` stores plan+0xc into saved-plan+0xa and action+0x60 into saved-action+8; it does not allocate a stack. Entrance/instance lookup is not null-checked before reading owner/class fields in action0. Join failure keeps action1 for retry. [sweep1.c](../../LEGOLAND/sweep1.c), [objmap.c](../../LEGOLAND/objmap.c), [original executable](../../../legoland/original/legoland.exe)

## Plan3: leaving

The leaving routine has action table0x44f118 and route-result table0x44f150. Action0 falls directly into the action1 route attempt in the same invocation. Flags8 remain set until the visitor is destroyed or another explicit consumer clears them. Logging helper0x44ed00 updates an eight-entry diagnostic ring as described below. [Original executable](../../../legoland/original/legoland.exe), [workers.c](../../LEGOLAND/workers.c)

| Action | Leaving handler0x44ed70 |
| --- | --- |
| 0 | Set flag8, retry byte+82=0, action=1; fall through to1 |
| 1 | SuggestNextMove(world,entrance destination,&leg). Result−2 sets action5. Results−3/−1/0 set state4, increment retry and set action2, except retry becoming8 sets action5. Result1: line(leg,6), action0 if flags64=0, otherwise state4/action2. Result2: line(leg,6), action a if flags64=0, otherwise state4/action2 |
| 2 | Set flag8 and action1 |
| 5 | Set flag8, log fallback, run PTP(world,entrance,&leg). Result0 state4/action6; result1 line(leg,b), changing action to6 only if flags64&1; result2 line(leg,b), action6 when flags64&1 else a |
| 6 | Log wandering; state4/action5 |
| a | Attempt Join(bloke,entrance.class,0); success sets flag8, selected element, increments action to b, saves plan/action and switches to plan5 |
| b | Target x=`(entrance.owner.x+class.right+6)<<8`, y=`(owner.y+class.top(+40)+8)<<8`; line(target,7), action c |
| c | RateBlokeOnLeaving(sign-extended score+7a), add exit-offset pair to prior target, line(target,7), action d |
| d | Log removal, DestroyBloke, decrement visitor count0x6661bc |
| 3,4,7,8,9 or >d | No handler work |

## Plan6: select, route and admit

At entry the visitor's unsigned name byte+0x81 is stored in logging selector0x813b08. Failed attempts distinguish a full object, a non-running object and failed admission in their mood-event/logging paths. The selected pointer stored at+0x14 is the candidate class's element at+0xc4. [Original executable](../../../legoland/original/legoland.exe), [rides.c](../../LEGOLAND/rides.c)

| Action | Selection/route handler0x44f610 |
| --- | --- |
| 0 | Rebuild/rank candidates. Skip the previous element (+18). For score<=10, mark the class visited only when its current count is zero, then try the next. Score>10 selects element+14, action1, retry byte+82=0. Initial empty candidate list sets action2; exhaustion after trying candidates starts plan3 immediately |
| 1 | SuggestNextMove(world,long destination,&leg), using the result rules immediately below |
| 2 or3 | Log wandering, state4, increment action (to3 or4) |
| 4 | Action0 |
| 5 | PTP(world,long destination,&leg). Result0 state4/action6; result1 line(leg,b), setting action6 only when flags64&1; result2 line(leg,b), action6 if flags64&1 else a |
| 6 | Log wandering; state4/action5 |
| a | Admission sequence below |
| 7,8,9 or>a | No handler work |

For action1, result−2 sets state a and leaves action unchanged. Results−3/−1/0 call the adjacent-entrance predicate0x44f180 on the long destination and selected class. False gives state4 without changing action. True computes the intended owner as `(destination>>8)-class.origin`; it looks for a matching placement after that cell, falling back to the first matching placement. No placement sets state4 and increments action. If the returned placement is the same owner, it only sets state4; otherwise it updates long destination to `(new owner+class.origin)<<8 +0x80` and returns. Result1 line(leg,6) sets action1 if flags64=0, otherwise0. Result2 line(leg,6) sets action a if flags64=0, otherwise0. [Original executable](../../../legoland/original/legoland.exe), [objmap.c](../../LEGOLAND/objmap.c)

Action a first requires the adjacent-entrance predicate for the visitor's current world position; failure sets action2. It resolves the packed owner through GetObjectUID, counts current riders against signed class capacity+0x2e, and if full applies mood event0 with signed scale+0x3a, marks that class visited if needed and sets action2. A stopped/unpowered object uses the same rejection path but mood event1. IsObjectRunning uses the established global power-switch rule and has its original unchecked null-cell dereference. [Original executable](../../../legoland/original/legoland.exe), [objmap2.c](../../LEGOLAND/objmap2.c), [sysmisc3.c](../../LEGOLAND/sysmisc3.c), [simcore2.c](../../LEGOLAND/simcore2.c)

Otherwise it resolves the instance at the owner and tests instance.flags bit2. Already-set bit2 takes the failed-admission path. With bit2 clear it switches to plan5 before attempting Join. It then reads **selected element+0x1c** and tests0x100000. Set takes Join with timer0; on success it re-counts capacity and sets instance.flags bit2 only when capacity is now full. Clear resolves the instance again (the result is unused), chooses timer `(rand()&0x1ff)+200` (decimal 200..711), and joins. Successful Join returns; failure applies mood event0, marks visited if necessary and writes action2 **without restoring plan6**, because plan5 was already installed. Thus the failure can leave null plan5/action2. The exact element+0x1c access is also retained: the element has stride0x14, so for an ordinary interior element this addresses the next element's flags+8, not the selected class's flags+0x1c. Treating it as class.flags would silently change the original. [Original executable](../../../legoland/original/legoland.exe), [LLIDB allocation/layout](../../LEGOLAND/llidb.c), [class layout](../../LEGOLAND/llidb_odf.c), [rides.c](../../LEGOLAND/rides.c)

## Plan d: CAFE BROLLY reservation

The element at0x6661c0 is CAFE BROLLY. The routine uses cell.flags bit1 as its reservation marker and stores the chosen packed owner at bloke+0x46. It does not use the general class rider chain. Every switch arm that computes a cell can dereference a null result after its bounds checks, so valid stored coordinates remain a precondition. [rides.c](../../LEGOLAND/rides.c), [original executable](../../../legoland/original/legoland.exe)

| Action | Handler0x44fe80 |
| --- | --- |
| 0 | Find first matching placement whose cell flag1 is clear, walking matching objects past occupied ones. None starts plan6. Save owner at+46 and destination `(owner+class.origin)<<8`; action1 |
| 1 | SuggestNextMove to destination. Results−3/−1/0 state4; −2 state a; result1 line(leg,6), action1 when flags64=0 else plan6; result2 line(leg,6), action2 when flags64=0 else plan6 |
| 2 | Re-read saved cell: wrong element or missing flag0x80 starts plan6; occupied flag1 restarts action0. Otherwise set flag1, set flag8, target=(destination.x−0x80,destination.y), line state7 with fixed facing7, elapsed+5c=0, action3 |
| 3 | Validate same element/flag0x80; invalid starts plan6. Wait until signed elapsed+5c>300 decimal, then action4 |
| 4 | Validate same element/flag0x80; invalid starts plan6. Clear reservation flag1, target=(destination.x+0x80,destination.y), line(target,7), action5 |
| 5 | Clear flag8 and start plan6 |
| Other | No work |

Failures after reservation do not run a universal cleanup: only action4 clears the reservation and only action5 clears flag8. The specification preserves these conditional writes; a replacement should not infer a guaranteed finally-style release that the original lacks. [Original executable](../../../legoland/original/legoland.exe)

## Plans e, f,14 and17

The following compact machines complete the remaining nontrivial visitor/worker return handlers. All unlisted action values do nothing. Animation helpers and person-kind dispatch use the existing recovered implementations. [Original executable](../../../legoland/original/legoland.exe), [blokeanim.c](../../LEGOLAND/blokeanim.c), [workers.c](../../LEGOLAND/workers.c)

| Plan/action | Exact behavior |
| --- | --- |
| e/0 | NewDirForAction(4). Visitor person kind1 sets flag0x100, animation2, frame0 and action1; other kinds set action2 |
| e/1 | Wait for PlayBlokeAnim success; then walk animation, frame0, clear0x100, action2 |
| e/2 | Low state d, action3 |
| e/3 | Person kind2→plan10, kind3→plan11, every other kind→plan6 |
| 14/0 | NewDirForAction(4), increment action |
| 14/1 | Low state d, increment action |
| 14/2 | Person kind2→plan10, kind3→plan11; other kinds unchanged |
| f/0 | Face chosen class rectangle through helper0x4503a0; timer+58=`(rand()&31)+10` (decimal10..41), action1 |
| f/1 | Decrement signed timer; advance to action2 only once negative |
| f/2 | Increment selected class visit counter for this visitor, then plan6 |
| 17/0 | Target=global cached entrance tile0x66b460 shifted left8 on both coordinates (no0x80 bias); line(target,f), action1 |
| 17/1 | Plan6 |

Facing helper0x4503a0 compares `world>>8` with class rectangle `+0x3c` biased by the stored +0x2c/+0x30 **as read**, without shifting that bias. To the right of the rectangle it chooses directions6/7/0 for above/within/below; to the left4/3/2; horizontally within chooses5 above and1 otherwise, including inside. Only +0x72 changes; it does not turn through NewDirForAction or validate a selected element. The tile/24.8 unit discrepancy is the original consumer's exact operation. [Original executable](../../../legoland/original/legoland.exe), [rides.c](../../LEGOLAND/rides.c)

## Source-covered worker plans

These six targets already have recovered C; their bodies were read while checking dispatch coverage, and the full work-order ownership/payment contracts remain in world.md. This table gives the complete action transitions needed to connect them with the recovered high-level dispatch. `route` here is worker FindPathLeg, and line motion uses low state c. [workers2.c](../../LEGOLAND/workers2.c), [bigsim.c](../../LEGOLAND/bigsim.c), [blokemisc.c](../../LEGOLAND/blokemisc.c), [world work orders](world.md)

| Plan | Action transitions |
| --- | --- |
| 10/11 idle | Set low state e then RunGardenerJob/RunMechanicJob; no action guard |
| 12 gardener build | 0 line to saved→b; b squared distance<0x9000→6b else64. 64 route:2→6a/6b by flags64 bit1,1 retains64 unless bit1→6a,0 gives back order and writes plan10/action64/state4/wait0x70. 6a wait0x70/state4/action64. 6b flag0x100, animation1/frame0x28→6c. 6c animation completion restores walk/clears0x100→6d. 6d clears8, builds; success frees order then next job or plan10, failure gives back, plan10 and message1 |
| 13 mechanic build | 0 line to saved→b; b squared distance<0x10000→6b else65. 65 route like gardener but failure plan11/action65/state4/wait0x70; 6a waits/retries65. 6b sets8, BuildObject: success clears+46→6c; failure gives back, starts plan11 then sets order+30=1. 6c clears order+30 and waits while cell0x20 construction flag remains; cleared→6d. 6d clears8, sets+46=1, frees and starts next job/plan11 |
| 15 gardener repair | 0 worker route:2→6/7 by flags64 bit1;1 retains0 unless bit1→6;0 walks/releases span and writes plan10/action0/state4/wait0x10. 6 wait0x70/state4/action0. 7 flags0x108, animation1, face order→8. 8 completion walk/clear0x100→9. 9 faces order; while cell.flags&0x88, charge accrued integer repair amount, preserve remainder+rate, tick life/dirty0x200; max life clears0x4000→a. No funds sets order+30. a clears8, frees and next job/plan10 |
| 16 mechanic repair | 0 line to saved→b; b squared distance<0x10000→6b else64. 64 route2→6a/6b;1 retains64 unless flags64 bit1→6a;0 walks/releases span and writes plan11/action0/state4/wait0x70. 6a wait0x70/state4/action64. 6b sets8/faces order, repairs like gardener; no funds sets+30 and message2; invalid cell.flags&0x88 advances instead of waiting. Completion→6c. 6c clears8, sets+46=1, frees and next job/plan11 |

## Missing admission and diagnostic helpers

The common Join routine0x44f4a0 is called by entry, leaving and object admission. It allocates a **20-byte** rider node `{next,prev,bloke,u16 owner,u16 zero,person}`. A first matching instance for class.element must exist. Its owner first increments matching 128-slot park bookkeeping via0x489f90. The new node is zeroed; bloke/person and timer are stored; bloke gets flag0x20, low state0, step0 and byte+0x35=0. Entrance joins take the first entrance owner; others derive owner from current world position with GetObjectUID. It ORs cell.flags4, appends at class+0xcc, ORs dirty mask0x20 and returns1. Allocation failure returns0. A successful allocation followed by no instance returns0 **without freeing the allocation**. Resolved owner-cell access is unchecked. [Original executable](../../../legoland/original/legoland.exe), [blokelist.c](../../LEGOLAND/blokelist.c), [sweep1.c](../../LEGOLAND/sweep1.c), [objmap2.c](../../LEGOLAND/objmap2.c)

| Helper | Complete operation |
| --- | --- |
| Adjacent entrance0x44f180 | Take position>>8; inspect north,south,west,east cells in that order. A valid cell needs flag0x80, nonnull element, element.class equal requested class, and owner+class.origin equal the original tile; first match returns1, otherwise0. No mutation |
| Count riders0x44f3d0 | Walk class+0xcc next chain, count nodes whose u16 owner+0xc equals requested key |
| Full predicate0x44f400 | Return count>=sign-extended class.capacity(+0x2e); negative capacity is immediately full |
| Slot counter0x489f90 | Compute u16 `(x<<8)+y`, search128 four-byte entries at0x7cb3e0; first matching key increments its u16+2 counter and returns1; no key returns0. Counter wraps; the Join caller ignores its return |
| Diagnostic0x44ed00 | Fill eight pointers at0x4b8348 with `0x6661cc+100*((old_count+i)&7)`, increment counter0x6664ec, format `%c:%s` into pointer7, then DBPrintf(`[Bloke %c] - %s\n`). Writes 100-byte ring strings without length checks; its caller's 100-byte temporary messages also use unbounded sprintf |

Rider list append/unlink, world-owner resolution, working-state tests, counter storage, mood adjustment, animation and path search are already covered by their linked C. No opaque semantic helper remains in these high-level plans; address names remain only where the original exported no symbol. The logging strings used by the recovered visitor branches are exact bytes in the manifest below, including punctuation and the leading decimal score format. [sweep1.c](../../LEGOLAND/sweep1.c), [blokelist.c](../../LEGOLAND/blokelist.c), [objmap.c](../../LEGOLAND/objmap.c), [objmap2.c](../../LEGOLAND/objmap2.c), [sysmisc3.c](../../LEGOLAND/sysmisc3.c), [sweep3.c](../../LEGOLAND/sweep3.c), [simcore2.c](../../LEGOLAND/simcore2.c), [original executable](../../../legoland/original/legoland.exe)

## Range and table evidence

The following ranges exclude alignment padding and jump-table data; ends are exclusive. Every range was decoded fully to its last ret. Jump tables are separately recorded, preventing disassembly of their bytes as if they were instructions. The read-only checker below verifies byte identity, instruction count, boundaries and direct-branch destinations for all 19 new ranges, plus all seven dispatch subtables. [Original executable](../../../legoland/original/legoland.exe)

| Role | Start | End | Instructions | SHA-256 |
| --- | --- | --- | --- | --- |
| Plan1 | `0x0044f170` | `0x0044f17b` | 3 | `959ff277e7d381dc1d55757ef604120e10b1c74f30a861db485d5c2cfb0e76da` |
| Plan2 | `0x0044ebf0` | `0x0044ecfa` | 92 | `9f827bc66a5f6ebffa7290f6d686ab4e80a111e79534c6c05c3668e2864ce294` |
| Plan3 | `0x0044ed70` | `0x0044f118` | 328 | `5710fd423321e0c482103feaaea45bb80f2f663b068dd11af6896bcd970a18e8` |
| Plan6 | `0x0044f610` | `0x0044fdc9` | 699 | `434c8bcd11c07d59794c7dfdc86841bdacf86def1e6edc70b3488ebdf95ce590` |
| Plan d | `0x0044fe80` | `0x00450220` | 337 | `cde585e146eff8c4f70da6cb5c5da9b388a8e48cc9919ec22121d2706933592c` |
| Plan e | `0x00450250` | `0x0045031b` | 77 | `3735e02619739f3d649e43c72bf5ff06cb61684c80321ee1d121cb58d01100fd` |
| Plan14 | `0x00450330` | `0x00450394` | 43 | `16c5143b1afa058e48b8bffea7e25d39846d827fbfc063a1f04922207f4b177e` |
| Plan f | `0x00450450` | `0x004504c5` | 48 | `75b73b407801af312f64c0d8f67d7dd8f1301862c1e9e9748b7ff306d4592c1a` |
| Plan17 | `0x0044fe10` | `0x0044fe80` | 43 | `20704bd87d89b273139c730a198457a3dd106c19a350a7491bf5bfde3e940880` |
| Plan18 | `0x0049a4a0` | `0x0049a4ab` | 3 | `48df8da3900603c53114f3e5afe2e12d121c3d3e1d271df9f04b78fa330cba4b` |
| Plan19 | `0x0049a4d0` | `0x0049a4db` | 3 | `48df8da3900603c53114f3e5afe2e12d121c3d3e1d271df9f04b78fa330cba4b` |
| DoNothing | `0x00484910` | `0x00484911` | 1 | `ae3f4619b0413d70d3004b9131c3752153074e45725be13b9a148978895e359e` |
| Adjacent entrance | `0x0044f180` | `0x0044f358` | 201 | `3402fb803730f50c55d919bed128070854ec9a5ba5c94cc61e0bc964f3cb1367` |
| Rider count | `0x0044f3d0` | `0x0044f3f6` | 15 | `ae13d69b158bf55f97d45085ef6ecd984e87870e414b0e180179cbad6e0728cd` |
| Capacity | `0x0044f400` | `0x0044f422` | 14 | `4284877bb551f88f0112017cb6c09c2d28482ddc9cf96562771253f10e8561fe` |
| Join | `0x0044f4a0` | `0x0044f604` | 121 | `dc9f6196325a1987ee62c9e9137f1c3892847fa8887ea9e7a9421b2f65bc35f6` |
| Facing | `0x004503a0` | `0x00450441` | 87 | `b5bd63e0a6a265db2fadd6925eb932fb8ced2f527ed321000df39b8b34d7d077` |
| Slot counter | `0x00489f90` | `0x00489fc7` | 17 | `03f77d73be84b7801b3d8b754a2b450ce636e3c88b5c49242ad3bd0e97fe605e` |
| Diagnostic | `0x0044ed00` | `0x0044ed6a` | 32 | `ea55d4b49302173b174da837646528b0cbafcd7721969dde73a812a5163c6b48` |

Each subtable is u32 pointers in numeric index order. Route-result indices are `result+3`; action indices use the unsigned action byte. These exact values establish the unusual unused-result arms and action holes described above. [Original executable](../../../legoland/original/legoland.exe)

| VA | Count | Values | SHA-256 |
| --- | --- | --- | --- |
| `0x0044f118` | 14 | `0044ed8e`, `0044ed9d`, `0044eeba`, `0044f112`, `0044f112`, `0044eec8`, `0044efc6`, `0044f112`, `0044f112`, `0044f112`, `0044eff2`, `0044f03a`, `0044f08b`, `0044f0f3` | `2bdb998cc1a77d881700a01030d991eb05a98faa712be094ab6b9b2e5fe55ebb` |
| `0x0044f150` | 6 | `0044edd3`, `0044edca`, `0044edd3`, `0044edd3`, `0044ee5c`, `0044edfe` | `fe97de23420a45d2688aea91fca8d78d379674ead6aa8ab9a4c82321953f8187` |
| `0x0044fdcc` | 11 | `0044f640`, `0044f78b`, `0044f96a`, `0044f96a`, `0044f983`, `0044f98f`, `0044fa8e`, `0044fb61`, `0044fb61`, `0044fb61`, `0044fabc` | `f8bed821491702c6d049ffe9ab04e7782b6a849da070e351228cf0657ec99cca` |
| `0x0044fdf8` | 6 | `0044f7c1`, `0044f7b3`, `0044f7c1`, `0044f7c1`, `0044f91a`, `0044f8c5` | `aa52c7958c773d4aeac5ff45f50d22b481d9f338b37c5a99f5abca8fa423f75a` |
| `0x00450220` | 6 | `0044fea4`, `0044ff35`, `0045001d`, `004500e7`, `00450159`, `00450208` | `a65dbcf066246caa06a725a36a0bd44a42666f6d8391d64602693e3b3262242f` |
| `0x00450238` | 6 | `0044ff6a`, `0044ff5d`, `0044ff6a`, `0044ff6a`, `0044ffca`, `0044ff77` | `2bcff1ab19cb5262fe1dbecdc3d76f9b49ef71a841652274222a9f1e9e8770dc` |
| `0x0045031c` | 4 | `00450270`, `004502ab`, `004502d9`, `004502e6` | `548bdcf2d00b0b5ae0f57eca229fc205ea64de5e3365886e9b3dd91e2b392ecf` |

Diagnostic messages are CP1252 NUL-terminated strings; hashes include their final NUL. The code's formatting side effects remain even if the debug display is not visible. [Original executable](../../../legoland/original/legoland.exe)

| VA | Exact string (`\n` denotes newline) | Bytes including NUL | SHA-256 |
| --- | --- | --- | --- |
| `0x004b858c` | `I've just been on the %s.` | 26 | `1a03fae057eb8a5b137700dd0c64a16d8cccfc06181233460d93eb9e8c5035e9` |
| `0x004b856c` | `The %s is not worth going on.` | 30 | `4e99f9b064b8c48656440e9738767ad2261baf63488f5321c9da906c07fc96b2` |
| `0x004b8554` | `(%d) I'll go to the %s` | 23 | `5cf28dddcc97476fb172a6809cc42ccdd33f69e72df87296c5d30f2ec7a823b9` |
| `0x004b8524` | `I've been on everything and I want to go home.` | 47 | `3573e70330e311e910fb5f0f9ea5eb99faedc2f190a362efc12ef611f9cd0547` |
| `0x004b8500` | `I can't go on this ride. It is full` | 36 | `beba9d20e279641f9d8adf7c367fa8e1affee20b7bb1d6bc4894a80e9758afdb` |
| `0x004b84d4` | `I can't go on this ride. It's not working` | 42 | `669d2c119a54f9aecb7e921703759b52810a789b4a700a0de6abd7655ee230f6` |
| `0x004b84bc` | `I'm going on the ride` | 22 | `fc7f7c39f5dea54c7c9feb8e003a2855f864dbd66949619f419ee79e8d7aa755` |
| `0x004b84a0` | `I can't get on the ride.` | 25 | `abc89dfb37d49f44e32d870c763dfbfe01563bb965a29bf1b2751e436200161e` |
| `0x004b8480` | `Couldn't find instance of %s\n` | 30 | `f98a9ace1b1769ed559cd125effd32f1498caa096300239cc414988c639ef0be` |
| `0x004b8458` | `Couldn't allocate BlokeOnRide for %s\n` | 38 | `057a896f42fd5a5d5684bf93a4d42b3a6d6826c5fbef88acf7cec4e6f7b77a31` |
| `0x004b8434` | `Stuck, Routing Point To Point...` | 33 | `20610ab2d63a02da7a279b4ed5c0782d730d3d05a870951907e3a07863be0790` |
| `0x004b8424` | `Wandering...` | 13 | `4cf2beee77eff40ca46fde962b26db350236d9febf7beb53a2f5c865e4d8500d` |
| `0x004b840c` | `Killing MiniFig: $%x\n` | 22 | `62a416803eef15bf2317c62c70f3b5bf55990b623ddce2eb1821f7fc4dec2da7` |
| `0x004b8404` | `%c:%s` | 6 | `0ce3b32e51e6b242b26eccb179232cf2b2b1958824dc200841eb7ae298af8e24` |
| `0x004b83f0` | `[Bloke %c] - %s\n` | 17 | `f5a33e12556642ddf489114aa44e2a3e6e2a055fac19ff3d6a886ef3d98860fd` |

Run this read-only command from the Scope J worktree root. It obtains the PE section map directly and rejects virtual-only data; it imports no reconstruction code and does not execute the game. Capstone is used solely to decode bytes. [Original executable](../../../legoland/original/legoland.exe)

```sh
python3 - <<'PY_AI'
from pathlib import Path
import struct, hashlib, re
import capstone
exe=Path('../legoland/original/legoland.exe').read_bytes()
assert hashlib.sha256(exe).hexdigest()=='c50865b60bfcb26c0a7329a75fb772b10ae234af324f669901906e5abb0e2bd9'
pe=struct.unpack_from('<I',exe,60)[0]
base=struct.unpack_from('<I',exe,pe+52)[0]
n=struct.unpack_from('<H',exe,pe+6)[0]
opt=struct.unpack_from('<H',exe,pe+20)[0]
sections=[]
for i in range(n):
    p=pe+24+opt+40*i
    vs,rva,size,raw=struct.unpack_from('<4I',exe,p+8)
    sections.append((base+rva,size,raw))
def read(va,n):
    for a,size,raw in sections:
        if a<=va and va+n<=a+size:
            return exe[raw+va-a:raw+va-a+n]
    raise ValueError(('not file-backed',hex(va),n))
page=Path('docs/runtime/ai-data.md').read_text()
cs=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_32)
rows=re.findall(r'^\| ([^|]+) \| `0x([0-9a-f]{8})` \| `0x([0-9a-f]{8})` \| (\d+) \| `([0-9a-f]{64})` \|$',page,re.M)
assert len(rows)==19
range_starts={int(row[1],16) for row in rows}
all_instructions=set()
for name,a,z,count,digest in rows:
    a,z=int(a,16),int(z,16);data=read(a,z-a)
    assert hashlib.sha256(data).hexdigest()==digest
    ins=list(cs.disasm(data,a));addresses={v.address for v in ins}
    all_instructions.update(addresses)
    assert len(ins)==int(count) and ins[-1].address+ins[-1].size==z
    assert ins[-1].mnemonic=='ret'
    for v in ins:
        if v.mnemonic.startswith('j') and v.op_str.startswith('0x'):
            assert int(v.op_str,16) in addresses,(name,hex(v.address),v.op_str)
    print(name,hex(a),hex(z),len(ins))
rows=re.findall(r'^\| `0x([0-9a-f]{8})` \| (\d+) \| ([^|]+) \| `([0-9a-f]{64})` \|$',page,re.M)
assert len(rows)==7
for a,count,values,digest in rows:
    a,count=int(a,16),int(count);data=read(a,4*count)
    assert hashlib.sha256(data).hexdigest()==digest
    destinations=struct.unpack('<'+'I'*count,data)
    assert destinations==tuple(int(x,16) for x in re.findall(r'`([0-9a-f]{8})`',values))
    assert set(destinations)<=all_instructions
rows=re.findall(r'^\| `0x([0-9a-f]{8})` \| `([^`]+)` \| (\d+) \| `([0-9a-f]{64})` \|$',page,re.M)
assert len(rows)==15
for a,value,size,digest in rows:
    data=read(int(a,16),int(size))
    assert hashlib.sha256(data).hexdigest()==digest
    assert data==value.replace('\\n','\n').encode('cp1252')+b'\0'
assert struct.unpack('<6i',read(0x4b8318,24))==(4096,0,13568,10240,4096,0)
assert hashlib.sha256(read(0x4b8368,104)).hexdigest()=='bc260d183c96d76ce61ec1cdd006779462f3dd6921a9afdf324bdc43bf8e94de'
dispatch_section=page.split('## Dispatch coverage',1)[1].split('## Shared fields',1)[0]
expected={}
for slots,target in re.findall(r'^\| ([0-9a-f,]+) \| (0x[0-9a-f]+|Null) \|',dispatch_section,re.M):
    for slot in slots.split(','):
        slot=int(slot,16)
        assert slot not in expected
        expected[slot]=0 if target=='Null' else int(target,16)
assert set(expected)==set(range(26))
assert struct.unpack('<26I',read(0x4b8368,104))==tuple(expected[i] for i in range(26))
source_targets={int(address,16) for source in Path('LEGOLAND').glob('*.c')
                for address in re.findall(r'//\s*(?:WIP-)?FUNCTION:\s*LEGOLAND\s+(0x[0-9a-fA-F]+)',source.read_text())}
targets=set(expected.values())-{0}
assert len(targets)==18 and len(targets & range_starts)==12
assert targets-range_starts<=source_targets and len(targets-range_starts)==6
print('PASS:19 ranges,7 jump tables/destinations,15 strings,coordinate defaults,26 dispatch slots/18 covered targets')
PY_AI
```

The checks establish complete high-level target coverage, original-byte identity and branch/table consistency. Source-backed worker plans and shared helper definitions remain linked to their primary specification; no unresolved high-level handler remains. This is a recovered behavioral specification, with the original faults retained, and does not certify a recompiled binary or runtime simulation. [World](world.md), [core data](core-data.md), [low-level evidence](ai-low-data.md)
