# Scope AE — high-level AI table handlers

NEW-FUNCTION `LEGOLAND/highlevelai.c`. Six of seven live group-13 handlers
are `audit [OK]`; plan 0x0d is an honest WIP. DEAD 0x004511e0..0x00451550
not touched.

## Table

| address | name | insns | audit | marker |
| --- | --- | ---: | --- | --- |
| 0x00450a40 | `SelectBloke` | 20 | [OK] | `FUNCTION` |
| 0x0044fe10 | `Visitor_WalkToEntrance` | 43 | [OK] | `FUNCTION` |
| 0x00450250 | `Visitor_WaveThenResume` | 77 | [OK] | `FUNCTION` |
| 0x00450330 | `Worker_ResumeIdle` | 43 | [OK] | `FUNCTION` |
| 0x004503a0 | `FaceClassRect` | 87 | [OK] | `FUNCTION` |
| 0x00450450 | `Visitor_FaceAndMark` | 48 | [OK] | `FUNCTION` |
| 0x0044fe80 | `Visitor_ReserveCafeBrolly` | 337 | no (76.7% matchfull; first diverge i19) | `WIP-FUNCTION` |

Reached by: 0x004b83c4 plan 0x17, 0x004b839c plan 0x0d, 0x004b83a0 plan 0x0e,
0x004b83b8 plan 0x14, 0x004b83a4 plan 0x0f; `FaceClassRect` is the 0x00450450
callee; `SelectBloke` is gameframe 0x00458ee0 (icon types 0x306..0x308).

## Names

- `SelectBloke` — hover/select a map person. Flag 0x28 already set is a
  no-op. Person kind 2/3 (gardener/mechanic) stamps `last_job` with
  `g_sim_frame`; anyone else ORs flag 8 and starts plan 0x0e.
- `Visitor_WalkToEntrance` — plan 0x17: line to the cached entrance tile
  (0x0066b460/0x0066b464, no +0x80 bias, low state 0xf) then plan 6.
- `Visitor_WaveThenResume` — plan 0x0e: face 4, visitor kind 1 plays anim 2
  (flag 0x100), then state 0xd, then plan 0x10/0x11/6 by kind.
- `Worker_ResumeIdle` — plan 0x14: face 4, state 0xd, then gardener/mechanic
  idle (kind 2 → 0x10, kind 3 → 0x11). Other kinds unchanged.
- `FaceClassRect` — writes +0x72 from `world>>8` vs class rect +0x3c biased
  by dest +0x2c/+0x30 as raw ints (no shift). Tile/24.8 mix is original.
- `Visitor_FaceAndMark` — plan 0x0f: face selected class, wait
  `(rand()&0x1f)+10`, increment that class's visit counter, plan 6.
- `Visitor_ReserveCafeBrolly` — plan 0x0d: CAFE BROLLY at 0x006661c0.

## `Visitor_ReserveCafeBrolly` residual

First diverge i19: original `test dl,1` / `je found`; ours `mov ebx,1` /
`test bl,dl`. ebx is already pushed for case 3's `CellAt` width temp, and
VC6 hoists the reservation mask into it. The empty-walk arm should be
`jne again` plus an inline `NewLongTermAction(6)` (one-call tail copy);
ours `je`s to the shared case-5 tail. SuggestNextMove `lea eax/ecx/edi`
and `dest.y = (by + stand_y)<<8` match once aligned. Same insn count
(337) but 944B vs 928B. Ruled out: one-`Pos` entrance global (wrong
relocs), `switch(act)` (spills to `[esp+8]`).

## Levers

- **`act = b->action; switch (b->action)`** (goldrush). An
  `unsigned char act = b->action; switch (act)` homes the byte in the
  dead `b` slot (`mov [esp+8],cl` / reload) and costs 2 extra insns on
  `Worker_ResumeIdle` and `Visitor_FaceAndMark`. Assign the local, switch
  on the field: `mov cl,[esi+0x60]; mov eax,ecx; and eax,0xff`. Case 1
  then `inc cl` / `b->action = (unsigned char)(act+1)`. Evidence:
  42/44 → 43/43 and 47/49 → 48/48.
- **Jump-table plan 0x0e** uses the same assign-then-switch-on-field so
  the index stays in ecx (`mov al,[action]; mov ecx,eax; and ecx,0xff;
  cmp ecx,3; ja; jmp [ecx*4+0x45031c]`) and case 2 can `inc al`. Case 3
  is `switch (kind)` with default (plan 6) inline, then 0x11, then 0x10
  (`sub eax,2; je k2; dec eax; je k3; push 6`).
- **Entrance tile is two ints**, `g_entrance_tile_x` @ 0x0066b460 and
  `g_entrance_tile_y` @ 0x0066b464, not `extern Pos`. The original
  relocates to those VAs separately. A `Pos t` with `t.y` then `t.x`,
  `t.x <<= 8`, `t.y <<= 8`, `b->target = t`, `CalcMoveLine(b->world, t,
  b->path)` puts y in ecx and x in eax (dest symbol `.x`). Evidence:
  35/43 → 43/43.
- **`FaceClassRect` load order**: `dest.x`, `world.x`, `world.y`,
  `dest.y`. Swapping dest.x after world.x is one swapped `mov` (86/87).
- **`SelectBloke`**: keep the flags word in `cx`, `if (kind >= 2 &&
  kind <= 3)` success-inline (`jl`/`jg` to the OR-8 / plan 0x0e arm).
- **Compare-chain plans 0x14 / 0x0f / 0x17** are written `case 0,1,2`
  (or `0,1`); the last case is the inline arm.

## Relocs / W3

`/W3` clean. `relocs.py`: 0 MISMATCH on the six `FUNCTION` bodies.
UNRESOLVED: WaveThenResume i7 jump table `$L418` (literal / local code
symbol; fine). WIP 0x0044fe80 is not in that gate.

## Extern-type notes

`IncrementBlokeCounter(ObjClass*, int)` here vs `Team*` in sweep3.c —
caller-side only; the push is a pointer. `GetFirstObjectMatching` /
`GetNextObjectMatching` match objmap.c (`Cell*`, `void*`).
`SuggestNextMove(Pos*, Pos*, Pos*)` matches bnvmove.c. `NewDirForAction`
takes `unsigned char`.
