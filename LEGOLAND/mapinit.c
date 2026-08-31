/* LEGOLAND — map init, element lookup, path-square insertion.
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd). Only the
 * struct field offsets and callee arg counts are load-bearing; names are ours.
 */
#include "legoland.h"

/* ---- globals ----------------------------------------------------------- */
extern void* g_castle_obj_elem;   /* 0x0080ff64 — "CASTLE OBJ" LLIDB element */

/* The FX (sound-effect) table InitGameMap registers: 0x17 entries of
 * {char* name, int, void* sample} at 0x004b9228. */
typedef struct FXEntry {
    char* name;     /* +0x00 */
    int   pad4;     /* +0x04 */
    void* sample;   /* +0x08 */
} FXEntry;
extern FXEntry g_game_fx[];       /* 0x004b9228 */

extern const char kCastleObjName[]; /* 0x004b5c0c "CASTLE OBJ" */

/* ---- callees ----------------------------------------------------------- */
extern int   LLIDB_FindElement(const char* name, void** out, unsigned int* outidx); /* 0x47b330 */
extern void  Load_FXList(FXEntry* list, int count);   /* 0x496dd0 */

/* ------------------------------------------------------------------------ */

/* Look up a database element by name. The out-pointer handed to
 * LLIDB_FindElement is the parameter's own slot — that is why the original
 * needs no stack frame and reads the result back out of [esp+0x10]. */
// FUNCTION: LEGOLAND 0x0047b3f0
void* ElemID(const char* name)
{
    LLIDB_FindElement(name, (void**)&name, 0);
    return (void*)name;
}

/* One-time per-level init: resolve the castle object element and load the
 * game's sound-effect table. */
// FUNCTION: LEGOLAND 0x00459850
void InitGameMap(void)
{
    g_castle_obj_elem = ElemID(kCastleObjName);
    Load_FXList(g_game_fx, 0x17);
}

/* ---- path squares ------------------------------------------------------ */

/* A path square: a singly-linked node holding an inclusive rectangle. Only
 * +0x00 (next) and +0x08..+0x14 (l,t,r,b) are touched here; the allocator
 * (0x00481730) makes them 0x24 bytes. */
typedef struct PathSquare {
    struct PathSquare* next;   /* +0x00 */
    int                pad4;   /* +0x04 */
    int                left;   /* +0x08 */
    int                top;    /* +0x0c */
    int                right;  /* +0x10 */
    int                bottom; /* +0x14 */
    char               pad18[0xc]; /* +0x18..0x23 */
} PathSquare;

extern PathSquare* FindPathSquare(Pos* p);   /* 0x481790 */
extern PathSquare* NewPathSquare(void);      /* 0x481730 */
extern void        PathSquareAdded(PathSquare* sq); /* 0x481b10 */

/* Add a 1x1 path square at p, unless one already covers that cell. */
// FUNCTION: LEGOLAND 0x00481c50
void AddPathSquare(Pos* p)
{
    PathSquare* sq;

    if (FindPathSquare(p) == 0) {
        sq = NewPathSquare();
        sq->left = sq->right = p->x;
        sq->top = sq->bottom = p->y;
        PathSquareAdded(sq);
    }
}
