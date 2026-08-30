/* LEGOLAND — map build-time helpers (called by LoadBaseMap).
 *
 * The build-stat accumulators at 0x00667ce0..0x00667cfc are the running
 * footprint/power tallies that PutObjOnMap advances as each object is placed;
 * LoadBaseMap resets them (via ResetBuildStats) before it walks the .MAP. */
#include "legoland.h"

extern int g_area_total;   /* 0x00667ce0 */
extern int g_area_type1;   /* 0x00667ce4 */
extern int g_area_type4;   /* 0x00667ce8 */
extern int g_area_type5;   /* 0x00667cec */
extern int g_area_type3;   /* 0x00667cf0 */
extern int g_count_env;    /* 0x00667cf4 */
extern int g_area_type2;   /* 0x00667cf8 */
extern int g_stat_cfc;     /* 0x00667cfc */

// FUNCTION: LEGOLAND 0x00459880
void ResetBuildStats(void)
{
    g_area_total = 0;
    g_area_type1 = 0;
    g_area_type4 = 0;
    g_area_type5 = 0;
    g_area_type3 = 0;
    g_count_env = 0;
    g_area_type2 = 0;
    g_stat_cfc = 0;
}

extern void* g_env_class;   /* 0x007fd624  environment class (no footprint) */

/* A placed map instance: its owning ObjClass pointer sits at +0x0c. */
typedef struct MapInst {
    char      pad0[0x0c];   /* +0x00 */
    ObjClass* cls;          /* +0x0c */
} MapInst;

/* Footprint cell tally callback: for one cell, decrement *blocked when the cell
 * is off-map / marked (flags&0x10) / owned by the environment class, and
 * increment *special when it holds a type 2/3 object. */
// FUNCTION: LEGOLAND 0x004598d0
void TallyFootprintCell(Pos* pos, int* blocked, int* special)
{
    int   px = pos->x;
    int   py;
    Cell* cell;

    if (px < 0)                goto fail;
    if (px >= g_map->width)    goto fail;
    py = pos->y;
    if (py < 0)                goto fail;
    if (py >= g_map->height)   goto fail;
    cell = &g_map_rows[py][px];
    if (cell == 0)             goto fail;

    if (cell->flags & 0x10) {
        (*blocked)--;
        return;
    }
    if (cell->flags & 0x80) {
        ObjClass* cls = ((MapInst*)cell->obj)->cls;
        if ((void*)cls == g_env_class) {
            (*blocked)--;
            return;
        }
        if (cls->type == 2 || cls->type == 3)
            (*special)++;
    }
    return;

fail:
    (*blocked)--;
}
