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
| 0x004659a0 | `BltAdvisor` | 57 | 100 | [OK] | `FUNCTION` |
| 0x00466080 | `PresentFlip` | 102 | 100 | [OK] | `FUNCTION` |
| 0x00468040 | `SoftBlitRLEFrameRecolour` | 303 | 100 | [OK] | `FUNCTION` |
| 0x00468410 | `SoftBlitRLEFrame` | 312 | 100 | [OK] | `FUNCTION` |
| 0x004640f0 | `PushRenderingStatusAndRelockVideoSurface` | 70 | 85.7 | no | `WIP` (21 residual) |
| 0x004632b0 | `ShowCapacityOverlay` | 98 | ~48 | no | `WIP` (42 residual) |

Gate: `blitmisc.c` audit PASS (5/7 `[OK]`), relocs 47 matched / 0 MISMATCH,
`/W3` clean. `rlepaint2.c` audit PASS (2/2 `[OK]`), relocs 20 matched / 0
MISMATCH, `/W3` clean.

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
- `SoftBlitRLEFrameRecolour` / `SoftBlitRLEFrame` kept from render3.c /
  bigrender.c. Prototypes match AB's HitClipLR (`dst, a, b, c, h, pitch,
  top, left, w, spare, mouse`); render3's `ctrl, p16, p8` names are the
  same wrong A/B/C labels AB already documented.

## Mechanics

- `ResetDamageClock`: `g_dmg_game_timer = GetGameTimer(); g_dmg_clock =
  GetSimClock();`
- `ClearScriptTexts`: shared `xor al,al` then `g_script_text2[0]`,
  `g_script_text1[0]`. Store order is text2 then text1.
- `FreeScriptStrings`: `int n = count; int i = 0;` then the non-empty arm
  owns the walking `char**` (esi push sinks). Reloads the count each
  iteration; `i++` before `p++`. Tail-duplicated `count = 0`.
- `BltAdvisor`: 16-bpp bottom-up DIB at (x, y). Opening order is pitch,
  `pitch*=y`, height, then mid-stream `push ebx/esi/edi`, bits, width,
  `dst+=x*2`, `src = dib + (h-1)*w*2 + 0x28`. ebp sinks into the
  non-zero-height arm. Inner walk is `src-dst` offset + `ax = [edx+ecx]`;
  RGB555→565 when `g_screen_depth == 2` (`(pix & 0x1f) | ((pix &
  0xffffffe0) << 1)`).
- `PresentFlip`: `LLSAuto`, optional cursor stamp, 28 ms `timeGetTime`
  floor, `Flip(0, DDFLIP_WAIT)` retrying `DDERR_SURFACEBUSY` /
  `DDERR_WASSTILLDRAWING`, restore both primary and `g_surface_78` on
  `DDERR_SURFACELOST`, then `GetFlipStatus(DDGFS_ISFLIPDONE)` wait, then
  the same fps/tick accounting as `FlipPrimary`. VC6 hoists the
  `timeGetTime` IAT into esi.
- Recolour / highlight: Type-3 A/B/C as AB. Both are Hit+ClipLR with a
  per-pixel store of `pixel & g_sp_recolour` (recolour) or
  `(pixel & g_sp_recolour) >> 1` (highlight). Literal runs cannot use
  `rep movsw` (software loop); repeat runs still `and ax,[mask]` then
  `rep stosw`. Alignment nops after the esi load and `mov ebx, 3` are in
  the original. Top-skip mishandles primary code 1 (fall through after
  the `0xAAAAAAAA` test) — same dormant defect as HitL/HitR/Hit. `lea`
  of C is *before* the hi test (AB has test then lea).
- `PushRenderingStatusAndRelockVideoSurface`: build inclusive screen
  rect, push lock status, unlock if locked (Restore retry), then always
  IntersectRect + Lock + `GetTransparentColour` + `g_video_locked = 1`.
  Residual is the status-push interleaving with `rect.bottom` (original
  keeps h in edx, reuses ecx for `g_status_sp` after storing right) and
  `dwSize = 0x6c` sitting in the IntersectRect argument pushes. 21 of 70,
  252 B vs 251 B.
- `ShowCapacityOverlay`: six `AICat` lines
  `[%s x %d]: Cap %d  x %d%% = %.2f (Capped %d) = %.2f` then
  `Tot Capacity = %.2f (limit %d - %d) = %d`. Product `cap*pct` is clamped
  to `scale*100`; acc sums clamped; y walks `0x14`..`0x78` in edi;
  `fild`+`fmul kHundredth` is `product * kHundredth` (float 0.01 at
  `0x004ab518`). Residual is the strength-reduced cat cursor (original
  esi at `+0x18` / scale; ours anchors at `+0x14` / pct) and a frame
  delta (`0x1f8`/`0x210` vs `0x208`). 42 of 98.

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
- BltAdvisor / both RLE painters: hand-written `__declspec(naked)` +
  `__asm`, same as AB. `NAKED` stays off the signature line. BltAdvisor's
  mid-stream callee-saved pushes and the painters' rotating-mask + nops
  do not lower from C.
- Relock: `g_screencfg` is the same `0x004bcbf4` record `surface.c` calls
  `g_screen` (one name here). `dwSize` after `IntersectRect` in the
  source sinks into the argument pushes; writing it before (as Lock
  does) hoists it above the call. Screen pointer in eax, `xor ecx,ecx` /
  `mov cx,[eax]` for width, `xor edx,edx` / `mov dx,[eax+2]` for height,
  then eax is overwritten with `g_video_locked`.
- Capacity: `fild` + `fmul dword` is `n * kHundredth`, not
  `(double)n * kHundredth` (`fld` + `fimul`). y in edi via
  `do { ... y += 0x14; } while (y < 0x8c)`. Walking `names++` gives
  `add ebx, 4`.

## Extern-type / name notes

- `GetSimClock` declared `unsigned int` (sysstubs.c definition);
  `GetGameTimer` is `int`.
- `g_script_strings` at `0x007fe120` is also `g_hint_strings` in
  eventmake.c / softblit.c — same array, caller-side name kept from
  savechunks.c / movie3.c.
- `g_ai_cat[6]` at `0x00832810` is `g_map_ai.cat` (bigsim.c); named
  from the first category so this file does not depend on MapAI's header
  fields.
- `kHundredth` is the float at `0x004ab518` (0.01f).
- `g_sp_recolour` (`0x007fe998`) already named in softblit.c /
  bigrender.c; first used as a word operand here.
