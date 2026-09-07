# Fable handoff — LL-wave WIP residuals (2026-09-08)

Grok closed what it could on **LL1–LL8**. **LL1, LL2, LL5, and LL8 are done and on `main`.**
Everything below is still `// WIP-FUNCTION:` on the named branch. Prefer
closing one body at a time; promote only when `audit.py` prints `[OK]`.

**Do not add Co-Authored-By / Co-authored-by trailers** (Cursor or Claude).
If the harness injects one, rewrite with `git commit-tree` / `update-ref`
before push.

## Environment

```
PY=/Users/systemadmin/.venvs/legoland/bin/python
export LEGOLAND_CL=/Users/systemadmin/Documents/Development/Github/alphateam/tools/wibo-msvc/cl
# work from the scope worktree, e.g.
cd /Users/systemadmin/Documents/Development/Github/legoland/.worktrees/scope-ll3
```

Gates: `audit.py` `[OK]`, `relocs.py` zero `MISMATCH`, `/W3` clean.
Objects under `/tmp/sllN_*`. No `verify.py` / `progress.py` / `coverage.py`.
Contract: `docs/PARALLEL_CONTRACT.md`. Index: `docs/SCOPE_LL_WAVE.md`.

**Main tip when this was written:** post-LL8 merge (AddScriptString fail-tail close).

---

## Closed this wave (lever notes for reuse)

### LL8 — `AddScriptString` `0x004689f0` (13/13 merged)

`!a`: `g_script_strings[count] = 0; goto bump;` duplicates the shared
`n = count; count++; return n` as eax-primary
`mov ecx,eax / pop esi / inc ecx`.
`!copy`: `slot = &g_script_strings[count]; *slot = a;` puts the index in
**EDX**. Volatile shims / `return count++` / LL2 `Fst` could not hold both
tails at once.

### LL7 — `Track_StepAlong` `0x00429f30` + `TrackRunSetSlope` `0x00429560` (exact on `scope/LL7`; scope **16/17**, tip `7c7c6c86`)

**StepAlong:** Named `float step2 = step * step` was the wall. Write the square
only at the store after t0: `t0 = cur.geom->t0; g_step_len2 = step * step;`

**SetSlope:** Latch spelling — `for (; steps > 0; steps--)` (not
`if (steps > 0) do { … } while (--steps)`) gets the 7 eax↔edx homes.
Same 90i/302B either way; only the IV/zero colouring differs.

Still open: `TrackShade_FillPoly` — **254/254i, 765/771B, 69.3%** (tip stripped
from `514073c5`). Pitch-volatile pin landed shl+adds. Residual: crow/zrow
base loads swapped; nshade still eax before crow store (want edx).

---

## Priority A — size-exact / high % with clear next lever

### LL3 — `Route_GetMassAndPower` `0x0041db90` (16/19 scope)

| Branch / file | `scope/LL3` · `LEGOLAND/coaster11.c` · tip `af5077cd` |

**77i / 259/259B**, matchfull **84%**, audit **42** mism — FLOOR notes.

Need / have:
- `lea ebx,[eax+0x70]` before first `rep movsd` / `mov ebx,eax` then `add` sunk before `Span_EvalRange`
- `mov edx,[esp+0x84]` then `add esp,4` / reverse (`q` in eax)
- hist `edx=*mass`, `ecx=i&0x3f` / swapped

`eax` stays live for `[eax+0x24]` after `mov ebx,eax` — sink-vs-fuse, not missing live use. LL2 `Fst` RTL does **not** transfer (that closes 3-scratch SIB lea, not `reg+disp8` dest-coalesce onto callee-saved `p`). Transparent helpers / two-web `t`/`n` / `Mass_End` fold to 65/77. Volatile `head` spills frame; `fr.f24` makes mov/add adjacent but not `lea`.

