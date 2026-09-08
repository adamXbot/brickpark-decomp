# Fable handoff — LL-wave open WIPs (2026-09-08)

**LL1, LL2, LL5, LL8 are on `main`.** Integrator merges a scope only at **N/N**.
**No Co-Authored-By trailers.**

```
export PATH="/usr/bin:/bin:/usr/sbin:/sbin:/opt/homebrew/bin:$PATH"
PY=/Users/systemadmin/.venvs/legoland/bin/python
export LEGOLAND_CL=/Users/systemadmin/Documents/Development/Github/alphateam/tools/wibo-msvc/cl
cd /Users/systemadmin/Documents/Development/Github/legoland/.worktrees/scope-llN
```

---

## Scoreboard

| scope | exact | tip | file | open |
| --- | ---: | --- | --- | ---: |
| LL1 | **22/22** | merged | `logflume8.c` | 0 |
| LL2 | **6/6** | merged | `logflume9.c` | 0 |
| LL3 | 16/19 | `5afbda03` | `coaster11.c` | 3 |
| LL4 | **6/8** | `66f05319` | `coastershade2.c` | **2** |
| LL5 | **3/3** | merged | `castletrack2.c` | 0 |
| LL6 | 22/24 | `20c6e48d` | `coaster12.c` | 2 |
| LL7 | 16/17 | `4d144e03` | `coaster13.c` | 1 |
| LL8 | **13/13** | merged | `gameframe2.c` | 0 |

**8 WIP bodies** left in the LL wave.

---

## Suggested attack order

1. **LL3 ClipPlane** — **FLOOR 39.2%** @`973278f2`; z-first↔ESCAPES, dest≠edi↔ebx=n, bits@0x2c↔dlt.
2. **LL6 `AddSpanRecord`** — **FLOOR 80.3%** @`20c6e48d`; ebx↔lea[+0x10] bind.
3. **LL4 FillFlat / Simpson** — FLOOR 98.1% / 80/81.
4. **LL7 FillPoly / GetTrackSegment / Mass / Trace** — floors.

---

## LL4 — tip `66f05319` · **6/8**

### Closed this stretch ✅

| addr | name | tip note |
| --- | --- | --- |
| 0x0041fba0 | Span_FillFlatZ | `74e58495` — after-ylast crow/zrow; n in esi→ebx reload |
| 0x0041fd80 | Span_FillShade | `d5da3ae5` — `{row,dead,pitch}`; **a[0],d[0],a[1],d[1]** interleave; n=flip; tag=pitch+pitch |
| 0x0041ff80 | Span_FillShadeZ | `745a5a11` — FlatZ homes + Shade n/tag latch; x/r4 interleave then z on else |

Also exact: Romberg, RetreatGeometry, GetCoasterTexture.

### `Span_FillFlat` `0x0041f8d0` — **FLOOR 104/106 = 98.1%** ⬅️

106i/307B, 10 mism. Tip **`66f05319`**.

**KEEP:** `{row,dead,volatile pitch}`, early ras, ushort color, a-then-d, asm lea done-block.
**Pins:** color after ylast+dead; `*(int*)&sl.pitch = g_zb_pitch` (non-vol write through
volatile field) before polys/color — keeps ebx=ramp pin.

**Hot lead:** F_hot_only `__asm { mov ebx,y }` between pix/color + C
`sl.row = ras + sl.pitch * y` → **106/107 = 99.1%** (310B). Color window + early
edi=ras + late pair exact; sole extra is C imul-site `mov ebx,y`.

**Core bind:** ebx through `imul` *and* `cmp ebx,[esi]` must be **y itself**. A
surviving copy (tag/grad) dies after imul and y rehomes to edi. Ordinary
`carrier = y` copy-props away (load stays at imul). Asm≠C-def of ebx; asm
`imul`/`lea` DCE’s early edi=ras. New codegen lever only.

