# Scope LL20 — partials wave D: the two renderview.c bodies (2026-09-08)

Branch `scope/LL20` from `origin/main` `8a2c0a8b`. PARTIAL scope over the two
worst rows of HANDOFF §6B (brief: `docs/SCOPE_LL20_partials_renderview.md`).
Object prefix `/tmp/sll20_`. Time-boxed; the previous attempt was killed by a
session limit before committing anything.

## Status

| address | name | before | after | audit | marker |
| --- | --- | --- | --- | --- | --- |
| 0x0045b180 | RenderView | 381/903 strict, first 67, 2893/2880 B | unchanged — retired, mechanism traced to its root | PASS, 0 OK (file has no exact bodies) | WIP |
| 0x004567a0 | RenderFullMap | 827/1161 strict, first 0, 4216/4225 B, frame 0xf4 | unchanged — retired; ONE lever found that moves index 0 (frame exact, pool exact) but regresses strict | PASS, 0 OK | WIP |

Nothing committed to either body; both notes carry a dated LL20 paragraph.
Gates at the tip: `audit.py LEGOLAND/renderview.c` PASS, `relocs.py` zero
MISMATCH, `/W3` silent.

## Method

The prior rounds' scratch tooling was never committed, so this lane wrote two
small scripts (kept in the session scratchpad, described here so they can be
rebuilt in minutes):

- `t.py` — compile a variant `.c` to `/tmp/sll20_<tag>.obj`, trim to the
  original's extent with `match.compiled_body`, print strict / register-blind
  / offset-blind / both-blind index-for-index mismatch, first diverging
  index, bytes, frame size and the indices of the callee-saved pushes; `run()`
  applies textual edits to the committed file and scores the result.
- `fm.py` — a control-flow-aware frame map: pushes of callee-saved registers
  (first push of each popped register) tracked separately from argument
  depth; argument depth reset to 0 at every branch target; `__stdcall`
  imports and COM calls pop their own arguments (arg counts keyed by the
  ORIGINAL's import address and zipped by order onto our body's indirect
  calls, including a `call reg` of a cached import). Prints slot -> (refs,
  first index) for both sides and can dump the instructions touching a
  frame range.

## RenderView 0x0045b180 — the qx-first cascade, read off the object

The committed order (qy/ry first) keeps the split prologue and 0..66 exact; the
original's order (qx/rx first, limits after the switch) has been known since
round w7 to break the split. This round built that order and read what breaks:

| region | original | qx-first variant |
| --- | --- | --- |
| 64..87 | sx ebp, th edi, tw ebx, `idiv ebx` x2 | shape-identical; sx ebx, tw ebp (one swap) |
| 88..101 | th reloaded into ebx, rx spilled then reloaded | th a memory divisor at frame+0xc (original +0x0); rx in ebx |
| gather loop 248..253 | `count` in memory (`mov edx,[count] / inc / mov`), px in ebx | `count` in ebx (`inc ebx`), px in ecx+memory |
| class walk 352 | `test esi,esi` / `test eax,eax` | `xor edi,edi` then `cmp esi,edi` / `cmp eax,edi` |
| queue head 378 | two scratch zeros (`xor edx,edx` / `xor ecx,ecx`) | one callee-saved zero (edi) for all eight zero uses |
| tail 863 | `mov edi,[count] / test edi,edi`, n in edi, pp in esi | `xor edi,edi`, count in esi, n copied to ebx |
| pops | ebp/ebx at 866/868, edi/esi at 889/891 | all four at 890..893 |
| pushes | esi@5 edi@7 ebx@58 ebp@60 | ebx@5 ebp@7 esi@15 edi@21 |

