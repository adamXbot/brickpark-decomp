# Scope AC — advisor movies and InitMan texture helpers

Branch `scope/AC` from main @ `4671f8fd`. Object prefix `/tmp/sac_`.
Worktree `.worktrees/scope-ac`.

## Status

**13 of 15 exact.** Remaining floors:

1. `PutOne3DBlokeOnRide` — 81i/221B size-exact, **34 mism**. Track lands in
   ESI not EDI; cascades loop regs. Probes (idx-live, volatile sink,
   rebuild-after-call, seats local, pointer-walk outer) did not flip EDI
   without breaking size/homes.
2. `LoadAltTextures` — 229i exact, `sub esp,0x3c0` via path/line/name/pad
   struct; **~78 mism / −4B**. Fail-path `xor ebx,ebx` vs `mov ebx,[text]`;
   countB unread on !text/!countA (original uninit `mov ebp,[esp+0x14]`);
   scalar-home permutation + tolower esi/edi swap.

## Per-function results

| Address | Name | Insns | Match | Audit | Marker |
| --- | --- | ---: | ---: | --- | --- |
| `0x00441910` | `RiderTrackToScreen` | 35 | 100% | [OK] | FUNCTION |
| `0x00441980` | `PutOne3DBlokeOnRide` | 81 | ~58% | WIP | WIP (34 mism) |
| `0x004427e0` | `ReadAltLine` | 57 | 100% | [OK] | FUNCTION |
| `0x00442860` | `FindAltNameIndex` | 45 | 100% | [OK] | FUNCTION |
| `0x00442980` | `LoadAltTextures` | 229 | ~72% | WIP | WIP (78 mism) |
| `0x00443bd0` | `LoadAdvisorMovie` | 123 | 100% | [OK] | FUNCTION |
| `0x00443d50` | `FreeAdvisorClip` | 22 | 100% | [OK] | FUNCTION |
| `0x00443d90` | `InitAdvisorBmi` | 5 | 100% | [OK] | FUNCTION |
| `0x00443dc0` | `StartAdvisorClip` | 33 | 100% | [OK] | FUNCTION |
| `0x00443f90` | `GetAdvisorClipByPose` | 17 | 100% | [OK] | FUNCTION |
| `0x00443fe0` | `NextAdvisorPose` | 15 | 100% | [OK] | FUNCTION |
| `0x00444020` | `AdvisorMovieTick` | 17 | 100% | [OK] | FUNCTION |
| `0x00444090` | `InitAdvisorMovies` | 51 | 100% | [OK] | FUNCTION |
| `0x00444150` | `KillAdvisorMovies` | 46 | 100% | [OK] | FUNCTION |
| `0x004441f0` | `ClearReportState` | 2 | 100% | [OK] | FUNCTION |

## Mechanics

### mantex.c
- **RiderTrackToScreen**: seat-track float pair → screen ints via
  `((int)(pos - cam)) / 2 + anim->origin`. Cam constants 320.f / 270.f at
  `0x004ab4c0` / `0x004ab4bc`.
- **ReadAltLine**: byte-at-a-time RES line reader; CR consumes following LF;
  returns 0 on EOF with empty buffer.
- **FindAltNameIndex**: NameCompare walk of a string list; -1 at empty.
- **PutOne3DBlokeOnRide** (WIP): clamps frame, fills `person->matrix[9]`
  column-major from seat-track floats × 65536 with sign tables at
  `0x004b7ae0..0x004b7b0c`, then `SetPersonPosition`.
- **LoadAltTextures** (WIP): LoadTextFile altman/altwoman (two-section pack
  like LookupTextureName); sprintf `.\3ddata\new\%s\%s`; sex picks outfit
  A/B globals; RES walk of visitor.txt with tolower-to-dot, sscanf, palette
  NameCompare, FindAltNameIndex into tabA/tabB.

### advisor.c
- **LoadAdvisorMovie**: OpenMovie without audio — video-only `'vids'`, open
  tally `0x00665f48`, skips clip+0x18.
- **InitAdvisorBmi**: wanted BMI 112×96, 16 bpp, sizeimage `0x5400`.
- **StartAdvisorClip**: clears four advisor state ints, optional stop
  callback, closes prior GETFRAME, opens new one on `g_advisor_bmi`.
- **FreeAdvisorClip**: closes GETFRAME / releases stream / frees record;
  `AVIFileExit` when the open tally hits zero (tail `jmp`).
- **GetAdvisorClipByPose / NextAdvisorPose**: jump-table switches on pose.
- **AdvisorMovieTick**: pose machine; installed at `clip+0x24`.
- **KillAdvisorMovies**: frees all six AD_*.avi clip slots.
- **ClearReportState**: zeros first dword of `g_report_state`.

## Levers

- **RiderTrackToScreen**: `float frame[3]; frame[0]=pos->x; frame[1]=pos->y;
  frame[0]-=320.f; frame[1]-=270.f;` yields `sub esp,0xc` and the
  `fld / mov ybits / fsub / fld / fsub / fstp` schedule (shape R).
- **ReadAltLine**: stop path must still `if (c != '\\r')` before the CR-eat
  so a maxlen hit on CR consumes LF.
- **FindAltNameIndex**: `while (NameCompare != 0)` with `strlen==0` (not
  `list[0]==0`) for the `repne scasb` empty test; `return idx` not
  `return 0` on the first hit.
- **LoadAdvisorMovie**: OpenMovie decl order without `audio = 0`; only
  `video = 0` so ebx is the shared zero/video carrier; frames/fps/w/h
  uninitialized; do not write clip+0x18.
- **LoadAltTextures**: leave countB unread on fail paths; path/line/name/pad
  struct locks `sub esp,0x3c0`; `index = 0` sinks between LoadTextFile pushes
  as `mov [esp+0x18],0`.
- **PutOne3DBlokeOnRide**: dual `Pos` locals → `sub esp,0x10`;
  `*(float*)&screen_y = 65536.f` + fistp on reused `frame`; **EDI for track
  not recovered**.

## Callees / globals named

- `g_ride_cam_ox/oy`, `g_ride_mtx_{chan,row,sign_a,sign_b}`, outfit tables
  (LoadAltTextures), `g_advisor_bmi`, six `g_ad_*` clip slots,
  AVIFile thunks (same non-dllimport spelling as movie.c).
