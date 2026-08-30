# Matching decompilation — workflow

The goal of the decomp proper: rewrite `legoland.exe` in C that, compiled with
the **Visual C++ 6.0 SP3** toolchain the game shipped with, reproduces the
original machine code **function-by-function**. `legoland.exe` exports 716
named functions ([`symbols/legoland.exports.txt`](../symbols/legoland.exports.txt)),
so we already have the names — a head start LEGO Island never had.

## Toolchain

We reuse the VC6 SP3 toolchain (wibo + `msvc6.3` + `sp3-libs`) — the same
compiler era as LEGO Alpha Team, and the one whose Rich header `legoland.exe`
carries (cl 12.00.8168 / codegen 8447). It runs natively on macOS/Linux via
[wibo](https://github.com/decompals/wibo), no Wine/Docker. `toolchain/` is a
machine-local symlink; point it at any assembled VC6 SP3 tree
(`ALPHATEAM_VC6_ROOT` overrides).

## The loop

1. Disassemble the target: `tools/disasm.py original/legoland.exe <ExportName|0xRVA>`.
2. Write matching C in `LEGOLAND/*.c`, annotated isle-style:

   ```c
   // FUNCTION: LEGOLAND 0x00441ec0
   void* GetSpriteForLayer(RenderObj* obj, int layer)
   {
       return obj->layers->sprites[layer];
   }
   ```

3. Check the match: `tools/match.py LEGOLAND/foo.c FuncName 0x00441ec0`. It
   compiles the file with VC6, extracts the function's code from the COFF object
   (applying relocations), and diffs it instruction-by-instruction against the
   original, printing a side-by-side and a match %.
4. Iterate the C until 100% (branch direction, local ordering, and struct field
   offsets are the usual levers — VC6 codegen is stable and reproducible).
5. `tools/verify.py` runs every annotated function and prints the tally.

Only struct **offsets** are load-bearing; field/type names are ours. Types live
in [`LEGOLAND/legoland.h`](../LEGOLAND/legoland.h).

## Status

The map/render accessors and the `SetMapTile` family — **13/13 at 100%**:

| addr | function | |
| --- | --- | --- |
| 0x004015c0 | `GetSpriteSize` | render accessors |
| 0x00460540 | `GetTileDimensions` | |
| 0x00441e80 | `GetLLSForSprite` | |
| 0x00441ea0 | `GetLLSForLayer` | |
| 0x00441ec0 | `GetSpriteForLayer` | |
| 0x00441ee0 | `GetRenderOffsetForLayer` | |
| 0x00461610 | `Get_RFFlags` | cell flag get/set |
| 0x00461760 | `Get_MapFlags` | |
| 0x004617d0 | `GetMapFlags` | |
| 0x004616e0 | `Set_RFFlags` | |
| 0x00461810 | `SetMapFlags` | |
| 0x00461780 | `SetMapTile` | |
| 0x00461630 | `GetCurrentRFFlags` | RF dispatch (calls a tile handler) |

`GetCurrentRFFlags` is the first non-trivial one — a bounds-checked lookup that
dispatches to a per-tile handler via a function pointer; it matched once the
handler was hoisted into a local (VC6 computes the callback once, then calls).

Next: the bigger map builders (`LoadBaseMap`, `PutObjOnMap`) and the LLIDB
loaders we already understand from the asset RE — then outward across the 716
exports.

## Legal

No game code or binaries live in the repo — only human-written C reconstructed
from analysis of a legally-owned copy, verified against the reader's own
`original/legoland.exe`. MIT-licensed.
