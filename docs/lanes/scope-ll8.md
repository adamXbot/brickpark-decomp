# Scope LL8 — gameframe / icon-UI leftovers

Branch `scope/LL8` from main @ `0facb9f5`. Object prefix `/tmp/sll8_`.
File `LEGOLAND/gameframe2.c`. Brief: `docs/SCOPE_LL8_frame_icon_ui.md`.

## Status

**13 of 13 exact.**

| address | name | insns | % | audit | marker |
| --- | --- | ---: | ---: | --- | --- |
| 0x0049a4a0 | Bloke_GoIdle | 3 | 100 | [OK] | FUNCTION |
| 0x0049a4d0 | Bloke_GoIdle2 | 3 | 100 | [OK] | FUNCTION |
| 0x0046ce00 | ResetInGameHelp | 5 | 100 | [OK] | FUNCTION |
| 0x0046cb20 | UnloadParkHelp | 13 | 100 | [OK] | FUNCTION |
| 0x0046d2f0 | ShowInstHelp | 15 | 100 | [OK] | FUNCTION |
| 0x0046dac0 | IconBarWheelDown | 36 | 100 | [OK] | FUNCTION |
| 0x0046db40 | IconBarWheelUp | 36 | 100 | [OK] | FUNCTION |
| 0x0046cff0 | ProcessInGameIconHelp | 44 | 100 | [OK] | FUNCTION |
| 0x0049cfc0 | FreeWorkerLists | 46 | 100 | [OK] | FUNCTION |
| 0x0046d590 | RemoveIconGroupRange | 56 | 100 | [OK] | FUNCTION |
| 0x0046ee00 | UpdateIconPage | 74 | 100 | [OK] | FUNCTION |
| 0x0046dd10 | SnapIconScroll | 92 | 100 | [OK] | FUNCTION |
| 0x004689f0 | AddScriptString | 91 | 100 | [OK] | FUNCTION |

## Names

- **AddScriptString** 0x004689f0: movie3.c / levelkw.c declare this address
  `NewScriptEvent`, but 0x00468910 already owns that name (sysstubs.c's
  calloc of a 0x44 ScriptEvent). The body intern's into
  `g_script_strings[]` / `g_script_string_count` and returns the index.
  `copy==0` stores `a` as-is; `copy && !a` stores NULL; `copy && a && !b`
  copies via `"%s"`; both strings are joined with `'@'` (0x40) via `"%s%c%s"`.
- **Bloke_GoIdle / Bloke_GoIdle2**: high-level table slots 0x18 / 0x19
  (`b->state = 14`). Distinct from the empty export `Bloke_DoNothing`
  (0x00484910).
- **ShowInstHelp**: ShowObjectHelp's sibling with `g_help_face_state = 1`.
  Null object returns without setting `g_help_requested`.
- **ResetInGameHelp**: `ResetScriptTimer` then zero `g_advisor_last` /
  `g_ingame_help_tick`.
- **UnloadParkHelp**: `g_last_hint = 0`; `UnloadSaveGameMap` if
  `g_state_810140` else `ResetLevelObjects`; then FreeScriptStrings,
  KillHelpText, KillAdvisorHelp, ResetInGameHelp; returns 1.
- **FreeWorkerLists**: drain gardener/mechanic orders, repair orders,
  then both worker lists.
- **IconBarWheelDown / IconBarWheelUp**: kept from input.c. Front-end
  (`g_game_mode==2 && g_screen_mode==3`) picks a column group from the
  cursor x (`<0xb2` → 0xc8, `<0x143` → 0x1f4, `<0x1d4` → 0x190 else
  0x12c); otherwise group 0xd2. Down is FindIcon(g+3)+ScrollUpInput;
  up is FindIcon(g+4)+ScrollDownInput. The third-band ternary is
  `x < 0x1d4 ? 0x190 : 0x12c` (setge/dec/and 0x64/add 0x12c).
- **ProcessInGameIconHelp**: names kept from callers. Null-test the
  GLOBAL (`g_focussed_icon`) so the pointer lands in EDX and flags in
  EAX (`test ah`). Rect store order `x0, y0=y-10, x1, y1`. Flag 0x1000
  walks `p->data->inst->f0` into ShowInstHelp; else ShowIdHelp.
- **RemoveIconGroupRange**: same walk as RemoveIconGroup on both lists;
  range `[group, group+7)`. First list increments dead `n`; `while (n--);`
  keeps `xor ebp / inc ebp`. Param is `int`.
