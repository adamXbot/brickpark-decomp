# Lane `fable-b` / `savechunks2` — the script EVENT serialisers

`LEGOLAND/savechunks2.c`, created 2026-09-05. **One of two functions exact**
(`tools/audit.py LEGOLAND/savechunks2.c` → PASS, `/W3` clean, nothing left in
the repo root, all objects under `/tmp/fb_savechunks2_*`).

| address | name | insns | pct | audit | promotable | marker |
| --- | --- | --- | --- | --- | --- | --- |
| 0x0046c700 | `SaveScriptEvent` | 77 | 100% | `[OK]` | yes (real `ret`, not recursive) | `// FUNCTION: LEGOLAND 0x0046c700` |
| 0x0046c7e0 | `LoadScriptEvent` | 124 | 94.4% (mismatch 26) | `[WIP]` | n/a | `// WIP-FUNCTION: LEGOLAND 0x0046c7e0  (94.4%, 124/124 insns, mismatch 26; cold-block ORDER only -- the ``head = 0`` arm is emitted at index 95 instead of 117, first diverging index 95)` |

Neither function was renamed: both names come from `savechunks.c`'s existing
`extern` declarations. `SaveScriptEvent` matched on the FIRST draft (77/77).

---

## Mechanics recovered

### The `ScriptEvent` record is a LIST, not one record

`savechunks.c`'s header describes the pair as running "over the pending event
0x00668784", singular. It is a **linked list of 0x44-byte records** and the
link field doubles as the on-disk terminator:

```
 +0x00  next     chain link; a record whose next == (ScriptEvent*)-1 on disk
                 ENDS the stream. The writer's terminator is an all-zero 0x44
                 record with only +0x00 set to -1.
 +0x04  elem     LLIDB element. Serialised BY NAME, not by pointer: the writer
                 stores elem->name as a <script string>, the reader resolves it
                 with ElemID() and frees the temporary string.
 +0x08  text     an owned string, stored as a <script string>
 +0x0c  kind     NewScriptEvent's 1st argument
 +0x10  flags    bit 0x20 = "+0x08 is heap-owned"; FreeScriptEvent frees the
                 text only when it is set, so the loader sets it after a
                 non-NULL read
 +0x38  param    NewScriptEvent's 2nd argument
 +0x3c  time     absolute game time
```

Per node the stream is `u8 rec[0x44]` then TWO `<script string>`s (element name
then text). A NULL field is still written — as `SaveScriptString(NULL)`, which
the string serialiser stores as length −1 — so the record count and the string
count always agree.

### The timestamp is rebased in place, on the caller's live list

The writer does `ev->time -= g_script_now;` **into the caller's record**, traces
it, writes the record, then adds `g_script_now` straight back; the reader adds
the loading session's `g_script_now` and traces twice
(`"Loading Event, TimeStamp = %d"` at 0x004ba838 before, `"Fixed up to %d\n"` at
0x004ba828 after). So a save file's times are relative to the save's
`GetGameTimer()` and portable across the absolute clock.

### Callees named for the first time (script.c, 0x00468910 family)

| address | name | what it is |
| --- | --- | --- |
| 0x00468910 | `NewScriptEvent(kind, param)` | `calloc(1, 0x44)`, then redundantly re-zeroes +0x00/+0x08 and stores the two arguments in +0x0c and +0x38. **Returns NULL on failure and the redundant stores are guarded, but every caller here ignores the NULL.** |
| 0x00468940 | `FreeScriptEvent(ev)` | `if ((ev->flags & 0x20) && ev->text) free(ev->text); free(ev);` — this is what pins bit 0x20 to "+0x08 is owned". |
| 0x00468970 | `FreeScriptEventList(ev)` | recursive: frees `ev->next`'s chain, then `FreeScriptEvent(ev)`. |

Format strings: 0x004ba808 `"Saving Event, Timestamp = %d\n"`, 0x004ba828
`"Fixed up to %d\n"`, 0x004ba838 `"Loading Event, TimeStamp = %d"` (no newline).
Note the string pool order is Saving / Fixed / Loading, i.e. NOT first-use order
across the two functions — irrelevant to matching (each `.c` gets its own pool)
but worth knowing when reading the original's `.rdata`.

---

## Codegen levers (for `docs/DECOMP.md`)

