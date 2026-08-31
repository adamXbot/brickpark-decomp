/* LEGOLAND — path-tile graphics helper + perimeter render-object builder.
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd).
 * Only struct field offsets and callee arg counts are load-bearing. */
#include "legoland.h"

/* ---- perimeter (cliff/bridge) terrain render objects -------------------- */
/* One 20-byte record as stored in the .MAP `n_extra` table. */
typedef struct PerimRec {
    int          x;      /* +0x00 iso screen x */
    int          y;      /* +0x04 iso screen y */
    int          x2;     /* +0x08 */
    int          y2;     /* +0x0c */
    unsigned int image;  /* +0x10 image&0xff = terrain .ILF frame,
                                  (image>>8)&0xff selects the bridge ILF */
} PerimRec;

/* The render object built from it — 0x24 bytes, singly linked. */
typedef struct TerrainObj {
    PerimRec           rec;    /* +0x00 copy of the .MAP record */
    int                sx;     /* +0x14 draw x */
    int                sy;     /* +0x18 draw y */
    struct TerrainObj* next;   /* +0x1c */
    void*              sprite; /* +0x20 bound later by RenderInit (0x462c60) */
} TerrainObj;

/* ---- globals ----------------------------------------------------------- */
extern TerrainObj* g_terrain_objs;     /* 0x00667ca8 list head */
extern void*       g_terrain_data;     /* 0x00667cac terrain element data */
/* Non-zero while the path "ghost"/preview overlay is live; AddPathTileGFX
 * refreshes the drawn square only then. */
extern int g_path_overlay_active;      /* 0x00832984 */

/* ---- callees ----------------------------------------------------------- */
extern void UpdatePathNeighbours(Pos* p);  /* 0x0045d260 */
extern void RefreshPathSquare(Pos* p);     /* 0x0045cd30 */
extern void* HeapAlloc_w(unsigned int size); /* 0x0049e4ff */

// FUNCTION: LEGOLAND 0x0045d350
void AddPathTileGFX(Pos* pos, unsigned short tile)
{
    g_map_rows[pos->y][pos->x].flags |= 0x10;
    g_map_rows[pos->y][pos->x].tile = tile;
    UpdatePathNeighbours(pos);
    if (g_path_overlay_active)
        RefreshPathSquare(pos);
}

/* Build a perimeter cliff/bridge render object from one 20-byte .MAP record
 * and append it to the terrain-object list (head @ 0x00667ca8). Called once
 * per `n_extra` record by LoadBaseMap; RenderInit (0x462c60) later binds each
 * object's sprite. Original name unknown — was sub_462c00. */
// FUNCTION: LEGOLAND 0x00462c00
void BuildPerimeterObject(PerimRec* rec)
{
    TerrainObj* tail = g_terrain_objs;
    TerrainObj* obj;

    while (tail != 0 && tail->next != 0)
        tail = tail->next;

    if (g_terrain_data == 0)
        return;

    obj = (TerrainObj*)HeapAlloc_w(0x24);
    obj->next = 0;
    if (tail != 0)
        tail->next = obj;
    else
        g_terrain_objs = obj;

    obj->sx = rec->x;
    obj->sy = rec->y;
    obj->rec = *rec;
}
