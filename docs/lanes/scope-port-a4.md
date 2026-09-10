# Scope PORT-A4 — the generator's last mismatch, indirect-call traps, wasm32 CI

> **Status: IN PROGRESS (claimed 2026-09-12).** Branch `scope/PORT-A4` from
> `595aae91` (the PORT-B3 merge). Files: `portable/tools/gen_link.py`,
> `linkreport.py`, `name_trap.py`, `portable/cmake/headless.cmake`,
> `portable/src/headless/**`, `portable/src/hostwin/kernel32.c`, `msvcrt.c`,
> plus the delegated `.github/workflows/progress.yml`.

## 1. The last wasm-ld signature mismatch: `printf` typed from the alias

PORT-M2 closed 18 of the 19 call-site mismatches and filed the survivor as
generator-side (`docs/lanes/scope-port-m2.md` section 5). It was exactly that.

`DebugPrint` (logflume2.c, logflume8.c, `void DebugPrint(const char*)`) is a
second name for 0x0049e5c5, which bigrender.c / printlist.c / unref7.c declare
as the CRT's `printf`. The alias machinery emitted

```c
extern void printf(unsigned int);                  /* the ALIAS's signature */
void DebugPrint(unsigned int a0) { printf(a0); }
```

because of one line:

```python
rsig = def_sig_of(real) or sig        # the signature of the ALIAS as fallback
```

against libc's `(i32, i32) -> i32`. wasm-ld warned and resolved **every** call
through that declaration — the game's own `printf` calls included — to a
trapping stub.

### The fix: libc's prototype, from a table, never the alias's

Three pieces in `gen_link.py`:

1. **`CRT_PROTOS`** — a table of the libc prototypes the game's CRT aliases
   reach, as `(return type, fixed parameter types, variadic?)` in C: `printf`,
   `sprintf`, `malloc`, `calloc`, `realloc`, `free`, `rand`, `srand`, the
   `str*`/`mem*` family, `qsort`, the stdio file calls, the MSVC `_stricmp`
   spellings, and the f64 math calls (the only rows whose wasm types are not all
   i32). `crt_wasm_sig` derives the wasm signature from a row, adding **one
   extra i32 for a variadic callee**, because that is how clang lowers `...` on
   wasm32: the variable arguments go into a buffer and its address is the last
   parameter. That is why libc's `printf` is `(i32, i32) -> i32`.

2. **`defined_here(name)`** — a new predicate, and the one that made the fix
   general. The existing `def_sig_of` falls back to the signature the
   *references* voted for, so `def_sig_of('malloc')` answers with the **game's**
   declaration even though no object in the tree defines `malloc`. Only
   `defined_here` (wasm `defined` map alone) says whose prototype wins. With the
   gate on `def_sig_of(...) is None` the fix reached 3 aliases; with
   `not defined_here(...)` it reaches 11, and `_stricmp` correctly stays on the
   old path because `portable/src/hostwin/msvcrt.c` really does define it.

3. **`crt_alias`** — emits the forwarder with **per-argument C casts, never a
   cast of the function pointer**. PORT-M2 section 4 is why: a cast call lowers
   to `call_indirect`, and binaryen's `directize` rewrites a constant-index
   `call_indirect` into a direct call whose argument types then disagree, so the
   module fails validation far from the defect.

Two spellings, and telling them apart is the whole subtlety:

| the alias is | example | what the forwarder does |
| --- | --- | --- |
| spelled variadic (its wasm signature has one parameter MORE than libc's fixed count, and that last one IS the varargs pointer clang handed it) | `Format` -> `sprintf`, `sprintf_w` -> `sprintf` | passes the pointer **straight through**: the callee is declared flattened (fixed parameters plus `void*`), which is precisely the wasm signature libc's variadic definition has |
| spelled non-variadic | `DebugPrint` -> `printf` | calls the **real variadic prototype**, so clang builds the (empty) buffer and emits the `(i32, i32) -> i32` call libc expects |

The flattened spelling gets its own C identifier with an `__asm__` label naming
the real symbol (`extern int ll_crt_sprintf_va(char*, const char*, void*)
__asm__("sprintf");`), so both spellings of one libc function can coexist in a
single translation unit — needed the day some alias of `sprintf` is spelled with
two arguments while `Format` keeps three.

### What the generated file looks like now

```c
extern int printf(const char*, ...);
void DebugPrint(unsigned int a0) { (void)(printf((const char*)(__UINTPTR_TYPE__)(a0))); }

extern int ll_crt_sprintf_va(char*, const char*, void*) __asm__("sprintf");
unsigned int Format(unsigned int a0, unsigned int a1, unsigned int a2)
{ return (unsigned int)(ll_crt_sprintf_va((char*)(__UINTPTR_TYPE__)(a0),
                                          (const char*)(__UINTPTR_TYPE__)(a1),
                                          (void*)(__UINTPTR_TYPE__)(a2))); }

extern void free(void*);
void HeapFree_w(unsigned int a0) { free((void*)(__UINTPTR_TYPE__)(a0)); }
```

`free` is worth a second look: it was `extern void free(unsigned int)` before,
which happened to have libc's wasm signature and so never warned. It is now
typed from libc like the rest, which means the next time emscripten's ABI makes
a pointer something other than one i32 this file does not quietly go wrong.

### Result

| | at lane start (`595aae91`) | after |
| --- | --- | --- |
| wasm-ld `function signature mismatch` lines, all seven links | 5 (`printf`, one per link that pulls libc's printf.o) | **0** |
| distinct symbols warned about | 1 | **0** |
| `wasm-opt --all-features` validator errors, all 7 modules | 0 | **0** |
| forwarders typed from libc rather than from the alias | 0 | **11** |
| CRT targets with no `CRT_PROTOS` row | — | **0** |

Both builds from clean directories: native `ninja` + `legoland_linkcheck`
("every symbol resolved") + `ctest` 5/5; wasm32 all seven targets + `ctest`
10/10.

## 2. A census the generator now publishes: cast forwarders

While closing the CRT rows, the manifest grew the section that names the **next**
defect class, which is deliverable 2's subject and PORT-M2's open follow-up: an
alias whose callers emitted a signature the real body does not have. There are
**80** of them, and every one is a latent runtime trap — the forwarder casts the
function pointer, the call becomes `call_indirect`, and wasm checks the type at
the call.

`gen/manifest.md` now ends with a table of all 80: alias, body, what the callers
emitted, what the body has. They are the `void Foo()`-in-a-table family PORT-M1
and PORT-M2 described (`Bank_Activate` -> `Bank_TickCustomers(i32)`,
`Temple_Interact` -> `Temple_Draw(i32 x 6)`, ...), plus a handful of real
typing defects (`InitRasterBuffer` -> `Render_SetPixelFormat` disagree on the
return, five `sub_*` names likewise). The fix belongs in the declaring game file,
which is lane PORT-M3's; the generator's job is to name them, not bridge them.
