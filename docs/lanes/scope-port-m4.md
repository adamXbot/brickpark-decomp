# Scope PORT-M4 — the externs with no address comment

> **Status: IN PROGRESS (claimed 2026-09-12 by PORT-M4).** Branch
> `scope/PORT-M4` from `0e0b2b76` (the PORT-B4 merge). The lane resolves every
> name in `linkreport.py`'s **Unclassified** bucket — externs the game declares
> without a trailing `/* 0x... */` address comment — so `gen_link.py` stops
> emitting a trapping stub for them.

## 1. The method

`linkreport.py` classifies an undefined symbol by the address comment on its
`extern`. With no comment it cannot tell a stale alias from unwritten work, so
it lands in *Unclassified* and `gen_link.py` emits a trap. The frontier is
closed (3281 exact + 42 WIP, every function in the game-code range has a body),
so an Unclassified **function** name is always one of:

* a second name for a function that IS defined under another spelling, or
* an address in the CRT / import range (the twelve CRT thunks were this).

The evidence is the relocation the original binary carries at the call site.
`tools/relocs.py` normally *resolves* these names out of `symbols/legoland.exports.txt`
and `docs/DECOMP.md`, so they never appear as `UNRESOLVED`. This lane forced
them unresolved with a ten-line monkeypatch over `relocs.symbol_address`
(`port-m4-probe.py` in the scratchpad) so that every relocation naming one of
them printed `original=0x...` — the address the *original* binary uses at that
exact instruction. That address's `// FUNCTION:` marker names the real body.

Nothing in this lane changes a byte: an address comment is a comment, and an
identifier rename with the same types emits the same instruction. Both arms of
the build therefore get the same edit (no `#ifdef LEGOLAND_PORTABLE`).

## 2. Name table

(filled in per batch below)

## 3. Gate results

(filled in per batch below)
