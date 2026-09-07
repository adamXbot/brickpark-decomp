# Scope AI — MIDI sequencer + path-square routes / object-class companions

Branch `scope/AI`. Files `LEGOLAND/music2.c`, `LEGOLAND/pathobj2.c`. Object
prefix `/tmp/sai_`. Brief: `docs/SCOPE_AI_midi_path.md` (inventory group 16).

## Status

| address | name | insns | % | audit | marker |
| --- | --- | ---: | ---: | --- | --- |
| 0x00480570 | MIDITimerTick | 33 | 100 | [OK] | FUNCTION |
| 0x004802c0 | ReadMidiVarLen | 19 | 100 | [OK] | FUNCTION |
| 0x004802f0 | ReadMidiEvent | 27 | 100 | [OK] | FUNCTION |
| 0x00480330 | UpdateMidiTrack | 157 | 100 | [OK] | FUNCTION |
| 0x004809d0 | LoadObjectClassSibling | 73 | 100 | [OK] | FUNCTION |
| 0x00480aa0 | SetWaterWorksClassOrigins | 51 | 100 | [OK] | FUNCTION |
| 0x00480b70 | SetEditObjectFromElem | 12 | 100 | [OK] | FUNCTION |
| 0x00481720 | GetPathSquareList | 2 | 100 | [OK] | FUNCTION |
| 0x004819a0 | CollectPathSquareNeighboursCounted | 127 | 100 | [OK] | FUNCTION |
| 0x00481f00 | FindPathSquareRoute | 104 | 100 | [OK] | FUNCTION |
| 0x00482a80 | ResetEntranceTile | 4 | 100 | [OK] | FUNCTION |
| 0x00482b10 | ResetEntranceTileTime | 3 | 100 | [OK] | FUNCTION |
| 0x00482cb0 | GetVisitorMoodIcon | 29 | 100 | [OK] | FUNCTION |
| 0x00482d70 | ResetMoodAdjustments | 17 | 100 | [OK] | FUNCTION |
| 0x00482ec0 | UnInitialiseBlokes | 12 | 100 | [OK] | FUNCTION |
| 0x00483090 | DestroyAllBlokes | 11 | 100 | [OK] | FUNCTION |
| 0x00489440 | AddMasterDir | 59 | 100 | [OK] | FUNCTION |
| 0x00489550 | RES_FindVolumeDir | 33 | 100 | [OK] | FUNCTION |

**18 / 18 exact** (773 instructions). Both files `/W3` clean; `relocs.py` 0
mismatches on every function (the only unresolved entries are switch
jump-table labels in UpdateMidiTrack / GetVisitorMoodIcon and the
compiler-injected `__chkstk` in FindPathSquareRoute).

## Names (all from evidence; none were exported)

- **MIDITimerTick** 0x00480570: lifecycle.c already declares it under this
  name as InitMIDIManager's `timeSetEvent(20, 10, ...)` callback. `__stdcall`,
  five DWORD args, `ret 0x14`.
- **ReadMidiVarLen / ReadMidiEvent / UpdateMidiTrack**: what they do (the
  standard MIDI VLQ, the status/running-status/meta reader, the per-track
  event pump 0x00480570 calls per track).
