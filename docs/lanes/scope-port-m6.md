# Scope PORT-M6 — the overlap hazard is smaller than it looked, and one spelling habit was costing three lists

> **Status: IN PROGRESS (claimed 2026-09-12 by PORT-M6).** Branch
> `scope/PORT-M6` from `feat/decomp-completion-next-steps-24a0d6` @ `9c646312`
> (the PORT-B7 merge). Matching-side lane: every change to `LEGOLAND/*.c` is
> either inside a `LEGOLAND_PORTABLE` arm or a declaration reformat that cannot
> move a byte, and every touched file is re-gated with `audit.py` +
> `relocs.py`. `portable/**` and `docs/HANDOFF.md` are PORT-B8's and the
> integrator's.

**Headline: A6's 167 "pairs where a single function stores through one of the
two" are 36 once the extent is the one the DECLARING TU declares, and 6 once it
is the bytes the function actually touches — but three of the rows that dropped
out were a real defect that had nothing to do with the optimiser.**
`pathmisc2.c` spells three pairs of pointers as one `extern` line with two
addresses in one comment, every address scanner in the tree reads the first
address for both declarators, and so in the portable build the A\* **open list
was the closed list** and the gardener and mechanic work-order **tails were
their heads**.

| | before | after |
| --- | --- | --- |
| `cdecl.py --overlaps` hazard pairs | 167 | 167 (the sweep is unchanged) |
| of those, overlapping as the declaring TU declares them | — | **36** |
| of those, touching the same BYTES in one function | — | **6**, all fixed |
| live wrong-address globals found on the way | — | **3** (`g_route_open`, `g_gardener_order_tail`, `g_mechanic_order_tail`) |
| `void`-declared `__stdcall` slots left in the tree | 1 (B7's, fixed at merge) | **1, and it is the fixed one** |
| cast forwarders in `gen-browser/aliases.c` | 82 | **72** (the 10 whose only disagreement was the RETURN type are gone) |
| census `unknown` rows wearing game names | 2 | **0** (`RES_LowRead`/`RES_LowSeek` renamed) |
| files touched | — | 19, `audit.py` **244 [OK] / 5 [WIP], 0 REJECT**, `relocs.py` **0 MISMATCH**, 3281/42 |
| ctest | wasm 16, native 10 | wasm **16/16**, native **10/10** |

---

## 1. The overlap hazard, measured instead of argued

### 1a. What the hazard actually is at `-O2`, and the answer

Four probes compiled with the browser target's own flags
(`emcc -O2`; `scratchpad/probe/alias.c`). `g_rec` is a record, `g_same` a second
extern name for its first word, `g_rec_b` a name for its second — the exact
shape `gen_link.py` produces for a merged split record
(`.set _g_same, _g_rec`).

| probe | C | `-O2` IR | right answer |
| --- | --- | --- | --- |
| `probe_same` | `g_rec.a = 7; return g_same;` | keeps the load | — |
| `probe_interior` | `g_rec_b = 9; return g_rec.b;` | keeps the load | — |
| **`probe_cse`** | `f = g_same; g_rec.a = f+1; return g_same - f;` | **`ret i32 0`** | **1** |

**The hazard is a LOAD that disappears, not a store that moves.** A store
through one name is not folded into a load of the other; a load of one name that
is *already available* from before a store to the other is CSE'd, and the
function returns a constant. Nothing about it is type-based: the two names are
distinct *objects*, so `-fno-strict-aliasing` (which the portable build already
passes) cannot help.

Which qualifier closes it (`scratchpad/probe/alias2.c`, same flags):

| the pair | `-O2` result | verdict |
| --- | --- | --- |
| neither `volatile` | `ret 0` | the defect |
| **`volatile` on the name that is READ** | both loads kept | **works** |
| `volatile` on the name that is WRITTEN | `ret 0` | **does NOT work** |
| both `volatile` | both loads kept | works |
| one declaration, no second name | `ret 1` | works, and costs nothing |

**A6 recipe 2 needs that sharpening: the `volatile` has to be on the LOAD.**
Putting it on the store side — the natural reading of *"`volatile` on the
lvalue"* — leaves the defect in place, because the store was never what was
being removed. Every fix in this lane therefore uses recipe 1 (one declaration),
which is both stronger and free.

### 1b. A6 recipe 3 is wrong: the same-address pairs are the clearest hazard

> *"Do not touch the 42 same-address pairs. Two names for one address are the
> same object to the linker and the same storage to the compiler; they were
> never the hazard."* — A6 §7

The linker half is right and the compiler half is not. `probe_cse` above **is** a
same-address pair and it miscompiles: the compiler cannot know the two names will
be resolved to one address, because that happens at link time, in
`gen/globals.c`'s `.set`. Two names for one address are 100 % byte overlap,
which makes them the *worst* case rather than the exempt one. Two of this lane's
six fixes are same-address pairs (`gameframe.c`, twice).

### 1c. Why 167 is really 36: the sweep uses the MERGED extent

`cdecl.py --overlaps` sizes the first name of a pair with
`ext[a.addr].size` — the largest extent ANY TU in the tree gives that address.
That is the right question for the generator ("which names end up inside one
block?") and the wrong one for the optimiser, which is decided entirely inside
one translation unit: clang only knows the extent *this* TU declares.

The clearest example is A6's own front-end table row for the info icons:

```c
/* iconui.c */
extern Icon* g_info_icon_g;   /* 0x007fdea4 */   <- FOUR bytes
extern Icon* g_info_icon_b;   /* 0x007fdfc0 */   <- FOUR bytes, 0x11c later
```

Nine pairs, every one reported. They do not overlap: they are nine separate
pointer variables, and the `w` beside them is `g_info_icon_g->flags |= 0x400`,
a write *through* the pointer. The 376 comes from `popup.c` and `bighelp.c`,
which call 0x007fdea4 the whole `PopUpUI g_popup` record — and
`gen-browser/extents.md:122` says so per TU ("`g_info_icon_g` Icon = 4
(iconui.c:94)"). `iconui.c`'s optimiser has never heard of `g_popup`.

The same reasoning retires most of A6 §7's named front-end table:
`gpu.c` declares `DDraw* g_ddraw1` (4 bytes, and `g_drvcaps` is 8 bytes later);
`blitmisc.c`/`surface.c` put `g_render_clip` at exactly `+0x6c` =
`sizeof(DDSurfaceDesc)`, i.e. adjacent, not inside; `fpui.c`, `bigscreens.c`
and `screencb.c`'s sprite-slot arrays are arrays of 4-byte `Sprite*`;
`movie.c`/`popupmisc.c`/`castleobj.c` declare `g_edit_changed` as a plain `int`.

Re-running the sweep with each name's own declared extent
(`scratchpad/port-m6-true-overlaps.py` — it imports `cdecl.py` and changes one
line):

```
116 pairs overlap AS THE DECLARING TU DECLARES THEM (40 same-address, 76 interior)
 34 of them have one function that STORES through one name
```

plus 2 whose first name this parser cannot size in its own TU
(`scratchpad/port-m6-unsized.py`: `eventtick.c`'s `extern char g_obj_list[]` and
`logflume2.c`'s forward-declared `EditCursorRec`) — **36** to judge by hand, not
167.

### 1d. Why 36 is really 6: an opaque call is a barrier, and disjoint bytes are disjoint

Two further facts cut the 36 down, and both are properties of the *function*,
not of the declarations:

1. **An external global is visible to every callee.** Between a store to one
   name and an access to the other, any call to a function clang cannot see is a
   full barrier: the callee may read or write either global, so neither access
   can move across it and no load can be forwarded over it. Every `ridecb*.c` /
   `unref4.c` / `buildtick.c` / `eventtick.c` row is this case — `g_edit_cursor`
   is only ever *address-passed* (`ValidateCursor(&g_edit_cursor, cls)`,
   `CursorIsValid`, `SetCursorError`), so all of its real memory traffic happens
   inside callees, behind barriers.
2. **Overlapping objects are not overlapping bytes.** `logflume.c` declares the
   6196-byte `g_edit_cursor` AND `g_mapref` (+0x1404) AND `g_8003f0` (+0x1830),
   but `LFTrack_Update` only touches the record at `.footprint`
   (+0x1414..+0x1428). Reordering accesses to disjoint bytes changes nothing.

Six rows survive both, and each was checked against the real `-O3` IR before and
after the fix with `scratchpad/port-m6-order.py`, which prints the sequence of
IR memory operations per symbol (`L`oad / `S`tore / `M`emcpy / `&` address
taken) next to the sequence the C asks for.

### 1e. The six, and the fix

All six are recipe 1 — spell the word once in the portable arm, keep the shipped
spelling in the `#ifndef` arm. Two of them are *deliberate VC6 codegen levers*
whose own comments in those files explain why they exist, which is exactly why
they cannot be collapsed for both builds.

| TU | function | the pair | why it is a hazard | `-O3` IR before → after |
| --- | --- | --- | --- | --- |
| `gamemain.c` | `ResetCurProfileDefaults` | `g_cur_profile` (0x110) / `g_vol_speech`, `g_vol_music`, `g_vol_sfx` (+0x24,+0x28,+0x2c) | `memset(&g_cur_profile, 0, sizeof)` covers all three, then stores them; sinking the memset zeroes the volumes | `g_cur_profile: M` + `g_vol_*: S` → `g_cur_profile: MSSS`, the volume names gone |
| `fpui3.c` | `PU_ToolB` | `g_query_cursor` (0x1834) / `g_query_block` (+0x1404) | `saved = g_query_cursor` copies all 6196 bytes including the block written two lines later, and `g_query_cursor = saved` restores them | `M&&&M` + `SS` → `MSS&&&M` |
| `misc3.c` | `PopUpCanDelete` | the same pair | the same save/probe/restore shape | `M&&M` + `SS` → `MSS&&M` |
| `gameframe.c` | `HandleMapClick` | `g_edit` / `g_edit_mode` — **same address** | the leading test reads the word through `g_edit.mode`, the switch writes it through `g_edit_mode` | `L` + `LLSSL` → `LLSSL` |
| `gameframe.c` | `HandleMapClick` | `g_sel_bpos` / `g_sel_bpos_wide` — **same address** | written through `g_sel_bpos.b`, read as a dword through `g_sel_bpos_wide` | `SSSSSS&L&` + `L` → `SSSSSS&L&` (the two reads became one 2-byte load, which is now legal) |
| `logflume9.c` | `LFTrack_CommitPlacement` | `g_lf_place_cursor_a`/`_b` / `g_lf_commit_a`/`_b` (+0x1830) | each commit word IS that cursor's `next`, cleared through one name and then written through `cur->next`, where `cur` provably points at the cursor | one more store on the record, the commit names gone |

In every case the second name has **no IR memory operations left** and the
record's sequence gained exactly what the second name lost — one object, program
order by construction.

The fixes are all the same shape, a `#define` in the portable arm so that no call
site moves:

```c
/* fpui3.c / misc3.c */
#ifndef LEGOLAND_PORTABLE
extern QueryBlock   g_query_block;    /* 0x00811564 */
#else
#define g_query_block (*(QueryBlock*)&g_query_cursor.raw[0x1404]) /* 0x00811564 */
#endif
```

`gameframe.c`'s two sit immediately after the record declarations they fold into
(`g_edit` at line 782, `g_sel_bpos` at 794) and so only rewrite the uses in
`HandleMapClick` and below; the earlier `g_edit_mode` reads at lines 585-586 are
in a function that does not touch `g_edit` at all, so they are not a pair.

### 1f. The three that were not an optimiser question at all

```c
/* pathmisc2.c, as it stood */
extern RouteNode *g_route_closed, *g_route_open;               /* 0x00668fc4, 0x00668fc0 */
extern WorkOrder *g_mechanic_orders, *g_mechanic_order_tail;    /* 0x0079a8c0, 0x0079a8c4 */
extern WorkOrder *g_gardener_orders, *g_gardener_order_tail;    /* 0x0079a8b0, 0x0079a8b4 */
```

Three lines, six objects, **three address comments that every scanner in the
tree reads as one**. `cdecl.py`, `linkreport.py`, `gen_link.py` and `callees.py`
all take one address per declaration line, so the second declarator silently
inherits the first one's address — and on the first line the addresses are in
DESCENDING order, so the inheritance is not even harmless rounding.
`gen-browser/globals.c` before this lane:

```c
/* 0x00668fc4 .data 8 bytes (+g_route_closed, g_route_open) */
__asm__(".globl _g_route_open\n.set _g_route_open, _g_closed_head\n");

/* 0x0079a8b0 .data 4 bytes (+g_gardener_orders) */
unsigned int g_gardener_order_tail[1];       /* and g_gardener_orders aliases IT */
/* ... and a separate, unreferenced g_gardener_orders_tail at 0x0079a8b4 */
```

So in the portable build:

* **`g_route_open` WAS `g_route_closed`.** The route search keeps an open list at
  0x00668fc0 sorted by `f` and a closed list at 0x00668fc4 (`simcore.c:47`,
  `:158-159`, `:678-679`, `workers3.c:89`). One word for both means every node is
  on both lists at once — every bloke's path, every frame.
* **`g_gardener_order_tail` WAS `g_gardener_orders`**, and the same for the
  mechanics. `NewGardenerOrder` writes the tail and then the head, so the head is
  always the newest order and the list is permanently one element long: a
  gardener or mechanic can only ever see the most recent job.

The right addresses are corroborated twice over without needing the
disassembly: **`simcore2.c:18-19` declares the same two names, one per line,
with `/* 0x00668fc4 */` and `/* 0x00668fc0 */`**, and `savechunks.c:362,440`
names the tails `g_gardener_orders_tail` / `g_mechanic_orders_tail` at
0x0079a8b4 / 0x0079a8c4. `workorder.c:15-16` and `workorder2.c:19` spell the
whole layout out in prose.

**Fixed for both builds** — the C was never wrong, only unreadable to the
scanners, and one declarator per line with its own address comment is the tree's
own rule (HANDOFF §3: *"Always give an `extern` a trailing `/* 0x0044xxxx */`
comment"*). Splitting a multi-declarator `extern` cannot change VC6's output, and
`audit.py` on `pathmisc2.c` is 16 `[OK]` with `relocs.py` 0 MISMATCH either way.
`gen-browser/globals.c` after:

```c
/* 0x00668fc0 .data 4 bytes (+g_route_open)   */ -> .set _g_route_open,   _g_open_head
/* 0x00668fc4 .data 8 bytes (+g_route_closed) */ -> .set _g_route_closed, _g_closed_head
/* 0x0079a8b0 .data 4 bytes */                    g_gardener_orders
/* 0x0079a8b4 .data 4 bytes (+g_gardener_orders_tail) */ g_gardener_order_tail
/* 0x0079a8c0 .data 4 bytes */                    g_mechanic_orders
/* 0x0079a8c4 .data 4 bytes (+g_mechanic_orders_tail) */ g_mechanic_order_tail
```

**The class is closed, and it was exactly three lines tree-wide.**
`scratchpad/port-m6-multidecl.py` is the sweep — every `extern` line with more
than one data declarator and more than one address in its trailing comment — and
it prints nothing now. Worth keeping as a round gate: it is four lines of regex
and the defect is invisible to every byte-level check there is, because the C is
correct and only the *tooling's* reading of it is wrong.

### 1g. The rest of the 36, with the reason each is not a fix

| TU(s) | pair | why not |
| --- | --- | --- |
| `ridecb2/5/6/7/8/9.c`, `unref4.c` | `g_edit_cursor` / `g_edit_cursor_rect` (+0x1414), `_next` (+0x1830), `g_ui_flags` (+0x1828) | the record is only address-passed to `ValidateCursor`/`CursorIsValid`/`SetCursorError`; those calls are barriers |
| `buildtick.c`, `eventtick.c` | `g_obj_list` / `g_view` (+0x1404), `g_draw_state` (+0x1830) | same — `g_obj_list` is `&`-only in both functions |
| `logflume.c`, `logflume9.c` | `g_edit_cursor` / `g_mapref` (+0x1404), `g_8003f0` (+0x1830) | the record is touched only at `.footprint` (+0x1414..+0x1428): disjoint bytes |
| `logflume2.c` | `g_edit_cursor` / `g_ui_flags` | `&`-only; `g_ui_flags`'s `LS` is one read-modify-write of itself |
| `screencb2.c` | `g_query_cursor` / `g_query_block` | the six reads are of the block (+0x1404) and the only store is `g_query_cursor.next` (+0x1830): disjoint |
| `screens3.c` | `g_cur_save_slot_wide` (dword at 0x0080ffe4) / `g_save_type` (+0x1) | the dword read is an argument to `LoadDateIntoTempProfile` and the byte store is after that call returns |
| `pathmisc2.c` | the three "same-address" rows | **not same-address at all** — §1f |
| the other 131 of the 167 | — | do not overlap as their own TU declares them (§1c) |

One residue worth naming for a future lane: the sweep only follows a *name*. It
cannot see a store through a POINTER that provably points at one of the pair, and
`logflume9.c`'s `cur->next` is exactly that shape — it was found by reading the
function, not by the sweep. A pointer-aware version of `--overlaps` would be a
real improvement and is not a small change.

---

## 2. The two PORT-B6 findings: one is a shipped bug, one is not a bug

### 2a. G1 — `SetVidAnim(NULL)`: the ORIGINAL dereferences first and tests later

`tools/disasm.py original/legoland.exe 0x43dc0`:

```
0x00443dc1  mov esi, [esp+8]          ; esi = clip
0x00443dc6  xor edi, edi
0x00443dc8  mov [0x665f68], edi       ; the four stores, unconditional
0x00443dce  mov [0x665f6c], edi
0x00443dd4  mov [0x665f64], edi
0x00443dda  mov [0x665eec], edi
0x00443de0  mov eax, [esi+0x20]       ; <-- clip->stop, NO test of esi first
0x00443de3  cmp eax, edi
0x00443de5  je  0x443ded
0x00443de7  push esi ; call eax       ;     clip->stop(clip)
0x00443ded  mov eax, [0x665f5c]       ; if (g_advisor_clip) { close; = 0 }
   ...
0x00443e08  cmp esi, edi              ; <-- if (clip), three statements later
0x00443e0a  mov [0x665f5c], esi       ;     g_advisor_clip = clip
0x00443e10  je  0x443e23
0x00443e12  mov edx, [esi+0x1c]       ;     clip->video
0x00443e1b  call 0x49e412             ;     AVIStreamGetFrameOpen
0x00443e20  mov [esi+0x14], eax
```

`advisor.c`'s body reproduces that statement for statement, in that order.
**Verdict: a shipped game bug, not a recovery error.** The instruction stream is
what the gates compare, so the test stays where the author put it in the VC6 arm;
`InitAdvisorMovies` ends with `SetVidAnim(g_ad_blink)` and `g_ad_blink` is 0
whenever `AD_Blink.avi` did not load, so as shipped that is an access violation
on Windows. Guarded in the portable arm only:

```c
#ifndef LEGOLAND_PORTABLE
    if (clip->stop)
#else
    if (clip && clip->stop)
#endif
        clip->stop(clip);
```

B6's prediction about the optimiser is confirmed by measurement, not argued. The
portable `-O3` IR of `SetVidAnim` **before** the guard has no test at all before
the call — clang used the unguarded `clip->stop` load to prove `clip != NULL`
and folded the later `if (clip)` away:

```llvm
15:                                         ; the `if (clip)` is GONE
  store ptr %0, ptr @g_advisor_clip
  %16 = getelementptr inbounds nuw i8, ptr %0, i32 28
  %17 = load ptr, ptr %16
  %18 = tail call ptr @AVIStreamGetFrameOpen(ptr %17, ptr @g_advisor_bmi)
```

and **after** it the branch is back:

```llvm
17:
  store ptr %0, ptr @g_advisor_clip
  br i1 %2, label %23, label %18              ; %2 = icmp eq ptr %0, null
```

That is why `portable/src/hostwin/avifil32.c`'s `AVIStreamGetFrameOpen` returns 0
without looking at its argument. **That defence is no longer load-bearing** —
though leaving it is still right; a stub should not follow a pointer.

### 2b. G2 — `OpenMovie`'s `pfile` is not uninitialised on any reachable rung

`tools/disasm.py original/legoland.exe 0x76460`:

```
0x0047648b  push eax                  ; path
0x0047648c  push ecx                  ; ecx = lea [esp+0x18] = &pfile
0x0047648d  call 0x49e400             ; AVIFileOpenA
0x00476492  test eax, eax
0x00476494  je   0x4764a1             ; success -> carry on
0x00476496  cmp  [0x668f98], ebx      ; failure -> the AVIFileExit rung ...
0x0047649c  jmp  0x4765d3             ;            ... and return 0
0x004764a1  mov  eax, [esp+0x10]      ; <-- pfile, read ONLY on the success path
```

The original tests the HRESULT and returns before `pfile` is ever read, exactly
as `movie.c` does. Every rung that reaches `AVIFileRelease(pfile)` — including
the no-video one — is downstream of a *successful* `AVIFileOpenA`, and a
successful `AVIFileOpenA` has written `*ppfile`.

**Verdict: neither a game bug nor a recovery error, and nothing to change.**
B6's own note concedes the control flow (*"Unreachable from a failed open —
`OpenMovie` returns first"*), and so do `avifil32.c`'s comments and its
`AVIFileOpenA` implementation. What is left is a *host contract*, already
honoured and already documented in that file: the shim must not return 0 from
`AVIFileOpenA` without writing `*ppfile`. `advisor.c`'s `LoadAdvisorMovie` is the
same shape and gets the same verdict.

---

## 3. `RES_LowRead` / `RES_LowSeek` are `ReadFile` / `SetFilePointer`, and now say so

They were never game functions. `/* [0x4ab104] */` and `/* [0x4ab264] */` are IAT
thunk addresses, `portable/tools/win32_imports.txt` already names those two slots
`SetFilePointer` and `ReadFile`, `memdb.c`'s own comment said `SetFilePointer` —
and, decisively, **three other game TUs import the same two slots under their
real names already**:

```
data2.c:310    __declspec(dllimport) int __stdcall ReadFile(int h, void* buf, unsigned int n,
data2.c:313    __declspec(dllimport) int __stdcall SetFilePointer(int h, int off, int* hi,
coaster7.c:366, unref3.c:175    the same ReadFile row
```

So `res.c`, `memdb.c` and `sweep4.c` were the outliers, and there was never a
name to invent. **Renamed for both builds** — 5 declarations and 3 call sites
across the three files:

* The names are the only thing that changed. `__declspec(dllimport)` stays, for
  the reason `res.c`'s own comment gives (*"a plain extern compiles to a direct
  rel32 call and will not match"*), and the relocation targets the same thunk, so
  the byte gate has nothing to see: `res.c` 2 `[OK]`, `memdb.c` 14 `[OK]`,
  `sweep4.c` 13 `[OK]`, 0 MISMATCH on all three.
* **The parameter types are deliberately left alone.** HANDOFF §3: an extern's
  types are a caller-side codegen lever. `res.c`'s `int n` and `data2.c`'s
  `unsigned int n` are both `i32` on wasm32, so there is nothing to gain from
  aligning them and an audit to lose.
* `sweep4.c`'s pair are declarations with no call site in that TU; renamed for
  the census.
* `memdb.c` also defines a *game* function called `RES_SetFilePointer`
  (0x00489d70) — a different name, no collision.

**Result**: the portable link binds the RES read path straight to `kernel32.c`'s
real `SetFilePointer` / `ReadFile`, and the census's last two `unknown` rows
(A6 §6, PORT-C finding 4) are gone. `win32_imports.txt` needs no new row: the
game now uses the names the table already has.

**For the integrator / PORT-B8 — three places are now dead code or stale prose**
(all harmless where they are, just unreferenced; `portable/**` is not this lane's
to edit):

* `portable/src/hostwin/kernel32.c:779-793` — the `RES_LowSeek` / `RES_LowRead`
  forwarders and the comment above them.
* `portable/tests/support/ll_test_host.c:197-210` — the same pair for the test
  host, plus "finding 1" in that file's header comment.
* `portable/README.md:171` and `docs/lanes/scope-port-c.md` §4 mention them in
  prose.

---

## 4. The two close-outs

### 4a. The void-declared slot class is closed, and the census is better than a grep

B7 found the class with `grep -rn 'void\s*(\s*__stdcall\s*\*' LEGOLAND/*.c`,
which sees one spelling of one declarator shape.
`scratchpad/port-m6-vtsweep.py` parses every `<ret> (__stdcall* <name>)(` in
`LEGOLAND/*.c` and `*.h` and groups by `(slot name, +0x offset comment)`:

```
368 __stdcall slot declarations, 116 distinct (name, offset)

slots declared `void`: 1
  LEGOLAND/spritemisc.c:27  Destroy      <-- inside the #ifndef arm; the #else
                                             arm at :34 says `long`

(name, offset) pairs two TUs type differently: 4
  AddRef     (+0x04): long / unsigned long   -> all i32 on wasm, harmless (33 decls)
  Destroy    (+0x08): long / void            -> spritemisc.c's two arms, i.e. B7's fix
  Release    (+0x08): long / unsigned long   -> all i32 on wasm, harmless (39 decls)
  SetPalette (+0x7c): int  / long            -> all i32 on wasm, harmless
```

**B7's claim holds and the class is closed.** The single remaining `void` is the
VC6 arm of B7's own fix, which is what it must be; the other three disagreements
are precisely the three B7 predicted and every one of them is `i32` on wasm, so
only a `void` can ever trap. `__stdcall` is the right filter, because COM is the
only thing in this tree that uses it and COM methods all return a value.

`MMTimeProc` re-verified independently, as the brief asked: `lifecycle.c:88`
declares it `void (__stdcall*)(unsigned int, unsigned int, unsigned long,
unsigned long, unsigned long)`; its only body is `music2.c:93`
`void __stdcall MIDITimerTick(unsigned int, unsigned int, unsigned long,
unsigned long, unsigned long)` (registered at `lifecycle.c:120`
`timeSetEvent(20, 10, MIDITimerTick, 0, 1)`); and `winmm.c:90`'s `LLTimeProc` is
the same shape. **Correct, as B7 said.**

A wider sweep over `void (*` / `void (__cdecl*` slots
(`scratchpad/port-m6-voidslot.py`) finds 119 void-returning function-pointer
declarators and **0** whose name is also a function the tree defines returning a
value. PORT-M3's `legoland_cbtypes` is the stronger statement for that half — it
type-checks 454 (slot, body) pairs at compile time, and it builds on both
targets in this branch.

### 4b. Cast forwarders: 82 → 72, and the 72 are one lane's work, not an afternoon's

`gen-browser/aliases.c` carries forwarders that **call through a cast**
(`((void (*)(unsigned int))&Body)(a0)`). Every one is a latent binaryen
`directize` failure: a constant-index `call_indirect` is rewritten into a direct
call with the wrong type and the module fails validation hundreds of functions
from the cause (PORT-M2 §4). The generator's own `manifest.md` table
(`## Cast forwarders: a stale name typed unlike its body`) is the work list.
Classified:

| the disagreement | rows |
| --- | --- |
| **return type only** | **10 — fixed in this lane** |
| argument list only | 61 |
| both | 11 |

The ten fixed. Each is one declaration in one file, stating the definition's real
return type under `#ifdef LEGOLAND_PORTABLE`; the VC6 arm is the shipped spelling
and cdecl discards EAX there, so no byte moves. `gamemain.c:175-179` already did
exactly this for `PauseCurrentTrack` and was the precedent to copy.

| stale name | declared in | the body at that address | fix |
| --- | --- | --- | --- |
| `ApplyMoodEvent` | `rides.c` | `simcore2.c:111 int AdjustMood(Bloke*, int, int)` | `void` → `int` |
| `InitRasterBuffer` | `data2.c` | `tri3d.c:371 void Render_SetPixelFormat(int)` | `void*` → `void` (the result is discarded at the one call site) |
| `Mat4_ToMat3` | `coaster13.c` | `coaster12.c:257 float* Mat3_FromMat4Transpose(float*, const float*)` | `void` → `float*` |
| `ResetFrontEnd` | `mapscreen.c`, `mapscreen2.c`, `screens3.c` | `int PauseCurrentTrack(void)` 0x00498920 | `void` → `int`, three files |
| `SetWorkingDirectory` | `coaster7.c` | `coastertiny.c:188 int CoasterModel_SetDirectory(const char*)` | `void` → `int` |
| `Span_EvalRange` | `coaster11.c` | `coastershade2.c:798 int Romberg_Evaluate(RombergFn, PhysOps*, float, float, PhysVec*)` | `void` → `int` — the same body PORT-M2 §4 fixed in `coaster10.c`, reached here under a third name |
| `sub_44db90` | `gameframe.c` | `goalstate.c:234 int AppraisalDueTick(void)` | `void` → `int` |
| `sub_46cb20` | `gameframe.c` | `gameframe2.c:166 int UnloadParkHelp(void)` | `void` → `int` |
| `sub_473640` | `gameframe.c` | `popupmisc.c:238 int ShowCursorErrorMessage(int)` | `void` → `int` |
| `sub_498b40` | `appraisalscreen.c`, `gameframe.c`, `screens3.c` | `narration2.c:622 int PumpNarration(void)` | `void` → `int`, three files; `screens3.c`'s declaration also had **no address comment** (HANDOFF §3) and now does |

Measured: `grep -c '))&' gen-browser/aliases.c` is **82 before, 72 after**, on a
regenerated closure, with the module still validating and ctest 16/16.

**The 72 that remain are a different shape and should not be rushed.** The caller
declares a vtable-slot name with an *empty* parameter list where the body takes
arguments:

```
| Bank_Activate   | Bank_TickCustomers | () -> void | (i32) -> void |
| TempleSlide_B0  | TempleSlide_Draw   | () -> void | (i32, i32, i32, i32, i32, i32) -> void |
```

and `interfaces.c:794,874` shows why: `extern void Bank_Activate(void);` then
`def->cb_activate = Bank_Activate;` — the name's address goes into an
object-definition table and is never called directly.

**And here is the reason not to pick these off one at a time.** Today the
forwarder is `() -> void`, which is also how the declaring file types the table
slot, so the indirect call through the slot *succeeds* and the body simply reads
a garbage argument. Correct the declaration alone and the forwarder becomes
`(i32) -> void` while the slot stays `() -> void` — and the call that works
today starts trapping. The declaration and the slot type have to move in the
same commit, which is PORT-M3's typed-callback pass continued rather than a
return-type edit. 72 rows, mostly in `interfaces.c` and the ride/attraction
files. **Recommend a PORT-M7 brief** with the generator's own manifest table as
the work list, `portable/tests/test_callback_types.c` as the compile-time gate
and `name_trap.py --continue` on `legoland_headless` as the runtime one.

---

## 5. Gate table

Every row measured on the committed tree, `LEGOLAND_CL` = the wibo VC6 `cl`.
`relocs.py` exits 2 on a clean file (bit 1 = "unresolved or skipped"), so the
column is a `grep -c MISMATCH`, per HANDOFF §4.

| file | item | what changed | `audit.py` | `relocs.py` MISMATCH |
| --- | --- | --- | --- | --- |
| `pathmisc2.c` | 1 | 3 multi-declarator externs split, one address each (both builds) | 16 `[OK]` | 0 |
| `gamemain.c` | 1 | `g_vol_*` addressed through `g_cur_profile` in the portable arm | 10 `[OK]` | 0 |
| `fpui3.c` | 1 | `g_query_block` through `g_query_cursor` | 7 `[OK]` | 0 |
| `misc3.c` | 1 | `g_query_block` through `g_query_cursor` | 7 `[OK]`, 1 `[WIP]` | 0 |
| `gameframe.c` | 1, 4 | `g_edit_mode` and `g_sel_bpos_wide` folded; 4 return types | 10 `[OK]` | 0 |
| `logflume9.c` | 1 | `g_lf_commit_a`/`_b` through the cursors' `next` | 6 `[OK]` | 0 |
| `advisor.c` | 2 | `if (clip && clip->stop)` in the portable arm | 10 `[OK]` | 0 |
| `res.c` | 3 | `RES_LowSeek`/`RES_LowRead` → `SetFilePointer`/`ReadFile` | 2 `[OK]` | 0 |
| `memdb.c` | 3 | `RES_LowSeek` → `SetFilePointer` | 14 `[OK]` | 0 |
| `sweep4.c` | 3 | both renamed (declarations only) | 13 `[OK]` | 0 |
| `rides.c` | 4 | `ApplyMoodEvent` return type | 13 `[OK]` | 0 |
| `data2.c` | 4 | `InitRasterBuffer` return type | 6 `[OK]` | 0 |
| `coaster13.c` | 4 | `Mat4_ToMat3` return type | 16 `[OK]`, 1 `[WIP]` | 0 |
| `coaster11.c` | 4 | `Span_EvalRange` return type | 17 `[OK]`, 2 `[WIP]` | 0 |
| `coaster7.c` | 4 | `SetWorkingDirectory` return type | 10 `[OK]` | 0 |
| `mapscreen.c` | 4 | `ResetFrontEnd` return type | 12 `[OK]` | 0 |
| `mapscreen2.c` | 4 | `ResetFrontEnd` return type | 2 `[OK]` | 0 |
| `screens3.c` | 4 | `ResetFrontEnd` + `sub_498b40` return types, one missing address comment added | 73 `[OK]` | 0 |
| `appraisalscreen.c` | 4 | `sub_498b40` return type | 1 `[WIP]` | 0 |
| **19 files** | | | **244 `[OK]`, 5 `[WIP]`, 0 REJECT / FAIL / COMPILE FAILED** | **0** |

Tree-level:

| check | result |
| --- | --- |
| exact marker SET before vs after (HANDOFF §4 `comm -23`) | **identical**, nothing lost, nothing gained |
| WIP marker SET before vs after | **identical** |
| duplicate marker addresses tree-wide | none |
| `tools/progress.py --check` | **3281 exact / 42 WIP**, 665/675 exports (98.5%); the report is regenerated in this branch because 19 files' line numbers moved |
| wasm: `ninja` + 5 extra targets + `ctest` | builds, **16/16** |
| native: `ninja` + `legoland_tests` + `legoland_cbtypes` + `ctest` | builds, **10/10** |
| `legoland_headless -nointro` under node, no `LL_TRAP_CONTINUE` | opens the window, enters the game loop, **no TRAP and no `unreachable`** (the repeated "cannot open output file" is the game's own `DebugPrintf`); it does not terminate, because the front end does not |
| `grep -c '))&' gen-browser/aliases.c` | 82 → **72** |
| both builds from a CLEAN directory (`rm -rf portable/build portable/build-wasm`) | wasm 16/16, native 10/10, 72 cast forwarders, the three addresses right -- so nothing above depends on a stale generated closure (A6 §10) |
| `cdecl.py --overlaps` before vs after | 372 -> **369** pairs, 233 -> **231** touched by one function; the diff is exactly the three `pathmisc2.c` lines and nothing else |
| generated closure: `g_route_open` / the two order tails | now at 0x00668fc0 / 0x0079a8b4 / 0x0079a8c4 |

The whole-tree `relocs.py --all` sweep is the integrator's (~30 min, alone); the
19 per-file runs above are clean and no byte in any of them changed.

---

## 6. For the integrator

1. **`pathmisc2.c` is a behaviour change in the portable build, and an
   intended one.** Three globals that resolved to the wrong address now resolve
   to the right one: the A\* open list, and the gardener and mechanic work-order
   tails. Nothing in the VC6 build moves. If a page session starts behaving
   differently around pathing or worker jobs, that is this.
2. **`scratchpad/port-m6-multidecl.py` deserves a place in the round
   checklist.** It is four lines, it takes a second, and the defect class it
   catches (a correct C declaration that every address scanner mis-reads) is
   invisible to `audit.py`, `relocs.py` and `verify.py` alike — the same blind
   spot the whole-tree relocs sweep exists for, one level further out.
3. **Three dead forwarders in `portable/**`** can go whenever convenient — §3.
4. **`avifil32.c`'s `AVIStreamGetFrameOpen` no longer has to defend itself**
   against the folded `if (clip)` — §2a. Keep the defence, drop the reasoning.
5. **A6 §7's recipe needs two corrections** before the next lane reads it: the
   `volatile` goes on the LOAD (§1a), and the same-address pairs are the
   clearest hazard rather than the exempt one (§1b). A6's §7 table of front-end
   pairs is also mostly merged-extent artefacts (§1c) — the rows that survive
   are in §1e.
6. **72 cast forwarders remain** and want their own lane — §4b. The generator's
   `manifest.md` table is the work list; it is a continuation of PORT-M3's
   typed-callback pass, not a return-type sweep.
7. `docs/LEGOLANDPROGRESS.HTML` is regenerated in this branch (line numbers
   moved in 19 files); `progress.py --check` is green on the committed tree.

## 7. Scratch

`port-m6-*` in the shared scratchpad. Four worth keeping:

* **`port-m6-multidecl.py`** — every `extern` line with two data declarators and
  two addresses in one comment. See §6.2.
* **`port-m6-true-overlaps.py`** — `cdecl.py --overlaps` with each name's own
  declared extent instead of the merged one, i.e. the optimiser's question
  rather than the generator's. 167 → 34 (+2 unsized, `port-m6-unsized.py`).
* **`port-m6-order.py`** — for one (TU, function, nameA, nameB), the sequence of
  `-O3` IR memory operations per symbol next to the sequence the C asks for.
  This is what turns "could the optimiser do this?" into a measurement, and it
  is 60 lines.
* **`probe/alias.c` + `probe/alias2.c`** — the proof that the hazard is a
  disappearing LOAD and that the `volatile` has to be on the read.

Also `port-m6-vtsweep.py` (§4a), `port-m6-voidslot.py` (§4a),
`port-m6-gate.sh` (the 19-file audit + relocs loop) and `port-m6-markers.sh`
(the HANDOFF §4 marker-set diff against the branch point).

---

## 8. Appendix: every hazard pair and its decision

Generated by `scratchpad/port-m6-decisions.py` on the committed tree.
`cdecl.py --overlaps` now reports **369** overlapping pairs (was 372) and **165**
where a single function stores through one of the two (was 167): the three
`pathmisc2.c` rows are gone, because they were never pairs (§1f), and two of those
three were hazard rows. **Nothing else in the sweep moved** -- the diff of the two
runs is exactly those three lines -- which is the cheapest proof that the portable
arms in §1e changed no declaration the sweep can see.

Reason codes:

* **FIXED** -- same bytes, same function, no opaque call between them; folded to
  one declaration in the portable arm (§1e). Nine rows for the six distinct
  hazards: `gamemain.c`'s one `memset` is three rows, one per volume.
* **no-overlap** -- the declaring TU's own extent for name A does not reach name
  B. A6's sweep sized A with the largest extent ANY TU in the tree gives that
  address, which is the generator's question and not the optimiser's (§1c).
* **barrier/disjoint** -- they do overlap as this TU declares them, but either the
  record is only address-passed, so every access to it is behind an opaque call
  (a full barrier for an external global), or the function touches different
  bytes through the two names (§1d, §1g).

Access letters are `r` read, `w` written, after any `.f` / `[i]` chain; a write
*through* a pointer name counts as `w` on that name, which is why nine of
`iconui.c`'s rows read as stores and are not.

| TU | name A | off | name B | function (A/B access) | decision | why |
| --- | --- | --- | --- | --- | --- | --- |
| `bigscreens.c` | `g_save_type_normal` | +0x4 | `g_save_type_free` | InitSavedGameScreen (w/w) | **no-overlap** | this TU declares g_save_type_normal as 4 bytes, g_save_type_free is +0x4 |
| `bigscreens.c` | `g_save_type_normal` | +0x8 | `g_save_corner_mask` | InitSavedGameScreen (w/w) | **no-overlap** | this TU declares g_save_type_normal as 4 bytes, g_save_corner_mask is +0x8 |
| `blitmisc.c` | `g_ddsd` | +0x6c | `g_render_clip` | PushRenderingStatusAndRelockVideoSurface (rw/r) | **no-overlap** | this TU declares g_ddsd as 108 bytes, g_render_clip is +0x6c |
| `buildtick.c` | `g_obj_list` | +0x1404 | `g_view` | ObjectIsBuilt (r/rw) | **barrier/disjoint** | section 1d |
| `buildtick.c` | `g_obj_list` | +0x1830 | `g_draw_state` | ObjectIsBuilt (r/w) | **barrier/disjoint** | section 1d |
| `castleobj.c` | `EditCursor` | +0x1828 | `g_8003e8` | Track_Update (r/w) | **no-overlap** | this TU declares EditCursor as 4 bytes, g_8003e8 is +0x1828 |
| `castleobj.c` | `EditCursor` | +0x1830 | `g_8003f0` | Castle_Update (r/w) | **no-overlap** | this TU declares EditCursor as 4 bytes, g_8003f0 is +0x1830 |
| `castleobj.c` | `EditMode` | +0x8 | `g_edit_class` | Track_Tick (w/w); Castle_Tick (w/w) | **no-overlap** | this TU declares EditMode as 4 bytes, g_edit_class is +0x8 |
| `castleobj.c` | `QueryCursor` | +0x1404 | `g_query_block` | Castle_Update2 (r/w) | **no-overlap** | this TU declares QueryCursor as 4 bytes, g_query_block is +0x1404 |
| `castleobj.c` | `g_castle_rec` | +0x10 | `g_829af0` | Castle_Add (rw/w) | **no-overlap** | this TU declares g_castle_rec as 4 bytes, g_829af0 is +0x10 |
| `castleobj.c` | `g_castle_rec` | +0x14 | `g_829af4` | Castle_Add (rw/w) | **no-overlap** | this TU declares g_castle_rec as 4 bytes, g_829af4 is +0x14 |
| `castleobj.c` | `g_castle_rec` | +0x18 | `g_829af8` | Castle_Destroy (w/r) | **no-overlap** | this TU declares g_castle_rec as 4 bytes, g_829af8 is +0x18 |
| `castleobj.c` | `g_castle_rec` | +0x24 | `g_829b04` | Castle_Destroy (w/r) | **no-overlap** | this TU declares g_castle_rec as 4 bytes, g_829b04 is +0x24 |
| `castleobj.c` | `g_castle_rec` | +0x4 | `g_829ae4` | Castle_Add (rw/rw) | **no-overlap** | this TU declares g_castle_rec as 4 bytes, g_829ae4 is +0x4 |
| `castleobj.c` | `g_castle_rec` | +0x8 | `g_castle_x` | Castle_Add (rw/w) | **no-overlap** | this TU declares g_castle_rec as 4 bytes, g_castle_x is +0x8 |
| `castleobj.c` | `g_castle_rec` | +0xa | `g_castle_y` | Castle_Add (rw/w) | **no-overlap** | this TU declares g_castle_rec as 4 bytes, g_castle_y is +0xa |
| `castleobj.c` | `g_castle_rec` | +0xa8 | `g_829b88` | Castle_Destroy (w/w); Castle_Add (rw/w); Castle_Remove (r/w) | **no-overlap** | this TU declares g_castle_rec as 4 bytes, g_829b88 is +0xa8 |
| `castleobj.c` | `g_castle_rec` | +0xac | `g_829b8c` | Castle_Destroy (w/rw); Castle_Add (rw/rw) | **no-overlap** | this TU declares g_castle_rec as 4 bytes, g_829b8c is +0xac |
| `castleobj.c` | `g_castle_rec` | +0xc | `g_829aec` | Castle_Add (rw/w) | **no-overlap** | this TU declares g_castle_rec as 4 bytes, g_829aec is +0xc |
| `castleobj.c` | `g_castle_rec` | +0xc0 | `g_829ba0` | Castle_Destroy (w/w); Castle_Add (rw/w); Castle_Remove (r/w) | **no-overlap** | this TU declares g_castle_rec as 4 bytes, g_829ba0 is +0xc0 |
| `castleobj.c` | `g_castle_rec` | +0xc4 | `g_829ba4` | Castle_Destroy (w/rw); Castle_Add (rw/rw) | **no-overlap** | this TU declares g_castle_rec as 4 bytes, g_829ba4 is +0xc4 |
| `catapult.c` | `g_edit_changed` | +0x8 | `g_edit_object` | Catapult_Select (w/rw) | **no-overlap** | this TU declares g_edit_changed as 4 bytes, g_edit_object is +0x8 |
| `coaster.c` | `EditCursor` | +0x1404 | `g_mapref` | Track_Update (r/rw) | **no-overlap** | this TU declares EditCursor as 4 bytes, g_mapref is +0x1404 |
| `coaster.c` | `EditCursor` | +0x1414 | `g_edit_footprint` | Track_Update (r/w) | **no-overlap** | this TU declares EditCursor as 4 bytes, g_edit_footprint is +0x1414 |
| `coaster.c` | `EditCursor` | +0x1828 | `g_8003e8` | TrackH_Update (r/w); TrackHP_Update (r/w) | **no-overlap** | this TU declares EditCursor as 4 bytes, g_8003e8 is +0x1828 |
| `coaster.c` | `EditCursor` | +0x1830 | `g_8003f0` | TrackH_Update (r/rw); TrackHP_Update (r/rw); Track_Update (r/rw) | **no-overlap** | this TU declares EditCursor as 4 bytes, g_8003f0 is +0x1830 |
| `coaster.c` | `g_mapref` | +0x10 | `g_edit_footprint` | Track_Update (rw/w) | **no-overlap** | this TU declares g_mapref as 8 bytes, g_edit_footprint is +0x10 |
| `eventtick.c` | `g_obj_list` | +0x1404 | `g_view` | PlaceScriptObject (r/rw) | **no-overlap** | this TU declares g_obj_list as unsized bytes, g_view is +0x1404 |
| `fpui.c` | `g_theme_legoland_off` | +0x4 | `g_theme_western_off` | Load_Interface_ThemeIcons (w/w); UnLoad_Interface_ThemeIcons (rw/rw) | **no-overlap** | this TU declares g_theme_legoland_off as 4 bytes, g_theme_western_off is +0x4 |
| `fpui.c` | `g_theme_legoland_off` | +0x8 | `g_theme_castle_off` | Load_Interface_ThemeIcons (w/w); UnLoad_Interface_ThemeIcons (rw/rw) | **no-overlap** | this TU declares g_theme_legoland_off as 4 bytes, g_theme_castle_off is +0x8 |
| `fpui.c` | `g_theme_legoland_off` | +0xc | `g_theme_adv_off` | Load_Interface_ThemeIcons (w/w); UnLoad_Interface_ThemeIcons (rw/rw) | **no-overlap** | this TU declares g_theme_legoland_off as 4 bytes, g_theme_adv_off is +0xc |
| `fpui.c` | `g_theme_legoland_on` | +0x4 | `g_theme_western_on` | Load_Interface_ThemeIcons (w/w); UnLoad_Interface_ThemeIcons (rw/rw) | **no-overlap** | this TU declares g_theme_legoland_on as 4 bytes, g_theme_western_on is +0x4 |
| `fpui.c` | `g_theme_legoland_on` | +0x8 | `g_theme_castle_on` | Load_Interface_ThemeIcons (w/w); UnLoad_Interface_ThemeIcons (rw/rw) | **no-overlap** | this TU declares g_theme_legoland_on as 4 bytes, g_theme_castle_on is +0x8 |
| `fpui.c` | `g_theme_legoland_on` | +0xc | `g_theme_adv_on` | Load_Interface_ThemeIcons (w/w); UnLoad_Interface_ThemeIcons (rw/rw) | **no-overlap** | this TU declares g_theme_legoland_on as 4 bytes, g_theme_adv_on is +0xc |
| `fpui3.c` | `g_query_cursor` | +0x1404 | `g_query_block` | PU_ToolB (rw/w) | **FIXED** | same bytes in one function, no barrier between |
| `gameframe.c` | `g_edit` | +0x0 | `g_edit_mode` | HandleMapClick (r/rw) | **FIXED** | same bytes in one function, no barrier between |
| `gameframe.c` | `g_edit_mode` | +0x8 | `g_edit_object` | HandleMapClick (rw/r) | **no-overlap** | this TU declares g_edit_mode as 4 bytes, g_edit_object is +0x8 |
| `gameframe.c` | `g_hit_type` | +0x4 | `g_icon_value` | InGameFrameBody (rw/rw) | **no-overlap** | this TU declares g_hit_type as 4 bytes, g_icon_value is +0x4 |
| `gameframe.c` | `g_hit_type` | +0x8 | `g_hit_cell` | InGameFrameBody (rw/rw) | **no-overlap** | this TU declares g_hit_type as 4 bytes, g_hit_cell is +0x8 |
| `gameframe.c` | `g_sel_bpos` | +0x0 | `g_sel_bpos_wide` | HandleMapClick (rw/r) | **FIXED** | same bytes in one function, no barrier between |
| `gamemain.c` | `g_cur_profile` | +0x24 | `g_vol_speech` | ResetCurProfileDefaults (w/w) | **FIXED** | same bytes in one function, no barrier between |
| `gamemain.c` | `g_cur_profile` | +0x28 | `g_vol_music` | ResetCurProfileDefaults (w/w) | **FIXED** | same bytes in one function, no barrier between |
| `gamemain.c` | `g_cur_profile` | +0x2c | `g_vol_sfx` | ResetCurProfileDefaults (w/w) | **FIXED** | same bytes in one function, no barrier between |
| `goldrush.c` | `g_edit_changed` | +0x8 | `g_edit_object` | CastleLevel1_SelectForPlacement (w/rw); Fort_SelectForPlacement (w/rw); Temple_SelectForPlacement (w/rw); GoldRush_SelectForPlacement (w/rw) | **no-overlap** | this TU declares g_edit_changed as 4 bytes, g_edit_object is +0x8 |
| `gpu.c` | `g_ddraw1` | +0x184 | `g_helcaps` | InitHostSystemGPU (r/rw) | **no-overlap** | this TU declares g_ddraw1 as 4 bytes, g_helcaps is +0x184 |
| `gpu.c` | `g_ddraw1` | +0x31c | `g_gdi_obj0` | KillHostSystemGPU (rw/r) | **no-overlap** | this TU declares g_ddraw1 as 4 bytes, g_gdi_obj0 is +0x31c |
| `gpu.c` | `g_ddraw1` | +0x320 | `g_gdi_obj1` | KillHostSystemGPU (rw/r) | **no-overlap** | this TU declares g_ddraw1 as 4 bytes, g_gdi_obj1 is +0x320 |
| `gpu.c` | `g_ddraw1` | +0x324 | `g_gdi_obj2` | KillHostSystemGPU (rw/r) | **no-overlap** | this TU declares g_ddraw1 as 4 bytes, g_gdi_obj2 is +0x324 |
| `gpu.c` | `g_ddraw1` | +0x328 | `g_gdi_obj3` | KillHostSystemGPU (rw/r) | **no-overlap** | this TU declares g_ddraw1 as 4 bytes, g_gdi_obj3 is +0x328 |
| `gpu.c` | `g_ddraw1` | +0x4 | `g_ddraw` | KillHostSystemGPU (rw/rw) | **no-overlap** | this TU declares g_ddraw1 as 4 bytes, g_ddraw is +0x4 |
| `gpu.c` | `g_ddraw1` | +0x8 | `g_drvcaps` | InitHostSystemGPU (r/rw) | **no-overlap** | this TU declares g_ddraw1 as 4 bytes, g_drvcaps is +0x8 |
| `iconui.c` | `g_info_icon_g` | +0x11c | `g_info_icon_b` | DisableInfoPopUPIcons (w/w) | **no-overlap** | this TU declares g_info_icon_g as 4 bytes, g_info_icon_b is +0x11c |
| `iconui.c` | `g_info_icon_g` | +0x120 | `g_info_icon_i` | DisableInfoPopUPIcons (w/w) | **no-overlap** | this TU declares g_info_icon_g as 4 bytes, g_info_icon_i is +0x120 |
| `iconui.c` | `g_info_icon_g` | +0x128 | `g_info_icon_h` | DisableInfoPopUPIcons (w/w) | **no-overlap** | this TU declares g_info_icon_g as 4 bytes, g_info_icon_h is +0x128 |
| `iconui.c` | `g_info_icon_g` | +0x134 | `g_info_icon_c` | DisableInfoPopUPIcons (w/w) | **no-overlap** | this TU declares g_info_icon_g as 4 bytes, g_info_icon_c is +0x134 |
| `iconui.c` | `g_info_icon_g` | +0x138 | `g_info_icon_a` | DisableInfoPopUPIcons (w/w) | **no-overlap** | this TU declares g_info_icon_g as 4 bytes, g_info_icon_a is +0x138 |
| `iconui.c` | `g_info_icon_g` | +0x13c | `g_info_icon_f` | DisableInfoPopUPIcons (w/w) | **no-overlap** | this TU declares g_info_icon_g as 4 bytes, g_info_icon_f is +0x13c |
| `iconui.c` | `g_info_icon_g` | +0x144 | `g_info_icon_j` | DisableInfoPopUPIcons (w/w) | **no-overlap** | this TU declares g_info_icon_g as 4 bytes, g_info_icon_j is +0x144 |
| `iconui.c` | `g_info_icon_g` | +0x15c | `g_info_icon_e` | DisableInfoPopUPIcons (w/w) | **no-overlap** | this TU declares g_info_icon_g as 4 bytes, g_info_icon_e is +0x15c |
| `iconui.c` | `g_info_icon_g` | +0x4 | `g_info_icon_d` | DisableInfoPopUPIcons (w/w) | **no-overlap** | this TU declares g_info_icon_g as 4 bytes, g_info_icon_d is +0x4 |
| `joust.c` | `g_edit_changed` | +0x8 | `g_edit_object` | Joust_SelectForPlacement (w/rw); TempleSlide_SelectForPlacement (w/rw) | **no-overlap** | this TU declares g_edit_changed as 4 bytes, g_edit_object is +0x8 |
| `logflume.c` | `g_edit_changed` | +0x8 | `g_edit_object` | LFEntrance_Tick (w/rw) | **no-overlap** | this TU declares g_edit_changed as 4 bytes, g_edit_object is +0x8 |
| `logflume.c` | `g_edit_cursor` | +0x1404 | `g_mapref` | LFTrack_Update (rw/r) | **barrier/disjoint** | section 1d |
| `logflume.c` | `g_edit_cursor` | +0x1830 | `g_8003f0` | LFTrack_Update (rw/w); LFEntrance_Update (r/w) | **barrier/disjoint** | section 1d |
| `logflume2.c` | `g_cursor_mode` | +0x8 | `g_cursor_def` | LFPiece_TickCommon (w/w) | **no-overlap** | this TU declares g_cursor_mode as 4 bytes, g_cursor_def is +0x8 |
| `logflume2.c` | `g_edit_cursor` | +0x1828 | `g_ui_flags` | LFPiece_TickCommon (r/w) | **no-overlap** | this TU declares g_edit_cursor as unsized bytes, g_ui_flags is +0x1828 |
| `logflume9.c` | `g_edit_cursor` | +0x1404 | `g_mapref` | LFPiece_UpdateCommon (rw/r) | **barrier/disjoint** | section 1d |
| `logflume9.c` | `g_edit_cursor` | +0x1830 | `g_8003f0` | LFPiece_UpdateCommon (rw/rw) | **barrier/disjoint** | section 1d |
| `logflume9.c` | `g_lf_place_cursor_a` | +0x1830 | `g_lf_commit_a` | LFTrack_CommitPlacement (r/w) | **FIXED** | same bytes in one function, no barrier between |
| `logflume9.c` | `g_lf_place_cursor_b` | +0x1830 | `g_lf_commit_b` | LFTrack_CommitPlacement (r/w) | **FIXED** | same bytes in one function, no barrier between |
| `mapscreen.c` | `g_screen_popup` | +0x4 | `g_cur_screen` | InitScreens (w/rw) | **no-overlap** | this TU declares g_screen_popup as 4 bytes, g_cur_screen is +0x4 |
| `mechrides.c` | `g_copters_path0` | +0x10 | `g_copters_path4` | Copters_Create (w/w) | **no-overlap** | this TU declares g_copters_path0 as 4 bytes, g_copters_path4 is +0x10 |
| `mechrides.c` | `g_copters_path0` | +0x14 | `g_copters_layers` | Copters_Create (w/rw) | **no-overlap** | this TU declares g_copters_path0 as 4 bytes, g_copters_layers is +0x14 |
| `mechrides.c` | `g_copters_path0` | +0x4 | `g_copters_path1` | Copters_Create (w/w) | **no-overlap** | this TU declares g_copters_path0 as 4 bytes, g_copters_path1 is +0x4 |
| `mechrides.c` | `g_copters_path0` | +0x8 | `g_copters_path2` | Copters_Create (w/w) | **no-overlap** | this TU declares g_copters_path0 as 4 bytes, g_copters_path2 is +0x8 |
| `mechrides.c` | `g_copters_path0` | +0xc | `g_copters_path3` | Copters_Create (w/w) | **no-overlap** | this TU declares g_copters_path0 as 4 bytes, g_copters_path3 is +0xc |
| `mechrides.c` | `g_edit_changed` | +0x8 | `g_edit_object` | SafariRide_Select (w/rw); SpiderRide_Select (w/rw); SpinningBarrels_Select (w/rw); SpaceTower_Select (w/rw); PlaneRide_Select (w/rw); Copters_Select (w/rw) | **no-overlap** | this TU declares g_edit_changed as 4 bytes, g_edit_object is +0x8 |
| `mechrides.c` | `g_safari_zstate` | +0x4 | `g_safari_zframe` | SafariRide_Create (w/w) | **no-overlap** | this TU declares g_safari_zstate as 4 bytes, g_safari_zframe is +0x4 |
| `mechrides.c` | `g_sbarrel_pivot_x` | +0x4 | `g_sbarrel_pivot_y` | SpinningBarrels_Create (w/w) | **no-overlap** | this TU declares g_sbarrel_pivot_x as 4 bytes, g_sbarrel_pivot_y is +0x4 |
| `mechrides.c` | `g_spacetower_state` | +0x4 | `g_spacetower_spr2` | SpaceTower_Create (w/w) | **no-overlap** | this TU declares g_spacetower_state as 4 bytes, g_spacetower_spr2 is +0x4 |
| `mechrides.c` | `g_spacetower_state` | +0x8 | `g_spacetower_spr3` | SpaceTower_Create (w/w) | **no-overlap** | this TU declares g_spacetower_state as 4 bytes, g_spacetower_spr3 is +0x8 |
| `mechrides.c` | `g_spacetower_state` | +0xc | `g_spacetower_phase` | SpaceTower_Create (w/w) | **no-overlap** | this TU declares g_spacetower_state as 4 bytes, g_spacetower_phase is +0xc |
| `misc3.c` | `g_query_cursor` | +0x1404 | `g_query_block` | PopUpCanDelete (rw/w) | **FIXED** | same bytes in one function, no barrier between |
| `movie.c` | `g_edit_changed` | +0x4 | `g_game_mode` | EnterParkPlayMode (w/w) | **no-overlap** | this TU declares g_edit_changed as 4 bytes, g_game_mode is +0x4 |
| `movie.c` | `g_edit_changed` | +0x8 | `g_edit_object` | EnterParkPlayMode (w/w) | **no-overlap** | this TU declares g_edit_changed as 4 bytes, g_edit_object is +0x8 |
| `objmap.c` | `g_cursor_mapref` | +0x10 | `g_edit_footprint` | CalcBasicObjectCursor (r/w) | **no-overlap** | this TU declares g_cursor_mapref as 4 bytes, g_edit_footprint is +0x10 |
| `objmap.c` | `g_edit_changed` | +0x8 | `g_edit_object` | SetEditObject (w/w) | **no-overlap** | this TU declares g_edit_changed as 4 bytes, g_edit_object is +0x8 |
| `objmap.c` | `g_edit_cursor` | +0x1414 | `g_edit_footprint` | CalcBasicObjectCursor (r/w) | **no-overlap** | this TU declares g_edit_cursor as 1 bytes, g_edit_footprint is +0x1414 |
| `pathobj2.c` | `g_edit_changed` | +0x8 | `g_edit_object` | SetEditObjectFromElem (w/rw) | **no-overlap** | this TU declares g_edit_changed as 4 bytes, g_edit_object is +0x8 |
| `popupmisc.c` | `g_edit_changed` | +0x8 | `g_edit_object` | SelectNextBuildObject (w/r) | **no-overlap** | this TU declares g_edit_changed as 4 bytes, g_edit_object is +0x8 |
| `ridecb2.c` | `g_edit_cursor` | +0x1414 | `g_edit_cursor_rect` | MonkeyFish_CalcCursor (r/rw) | **barrier/disjoint** | section 1d |
| `ridecb2.c` | `g_edit_cursor` | +0x1830 | `g_edit_cursor_next` | MonkeyFish_CalcCursor (r/w) | **barrier/disjoint** | section 1d |
| `ridecb2.c` | `g_edit_cursor_origin` | +0x10 | `g_edit_cursor_rect` | MonkeyFish_CalcCursor (r/rw) | **no-overlap** | this TU declares g_edit_cursor_origin as 8 bytes, g_edit_cursor_rect is +0x10 |
| `ridecb5.c` | `g_edit_cursor` | +0x1414 | `g_edit_cursor_rect` | Roads_CalcCursor (r/rw) | **barrier/disjoint** | section 1d |
| `ridecb5.c` | `g_edit_cursor` | +0x1830 | `g_edit_cursor_next` | Roads_CalcCursor (r/w) | **barrier/disjoint** | section 1d |
| `ridecb5.c` | `g_edit_cursor_origin` | +0x10 | `g_edit_cursor_rect` | Roads_CalcCursor (r/rw) | **no-overlap** | this TU declares g_edit_cursor_origin as 8 bytes, g_edit_cursor_rect is +0x10 |
| `ridecb6.c` | `g_edit_cursor` | +0x1414 | `g_edit_cursor_rect` | BsWater_CalcCursor (r/rw) | **barrier/disjoint** | section 1d |
| `ridecb6.c` | `g_edit_cursor` | +0x1830 | `g_edit_cursor_next` | BsWater_CalcCursor (r/w) | **barrier/disjoint** | section 1d |
| `ridecb6.c` | `g_edit_cursor_origin` | +0x10 | `g_edit_cursor_rect` | BsWater_CalcCursor (r/rw) | **no-overlap** | this TU declares g_edit_cursor_origin as 8 bytes, g_edit_cursor_rect is +0x10 |
| `ridecb7.c` | `g_edit_cursor` | +0x1414 | `g_edit_cursor_rect` | JcWater_CalcCursor (r/rw) | **barrier/disjoint** | section 1d |
| `ridecb7.c` | `g_edit_cursor` | +0x1830 | `g_edit_cursor_next` | JcWater_CalcCursor (r/w) | **barrier/disjoint** | section 1d |
| `ridecb7.c` | `g_edit_cursor_origin` | +0x10 | `g_edit_cursor_rect` | JcWater_CalcCursor (r/rw) | **no-overlap** | this TU declares g_edit_cursor_origin as 8 bytes, g_edit_cursor_rect is +0x10 |
| `ridecb8.c` | `g_edit_changed` | +0x8 | `g_edit_object` | Food_SelectForPlacement (w/rw); Brolly_SelectForPlacement (w/rw); BoatingSchool_SelectForPlacement (w/w); Mermaid_SelectForPlacement (w/w); Roads_SelectForPlacement (w/w); ZebraCrossing_SelectForPlacement (w/w); Pump_SelectForPlacement (w/w); BsWater_SelectForPlacement (w/rw); Balloonz_SelectForPlacement (w/rw); Carousel_SelectForPlacement (w/rw); EarthSlide_SelectForPlacement (w/rw) | **no-overlap** | this TU declares g_edit_changed as 4 bytes, g_edit_object is +0x8 |
| `ridecb8.c` | `g_edit_cursor` | +0x1414 | `g_edit_cursor_rect` | Mermaid_CalcCursor (r/rw) | **barrier/disjoint** | section 1d |
| `ridecb8.c` | `g_edit_cursor` | +0x1830 | `g_edit_cursor_next` | Mermaid_CalcCursor (r/w) | **barrier/disjoint** | section 1d |
| `ridecb8.c` | `g_edit_cursor_origin` | +0x10 | `g_edit_cursor_rect` | Mermaid_CalcCursor (r/rw) | **no-overlap** | this TU declares g_edit_cursor_origin as 8 bytes, g_edit_cursor_rect is +0x10 |
| `ridecb9.c` | `g_edit_changed` | +0x8 | `g_edit_object` | MechanicsHut_Select (w/rw); PottingShed_Select (w/rw) | **no-overlap** | this TU declares g_edit_changed as 4 bytes, g_edit_object is +0x8 |
| `ridecb9.c` | `g_edit_cursor` | +0x1414 | `g_edit_cursor_rect` | MonkeyTree_CalcCursor (r/rw) | **barrier/disjoint** | section 1d |
| `ridecb9.c` | `g_edit_cursor` | +0x1830 | `g_edit_cursor_next` | MonkeyTree_CalcCursor (r/w) | **barrier/disjoint** | section 1d |
| `ridecb9.c` | `g_edit_cursor_origin` | +0x10 | `g_edit_cursor_rect` | MonkeyTree_CalcCursor (r/rw) | **no-overlap** | this TU declares g_edit_cursor_origin as 8 bytes, g_edit_cursor_rect is +0x10 |
| `schoolcar4.c` | `g_cc_obj` | +0x4 | `g_cc_obj_len` | LoadCoasterModelSet (rw/r) | **no-overlap** | this TU declares g_cc_obj as 4 bytes, g_cc_obj_len is +0x4 |
| `schoolcar4.c` | `g_cc_txt` | +0x4 | `g_cc_txt_len` | LoadCoasterModelSet (rw/r) | **no-overlap** | this TU declares g_cc_txt as 4 bytes, g_cc_txt_len is +0x4 |
| `screencb.c` | `g_oct_tab_aa` | +0x10 | `g_oct_tab_ca` | OctopusCafe_Create (w/w) | **no-overlap** | this TU declares g_oct_tab_aa as 4 bytes, g_oct_tab_ca is +0x10 |
| `screencb.c` | `g_oct_tab_aa` | +0x14 | `g_oct_tab_cb` | OctopusCafe_Create (w/w) | **no-overlap** | this TU declares g_oct_tab_aa as 4 bytes, g_oct_tab_cb is +0x14 |
| `screencb.c` | `g_oct_tab_aa` | +0x18 | `g_oct_tab_da` | OctopusCafe_Create (w/w) | **no-overlap** | this TU declares g_oct_tab_aa as 4 bytes, g_oct_tab_da is +0x18 |
| `screencb.c` | `g_oct_tab_aa` | +0x1c | `g_oct_tab_db` | OctopusCafe_Create (w/w) | **no-overlap** | this TU declares g_oct_tab_aa as 4 bytes, g_oct_tab_db is +0x1c |
| `screencb.c` | `g_oct_tab_aa` | +0x20 | `g_oct_tab_ea` | OctopusCafe_Create (w/w) | **no-overlap** | this TU declares g_oct_tab_aa as 4 bytes, g_oct_tab_ea is +0x20 |
| `screencb.c` | `g_oct_tab_aa` | +0x24 | `g_oct_tab_eb` | OctopusCafe_Create (w/w) | **no-overlap** | this TU declares g_oct_tab_aa as 4 bytes, g_oct_tab_eb is +0x24 |
| `screencb.c` | `g_oct_tab_aa` | +0x28 | `g_oct_tab_fa` | OctopusCafe_Create (w/w) | **no-overlap** | this TU declares g_oct_tab_aa as 4 bytes, g_oct_tab_fa is +0x28 |
| `screencb.c` | `g_oct_tab_aa` | +0x2c | `g_oct_tab_fb` | OctopusCafe_Create (w/w) | **no-overlap** | this TU declares g_oct_tab_aa as 4 bytes, g_oct_tab_fb is +0x2c |
| `screencb.c` | `g_oct_tab_aa` | +0x30 | `g_oct_tab_ga` | OctopusCafe_Create (w/w) | **no-overlap** | this TU declares g_oct_tab_aa as 4 bytes, g_oct_tab_ga is +0x30 |
| `screencb.c` | `g_oct_tab_aa` | +0x34 | `g_oct_tab_gb` | OctopusCafe_Create (w/w) | **no-overlap** | this TU declares g_oct_tab_aa as 4 bytes, g_oct_tab_gb is +0x34 |
| `screencb.c` | `g_oct_tab_aa` | +0x38 | `g_oct_tab_ha` | OctopusCafe_Create (w/w) | **no-overlap** | this TU declares g_oct_tab_aa as 4 bytes, g_oct_tab_ha is +0x38 |
| `screencb.c` | `g_oct_tab_aa` | +0x3c | `g_oct_tab_hb` | OctopusCafe_Create (w/w) | **no-overlap** | this TU declares g_oct_tab_aa as 4 bytes, g_oct_tab_hb is +0x3c |
| `screencb.c` | `g_oct_tab_aa` | +0x4 | `g_oct_tab_ab` | OctopusCafe_Create (w/w) | **no-overlap** | this TU declares g_oct_tab_aa as 4 bytes, g_oct_tab_ab is +0x4 |
| `screencb.c` | `g_oct_tab_aa` | +0x8 | `g_oct_tab_ba` | OctopusCafe_Create (w/w) | **no-overlap** | this TU declares g_oct_tab_aa as 4 bytes, g_oct_tab_ba is +0x8 |
| `screencb.c` | `g_oct_tab_aa` | +0xc | `g_oct_tab_bb` | OctopusCafe_Create (w/w) | **no-overlap** | this TU declares g_oct_tab_aa as 4 bytes, g_oct_tab_bb is +0xc |
| `screencb.c` | `g_oct_tent_a` | +0x10 | `g_oct_tent_e` | OctopusCafe_Create (w/w) | **no-overlap** | this TU declares g_oct_tent_a as 4 bytes, g_oct_tent_e is +0x10 |
| `screencb.c` | `g_oct_tent_a` | +0x14 | `g_oct_tent_f` | OctopusCafe_Create (w/w) | **no-overlap** | this TU declares g_oct_tent_a as 4 bytes, g_oct_tent_f is +0x14 |
| `screencb.c` | `g_oct_tent_a` | +0x18 | `g_oct_tent_g` | OctopusCafe_Create (w/w) | **no-overlap** | this TU declares g_oct_tent_a as 4 bytes, g_oct_tent_g is +0x18 |
| `screencb.c` | `g_oct_tent_a` | +0x1c | `g_oct_tent_h` | OctopusCafe_Create (w/w) | **no-overlap** | this TU declares g_oct_tent_a as 4 bytes, g_oct_tent_h is +0x1c |
| `screencb.c` | `g_oct_tent_a` | +0x20 | `g_oct_kiosk` | OctopusCafe_Create (w/w) | **no-overlap** | this TU declares g_oct_tent_a as 4 bytes, g_oct_kiosk is +0x20 |
| `screencb.c` | `g_oct_tent_a` | +0x4 | `g_oct_tent_b` | OctopusCafe_Create (w/w) | **no-overlap** | this TU declares g_oct_tent_a as 4 bytes, g_oct_tent_b is +0x4 |
| `screencb.c` | `g_oct_tent_a` | +0x8 | `g_oct_tent_c` | OctopusCafe_Create (w/w) | **no-overlap** | this TU declares g_oct_tent_a as 4 bytes, g_oct_tent_c is +0x8 |
| `screencb.c` | `g_oct_tent_a` | +0xc | `g_oct_tent_d` | OctopusCafe_Create (w/w) | **no-overlap** | this TU declares g_oct_tent_a as 4 bytes, g_oct_tent_d is +0xc |
| `screencb2.c` | `g_query_cursor` | +0x1404 | `g_query_block` | BoatingSchool_DrawSelection (w/r) | **barrier/disjoint** | section 1d |
| `screencb4.c` | `g_edit_changed` | +0x8 | `g_edit_object` | DrivingSchool_SelectForPlacement (w/rw) | **no-overlap** | this TU declares g_edit_changed as 4 bytes, g_edit_object is +0x8 |
| `screencb6.c` | `g_edit_changed` | +0x8 | `g_edit_object` | JungleCruise_SelectForPlacement (w/w); JcMonkeyTree_SelectForPlacement (w/w); JcMonkeyFish_SelectForPlacement (w/w); JcWater_SelectForPlacement (w/rw) | **no-overlap** | this TU declares g_edit_changed as 4 bytes, g_edit_object is +0x8 |
| `screencb6.c` | `g_edit_cursor` | +0x1828 | `g_ui_flags` | JcMonkeyTree_SelectForPlacement (r/w); JcMonkeyFish_SelectForPlacement (r/w); JcWater_SelectForPlacement (r/w) | **no-overlap** | this TU declares g_edit_cursor as 1 bytes, g_ui_flags is +0x1828 |
| `screens3.c` | `g_cur_save_slot` | +0x1 | `g_save_type` | LoadAcceptInput (r/w) | **no-overlap** | this TU declares g_cur_save_slot as 1 bytes, g_save_type is +0x1 |
| `screens3.c` | `g_cur_save_slot_wide` | +0x1 | `g_save_type` | LoadAcceptInput (r/w) | **barrier/disjoint** | section 1d |
| `screens3.c` | `g_edit_changed` | +0x4 | `g_game_mode` | BriefIconInput (w/r); QueryIconInput (w/r); EraserIconInput (w/r); MapIconInput (w/rw); OptionsIconInput (w/rw); LegolandThemeInput (w/r); WesternThemeInput (w/r); CastleThemeInput (w/r); AdventureThemeInput (w/r) | **no-overlap** | this TU declares g_edit_changed as 4 bytes, g_game_mode is +0x4 |
| `screens3.c` | `g_temp_name` | +0x1e | `g_temp_name_len` | SaveGameOkInput (rw/w) | **no-overlap** | this TU declares g_temp_name as 30 bytes, g_temp_name_len is +0x1e |
| `simcore.c` | `g_edit_cursor` | +0x1404 | `g_edit_cursor_origin` | UpdateMapDrag (r/w) | **no-overlap** | this TU declares g_edit_cursor as 1 bytes, g_edit_cursor_origin is +0x1404 |
| `simcore.c` | `g_edit_cursor` | +0x1414 | `g_edit_cursor_rect` | UpdateMapDrag (r/w) | **no-overlap** | this TU declares g_edit_cursor as 1 bytes, g_edit_cursor_rect is +0x1414 |
| `simcore.c` | `g_edit_cursor` | +0x1830 | `g_edit_cursor_next` | UpdateMapDrag (r/w) | **no-overlap** | this TU declares g_edit_cursor as 1 bytes, g_edit_cursor_next is +0x1830 |
| `simcore.c` | `g_edit_cursor_origin` | +0x10 | `g_edit_cursor_rect` | UpdateMapDrag (w/w) | **no-overlap** | this TU declares g_edit_cursor_origin as 8 bytes, g_edit_cursor_rect is +0x10 |
| `surface.c` | `g_ddsd` | +0x6c | `g_render_clip` | PushRenderingStatusAndLockVideoSurface (rw/r); PopRenderingStatus (rw/r) | **no-overlap** | this TU declares g_ddsd as 108 bytes, g_render_clip is +0x6c |
| `tinystubs.c` | `g_info_icon_g` | +0x128 | `g_info_icon_h` | DisablePopUpInputs (w/w) | **no-overlap** | this TU declares g_info_icon_g as 4 bytes, g_info_icon_h is +0x128 |
| `tinystubs.c` | `g_info_icon_g` | +0x138 | `g_info_icon_a` | DisablePopUpInputs (w/w) | **no-overlap** | this TU declares g_info_icon_g as 4 bytes, g_info_icon_a is +0x138 |
| `tinystubs.c` | `g_info_icon_g` | +0x13c | `g_info_icon_f` | DisablePopUpInputs (w/w) | **no-overlap** | this TU declares g_info_icon_g as 4 bytes, g_info_icon_f is +0x13c |
| `uimisc.c` | `g_info_icon_g` | +0x11c | `g_info_icon_b` | ResetInfoPopUp (w/w) | **no-overlap** | this TU declares g_info_icon_g as 4 bytes, g_info_icon_b is +0x11c |
| `uimisc.c` | `g_info_icon_g` | +0x128 | `g_info_icon_h` | ResetInfoPopUp (w/w) | **no-overlap** | this TU declares g_info_icon_g as 4 bytes, g_info_icon_h is +0x128 |
| `uimisc.c` | `g_info_icon_g` | +0x138 | `g_info_icon_a` | ResetInfoPopUp (w/w) | **no-overlap** | this TU declares g_info_icon_g as 4 bytes, g_info_icon_a is +0x138 |
| `uimisc.c` | `g_info_icon_g` | +0x13c | `g_info_icon_f` | ResetInfoPopUp (w/w) | **no-overlap** | this TU declares g_info_icon_g as 4 bytes, g_info_icon_f is +0x13c |
| `unref4.c` | `g_edit_changed` | +0x8 | `g_edit_object` | JcDeco_SelectForPlacement (w/w) | **no-overlap** | this TU declares g_edit_changed as 4 bytes, g_edit_object is +0x8 |
| `unref4.c` | `g_edit_cursor` | +0x1414 | `g_edit_cursor_rect` | JcDeco_CalcCursor (r/rw) | **barrier/disjoint** | section 1d |
| `unref4.c` | `g_edit_cursor` | +0x1828 | `g_ui_flags` | JcDeco_SelectForPlacement (r/w) | **barrier/disjoint** | section 1d |
| `unref4.c` | `g_edit_cursor` | +0x1830 | `g_edit_cursor_next` | JcDeco_CalcCursor (r/w) | **barrier/disjoint** | section 1d |
| `unref4.c` | `g_edit_cursor_origin` | +0x10 | `g_edit_cursor_rect` | JcDeco_CalcCursor (r/rw) | **no-overlap** | this TU declares g_edit_cursor_origin as 8 bytes, g_edit_cursor_rect is +0x10 |
| `waterworks.c` | `g_edit_changed` | +0x8 | `g_edit_object` | Hedge_SelectForPlacement (w/rw); Flowers_SelectForPlacement (w/rw) | **no-overlap** | this TU declares g_edit_changed as 4 bytes, g_edit_object is +0x8 |
| `waterworks.c` | `g_edit_cursor` | +0x1414 | `g_edit_footprint` | WaterBlock_Update (r/w); Shower_Update (r/w); ElephantFountain_Update (r/w); CrocodileFountain_Update (r/w) | **no-overlap** | this TU declares g_edit_cursor as 1 bytes, g_edit_footprint is +0x1414 |
| `waterworks.c` | `g_edit_cursor` | +0x1830 | `g_8003f0` | WaterBlock_Update (r/w); Shower_Update (r/w); ElephantFountain_Update (r/w); CrocodileFountain_Update (r/w) | **no-overlap** | this TU declares g_edit_cursor as 1 bytes, g_8003f0 is +0x1830 |
| `waterworks.c` | `g_mapref` | +0x10 | `g_edit_footprint` | WaterBlock_Update (r/w); Shower_Update (r/w); ElephantFountain_Update (r/w); CrocodileFountain_Update (r/w) | **no-overlap** | this TU declares g_mapref as 8 bytes, g_edit_footprint is +0x10 |
| `westtown.c` | `g_edit_changed` | +0x8 | `g_edit_object` | GeneralStore_SelectForPlacement (w/rw); Sheriff_SelectForPlacement (w/rw); Bank_SelectForPlacement (w/rw); Saloon_SelectForPlacement (w/rw); Explorers_SelectForPlacement (w/rw); LegoShop1_SelectForPlacement (w/rw); LegoShop2_SelectForPlacement (w/rw); LegoMedia_SelectForPlacement (w/rw); JailCell_SelectForPlacement (w/rw) | **no-overlap** | this TU declares g_edit_changed as 4 bytes, g_edit_object is +0x8 |

165 hazard pairs: {'no-overlap': 133, 'barrier/disjoint': 23, 'FIXED': 9}
