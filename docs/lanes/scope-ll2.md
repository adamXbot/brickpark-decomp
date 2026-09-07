# Scope LL2 — logflume drop / track tick cluster

Branch `scope/LL2`. File `LEGOLAND/logflume9.c`. Object prefix `/tmp/sll2_`.
Brief: `docs/SCOPE_LL2_logflume_drop.md`.

## Status

| address | name | insns | % | audit | marker |
| --- | --- | ---: | ---: | --- | --- |
| 0x0040d420 | LFGeom_ApplyCursors | 73 | 100 | [OK] | FUNCTION |
| 0x0040d520 | LFTrack_CommitPlacement | 128 | 100 | [OK] | FUNCTION |
| 0x0040d6f0 | LFPiece_UpdateCommon | 151 | 99.3 | 1 | WIP |
| 0x0040d900 | LFPiece_AddCommon | 83 | 100 | [OK] | FUNCTION |
| 0x0040da10 | LFTrack_UnlinkNeighbours | 61 | 100 | [OK] | FUNCTION |
| 0x0040db00 | LFPiece_RemoveCommon | 61 | 100 | [OK] | FUNCTION |

**5 / 6 exact.** Relocs clean on the five FUNCTION bodies. `/W3` clean.
Neighbour-helper names taken from scope LL1. 0x00409a90 / 0x0040a080 still
unmatched in LL1; named here `LFTrack_ReshapeEnds` / `LFTrack_LinkEnds`.

## UpdateCommon residual (index 121, SIB only)

151i/520B vs 151i/520B (byte-exact). First **120** instructions match, and
the people-rect schedule is now the original's: `lea` into edx, y load
into ecx, then `mov [esp],edx`. ox stays in eax through `r.right`.

The only remaining delta is lea SIB base/index:

```
orig: lea edx,[eax+ecx]    ; 8D 14 08  base=eax (ox)  index=ecx (v0)
ours: lea edx,[ecx+eax]    ; 8D 14 01  base=ecx (v0)  index=eax (ox)
```

One mismatch. `norm` does not commute lea operands.

### Lever that got the lea

`o.y` holds v0, then is overwritten with `g_mapref.y`. Combined with an
`unsigned left` dest that is stored after that overwrite:

```
o.x = g_mapref.x;
o.y = g_edit_cursor.footprint.v[0];
left = (unsigned)o.x + (unsigned)o.y;
o.y = g_mapref.y;
r.left = (int)left;
```

The two-def `o.y` web keeps v0 live across the add (dest cannot coalesce)
and dies before the y load (y reuses ecx). Unsigned `left` is required:
plain `r.left = o.x + o.y` dest-coalesces (`add ecx,eax`, index 121).
Volatile v1 on top is unchanged.

### SIB is tied to load order

VC6 uses the **last-loaded addend as lea base**:

- ox then v0 → correct loads, `lea [ecx+eax]` (kept)
- v0 then ox → `lea [eax+ecx]`, but `mov ecx,[v0]` before `mov eax,[ox]`

Those are two 150/151 residuals; they do not combine. Tried and inert for
the SIB (same `[ecx+eax]` or a worse floor): commuting the unsigned add,
`(char*)o.x + o.y`, `&((char*)o.x)[o.y]`, pointer-typed `o.x`, `__inline`
`p+i` helper, `char* px` instead of `o.x` (drops to index 120), dummy
redef/`+0`/cast of `o.x` after v0, volatile ox preload, `int ox` copy
into `o.x` after v0, both addends as memory (breaks the later rect),
y-first struct layout, `Pos s` member dest.

Need a spelling that keeps ox-first loads *and* treats ox as the lea
base, without a second ox load. No such form found.

2026-09-08 SIB pass (still 150/151, body unchanged). Confirmed the two
150/151 residuals still do not combine. Extra measurements after the
unsigned-`left` / two-def `o.y` lea:

- Commuting `o.y + o.x`, pointer-typed `o.x` / `char *p` copy after v0,
  `&p[i]`, `left` as `char*`, union `{int; char*}`, empty-if, `left +=`,
  `ox - (-v0)`, assignment-in-expr, `__inline p+i` / param+global /
  RTL `H(v0,(char*)ox)`, address-taken `o.x`, `*(int*volatile)&o.x`
  after v0: same `[ecx+eax]` (forwarded; no extra insn).
- `o.y = v[0]; o.x = g_mapref.x; left = o.x + o.y` (and comma-hoist
  variants that DCE): correct SIB, swapped loads (`mov ecx,[v0]` first).
- Track `v[0] + g_mapref.x` without both addends as named regs: dest-
  coalesces (`add edx,ecx` / `add edx,eax`), 140–149/151. REG+MEM
  loses the lea; both addends must stay register symbols.
- Clean `char *px` / pointer-struct without `Pos o.x` drops to 149
  (`add edx,eax`). Second ox load or `o.x =` after the lea shuffles
  the later rect (141).

`LFTrack_Update`'s `[eax+ecx]` is mem+mem RTL (`v[0]+g_mapref.x`)
under saved ebx/esi/edi. The two-def that produces the 3-scratch lea
here turns that add into reg+reg and flips the SIB. Still no spelling
that keeps ox-first moffs32 loads *and* ox as lea base.

2026-09-08 earlier floors (still true of spellings that drop the two-def
`o.y` / unsigned `left` pair):

- **Index 121:** `Pos o` + `r.left = o.x + v[0]` + volatile v1. v0 in ecx;
  dest-coalesces (`add ecx,eax`) and stores immediately. 519/520B.
- **Index 120:** named sum / delayed store without the `o.y` two-def.
  y-before-store but v0 in edx (`add edx,eax`).

`LFTrack_Update` emits `lea edx,[eax+ecx]` from four plain assigns because
ebx/esi/edi are still saved; the same four assigns here dest-coalesce or
take index 120. The original people block sits after `pop edi / pop esi`,
so the 3-scratch lea is real — Track's extra pushes are not a missing
Common `push ebx`.

Also ruled out this pass: silent post-add v0 (DCE to index 120),
observable post-add v0 (extra store), zero-table index (extra load or
DCE), `*(int*)&ox + v0` (index 121 or 120), cost/people as dest (120),
RTL `FillR` / second live sum (esi, moved pops). `__asm` not used.

Levers that landed the rest: union `{packed, nb}` in the fp slot; `mode=0`
after ScreenToMapRef so packed cannot colour onto mode; separate
`FirstRun` / `KeepRun` (nested call splits leftover); `if (count==0)` so
the short fail is inline; `if (probe!=0)` so success is inline; people
`if (n != -1) { if (n==1) err 3; } else err 4`.

## Names

- `LFTrack_UnlinkNeighbours` / `LFTrack_CommitPlacement` / `LFPiece_UpdateCommon`
  / `LFPiece_AddCommon` / `LFPiece_RemoveCommon` already declared in logflume.c.
- `sub_40d420` → `LFGeom_ApplyCursors`: stamps LFGeom connection points onto
  the two preview cursors at 0x004c4468 / 0x004c5ca0.

## Globals first named here

- `g_lf_geom_cursor_a` 0x004c4468 / `g_lf_geom_cursor_b` 0x004c5ca0
- `g_lf_place_cursor_a` 0x004ca5b0 / `g_lf_place_cursor_b` 0x004c1260
- `g_lf_commit_a` 0x004cbde0 / `g_lf_commit_b` 0x004c2a90 (zeroed by CommitPlacement)
