# Scope AF — bubble-help / text-cache helpers

Branch `scope/AF` from main @ `f2ff6920`. Object prefix `/tmp/saf_`.
Worktree `.worktrees/scope-af`. File `LEGOLAND/bubblecache.c`.

## Status

**8 of 8 exact.**

## Per-function results

| Address | Name | Insns | Match | Audit | Marker |
| --- | --- | ---: | ---: | --- | --- |
| `0x00454a10` | `UnloadBubbleHelpGFX` | 92 | 100% | [OK] | FUNCTION |
| `0x00455a10` | `FindCachedEntryBySprite` | 25 | 100% | [OK] | FUNCTION |
| `0x00455a50` | `DrawCachedTextSprite` | 122 | 100% | [OK] | FUNCTION |
| `0x00455bb0` | `RasterizeText` | 74 | 100% | [OK] | FUNCTION |
| `0x00455c80` | `FindCachedTextBox` | 75 | 100% | [OK] | FUNCTION |
| `0x00455e50` | `PrintCachedText` | 49 | 100% | [OK] | FUNCTION |
| `0x00455fc0` | `VisitorBubbleHelp` | 275 | 100% | [OK] | FUNCTION |
| `0x00457970` | `FootprintClearanceTest` | 85 | 100% | [OK] | FUNCTION |