- **UpdateIconPage**: `g_game_mode==1` shares `page=4` with switch case 4.
  Jump-table case order is **1, 0, 2, 3, 4** so Path strcmp is first.
  `strcmp(g_str_path, obj->name)` (s1 in eax). Page stays in ESI.
- **SnapIconScroll**: load the list head before testing delta. Finish is
  `if (best) { if (y == limit) return 0; return calc; }` so `je` fails
  into a shared `xor eax,eax` epilogue and success falls through.
- **AddScriptString**: closed. `table[count]=0; goto bump` duplicates the
  shared increment as eax-primary `mov ecx,eax / pop esi / inc ecx`.
  `slot = &table[count]; *slot = a` puts the !copy index in EDX. Both
  keep the one-string EDX store. Volatile idx / `return count++` were
  the 87/91 and 86/91 attractors; they cannot hold both polarities.

## Levers

- Icon-bar third-band: `x < K ? A : B` for setge, not `x >= K ? B : A`
  (setl). Same rule as math3d.c's ArcTan256 axis case.
- UnloadParkHelp: capture `g_state_810140` before clearing `g_last_hint`
  so the load stays first.
- ProcessInGameIconHelp: test the GLOBAL for null, not the local copy,
  to rotate the icon pointer eax→edx.
- UpdateIconPage: jump-table block order follows source case order;
  write Path (case 1) first. `strcmp(s1, s2)` not reversed.
- SnapIconScroll: inverted `jne` + copied fail tail is the residual;
  fall-through success with `if (y == limit) return 0` matches.
