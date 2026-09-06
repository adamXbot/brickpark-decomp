# Scope Z — the low-level bloke AI: the 16-entry state dispatch and its helpers (2026-09-06)

> **Status: OPEN, unclaimed.** Branch `scope/Z`. Notes: `docs/lanes/scope-z.md`.
> Object prefix `/tmp/sz_`. Any agent. Cut from inventory groups 20–21
> (`tools/inventory.py`, 2026-09-06, the tree at the R/X merge).

**Read `docs/PARALLEL_CONTRACT.md` first; it carries everything not written
here** — including the relocation step of the gate
(`$PY tools/relocs.py LEGOLAND/<file>.c`, zero `MISMATCH` lines).

NEW-FUNCTION scope, **28 functions, ≈1,650 instructions**, one new file,
`LEGOLAND/lowlevelai.c`. One function (0x004841e0, 18 instructions) is dead
code — match it anyway, it is short.

## What this tier is

`blokemisc.c` line 32 already names the table: `extern void
(*g_lowlevel_ai[])(Bloke*);` at **0x004bd34c**, "low-level AI dispatch table,
indexed by `Bloke::state`". Its 16 entries, in table order, are the 16
handlers below (index 0 is an 8-instruction `DBPrintf` stub for an
unimplemented state; index 13 is 11 instructions). The consumer is
`sweep3.c`'s exact function at 0x00484910 (it reads the table at
0x00484935); `bnvpath.c` 0x00484950 and `blokemisc.c` 0x00484920 are the
matched neighbours on either side, so the `Bloke` layout those files use
is the one to reuse (`+0x68`/`+0x6c` position, `+0x72` a byte, `+0x7f`
heading byte — read off 0x00483d10's prologue: `movzx ax, byte ptr
[esi+0x7f] / mov cl, byte ptr [esi+0x72] / push eax / push ecx / call
0x004831a0`).

The helpers are the arithmetic the handlers share:

- **0x004831a0 (13 insns) turns a heading byte and a speed into a delta
  pair**: `and ecx,0xff / shl ecx,2 / movsx eax,[ecx+0x4bd32c] / movsx
  edx,[ecx+0x4bd32e] / imul eax,esi / imul edx,esi / sar 8` — a 256-entry
  table of `short {cos, sin}` at **0x004bd32c**, and the result comes back
  in eax:edx, i.e. the function returns an 8-byte value (`__int64` or a
  two-int struct — measure which spelling VC6 reproduces; the callers read
  `mov edi,eax / mov ebx,edx`).
- **0x00483160 (21 insns) is a map-bounds test**: both arguments must be
  `> 0` and `< (g_map->w << 8)` / `< (g_map->h << 8)` with `g_map` at
  0x004bcbf4 and the `unsigned short` width/height at +0x14/+0x16 (the
  same fields `mapbuild2.c` and `cursorseg.c` use).
- 0x00483260 (54), 0x00483300 (79), 0x00483580 (74), 0x00483680 (107),
  0x00483850 (21), 0x00483890 (3), 0x00483b60 (76), 0x00483c20 (97) and
  0x004841a0 (21) are called only from inside this tier (see the callers
  column); name them from what they do.

Provisional handler names are `LowAI_State<N>` by table index — rename
from the body (the state numbers are the `Bloke::state` values `blokeai.c`,
`blokemisc.c` and `workers*.c` store; grep those files for the constant a
handler tests or stores to find its real name).

## `LEGOLAND/lowlevelai.c` — the 16 handlers (table order) and the 12 helpers

| address | provisional name | insns | reached by; evidence |
| --- | --- | ---: | --- |
| 0x004838a0 | `LowAI_State0` | 8 | `g_lowlevel_ai[0]`; `DBPrintf(0x004bdcd8, [0x008119a4], arg)` stub |
| 0x004838c0 | `LowAI_State1` | 10 | `g_lowlevel_ai[1]` |
| 0x00483ef0 | `LowAI_State2` | 159 | `g_lowlevel_ai[2]`; calls 0x004831a0, 0x00483300, 0x00483680 |
| 0x00484090 | `LowAI_State3` | 114 | `g_lowlevel_ai[3]`; calls 0x004831a0, 0x00483300, 0x00483680 |
| 0x00483d10 | `LowAI_State4` | 54 | `g_lowlevel_ai[4]`; calls 0x004831a0, 0x00483300, 0x00483680, 0x00483c20 |
| 0x004838e0 | `LowAI_State5` | 19 | `g_lowlevel_ai[5]`; calls 0x00483850 |
| 0x00484220 | `LowAI_State6` | 112 | `g_lowlevel_ai[6]`; calls 0x004841a0 |
| 0x004845d0 | `LowAI_State7` | 35 | `g_lowlevel_ai[7]`; calls 0x004841a0 |
| 0x00484630 | `LowAI_State8` | 34 | `g_lowlevel_ai[8]`; calls 0x00483850 |
| 0x00484790 | `LowAI_State9` | 123 | `g_lowlevel_ai[9]`; calls 0x00483890; reads the table itself at 0x004848b0 |
| 0x00483e20 | `LowAI_State10` | 88 | `g_lowlevel_ai[10]`; calls 0x004831a0, 0x00483300, 0x00483680, 0x00483b60, 0x00483c20 |
| 0x00484470 | `LowAI_State11` | 64 | `g_lowlevel_ai[11]`; calls 0x004841a0 |
| 0x00484520 | `LowAI_State12` | 64 | `g_lowlevel_ai[12]`; calls 0x00483580, 0x00483680, 0x004841a0 |
| 0x004848e0 | `LowAI_State13` | 11 | `g_lowlevel_ai[13]` |
| 0x00483d90 | `LowAI_State14` | 60 | `g_lowlevel_ai[14]`; calls 0x004831a0, 0x00483300, 0x00483680, 0x00483c20 |
| 0x00484350 | `LowAI_State15` | 108 | `g_lowlevel_ai[15]`; calls 0x004841a0 |
| 0x00483160 | `MapPointInBounds` | 21 | called by 0x00483300; `g_map` +0x14/+0x16 `<< 8` bounds |
| 0x004831a0 | `HeadingDelta` | 13 | called by the state handlers; `short {cos,sin}[256]` at 0x004bd32c, returns eax:edx |
| 0x00483260 | `sub_483260` | 54 | called by 0x00483300 |
| 0x00483300 | `sub_483300` | 79 | called by states 2, 3, 4, 10, 14; calls 0x00483160, 0x00483260 |
| 0x00483580 | `sub_483580` | 74 | called by 0x00483c20 and state 12 |
| 0x00483680 | `sub_483680` | 107 | called by states 2, 3, 4, 10, 12, 14 |
| 0x00483850 | `sub_483850` | 21 | called by states 5 and 8 |
| 0x00483890 | `sub_483890` | 3 | called by state 9 |
| 0x00483b60 | `sub_483b60` | 76 | called by state 10 |
| 0x00483c20 | `sub_483c20` | 97 | called by states 4, 10, 14; calls 0x00483580 |
| 0x004841a0 | `sub_4841a0` | 21 | called by states 6, 7, 11, 12, 15 |
| 0x004841e0 | `sub_4841e0` | 18 | dead (unreferenced) — match it, note it dead |

Sizes are the inventory's; `tools/matchfull.py` prints the authoritative
extent. Callers/callees above are the inventory's call graph
(`tools/inventory.py --json` → `functions[].callers`); confirm from the
disassembly (`tools/disasm.py original/legoland.exe 0x<RVA> <n>`, RVA = VA −
0x400000).

## Levers to expect

The event-tick briefs (V, W, X) are the closest template: a table of
same-shaped handlers, each a short body over one struct pointer.
`docs/LEVERS.md` first, then the newest DECOMP folds (`scope-x`: naming the
definition pointer decides scratch-register allocation; a position
aggregate's scope decides store scheduling; `scope-r`: the two-flag
set-and-break lever; `scope-t`: an `unsigned short` field's `|= K` narrows
to `or byte ptr`). The `movzx ax, byte ptr` (16-bit destination) in
0x00483d10's prologue is a `short` parameter fed from a byte field — spell
the parameter `short`, the argument `bloke->heading` as `unsigned char`.

## Owned elsewhere — do not create or edit

`blokemisc.c` (declares `g_lowlevel_ai`; keep its name), `sweep3.c`,
`bnvpath.c`, `blokeai.c`, `workers.c`, `workers2.c`, `workers3.c`,
`tilehelp.c`, `pathtile2.c`, `tri3d.c`; `eventgoal.c`, `eventtick.c` (V);
Codex-F's files; every file in scopes F, G, H (rides, coaster, flume,
jungle cruise); every existing `.c`. Declare what you call `extern` with
its address in a comment, as the sibling files do.