**2026-09-08 probes (still 65/77):** `lea ebx,[eax+0x70]` is unique in `.text`. Live-eax lea sibling is **SetTrainAt** (needs early push/use of head — does not transfer). No-use-lea sibling is **CollectCarSample** (`lea esi,[eax+0x70]` between call pushes) — cannot host Mass’s head lea: Mass already has `lea edx,[eax+0xc]` in-slot and kills eax for f24 before `Span_EvalRange`. Named `&p->pos` + delayed `n` mutates eax (**58/78**). Dest-coalesce stays the attractor.

(`Raster_ClipPoly` closed via `if (1) { switch (flags) … } return count`.)
Trace NG22 / ClipPlane ESCAPES unchanged.

### LL6 — `GetTrackSegment` `0x00424050` (22/24 scope)

| | |
| --- | --- |
| Branch / file | `scope/LL6` · `LEGOLAND/coaster12.c` |
| Tip | `00779571` |
| Notes | `docs/lanes/scope-ll6.md` |
| Score | **87/87i, 232/232B, 34 mism (~83%)** — Grok FLOOR |

Pending `did_match` keeps closed/tail as `jne loop`. Second `return 0` always
ICF-merges into fail1 (head stays `je fail1 / jmp loop`). Helpers stay
ch-then-tail (LIFO). `goto match_tail` undoes latches. Needs a fail2 that
survives ICF **and** tail-then-ch helper order.

Ruled out: empty `__asm {}` on fail2 (LL4 Simpson pattern) — frame lever only,
**64/89 = 71.9%**; main has no sibling that keeps two `pop/xor/ret` copies.
**Do not** use `if (0) { match_tail: … }` outlining — drops to ~11%.

### LL6 — `Raster_AddSpanRecord` `0x00423200`

**61/61i, 175/175B, 35 mism (~61%)** — Grok FLOOR.
`push ecx` / cursor in ecx landed. Residual: count in **esi not ebx**; no
`mov edx,ecx` (y from `[ecx+4]`); keys-1 IV (`lea edx,[keys-8]`) vs
`add edx,8` / `[edx-8]`; `dec edi` not `dec ebx`. Dropping volatile home
gets `dec ebx` but loses the frame.

Ruled out: ridemisc-style biased `int*` on `&keys->idx` (`k += 2`, `k[-2]`)
→ **38/61**, wrong IV anchor (`add edx,4` / `[edx]`).

---

## Priority B — documented floors / ZBuffer-class (low ROI unless new lever)

### LL4 — four `Span_Fill*` + `IntegrateSimpson` (3/8 scope)

| Branch / file | `scope/LL4` · `LEGOLAND/coastershade2.c` |
| Tip | `c81396e2` |
| Notes | `docs/lanes/scope-ll4.md` |

| address | name | residual |
| --- | --- | --- |
| 0x0041f8d0 | Span_FillFlat | 106/106i, 309/307B, 88 mism — `sub esp,0x58` vs `0x60`; row/dead in n/key slots |
| 0x0041fba0 | Span_FillFlatZ | 136/136i, 398/399B, 62 mism — NG47 row homes |
| 0x0041fd80 | Span_FillShade | 162/162i, 507/505B, 106 mism |
| 0x0041ff80 | Span_FillShadeZ | 202/202i, **633/633B**, 62 mism — ZBuffer_FillPoly ceiling |
| 0x00420200 | IntegrateSimpson | 81/81i, 251/258B, 61 mism — volatile 0x28 frame; `add esp,8`; latch `jle` not `jg` |

Exact already: Romberg_Evaluate, TrackCursor_RetreatGeometry, GetCoasterTexture.
Same class as `ZBuffer_FillPoly` (count-exact, homes wrong). Pos/volatile/latch
probes exhausted by Grok.

### LL7 — `TrackShade_FillPoly` `0x00428860`

**Improved, still WIP** (16/17). Sibling of `ZBuffer_FillPoly` (schoolcar6.c).
**254/254i, 765/771B, frame 0x70**, matchfull **176/254 = 69.3%**.

