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
