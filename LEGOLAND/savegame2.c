/* LEGOLAND — save/load helpers: the "under construction" block, the path-rect
 * block, the script-string reader and the model recolour applied on load.
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd). Only the
 * struct field offsets, callee argument counts and global addresses are
 * load-bearing; names are ours. Types are defined LOCALLY on purpose (the
 * shared header is owned elsewhere).
 *
 * savegame.c's file header carries the whole .sav layout and calls these;
 * savechunks.c / savechunks2.c hold the neighbouring block serialisers.
 */
#include "legoland.h"

/* ---- save-game primitives (saveprof.c / profiles.c) --------------------- */
extern int  SaveGameRead(void* buf, unsigned int n);         /* 0x0047d730 */
extern int  SaveGameWrite(const void* buf, unsigned int n);  /* 0x0047d760 */
extern int  FindeIneList(void* pval);                        /* 0x0047d880 [sic] */

/* ======================================================================== *
 *  BLK 11 — the "under construction" slots                                 *
 * ======================================================================== */

struct Obj;

/* One construction slot — 12 bytes, 256 of them (the same table buildtick.c,
 * mappath.c and popup.c describe). The whole 0xc00 region is ALSO written raw
 * as part of BLK 9; BLK 11 is the pointer-bearing view of it. */
typedef struct BuildSlot {
    struct Obj* obj;   /* +0x00  object under construction (0 = free slot) */
    short       key;   /* +0x04  map cell it was placed on */
    short       pad6;  /* +0x06 */
    int         timer; /* +0x08  ticks elapsed since construction started */
} BuildSlot;

/* Only the field the save needs: the element the object was built from. */
typedef struct Obj {
    char pad00[0xc4];  /* +0x00 */
    int  elem;         /* +0xc4  LLElem* -> e-list index on the way out */
} Obj;

extern BuildSlot g_build_slots[256];   /* 0x006664f8 */

/* BLK 11 of the .sav: a count of occupied construction slots followed by one
 * 12-byte record each, with the live object pointer replaced by its g_elist
 * index. Renamed from savegame.c's placeholder `SaveBlock11`, the way
 * SaveBlock5..8 became SaveGardeners/SaveMechanics/Save*Orders.
 *
 * Both loops are SUBSCRIPT walks: VC6 strength-reduces `&g_build_slots[i]` and
 * turns `i < 256` into the signed `cmp <cursor>,0x6670f8 / jl` seen here — a
 * pointer walk would compare unsigned (`jb`).
 *
 * As shipped, neither SaveGameWrite result is checked (the neighbouring block
 * writers do check theirs); a failed write is silently ignored. */
// FUNCTION: LEGOLAND 0x00450a80
void SaveBuildSlots(void)
{
    int       n;
    int       i;
    int       elem;
    BuildSlot rec;

    n = 0;
    for (i = 0; i < 256; i++) {
        if (g_build_slots[i].obj)
            n++;
    }
    SaveGameWrite(&n, 4);

    for (i = 0; i < 256; i++) {
        Obj* o = g_build_slots[i].obj;
        if (o) {
            rec = g_build_slots[i];
            elem = o->elem;
            FindeIneList(&elem);
            rec.obj = (Obj*)elem;
            SaveGameWrite(&rec, 0xc);
        }
    }
}

/* ======================================================================== *
 *  Script strings                                                          *
 * ======================================================================== */

/* Statically-linked CRT (>= 0x0049e000): NOT decompilation targets. */
extern void* MemAlloc(unsigned int n);   /* 0x0049e4ff (malloc) */
extern void  free(void* p);              /* 0x0049e4d0 */

/* Bumped by every script serialiser that fails; savechunks2.c's LoadScripts
 * polls it instead of the return value. */
extern int   g_script_errors;            /* 0x006687a0 */

/* The other half of savechunks2.c's string framing: a u32 length followed by
 * that many bytes, NUL-terminated here rather than on disk. A length of -1 is
 * the "no string" marker and is NOT an error — it returns 0 without touching
 * g_script_errors, which is exactly what lets savechunks2.c distinguish an
 * absent name from a failed read.
 *
 * The allocation is not checked, so a failed malloc reads into a null pointer.
 * Reproduced, not fixed. */
// FUNCTION: LEGOLAND 0x0046c680
char* LoadScriptString(void)
{
    int   len;
    char* s;

    if (!SaveGameRead(&len, 4)) {
        g_script_errors++;
        return 0;
    }
    if (len == -1)
        return 0;

    s = (char*)MemAlloc(len + 1);
    if (!SaveGameRead(s, len)) {
        free(s);
        g_script_errors++;
        return 0;
    }
    s[len] = 0;
    return s;
}

/* ======================================================================== *
 *  BLK 10 — the path squares                                               *
 * ======================================================================== */

/* The rectangles paths are made of (pathsq.c's list; head 0x0066b44c). 0x24
 * bytes: the link, an unsaved word, the rect, and the two scalars. */
typedef struct PathSquare {
    struct PathSquare* next;      /* +0x00 */
    int                pad4;      /* +0x04  not saved */
    Rect               rect;      /* +0x08  0x14 bytes, `next` included */
    int                distance2; /* +0x1c */
    int                flags;     /* +0x20 */
} PathSquare;

extern PathSquare* g_path_squares;   /* 0x0066b44c */

/* screens3.c's teardown: frees the whole path-square list and tail-jumps into
 * the rest of the world teardown. Named there, so named that way here. */
extern void sub_4828f0(void);        /* 0x004828f0 */

