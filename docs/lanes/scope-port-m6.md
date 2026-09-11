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
object-definition table and is never called directly. Getting the declaration
right therefore means deciding what the *slot* type is, which is PORT-M3's
typed-callback pass continued rather than a return-type edit. 72 rows, mostly in
`interfaces.c` and the ride/attraction files, one declaration each.
**Recommend a PORT-M7 brief** with the generator's own manifest table as the work
list and `portable/tests/test_callback_types.c` as the gate.

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
| `grep -c '))&' gen-browser/aliases.c` | 82 → **72** |
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
