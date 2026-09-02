# Ride class -> callback table

Recovered by matching all fifteen remaining `*_GetInterfaces` providers
(`LEGOLAND/interfaces.c`) plus the three already matched
(`Joust_GetInterfaces`, `TempleSlide_GetInterfaces` in `ridesave.c` and
`CastleObj_GetInterfaces` in `castleobj.c`).

A `GetInterfaces` provider compares an element's class NAME and, on a match,
stores a run of function pointers into the object definition's callback slots.
So this table names 265 callback addresses AND says what each one is for,
which is how a cluster can be reconstructed without reading it first.

Slot meanings: `8c` tick/select, `90` update, `94` update2, `98` add,
`9c` remove, `a0` draw, `a4` create, `a8` activate, `ac` destroy,
`b0` interact, `b8` load, `bc` save, `c0` extra.

```
DATA STRUCTURES AND RULES LEARNED

1. THREE CODEGEN SHAPES, and which C produces each.
 (a) ONE class, no saved register (the eleven small ones). `mov eax,[esp+4] / mov ecx,[eax] / push ecx / push str / call 0x4aab90 / add esp,8 / test eax,eax / jne end / mov eax,[esp+8]` then the run of stores. This is Joust_GetInterfaces verbatim: `if (NameCompare("NAME", elem->name) == 0) { def->slot = Fn; ... }`. Argument order matters — the literal is the FIRST argument (it is pushed second). `def` is re-read from [esp+8] after the `add esp,8`, which is why no register needs saving.
 (b) SEVERAL classes (WaterWorks, WesternTown, LogFlume): `push esi / mov esi,[esp+8]` pins `elem`, `elem->name` is RELOADED (`mov eax/ecx/edx,[esi]`) for every compare rotating eax->ecx->edx, `def` comes from [esp+0xc], and each arm except the last does `pop esi` BEFORE its stores and returns; the last arm falls into one shared `pop esi / ret`. A plain `else if` chain produces exactly this — no lever needed.
 (c) Garden (0x004329c0) uses `#pragma intrinsic(strcmp)` — VC6's inline two-bytes-per-iteration loop (`mov dl,[eax] / mov bl,[esi] / mov cl,dl / cmp dl,bl / jne / test cl,cl / je / ...+1 / add eax,2 / add esi,2` then `xor eax,eax` or `sbb eax,eax / sbb eax,-1`), with the class name as the FIRST strcmp argument and the literal as the second. It pushes ebx/esi/edi, keeps `elem->name` in edi across both compares and re-seeds eax from edi for the second.

2. THE COMPARE IS NOT UNIFORM ACROSS THE GAME. Fourteen providers call the CRT _stricmp at 0x004aab90 (case-insensitive); Garden calls plain strcmp. So HEDGE and FLOWERS are the only two classes in LEGOLAND whose custom callbacks would NOT be installed if the .ODF spelled the class name in a different case. Reproduced as an original quirk.

3. A SIDE EFFECT BEYOND SLOT STORES. LogFlume's "LOG FLUME TRACK" arm stores `elem->data` (the ObjDef) into a module global at 0x0082c688 — emitted before the `pop esi`, i.e. the first statement of that arm. Named g_logflume_track_def. It is the only provider in the lane that writes anything other than def->slots.

4. CROSS-CHECKS AGAINST ridesave.c, both confirmed: GOLD RUSH's +0xbc/+0xb8 are SaveGoldWash 0x00407800 / LoadGoldWash 0x00407870; JAIL CELL's are SaveJailCells 0x00438780 / LoadJailCells 0x004387f0. The slot->save-record mapping in ridesave.c's header is right.

5. SHARED HANDLERS ACROSS CLASSES — three of them, worth matching once each:
   0x0043a390 Shop_Draw   (+0xa0) — all NINE western-town/shop classes
   0x0043a3d0 Shop_Remove (+0x9c) — six of the nine (all but JAIL CELL, LEGO SHOP 1, LEGO MEDIA SHOP, which own placement state)
   0x0040ed50 LFPiece_Draw(+0xa0) — all eight non-ENTRANCE, non-TRACK log-flume pieces

6. SLOT-SHAPE REGULARITIES. Every ride with a save chunk fills +0xb8/+0xbc as a pair. +0x90/+0x94 appear ONLY in the log flume (both) and WATER WORKS (+0x90 only) — so those two subsystems are the only ones using the secondary update slots outside CASTLE OBJ. +0xc0 is filled exactly once in the whole lane (LOG FLUME ENTRANCE, 0x004119c0). Two classes are deliberately thin: FLOWERS has no remove handler, and WATER WORKS CROCODILE FOUNTAIN has only +0x90/+0x98/+0x9c — no create, so it never loads resources of its own.