**Confirmed dead (2026-09-08):** F_hot+C row = 106/107 only. F_hot+asm imul =
105i (edi DCE). F_hot+asm imul+C `ras+off` / volatile ras pin = 109–110i.
`yy`/`tag=y`/comma/`register` between pix/color sink load to imul (104/106).
`lea [ras+edx*2]` = address-of-slot (0x64). `#pragma optimize` g/gt/y/a off
inert or blows 0x60. Volatile color inert or breaks pin. Mid-fn `#pragma` illegal.

**Note:** FillShade keeps ebx=y from first load because ramp goes through eax into
`g_span_ramp`; FillFlat must keep ebx=ramp at `mov ax,[ebx+eax*2]` — that web does
not transfer. Color-load register-web / delayed-ras / early-asm-edi all ≤92%.

### `IntegrateSimpson` `0x00420200` — **FLOOR 80/81**

258/258B. Og-off fstp/esp swap. New codegen lever only.

**Confirmed dead (2026-09-08):** Og-off hard-glues `call; add esp,4; fstp fa`.
Og-on → fstp-first but `add esp,8` + loses fsubr / integer `push x` /
global-first fmul (~69%). Mid-fn `#pragma optimize` illegal. Only
`fstp; add esp,4` site in exe (TravelThisTick is `fstp [esp+N]`). Empty-if /
forceinline barrier / mixed-volatile fa / FP11 casts / loop_opt /
float_control / fenv / `/Os` / alloca / TravelThisTick clones — all ≤80/81.
Pragma letter sweep (g/t/y/s/p/a/w + combos on|off): **no `FSTP_ADD4`**; best
remain `ADD_FSTP` 80/81. Asm first-call gets order but pushes ebx/esi/edi (≤76%).
Manual `__asm { add esp,4 }` after og-on fstp → order ok then second `add esp,8`.

---

## LL3 — `5afbda03` · 16/19

| addr | name | note |
| --- | --- | --- |
| 0x0041f050 | Span_ClipPlane | **FLOOR 73/186 = 39.2%**, 179i/604B |
| 0x0041db90 | MassAndPower | **FLOOR 70/77 = 90.9%** — restore_rt live-q + prt-lea + hist |
| 0x0041c940 | BsRoute_Trace | NG22 |

ClipPlane KEEP: 0x2c, ebx=n, and-ebx, latch jne, je nest, no ESCAPES, 2-ret, dest@+0x14,
cursor-first, drop pa, LEAVE reload, destrel-after-fsubr (plane→ecx), cur-home before dest,
**prev-first named sum** both lerp arms (`s=prev_abs; s+=na; t=prev/s` / `t=na/s`) →
`fld [esp+0x34]`.

MassAndPower KEEP: named `prt=&rt` then `q=*(volatile*)prt` in accel loop (early
`lea ebx,[eax+0x70]`); **restore_rt** second prt reload at exit (not `end-0x70`); hist
edx/ecx/and 0x3f. Still: after GetAccel `add esp,4` then `mov eax,[esp+0x80]`/`add eax,0x70`
(not `mov edx,[esp+0x84]` before cleanup / `lea eax,[edx+0x70]` / live `mov eax,edx`);
f24-home vs sample-lea order.

**Mass confirmed dead (2026-09-08):** end-after-`+=`; live-q/split without restore_rt; FP11;
eax occupy; integer `end-0x70`; q-pin in fmul window; StationDerivative; `if(1)`; comma-end;
end_rt_direct / end_vol_rt / end_char_plus; vol_sink_a; asm nop; capture_rt_slot;
volatile_q_assign; f24_first_order; help_nohoist `&rt` (delay-slot loads **temp** not rt
arg; 0x74 or fr+4 → ≤64); bar_acc (loads acc slot); live-out q/dst; amulK lea-in-slot;
helper hoist. Sticky: rt-arg reload into edx before `add esp,4`.

