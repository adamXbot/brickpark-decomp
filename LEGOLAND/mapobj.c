/* LEGOLAND — place an object descriptor onto the map (build-time).
 *
 * 100% normalized full-body match (128/128). The "ENTRANCE 1" coordinate tail
 * needed two specific codegen levers to reproduce VC6's register allocation:
 *   (1) fetch the element data via a named `Elem*` intermediate
 *       (`Elem* e = ElemID(...); data = e->data;`) rather than
 *       `ElemID(...)->data` — this stops VC6 reusing the ElemID return register
 *       in place, so `data` lands in edx, `pos->x` in eax, g_map in ecx.
 *   (2) form the cell = 0 / cell = &g_map_rows[py][px] join with explicit
 *       `goto`s (nocell:/have:) so the address `lea` targets the index register
 *       (ecx) and frees eax for the entrance_x accumulator — matching the
 *       original exactly. A plain if/else lets the lea reuse the base register
 *       (eax) instead, which strands 12 instructions in a register swap. */
#include "legoland.h"

/* Build-time accumulators / state (offsets are the load-bearing part). */
extern void* g_placing_obj;    /* 0x0080ff64 */
extern int   g_placed_flag;    /* 0x0079a8d0 */
extern void* g_env_class;      /* 0x007fd624  environment class (no footprint) */
extern int   g_count_env;      /* 0x00667cf4 */
extern int   g_area_total;     /* 0x00667ce0 */
extern int   g_have_special;   /* 0x00667d0c */
extern int   g_area_type1;     /* 0x00667ce4 */
extern int   g_area_type2;     /* 0x00667cf8 */
extern int   g_area_type3;     /* 0x00667cf0 */
extern int   g_area_type4;     /* 0x00667ce8 */
extern int   g_area_type5;     /* 0x00667cec */
extern int   g_entrance_x;     /* 0x004b8320 */
extern int   g_entrance_y;     /* 0x004b8324 */
extern int   g_map_dirty;      /* 0x00668610 */

extern int   GetRectArea(Rect* rect);
extern void  AddObjectsPowerStats(void* obj, Pos* pos);
#ifndef LEGOLAND_PORTABLE
extern void  MarkObjectTiles(Pos* pos);          /* 0x00489f00 (unconfirmed name) */
#else
extern int MarkObjectTiles(Pos* pos);          /* 0x00489f00 (unconfirmed name) */
#endif
extern Elem* ElemID(const char* name);

// FUNCTION: LEGOLAND 0x00459ad0
void PutObjOnMap(ObjClass* cls, void* obj, Pos* pos)
{
    int flag = 1;

    if (obj == g_placing_obj)
        g_placed_flag = 1;

    cls->place(obj, pos);

    if ((void*)cls == g_env_class) {
        g_count_env++;
        g_area_total++;
        g_have_special = 1;
    } else {
        int area = GetRectArea(&cls->rect);
        switch (cls->type - 1) {
        case 0:
            MarkObjectTiles(pos);
            g_area_type1 += area;
            break;
        case 1:
            g_have_special = 1;
            g_area_type2 += area;
            break;
        case 2:
            g_area_type3 += area;
            break;
        case 3:
            MarkObjectTiles(pos);
            g_area_type4 += area;
            break;
        case 4:
            MarkObjectTiles(pos);
            g_area_type5 += area;
            break;
        }
        g_area_total += area;
        AddObjectsPowerStats(obj, pos);
        flag = 1;
    }

    if (obj == (void*)ElemID("ENTRANCE 1")) {
        Elem* e = ElemID("ENTRANCE 1");
        ElemData* data = e->data;
        Cell* cell;
        int px = pos->x;
        if (px < 0 || px >= g_map->width) goto nocell;
        {
            int py = pos->y;
            if (py < 0 || py >= g_map->height) goto nocell;
            cell = &g_map_rows[py][px];
            goto have;
        }
    nocell:
        cell = 0;
    have:
        g_entrance_x = ((*(unsigned char*)((char*)cell + 4) + data->origin) << 8) - 0x100;
        {
            int b = data->base;
            g_entrance_y = (((data->span - b) << 7) & ~0xFF)
                         + ((*(unsigned char*)((char*)cell + 5) + b) << 8);
        }
    }

    g_map_dirty |= flag;
}