Landed through pitch-volatile pin: `last`/ShadeSetup/`y` split; eax
`g_zb_polys++`; `imul [ebp-4]`; `py = *(int volatile*)&s.pitch * y` then
volatile zrow so zrow cannot hoist past the product — `shl` + both `add`s.

**Residual (firstX=92 through crow store; yp-pin committed):** ours does
`store zrow; mov eax,[g_shade_count]; test eax` vs original
`mov edx,[g_shade_count]; test edx; store zrow`. Killing crow’s edx at the
store (crowp / StoreThenN / drop named `c`) does **not** free edx for nshade
without the 66.1% between-stores wall (swapped adds + `g_zb_polys++` in ebx).
Keeping `c` live across nshade only hoists the load before crow store (still
eax). Need nshade born after crow’s edx dies, without being live during add /
`g_zb_polys++` allocation.

### LL3 — `BsRoute_Trace` `0x0041c940`

**FLOOR (NG22).** 130i/339B byte-exact, 105 mism. Same phase-order allocation
as `JungleCruise_TraceRoute` (see LEVERS NG22). No source spelling has both
the west tail→loop and the original register ranking.

### LL3 — `Span_ClipPlane` `0x0041f050`

**9% ESCAPES**, 179i, 597/593B. Need 0x2c frame (have 0x24), `in++` in latch,
three-way sign classify on `(prev_sign>>1)|next_sign` vs
`0x80000000 / 0xC0000000 / 0x40000000`. Large reconstruct — not a polish job.

---

## Suggested Fable attack order

1. **LL3 MassAndPower** — size-exact 42 mism; dest-coalesce sink; LL2 RTL Fst inert.
2. **LL3 Trace / ClipPlane** — NG22 / ESCAPES floors.
3. **LL6 GetTrackSegment / AddSpanRecord** — size-exact floors; only with new ICF/IV levers.
4. **LL7 ShadeFill** — ZBuffer floor (StepAlong + SetSlope closed on branch).
5. **LL4 Span family** — last.

When a scope hits **N/N exact**, stop and report tip SHA for integrator merge.
Do **not** merge partial scopes yourself.

---

## Scoreboard (parked branches)

| scope | exact | tip (approx) | file |
| --- | ---: | --- | --- |
| LL1 | **22/22** | merged `main` | `logflume8.c` |
| LL2 | **6/6** | merged `main` | `logflume9.c` |
| LL3 | 16/19 | `af5077cd` | `coaster11.c` |
| LL4 | 3/8 | `c81396e2` | `coastershade2.c` |
| LL5 | **3/3** | merged `main` | `castletrack2.c` |
| LL6 | 22/24 | `00779571` | `coaster12.c` |
| LL7 | 16/17 | `7c7c6c86` | `coaster13.c` |
| LL8 | **13/13** | merged `main` | `gameframe2.c` |

**WIP count in this wave:** 0+0+3+5+0+2+1+0 = **11 bodies**.

---

## Other open lanes (not LL — for awareness)

| Lane | Branch | Status | Notes |
| --- | --- | --- | --- |
| Codex-F | `codex/scope-f` | 25/26 | `Copters_UpdateCarRider` `0x00404630`; decomp.me [Pnxtm](https://decomp.me/scratch/Pnxtm) |
| AC | `scope/AC` | 13/15 | `PutOne3DBlokeOnRide`, `LoadAltTextures`; [NJwTi](https://decomp.me/scratch/NJwTi), [m2smI](https://decomp.me/scratch/m2smI) |
| AG | `scope/AG` | 2/3 | `WinMain` matchfull OK / audit ESCAPES (SEH) |
| V / FGH | external | WIP | EventTick_Clear etc.; [TG0H2](https://decomp.me/scratch/TG0H2) |
| X | external | WIP | parallel leftovers |