===========================================================================
RIDE -> CALLBACK TABLE (slot: 8c tick/select, 90 upd, 94 upd2, 98 add,
9c remove, a0 draw, a4 create, a8 activate, ac destroy, b0 interact,
b8 load, bc save, c0 extra). All addresses are VAs.
===========================================================================
CASTLE LEVEL 1 (0x00403080): a4 402ca0 | ac 402ce0 | 8c 402ff0 | 98 403060 | 9c 403030 | a8 402dc0 | b0 402d00
FORT (0x004068b0): a4 406240 | ac 4062a0 | 8c 406820 | a8 406660 | b0 4062c0 | 9c 406880 | 98 406860
TEMPLE (0x00416e50): a4 4169c0 | ac 416a30 | 8c 416dc0 | a8 416b50 | b0 416a60 | 9c 416e20 | 98 416e00
GOLD RUSH (0x004078f0): a4 406a10 | ac 406ab0 | 8c 4075b0 | a8 4072b0 | b0 406b10 | 98 4075f0 | 9c 4076e0 | b8 407870(LoadGoldWash) | bc 407800(SaveGoldWash)
CATAPULT (0x00403bb0): a4 4031e0 | ac 403250 | 8c 403930 | a8 403820 | b0 403270 | 9c 4039a0 | 98 403970 | a0 4039e0 | bc 403a20 | b8 403af0
COPTERS (0x00405110): a4 403d90 | 8c 404450 | 98 404600 | 9c 404580 | a8 404be0 | a0 404490 | b0 404290 | ac 404040 | bc 404f60 | b8 405050
SAFARI RIDE (0x00415030): a4 414d90 | ac 414ea0 | 8c 414f00 | a8 415220 | b0 414b80 | 9c 414f40 | 98 414fc0 | a0 414ff0 | bc 4157b0 | b8 415820
SPIDER RIDE (0x00416160): a4 415e80 | ac 415fd0 | 8c 416060 | a8 416330 | b0 415ae0 | 9c 4160a0 | 98 4160f0 | a0 416120 | bc 416880 | b8 4168f0
SPACE TOWER RIDE (0x0043b780): a4 43b2b0 | 8c 43b420 | a8 43bac0 | a0 43b4e0 | b0 43af50 | 9c 43b460 | 98 43b4b0 | ac 43b570 | bc 43b5d0 | b8 43b6a0
SPINNING BARRELS RIDE (0x0043c760): a4 43c340 | 8c 43c490 | a8 43c950 | b0 43be70 | 9c 43c4f0 | 98 43c540 | ac 43c5b0 | a0 43c570 | bc 43c620 | b8 43c690
PLANE RIDE (0x0043e220): a4 43dda0 | ac 43dee0 | 8c 43df50 | a8 43e410 | b0 43da60 | 9c 43df90 | 98 43dfe0 | a0 43e010 | b8 43e110 | bc 43e0a0

GARDEN (0x004329c0, strcmp):
  HEDGE:   a4 432480 | 8c 4324d0 | 98 4325e0 | 9c 432700 | a0 432810 | ac 4324c0
  FLOWERS: a4 432870 | 8c 4328c0 | 98 432900 | a0 432960 | ac 4328b0   (no remove)

WATER WORKS (0x00418c80):
  WATER WORKS ENTRANCE:            a4 417c00 | ac 417ae0 | 98 417c20 | 9c 417c70
  WATER WORKS WATER BLOCK:         a4 417d30 | 98 4181e0 | 9c 418230 | a8 417f90 | ac 417e40 | a0 418110 | b0 4181a0 | 90 417dd0 | bc 418aa0 | b8 418b10
  WATER WORKS SHOWER:              a4 4182e0 | 98 4184e0 | 9c 418510 | ac 418330 | a0 418540 | b0 418450 | a8 4183a0 | 90 4185c0
  WATER WORKS ELEPHANT FOUNTAIN:   a4 4186b0 | 98 4188d0 | 9c 418910 | ac 4186f0 | b0 4188c0 | a8 4187f0 | 90 418950 | bc 418b90 | b8 418c00
  WATER WORKS CROCODILE FOUNTAIN:  90 418a30 | 98 4189c0 | 9c 418a10

