# Scope LL10 — unreferenced (dead) coaster model / raster functions

Branch `scope/LL10`. New file `LEGOLAND/unref2.c`. Object prefix `/tmp/sll10_`.
Brief: `docs/SCOPE_LL10_unref_coaster_model_b.md`.

All thirteen are DEAD: nothing live in the image calls, tail-jumps to or takes
the address of them (`tools/inventory.py`, scope N). They read as the coaster
module's developer tooling — a wireframe painter, a debug box builder, a
"dump the two model images back to disk" pair, a shaded texture blitter and a
mesh back-face stripper — kept because the game was built without `/OPT:REF`.

## Status

| address | name | insns | % | audit | marker |
| --- | --- | ---: | ---: | --- | --- |
| 0x00420fd0 | `Model_BuildBoxBelow` | 114 | 100 | [OK] | FUNCTION |
| 0x00421130 | `Model_BuildBoxCentred` | 112 | 100 | [OK] | FUNCTION |
| 0x00421530 | `Unref_00421530` | 3 | 100 | [OK] | FUNCTION |
| 0x00422520 | `WriteWholeFile` | 43 | 100 | [OK] | FUNCTION |
| 0x004225e0 | `CoasterModel_GetTextureName` | 8 | 100 | [OK] | FUNCTION |
| 0x00422650 | `CoasterModel_SaveImages` | 31 | 100 | [OK] | FUNCTION |
| 0x004227a0 | `CoasterModel_FreeImages` | 8 | 100 | [OK] | FUNCTION |
| 0x004227c0 | `Mesh_DropBackFaces` | 521 | 98.8 | [WIP] | WIP-FUNCTION |
| 0x00423060 | `CoasterShades_Free` | 7 | 100 | [OK] | FUNCTION |
| 0x00423080 | `Raster_BlitTextureShaded` | 64 | 100 | [OK] | FUNCTION |
| 0x00423750 | `Unref_00423750` | 1 | 100 | [OK] | FUNCTION |
| 0x004237a0 | `Raster_DrawWireframe` | 28 | 100 | [OK] | FUNCTION |
| 0x004237f0 | `Raster_DrawLine` | 51 | 100 | [OK] | FUNCTION |

**12 / 13 exact, 470 / 991 instructions exact.** `audit.py` ends PASS,
`relocs.py` zero MISMATCH (UNRESOLVED only the two string literals, the pooled
`1/30` float and the two `__ftol` calls), `/W3` clean.

## Names chosen

No exports, so every name here is ours.

* `Model_BuildBoxBelow` (0x00420fd0) and `Model_BuildBoxCentred` (0x00421130)
  are near twins that differ in ONE statement — the top quad's z is `0.0f` in
  the first and `+hz` in the second, so one box hangs below the origin and the
  other is centred on it.
* `WriteWholeFile` (0x00422520) is the exact inverse of coaster7.c's
  `CoasterModel_LoadFile`; `CoasterModel_SaveImages` (0x00422650) is the
  inverse of schoolcar4.c's `LoadCoasterModelSet`.
* `CoasterModel_GetTextureName` (0x004225e0) is the `.txt`-image twin of
  schoolcar8.c's `CoasterModel_GetRecordName` (0x004225b0, the `.obj` image).
* `CoasterShades_Free` (0x00423060) undoes schoolcar5.c's
  `CoasterShades_Init` (0x00422fe0).
* `Raster_DrawLine` / `Raster_DrawWireframe` / `Raster_BlitTextureShaded` are
  named for what they paint.
* `Mesh_DropBackFaces` (0x004227c0) rebuilds a mesh with its back-facing
  triangles, and everything only they used, removed.
* **`Unref_00421530`** and **`Unref_00423750`** have no recoverable behaviour:
  the first stores its argument into a write-only global and the second is a
  one-byte `ret`. Recorded under the fallback naming as the brief allows.

## Globals and callees first named

