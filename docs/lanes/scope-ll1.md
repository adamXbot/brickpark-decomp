# Scope LL1 — logflume track / entrance helpers (inventory group 1)

Branch `scope/LL1`. File `LEGOLAND/logflume8.c`. Object prefix `/tmp/sll1_`.
Brief: `docs/SCOPE_LL1_logflume_track.md`.

## Status

| address | name | insns | % | audit | marker |
| --- | --- | ---: | ---: | --- | --- |
| 0x00409620 | LFPiece_AttachN | 25 | 100 | [OK] | FUNCTION |
| 0x00409680 | LFPiece_AttachS | 25 | 100 | [OK] | FUNCTION |
| 0x004096e0 | LFPiece_AttachE | 26 | 89 | WIP | WIP (ESCAPES) |
| 0x00409740 | LFPiece_AttachW | 25 | 100 | [OK] | FUNCTION |
| 0x004097a0 | LFTrack_ReshapeNeighbours | 170 | 100 | [OK] | FUNCTION |
| 0x00409a50 | LFPiece_MakeStraight | 12 | 75 | WIP | WIP |
| 0x00409a90 | LFTrack_AttachNeighbours | 49 | 100 | [OK] | FUNCTION |
| 0x00409b10 | LFRoute_OrientPair | 37 | 100 | [OK] | FUNCTION |
| 0x0040a010 | LFRoute_SplicePair | 46 | 100 | [OK] | FUNCTION |
| 0x0040a080 | LFTrack_SpliceNeighbours | 45 | 100 | [OK] | FUNCTION |
| 0x0040a0f0 | LFNb_DetachN | 38 | 76 | WIP | WIP |
| 0x0040a160 | LFNb_DetachE | 37 | 55 | WIP | WIP |
| 0x0040a1d0 | LFNb_DetachS | 37 | 81 | WIP | WIP |
| 0x0040a230 | LFNb_DetachW | 40 | 89 | WIP | WIP |
| 0x0040a2a0 | LFTrack_RedrawNeighbours | 23 | 100 | [OK] | FUNCTION |
| 0x0040b290 | LFPiece_DrawAnim | 93 | 100 | [OK] | FUNCTION |
| 0x0040ce20 | LFGeom_FillNeighbours | 72 | 100 | [OK] | FUNCTION |
| 0x0040cf10 | LFGeom_ProbeNeighbours | 7 | 100 | [OK] | FUNCTION |
| 0x0040cf30 | LFNb_Count | 10 | 100 | [OK] | FUNCTION |
| 0x0040cf50 | LFNb_KeepRun | 15 | 100 | [OK] | FUNCTION |
| 0x0040cf80 | LFNb_FirstRun | 14 | 100 | [OK] | FUNCTION |
| 0x0040cfa0 | LFNb_DropFull | 15 | 100 | [OK] | FUNCTION |

**16 / 22 exact.** `audit.py` PASS (0 extent failures), `relocs.py` zero MISMATCH (4 UNRESOLVED, all `$L62x` jump-table labels inside `LFTrack_ReshapeNeighbours`), `/W3` clean.

## Names

Callers already named `LFTrack_ReshapeNeighbours` (0x004097a0), `LFTrack_RedrawNeighbours` (0x0040a2a0) and `LFPiece_DrawAnim` (0x0040b290). New names from body evidence:

- `LFGeom_FillNeighbours` / `LFGeom_ProbeNeighbours` — set-piece geom probe that zeros `g_lf_nb` and steps one flume cell from each published end (flag bits 1/2/4/8 = N/E/S/W). Twin of `LFTrack_FillNeighbours` / `LFTrack_ProbeNeighbours`.
- `LFNb_Count` / `LFNb_KeepRun` / `LFNb_FirstRun` / `LFNb_DropFull` — loop forms of the unrolled TRACK helpers in logflume2.c (`LFTrack_CountNeighbours` reads the GLOBAL; `LFNb_Count` walks the pointer it is handed).
- `LFPiece_AttachN/E/S/W` — reshape a neighbour that is gaining a connection on that side (4→3 endpoint, 3→1 straight or 2 corner).
- `LFPiece_MakeStraight` — force kind=1; even dir→0, odd dir→1.
- `LFTrack_AttachNeighbours` — per occupied mask bit, straighten our end and attach the neighbour.
- `LFRoute_OrientPair` / `LFRoute_SplicePair` / `LFTrack_SpliceNeighbours` — two-piece form of `LFRoute_Join` + `LFPiece_LinkBefore`/`LinkAfter`.
- `LFNb_DetachN/E/S/W` — inverse of Attach* (used by RedrawNeighbours).

## Mechanics (spec for a runtime)

