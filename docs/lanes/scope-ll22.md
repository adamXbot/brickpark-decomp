# Scope LL22 — partials wave E: byte-divergent residuals

Branch `scope/LL22` from `origin/main`. Four WIP bodies, each
instruction-exact but not byte-exact at the start of the lane.
Brief: `docs/SCOPE_LL22_partials_bytediff.md`.

| address | name | file | before | after | marker |
| --- | --- | --- | --- | --- | --- |
| 0x00442980 | LoadAltTextures | mantex.c | 229i/748B, 78 mism | **229i/752B, 0 — `[OK]`** | `// FUNCTION:` |
| 0x00475630 | InsertChildIntoList | fpui.c | 68i/170B, 22 mism | 68i/172B, **1 mism** | `// WIP-FUNCTION:` |
| 0x00473b00 | UpdateControllerFromMouseData | input.c | 109i/280B, 13 mism | unchanged, floor bounded | `// WIP-FUNCTION:` |
| 0x004718c0 | ClampPopUpToScreen | misc3.c | 43i/143B, 3 mism | unchanged, floor re-confirmed | `// WIP-FUNCTION:` |

Method note that paid for itself: the lane's first move was to build a
**positional side-by-side** (`audit.py`'s own `true_extent` + `norm2`,
index for index, not a diff). `matchfull.py`'s LCS diff mis-aligns around
inserted/deleted instructions and made three of these bodies look far worse
than they are; the positional listing showed immediately that
`UpdateControllerFromMouseData` differs at 13 consecutive positions and
nowhere else, that `ClampPopUpToScreen` differs at exactly indices 25..27,
and that `LoadAltTextures` was already aligned end to end. Every subsequent
decision came from that listing.

## LoadAltTextures 0x00442980 — CLOSED, 229i/752B, 0 mismatches

Four reconstruction errors, in the order they were found. Each is a general
lever.

- **THREE SEPARATE BUFFERS, NOT ONE STRUCT — an aggregate's *arity* is a
  frame lever even when its size and address are identical.** The body was
  written with `struct { char path[0x100]; char line[0x200]; char name[0x60];
  char pad[0x20]; } buf;`, which reproduces the original's `sub esp,3c0` and
  puts `path`/`line`/`name` at exactly the original's `[esp+50h]`,
  `[esp+150h]`, `[esp+350h]`. It is still wrong: VC6 packs ONE aggregate and
  THREE aggregates in different orders, and that **rotates the whole
  scalar-home sequence by one**. With the struct, `listA`/`listB`/`text2`
  land at 0x2c/0x30/0x34 instead of the original's 0x30/0x34/0x2c, and the
  five address-taken `sscanf` ints rotate with them (position->slot map
  `[0x48,0x40,0x4c,0x3c,0x44]` for the struct, the original's
  `[0x40,0x4c,0x3c,0x44,0x48]` for three arrays). Three plain arrays —
  `char path[0x100]; char line[0x200]; char name[0x80];` — give all sixteen
  homes exactly. `name` is **0x80, not 0x60 plus padding**: 0x50+0x380
  lands on 0x3c0 with no slack.
- **`tolower(*p++)` — the increment must be INSIDE the call argument.** With
  `ch = (char)tolower(*p); p++;` VC6 sinks the `inc esi` below the `call`
  and the register assignment of the ENTIRE rest of the body rotates with
  it. This one change took the body from 55 mismatches to 11; `*q++ = ch`
  and a `do/while(ch)` form are equivalent, `int n` instead of `char ch`
  costs one more.
- **`countA` is read UNINITIALISED when the file is missing, exactly like
  `countB`.** There is no `else` arm. The original's `mov ebx,[esp+24h]` is
  a read of countA's own never-stored home, which VC6 overlapped onto
  `text`'s slot (as it overlaps countB's onto `file`'s). Writing
  `countA = (int)text;` is what produced the old `xor ebx,ebx`; deleting the
  else arm entirely fixed the load AND moved seven frame slots into place in
  one step (4/16 -> 8/16 homes correct). Three original bugs are now
  reproduced in this body: uninitialised `countA`, `countB` and `text2`.
- **The first `NameCompare` takes `text2`, a second copy of the pointer made
  inside `if (text)`.** `text` itself is only the `HeapFree_w` argument and
  countA's slot donor. This is what gives the sixteenth frame slot; without
  it VC6 has fifteen and every buffer address is 4 low.
