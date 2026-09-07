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

### LL7 — `Track_StepAlong` `0x00429f30` + `TrackRunSetSlope` `0x00429560` (exact on `scope/LL7`; scope **16/17**, tip `cef28f27`)

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

| Branch / file | `scope/LL3` · `LEGOLAND/coaster11.c` · tip `57e06366` |

**77i / 259/259B**, matchfull **84%**, audit **42** mism — **FLOOR** (dest-coalesce).

Need / have:
- `lea ebx,[eax+0x70]` before first `rep movsd` / `mov ebx,eax` then `add` sunk before `Span_EvalRange`
- `mov edx,[esp+0x84]` then `add esp,4` / reverse (`q` in eax)
- hist `edx=*mass`, `ecx=i&0x3f` / swapped

Original delay slot is already `lea edi,[esp+0x14]` then wanted `lea ebx`;
VC6 fills it with a callee-saved **copy of p** and sinks `+0x70`. Interleave
wave (pos→head→f24, named src, two-step, MassSnap keep, dying next,
`n` from `g_route_eval`, n-in-EvalRange comma, vol `fr.f24`) still **65/77**
or worse (54–64%). q still after `add esp,4`; hist swap sticky.

Earlier ruled out: LL2 `Fst`; volatile head; SetTrainAt / CollectCarSample
lea siblings do not transfer; named `&p->pos` + delayed `n` → **58/78**.

(`Raster_ClipPoly` closed via `if (1) { switch (flags) … } return count`.)
Trace NG22 / ClipPlane ESCAPES unchanged. Park Mass until a new fuse lever.

### LL6 — `GetTrackSegment` `0x00424050` (22/24 scope)

| | |
| --- | --- |
| Branch / file | `scope/LL6` · `LEGOLAND/coaster12.c` |
| Tip | `00779571` |
| Notes | `docs/lanes/scope-ll6.md` |
| Score | **87/87i, 232/232B, 34 mism (~83%)** — **FLOOR** (ICF + layout) |

Pending `did_match` keeps closed/tail as `jne loop`. Second `return 0` always
ICF-merges into fail1 (identical `pop/xor/ret` bytes). Helpers stay
ch-then-tail (LIFO). `goto match_tail` undoes latches.

**Nest probe:** `if (1) { whole head walk }` can emit a real fail2
fall-through, but then head-empty cannot `je fail1` — the two goals
fight (91i/237B or retarget). Need fail2 distinct **and** head-loop
fall-through **and** head-empty → fail1.

Ruled out: empty `__asm {}` on fail2 — **64/89 = 71.9%**; noinline/volatile
second epilogue after helpers; `#pragma optimize("g", off)`.
**Do not** use `if (0) { match_tail: … }` outlining — drops to ~11%.

### LL6 — `Raster_AddSpanRecord` `0x00423200`

**61/61i, 175/175B, 35 mism (~61%)** — Grok FLOOR (rechecked 37/61).
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
| Tip | `39bc0fac` |
| Notes | `docs/lanes/scope-ll4.md` |

| address | name | residual |
| --- | --- | --- |
| 0x0041f8d0 | Span_FillFlat | 106/106i, 309/307B, 88 mism — `sub esp,0x58` vs `0x60`; row/dead in n/key slots |
| 0x0041fba0 | Span_FillFlatZ | 136/136i, 398/399B, 62 mism — NG47 row homes |
| 0x0041fd80 | Span_FillShade | 162/162i, 507/505B, 106 mism |
| 0x0041ff80 | Span_FillShadeZ | 202/202i, **633/633B**, 62 mism — ZBuffer_FillPoly ceiling |
| 0x00420200 | IntegrateSimpson | **80/81 = 98.8%**, 81i/**258**/258B, **2 mism** — only `fn(a)` order |

**Simpson lever (landed):** `#pragma optimize("g", off)` +
`f.x = a + (f.h = g_half * f.h)` → jmp-to-test `jg`, two `add esp,4`,
`fsubr`, `fst h` chain. Og-on re-merges `add esp,8` and inverts the for.

**Simpson residual:** ours `call; add esp,4; fstp fa` vs orig
`call; fstp fa; add esp,4`. Asm-call probe saved esi/ebx/edi → 75.9%.
Need fstp-before-cleanup without losing Og-off gains.

Exact already: Romberg_Evaluate, TrackCursor_RetreatGeometry, GetCoasterTexture.
Same class as `ZBuffer_FillPoly` (count-exact, homes wrong). Pos/volatile/latch
probes exhausted by Grok.

### LL7 — `TrackShade_FillPoly` `0x00428860`

**Improved, still WIP** (16/17). Sibling of `ZBuffer_FillPoly` (schoolcar6.c).
**254/254i, 765/771B, frame 0x70**, matchfull **176/254 = 69.3%**.

