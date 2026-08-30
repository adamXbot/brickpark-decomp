/* LEGOLAND — place an object descriptor onto the map (build-time).
 *
 * WIP: PutObjOnMap is fully reversed and logically complete, but currently at
 * ~79.7% normalized instruction match: the first 102/128 instructions match
 * exactly (the placement callback, the object-type stat switch, GetRectArea,
 * AddObjectsPowerStats). The remaining 26 are a register-allocation divergence
 * in the "ENTRANCE 1" coordinate tail (VC6 keeps the element data pointer in
 * edx and reuses eax for pos->x; our build allocates them the other way). The
 * marker below is intentionally NOT the verify.py form until it reaches 100%. */
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
extern void  MarkObjectTiles(Pos* pos);          /* 0x00489f00 (unconfirmed name) */
extern Elem* ElemID(const char* name);

// WIP-FUNCTION: LEGOLAND 0x00459ad0  (79.7% normalized; tail register alloc)
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
        ElemData* data = ElemID("ENTRANCE 1")->data;
        Cell* cell;
        int px = pos->x;
        if (px < 0 || px >= g_map->width) {
            cell = 0;
        } else {
            int py = pos->y;
            if (py < 0 || py >= g_map->height)
                cell = 0;
            else
                cell = &g_map_rows[py][px];
        }
        g_entrance_x = ((*(unsigned char*)((char*)cell + 4) + data->origin) << 8) - 0x100;
        {
            int b = data->base;
            g_entrance_y = (((data->span - b) << 7) & ~0xFF)
                         + ((*(unsigned char*)((char*)cell + 5) + b) << 8);
        }
    }

    g_map_dirty |= flag;
}
