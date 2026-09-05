/* LEGOLAND -- the smallest SetCustomCallbacks handlers (scope codex-c).
 * Names follow screen.c's class arms and slots: +a4 create, +ac destroy,
 * +94 draw-selection. VC6 SP3 /O2 /Gy /Gd; local types preserve the ABI.
 */
typedef struct Elem { const char* name; void* image; int flags; void* data; } Elem;
extern void* g_jc_water_cls;                                /* 0x0081cb54 */
extern void* g_jc_monkey_tree_sprite;                       /* 0x0081cb68 */
extern void* g_jc_fish_jump;                                /* 0x0081cb6c */
extern int g_power_station_sound_refs;                     /* 0x00667118 */
extern int g_dino_sound_refs;                              /* 0x0066711c */
extern unsigned char g_power_station_fx[];                 /* 0x004b8750 */
extern unsigned char g_dino_fx[];                          /* 0x004b8768 */
extern void KillSprite(void* sprite);                       /* 0x00497bd0 */
extern void BasicObjectDCalcCursor(void* elem, void* pos);   /* 0x00480bb0 */
extern void Kill_FXList(void* list, int count);              /* 0x00496e30 */
extern void Load_FXList(void* list, int count);              /* 0x00496dd0 */

/* JUNGLE CRUISE WATER +a4 caches its object definition. */
// FUNCTION: LEGOLAND 0x00436190
void JungleCruiseWater_Create(Elem* elem)
{
    g_jc_water_cls = elem->data;
}

/* JUNGLE CRUISE MONKEY TREE +ac releases the shared sprite. */
// FUNCTION: LEGOLAND 0x00433cd0
void JungleCruiseMonkeyTree_Destroy(void)
{
    KillSprite(g_jc_monkey_tree_sprite);
}

/* JUNGLE CRUISE MONKEY FISH +ac releases its jump sprite. */
// FUNCTION: LEGOLAND 0x004340b0
void JungleCruiseMonkeyFish_Destroy(void)
{
    KillSprite(g_jc_fish_jump);
}

/* JUNGLE CRUISE MONKEY TREE +94 uses the standard cursor calculation. */
// FUNCTION: LEGOLAND 0x00433fa0
void JungleCruiseMonkeyTree_DrawSelection(void* elem, void* pos)
{
    BasicObjectDCalcCursor(elem, pos);
}

/* JUNGLE CRUISE MONKEY FISH +94 has the same standard cursor calculation. */
// FUNCTION: LEGOLAND 0x00434650
void JungleCruiseMonkeyFish_DrawSelection(void* elem, void* pos)
{
    BasicObjectDCalcCursor(elem, pos);
}

/* Shared power-station +ac: release both sounds on the final reference. */
// FUNCTION: LEGOLAND 0x00452ab0
void PowerStation_AC(void)
{
    if (--g_power_station_sound_refs == 0)
        Kill_FXList(g_power_station_fx, 2);
}

/* Dinosaur class installation: load five sounds on the first reference. */
// FUNCTION: LEGOLAND 0x00452b70
void Dino_InitSound(void)
{
    if (g_dino_sound_refs++ == 0)
        Load_FXList(g_dino_fx, 5);
}