WESTERN TOWN / SHOPS (0x0043a400)  [a0 = 43a390 shared; 9c = 43a3d0 shared]:
  GENERAL STORE:       a4 4375d0 | ac 437610 | 8c 437630 | a0 43a390 | a8 4378e0 | 9c 43a3d0 | b0 437670
  SHERIFF:             a4 437ba0 | ac 437bd0 | 8c 437bf0 | a0 43a390 | a8 437c90 | 9c 43a3d0 | b0 437c30
  JAIL CELL:           a4 438070 | ac 4380f0 | 8c 438110 | a0 43a390 | a8 438430 | 98 437f60 | 9c 438020 | b0 438150 | b8 4387f0(LoadJailCells) | bc 438780(SaveJailCells)
  BANK:                a4 438870 | ac 4388a0 | 8c 4388c0 | a0 43a390 | a8 438960 | 9c 43a3d0 | b0 438900
  SALOON:              a4 438c60 | ac 438ca0 | 8c 438cc0 | a0 43a390 | a8 438f10 | 9c 43a3d0 | b0 438d00
  EXPLORERS INSTITUTE: a4 43a0f0 | ac 43a120 | 8c 43a140 | a0 43a390 | a8 43a1e0 | 9c 43a3d0 | b0 43a180
  LEGO SHOP 1:         a4 439200 | 98 439320 | 9c 439350 | ac 4393e0 | 8c 4393a0 | a0 43a390 | a8 439460 | b0 439400
  LEGO SHOP 2:         a4 4396d0 | ac 439700 | 8c 439720 | a0 43a390 | a8 439950 | 9c 43a3d0 | b0 439760
  LEGO MEDIA SHOP:     a4 439c20 | 98 439c60 | 9c 439c90 | ac 439ce0 | 8c 439d00 | a0 43a390 | a8 439ef0 | b0 439d40

LOG FLUME (0x00410d60)  [a0 = 40ed50 shared by the last eight]:
  LOG FLUME ENTRANCE:  a4 40a2e0 | 8c 40a540 | 90 40a930 | 94 40aac0 | 98 40a600 | 9c 40abf0 | a8 40bf70 | b0 40b420 | ac 40a410 | bc 410930 | b8 410c10 | c0 4119c0
  LOG FLUME TRACK:     [g_0x0082c688 = elem->data] a4 40c350 | 8c 40dbb0 | a0 40c970 | b0 40cc50 | 90 40c4a0 | 94 40c6c0 | 98 40c780 | 9c 40c8d0 | ac 40c430
  SPECIAL CORNER 1:    a4 40e8b0 | 8c 40e630 | a0 40ed50 | b0 40edb0 | 90 40e6f0 | 94 40e830 | 98 40ead0 | 9c 40ec90 | ac 40ea30
  SPECIAL CORNER 2:    a4 40e920 | 8c 40e660 | a0 40ed50 | b0 40ee60 | 90 40e740 | 94 40e850 | 98 40eb40 | 9c 40ecc0 | ac 40ea60
  SPECIAL CORNER 3:    a4 40e970 | 8c 40e690 | a0 40ed50 | b0 40ef00 | 90 40e790 | 94 40e870 | 98 40ebb0 | 9c 40ecf0 | ac 40ea80
  SPECIAL CORNER 4:    a4 40e9e0 | 8c 40e6c0 | a0 40ed50 | b0 40efb0 | 90 40e7e0 | 94 40e890 | 98 40ec20 | 9c 40ed20 | ac 40eab0
  LOG FLUME CSAW:      a4 40f8b0 | 8c 40fa00 | a0 40ed50 | b0 40f920 | 90 40fa20 | 94 40fa50 | 98 40fa60 | 9c 40fab0 | ac 40f900
  LOG FLUME TUNNEL:    a4 40f3e0 | 8c 40f4f0 | a0 40ed50 | b0 40f450 | 90 40f510 | 94 40f5a0 | 98 40f540 | 9c 40f580 | ac 40f430
  LOG FLUME DROP:      a4 4103e0 | 8c 4106e0 | a0 40ed50 | b0 4104b0 | 90 410700 | 94 410730 | 98 410740 | 9c 410790 | ac 410450
  LOG FLUME HOLD UP:   a4 40ff30 | 8c 4100b0 | a0 40ed50 | b0 40ffd0 | 90 4100d0 | 94 410100 | 98 410110 | 9c 410160 | ac 40ffa0

NEW GLOBAL FOUND: 0x0082c688 = the LOG FLUME TRACK ObjDef pointer (g_logflume_track_def), cached by LogFlume_GetInterfaces so the track code reaches its ObjDef without an LLIDB lookup.

CLASS-NAME STRINGS (.rdata, for anyone matching the ride bodies):
 4b408c "CASTLE LEVEL 1" | 4b45a0 "FORT" | 4b4ef0 "TEMPLE" | 4b4670 "GOLD RUSH" | 4b412c "CATAPULT" | 4b43e4 "COPTERS" | 4b4d74 "SAFARI RIDE" | 4b4eb8 "SPIDER RIDE" | 4b789c "SPACE TOWER RIDE" | 4b7978 "SPINNING BARRELS RIDE" | 4b7a6c "PLANE RIDE" | 4b7138 "HEDGE" | 4b7130 "FLOWERS" | 4b50f4/50dc/50c8/50a8/5088 the five WATER WORKS names | 4b75fc/75f4/75e8/75e0/75d8/75c4/75b8/75ac/759c the nine western-town names | 4b4bb4/4ba4/4b88/4b6c/4b50/4b34/4b24/4b10/4b00/4aec the ten LOG FLUME names
```