- AddScriptString: `n = count; count++; return n` is `mov eax,ecx / inc
  ecx`, not `lea`. One-string malloc is `not ecx` (strlen+1, no dec).
  `#pragma intrinsic(strlen)`. `!a` is `table[count]=0; goto bump` (not
  a private `return count++` / volatile n). `!copy` stores through
  `&table[count]` so the index is EDX. The two fail tails share
  allocation: volatile idx keeps one-string EDX but pins !copy to EAX.
  The `!copy` volatile `idx` web is load-bearing for one-string EDX
  (EAX holds malloc; the extra live int rotates the count load). Drop
  it and one-string goes ECX (74/91). Drop only the `!a` volatile and
  !a becomes ecx-primary: `mov ecx,[count] / pop edi / mov eax,ecx /
  inc ecx / pop esi / count then table` (83/91) — pop edi is early,
  but eax/ecx and store order flip. `count = n + 1`, `-~n`, `n - (-1)`
  all fuse to `lea ecx,[eax+1]`, sink pops, and move `!copy`'s `a` into
  EDX (73/91). `n + copy` / `count += copy` keep copy live, steal the
  prologue into ecx, and emit `add` not `inc`. Named table base,
  `s = a`, `read_count` / `read_count_p`, pointer-to-count, post-inc
  subscript, comma, `return count++`, store proven-0 `a`, volatile
  store of 0: DCE to this 85/91 or worse. Earlier rejects still stand:
  live `int z=0` (DCE, 19 mism); `static __inline` intern_null/intern_raw
  (lea, broke one-string EDX); helper-born `read_count()` (264B / 8 mism,
  !a back in ECX); `read_count_skip(copy)` (17 mism, one-string ECX);
  `t=n; t++` → lea; proven-0 `copy` cannot occupy EAX. Closed by
  `table[count]=0; goto bump` plus `slot=&table[count]; *slot=a`.
  Earlier floor probes (kept for the attractors they mapped):
  - Out-of-line helpers (`intern_null` / `intern_raw`, tagged bump_null /
    bump_raw) emit `call` (77/91, 234–241B). `/Gy` ICF cannot hide that:
    the original fail tails have no calls. Inlined twins with `count++`
    (not `n+1`) still allocate as one function: both tails go ecx-primary
    and one-string loses EDX (76–77/91). Different signatures / dummy
    `tag` args do not split the webs.
  - Distinct volatile objects per edge: volatile `a` / table-base /
    dummy locals / `b` / `copy` on `!copy` while keeping volatile count
    on `!a`. Best was volatile-`a` (82/90) or volatile table pointer
    (86/92, extra `mov eax,imm` base). A live `*(volatile*)&copy` reload
    occupies ECX then is clobbered; using it as `a+c` adds `add` and
    breaks one-string (83/92).
  - `goto fail_a` / `fail_copy` / `fail_bump` is the same CFG as today
    once `if (copy)` is kept. Inverting to `if (!copy) goto` wrecks the
    prologue (67/90).
  - `return g_script_string_count++` on `!a` only (volatile `idx` on
    `!copy`) is **86/91, exact 264B**, both/one-string still exact.
    `!a` becomes `mov ecx,[count] / pop edi / mov eax,ecx / pop esi /
    table[ecx]=0 / inc ecx / store count` — pop esi is in the right
    gap, but the tail is ecx-primary and `inc` is after the table store.
    That is the right size with the opposite polarity of the 267B floor
    (`mov eax,[count]` + reload). Combining it with any EDX force on
    `!copy` drops one-string or CSEs the bump.
  - `count = n; count++` / `volatile int c = n; ++c` / bump helper
    taking `n` all fold to `lea ecx,[eax+1]` and sink `pop ebx` before
    the lea (84/90). They also rotate `!copy`'s `a` into EDX.
  - Early `n = count` / keep-n-in-EDX across both-string, `register`,
    declaration swap, pointer-to-count, comma, `p = a`, extra `m`,
    `SetScriptEventText`-style push sinking: inert or first-break at
    the one-string EDX load (index 53). Sibling `SetScriptEventText`
    (0x00468b40) tests `copy` *before* the saved-register pushes, so
    its `!copy` arm has no esi/edi — not a usable pattern here.
  - **LL2 RTL `Fst` (2026-09-08):** `static __inline int Fst(int a, int b)
    { return a; }` (cdecl RTL, second arg first) closed UpdateCommon's
    ox-first moffs because the second arg was a **used store**
    (`o.x = ox`) and the return was a **different dest** (`o.y = v0`).
    The same lever does not split this function's fail-tail coloring.
    Best remains **87/91, 267/264B, 6 mism**. Still 12/13.
    - Unused / proven-const second args DCE or forward-substitute
      (LEVERS: inline wrappers do not create an uncoalesced temp).
      `Fst(count, copy)`, `FstP(a, copy)`, `FstP(a, z=copy)`,
      `PutK(i, a, copy)` (`v+k` with proven-0 `k`) are inert on the
      87/91 floor when volatile `idx` stays. `copy` is proven 0 on
      `!copy` and proven 1 on `!a`, so it cannot occupy EAX.
    - String-arg `Fst` on the already-exact both/one-string path
      (`strlen(FstP(a,…))`, `sprintf` args) is fully inert (87/91).
    - LL2 `Pos` spelling `o.y = Fst(count, o.x = a)` without volatile
      `idx` is 82/90 (loses one-string EDX). With volatile `idx` it
      is inert (87/91).
    - `Fst(count, table[count]=0)` / `Fst(count, z=0)` / `Id(count)`
      collapse to the known **ecx-primary** 85–86/91 attractors
      (either `return count++` or count-store-before-table).
    - H-style bump `{ c=n; c++; table[n]=0; count=c; return n; }`
      is eax-primary and table-then-count but **`lea ecx,[eax+1]`**
      and hoists `pop edi/esi/ebx` before the lea (84/90, 263B).
      Fst between `c=n` and `c++` does not split the lea. `pop esi`
      between `mov ecx,eax` and `inc ecx` is what the original uses
      to block lea; no RTL dummy created that gap on the eax-primary
      web.
    - `SS_HA(&table[count], a, copy)` substitutes for volatile `idx`
      (one-string EDX survives) but `!copy` count stays in EAX.
    - Any real EDX force on `!copy` (`FstP(a, z=0)` + `s+z` /
      `table[count+z]`, `Put` without the extra live int) drops
      one-string EDX and often CSEs the bump (76–82/90).
    Three !a attractors, none original:
      (1) volatile: eax-primary, **reload**, pops after inc, table
      then count — **87/91, 267B** (kept).
      (2) `return count++`: ecx-primary, copy, pops interleaved,
      table then inc — 86/91, 264B.
      (3) plain/`Fst`: ecx-primary, copy, inc then **count-store
      before table** — 85/91, 264B.
    Original is (1)'s eax-primary + (2)'s `mov ecx,eax / pop esi /
    inc ecx` + table-then-count. RTL evaluation order cannot hold
    both polarities at once while the extra live int that saves
    one-string EDX is the same `!copy` index web.