- **Geom probe** (`LFGeom_FillNeighbours`): `dx = footprint.v[2]-v[0]`, `dy = v[3]-v[1]`. For each flag bit, build a `BPos` from the geom's `ends[dir]` stepped by one cell (N: y-dy, E: x+dx, S: y+dy, W: x-dx) and `LFTrack_FindPiece`. Then `*out = g_lf_nb`.
- **Attach** on an isolated piece (kind 4) makes an endpoint whose dir is the FREE end (N→0, E→1, S→2, W→3). A second connection opposite the existing end becomes a straight (kind 1); an adjacent one becomes a corner (kind 2) with a packed dir.
- **ReshapeNeighbours** (the 170i body): `NeighbourMask(nb)`, bail if `piece` is null. If `piece->sub` is null, a sparse switch on the mask sets kind (single end→3, straight→1, corner→2). A second sparse switch always runs: set dir (only when `!sub`) and `Attach*` every occupied neighbour. Singles: N dir=2, S dir=0, E dir=3, W dir=1. Straights: NS dir=0, EW dir=1. Corners: NE 0, SE 1, SW 2, NW 3.
- **RedrawNeighbours** calls `NeighbourMask` and discards it, then `Detach*` on all four slots if `piece` is non-null (the callees tolerate null).
- **DrawAnim**: `flag==0` walks `end_a` via `prev`, else `end_b` via `next`. For each square, every boat on `piece->run` that `LFBoat_IsOnPiece` accepts is `LFBoat_Draw(..., 1)`. Overlay frames are `{dx, dy, sprite}` (12 B); `dy` is the span. After the walk (or if there is no end) the current frame's sprite is `PrintSprite`'d at (x,y) with mode 0.
- **OrientPair**: same degenerate `if (oj) Reverse(a); else Reverse(a);` as `LFRoute_Join` when `a` is not on the cursor; both-ok prints `g_lf_both_msg` and returns. `SplicePair` orients when both fwd or both back, then `LinkAfter` if `!a->fwd` else `LinkBefore`.

## Residuals (WIP)

- **0x00409a50 MakeStraight** — first diverging store: case 0/2 `mov [eax+18], 1` vs original `mov [eax+18], edx`. Fall-through of cases 1/3 into default is correct; the hoisted `edx=1` is not kept live into the isolated 0/2 arm. Tried named `one`, `piece`/`p` split, volatile store, inline helper.
- **0x004096e0 AttachE** — dir==0 arm CSEs the two stores of 2 into `mov ecx,2 / store / store` (copying the first arm's named-`one` pattern). Original uses two immediates, so later labels shift (ESCAPES). Tried `3-1`, volatile stores.
- **0x0040a0f0 DetachN** — need `push esi` before the null test and dir in esi while kind stays in ecx. VC6 jump-threads `kind==3 && dir!=0` to the epilogue (kind is proven 3) instead of falling into the dead `cmp ecx,1`. Goto form scrambled block order.
- **0x0040a160 DetachE** — `three` should be esi and `one=1` scheduled between `cmp kind,3` and `jne` (mov does not clobber flags). Last corner arm missing (jump-thread).
- **0x0040a1d0 DetachS** — same esi-for-dir / jump-thread as DetachN. Instruction count already matches (37/37).
- **0x0040a230 DetachW** — kind==1 dir compare must be `push edi / mov edi,[eax+1c] / cmp edi,edx / pop edi`, not a memory compare. Closest of the four detaches (23 mismatch).

## Levers

- **Named pointer copy advances rotation (RA01).** `LFPiece* p = piece; int kind = p->kind;` puts kind in edx (original). A rename of the parameter alone left kind in ecx.
- **Dead store of dir kept via volatile.** AttachS (`dir==0`: `p->dir = dir`) and AttachW (`dir==1`: `p->dir = dir`) are stores of a value just loaded from the same field. `*(volatile int*)&p->dir = dir` preserves `mov [eax+1c], ecx`.
- **NeighbourMask result discarded.** RedrawNeighbours calls it then tests `piece`, not the mask. Reproduced.
- **DrawAnim overlay offset is function-inner.** `int off = 0` must be declared inside `if (walk)` so its zero store lands after `push edi` at `[esp+0x14]`. Function-scope `off` wrote `[esp+0x10]` (98.9%).
- **Sparse switch on mask.** ReshapeNeighbours is two `switch (mask)` with `cmp esi, 0x4f / ja` + byte-index table. Case/block order of the second switch is 1, 0x10, 4, 0x40, 0x11, 0x44, 5, 0x14, 0x50, 0x41.
- **OrientPair degenerate reverse.** `if (!oi) { if (oj) Reverse(a); else Reverse(a); }` leaves a dead `test eax,eax` between the push and the call.

## Extern-type notes

None that diverge from logflume.c / logflume2.c. `LFTrack_FindPiece` is `const BPos*` as in logflume2.c. `PrintSprite` matches logflume.c. `LFBoat_Draw` is `(LFBoat*, LFPiece*, int)` as in logflume6.c.