| address | name | what it is |
| --- | --- | --- |
| 0x004b5958 | `g_4b5958` | write-only int; its ONLY reference in the whole image is the store in 0x00421530 |
| 0x004b58c8 | `g_box_template` | the 0x90-byte `ModelMesh` template the two box builders copy with `rep movsd` |
| 0x004b5808 | `g_box_tris[12]` | the SECOND copy of the cube's twelve triangles, 16 bytes each |
| 0x004b5748 | (unnamed) | the first copy of the same twelve triangles; the template points +0x18 at it |
| 0x004b5700 | (unnamed) | the cube's six face normals, `Vec3f[6]`; the template points +0x10 at it |
| 0x00420780 | `CoasterModel_GetTexture(int)` | `return g_coaster_tab_c[i];` — the texture table at 0x004d89c8 (schoolcar.c's name), indexed by int |
| 0x004238a0 | `Raster_PlotPoints(PlotPoint*, int, short)` | plots `count` 16-byte points, each clipped against the four view bounds 0x008299ac..0x008299b8, into the 16-bit target |

The `ModelMesh` layout recovered from `g_box_template` and the two builders
(coaster9.c only had `{int count; char pad[8]; Vec3f* vertices;}`):

```
+0x00 int       vertex count (8)          +0x14 Vec3f*    per-vertex normals
+0x04 int                                 +0x18 ModelTri* pass-1 triangles
+0x08 int       face count (12)           +0x1c int       pass-1 count
+0x0c Vec3f*    vertices                  +0x20 ModelTri* pass-2 triangles
+0x10 Vec3f*    face normals (6)          +0x24 int       pass-2 count
                                          total 0x90
```

`ModelTri` is 16 bytes: `{short, short shade, short v[3], short t[3]}` —
CoasterModel_DrawPass1 (0x00420810) reads the three vertex indices at +4/+6/+8
with `movsx`, and the box builders stamp `shade = -1` and copy `v[]` over
`t[]`.

## Mechanics recovered

**The debug box (0x00420fd0 / 0x00421130).** `*model = g_box_template;` (a
144-byte `rep movsd`), point the mesh at the caller's vertex and normal
arrays, then write the bottom quad from the three half-extents:
`(hx,-hy,-hz), (hx,hy,-hz), (-hx,hy,-hz), (-hx,-hy,-hz)`; mirror it up with
`vertex[i+4] = vertex[i]; vertex[i+4].z = 0.0f` (or `hz`); if a normal array
was supplied, derive one normal per vertex by NORMALISING the vertex itself
(0x00425d50 on a copy — the box is centred on the origin so the position is
the outward direction); then move the twelve triangles from the mesh's FIRST
triangle slot to the second, pointing at the second copy of the table at
0x004b5808 after stamping `shade = -1` and copying each triangle's vertex
indices over its texture indices. Both take an unused fourth argument between
the normal array and the three extents; with no callers, what it carried is
lost.

**The model-image dump (0x00422650 / 0x00422520 / 0x004227a0).** The module
keeps its two loaded files as `{void* data; int length;}` pairs at 0x004dd758
(".txt") and 0x004dd860 (".obj") with the base name at 0x004dd760
(schoolcar4.c). 0x00422650 rebuilds "<name>.txt" and "<name>.obj" from that
base name and writes both images back out through 0x00422520 — CreateFileA
with GENERIC_WRITE / no sharing / CREATE_ALWAYS / FILE_FLAG_SEQUENTIAL_SCAN,
one WriteFile, success iff the reported byte count equals the request.
0x004227a0 frees both image blocks without clearing or null-checking either,
so it is the loader's unwinder rather than a general teardown.

**The wireframe painter (0x004237a0 / 0x004237f0 / 0x004238a0).**
`Raster_DrawWireframe` draws one white (-1) line per entry of the mesh's pair
table, between the two vertices that entry names. `Raster_DrawLine` does not
Bresenham: it walks THIRTY samples of `from + k*(to - from)/30` into a local
array of 16-byte points, truncating each with the game's `__ftol` helper, and
hands the whole array to the plotter, so the last sample is one step short of
`to` and a long line is drawn dotted. The plotter clips each sample against
0x008299ac..0x008299b8 and pokes a 16-bit pixel.

**The shaded blitter (0x00423080).** Fetch a texture from the module's texture
table (0x004d89c8) — `{int w; int h; unsigned char pixels[]}`, one palette
index per pixel — and blit it at (x, y) with every source byte run through
that colour's 128-byte shade ramp (`g_shade_tab[c][shade]`,
schoolcar5.c's table at 0x00829c60). The destination is 0x004b5b20 as a 16-bit
base with 0x004b5b28 as the pitch in PIXELS. `Raster_SaveState`'s result is
DISCARDED here, unlike Coaster3D_DrawModel (0x00420e90), which skips the whole
paint when it fails.

**The back-face stripper (0x004227c0).** Five passes over three mark arrays,
one flag per triangle / pair / vertex, all allocated up front and all freed on
the way out:

1. Mark every triangle whose 2D cross product `dy2*dx1 - dx2*dy1` is NEGATIVE.
   That is Coaster3D_DrawMesh's (schoolcar3.c, 0x004234e0) facing test with
   the sense flipped, and it is taken on the SIGN BIT of the float rather than
   by comparing against zero, so `-0.0` counts as back-facing too.
2. A pair used by a marked triangle is itself marked unless some UNMARKED
   triangle also uses it. The comparison is `(a ^ b) & 0x7fffffff`, so the two
   seam directions of one pair count as the same pair.
3. A vertex named by a marked pair is marked unless some unmarked pair also
   names it.
4. Count the marked pairs and vertices and size ONE block:
   `28 + kept_pairs*8 + kept_tris*12 + kept_verts*20`, laid out header, pairs,
   triangles, vertices.
5. Compact. **Each mark array is REUSED as its own renumbering table**: as an
   item survives, its slot is overwritten with `old_index - new_index`, so the
   later passes renumber with a subtraction. Vertices go first because the
   pairs need their table, pairs next because the triangles need theirs.

### Correction to schoolcar3.c's MeshDesc

`MeshDesc`'s +0x08 is the PAIR count, not a vertex count. schoolcar3.c calls
it `nverts` from `g_track_mesh.nverts = 18 * last + 6`, but 18 is the
per-instance PAIR stride (six per ring gives `6*last + 6` vertices), and every
reader — this scope's 0x004227c0 first pass, its pair loops, and
`Raster_DrawWireframe` — uses it as the bound of the +0x14 pair array. There
is no vertex-count field at all: 0x004227c0 derives one by scanning the pairs
for the largest index. The declaration in schoolcar3.c was NOT changed; the
divergence is recorded here and `unref2.c` names the field `npairs`.

### Original bugs, reproduced

* **0x004227c0 writes `kept - 1` into the new descriptor's +0x08 and +0x04.**
  The triangle count at +0x0c has no such `- 1`. Since +0x08 is a strict loop
  bound for every reader (including this function's own first pass), the last
  surviving pair of a compacted mesh is invisible. Reproduced as written.
* **0x004227c0 never writes the new descriptor's +0x00**, so the tag field
  keeps whatever the allocator left there.
* **0x004237f0 stops one step short of `to`** — the step is `(to-from)/30` and
  thirty samples are plotted starting at `from`.

## Levers learned

* **[TY07 extension] An `unsigned char` local homed in a dead 4-byte parameter
  slot is written as a BYTE and read back as an aligned DWORD plus
  `and 0xff`.** No union or packed struct is needed. Evidence:
  `Raster_BlitTextureShaded` 0x00423080 — `unsigned char c = *src++;` in the
  inner loop, all seven GPRs busy, so `c` is homed in the dead `y` argument
  slot and emits `mov byte ptr [esp+0x30],bl` / `mov ebx,[esp+0x30]` /
  `and ebx,0xff`. Exact first try; the plain `movzx` reading was wrong.
* **[BL01/BL02] To exile a `return 1` past a shared `return 0` epilogue, the
  failure arm must end in a `goto` to that epilogue.** `WriteWholeFile`
  0x00422520: `if (written == length) { CloseHandle; return 1; } CloseHandle;
  return 0;` keeps the success INLINE and exiles the failure (86%, 3 strict);
  `if (written != length) { CloseHandle; return 0; } CloseHandle; return 1;`
  gets the order right but gives all three leading guards their own inline
  epilogue (43i → 54i). The exact form is three `goto fail` guards plus
  `if (written != length) { CloseHandle(h); goto fail; }` with `fail: return 0;`
  as the last statement — the guard's `goto` merges into the same block that
  the failure arm falls into, and `return 1` becomes the exiled tail. 43/43.
* **[RC08] Three uses of a literal zero buy a zero register and a fourth
  callee-saved push.** `Model_BuildBoxBelow` needs `0` for the top quad's z,
  for `tri1_count = 0` and for the null test, so VC6 hoists it into edi and
  has to push ebp to carry the copy loop's temporary — 114 instructions.
  `Model_BuildBoxCentred` stores a float there instead, needs the zero twice,
  and pushes three — 112 instructions. The two bodies are otherwise the same
  source.
* **Strength-reduced `for` bounds read back the source's comparison.**
  `cmp eax,0x24 / jle` on a 12-byte stride is `i <= 3`, not `i < 4` (which
  gives `cmp eax,0x30 / jl`); `cmp esi,0x54 / jle` is `i <= 7`. But a cursor
  compared against a global array's END address with `jl` is `i < 12`. All
  three appear in the same function (0x00420fd0).
* **[RA13] `model->vertex` must be re-read from the mesh before every
  component store**, because the store through it may alias the mesh header.
  Twelve reloads in a row in both box builders.
* **Adjacent field stores permute.** `tri2 = table; tri2_count = 12` emits the
  count first; writing the count first in the source is what matches
  (0x00420fd0 was 113/114 until the two lines were swapped).
* **Hoisting the row pointer of a 2-D index out of an intervening inner loop
  is worth 15 instructions.** In 0x004227c0's third pass,
  `m->pairs[i][k] == m->pairs[j][0]` inside a `j` loop recomputes the whole
  `i,k` index every iteration; `const int* pv = m->pairs[i];` outside the `k`
  loop, then `pv[k]`, strength-reduces to the original's
  `lea ebx,[ecx+edx*8]` + `mov edx,[ebx]` + `add ebx,4` in the `k` latch.
* **`*s++` versus `s[k]` decides where the cursor increment lands.** In
  0x004227c0's seam-decode loop the original advances both cursors in the
  LATCH (`add edx,4 / add ecx,4 / dec esi`), which is `m->tris[i][k]` with `k`
  a down counter — not schoolcar3.c's `int e = *s++;`, which hoists the
  increment to just after the load.
* **Naming the three vertex POINTERS of a triangle can re-phase the whole
  body.** In 0x004227c0, `a = &m->verts[v[0]]; b = ...; c = ...;` before the
  four deltas is worth +24 instructions: it drops one register from the
  cross-product block's simultaneous demand, which lets the loop counter keep
  ebx (coalesced with the returned pointer) instead of being pushed out to
  esi, and that rotation was visible over ~80 instructions.
* **An empty `if` between four float definitions and their consumer pins the
  `fild`s at the definition sites -- VC6 forward-substitutes a single-use
  float local only within one basic block.** 0x004227c0's cross product emits
  `fild dx1 / fild dy1 / fild dx2 / fild dy2 / fmul st(3) / fxch st(1) / fmul
  st(2) / fsubp st(1)` plus a dead `fstp st(0)` pair -- the signature of four
  x87-ENREGISTERED float locals, the same shape as Raster_DrawLine's
  `x/y/sx/sy` (four dead pops after its loop). Forty-odd consumer-side
  spellings (float / double / long double; all six operand orders with and
  without parentheses; separate statements, initialisers, inner and function
  scope; int locals converted at the use site; named products; three
  accumulator forms; a `static __inline` four-float helper; a `Delta()` inline
  helper; `register`; four spellings of the sign test; an unreferenced label)
  all give the expression-order `fild / fild / fmulp / fild / fild / fmulp /
  fsubp`, four instructions shorter, because none of them splits the block.
  `if (k) ;` after the fourth delta (LEVERS SA07's zero-cost empty integer
  test) closes the whole block, 495 -> 515 of 521, and makes the instruction
  and byte counts exact; `if (i)`, `if (v[2])`, `if (deadtris)` and
  `if (trimark[i])` are byte-identical to it. The boundary must follow ALL
  four deltas (after dy1: 505; after dx2: 504; before dx1: inert), `float` is
  then the right type (`double` was a scheduling proxy), and the third vertex
  pointer must be materialised after dy1 (assigned between dy1 and dx2, or the
  index spelled inline -- identical; assigned with a and b before dx1 costs 3).
  The `mov eax,[area] / test eax,0x80000000` sign test falls out of the same
  split.
* **Measured negative: a STORE's SIB base/index order is not reachable from the
  counter's spelling.** The last six mismatches of 0x004227c0 are the six
  `out->tris[n][k]` stores addressed `[edi + ecx]` (reloaded `out->tris` as
  base, stride-12 IV as index) where this build emits `[ecx + edi]`; same
  length, registers and schedule. Every LOAD through a 12-stride IV in the
  function already emits `[ptr + iv]` and matches; every store this build emits
  puts the IV first whichever IV or pointer it is (diagnostics: the primary
  counter `out->tris[i]`, the parameter `m->tris[n]`). Inert at 515: the
  counter's name, scope (block `int t`) and signedness; `for (i = 0, n = 0;)`;
  a `while` form; `((int*)out->tris)[n*3+k]`; `*(int*)((char*)out->tris +
  n*12 + 4k)`; a three-int struct view; two pointer-arithmetic spellings;
  declaration order of `n` and `out`. A user byte offset (`off += 12`) keeps
  the swap and costs 3. A single store after the if/else join (value in `e`,
  or a ternary) is 427/515: VC6 keeps one store with a join, so the original's
  per-arm stores (value in eax in one arm, edi in the other) are the source
  shape.

## What a stronger model could still move

`Mesh_DropBackFaces` 0x004227c0 is at 515/521 aligned with the instruction
count (521) and byte count (1613) both exact. The residual is six SIB bytes:
the triangle-compaction stores take the reloaded `out->tris` as base and the
stride-12 IV as index, this build the reverse. It is an operand-rank decision
inside VC6's store-address formation (loads through the same IV shape already
match), and the spellings listed above under "measured negative" did not move
it. The cross-product block that was this function's headline residual is
closed by the block-split lever and should not be reopened.
