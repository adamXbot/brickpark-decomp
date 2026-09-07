# Scope AJ — RES volume + frontend save / sample helpers (inventory group 17)

NEW-FUNCTION scope, two files. **27/27 `audit [OK]`** (853 instructions).
`relocs.py`: 0 MISMATCH on both files (149 relocations, 148 matched; the one
UNRESOLVED is `GetProfileOffSprite`'s jump table `$L800`, a local code
symbol). `/W3` clean on both.

## `LEGOLAND/resaudio2.c` (4/4)

| address | name | insns | audit | marker |
| --- | --- | ---: | --- | --- |
| 0x004895a0 | `RES_LoadDirectory` | 147 | [OK] | `FUNCTION` |
| 0x004921c0 | `ConvertWAVToPCM` | 155 | [OK] | `FUNCTION` |
| 0x00492c60 | `SuspendMusicThread` | 7 | [OK] | `FUNCTION` |
| 0x00492c80 | `ResumeMusicThread` | 7 | [OK] | `FUNCTION` |

## `LEGOLAND/frontend2.c` (23/23)

| address | name | insns | audit | marker |
| --- | --- | ---: | --- | --- |
| 0x00489f90 | `BumpSlotCounter` | 17 | [OK] | `FUNCTION` |
| 0x00489fd0 | `GetRideVisitCountAt` | 16 | [OK] | `FUNCTION` |
| 0x0048a040 | `FreeClassInstanceLists` | 22 | [OK] | `FUNCTION` |
| 0x0048a6e0 | `UnlockFreePlayEntry` | 43 | [OK] | `FUNCTION` |
| 0x0048a750 | `UnlockClassesForFreePlay` | 15 | [OK] | `FUNCTION` |
| 0x0048a780 | `InitProfileBlock` | 1 | [OK] | `FUNCTION` |
| 0x0048a800 | `ResetFreePlayTable` | 20 | [OK] | `FUNCTION` |
| 0x0048c5e0 | `GetProfileOffSprite` | 23 | [OK] | `FUNCTION` |
| 0x0048c860 | `InitSaveDeletePopUp` | 80 | [OK] | `FUNCTION` |
| 0x0048d470 | `SaveSavedGameIconHandlers` | 5 | [OK] | `FUNCTION` |
| 0x0048d490 | `RestoreSavedGameIconHandlers` | 5 | [OK] | `FUNCTION` |
| 0x0048e0c0 | `AddNodeToSavedGameList` | 52 | [OK] | `FUNCTION` |
| 0x0048e3d0 | `SetTempProfileName` | 28 | [OK] | `FUNCTION` |
| 0x0048e420 | `ResetSavePopupIcons` | 12 | [OK] | `FUNCTION` |
| 0x0048e450 | `SaveDeleteOkInput` | 15 | [OK] | `FUNCTION` |
| 0x0048eac0 | `MarkerXToVolume` | 12 | [OK] | `FUNCTION` |
| 0x0048eaf0 | `VolumeToMarkerX` | 12 | [OK] | `FUNCTION` |
| 0x0048eb20 | `SaveOptionIconHandlers` | 5 | [OK] | `FUNCTION` |
| 0x0048eb40 | `RestoreOptionIconHandlers` | 5 | [OK] | `FUNCTION` |
| 0x0048f9f0 | `SaveFrontEndState` | 18 | [OK] | `FUNCTION` |
| 0x0048fc30 | `HaveCurrentProfile` | 5 | [OK] | `FUNCTION` |
| 0x00491540 | `TempProfileHasName` | 5 | [OK] | `FUNCTION` |
| 0x00491e40 | `PrintTextGetEnd` | 121 | [OK] | `FUNCTION` |

## Names

Names already in the tree were kept where they were right
(`BumpSlotCounter`, `GetRideVisitCountAt`, `UnlockFreePlayEntry`,
`InitProfileBlock`, `GetProfileOffSprite`, `InitSaveDeletePopUp`,
`RestoreSavedGameIconHandlers`, `AddNodeToSavedGameList`,
`ResetSavePopupIcons`, `MarkerXToVolume`, `VolumeToMarkerX`,
`SaveOptionIconHandlers`, `RestoreOptionIconHandlers`, `HaveCurrentProfile`,
`TempProfileHasName`, `PrintTextGetEnd`, `SuspendMusicThread`,
`ResumeMusicThread`, `RES_LoadDirectory`, `ConvertWAVToPCM`). New or
corrected (callers keep their own extern names; symbol names are not
levers):

