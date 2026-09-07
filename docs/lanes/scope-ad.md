# Scope AD — SoftBlitRLE recolour/highlight + remaining blit helpers

Branch `scope/AD`. New files `LEGOLAND/blitmisc.c`, `LEGOLAND/rlepaint2.c`.
Object prefix `/tmp/sad_`. Continues after AB (`rlepaint.c`). Did not edit
`rlepaint.c`, `softblit2.c`, `render3.c`, `bigrender.c`.

## Per-function status

| address | name | insns | pct | audit | marker |
| --- | --- | ---: | ---: | --- | --- |
| 0x00463560 | `ResetDamageClock` | 5 | 100 | [OK] | `FUNCTION` |
| 0x00468830 | `ClearScriptTexts` | 4 | 100 | [OK] | `FUNCTION` |
| 0x004689a0 | `FreeScriptStrings` | 26 | 100 | [OK] | `FUNCTION` |
| 0x00466080 | `PresentFlip` | 102 | 100 | [OK] | `FUNCTION` |
| 0x004640f0 | `PushRenderingStatusAndRelockVideoSurface` | 70 | ~80 | no | `WIP` |
| 0x004659a0 | `BltAdvisor` | 57 | ~45 | no | `WIP` |
| 0x004632b0 | `ShowCapacityOverlay` | 98 | ~48 | no | `WIP` |
| 0x00468040 | `SoftBlitRLEFrameRecolour` | 303 | — | — | not yet |
| 0x00468410 | `SoftBlitRLEFrame` | 312 | — | — | not yet |

Gate so far: `audit.py` PASS (4/7 `[OK]`), `relocs.py` zero MISMATCH on the
four `FUNCTION` bodies (43 matched relocations), `/W3` clean.

## Names

- `ResetDamageClock` / `ClearScriptTexts` / `FreeScriptStrings` kept from
  movie3.c. `0x00667d54` first named here as `g_dmg_game_timer` (paired with
  `g_dmg_clock` at `0x00667d58`).
- `BltAdvisor` kept from screens3.c (scope's provisional `BlitAdvisorFrame`).
- `PushRenderingStatusAndRelockVideoSurface` kept from gpu.c.
- `PresentFlip` first named here: the Flip-based presenter that is
  `g_present`'s initial value (`0x004b9ca4` → `0x00466080`). scope-n called
  this the "windowed" presenter and `FlipPrimary` the fullscreen one; the
  APIs are the other way around (`Flip`+`GetFlipStatus` here, `Blt` to the
  client rect in `FlipPrimary`).
- `ShowCapacityOverlay` first named here: Shift+capacity overlay from
  gameframe's `sub_4632b0`.

## Mechanics

- `ResetDamageClock`: `g_dmg_game_timer = GetGameTimer(); g_dmg_clock =
  GetSimClock();`
- `ClearScriptTexts`: shared `xor al,al` then `g_script_text2[0]`,
  `g_script_text1[0]`. Store order is text2 then text1.
- `FreeScriptStrings`: `int n = count; int i = 0;` then the non-empty arm
  owns the walking `char**` (esi push sinks). Reloads the count each
  iteration; `i++` before `p++`. Tail-duplicated `count = 0`.
- `PresentFlip`: `LLSAuto`, optional cursor stamp, 28 ms `timeGetTime`
  floor, `Flip(0, DDFLIP_WAIT)` retrying `DDERR_SURFACEBUSY` /
  `DDERR_WASSTILLDRAWING`, restore both primary and `g_surface_78` on
  `DDERR_SURFACELOST`, then `GetFlipStatus(DDGFS_ISFLIPDONE)` wait, then
  the same fps/tick accounting as `FlipPrimary`. VC6 hoists the
  `timeGetTime` IAT into esi.
- `BltAdvisor`: 16-bpp bottom-up DIB at (x, y); RGB555→565 when
  `g_screen_depth == 2`. Opening load order is pitch, `pitch*=y`, height,
  bits, width, `dst+=x*2`, `src = dib + (h-1)*w*2 + 0x28`. Residual is
  register allocation / deferred `push ebx` / inner-loop counter (ebp vs
  ebx) — not a reconstruction error on the algorithm.
- `PushRenderingStatusAndRelockVideoSurface`: build inclusive screen rect,
  push lock status, unlock if locked (Restore retry), then always
  IntersectRect + Lock + `GetTransparentColour` + `g_video_locked = 1`.
  Residual is the status-push interleaving with `rect.bottom` (h stays in
  edx in the original) and `dwSize = 0x6c` sitting in the IntersectRect
  argument pushes.
- `ShowCapacityOverlay`: six `AICat` lines
  `[%s x %d]: Cap %d  x %d%% = %.2f (Capped %d) = %.2f` then
  `Tot Capacity = %.2f (limit %d - %d) = %d`. Product `cap*pct` is clamped
  to `scale*100`; acc sums clamped; y walks `0x14`..`0x78` in edi. Residual
  is the strength-reduced cat cursor (original esi at `+0x18` / scale) and
  a 16-byte frame delta.

## Levers

- ClearScriptTexts: `char z = 0;` then the two stores — two immediate
  `mov byte, 0` misses the `xor al,al`.
- FreeScriptStrings: function-scope `i = 0` (unconditional `push edi` /
  `xor edi,edi`) and inner-scope `p` (esi sinks). `n = (int)*p; if (n)
  HeapFree_w((void*)n);` keeps the pointer value in eax for the call.
  `if (n > 0)` is `test / jle`.
- PresentFlip: same cursor/fps shape as `FlipPrimary` (sysmisc.c). The
  Flip retry is `while (hr) { if (LOST) restore both, return 0; if (hr !=
  BUSY && hr != STILLDRAWING) return 0; Flip again; }`. GetFlipStatus is
  a peeled `while`.
- Relock: `g_screencfg` is the same `0x004bcbf4` record `surface.c` calls
  `g_screen` (one name here). `dwSize` after `IntersectRect` in the
  source sinks into the argument pushes, matching the original; writing
  it before (as Lock does) hoists it above the call.

## Extern-type / name notes

- `GetSimClock` declared `unsigned int` (sysstubs.c definition);
  `GetGameTimer` is `int`.
- `g_script_strings` at `0x007fe120` is also `g_hint_strings` in
  eventmake.c / softblit.c — same array, caller-side name kept from
  savechunks.c / movie3.c.
- `g_ai_cat[6]` at `0x00832810` is `g_map_ai.cat` (bigsim.c); named
  from the first category so this file does not depend on MapAI's header
  fields.
- `kHundredth` is the float at `0x004ab518` (0.01f). `fild` + `fmul
  dword` is `product * kHundredth` (default-promoted to double at
  sprintf), not `(double)product * kHundredth` (`fld` + `fimul`).