/* BLK 10 out: a count, then three writes per square. The rect is written with
 * its `next` field still in it (bytes +0x18..+0x1b of the record are a live
 * pointer on disk); LoadPathRects reads it straight back and then overwrites
 * the link, so the file round-trips.
 *
 * The counter is address-taken (SaveGameWrite(&n, 4)), which is why it is
 * incremented through memory in the counting loop instead of a register.
 * Every failure arm shares the ONE inline `return 0` after the first write —
 * VC6 cross-jumps the later ones backwards into it. */
// FUNCTION: LEGOLAND 0x00482860
int SavePathRects(void)
{
    int         n;
    PathSquare* sq;

    n = 0;
    sq = g_path_squares;
    while (sq) {
        n++;
        sq = sq->next;
    }

    if (!SaveGameWrite(&n, 4))
        return 0;

    for (sq = g_path_squares; sq; sq = sq->next) {
        if (!SaveGameWrite(&sq->rect, 0x14))
            return 0;
        if (!SaveGameWrite(&sq->distance2, 4))
            return 0;
        if (!SaveGameWrite(&sq->flags, 4))
            return 0;
    }
    return 1;
}

/* BLK 10 in: drop the current list, then rebuild it by pushing each record on
 * the head — so the list comes back REVERSED relative to the save. Nothing
 * downstream depends on the order.
 *
 * `while (n-- != 0)` on an UNSIGNED count is the mov/dec/test/store/je shape
 * here; the allocation is inline (pathsq.c's NewPathSquare would also zero
 * +0x1c/+0x20) and unchecked, and `rect.next` is restored from the file as a
 * stale pointer before the three reads overwrite nothing of it. */
// FUNCTION: LEGOLAND 0x00482920
int LoadPathRects(void)
{
    unsigned int n;
    PathSquare*  sq;

    sub_4828f0();

    if (!SaveGameRead(&n, 4))
        return 0;

    while (n-- != 0) {
        sq = (PathSquare*)MemAlloc(sizeof(PathSquare));
        sq->next = g_path_squares;
        g_path_squares = sq;
        if (!SaveGameRead(&sq->rect, 0x14))
            return 0;
        if (!SaveGameRead(&sq->distance2, 4))
            return 0;
        if (!SaveGameRead(&sq->flags, 4))
            return 0;
    }
    return 1;
}

/* ======================================================================== *
 *  Model recolouring (savechunks.c's MakeAnimInstance neighbour)           *
 * ======================================================================== */

/* A colour key/replacement as the recolour pass wants it — three bytes in the
 * reverse order of the bloke palette entry (savechunks.c builds them). */
typedef struct Colour3 {
    unsigned char c0;  /* +0x00 */
    unsigned char c1;  /* +0x01 */
    unsigned char c2;  /* +0x02 */
} Colour3;

/* One 36-byte part of a copied 3D model instance. */
typedef struct ModelPart {
    int           flags;   /* +0x00  bit 0x2000 = recolourable */
    unsigned char c0;      /* +0x04 */
    unsigned char c1;      /* +0x05 */
    unsigned char c2;      /* +0x06 */
    unsigned char pad07;   /* +0x07 */
    int           shade;   /* +0x08  shading-ramp handle */
    char          pad0c[0x24 - 0x0c];
} ModelPart;                          /* 0x24 */

/* tri3d.c: build (or find) the shading ramp for one RGB triple. */
extern int MakeShadedColour(int levels, unsigned char* rgb);  /* 0x00486280 */

/* Recolour a freshly copied model instance: every part flagged 0x2000 whose
 * colour equals `k1` is repainted, and its shading ramp rebuilt.
 *
 * ORIGINAL BUG, reproduced: `k2` is never read. The intent was evidently two
 * key/replacement PAIRS (savechunks.c's caller builds key1/repA and
 * key2/repB); what shipped tests `k1` for every part and picks the
 * replacement by the part's position — the first half of the parts gets `r2`,
 * the second half `r1`. With savechunks.c's arguments that means only the
 * 0x56 grey is ever matched and the black key2 does nothing.
 *
 * `rgb = *(int*)r1` reads FOUR bytes out of a three-byte Colour3, so the top
 * byte handed to MakeShadedColour is whatever follows the key in the caller's
 * frame. Also reproduced — MakeShadedColour only reads three of them.
 *
 * The two arms are written out in full: a `rep` pointer local would emit one
 * shared copy of the three byte stores. And the for-increment order `p++, i++`
 * is what puts `add esi,0x24` before `inc edi` in the latch (`i++, p++` swaps
 * them; the initialiser order is inert).
 *
 * savechunks.c declares this with `void* inst`; the ModelPart* here is the
 * same ABI and is left divergent on purpose. */
// FUNCTION: LEGOLAND 0x004424e0
void RecolourModelParts(const Colour3* k1, const Colour3* r1, const Colour3* k2,
                        const Colour3* r2, ModelPart* parts, int n)
{
    int        i;
    int        rgb;
    ModelPart* p;

    for (i = 0, p = parts; i < n; p++, i++) {
        if (p->flags & 0x2000) {
            if (p->c2 == k1->c2 && p->c1 == k1->c1 &&
                p->c0 == k1->c0) {
                if (i >= (n >> 1)) {
                    p->c2 = r1->c2;
                    p->c1 = r1->c1;
                    p->c0 = r1->c0;
                    rgb = *(int*)r1;
                } else {
                    p->c2 = r2->c2;
                    p->c1 = r2->c1;
                    p->c0 = r2->c0;
                    rgb = *(int*)r2;
                }
                p->shade = MakeShadedColour(0x40, (unsigned char*)&rgb);
            }
        }
    }
}