- **LoadObjectClassSibling** 0x004809d0: saveprof.c's extern name for it,
  kept (one name per address). Its speculative comment there ("builds
  `<prefix><name>` into a stack buffer") is wrong: the function walks the
  five-entry `ClassGroup` table at 0x004bcdcc and loads each ';'-terminated
  companion class — the sibling is a companion, not a prefixed name.
- **SetWaterWorksClassOrigins** 0x00480aa0: the three `_stricmp`s against
  "Water Works Shower" / "Water Works Water Block" / "Water Works Elephant
  Fountain" and the origin stores at ObjDef+0x0c/+0x10/+0x24/+0x25.
- **SetEditObjectFromElem** 0x00480b70: objmap.c's SetEditObject from the
  class record; this is the same from the LLIDB element (`elem->data`).
- **GetPathSquareList** 0x00481720: returns pathsq.c's `g_path_squares`.
- **FindPathSquareRoute** 0x00481f00: the BFS bnvmove.c's SuggestNextMove
  calls with (from, to, &next).
- **ResetEntranceTile / ResetEntranceTileTime**: objdoor.c's
  UpdateEntranceTile recomputes when `g_entrance_tile.x == 0`; the second
  stamps tinystubs.c's `g_entrance_tile_time` from GetGameTimer.
- **GetVisitorMoodIcon** 0x00482cb0: gameframe.c's VisitorBubbleHelp uses
  the result as a 1-based icon index beside the visitor's name.
- **ResetMoodAdjustments** 0x00482d70: the 13 defaults of eventgoalprim.c's
  `g_mood_adjustments` (SetHappinessFactor overrides single entries).
- **UnInitialiseBlokes / DestroyAllBlokes**: free blokeai.c's `g_bloke_base`
  pool + forget the list; DestroyBloke the head until empty + zero
  `g_visitor_count`.
- **AddMasterDir** 0x00489440: find-or-create half of audio3.c's
  GetMasterDirPtr over `g_master_dirs`.
- **RES_FindVolumeDir** 0x00489550: data2.c's RES_OpenFileFromVolume calls
  it and walks on from the `*member` it hands back.

## Mechanics

- **Sequencer clock**: the file clock (+0x08) advances by `tempo` every 20 ms
  tick and is compared `>> 8` against a track's absolute event tick, so the
  clock is in 1/256 ticks and `tempo = tick_scale / T` with `tick_scale =
  division * 20000`.
- **Set Tempo bug reproduced**: only the FIRST TWO of the three tempo bytes
  are read (`(b0 << 8) | b1`); the third is skipped unread, so the 24-bit
  us-per-quarter is used as its top 16 bits. Division by zero is possible
  for a tempo whose top 16 bits are 0 (< 65536 us — unrealistic, so latent).
- **Event coverage**: 0x8n/0x9n/0xbn/0xen and 0xcn go to midiOutShortMsg;
  0xan (2 data bytes) and 0xdn (1) are skipped; SysEx (0xf0/0xf7) and every
  other meta read a ONE-byte length (not a VLQ) and skip it; 0xff2f clears
  `active` and returns 0. Running status is honoured by backing `pos` up.
- **BFS direction**: FindPathSquareRoute expands from `to` towards `from`,
  so the square returned is the one adjacent to `from` on a shortest route.
  It marks squares with bit 0 of PathSquare+0x20 and never clears them
  (ClearPathSquareVisited does); an already-marked `from` returns 0 at once.
  Two 1020-slot wave buffers, no bound check.
- **Companion class table** (0x004bcdcc, 5 × {name, members}): CASTLE OBJ →
  CASTLE_DUMMY;ROLLER COASTER TRACK;SQUARE_TRACK; / LOG FLUME ENTRANCE →
  LOG FLUME TRACK; / JUNGLE CRUISE → JUNGLE CRUISE WATER; / DRIVING SCHOOL →
  DRIVING SCHOOL ROADS;ZEBRA CROSSING; / BOATING SCHOOL → BOATING SCHOOL
  WATER;. Only names followed by ';' are seen. ClassElem+0x08 bit 2 marks
  "loaded as a class"; set after a successful LLIDB_LoadData.
- **Mood icon**: lt_action 3 → 4, 11/12 → 1, 13 → 5; otherwise mood <
  `g_mood_low` → 3, < `g_mood_high` → 10, else 2.
- **Water Block** gets ObjDef+0x2e = 10 in addition to its (1, 0) origin.

## Extern-type notes

- `MidiFile.time` is `unsigned int` here (the track compare is `jb`);
  music.c declares it `int`. Same layout.
- Both files declare their record types locally (music.c, pathsq.c,
  saveprof.c, audio3.c, data2.c own the canonical copies); offsets agree.

## Levers

- **`__stdcall` bodies must be FIRST in the file.** audit.py resolves a
  decorated `_MIDITimerTick@20` only through its first-COMDAT fallback, so
  MIDITimerTick leads music2.c. Not a codegen lever — a tooling one.
- **UpdateMidiTrack's Set-Tempo bytes are ONE accumulator.** `n = byte;
  n = (n << 8) | byte` keeps the cursor in ecx and the value in eax; two
  named bytes `b0`/`b1` (any type or order) put the cursor copy in eax and
  the first byte in ecx and let VC6 hoist `t->data` above the length-skip
  store (18 strict).
- **Channel sends accumulate into `status` itself** (`status |= byte << 8`),
  not a separate `msg`: keeps the message in eax across the two loads.
- **Meta/SysEx skip is `n = data[pos++]; while (n-- != 0) pos++;` on an
  UNSIGNED counter** — the dead `dec`/`inc` pair (LP01).
- **`switch (status)` with the channel `switch (status & 0xf0)` as its
  `default`.** Four sparse cases (0xf0, 0xf7, 0xff2f, 0xff51) come out as the
  original's compare chain split at 0xff2f; the inner switch is the jump
  table.
- **`ReadMidiVarLen`'s byte is an `int` local**, not `unsigned char`: the
  original zero-extends once and tests `c & 0x80` on the int.
- **LoadObjectClassSibling: `tok = buf` BEFORE the strcpy.** Live across the
  `rep movsd/movsb` (which clobbers esi/edi) it can only sit in ebp; with
  cursor/tok/p/e then holding all four callee-saved registers VC6 leaves the
  flag constant as an immediate (`test byte ptr [edi+8],4` / `mov eax,[edi+8];
  or al,4; mov [edi+8],eax`). Assigned after the copy, `tok` shares edi with
  `e` and the freed ebx carries a hoisted `mov ebx,4` (15 strict). The
  constant is hoisted whatever its spelling — bitfield, union byte view,
  `4u`, macro — only register pressure stops it.
- **LoadObjectClassSibling: UNSIGNED index over the group table.** `for
  (i = 0; i < 5; i++)` with `unsigned int i` anchors the strength-reduced
  cursor on `.members` (+4) and compares `jb` against the table end; a
  pointer walk anchors on `.name` (+0), a signed index compares `jl`.
- **FindPathSquareRoute: TOP-tested `while (count != 0)` with `count = 1`
  going in.** VC6 folds the constant guard and bottom-tests it, and the
  loop's fall-through IS the `return 0` epilogue, which the entry guard
  (`from->flags & 1`) shares. `do { } while (n != 0)` computes the same
  thing but exiles that epilogue past the found-it block behind a `je`/`jmp`
  pair (20 strict). `while (count > 0)` is 1 off (`jg`).
- **SetEditObjectFromElem: name the class.** `ObjDef* d = elem->data;` then
  `g_edit_changed = 1; g_edit_object = d;` — inline, the load goes to ecx and
  the post-DefaultCursor reload lands in edx (5 strict); the named load
  reuses eax (RA01). The footprint is read back through the GLOBAL after the
  call.
- **GetVisitorMoodIcon: `mood < g_mood_high ? 10 : 2`** for the `setl`-based
  select; the `>=` spelling flips to `setge`/different constants.
- **AddMasterDir: `nd->next = g_master_dirs; nd->pad4 = 0;`** in that order
  (the original stores the link before zeroing +4).
- **CollectPathSquareNeighboursCounted**: a separate `n` index alongside the
  global count (both incremented) reproduces the original's paired
  `inc`/`inc [mem]`; jumping the cursor to `square->rect.right/bottom` and
  letting the `for` increment step past it is what the original does.
