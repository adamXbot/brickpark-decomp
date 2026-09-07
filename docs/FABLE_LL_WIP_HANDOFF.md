# Fable handoff — LL-wave WIP residuals (2026-09-08)

Grok closed what it could on **LL1–LL8**. **LL1, LL2, and LL5 are done and on `main`.**
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
cd /Users/systemadmin/Documents/Development/Github/legoland/.worktrees/scope-ll2
git pull --ff-only origin scope/LL2
```

Gates: `audit.py` `[OK]`, `relocs.py` zero `MISMATCH`, `/W3` clean.
Objects under `/tmp/sllN_*`. No `verify.py` / `progress.py` / `coverage.py`.
Contract: `docs/PARALLEL_CONTRACT.md`. Index: `docs/SCOPE_LL_WAVE.md`.

**Main tip when this was written:** post-LL2 merge (UpdateCommon RTL SIB close).
Rebase/ff worktrees onto current `main` only if you need merged helpers;
otherwise stay on the scope branch tip.

---

## Priority A — one mismatch / one byte / few mism (highest ROI)

### LL8 — `AddScriptString` `0x004689f0` (12/13 scope)

| | |
| --- | --- |
| Branch / file | `scope/LL8` · `LEGOLAND/gameframe2.c` |
| Tip | `f8c1f002` |
| Notes | `docs/lanes/scope-ll8.md` |
| Score | 91i, **267/264B**, **6 mism** — FLOOR |

Both-string and one-string paths stay exact. The two fail tails **share
allocation**: lea/CSE/volatile flips that fix one recolor the other (or the
exact one-string EDX store).

- Drop `!copy` volatile → loses one-string EDX (74/91)
- Drop only `!a` volatile → early `pop edi`, flips eax/ecx and store order (83/91)
- `count = n + 1` / `-~n` → `lea`, moves `!copy`’s `a` into EDX (73/91)
- Out-of-line helpers → `call` (77/91); inlined twins still share allocator
- Per-edge volatiles → best 86/92 (+extra base load)
- `return count++` on `!a` → **86/91 exact 264B** but won’t combine with EDX `!copy`
- Early live `n`, `register`, decl order, sibling push-sink: inert or first-break

Still need `mov ecx,eax / pop esi / inc ecx` on `!a` **and** `mov edx,[count]`
on `!copy` together. Name: keep **`AddScriptString`**.

---

## Priority B — size-exact / high % with clear next lever

### LL3 — `Route_GetMassAndPower` `0x0041db90` (16/19 scope)

| Branch / file | `scope/LL3` · `LEGOLAND/coaster11.c` · tip `58c2dcca` |

**77i / 259/259B**, matchfull **84%**, audit **42** mism — FLOOR notes.

Need / have:
- `lea ebx,[eax+0x70]` before first `rep movsd` / `mov ebx,eax` then `add` sunk before `Span_EvalRange`
- `mov edx,[esp+0x84]` then `add esp,4` / reverse (`q` in eax)
- hist `edx=*mass`, `ecx=i&0x3f` / swapped

`eax` stays live for `[eax+0x24]` after `mov ebx,eax` — sink-vs-fuse, not missing live use. Comma/`char*+0x70`/helpers, early `n`, named `end`, hist `i++`, restore copies → same 65/77. `q` in edx + hist ecx only with extra volatiles (**80i**). Volatile `fr.f24` moves the add earlier but breaks call-arg `rep movsd` interleave (64/77).

(`Raster_ClipPoly` closed via `if (1) { switch (flags) … } return count`.)
Trace NG22 / ClipPlane ESCAPES unchanged.

### LL7 — `Track_StepAlong` `0x00429f30` (14/17 scope)

| | |
| --- | --- |
| Branch / file | `scope/LL7` · `LEGOLAND/coaster13.c` |
| Tip | `afd8951d` |
| Notes | `docs/lanes/scope-ll7.md` |
| Score | 70i, **232/232B**, audit **11** mism |

Live-across `org = origin` + early `t0` closed the 6-byte gap. Loop exact.
Original order after `rep movsd`:
`geom, esi=out_t, push esi, t0, eax=step, fstp len2, fld/fadd, …, push tol, g_step_len, fld/fmul`

Two attractors — no tested spelling emits that order:

- **A (kept, 11 mism):** `t0` before `len2` — correct FP; `push esi` 7 late, `g_step_len` 2 late.
- **B (21 mism, ~90%):** `g`; `len2`; `t0` — early push + correct len; `fstp`/`fadd` hoist.

Also ruled out: finished pre-call `t0` (batches push with tol), helper/comma hi,
empty `__asm` (EBP), Joust `H(&ot,out_t)`, union/`unsigned` t0bits (71i),
`g->t0` as call arg, `sol = g_track_solver`. Sticky schedule split; not formally floored. Ideas 1–6 (g+comma-lo, held-off step2, volatile out_tp, split lo/hi2, home reorder, MeasureDistance/Bisect copy) stayed on A or worse; homes not swapped.

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

### LL6 — `Raster_AddSpanRecord` `0x00423200`

**61/61i, 175/175B, 35 mism (~61%)** — Grok FLOOR.
`push ecx` / cursor in ecx landed. Residual: count in **esi not ebx**; no
`mov edx,ecx` (y from `[ecx+4]`); keys-1 IV (`lea edx,[keys-8]`) vs
`add edx,8` / `[edx-8]`; `dec edi` not `dec ebx`. Dropping volatile home
gets `dec ebx` but loses the frame.

Ruled out: ridemisc-style biased `int*` on `&keys->idx` (`k += 2`, `k[-2]`)
→ **38/61**, wrong IV anchor (`add edx,4` / `[edx]`).

---

## Priority C — documented floors / ZBuffer-class (low ROI unless new lever)

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

### LL7 — `TrackRunSetSlope` `0x00429560`

**Floor.** 90i/302B, **7 eax↔edx**. `--steps` IV always wins eax; loop zero in edx.
Volatile zero / named remain / live-across-call: inert or +bytes.

### LL7 — `TrackShade_FillPoly` `0x00428860`

**ZBuffer floor.** Sibling of `ZBuffer_FillPoly` (schoolcar6.c). 254/254i,
748/771B, frame 0x6c vs 0x70. EBP, `xchg ebx,eax`, `add ebx,1`, mixed `__asm`.
Param is `nkeys` (`ne` is MASM reserved).

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

1. **LL8 AddScriptString** — fail-tail shared allocation (floored unless new coloring).
2. **LL3 MassAndPower** — size-exact 42 mism; lea ebx vs add; q-load schedule.
3. **LL7 StepAlong** — size-exact 11 mism; t0↔len2 two-attractor (ideas 1–6 ruled out).
4. **LL3 Trace / ClipPlane** — NG22 / ESCAPES floors.
5. **LL6 GetTrackSegment / AddSpanRecord** — size-exact floors; only with new ICF/IV levers.
6. **LL4 Span family / LL7 Slope+ShadeFill** — last.

When a scope hits **N/N exact**, stop and report tip SHA for integrator merge.
Do **not** merge partial scopes yourself.

---

## Scoreboard (parked branches)

| scope | exact | tip (approx) | file |
| --- | ---: | --- | --- |
| LL1 | **22/22** | merged `main` | `logflume8.c` |
| LL2 | **6/6** | merged `main` | `logflume9.c` |
| LL3 | 16/19 | `58c2dcca` | `coaster11.c` |
| LL4 | 3/8 | `c81396e2` | `coastershade2.c` |
| LL5 | **3/3** | merged `main` | `castletrack2.c` |
| LL6 | 22/24 | `00779571` | `coaster12.c` |
| LL7 | 14/17 | `afd8951d` | `coaster13.c` |
| LL8 | 12/13 | `f8c1f002` | `gameframe2.c` |

**WIP count in this wave:** 0+0+3+5+0+2+3+1 = **14 bodies**.

---

## Other open lanes (not LL — for awareness)

| Lane | Branch | Status | Notes |
| --- | --- | --- | --- |
| Codex-F | `codex/scope-f` | 25/26 | `Copters_UpdateCarRider` `0x00404630`; decomp.me [Pnxtm](https://decomp.me/scratch/Pnxtm) |
| AC | `scope/AC` | 13/15 | `PutOne3DBlokeOnRide`, `LoadAltTextures`; [NJwTi](https://decomp.me/scratch/NJwTi), [m2smI](https://decomp.me/scratch/m2smI) |
| AG | `scope/AG` | 2/3 | `WinMain` matchfull OK / audit ESCAPES (SEH) |
| V / FGH | external | WIP | EventTick_Clear etc.; [TG0H2](https://decomp.me/scratch/TG0H2) |

Full per-body evidence lives in each `docs/lanes/scope-llN.md` on the
corresponding worktree (copy into the Fable session’s tree if missing).