- Minor but load-bearing: `q = path;` before `p = line;` (source order picks
  the two `lea`s), and `q += 4; listA = q;` rather than `listA = q + 4;`
  (the latter computes into eax and costs a `mov edx,eax` because the scan
  loop wants the cursor in edx).

**Negatives worth keeping (all measured on this body):**

- **Local declaration order is completely inert for spilled scalars**, and
  so are local NAMES. All permutations of the eleven pointer/int
  declarations, and renaming `palA`->`zzzpalA`, `tabA`->`q3` etc., are
  byte-identical. (Confirms scope AK's "frame-slot declaration order stays
  irrelevant".) Declaration order of the three ARRAYS is inert too — all six
  orders are byte-identical.
- **VC6 maps address-taken locals by ARGUMENT POSITION, not by identity.**
  All 120 permutations of `&a, &b, &c, &d, &e` in the `sscanf` call emit the
  *same five addresses in the same order*: permuting the argument list
  permutes the slot map with it, so the emitted `lea` sequence never moves.
  Separate `int a; int b; ...` declarations DO change the map, but only into
  other permutations of the same cycle. The only thing that fixed it was the
  struct->three-arrays change above.
- The order of the four `tabA`/`tabB`/`palA`/`palB` assignments inside the
  sex branches does not move a single frame slot (all 24 permutations), and
  neither does moving `text2 = text;` around inside the `if (text)` block
  (moving it past the `if (countA)` block costs 8 bytes and 3 more homes).
- A `*(char* volatile*)&text` read in the else arm DOES reproduce the
  original's `mov ebx,[text]` — but it puts `text` in the wrong slot
  (0x14 instead of 0x24) and leaves 11 mismatches. The uninitialised read is
  both more faithful and strictly better.

## InsertChildIntoList 0x00475630 — 22 mismatches -> 1

**Retirement withdrawn.** Five earlier lanes (~185 variants) endorsed
retiring this body because the missing `mov edx,eax` at index 46 "has
neither a phi nor interference behind it". They were looking for the wrong
thing: the original does not add a COPY to the single `p->obj` web, it has
TWO webs there.

- **A per-site volatile read gives a pushed argument its own register web.**
  `GetObjCost(*(ObjDef* volatile*)&p->obj)` takes the body from 22 audit
  mismatches / 170B to **1 mismatch / 172B** (matchfull 67/68 = 98.5%).
- **The independent confirmation that the two-web reading is right:** the
  earlier lanes recorded two separate register divergences — the missing
  argument copy AND the spilled first cost reloading into edx where the
  original uses ecx (`mov ecx,[esp+1ch]` / `cmp eax,ecx`). Adding the second
  web fixes BOTH at once. They were never two problems.
- What is left is one operand, not one instruction: original
  `mov edx,eax` (2 bytes, a copy), ours `mov edx,[esi+4]` (3 bytes, a
  reload). The open question is no longer "why is there a copy" but "how
  does a second pointer web get DEFINED BY A COPY rather than by a reload" —
  the same question `BoatingSchool_Tick` (ridecb5.c, idx 105/206) asks.
- **Inert on this base** (all 68 instructions, none producing a copy): the
  scope-V cancelled-pair copy web through a struct member (`t.o = (int)p->obj;
  t.o += (int)n; t.o -= (int)n;`), and its `^=` / `|= 0` / `&= -1` variants —
  VC6 folds every cancel before allocation; a plain `int` cancel; a
  pointer-typed struct member; the Codex-F `char*` cast; `(ObjDef volatile*)`
  and `(const volatile*)` POINTEE casts (only a volatile READ makes a web, a
  volatile pointee does not); a comma expression; an `ObjDef**` alias to
  `&p->obj`; the dead-`d` carrier. Naming the volatile read into a local
  (`b = *(volatile...)`) gives the frame-exact 171B but homes `b` (25).
- **Free volatile reads elsewhere are losses, and one is instructive:** on
  `a = n->obj` VC6 folds the parent compare into `cmp ecx,[eax+58h]` and
  loses an instruction; on `g_object_list` it costs 3; on the `p = p->next`
  latch it is inert. Moving the volatile to the parent compare instead of
  the argument re-plans the whole body (26 mismatches, `prev` moves to ebx).

## UpdateControllerFromMouseData 0x00473b00 — at its floor (13), now bounded

Unchanged, but the "two effects" reading is retired: it is ONE allocator
decision and the two halves are strictly **coupled**.

- 13 of 109 positions differ and every one lies in the window 64..77, the
  two low clamps. Indices 0..63 and 78..108 are identical.
- **The coupling, as a positive control:** drop the `volatile` cast on the
  second `accel_t1` read and indices 63..77 become EXACT — the 0 is in edx,
  the x-low clamp is `mov eax,[ecx+8] / xor edx,edx / cmp eax,edx / jge /
  mov [ecx+8],edx` and the y-low clamp is the memory form
  `cmp [ecx+0ch],edx`, byte for byte. The price is the `accel_t1` CSE temp,
  which can only live in ebp (ecx is `c`; the `abs` cdq pairs clobber
  eax/edx), so the prologue gains `push ebp`/`pop ebp` and the entry, delta
  write-back and button tail all re-plan: 53 of 109 positions differ. We can
  reach (ebp open, 0 in edx) or (ebp closed, 0 immediate) — never the
  original's (ebp closed, 0 in edx).
- The x-low clamp's extra `mov eax,[ecx+8]` and the y-low clamp's memory
  compare are not separate divergences: they are what VC6 emits once the 0
  is a register candidate (the load is scheduled to pair with the
  `xor edx,edx` that DEFINES the constant; the y clamp needs no definition,
  so it takes the memory form). Fix the candidate and all three lines
  follow, as the control proves.
- **Ruled out this lane** (byte-identical to the committed body): all six
  declaration orders of `dx`/`dy`/`accel` plus the one-line form; `dx`/`dy`
  as the two members of one flattened aggregate; a per-site
  `*(volatile int*)&c->x` read at the x-low clamp; `lo` carriers declared
  first and last. Reproducing the no-volatile ebp form exactly, so NOT a
  third state: an explicit `t1 = c->accel_t1` local used twice, and a
  `cc = c` alias pointer for the second read — **the Codex-F alias lever does
  defeat the CSE here, but VC6 then makes the same ebp choice as the plain
  double read**, which is a useful bound on that lever.

## ClampPopUpToScreen 0x004718c0 — floor re-confirmed (3)

3 of 43 positions differ and they are indices 25..27 and nothing else:
original `cmp esi,25h / jge / mov eax,25h`, ours `mov eax,25h / cmp esi,eax /
jge`. The two levers unavailable to the four earlier lanes were tried:

- The scope-V cancelled-pair copy web on the arm's constant — through a
  struct member (`t.v = 0x25; t.v += x; t.v -= x;`), through the same member
  with an `^=` pair, and through a plain `int` — is **inert**, all
  byte-identical (43i/143B/3). VC6 folds every cancel before web building,
  so it never separates the arm's `0x25` from the compare's. This is the
  same negative the `InsertChildIntoList` sweep produced, on a different
  shape: **the cancelled pair needs a use of a DIFFERENT WIDTH (scope V's
  byte store) to survive coalescing; on an all-`int` web it always folds.**
- `return (g_popup_y = 0x25), 0x25;` byte-identical; a volatile STORE to
  `g_popup_y` in the arm is worse (6). On the compare side `!(y >= 0x25)`
  and `y < 0x25 && 1` are byte-identical, `y - 0x25 < 0` costs 18, and a
  volatile read of `g_popup_y` in the compare costs 18 and 5 bytes.
- The `y <= 0x24` trade (2 mismatches / 144 bytes) remains rejected.

## Cross-file finding (NOT fixed here — outside this scope)

`tools/relocs.py LEGOLAND/input.c` reports two MISMATCH lines in `ScanMouse`
(0x00473a80), which the normalised gate passes as exact:

    i=26 original=0x0046db40 ours=0x0046dac0 symbol=_IconBarWheelDown
    i=29 original=0x0046dac0 ours=0x0046db40 symbol=_IconBarWheelUp

`gameframe2.c` names 0x0046dac0 `IconBarWheelDown` (body: `FindIcon(group+3)`
then `ScrollUpInput`) and 0x0046db40 `IconBarWheelUp` (body: `FindIcon(group+4)`
then `ScrollDownInput`), so the two names look inverted and `ScanMouse` emits
calls to each other's addresses. Both bodies are exact `// FUNCTION:` bodies
in files this scope does not own; the fix is a rename (byte-neutral), not a
call swap. Pre-existing on `main`, not introduced here.

## Gate

`audit.py` PASS on all four files; `[OK]` counts 18 (fpui.c, was 18),
5 (mantex.c, was 4), 7 (misc3.c, was 7), 4 (input.c, was 4).
`relocs.py` zero MISMATCH on fpui.c, mantex.c, misc3.c; input.c carries the
two pre-existing `ScanMouse` hits above. `/W3` clean on all four.
