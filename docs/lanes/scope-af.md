# Scope AF — bubble-help / text-cache helpers

Branch `scope/AF` from main @ `f2ff6920`. Object prefix `/tmp/saf_`.
Worktree `.worktrees/scope-af`. File `LEGOLAND/bubblecache.c`.

## Status

**4 of 8 exact.** Four WIPs with a diagnosed residual each.

## Per-function results

| Address | Name | Insns | Match | Audit | Marker |
| --- | --- | ---: | ---: | --- | --- |
| `0x00454a10` | `UnloadBubbleHelpGFX` | 92 | 100% | [OK] | FUNCTION |
| `0x00455a10` | `FindCachedEntryBySprite` | 25 | 100% | [OK] | FUNCTION |
| `0x00455a50` | `DrawCachedTextSprite` | 122 | 87.7% | WIP | WIP (edi zero-web) |
| `0x00455bb0` | `RasterizeText` | 74 | 87.8% | WIP | WIP (store schedule) |
| `0x00455c80` | `FindCachedTextBox` | 75 | 100% | [OK] | FUNCTION |
| `0x00455e50` | `PrintCachedText` | 49 | 100% | [OK] | FUNCTION |
| `0x00455fc0` | `VisitorBubbleHelp` | 275 | 63.7% | WIP | WIP (first draft) |
| `0x00457970` | `FootprintClearanceTest` | 85 | 90.2% | WIP | WIP (dead-arg home) |

Names: `UnloadBubbleHelpGFX` from gamemain.c; `RasterizeText` / `PrintCachedText` from fpui2.c / popup2.c / money.c; `FindCachedTextBox` is the sized lookup money.c described (distinct from fpui3.c's `FindCachedText` at `0x00455d40`); `FindCachedEntryBySprite` / `DrawCachedTextSprite` from the function-sprite pair; `FootprintClearanceTest` from the brief; `VisitorBubbleHelp` from gameframe.c hit 0x306 (visitor + mood icon).

## Mechanics

- **UnloadBubbleHelpGFX**: inverse of text.c's `LoadBubbleHelpGFX`. Clears `g_bubble_loaded`, `LLIDB_UnLoadData`s `"SPEECH BUBBLE"` on a successful find, then kills the ten `mi_*.lls` / mood sprites one by one (unrolled; a counted loop collapses them).
- **FindCachedEntryBySprite**: walk `g_text_cache` by `+0x1c` (the function-sprite). Optional out-index. Volatile count in the latch.
- **FindCachedTextBox**: sized key `(w, h, format, ink, paper, font, text)`. Intrinsic `strcmp`. Distinct from `FindCachedText` (no w/h).
- **PrintCachedText**: `FindCachedTextBox` or `RasterizeText`, then `PrintSprite(entry->sprite, x, y, 0, 0)`.
- **RasterizeText** (WIP): `DBPrintf("Creating Cell (%d) %s\n")`, flush at 50 via `ExpireCachedText(1)`, append, `HeapAlloc_w`+`strcpy` the text, `CreateFunctionBasedSprite(DrawCachedTextSprite, w, h)`, `flags |= 0x40`.
- **DrawCachedTextSprite** (WIP): lookup by sprite; GetDC the draw surface (hdc reuses the dead sprite-arg slot); `GetNearestColor` of ink, fill paper, `DrawTextA`; `SetColorKey` to `GetNearestColour` of that COLORREF. GDI body matches; prologue does not.
- **FootprintClearanceTest** (WIP): add ox/oy back, walk `[x, x+fp_w) × [y, y+fp_h)`. Fail on off-map / null / flags `0x8f8` / `extra != 0` unless `g_edit_object->inst` is the `PATH CONTROL` element. Empty footprint succeeds. Body matches; `elem` is homed in the dead x-arg (original `push ecx`).
- **VisitorBubbleHelp** (WIP): `HTBubbleHelp` plus a 1-based mood-icon index. Non-zero icon leaves a 40-pixel pad and `PrintSprite`s `g_bubble_icons[icon]`. Text is `DrawTextA`'d onto the video surface (cache is size-only). First draft.

## Levers

- **FindCachedEntryBySprite / FindCachedTextBox**: `*(volatile int*)&g_text_cache_count` in the for-guard/latch — same free volatile as fpui3.c's `FindCachedText` and render5.c's `ExpireCachedText`. Cursor at `+0x1c` (sprite) and `+0x04` (h) respectively.
- **PrintCachedText**: four callee-saved hold `(f2, font, paper, ink)` across the lookup so the miss path does not reload them.
- **UnloadBubbleHelpGFX**: `xor esi, esi` shared zero; ten unrolled `if (sprite) { KillSprite; sprite = 0; }`. `g_bubble_loaded = 0` sinks between the FindElement pushes and the call.
- **RasterizeText**: `g_text_cache_count` declared `volatile` so the increment is a second load (`mov ecx,[count]; inc ecx`) rather than `inc` of the index eax. Residual: stores through `e->` are reordered (format/h, paper/ink); `*(volatile int*)&e->format` and `&e->paper` force those two first but the companion load stays in the wrong register. Ruled out: width/height named locals (pull format/w/h above the increment).
- **DrawCachedTextSprite**: hdc in the dead sprite-arg (`GetDC(..., (void**)&s)`). GDI sequence and `SetColorKey` tail match at 107/122. Residual: four `rc = 0` stores form an edi zero-web that pulls `push edi` above the find (`cmp esi, edi` vs `test esi, esi`). Ruled out: `int z = 0` (edi still), inner-scope brush/oldfont (push does not sink), three-zeros + volatile bottom (edi still).
- **FootprintClearanceTest**: `oy` loaded before `ox`; assign into `y` (and `x`) so y0 lives in the y-arg slot; outer latch as `if (yy >= end) goto success; goto loop;` so fail sits between the jmp and success (`jge` + `jmp`, not inverted `jl`). Residual: `void* elem` (function-level, block-scope, 1-pointer aggregate, 1-element array, address-taken wrapper) all home in the dead x-arg — no `push ecx`. FR02. Size 83i/240B vs 85i/242B.
- **VisitorBubbleHelp**: extra arg is a 1-based index into the ten bubble faces via base `0x008139e0` (`g_bubble_state - 4`). Pad `0x28` when icon != 0. Screen width is `g_map->w` (u16 at +0 of `0x004bcbf4`), not fpui2.c's `g_screen->width`. First draft 177/278; esi vs ebx as the zero register.

## Extern-type divergences

- `CreateFunctionBasedSprite` declared `(fn, int w, int h)` here (caller pushes dwords). sprite2.c defines `(fn, short w, short h)`.
- `g_text_cache_count` is `volatile int` here (RasterizeText increment). fpui3.c / render5.c use a `*(volatile int*)&` cast on a plain `int`; pathmask.c already declares the global volatile.

## Relocs / /W3

`relocs.py`: 0 MISMATCH, 1 UNRESOLVED (Unload's `"SPEECH BUBBLE"` literal — same as text.c's Load). `/W3` clean. `audit.py` PASS, four `[OK]`.