Names: `UnloadBubbleHelpGFX` from gamemain.c; `RasterizeText` / `PrintCachedText` from fpui2.c / popup2.c / money.c; `FindCachedTextBox` is the sized lookup money.c described (distinct from fpui3.c's `FindCachedText` at `0x00455d40`); `FindCachedEntryBySprite` / `DrawCachedTextSprite` from the function-sprite pair; `FootprintClearanceTest` from the brief; `VisitorBubbleHelp` from gameframe.c hit 0x306 (visitor + mood icon).

## Mechanics

- **UnloadBubbleHelpGFX**: inverse of text.c's `LoadBubbleHelpGFX`. Clears `g_bubble_loaded`, `LLIDB_UnLoadData`s `"SPEECH BUBBLE"` on a successful find, then kills the ten `mi_*.lls` / mood sprites one by one (unrolled; a counted loop collapses them).
- **FindCachedEntryBySprite**: walk `g_text_cache` by `+0x1c` (the function-sprite). Optional out-index. Volatile count in the latch.
- **FindCachedTextBox**: sized key `(w, h, format, ink, paper, font, text)`. Intrinsic `strcmp`. Distinct from `FindCachedText` (no w/h).
- **PrintCachedText**: `FindCachedTextBox` or `RasterizeText`, then `PrintSprite(entry->sprite, x, y, 0, 0)`.
- **RasterizeText**: `DBPrintf("Creating Cell (%d) %s\n")`, flush at 50 via `ExpireCachedText(1)`, append, `HeapAlloc_w`+`strcpy` the text, `CreateFunctionBasedSprite(DrawCachedTextSprite, w, h)`, `flags |= 0x40`.
- **DrawCachedTextSprite**: lookup by sprite; GetDC the draw surface (hdc reuses the dead sprite-arg slot); `GetNearestColor` of ink, fill paper, `DrawTextA`; `SetColorKey` to `GetNearestColour` of that COLORREF.
- **VisitorBubbleHelp**: `HTBubbleHelp` plus a 1-based mood-icon index. Non-zero icon leaves a 40-pixel pad; the box grows by `pad/2` and `PrintSprite`s `g_bubble_icons[icon]` at `box.x1 - pad/2`. Text is `DrawTextA`'d onto the video surface (cache is size-only). Width is `g_map->w`.
- **FootprintClearanceTest**: add ox/oy back, walk `[x, x+fp_w) × [y, y+fp_h)`. Fail on off-map / null / flags `0x8f8` / `extra != 0` unless `g_edit_object->inst` is the `PATH CONTROL` element. Empty footprint succeeds. Taken as `Pos` by value so the two incoming slots are one aggregate.

## Levers

- **FindCachedEntryBySprite / FindCachedTextBox**: `*(volatile int*)&g_text_cache_count` in the for-guard/latch — same free volatile as fpui3.c's `FindCachedText` and render5.c's `ExpireCachedText`. Cursor at `+0x1c` (sprite) and `+0x04` (h) respectively.
- **PrintCachedText**: four callee-saved hold `(f2, font, paper, ink)` across the lookup so the miss path does not reload them.
- **UnloadBubbleHelpGFX**: `xor esi, esi` shared zero; ten unrolled `if (sprite) { KillSprite; sprite = 0; }`. `g_bubble_loaded = 0` sinks between the FindElement pushes and the call.
- **RasterizeText**: `g_text_cache_count` declared `volatile` so the increment is a second load (`mov ecx,[count]; inc ecx`) rather than `inc` of the index eax. `int f = *(volatile int*)&format` so format wins eax and its store sits before strlen's `xor eax`; w/h then sink into the scasb setup. Paper/ink via an inline helper with the pair written ink-then-paper (CC05) so the inlined schedule lands paper-then-ink. `HeapAlloc_w(strlen(text)+1)` must stay as one expression (`not ecx / push ecx`); a named `n = strlen(text)` emits `dec/inc` and breaks the fold.
- **DrawCachedTextSprite**: hdc in the dead sprite-arg (`GetDC(..., (void**)&s)`). `p = s` first so ecx holds the sprite before `xor eax`. `rc.left = 0` then `memset(&rc.top, 0, 12)` (RC08) splits the four-zero web: eax stays the zero, one store is immediate, and `push edi` stays below the find (`test esi, esi`).
- **VisitorBubbleHelp**: HTBubbleHelp shape plus `pad` / `half = pad/2`. `rc.right = left + tw + half`. Icon at `g_bubble_icons[icon]` (`icon*4 + 0x8139e0`), x = `box.x1 - half`, y = `(box.y0 + box.y1)/2 - 0x14`. Block-scope miss-path DC; function-level hdc for the video `DrawTextA`. Width via `(int)g_map->w` (`xor` + `mov ax`).
- **FootprintClearanceTest**: `oy` loaded before `ox`; assign into `p.x` / `p.y` so ebx loads the x field (`mov ebx,[esp+0xc]`) and y0 lives in the y-arg slot; outer latch as `if (yy >= end) goto success; goto loop;` so fail sits between the jmp and success (`jge` + `jmp`, not inverted `jl`). The FR02 dead-arg floor (elem in the x-arg, 83i/240B, no `push ecx`) was two separate `int` parameters: every 4-byte address-taken out-local reused that dead slot. Taking the cell as one `Pos` by value makes the incoming slots one aggregate, so elem cannot reuse them and the original `push ecx` / `lea ecx,[esp+0x10]` falls out (fable-b two-ints→Pos, inverted: here the aggregate is the *parameter*, not a local). A live-across-call `ObjDef* ed` also forced `push ecx` but spilled ed into that slot and reloaded inst from the spill (94.2%, one extra store). ABI matches `sub_457970(pos.x, pos.y)` in gameframe.c.

## Extern-type divergences

- `CreateFunctionBasedSprite` declared `(fn, int w, int h)` here (caller pushes dwords). sprite2.c defines `(fn, short w, short h)`.
- `g_text_cache_count` is `volatile int` here (RasterizeText increment). fpui3.c / render5.c use a `*(volatile int*)&` cast on a plain `int`; pathmask.c already declares the global volatile.
- `FootprintClearanceTest` is `Pos` by value here; gameframe.c declares `sub_457970(int x, int y)`. Same two-dword cdecl push.

## Relocs / /W3

`relocs.py`: 0 MISMATCH, 1 UNRESOLVED (Unload's `"SPEECH BUBBLE"` literal — same as text.c's Load). `/W3` clean. `audit.py` PASS, eight `[OK]`.
