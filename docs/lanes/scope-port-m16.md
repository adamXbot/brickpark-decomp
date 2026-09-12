# Scope PORT-M16 — P2-2: the LEGOLAND theme button lost to the panel/MAP race

> **PORT-M16 — Status: IN PROGRESS (claimed 2026-09-12 by PORT-M16)** — branch
> `scope/PORT-M16`, cut from the PORT-P2 merge (`6cd9c232`). A matching-side
> lane (VC6-gated) with licence to edit `portable/src/hostwin/*.c` if the cause
> turns out to be host timing.
> Brief: `docs/SCOPE_PORT_WAVE.md`; the defect is PORT-P2's **P2-2**
> (`docs/lanes/scope-port-p2.md` §3). Files: this note, whatever `LEGOLAND/*.c`
> the verdict requires, and `portable/src/browser/replays/m16-*.js`.

**The question this lane has to answer first:** does the ORIGINAL lose the
theme button to the same race (a shipped bug, to be preserved for matching) or
is the port's behaviour different (an aliased state flag, a dual-address
global, a by-value square, the shim's message ordering, or timer pacing)?

## Baselines at the claim

| gate | result |
| --- | --- |
| `ninja -C portable/build-wasm` + the seven extra targets + `ctest` | 24/24 PASS |
| `ninja -C portable/build` + `legoland_tests` `legoland_cbtypes` + `ctest` | 17/17 PASS |
| `$PY tools/progress.py --check` | (recorded with the first game-file commit) |

## 1. Frame-by-frame record

(to be written)

## 2. Verdict

(to be written)

## 3. The fix

(to be written)

## 4. Gate table

(to be written)