Measures: strict 519 / rb 479 / ob 476, first 5, 2875 B, frame 0x2f90. The
chain is: qx-first -> th spilled / count enregistered in the gather loop ->
the entry const-0 web (edi) extended through the class walk, the queue head
and the tail (three rematerialisations) -> n displaced into ebx -> ebx live
to the epilogue -> the {ebx,ebp} save pair cannot bracket [58,866] -> both
pushes at the entry. That is LEVERS RA09's RenderView sentence, now with the
first link (the gather loop's count/px flip) identified.

Fourteen spellings on the qx-first body, all byte-identical (519) unless
noted: `while (--count)` (533); `if (count > 0) { n = count;` (533); a
volatile count reload (614, frame shrinks to 0x2f8c); volatile
`g_show_cursor`; volatile `cell->obj` (527); the LL10 pin `if (count) ;`;
block-scope n/pp; `g_sort_count = 0` inside the non-null arm (520, exactly
2880 B); the same plus the count loop (534); `visible[count] = owner;
count++;`; px declared ahead of count; px block-scoped. The zero-web
extension is not reachable from the tail, the counter or the sort-count
placement; it is decided together with the geometry allocation. The LL14
count model does not apply (no oversubscribed named candidates: count has 7
appearances and px 3 in both orders, yet the original enregisters px).

Frame map (push-depth resolved): 40 slots both sides, identical from +0x34
up; the low thirteen carry 134 references against the original's 130
(orig 20 14 21 14 9 9 7 7 7 6 6 6 4; ours 22 21 14 13 9 9 7 7 7 7 7 5 6).
Retired at 381.

## RenderFullMap 0x004567a0 — index 0 is two allocation facts

### `push ebp` vs `push ebx` (index 1)

The register is the whole zero web AND pass 2's outer counter y: the original's
323 `xor ebp,ebp` is both the zero for `cmp word ptr [ecx+0x16],bp` and
`y = 0` (347 `[ecx+ebp*4]`, 430 `inc ebp`); pass 2's x is ebx (329). Ours is
the identical web with ebx and ebp swapped. Zero-use counts through pass 2 are
equal (25 each), and over the web's range neither ebx nor ebp is otherwise
written in either body (first write of the other register is `set` at 216 in
both). So this is a pure ebx/ebp tie-break in VC6's colouring order between
the zero/y node and x. Inert: `int y, x` declaration order; block-scope
`int x, y` around pass 2; pass 2 on fresh names `x2`/`y2` (all 827,
byte-identical).

### Frame 0xf4 vs 0xf8 (index 0) — and the lever that fixes it

The frame map says the pool differs by +20 and -16, not by one slot:

| | original | committed body |
| --- | --- | --- |
| below the pool | e_track_h at +0xa4, link at +0xa8 | three ElemID homes and link at +0xac..+0xb8 |
| sd (24 B) | +0xac | +0xbc |
| Pos slots | +0xc4 roads, +0xcc single sprite, +0xd4 mark, +0xdc p1 | +0xd4 single, +0xdc roads, +0xe4 ILF/p1, +0xec mark |
| pass-4 Cell copy | +0xe4..+0xf8 (525 `lea edi,[esp+0xf4]`) | shares the pass-1/2 slot (521 `lea edi,[esp+0x6c]`) |

The `CellMarkTest(c)` by-value helper buys nothing in the committed body: VC6
forwards `c`'s fields into it and emits no inline temporary.

**Measured lever (not committed):** move the ENTIRE pass-4 loop body into one
`static __inline` helper that takes the cell by value, and call it with
`*chain`:

```c
static __inline void FullMap_ChainCell(Cell c, BPos bpos, int scale_x, int scale_y,
                                       Elem* e_track, Elem* e_track_h, Elem* e_track_h0,
                                       Elem* e_track_hp, Elem* e_castle, Elem* e_roads,
                                       Sprite* s_blob, Sprite* s_stick, Sprite* s_lights,
                                       void* pen, TileBounds* tbp, Pos* tilep, Pos* gtbp,
                                       SpriteDesc* sdp)
{   /* the committed loop body verbatim, with tb./tile./gtb./sd. spelled
       through the pointers, every `continue` a `return`, and desc/ilf/slot/
       def/road/lox/loy/i/mx/my/x0/y0 as helper locals */ }

    for (chain = GetFirstRenderObject(); chain; chain = GetNextRenderObject(chain)) {
        bpos = chain->base;
        FullMap_ChainCell(*chain, bpos, scale_x, scale_y, e_track, e_track_h, e_track_h0,
                          e_track_hp, e_castle, e_roads, s_blob, s_stick, s_lights, pen,
                          &tb, &tile, &gtb, &sd);
    }
```

| measure | committed | by-value helper |
| --- | --- | --- |
| frame | 0xf4 | **0xf8** |
| first diverging index | 0 | **1** (`push ebx`) |
| pool | see above | **exactly the original's**: sd +0xac, Pos +0xc4/+0xcc/+0xd4/+0xdc in the original's order, Cell +0xe4; 521..526 `lea edi,[esp+0xf4] / rep movsd / mov eax,[esp+0x100]` |
| `rep movsd` count | 3 | 3 |
| frame references | 258 vs 260 | 260 = 260 |
| strict / rb / ob | 827 / 763 / 758 | 864 / 814 / 786 |
| length | 1161 | 1164 (ESCAPES: the trimmed body hides a jump target) |
| block layout | sd fill before the join, track arm mid-body | sd fill before the join, **track arm laid out last** (876..1121, after the ILF loop) |

So an inlined by-value body is the first construct in eleven rounds that
reproduces the original's pool, and the first that moves the block-layout bit
at all — but it moves the track arm the wrong way, which is what costs the
strict count. Two respellings of the helper are byte-identical to it: the
track arm as `} else {` around the sprite path instead of `return`, and the
mark test as a direct `c.flags` expression. Not committed: strict, rb and ob
all regress and the body is three instructions long. Recorded as the next
lane's starting point — the frame is now known to be reachable, and the
layout question becomes "which arm shapes inside an inlined body put the
track arm between the sd fill and the single-sprite tail".

## Levers for LEVERS (candidates)

- **A by-value aggregate parameter gets its inline-expansion temporary only
  when the inlined body actually needs the copy**: `CellMarkTest(c)` on an
  already-copied local is forwarded away (no temp, no pool slot); the whole
  loop body as one by-value inline helper keeps the copy live across the body
  and lands it at the top of the frame exactly as the original (RenderFullMap).
- **Inlining a loop body changes VC6's block layout**: the same CFG with
  `continue` -> `return` inside a `static __inline` helper laid the exiled
  track arm last instead of mid-body — statement order never moved it, an
  inline boundary did (RenderFullMap).
- **A constant-zero web's extension is decided with the surrounding
  allocation, not at its uses**: fourteen respellings of the zero's tail and
  loop-head uses were inert while the geometry order alone switched the web
  between "entry only" and "entry + three rematerialised segments"
  (RenderView).
