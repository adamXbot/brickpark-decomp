# Codex B work log

Base: `9d7354e` (`origin/main`, 2026-09-05). Scope: `docs/SCOPE_CODEX_B.md`.
The user approved substituting the existing local environment for the paths
in the brief: Homebrew `python3` (Capstone 5.0.7), and
`/Users/systemadmin/Downloads/alpha team/alphateam/tools/wibo-msvc/cl`.
The isolated worktree is `legoland-scope-b`, branch `codex/scope-b`.
Original binaries and the existing toolchain are local-only symlinks; neither
is staged or committed.

## Stages and completion gates

1. Establish environment and ownership: all 60 scoped addresses checked for
   existing markers; existing matched code passes a per-file audit.
2. Recover `screencb6.c`: disassemble all 18, name/group by callback slot;
   audit each body and compile `/W3`; commit and push the complete file.
3. Recover `audio4.c`: seven full bodies, audit and `/W3`, then commit/push.
4. Recover `uimisc.c`: 35 full bodies, audit and `/W3`, then commit/push.
5. Review every reported exact marker against audit output, document any
   honest WIP residual, and check the diff contains only assigned files.
6. Return to `codex/audio-extraction` for the pending video work: review the
   saved diff, run media and playback checks, and deliver a separate checkpoint.

## Prior audio alignment

The audio branch's WAV extraction, ADPCM decoding, voice pause/resume and
DirectMusic rendering are host playback/reference work. They help exercise the
same audio behaviours but implement none of the seven native `audio4.c` targets.
The audio voice, decoder and Data Lab checks passed on 2026-09-05. Video work
is preserved uncommitted in that separate worktree until this scope is finished.

## Current state

Environment/ownership complete; baseline `screencb4.c` is 3/3 audit OK.
`screencb6.c` is 18/18 audit OK (309 instructions), with a clean `/W3` compile.
All rows below are 100%, audit `[OK]` yes, committed marker `FUNCTION`, no
diverging index or residual. Counts come from the complete-body extent audit.

| Address | Name | Instructions | Naming evidence |
| --- | --- | ---: | --- |
| 00452ba0 | Dino_AC | 9 | existing destroy name, +ac |
| 004529c0 | Fountain_AC | 9 | existing destroy name, +ac |
| 00434f50 | JungleCruise_SelectForPlacement | 11 | JUNGLE CRUISE, +8c |
| 00433ca0 | JcMonkeyTree_Create | 12 | MONKEY TREE, +a4 |
| 00434040 | JcMonkeyTree_GetDrawDesc | 12 | MONKEY TREE, +a0 |
| 00434080 | JcMonkeyFish_Create | 12 | MONKEY FISH, +a4 |
| 00433ce0 | JcMonkeyTree_SelectForPlacement | 14 | MONKEY TREE, +8c |
| 004340c0 | JcMonkeyFish_SelectForPlacement | 14 | MONKEY FISH, +8c |
| 004314f0 | OctopusCafe_Add | 15 | OCTOPUS CAFE, +98 |
| 004304a0 | Restaurant2_GetDrawDesc | 16 | RESTAURANT 2, +a0 |
| 00431120 | Restaurant2_Destroy | 18 | RESTAURANT 2, +ac |
| 004312c0 | FoodService_Remove | 19 | shared three foodcart/three cafe classes, +9c |
| 00436160 | JungleCruise_BestValue | 22 | JUNGLE CRUISE, +c0; matches BestTake sibling |
| 004361a0 | JcWater_SelectForPlacement | 22 | JUNGLE CRUISE WATER, +8c |
| 004529e0 | Fountain_Add | 23 | existing add name, +98 |
| 0042b9d0 | Balloonz_Destroy | 26 | BALLOONZ, +ac |
| 0042c3f0 | Carousel_Destroy | 27 | CAROUSEL, +ac |
| 00452bc0 | Dino_Add | 28 | existing add name, +98 |

### Callback mechanics and codegen evidence

- **Placement callbacks**: select the class, reset the cursor and pass the
  footprint. Jungle Cruise links its two dock rectangles; water copies its
  fixed five-word footprint. Tree/fish/water set UI flag 8.
- **Resource lifetimes**: dino/fountain destroy decrements the shared sound
  refcount and only frees five FX entries at zero. Restaurant/Balloonz/Carousel
  destroy their exact ordered sprite/bundle lists. Unnamed list-clearing callees
  at 0042a9f0/0042bc40 are named `Balloonz_FreeRecords`/`Carousel_FreeRecords`:
  both repeatedly delete their global list head until empty.
- **Best value**: station +40 is compared against zero and the running maximum;
  `working_only` requires a non-null route at +08. This is not a capacity field.
- **Packed-square removal ABI**: food-service removal takes a packed map square
  in one stack slot (low two bytes x/y), then a separate cursor pointer.
  Removal receives both; `RemoveAllBlokesFromRide` gets the square, and money
  FX gets its stack address. Independent callee review caught an initial
  misleading two-int Pos interpretation despite normalized code equality.