- `FreeClassInstanceLists` (0x0048a040, `sub_48a040` in gameframe.c) — walks
  the class chain @ 0x00669240 and `MemFree`s every node of each class's
  +0x04 instance list (linked through +0x00), then clears the head.
- `UnlockClassesForFreePlay` (0x0048a750, `sub_48a750` in gameframe.c /
  goalstate.c) — for every class whose LLIDB element (+0xc4) has flag 2
  CLEAR, `UnlockFreePlayEntry(elem)`.
- `ResetFreePlayTable` (0x0048a800; screens3.c's `SelectProfileSlot`
  description was wrong) — zeroes the +0x0c word of every row of fpui's
  `g_fp_table` until the empty-name terminator.
- `SaveSavedGameIconHandlers` (0x0048d470, bigscreens.c's
  `SavedGame_48d470`) — stashes `g_icon_handler1/2` in 0x007986f8 /
  0x007986f4 (note the reversed slot order; 0x0048d490 restores them).
- `SetTempProfileName` (0x0048e3d0, screens3.c's `sub_48e3d0`) — `strcpy`
  into `g_temp_profile.name`, length byte at +0x1e, `g_7986f0 = 1`.
- `SaveDeleteOkInput` (0x0048e450) — the saved-game delete popup's OK
  handler: on `msg & 2` with a save slot selected, `CloseFontEndCheckBox`,
  clear `g_delete_popup_up`, `RemoveSaveGame(slot)`, `g_cur_screen = -1`,
  slot = 0; always returns 1.
- `SaveFrontEndState` (0x0048f9f0; screens3.c calls it `PlayTitleMovie`,
  uimisc3.c already describes it as the SAVE half) — the push that movie.c's
  `RestoreFrontEndState` (0x0048fa40) pops: `ui->icons2_mode`, then the
  `EditState` triple (0x008119b0), then the `FrontEndState` triple
  (0x0080ff80), in that order.
- `RES_GetOrAddMasterDir` (0x00489440, first named here, not in scope) —
  master-directory bucket lookup that creates the bucket when missing;
  audio3.c's `GetMasterDirPtr` (0x004894d0) is the lookup-only sibling.

## Mechanics recovered

- **RES directory image.** A volume's directory image is a tree of
  0x14+name nodes `{sib, sub, isdir, size, base, name[]}` linked by IMAGE
  OFFSETS (-1 = none). `RES_LoadDirectory(node, v, base, path)` files a
  file node (`isdir == 0`) as a 0x20-byte `RDirEnt` on three chains at once
  — the all-files chain @ 0x0079862c (+0x00), the directory bucket's list
  (+0x04) and the volume's list (+0x08) — with `dir`/`vol` back-pointers at
  +0x0c/+0x10, `size`/`base` at +0x14/+0x18 and a `MemAlloc(strlen+1)`
  copy of the name at +0x1c. It then recurses into `sub` with the same
  path, and into `sib` with `path + name + "\"` (the image spells a
  directory's contents as the siblings of the directory node). Every offset
  is rewritten in place to `offset + base` before it is followed, so the
  image is a pointer tree afterwards (and must not be reloaded).
  data2.c's caller passes `(dir, v, dir, g_res_root)`: the root node IS
  the image.
- **WAV -> PCM.** `ConvertWAVToPCM(data, fmt, len)` opens an ACM stream
  from `*fmt` to `{PCM, channels, rate, rate*channels*2, channels*2, 16, 0}`
  with `ACM_STREAMOPENF_NONREALTIME`, sizes the destination with
  `ACM_STREAMSIZEF_SOURCE`, converts with `ACM_STREAMCONVERTF_START`
  (0x10), frees the source, rewrites `*len = cbDstLengthUsed` and
  `*fmt = pcm`, unprepares and returns the new buffer. **ORIGINAL BUG,
  reproduced:** the stream is never `acmStreamClose`d.
- **Marked-tile counters.** `g_marked_tiles[128]` (pathmisc.c) keys on
  `(unsigned short)((x << 8) + y)`; `BumpSlotCounter` increments the hit's
  count and returns 1, `GetRideVisitCountAt` returns it (0 if absent).
- **Volume slider.** 241 pixels from x = 0x7c: `vol*241/100 + 0x7c` and
  `(x - 0x7c)*100/241`.
- **`PrintTextGetEnd`** is movie.c's `PrintCursor` / text.c's `NewPrintCent`
  body with a `DT_CALCRECT` (0x425) measurement into a local copy of the
  box before the real `DrawText` (0x25), returning
  `(rc.left + rc.right)/2 + (calc.right - calc.left)/2` — the x where the
  centred text ends. `oldfont` is homed in the dead `white` parameter slot
  by VC6 on its own.

## Levers (with evidence)

- **`int i = 0` before the key computation** puts `xor edx,edx` between
  the argument load and `mov cx,[eax]` (`BumpSlotCounter` /
  `GetRideVisitCountAt`, 16/17 → 17/17). With `int i;` and `i = 0` in the
  `for` header the zero sinks below the key.
- **Two-term sum order is the load order.** `(rc.left + rc.right) / 2`
  loads `[esp+0x30]` then `[esp+0x38]`; `rc.right + rc.left` swaps the two
  loads (119/121 → 121/121, `PrintTextGetEnd`).
- **Adjacent stores of two spilled pointers.** `e->dir = d; e->vol = v;`
  emits load v / load d / store vol / store dir; the other source order
  swaps both pairs (143/147 → 147/147, `RES_LoadDirectory`). VC6 reorders
  the adjacent pair as a unit.
- **Pack the 18-byte WAVEFORMATEX.** `pcm = *fmt` is four dword moves plus
  a word move only when `sizeof` is 18 (`#pragma pack(2)`); at the natural
  20 it becomes `rep movsd` of 5 dwords and every later frame offset shifts
  (2/155 → 155/155, `ConvertWAVToPCM`).
- **Struct-then-override.** `pcm = *fmt; pcm.wFormatTag = 1; ...` keeps
  the tag store interleaved with the copy (`mov [esp+0x20],ecx` then
  `mov word [esp+0x24],1`); the copy's dword 0 is not elided.
- **`if (!dst) return 0;` on a fresh malloc result** emits the bare
  `pop/ret` path with no `xor eax,eax` (VC6 knows eax is already 0).
- **`memset` then five field stores** on an address-taken 0x54 header:
  `rep stosd` of 0x15 then the stores scheduled around the argument
  pushes — natural source order (`cbStruct, pbSrc, cbSrcLength, pbDst,
  cbDstLength`) matches.
- **Pointer walk of a 16-byte table with `strlen(t->name) != 0` as the
  `for` test** anchors the cursor at the name field (`mov edx,0x4bdebc;
  mov [edx+8],0; mov edi,[edx+0x10]; add edx,0x10`) — the field with the
  most references wins, and the entry test is peeled (`ResetFreePlayTable`,
  `UnlockFreePlayEntry`).
- **`(unsigned char)strlen(name)` into a byte field** is the `not ecx / dec
  ecx / mov [..],cl` tail (`SetTempProfileName`).
- **`node->valid = 0` after `memset`** is a real store in the original's
  else arm (`mov dword [edx+0x114],0`); the profile-list twin
  (`AddNodeToProfileList`) has no such store. Reproduced.
- **`char` argument push.** `RemoveSaveGame(unsigned char)` is called with
  `mov al,[0x80ffe4]; push eax` — the upper bytes are whatever was in eax;
  declaring the parameter `unsigned char` reproduces it.
- **Compare-chain vs jump-table switch on `char`.** Eight dense cases
  returning eight globals give `movsx / dec / cmp 7 / ja / jmp [table]`
  with the `xor eax,eax` default after the table (`GetProfileOffSprite`).
- **12-byte struct copies** of the state triples are three register pairs;
  order of the three assignments (`ui`, `*game`, `*screen`) is the emitted
  order (`SaveFrontEndState`).

## Extern-type notes

- `InsertIcon(short, short, unsigned short, Sprite*)` as screens2.c;
  `RemoveSaveGame(unsigned char)` as profiles.c; `UpDateCurrentProfile`
  returns `char` as profiles.c.
- `ConvertWAVToPCM`'s third parameter is `unsigned long*` here (data2.c
  declares `unsigned int*`; same push).
- ACM entry points declared `__stdcall` WITHOUT `dllimport` (linker thunks),
  as audio4.c / movie2.c; `SuspendThread` / `ResumeThread` ARE `dllimport`
  (`call dword ptr [0x4ab100]` / `[0x4ab0f0]`).
- `g_cur_save_slot` (0x0080ffe4) and `g_have_profile` (0x0080ffd9) are read
  as single bytes here (screens3.c reads 0x0080ffe4 wide in other paths).
