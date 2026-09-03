# Lane brief (verbatim text given to every matching agent)

This is the `CTX` block from the last workflow script (`ll-batch17`). Paste it at the top of any new lane prompt, then add the lane name, file, state and function list.

**2026-09-03 update (not yet folded into the block below):** `tools/match.py` now applies `audit.py`'s extent rules itself, so a void tail-`jmp` wrapper, a `noreturn` tail or a recursive function no longer needs the "match.py cannot bound" WIP form once a clean `verify.py` run has confirmed the port — until then keep using it. The compiler wrapper path can be overridden with `LEGOLAND_CL`.

```
PROJECT: /Users/systemadmin/Downloads/legoland/legoland — a matching decompilation of
LEGOLAND. Human-written C that, compiled with the VC6 SP3 toolchain the game shipped
with (/O2 /Gy /Gd), must reproduce the original machine code function-by-function. No
game binaries or code live in the repo; everything is reconstructed from analysis of the
reader's own original/legoland.exe.

STATUS: 620 functions match at 100% — 589 of the 675 CODE exports (87%) plus 31
recovered internal functions. Read docs/DECOMP.md (status, levers, tooling).

YOUR FILE ALREADY EXISTS. Previous agents on this exact lane were killed by an API
session limit mid-run (twice) and left your file on disk with most functions done.
DO NOT start over. First:
  1. python3 tools/audit.py <yourfile>   — see what is [OK], [WIP], [REJECT], or
     COMPILE FAILED (a body may be half-edited).
  2. Fix anything that fails: a REJECT is a '// FUNCTION:' marker claiming a match that
     is not one — either finish it to [OK] or demote it to
     '// WIP-FUNCTION: ... (<pct>, <reason>)'. Work the WIPs from the FIRST diverging
     instruction outward with a side-by-side lister (scratchpad/cstm/sbs.py,
     scratchpad/bigsim/sbs.py — copy one into your scratch dir).
  3. Read the file header and notes the previous agents left, then continue with the
     functions from the list below that carry no marker yet.
  4. Finish with audit.py ending PASS and a clean /W3 compile.
  IMPORTANT: keep '// WIP-FUNCTION:' on a body until audit.py prints [OK] for it, so an
  interrupted run never leaves a false claim. Save progress often: the file on disk is
  what survives if you are cut off.

METHOD PER FUNCTION
0. grep LEGOLAND/*.c for the function's address and name; skip if another file marks it.
1. Disassemble:  python3 tools/disasm.py original/legoland.exe <RVA>   (RVA = VA - 0x400000)
2. Write C into YOUR OWN file. Marker on the line IMMEDIATELY above the SIGNATURE, //
   form only:   // FUNCTION: LEGOLAND 0x<VA>
3. Verify:  python3 tools/matchfull.py <yourfile> <Name> 0x<VA> --obj /tmp/<lane>_<Name>.obj
4. THEN the authoritative gate:  python3 tools/audit.py <yourfile>

*** THE VERIFICATION GATE ***
tools/audit.py is the authority: it finds where the original really ends (first ret or
unconditional jmp that nothing jumps past; switch case blocks are followed through the
.rdata jump table), trims the compiled body to that extent, and requires instruction
count, byte length, a strict index-for-index comparison, and no branch escaping the
extent ('ESCAPES' = a branch in our body targets past the original's end: usually a
duplicated tail block or a different block layout). A function counts ONLY when
audit.py prints [OK]. matchfull is the iteration tool (it can over- or under-report).
A void tail-call wrapper (ends in jmp, no ret) is marked
'// WIP-FUNCTION: LEGOLAND 0x<VA>  (100% by audit.py; match.py cannot bound a tail-jmp function)'.
For a big function you get to 90%+ but not 100%, leave an honest WIP with the residual
described precisely in a comment.

VC6 SP3 LEVER PLAYBOOK (hard-won)
- Register tie-breaks: the source order of INDEPENDENT computations decides which value
  stays in eax in place — permute early. 'v += e; if (x > v) x = v;' gives v two defs
  (kept across a call); 'if (x > v + e) x = v + e;' keeps one def. Reads of a stack
  ARGUMENT are CSE'd function-wide into one value whose priority falls with live-range
  length: inline early uses rather than routing them through a named temp.
- Zero registers: VC6 hoists a constant zero into a callee-saved register only when that
  register is pushed anyway — where a flag local is ASSIGNED relative to a rep-movsd
  struct copy decides whether a 4th push (and the zero register) appears. Boolean tests
  (while (d), if (d->x && d->y)) rather than '!= 0' stop a zero register leaking into a
  pre-push branch.
- Spill slots: DECLARATION ORDER is irrelevant; an aggregate local sits at the top of
  the frame; contending locals in ONE struct pin their relative slots; static __inline
  helper temporaries share the spill-home pool; a union making an ENREGISTERED scalar
  address-taken is a TRAP; which scalar carries an '= 0' initialiser can change slots;
  carrying a value in a scratch local lets VC6 coalesce two names into one register;
  a probe macro over ONE function-level struct (not a helper with its own local) keeps
  VC6's not-a-kill rep-movsd dead-store pattern.
- Globals that are one object must be ONE struct; two globals that must may-alias must
  be ONE struct; a packed {u8,u8}/u16 union read straight from the GLOBAL; direct
  global reads (not a local) so VC6 hoists ONE load and reloads only after aliasing
  stores/calls.
- Branch DIRECTION: 'je <end>' = success falls through; 'jne <inline>' = failure inline.
  Nested ifs, success early, failures to ONE trailing 'return 0'; two leading guards
  keep inline copies while later failures share one via 'goto fail'. ONE 'mov eax,1'
  block = exactly one textual 'return 1'. 'if (!x) return 0;' on the value just loaded
  into eax emits a bare ret. A bare leading 'return K' is redirected only to the FINAL
  block when it returns the same constant; a redundant, threaded-away 'if (n != 4)
  goto' after a 'while ((n = read) == 4)' loop flips the leading guard back to an
  inline epilogue. Success arm INLINE when the original keeps it first.
- Deferred/split prologue: pushes sink into the non-null path when loop locals live in
  a POST-GUARD INNER SCOPE and the loop exits via 'break', or a pointer lives in a
  post-guard block ending in one 'rc = ..; return rc;'; in non-loop functions a static
  __inline bounds helper after the guards does it.
- Lazy vs eager field reads decide registers — try both; e.g. read an order's byte
  heading BEFORE the action++ so the pointer loads first. Root copy of a parameter at
  entry: eager 'mov esi,[esp+8]'. A helper taking a Pos* vs ints decides which early
  value lands in eax; inlined-helper scalar args keep a byte-load CSE alive across an
  address-taken struct fill. Hoist a field read before a store to the same struct to
  avoid the alias-forced reload. A loop-invariant global field in a for-condition IS
  hoisted and pins the base in esi.
- 'return ok;' with ok already in eax elides xor; 'a && b' inserts 'mov eax,1';
  'return f(x) != 0' gives neg/sbb/neg; a char 'x ? 1 : -1' narrows only via a char
  LOCAL; a return type of char (not int) can free ebp for a hoisted import. A call
  result passed straight into another call splits the add esp — store it in a local to
  merge. A merged 'add esp,N' shifts every [esp+x] parameter read.
- 'if (len >= 12) n = 12; else n = len;' keeps the immediate compare; 'cap; if (q < cap)
  cap = q' vs 'v; if (v >= cap) v = cap' are different shapes; ternary flips polarity.
  'g += N' with pre-value used: mov/add/lea. lea operand order follows temp creation
  order. 'x - 1' as an expression (dec + separate test) vs 'x--' (fused dec/js).
  'x*2' must be x+x. 'h - 2*t' spelled inline in both calls CSEs into a dead arg slot.
- Loops: 'while (n-- != 0)' UNSIGNED gives mov/dec/test/je/inc; an unsigned char counter
  is widened plus a trip counter; a decrement in the for-increment vs the body reorders
  two ALU ops; 'while (p && p->next)' = rotated walk; list search 'while (p) { if (hit)
  return; p = p->next; } return 0;' for ONE shared xor/ret; unlink idiom + ONE trailing
  free (tail-duplicated); 'for (s = head; s; s = next) { next = s->next; ... continue; }
  prev = s;' puts match in esi and next in edi; walking by the link slot alone;
  insertion sort = for(;;) with insert-before inside and a break to the append; head
  read at declaration + a second read in the guard gives mov/test/mov; an inner loop
  may reuse the OUTER step counter (reproduce it).
- switch for dispatch; CASE ORDER decides block layout (e.g. 0,1,2 puts case 2 inline
  and 1,0 after the ret); a switch on a char param gives movsx/dec/cmp/ja/jmp [table];
  signed/unsigned decide jl/jb, 'jae' needs unsigned int.
- Stores cannot migrate across a call; VC6 reorders ADJACENT stores; stores to an
  address-taken local struct are reordered freely (fix by evaluating the values as
  scalar args of an inlined helper); two short field copies need explicit temporaries;
  re-assign all fields of a rect before a second block so VC6 rematerialises constants;
  a clamp stored then re-stored on each clamp is the two-def form. u16 flags '|= 0x100'
  narrows to a byte OR, '&= ~' does not; dword flags stay dword. A mouse-in-rect test =
  static __inline taking a 4-int rect BY VALUE built from the icon, with the icon
  global read directly at each call site.
- Win32/COM: '__declspec(dllimport) __stdcall'; Release = vtable +0x08; 'return
  obj->vt->Method(obj,x) >= 0' gives setge; i64 arg = two pushes; CRT exit() must be
  __declspec(noreturn); uninitialised out-pointer locals are homed in dead argument
  slots; WinRect by value = four arg slots. abs() -> cdq/xor/sub; #pragma
  intrinsic(strlen,strcpy) -> repne scasb / rep movsd+movsb; memset fixed size unrolls
  from eax=0; memset(p,0,n*4) -> bare rep stosd; whole-struct assignment -> rep movsd;
  a 16-byte copy is four register moves.
- Params: 'mov cx,[esp+8]' -> short; 'short' return -> or ax,0FFFFh; 'unsigned short t =
  u8field' -> movzx ax + and eax,0xffff; byte-compared char param -> cmp dl,1; a packed
  {u8 x; u8 y} by value is forwarded as a dword and masked; an 8-byte struct by value
  copied as a unit / returned in eax:edx; a dead parameter's slot may hold an
  UNINITIALISED local or a spilled temp (price, y-1); assigning into a parameter after
  its last use reuses the slot.
- x87: float vs double LOCALS decide the fxch pair and constant widths; a two-step float
  local prevents reassociation; (int) cast emits __ftol = the game's 0x458930 helper;
  'if (amount >= 1.0)' compares against a pooled double. __asm blocks force an ebp
  frame and full ebx/esi/edi save.
- A dead 'and dx,0x20' is a merged-arm ghost. A dead counter survives only via
  'while (n--) ;'. A 'volatile' read used once stops a CSE (use sparingly, comment it).
  VC6 sometimes emits a DEGENERATE branch — 'if (c) f(a); else f(a);' with a comment.
- Struct field OFFSETS are load-bearing; names are yours. Define structs LOCALLY — do NOT
  edit legoland.h. Extern prototype TYPES are caller-side codegen levers (unsigned short
  vs int params decide a 16-bit load) — declare externs the way YOUR function's
  disassembly needs them and note when that differs from another file's definition.

HARD RULES
- Create/edit ONLY your own assigned file and scratchpad/<lane>/. Do NOT touch other
  LEGOLAND/*.c, legoland.h, tools/**, docs/**.
- OTHER lanes are in flight owning: objmap2.c, bnvmove.c, workers2.c, fpui2.c,
  bigscreens.c, bigrender.c, bighelp.c. Declare callees extern and move on.
- Do NOT run tools/verify.py or tools/match.py. tools/audit.py is safe.
- No git commands that change state. Objects under /tmp; never /Zi.
- Do NOT break semantics for a higher number. Reproduce original bugs faithfully, commented.
- Compile clean at /W3.

REPORT per function: address, name, percentage, audit [OK] or not, marker, the FIRST
diverging instruction and hypothesis if not exact — AND what you learned about the
data structures and rules.
```