- **Sound record offsets**: FX entries are 12 bytes with sample at +8, matching
  `audiomisc.c`'s loader. Confirmed absolute reads at 004b8718 and indexed
  004b8770, not merely relocation-normalized audit equality.
- **Named random index schedules argument setup**: `which = rand() % 5`
  before the PlayInstance call changes Dino_Add from 28i/89B to the exact
  28i/87B. The nested expression moved stack cleanup/argument work before rand.
  Caller-local unsigned `rand` reproduces the original unsigned `div`; some
  existing translation units declare signed `rand`. Names are unchanged.
- **Original quirks retained**: the kind-2 source leaves its unused object
  member uninitialized; destroy refcounts are not protected against underflow.
  No speculative null guards or altered call order were added.

## Native audio checkpoint

`audio4.c`: 7/7 functions, 320 instructions, 100%, audit `[OK]` yes, committed
marker `FUNCTION` for every row; no divergent index/residual. `/W3` clean.

| Address | Name | Instructions |
| --- | --- | ---: |
| 00480170 | ReadBE16 | 15 |
| 00480150 | ReadBE32 | 15 |
| 004927b0 | StopPlayableSample | 32 |
| 004801a0 | ReadMidiTrack | 34 |
| 00498920 | PauseCurrentTrack | 35 |
| 00492130 | InitSoundSampleSystem | 39 |
| 00498630 | PlayNarrationFile | 150 |

- **Important correction to existing caller comments**: `PauseCurrentTrack`
  is destructive streamed-SPEECH teardown, not music ducking. It stops/rewinds,
  unprepares and closes ACM, closes the descriptor, releases DirectSound,
  frees all three heap blocks, and clears the state. `PlayNarrationFile`
  prepares NEW speech; `ResumeCurrentTrack` (00498b00) primes and starts that
  stream, not the old one. No DirectMusic calls occur here. Existing `fpui5.c`
  and front-end aliases are read-only in this lane; integrator should correct
  their explanatory comments separately.
- **Speech states**: 0 no resources, 1 loaded/stopped/rewound, 2 ring primed,
  3 playing. Verified by the adjacent 00498870/004988c0/004989b0/00498b00
  routines. New callee names follow those bodies: `ResetNarrationStreamState`,
  `StopNarrationPlayback`, `ReadNarrationWaveHeader`, `RewindNarrationSource`.
- **Conversion fidelity**: local `speech\\filename` first, resource-volume
  prefix fallback; no extension appended. Source scratch is ten source blocks.
  ACM converts to PCM 16-bit with source channels/rate unchanged. DirectSound
  has a 0xa000-byte refillable ring; its looping flag is not file looping.
- **Original risks preserved**: MIDI chunk ID/read counts/allocations are not
  validated; the rest of the track is left uninitialized until its player.
  Speech uses unbounded path concatenation and ignores ACM/heap/buffer setup
  failures; a null DirectSound result is dereferenced. WAV-header failure
  closes the file but may leak its allocated format. No added guards.
- **Inline swap evidence**: EBP frames plus `bswap edx` / `xchg dl,dh` in
  these tiny /O2 readers support source inline assembly. VC6 MASM's spelling
  `xchg dh,dl` reproduces the original opcode/decoded operand order; the
  opposite spelling has identical semantics but one strict mismatch.
  The pointer parameter is `dest`: `out` is a MASM reserved word and produced
  C4405 warnings despite matching code; renaming cleared `/W3` without a byte change.
- **Failure block placement**: InitSoundSampleSystem's first guards jump
  INTO the GetCaps failure block, leaving success last. A nested success
  return (including a trailing success label) gives 11/39 strict mismatches;
  the shared failure-block spelling is exact 39i/134B.
- **ACM header store order is a scheduling lever**: src, size, status,
  srcLength, dst, dstLength closes PlayNarrationFile at 150i/566B. An initial
  dst-first order was 150i/565B with nine strict mismatches. A bounded sweep
  stopped at the first exact order (33 variants); no asm used in the loader.
- **Extern type notes**: the two speech functions return int despite callers'
  old void declarations. ACM/DirectSoundCreate imports are stdcall direct
  thunks, not dllimport indirect calls (verified against the PE import table).
  CRT names retain established aliases; memset/strcpy/strcat inline here.

## UI/help/report checkpoint

`uimisc.c`: 35/35 functions, 795 instructions. Every row is 100%, audit
`[OK]` yes, committed marker `FUNCTION`; no diverging index or residual.
`/W3` is warning-free. All three files together: **60 exact, 1424 instructions**.