**Coupled residuals (new lever only):**
1. z-first needs live second float home → `fadd st(1)` ESCAPES; integer barriers CSE to y-then-z
2. dest≠edi → CSE back, or nxt=edi with dest in **ebx** (kills ebx=n)
3. bits@0x2c fights dlt already at +0x2c / n-as-abs; lands 0x44

**ClipPlane confirmed dead (2026-09-08):** dest-after-close, delayed nxt, n-slot←in ±
byte-abs, t through `in_v` (t@0x44), walk prev/nxt in place, k/delta union, late-in,
named stride/out; pz-cursor / z-then-y / only-nx; occupy-out; fn-scope k/dlt; drop
n-as-abs; n-slot+destrel-at-lerp; destrel-at-LEAVE/after-abs; Mass-prt analog on
dest/in/cur; dest-slot volatile; comma-vol dest; named out / out-as-int; dlt through
in_v/n/cursor; 8-byte bits — all CSE to 73 or drop KEEP.

## LL6 — `20c6e48d` · 22/24

**GetTrackSegment** `0x00424050` — **FLOOR 53/87 (60.9% strict)**, 87i/232B;
matchfull often **70/84**. Bind: head-empty `je fail1` and real fail2 fall-through
cannot coexist; shared `did_match` ICF-merges fail2→fail1.

**Confirmed dead (2026-09-08):** `if (!from_tail)` helper patterns (still 70/84);
`if (0)` outlining / flattened goto open / for(;;) / while(1) / volatile nest —
fold or invert empties; BL07 jump-into-`if (state==2){fail1:}` gets fail2 @87i but
retargets empties onto fail2 (other side of bind, not KEEP).

**AddSpanRecord** `0x00423200` — **FLOOR 49/61 = 80.3%**, 61i/175B
Levers: one cursor `saved` + `dst` before `ne>0` → push/dec ebx; `SortKey* nxt=k+1`
→ `add edx,8`/`[edx-8]`.
**Bind (confirmed):** `push ebx` and `lea eax,[ecx+0x10]` never co-occur on
this 61i body — two live pointer names make lea but count goes to esi (60i) and
still no kept `mov edx,ecx`. Asm lea blows the push-ecx frame. New lever only.

**Confirmed dead (2026-09-08):** one-pointer web → add+0x10 attractor (49/61);
two names / lim / helpers / ZCmd*+1 / int*+4 → lea but count→esi (60i);
drop volatile spill → lea + edi, push-ecx dies (33/57); volatile copy-break →
inverted regs (48/61); `__asm { mov edx,ecx }` → ebp frame (~27/65).

## LL7 — `4d144e03` · 16/17

**TrackShade_FillPoly** `0x00428860` — **178/254 = 70.1%** (254i, 764/771B, 0x70).
Climbs: a-d-first (`489fd5d3`), nar_first span test (`4d144e03`). Crow store exact.
Orig keeps `py` in eax and `zrow` in ecx across `store crow`, so freed edx takes
`g_shade_count` between stores. Any C nshade birth between stores → **66.1%**
add-swap attractor. After-both-stores nshade stays in **eax**.

**Confirmed dead (2026-09-08):** asymmetric short*/char* adds; nshade-as-tmask;
`yp=&g_shade_count`; bit1 IV; after-store eax occupy; `__asm` shade (firstX=3);
c-redef after both stores; py copy-split; zrow recompute; pre-add+Fst (65.9%);
nkeys-as-IV / register nshade / pointer-nshade / lut-then-n / aic / LoadN / Npy /
py-as-IV; src/dst occupy; zkeep_mid; named `a0` hoist (185/256); cr_occ / Fst;
ebx-pin between-store; y mem-inc; `for (nshade…)`; `if (nshade)`; else a[2]-first.
---

## Integrator

LL4 is **6/8** — **do not merge** until FillFlat + Simpson are `[OK]`.
Worktrees under `.worktrees/scope-llN`. `disasm.py` uses **RVA**.
