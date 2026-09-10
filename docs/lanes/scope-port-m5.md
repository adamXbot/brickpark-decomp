# Scope PORT-M5 — the port lanes' recovery findings, and the last three asm rasterisers

> **Status: IN PROGRESS (claimed 2026-09-12).** Branch `scope/PORT-M5` from
> `07f11d66` (the PORT-A5 merge).

What this lane owes:

1. the four caller/definition **name disagreements** PORT-M4 recorded, plus
   `0x00443dc0` (`SetVidAnim` vs `StartAdvisorClip`) — decided from the
   disassembly and the callers, renamed for both builds, evidence recorded;
2. PORT-M3's **open callback slots** — `DrawBasicPath` in `ObjDef +0xa0`,
   `schoolcar3.c hooks[3]`, `coaster8.c attach`/`detach`;
3. PORT-B5's findings — the transposed texel formula in the tri3d.c /
   texture.c HEADER prose, and the **`sub_458930` audit** (the `(int)<float>`
   helper that rounds to nearest);
4. the last three inline-asm bodies ported in their `#else` arms —
   `TrackShade_FillPoly` (coaster13.c), `Span_FillShade` and `Span_FillShadeZ`
   (coastershade2.c) — with tests.

## 1. The five name disagreements

(filled in as each lands)

## 6. Gate results

(filled in after the quiet window)
