# Scope LL23 — partials wave F: the coaster span / raster family

Branch `scope/LL23` from `origin/main` `b49871b7`. **PARTIAL scope**: edit only
the WIP bodies below and the notes above their markers (see
`docs/PARALLEL_CONTRACT.md`, "Extra rules for PARTIAL scopes"). Object
prefix `/tmp/sll23_`. Findings go in `docs/lanes/scope-ll23.md`.

## Why these are being reopened

These eleven bodies are the WIP leftovers of scopes LL3, LL4, LL6 and LL7,
all merged 2026-09-09. **Read `docs/lanes/scope-ll3.md`, `scope-ll4.md`,
`scope-ll6.md` and `scope-ll7.md` first** — those lanes ground these bodies
hard and their notes list what is already inert. Do not repeat it.

They are grouped as one scope because they are one family: the coaster span
rasterisers and their track/segment feeders, sharing frame layout, row-pointer
homing and induction-variable shape. A lever found on one is likely to move
several. Seven of the eleven are already instruction-exact.

Two levers from scope Codex-F (closed 2026-09-09, `docs/lanes/codex-f.md`,
entry in `docs/DECOMP.md`) are new since these lanes ran and are directly
aimed at this residual class:

1. **The `/FAcs` frame symbol table names the frame problem for you.**
   `ALPHATEAM_VC6_ROOT="$PWD/toolchain" "$LEGOLAND_CL" /nologo /c /O2 /Gy /Gd
   /FAcs /Fa<out>.asm /Fo/tmp/sll23_x.obj LEGOLAND/<file>.c` prints VC6's own
   equates, so a local that cost a frame slot appears as `_name$ = -N`. Four
   of the bodies below have a frame-size or "row home in arg slot" residual
   recorded — that is exactly what the equates list settles in one compile.
   It also confirms VC6 overlaps locals onto dead PARAMETER slots, which is
   what "row homes in arg slots" means, and it is normal, not a bug.
2. **A scope-V cancelled pair steers allocator priority; its ANCHOR is the
   control input, and the anchor takes the priority bump.** Use a link-time
   address constant or an already-materialised induction value as the anchor,
   never the value you are trying to place. Both operands must be members of
   the same struct, and a member living across a loop costs a frame slot —
   keep the carrier inside one basic block. This closed two
   induction-variable/register-ranking floors in Codex-F.

## Functions (11) — 1,475 instructions

| address | name | file | state | residual (from the marker) |
| --- | --- | --- | --- | --- |
| 0x00420200 | IntegrateSimpson | coastershade2.c | 81i/258B exact, **2 mism** | `fn(a)` `add esp,4` / `fstp` swapped — closest body in the scope, start here |
| 0x0041ff80 | Span_FillShadeZ | coastershade2.c | 202i/633B exact, 62 mism | row homes vs original frame |
| 0x0041fba0 | Span_FillFlatZ | coastershade2.c | 136i, 398B vs 399B, 62 mism | row homes in arg slots |
| 0x0041f8d0 | Span_FillFlat | coastershade2.c | 106i, 309B vs 307B, 88 mism | 0x58 vs 0x60 frame |
| 0x0041fd80 | Span_FillShade | coastershade2.c | 162i, 507B vs 505B, 106 mism | row home in arg slot |
| 0x0041db90 | Route_GetMassAndPower | coaster11.c | 77i/259B exact, 42 mism | `lea ebx` coupled to an imm8 store |
| 0x0041c940 | BsRoute_Trace | coaster11.c | 130i/339B exact, 105 mism | marked FLOOR by LL3; re-verify before grinding |
| 0x0041f050 | Span_ClipPlane | coaster11.c | 179i, 606B vs 593B, 173 mism | **reports ESCAPES** — fix the extent first; latch `jne`, `ebx=n`, frame 0x2c |
| 0x00424050 | GetTrackSegment | coaster12.c | 87i/232B exact, 34 mism | head fail2 / call-site order |
| 0x00423200 | Raster_AddSpanRecord | coaster12.c | 61i/175B exact, 35 mism | ebx/edx/keys induction variable |
| 0x00428860 | TrackShade_FillPoly | coaster13.c | 254i, 765B vs 771B, 147 mism | crow store exact, nshade-eax after zrow |

`Span_ClipPlane` is the only body that fails the extent gate (ESCAPES). Treat
that as a correctness bug in the reconstruction, not a matching residual, and
fix it before anything else in `coaster11.c`.

## Rules

- Read the note above each marker FIRST and do not repeat what it lists.
- Reconstruction-error pass before any variant search.
- After EVERY change `audit.py` the whole file: it must still end PASS and
  the count of `[OK]` lines must not drop; if it does, revert.
- `relocs.py` zero `MISMATCH`, `/W3` clean, before each commit.
- Marker stays `// WIP-FUNCTION: LEGOLAND 0x<VA>  (<pct>, <residual>)` until
  `audit.py` prints `[OK]`; then exactly `// FUNCTION: LEGOLAND 0x<VA>`.
- Retire honestly: if a lever does not move a body, add one paragraph to its
  note saying what you measured and move on.
- Do not run `verify.py` / `progress.py` / `coverage.py`; do not edit
  `tools/`, `docs/DECOMP.md`, `docs/HANDOFF.md`, `README.md`.
- Commit to `scope/LL23` only; no push; no merge; **no Co-Authored-By
  trailer of any kind.**
