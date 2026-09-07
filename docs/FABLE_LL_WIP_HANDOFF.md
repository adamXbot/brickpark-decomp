# Fable handoff — LL-wave WIP residuals (2026-09-08)

Grok closed what it could on **LL1–LL8**. **LL1 and LL5 are done and on `main`.**
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

**Main tip when this was written:** post-LL1 merge (`0b24fc30` + docs bump).
Rebase/ff worktrees onto current `main` only if you need merged helpers;
otherwise stay on the scope branch tip.

---

## Priority A — one mismatch / one byte / few mism (highest ROI)

### LL2 — `LFPiece_UpdateCommon` `0x0040d6f0` (5/6 scope)

| | |
| --- | --- |
| Branch / file | `scope/LL2` · `LEGOLAND/logflume9.c` |
| Tip | `7bea1842` |
| Notes | `docs/lanes/scope-ll2.md` |
| Score | 151i, **519/520B**, **3 mism** — FLOOR |

```
need: lea edx,[eax+ecx] / mov ecx,[y] / mov [esp],edx
have: add ecx,eax       / mov [esp],ecx / mov ecx,[y]
```

The intervening ecx load is **`g_mapref.y`**, not v1. Two mutually exclusive
149/151 floors:

- **Index 121 (kept):** `Pos o` + direct `r.left = o.x + v[0]` + volatile v1 —
  v0 stays in ecx; dest-coalesces to `add ecx,eax` / store immediately.
- **Index 120:** named/`left = ox+v0` then store later — correct y-before-store
  schedule but v0 lands in edx (`add edx,eax`).

People block already runs **after** `pop edi / pop esi` (3-scratch lea is
possible). Silent post-add v0 uses DCE to index-120 edx coloring; observable
uses add an insn (`if (v0);` → `test`, 139/151). Still need a non-DCE’d v0
use in the add→y window with zero extra insn, or a dest-symbol that forces
3-address lea under post-pop allocation.

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

### LL3 — `Raster_ClipPoly` `0x0041ef60` (15/19 scope)

| Branch / file | `scope/LL3` · `LEGOLAND/coaster11.c` · tip `3db23e0b` |

**80i / 193/194B**, 44 mism (~86%). Default is `return count` (not `v`).
Cases 2–3 already emit unmerged `add ecx,OFF`. Case 1’s `p = g; p+4` blocks
tail merge but colors `g` in eax (the missing byte). `r = count` + `break`
gives original `dec/jne default` layout but hoists count into eax. Need
case-1 ecx **and** that layout together.

### LL3 — `Route_GetMassAndPower` `0x0041db90`

**70%**, 77i, 255/259B. Unchanged. **rt in ebp.** Acc `fstp`s over the rt arg
slot, not power `[esp+0x88]`. Root-copy / comma eval-at did not move it.

### LL7 — `Track_StepAlong` `0x00429f30` (14/17 scope)

| | |
| --- | --- |
| Branch / file | `scope/LL7` · `LEGOLAND/coaster13.c` |
| Tip | `0f653d1e` |
| Notes | `docs/lanes/scope-ll7.md` |
| Score | 70i, **226/232B**, audit **47**, ~72.5% |

Best: `hi = tol; … hi = t;` + late `t0`. Lands early `push out_t` and
`fstp [esp]`. Residual: push `tol` as hi placeholder (not `geom`); dword-move
`t0` into dead `from` **during** hi2 setup. Not formally floored — 6-byte gap.

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

1. **LL2 UpdateCommon** — `lea` vs `add`; silent v0 uses DCE or cost an insn.
2. **LL8 AddScriptString** — fail-tail shared allocation (floored unless new coloring).
3. **LL3 ClipPoly** — 193→194B; case-1 ecx vs default `return count` layout.
4. **LL7 StepAlong** — 6-byte placeholder/t0 gap (not floored).
5. **LL3 MassAndPower** — rt ebp / fstp slot.
6. **LL6 GetTrackSegment / AddSpanRecord** — size-exact floors; only with new ICF/IV levers.
7. **LL4 Span family / LL7 Slope+ShadeFill / LL3 Trace+ClipPlane** — last.

When a scope hits **N/N exact**, stop and report tip SHA for integrator merge.
Do **not** merge partial scopes yourself.

---

## Scoreboard (parked branches)

| scope | exact | tip (approx) | file |
| --- | ---: | --- | --- |
| LL1 | **22/22** | merged `main` | `logflume8.c` |
| LL2 | 5/6 | `7bea1842` | `logflume9.c` |
| LL3 | 15/19 | `3db23e0b` | `coaster11.c` |
| LL4 | 3/8 | `c81396e2` | `coastershade2.c` |
| LL5 | **3/3** | merged `main` | `castletrack2.c` |
| LL6 | 22/24 | `00779571` | `coaster12.c` |
| LL7 | 14/17 | `0f653d1e` | `coaster13.c` |
| LL8 | 12/13 | `f8c1f002` | `gameframe2.c` |

**WIP count in this wave:** 0+1+4+5+0+2+3+1 = **16 bodies**.

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