| Address | Name | Instructions |
| --- | --- | ---: |
| 004714a0 | ResetInfoPopUp | 11 |
| 00471d60 | ResetToolIcons | 12 |
| 0046df30 | ClipToIcon | 12 |
| 0046f330 | IconHitTest | 13 |
| 0046b4f0 | NewScriptStep | 13 |
| 00459820 | EndLevel | 14 |
| 0046fbc0 | IndicatorInput | 14 |
| 004730f0 | PU_CloseInput | 15 |
| 0046d340 | ShowObjectHelp | 15 |
| 0046d230 | ShowIdHelp | 16 |
| 004731a0 | PU_DeleteInput | 16 |
| 004733b0 | PU_PrevInput | 17 |
| 0046de50 | GetIconBounds | 17 |
| 004993c0 | ThawGameClock | 17 |
| 0046b200 | ScriptEventDue | 17 |
| 00473310 | PU_ToolA | 21 |
| 0046d280 | ShowHelpPopup | 21 |
| 0046d460 | UnlinkIcon | 22 |
| 0046d4a0 | UnlinkIcon2 | 22 |
| 0046c5c0 | KillHelpText | 22 |
| 00473360 | PU_NextInput | 23 |
| 0046d4e0 | DeleteIcon | 23 |
| 0046b520 | FreeScriptStep | 25 |
| 00444200 | SaveReport | 28 |
| 00444260 | LoadReport | 30 |
| 00474130 | GetTypedChar | 30 |
| 0046b6b0 | ShowScriptStepText | 30 |
| 004733f0 | PU_GardenerInput | 31 |
| 00473460 | PU_MechInput | 31 |
| 004585c0 | KillCurrentScreen | 32 |
| 00490b20 | ReportNextPageInput | 32 |
| 0048a790 | RestoreFreePlaySelections | 38 |
| 0048ac60 | FreePlayAcceptInput | 38 |
| 00490aa0 | UpdateReportPageIcons | 38 |
| 00490970 | ReportAcceptInput | 39 |

### UI mechanics, quirks and levers

- **RestoreFreePlaySelections** replaces the placeholder `FreePlayInit_48a790`.
  It replays chosen class icons in groups 200/500/400/300 with a bulk-update
  latch suppressing individual sounds. It assumes the lookup yields a valid
  row and the icon has an input callback; those unchecked dereferences remain.
- **Report index**: `g_rep_page` is a 1-based starting LINE index (1,15,29),
  not a page ordinal. Buttons advance by 14; previous is disabled at <=1,
  next when start+14 exceeds line count. Narration gets start/14.
- **Report close** frees UI/text, optionally plays the queued movie, destroys
  speech, thaws the clock, resumes singly paused samples and resets help.
  The sample-resume operation is separate from `PauseCurrentTrack`.
- **Save/load report** preserves the opaque 160-byte report block and a relative
  appraisal deadline, with -1 meaning no deadline. Failed reads can partially
  mutate global state; no rollback added. Returning the named failed result
  reproduces the original bare early returns.
- **Help ownership**: ShowScriptStepText creates a kind-1, priority-2 event.
  Copy mode retains the step string; transfer mode nulls the step's string
  and sets event flags to 0x20. KillHelpText frees goals, object-help events,
  script events and the step list, then clears both step pointers.
- **Help hover**: playing narration suppresses target changes; the object/id
  variants still mark help requested on that path. Forced popup accepts only
  a new valid id. UI press/hold uses bits 1/4; release actions use bit 2.
- **Icon unlink quirk**: the focus comparison is against the successor, not the
  removed node. The separate FreeIcon destructor also clears focus for the
  removed node. Tail globals can point at a list-head link slot; casts retain
  the original sentinel convention. A null deletion on an empty list is not
  guarded in the original and would dereference null; reproduced.
- **Named next pointer closes DeleteIcon**: the simple prev->next loop was
  23i/56B with nine strict mismatches. A free volatile head read was inert;
  naming `next = prev->next` inside the loop changes the allocation web,
  restoring 23i/57B exactly without extra reads.
- **Read order closes GetTypedChar**: evaluating current key state before the
  previous-state bit (even though VC6 schedules the previous byte first) adds
  the original EBX web. Previous-first was 29i/80B, current-first is 30i/81B
  exact. Return int/char and a free volatile state read did not close it.
  Its private previous-state array is 00668de4, NOT GetInputChar's 00668da8.
  Last newly pressed mapped key wins; -10 becomes ':'.
- **Clock ambiguity retained**: the wall-clock offset is clear, but ThawGameClock
  also adds `[008119a4]-[0079a89c]` to 0079a8a0. Other files call 008119a4
  `g_detail`/`g_ms_flags`; this lane preserves the full-width read and calls
  the second pair auxiliary rather than inventing timer semantics.
- **Naming/prototypes**: DeleteIcon keeps the scope/screens3 name for 0046d4e0
  (iconui's older declaration is RemoveIcon). No duplicate definition is added.
  RestoreFreePlaySelections is the sole body for 0048a790. Callback input
  pointers retain the four-argument ABI where the original forwards all four.
  Existing declaration aliases remain untouched in accordance with the scope.

## Verification and handoff

- Full-file audits of all three assigned files: 60 `[OK]`, zero rejected/WIP
  functions, equal full instruction/byte extents and no escaping branches.
- Each new translation unit compiles warning-free at /W3 /O2 /Gy /Gd.
- Scope branch contains only the three new C files and this lane note; no
  existing C files, tools, integration docs, binaries or assets were changed.
- Independent read-only review completed for all 60 bodies' absolute globals,
  callback targets, callees, strings, field offsets and vtable slots. It caught
  the packed-square removal interpretation described above; after correction
  no further actionable findings remained.
