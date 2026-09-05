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
- **By-value position**: food-service removal passes x/y to removal, then x to
  `RemoveAllBlokesFromRide`, then the address of the stack position to money FX.
  The second call's original `[esp+18]` is entry+8 after four pushes, hence x.
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
