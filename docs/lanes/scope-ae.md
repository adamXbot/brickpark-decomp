# Scope AE — high-level AI table handlers

NEW-FUNCTION `LEGOLAND/highlevelai.c`. All seven live group-13 handlers
are `audit [OK]` (7/7). DEAD 0x004511e0..0x00451550 not touched.

## Table

| address | name | insns | audit | marker |
| --- | --- | ---: | --- | --- |
| 0x00450a40 | `SelectBloke` | 20 | [OK] | `FUNCTION` |
| 0x0044fe10 | `Visitor_WalkToEntrance` | 43 | [OK] | `FUNCTION` |
| 0x00450250 | `Visitor_WaveThenResume` | 77 | [OK] | `FUNCTION` |
| 0x00450330 | `Worker_ResumeIdle` | 43 | [OK] | `FUNCTION` |
| 0x004503a0 | `FaceClassRect` | 87 | [OK] | `FUNCTION` |
| 0x00450450 | `Visitor_FaceAndMark` | 48 | [OK] | `FUNCTION` |
| 0x0044fe80 | `Visitor_ReserveCafeBrolly` | 337 | [OK] | `FUNCTION` |

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

## `Visitor_ReserveCafeBrolly` — closed (audit `[OK]`, 337i/928B)

The walk closed first (i17 / empty-walk, 92.1% → 96.5%): `for` latch
on GetNext plus ordinary `reserved = flag0 & 1`, and case 2's obj
compare split from the flags word so cafe is `mov eax,moffs32`.

The last residual was the two SuggestNextMove result arms: original
loads y then x, stores target.x, pushes path/y/x, loads world.y
(`[edi+4]`), **then** stores target.y, then loads world.x. With
`Pos* world = &b->world; Pos* out = &leg;` pointer locals and
`CalcMoveLine(*world, *out, b->path)` the target.y store sank one
slot early (world.x loaded before world.y) — 96.5%, and no store-order
or volatile spelling moved it (dual-volatile obj+flags 91.5%; `for` +
vol flags 87.4%; vol cell pointer 84.2%; `Pos t = *out` 95.9%; dest.y
after the call 91.7%; named-y delay 93.4%; volatile target.y 95.4%;
`from.x/.y` locals 92.5%).

Fix: drop the pointer locals and mirror the exact `Garderner_Repair`
(bigsim.c 0x0049ba10) spelling — `SuggestNextMove(&b->world, &b->dest,
&leg)`, `b->target.x = leg.x; b->target.y = leg.y;
CalcMoveLine(b->world, leg, b->path)`. That interleave is
byte-identical to Garderner_Repair's two FindPathLeg arms. matchfull
97.7% (the 8 remaining "X" lines are `.rdata` jump-table bytes past
`ret`); `audit.py` mismatch=0.

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
- **Map `Cell` is 0x14 bytes.** `CellAt` is `&g_map_rows[y][x]` with
  stride 20 (`lea [eax+eax*4]` / `lea [ecx+edx*4]`). A 16-byte cell
  (missing `pad0e[6]`) is `shl eax,4` and costs the cafe-brolly body
  ~11 matchfull points.
- **Split the reservation bit from the branch.** `reserved = f & 1;
  cls = e->cls; if (!reserved)` plus `*(unsigned char volatile*)`
  on the flags byte kills the loop-invariant `mov ebx,1` and keeps
  `test dl,1`. Volatile flags also schedules the flags load before
  `cell->obj`; dual-volatile obj+flags flips that pair back and
  moves cls before the test.
- **for-latch GetNext, ordinary flags.** After a separate GetFirst
  null `break`, `for (; cell; cell = GetNextObjectMatching(...))`
  with ordinary `f = cell->f.flag0; reserved = f & 1` emits obj then
  flags, `test dl,1`, cls, `jne again` and an inline plan-6 copy.
  Volatile flags is not needed once GetNext is the latch — the
  do-while peel was what hoisted `1` and swapped the loads. Evidence:
  92.1% → walk-exact.
- **Case 2: compare obj, then load the flags word.** Combined
  `obj != cafe || !(fl & 0x80)` puts cafe in edi (6-byte) and hoists
  `mov ax,[ecx+0xc]` above the cmp. Split so cafe is `mov eax,moffs32`.
- **Pointer locals into a by-value struct call sink a sibling store.**
  `Pos* world = &b->world; Pos* out = &leg; b->target.y = out->y;
  CalcMoveLine(*world, *out, path)` emits the target.y store BEFORE
  the `world.y` load (world.x then loaded first). The direct spelling
  `b->target.y = leg.y; CalcMoveLine(b->world, leg, path)` interleaves
  `mov eax,[edi+4]; mov [esi+0x28],ecx; mov ecx,[edi]` — the shape of
  every exact CalcMoveLine leg in bigsim.c / workers2.c. Naming the
  pointer is an extra IR temporary that changes the schedule, not the
  registers. Evidence: 96.5% → `[OK]`.

## Relocs / W3

`/W3` clean. `relocs.py`: 0 MISMATCH on all seven `FUNCTION` bodies
(52 relocations, 49 matched). UNRESOLVED: WaveThenResume i7 and
ReserveCafeBrolly i10 / i76 jump tables (`$L434`, `$L462`, `$L463` —
local code symbols; fine).

## Extern-type notes

`IncrementBlokeCounter(ObjClass*, int)` here vs `Team*` in sweep3.c —
caller-side only; the push is a pointer. `GetFirstObjectMatching` /
`GetNextObjectMatching` match objmap.c (`Cell*`, `void*`).
`SuggestNextMove(Pos*, Pos*, Pos*)` matches bnvmove.c. `NewDirForAction`
takes `unsigned char`.