- **A CALL RESULT STAYS IN `eax` ACROSS AN INTERVENING FIELD UPDATE, AND THAT IS
  HOW YOU SPELL "RESTORE, THEN TEST".** `SaveScriptEvent` emits
  `call SaveGameWrite / mov ecx,[g_script_now] / mov edx,[esi+0x3c] / add esp,0x10
  / add edx,ecx / test eax,eax / mov [esi+0x3c],edx / je fail`. The restore is
  scheduled *between* the call and the test, so the source is
  `ok = SaveGameWrite(ev, 0x44); ev->time += g_script_now; if (!ok) return 0;` —
  a named temporary, not `if (!SaveGameWrite(...))`. Writing the test first puts
  the `add` after the branch. (Evidence: 77/77 first try.)

- **TWO ARMS THAT CALL THE SAME FUNCTION ARE TWO TEXTUAL CALLS, NOT A TERNARY
  ARGUMENT.** `SaveScriptEvent` has four `call SaveScriptString` sites, in pairs
  of `test/je` + `jmp` over an `else` that pushes 0. Each arm carries its own
  `add esp,4`, which is the tell: a ternary argument (`f(p ? p->name : 0)`) would
  materialise the value in one register and leave ONE call and ONE `add esp,4`.
  This is the same "a pending cdecl `add esp` cannot cross a branch join" lever
  seen from the other side.

- **`memset` THE WHOLE STRUCT, THEN STORE THE ONE NON-ZERO FIELD.** The
  terminator record is `mov ecx,0x11 / xor eax,eax / lea edi,[esp+8] / rep stosd`
  followed by `mov dword ptr [esp+0x10], 0xffffffff` *after* both argument
  pushes. `memset(&term, 0, sizeof(term)); term.next = (ScriptEvent*)-1;` gives
  exactly that; a field-by-field zero-fill does not. (`#pragma intrinsic(memset)`
  is required for the `rep stosd`.)

- **THE SCRATCH REGISTER FOR A `rep stosd` IS PUSHED WHERE IT IS FIRST NEEDED,
  NOT IN THE PROLOGUE — AND THE SHARED `return 0` BLOCK THEN POPS ONE REGISTER
  FEWER.** `SaveScriptEvent` pushes only `esi` at entry and `push edi` at the top
  of the post-loop terminator block (0x0046c79f); its five `return 0` sites
  branch to a tail that pops `esi` alone. This falls out for free when the only
  address-taken aggregate lives *after* the loop — no `goto`, no scoping trick
  needed. The `pop edi` then lands **inside** the `neg/sbb/neg` of
  `return SaveGameWrite(&term, 0x44) != 0;` (between the `sbb` and the second
  `neg`); do not try to "fix" that.