Landed through pitch-volatile pin: `last`/ShadeSetup/`y` split; eax
`g_zb_polys++`; `imul [ebp-4]`; `py = *(int volatile*)&s.pitch * y` then
volatile zrow so zrow cannot hoist past the product — `shl` + both `add`s.

**FLOOR (source-level register web).** Tip `7c7c6c86`, firstX=92 through crow
store. Residual: ours `store zrow; mov eax,[g_shade_count]; test eax` vs orig
`mov edx,[g_shade_count]; test edx; store zrow`.

Ruled out: crowp / StoreThenN / drop-`c` (still 66.1% if nshade sits between
stores); late-birth naming of both add temps first (65.9%, ebx imul /
`g_zb_polys++` in edi); if-fold / ternary (37–41%); `c_redef` / Fst-on-zrow
(closest — paired adds exact, zrow in test/jle gap, but nshade hoists one slot
*before* crow store into **eax** while edx+ecx still hold adds). Copy-prop
kills same-var redef; volatile crow-store barrier unpaired adds (68.1%) or
66.1%.

Binding: any IR that keeps nshade off the add web hoists it into eax while
edx is live; any IR that makes nshade live during the adds steals
`g_zb_polys++` into ebx. Park until a new lever (not more C spellings of the
same window).

### LL3 — `Span_ClipPlane` `0x0041f050`

**9% ESCAPES**, 179i, 597/593B, frame **0x24** (want **0x2c**). Reconstruct.

Original control flow (2026-09-08 disasm pass):
- Close `in[n]=in[0]`, then `cmp n,1 / jl` to a **trailing** early epilogue.
- Then `in++`, `left=n` (homes in dead `n` slot), `jmp` into body so continue
  reloads abs/sign.
- Latch: `in += 4; dec left; jne` (not `in[i+1]`).
- Classify: signed `sar 1 / and 0x40000000 / or next_sign` as `cls` (also next
  prev_sign). Tests `0x80000000` enter, `0xC0000000` both-in, `0x40000000` leave.
- Divide uses **`fld` of bit-abs** (`and 0x7fffffff` then load those bits), not
  `fild` / `(float)abs` — current C stays ~1–2/179 without that.
- Prologue coloring: load **cursor, then `n` into ebx, then `*cursor` into edx**.
  If `*cursor` takes ebx first, dst steals the callee-save and **0x2c homes never
  appear**. `t` in dead `in` arg; in-cursor in dead `n` arg; prev_abs at `+0x24`.
- ENTER: `k` spilled, delta in ebp. LEAVE: emit prev first, `k` in ebp, delta spilled.

Variants that grew frame (0x28/0x38) or forced `in++`/`fld`/signed classify alone
stayed **2/179**. **ebx=n wave (did not land):** long-lived dest always wins
ebx; n→esi/edi. Homing dest moves deref to eax but prev_abs/out take ebx.
`--n` live across `__ftol` blocks ebx reuse (n → spill left → next_abs).
**Next:** n’s live range must **die at the left home** before abs is computed,
and dest must not be a callee-save candidate at the same time. Tip shape has
in++/fld/classify; still ~9%/0x24.

### LL3 — `BsRoute_Trace` `0x0041c940`

**FLOOR (NG22).** 130i/339B byte-exact, 105 mism. Same phase-order allocation
as `JungleCruise_TraceRoute` (see LEVERS NG22). No source spelling has both
the west tail→loop and the original register ranking.

---

## Suggested Fable attack order

1. **LL4 IntegrateSimpson** — last residual: `fstp fa` before `add esp,4` after `fn(a)` (80/81).
2. **LL3 Span_ClipPlane** — n dies at left-home before abs; dest must not take ebx.
3. **LL4 Span_Fill*** — ZBuffer-class; only after Simpson or with a new frame lever.
4. **LL6 / LL7 / Mass / Trace** — parked floors (ICF, nshade, dest-coalesce, NG22).

When a scope hits **N/N exact**, stop and report tip SHA for integrator merge.
Do **not** merge partial scopes yourself.

---

## Scoreboard (parked branches)

| scope | exact | tip (approx) | file |
| --- | ---: | --- | --- |
| LL1 | **22/22** | merged `main` | `logflume8.c` |
| LL2 | **6/6** | merged `main` | `logflume9.c` |
| LL3 | 16/19 | `57e06366` | `coaster11.c` |
| LL4 | 3/8 | `39bc0fac` | `coastershade2.c` |
| LL5 | **3/3** | merged `main` | `castletrack2.c` |
| LL6 | 22/24 | `b533c24f` | `coaster12.c` |
| LL7 | 16/17 | `cef28f27` | `coaster13.c` |
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