- **A `u8` FLAG FIELD OR'd WITH A SMALL CONSTANT IS A BYTE `or`.**
  `ev->flags |= 0x20;` on an `unsigned char` at +0x10 →
  `or byte ptr [esi+0x10], 0x20`. (Confirms the existing "u16 `|= 0x100` narrows
  to a byte OR" entry from the byte side.)

- **THE VALUE JUST STORED IS THE VALUE PUSHED.** `sub edx,eax / mov eax,edx /
  mov [esi+0x3c],edx / push eax` is `ev->time -= g_script_now;` followed by
  `DBPrintf(fmt, ev->time);` — VC6 keeps the computed value in a second register
  for the argument rather than reloading the field.

### NEW, and the one that cost this lane its second function

- **VC6 LAYS COLD BLOCKS OUT IN SOURCE-GENERATION ORDER, AND A SINGLE-
  PREDECESSOR `goto` TARGET IS GENERATED INLINE WITH THE BRANCH THAT REACHES IT.**
  So the cold arm of an `if` inside an already-cold block is emitted IMMEDIATELY
  after that block — moving the label to the end of the function does not move
  the code. Two cold blocks reached from the *main* chain (here the two
  `if (g_script_errors)` handlers) are then emitted after it, in source order.
  Measured over ~45 spellings in `LoadScriptEvent`.
  *Corollary confirmed by the same sweep:* which arm is inline follows the
  SOURCE ARM ORDER — `if (prev) A; else B;` puts `A` inline and `je` to `B`;
  swapping the arms flips to `jne`.

- **VC6's CONSTANT PROPAGATION IS GLOBAL, NOT BLOCK-LOCAL, AND IT WILL DELETE A
  WHOLE RETURN BLOCK.** `L: head = 0; return head;` as the last statement of a
  function becomes `return 0` → `xor eax,eax` → cross-jumped into a neighbouring
  handler's `return 0` tail, so the block *vanishes* (124 → 117 instructions).
  It survives every disguise tried: `return head = 0;`, `*&head = 0;`, a cast,
  `memset(&head,0,sizeof head)`, a second variable coalesced with `head`,
  `head = prev;` (VC6 knows `prev == 0` from the compare that got you there),
  and splitting the store from the return across a label or a `goto`.
  **Therefore: an original that shows `xor <callee-saved>,<callee-saved>` followed
  by `mov eax,<that register>` is proof of a TWO-PREDECESSOR JOIN** — a phi copy
  in one arm plus a tail-duplicated shared `return v` — not a straight-line
  `v = 0; return v;`.

- **…AND THE TWO ARE RIGIDLY COUPLED.** Every spelling that keeps the join (and
  so the un-folded `xor ebx,ebx`) also drags the block back adjacent to its
  `if`; every spelling that leaves the block last folds it away; and the
  spellings that keep it last *and* unfolded invert the branch (`jne` at index
  87 with the block at 88 instead of `je` with the block at 117). This is the
  unresolved residual below — worth a dedicated lever hunt, because the same
  pattern (a cold `if`'s arm exiled past two other cold blocks) shows up
  wherever a list builder has a "nothing was loaded" exit.

---

## Original bugs reproduced (commented at the site)

1. **`LoadScriptEvent` leaks the element name on the first error path.** After
   `name = LoadScriptString();`, if `g_script_errors` is set the record is
   released with the raw CRT `free()` (not `FreeScriptEvent`) and `name` is
   dropped on the floor. The *second* error path, after the text string, uses
   `FreeScriptEvent` — correct, because by then the record owns +0x08 and has
   bit 0x20 set. The asymmetry is real and is reproduced.
2. **`NewScriptEvent`'s NULL is never checked.** `LoadScriptEvent` calls it
   twice and reads 0x44 bytes through the result immediately; `0x00468910`
   returns NULL when its `calloc` fails.
3. **`SaveScriptEvent` mutates the caller's live list while writing.** The
   `-=` / `+=` pair straddles `SaveGameWrite`, so a failure *inside* the write
   leaves that node's `+0x3c` relative. (The `+=` is executed before the result
   is tested, so only a crash inside the write can expose it.)
4. Both ends ignore short reads/writes past the point where a partially built
   list has already been published — the same family defect `savechunks.c`
   documents for the worker and work-order chunks.

## Extern-type divergences and one-name-per-address notes

- `savechunks.c` declares `extern int SaveScriptEvent(void* ev);` and
  `extern void* LoadScriptEvent(void);` (it has no `ScriptEvent` type). This
  file defines the record and types them `ScriptEvent*` / `ScriptEvent*` —
  identical codegen, but **do not "align" savechunks.c to it**; its `void*`
  spelling is what its own call sites need.
- **0x0049e4d0 has three names in the tree.** The majority spell it
  `HeapFree_w` (`fpui.c`, `coaster.c`, `gpu.c`, `objmap2.c`, …); `data2.c` calls
  it `MemFree`; `memdb.c` and `logflume2.c` call it `free`. It is inside the
  statically-linked CRT (≥ 0x0049e000), so it really is `free`, and this file
  follows `memdb.c`/`logflume2.c`. Flagged here rather than changed anywhere.
- `ElemID` is declared `LLElem* ElemID(const char*)` here, matching
  `savechunks.c`; `loadmap.c`/`render3.c` use `void*`. No codegen difference at
  these call sites (the result is stored straight into a pointer field).
- `LLElem` comes from `legoland.h` (name at +0x00) — a local redefinition is a
  C2011 error, so do not add one.

## Residual for `LoadScriptEvent` (HANDOFF §6B triage)

`ours = 124i/323B`, `orig = 124i/319B`, **mismatch 26, first diverging index 95**.
Every instruction of the original is present and every block is byte-correct;
only the ORDER of three cold blocks differs:

```
original    readfail 0x46c8ae | terminator 0x46c8cf | err-after-name 0x46c8ea
            | err-after-text 0x46c900 | `head = 0` 0x46c916
this body   readfail | terminator | `head = 0` | err-after-name | err-after-text
```

Class: **structural** (a pure cold-block permutation — not allocation, not
frame, not scheduling; `strict`, register-blind and offset-blind all agree, and
no free `volatile` read is relevant because no value is wrong). Reachable only
with a block-ordering lever that neither of the two coupled behaviours above
lets you spell today; the full 45-spelling search is written up in the block
comment above the function so the next lane does not repeat it.
